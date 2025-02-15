// This file is complete_def.cc
// It implements the builtins "complete", "compgen", and "compopt" in Bash.

// Copyright (C) 1999-2020 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

// $BUILTIN complete
// $DEPENDS_ON PROGRAMMABLE_COMPLETION
// $FUNCTION complete_builtin
// $SHORT_DOC complete [-abcdefgjksuv] [-pr] [-DEI] [-o option] [-A action] [-G globpat] [-W wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix] [name ...]
// Specify how arguments are to be completed by Readline.

// For each NAME, specify how arguments are to be completed.  If no options
// are supplied, existing completion specifications are printed in a way that
// allows them to be reused as input.

// Options:
//   -p	print existing completion specifications in a reusable format
//   -r	remove a completion specification for each NAME, or, if no
// 		NAMEs are supplied, all completion specifications
//   -D	apply the completions and actions as the default for commands
// 		without any specific completion defined
//   -E	apply the completions and actions to "empty" commands --
// 		completion attempted on a blank line
//   -I	apply the completions and actions to the initial (usually the
// 		command) word

// When completion is attempted, the actions are applied in the order the
// uppercase-letter options are listed above. If multiple options are supplied,
// the -D option takes precedence over -E, and both take precedence over -I.

// Exit Status:
// Returns success unless an invalid option is supplied or an error occurs.
// $END

#include "config.h"

#include "bashtypes.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "builtins.hh"
#include "pcomplete.hh"
#include "shell.hh"

#include "common.hh"

#include "readline.hh"

namespace bash
{

/* Structure containing all the non-action (binary) options; filled in by
   build_actions(). */
struct _optflags
{
  int pflag;
  int rflag;
  int Dflag;
  int Eflag;
  int Iflag;
};

static int find_compact (const char *);
static int find_compopt (const char *);

static int build_actions (WORD_LIST *, struct _optflags *, unsigned long *,
                          unsigned long *);

static int remove_cmd_completions (WORD_LIST *);

static int print_one_completion (const char *, COMPSPEC *);
static int print_compitem (BUCKET_CONTENTS *);
static void print_compopts (const char *, COMPSPEC *, int);
static void print_all_completions (void);
static int print_cmd_completions (WORD_LIST *);

static void print_compoptions (unsigned long, int);
static void print_compactions (unsigned long);
static void print_arg (const char *, const char *, int);
static void print_cmd_name (const char *);

static char *Garg, *Warg, *Parg, *Sarg, *Xarg, *Farg, *Carg;

static const struct _compacts
{
  const char *actname;
  unsigned long actflag;
  int actopt;
} compacts[] = {
  { "alias", CA_ALIAS, 'a' },     { "arrayvar", CA_ARRAYVAR, 0 },
  { "binding", CA_BINDING, 0 },   { "builtin", CA_BUILTIN, 'b' },
  { "command", CA_COMMAND, 'c' }, { "directory", CA_DIRECTORY, 'd' },
  { "disabled", CA_DISABLED, 0 }, { "enabled", CA_ENABLED, 0 },
  { "export", CA_EXPORT, 'e' },   { "file", CA_FILE, 'f' },
  { "function", CA_FUNCTION, 0 }, { "helptopic", CA_HELPTOPIC, 0 },
  { "hostname", CA_HOSTNAME, 0 }, { "group", CA_GROUP, 'g' },
  { "job", CA_JOB, 'j' },         { "keyword", CA_KEYWORD, 'k' },
  { "running", CA_RUNNING, 0 },   { "service", CA_SERVICE, 's' },
  { "setopt", CA_SETOPT, 0 },     { "shopt", CA_SHOPT, 0 },
  { "signal", CA_SIGNAL, 0 },     { "stopped", CA_STOPPED, 0 },
  { "user", CA_USER, 'u' },       { "variable", CA_VARIABLE, 'v' },
  { (char *)NULL, 0, 0 },
};

/* This should be a STRING_INT_ALIST */
static const struct _compopt
{
  const char *optname;
  unsigned long optflag;
} compopts[] = {
  { "bashdefault", COPT_BASHDEFAULT },
  { "default", COPT_DEFAULT },
  { "dirnames", COPT_DIRNAMES },
  { "filenames", COPT_FILENAMES },
  { "noquote", COPT_NOQUOTE },
  { "nosort", COPT_NOSORT },
  { "nospace", COPT_NOSPACE },
  { "plusdirs", COPT_PLUSDIRS },
  { (char *)NULL, 0 },
};

static int
find_compact (const char *name)
{
  for (int i = 0; compacts[i].actname; i++)
    if (STREQ (name, compacts[i].actname))
      return i;
  return -1;
}

static int
find_compopt (const char *name)
{
  for (int i = 0; compopts[i].optname; i++)
    if (STREQ (name, compopts[i].optname))
      return i;
  return -1;
}

/* Build the actions and compspec options from the options specified in LIST.
   ACTP is a pointer to an unsigned long in which to place the bitmap of
   actions.  OPTP is a pointer to an unsigned long in which to place the
   bitmap of compspec options (arguments to `-o').  PP, if non-null, gets 1
   if -p is supplied; RP, if non-null, gets 1 if -r is supplied.
   If either is null, the corresponding option generates an error.
   This also sets variables corresponding to options that take arguments as
   a side effect; the caller should ensure that those variables are set to
   NULL before calling build_actions.  Return value:
        EX_USAGE = bad option
        EXECUTION_SUCCESS = some options supplied
        EXECUTION_FAILURE = no options supplied
*/

int
Shell::build_actions (WORD_LIST *list, struct _optflags *flagp, unsigned long *actp,
               unsigned long *optp)
{
  int opt, ind, opt_given;
  unsigned long acts, copts;
  WORD_DESC w;

  acts = copts = (unsigned long)0L;
  opt_given = 0;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "abcdefgjko:prsuvA:G:W:P:S:X:F:C:DEI"))
         != -1)
    {
      opt_given = 1;
      switch (opt)
        {
        case 'r':
          if (flagp)
            {
              flagp->rflag = 1;
              break;
            }
          else
            {
              sh_invalidopt ("-r");
              builtin_usage ();
              return EX_USAGE;
            }

        case 'p':
          if (flagp)
            {
              flagp->pflag = 1;
              break;
            }
          else
            {
              sh_invalidopt ("-p");
              builtin_usage ();
              return EX_USAGE;
            }

        case 'a':
          acts |= CA_ALIAS;
          break;
        case 'b':
          acts |= CA_BUILTIN;
          break;
        case 'c':
          acts |= CA_COMMAND;
          break;
        case 'd':
          acts |= CA_DIRECTORY;
          break;
        case 'e':
          acts |= CA_EXPORT;
          break;
        case 'f':
          acts |= CA_FILE;
          break;
        case 'g':
          acts |= CA_GROUP;
          break;
        case 'j':
          acts |= CA_JOB;
          break;
        case 'k':
          acts |= CA_KEYWORD;
          break;
        case 's':
          acts |= CA_SERVICE;
          break;
        case 'u':
          acts |= CA_USER;
          break;
        case 'v':
          acts |= CA_VARIABLE;
          break;
        case 'o':
          ind = find_compopt (list_optarg);
          if (ind < 0)
            {
              sh_invalidoptname (list_optarg);
              return EX_USAGE;
            }
          copts |= compopts[ind].optflag;
          break;
        case 'A':
          ind = find_compact (list_optarg);
          if (ind < 0)
            {
              builtin_error (_ ("%s: invalid action name"), list_optarg);
              return EX_USAGE;
            }
          acts |= compacts[ind].actflag;
          break;
        case 'C':
          Carg = list_optarg;
          break;
        case 'D':
          if (flagp)
            {
              flagp->Dflag = 1;
              break;
            }
          else
            {
              sh_invalidopt ("-D");
              builtin_usage ();
              return EX_USAGE;
            }
        case 'E':
          if (flagp)
            {
              flagp->Eflag = 1;
              break;
            }
          else
            {
              sh_invalidopt ("-E");
              builtin_usage ();
              return EX_USAGE;
            }
        case 'I':
          if (flagp)
            {
              flagp->Iflag = 1;
              break;
            }
          else
            {
              sh_invalidopt ("-I");
              builtin_usage ();
              return EX_USAGE;
            }
        case 'F':
          w.word = Farg = list_optarg;
          w.flags = 0;
          if (check_identifier (&w, posixly_correct) == 0
              || strpbrk (Farg, shell_break_chars) != 0)
            {
              sh_invalidid (Farg);
              return EX_USAGE;
            }
          break;
        case 'G':
          Garg = list_optarg;
          break;
        case 'P':
          Parg = list_optarg;
          break;
        case 'S':
          Sarg = list_optarg;
          break;
        case 'W':
          Warg = list_optarg;
          break;
        case 'X':
          Xarg = list_optarg;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }

  *actp = acts;
  *optp = copts;

  return opt_given ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
}

/* Add, remove, and display completion specifiers. */
int
complete_builtin (WORD_LIST *list)
{
  int opt_given, rval;
  unsigned long acts, copts;
  COMPSPEC *cs;
  struct _optflags oflags;
  WORD_LIST *l, *wl;

  if (list == 0)
    {
      print_all_completions ();
      return EXECUTION_SUCCESS;
    }

  opt_given = oflags.pflag = oflags.rflag = 0;
  oflags.Dflag = oflags.Eflag = oflags.Iflag = 0;

  acts = copts = (unsigned long)0L;
  Garg = Warg = Parg = Sarg = Xarg = Farg = Carg = (char *)NULL;
  cs = (COMPSPEC *)NULL;

  /* Build the actions from the arguments.  Also sets the [A-Z]arg variables
     as a side effect if they are supplied as options. */
  rval = build_actions (list, &oflags, &acts, &copts);
  if (rval == EX_USAGE)
    return rval;
  opt_given = rval != EXECUTION_FAILURE;

  list = loptend;

  if (oflags.Dflag)
    wl = make_word_list (make_bare_word (DEFAULTCMD), (WORD_LIST *)NULL);
  else if (oflags.Eflag)
    wl = make_word_list (make_bare_word (EMPTYCMD), (WORD_LIST *)NULL);
  else if (oflags.Iflag)
    wl = make_word_list (make_bare_word (INITIALWORD), (WORD_LIST *)NULL);
  else
    wl = (WORD_LIST *)NULL;

  /* -p overrides everything else */
  if (oflags.pflag || (list == 0 && opt_given == 0))
    {
      if (wl)
        {
          rval = print_cmd_completions (wl);
          dispose_words (wl);
          return rval;
        }
      else if (list == 0)
        {
          print_all_completions ();
          return EXECUTION_SUCCESS;
        }
      return print_cmd_completions (list);
    }

  /* next, -r overrides everything else. */
  if (oflags.rflag)
    {
      if (wl)
        {
          rval = remove_cmd_completions (wl);
          dispose_words (wl);
          return rval;
        }
      else if (list == 0)
        {
          progcomp_flush ();
          return EXECUTION_SUCCESS;
        }
      return remove_cmd_completions (list);
    }

  if (wl == 0 && list == 0 && opt_given)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  /* If we get here, we need to build a compspec and add it for each
     remaining argument. */
  cs = compspec_create ();
  cs->actions = acts;
  cs->options = copts;

  cs->globpat = STRDUP (Garg);
  cs->words = STRDUP (Warg);
  cs->prefix = STRDUP (Parg);
  cs->suffix = STRDUP (Sarg);
  cs->funcname = STRDUP (Farg);
  cs->command = STRDUP (Carg);
  cs->filterpat = STRDUP (Xarg);

  for (rval = EXECUTION_SUCCESS, l = wl ? wl : list; l;
       l = (WORD_LIST *)l->next)
    {
      /* Add CS as the compspec for the specified commands. */
      if (progcomp_insert (l->word->word, cs) == 0)
        rval = EXECUTION_FAILURE;
    }

  dispose_words (wl);
  return rval;
}

static int
remove_cmd_completions (WORD_LIST *list)
{
  WORD_LIST *l;
  int ret;

  for (ret = EXECUTION_SUCCESS, l = list; l; l = (WORD_LIST *)l->next)
    {
      if (progcomp_remove (l->word->word) == 0)
        {
          builtin_error (_ ("%s: no completion specification"), l->word->word);
          ret = EXECUTION_FAILURE;
        }
    }
  return ret;
}

static void
print_compoptions (unsigned long copts, int full)
{
  const struct _compopt *co;

  for (co = compopts; co->optname; co++)
    if (copts & co->optflag)
      printf ("-o %s ", co->optname);
    else if (full)
      printf ("+o %s ", co->optname);
}

static void
print_compactions (unsigned long acts)
{
  const struct _compacts *ca;

  /* simple flags first */
  for (ca = compacts; ca->actname; ca++)
    if (ca->actopt && (acts & ca->actflag))
      printf ("-%c ", ca->actopt);

  /* then the rest of the actions */
  for (ca = compacts; ca->actname; ca++)
    if (ca->actopt == 0 && (acts & ca->actflag))
      printf ("-A %s ", ca->actname);
}

static void
print_arg (const char *arg, const char *flag, int quote)
{
  char *x;

  if (arg)
    {
      x = quote ? sh_single_quote (arg) : (char *)arg;
      printf ("%s %s ", flag, x);
      if (x != arg)
        free (x);
    }
}

static void
print_cmd_name (const char *cmd)
{
  if (STREQ (cmd, DEFAULTCMD))
    printf ("-D");
  else if (STREQ (cmd, EMPTYCMD))
    printf ("-E");
  else if (STREQ (cmd, INITIALWORD))
    printf ("-I");
  else if (*cmd == 0) /* XXX - can this happen? */
    printf ("''");
  else
    printf ("%s", cmd);
}

static int
print_one_completion (const char *cmd, COMPSPEC *cs)
{
  printf ("complete ");

  print_compoptions (cs->options, 0);
  print_compactions (cs->actions);

  /* now the rest of the arguments */

  /* arguments that require quoting */
  print_arg (cs->globpat, "-G", 1);
  print_arg (cs->words, "-W", 1);
  print_arg (cs->prefix, "-P", 1);
  print_arg (cs->suffix, "-S", 1);
  print_arg (cs->filterpat, "-X", 1);

  print_arg (cs->command, "-C", 1);

  /* simple arguments that don't require quoting */
  print_arg (cs->funcname, "-F", 0);

  print_cmd_name (cmd);
  printf ("\n");

  return 0;
}

static void
print_compopts (const char *cmd, COMPSPEC *cs, int full)
{
  printf ("compopt ");

  print_compoptions (cs->options, full);
  print_cmd_name (cmd);

  printf ("\n");
}

static int
print_compitem (BUCKET_CONTENTS *item)
{
  COMPSPEC *cs;
  char *cmd;

  cmd = item->key;
  cs = (COMPSPEC *)item->data;

  return print_one_completion (cmd, cs);
}

static void
print_all_completions ()
{
  progcomp_walk (print_compitem);
}

static int
print_cmd_completions (WORD_LIST *list)
{
  WORD_LIST *l;
  COMPSPEC *cs;
  int ret;

  for (ret = EXECUTION_SUCCESS, l = list; l; l = (WORD_LIST *)l->next)
    {
      cs = progcomp_search (l->word->word);
      if (cs)
        print_one_completion (l->word->word, cs);
      else
        {
          builtin_error (_ ("%s: no completion specification"), l->word->word);
          ret = EXECUTION_FAILURE;
        }
    }

  return sh_chkwrite (ret);
}

// $BUILTIN compgen
// $DEPENDS_ON PROGRAMMABLE_COMPLETION
// $FUNCTION compgen_builtin
// $SHORT_DOC compgen [-abcdefgjksuv] [-o option] [-A action] [-G globpat] [-W
// wordlist] [-F function] [-C command] [-X filterpat] [-P prefix] [-S suffix]
// [word] Display possible completions depending on the options.

// Intended to be used from within a shell function generating possible
// completions.  If the optional WORD argument is supplied, matches against
// WORD are generated.

// Exit Status:
// Returns success unless an invalid option is supplied or an error occurs.
// $END

int
Shell::compgen_builtin (WORD_LIST *list)
{
  int rval;
  unsigned long acts, copts;
  COMPSPEC *cs;
  STRINGLIST *sl;
  char **matches;
  char *old_line;
  int old_ind;

  if (list == 0)
    return EXECUTION_SUCCESS;

  acts = copts = (unsigned long)0L;
  Garg = Warg = Parg = Sarg = Xarg = Farg = Carg = (char *)NULL;
  cs = (COMPSPEC *)NULL;

  /* Build the actions from the arguments.  Also sets the [A-Z]arg variables
     as a side effect if they are supplied as options. */
  rval = build_actions (list, (struct _optflags *)NULL, &acts, &copts);
  if (rval == EX_USAGE)
    return rval;
  if (rval == EXECUTION_FAILURE)
    return EXECUTION_SUCCESS;

  list = loptend;

  const char *word = (list && list->word) ? list->word->word : "";

  if (Farg)
    builtin_error (_ ("warning: -F option may not work as you expect"));
  if (Carg)
    builtin_error (_ ("warning: -C option may not work as you expect"));

  /* If we get here, we need to build a compspec and evaluate it. */
  cs = compspec_create ();
  cs->actions = acts;
  cs->options = copts;
  cs->refcount = 1;

  cs->globpat = STRDUP (Garg);
  cs->words = STRDUP (Warg);
  cs->prefix = STRDUP (Parg);
  cs->suffix = STRDUP (Sarg);
  cs->funcname = STRDUP (Farg);
  cs->command = STRDUP (Carg);
  cs->filterpat = STRDUP (Xarg);

  rval = EXECUTION_FAILURE;

  /* probably don't have to save these, just being safe */
  old_line = pcomp_line;
  old_ind = pcomp_ind;
  pcomp_line = (char *)NULL;
  pcomp_ind = 0;
  sl = gen_compspec_completions (cs, "compgen", word, 0, 0, 0);
  pcomp_line = old_line;
  pcomp_ind = old_ind;

  /* If the compspec wants the bash default completions, temporarily
     turn off programmable completion and call the bash completion code. */
  if ((sl == 0 || sl->list_len == 0) && (copts & COPT_BASHDEFAULT))
    {
      matches = bash_default_completion (word, 0, 0, 0, 0);
      sl = completions_to_stringlist (matches);
      strvec_dispose (matches);
    }

  /* This isn't perfect, but it's the best we can do, given what readline
     exports from its set of completion utility functions. */
  if ((sl == 0 || sl->list_len == 0) && (copts & COPT_DEFAULT))
    {
      matches = rl_completion_matches (word, rl_filename_completion_function);
      strlist_dispose (sl);
      sl = completions_to_stringlist (matches);
      strvec_dispose (matches);
    }

  if (sl)
    {
      if (sl->list && sl->list_len)
        {
          rval = EXECUTION_SUCCESS;
          strlist_print (sl, (char *)NULL);
        }
      strlist_dispose (sl);
    }

  compspec_dispose (cs);
  return rval;
}

// $BUILTIN compopt
// $DEPENDS_ON PROGRAMMABLE_COMPLETION
// $FUNCTION compopt_builtin
// $SHORT_DOC compopt [-o|+o option] [-DEI] [name ...]
// Modify or display completion options.

// Modify the completion options for each NAME, or, if no NAMEs are supplied,
// the completion currently being executed.  If no OPTIONs are given, print
// the completion options for each NAME or the current completion
// specification.

// Options:
// 	-o option	Set completion option OPTION for each NAME
// 	-D		Change options for the "default" command completion
// 	-E		Change options for the "empty" command completion
// 	-I		Change options for completion on the initial word

// Using `+o' instead of `-o' turns off the specified option.

// Arguments:

// Each NAME refers to a command for which a completion specification must
// have previously been defined using the `complete' builtin.  If no NAMEs
// are supplied, compopt must be called by a function currently generating
// completions, and the options for that currently-executing completion
// generator are modified.

// Exit Status:
// Returns success unless an invalid option is supplied or NAME does not
// have a completion specification defined.
// $END

int
Shell::compopt_builtin (WORD_LIST *list)
{
  int opts_on, opts_off, *opts, opt, oind, ret, Dflag, Eflag, Iflag;
  WORD_LIST *l, *wl;
  COMPSPEC *cs;

  opts_on = opts_off = Eflag = Dflag = Iflag = 0;
  ret = EXECUTION_SUCCESS;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "+o:DEI")) != -1)
    {
      opts = (list_opttype == '-') ? &opts_on : &opts_off;

      switch (opt)
        {
        case 'o':
          oind = find_compopt (list_optarg);
          if (oind < 0)
            {
              sh_invalidoptname (list_optarg);
              return EX_USAGE;
            }
          *opts |= compopts[oind].optflag;
          break;
        case 'D':
          Dflag = 1;
          break;
        case 'E':
          Eflag = 1;
          break;
        case 'I':
          Iflag = 1;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  if (Dflag)
    wl = make_word_list (make_bare_word (DEFAULTCMD), (WORD_LIST *)NULL);
  else if (Eflag)
    wl = make_word_list (make_bare_word (EMPTYCMD), (WORD_LIST *)NULL);
  else if (Iflag)
    wl = make_word_list (make_bare_word (INITIALWORD), (WORD_LIST *)NULL);
  else
    wl = (WORD_LIST *)NULL;

  if (list == 0 && wl == 0)
    {
      if (RL_ISSTATE (RL_STATE_COMPLETING) == 0 || pcomp_curcs == 0)
        {
          builtin_error (_ ("not currently executing completion function"));
          return EXECUTION_FAILURE;
        }
      cs = pcomp_curcs;

      if (opts_on == 0 && opts_off == 0)
        {
          print_compopts (pcomp_curcmd, cs, 1);
          return sh_chkwrite (ret);
        }

      /* Set the compspec options */
      pcomp_set_compspec_options (cs, opts_on, 1);
      pcomp_set_compspec_options (cs, opts_off, 0);

      /* And change the readline variables the options control */
      pcomp_set_readline_variables (opts_on, 1);
      pcomp_set_readline_variables (opts_off, 0);

      return ret;
    }

  for (l = wl ? wl : list; l; l = (WORD_LIST *)l->next)
    {
      cs = progcomp_search (l->word->word);
      if (cs == 0)
        {
          builtin_error (_ ("%s: no completion specification"), l->word->word);
          ret = EXECUTION_FAILURE;
          continue;
        }
      if (opts_on == 0 && opts_off == 0)
        {
          print_compopts (l->word->word, cs, 1);
          continue; /* XXX -- fill in later */
        }

      /* Set the compspec options */
      pcomp_set_compspec_options (cs, opts_on, 1);
      pcomp_set_compspec_options (cs, opts_off, 0);
    }

  if (wl)
    dispose_words (wl);

  return ret;
}

} // namespace bash
