// This file is fc.def, from which is created fc.c.
// It implements the builtin "fc" in Bash.

// Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

// $PRODUCES fc.c

// $BUILTIN fc
// $FUNCTION fc_builtin
// $DEPENDS_ON HISTORY
// $SHORT_DOC fc [-e ename] [-lnr] [first] [last] or fc -s [pat=rep] [command]
// Display or execute commands from the history list.

// fc is used to list or edit and re-execute commands from the history list.
// FIRST and LAST can be numbers specifying the range, or FIRST can be a
// string, which means the most recent command beginning with that
// string.

// Options:
//   -e ENAME	select which editor to use.  Default is FCEDIT, then EDITOR,
// 		then vi
//   -l 	list lines instead of editing
//   -n	omit line numbers when listing
//   -r	reverse the order of the lines (newest listed first)

// With the `fc -s [pat=rep ...] [command]' format, COMMAND is
// re-executed after the substitution OLD=NEW is performed.

// A useful alias to use with this is r='fc -s', so that typing `r cc'
// runs the last command beginning with `cc' and typing `r' re-executes
// the last command.

// Exit Status:
// Returns success or status of executed command; non-zero if an error occurs.
// $END

#include "config.hh"

#if defined (HISTORY)

#if defined (HAVE_SYS_PARAM_H)
#  include <sys/param.h>
#endif

#include "bashtypes.hh"
#include "posixstat.hh"

#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "chartypes.hh"
#include "bashintl.hh"

#include "shell.hh"
#include "builtins.hh"
#include "flags.hh"
#include "parser.hh"
#include "maxpath.hh"
#include "history.hh"
#include "bashgetopt.hh"
#include "common.hh"

namespace bash
{

#define HIST_INVALID INT_MIN
#define HIST_ERANGE  INT_MIN+1
#define HIST_NOTFOUND INT_MIN+2

/* Values for the flags argument to fc_gethnum */
#define HN_LISTING	0x01
#define HN_FIRST	0x02

extern int unlink (const char *);

extern FILE *sh_mktmpfp (const char *, int, char **);

extern bool suppress_debug_trap_verbose;

/* **************************************************************** */
/*								    */
/*	The K*rn shell style fc command (Fix Command)		    */
/*								    */
/* **************************************************************** */

/* fc builtin command (fix command) for Bash for those who
   like K*rn-style history better than csh-style.

     fc [-e ename] [-nlr] [first] [last]

   FIRST and LAST can be numbers specifying the range, or FIRST can be
   a string, which means the most recent command beginning with that
   string.

   -e ENAME selects which editor to use.  Default is FCEDIT, then EDITOR,
      then the editor which corresponds to the current readline editing
      mode, then vi.

   -l means list lines instead of editing.
   -n means no line numbers listed.
   -r means reverse the order of the lines (making it newest listed first).

     fc -e - [pat=rep ...] [command]
     fc -s [pat=rep ...] [command]

   Equivalent to !command:sg/pat/rep execpt there can be multiple PAT=REP's.
*/

/* Data structure describing a list of global replacements to perform. */
typedef struct repl {
  struct repl *next;
  char *pat;
  char *rep;
} REPL;

/* Accessors for HIST_ENTRY lists that are called HLIST. */
#define histline(i) (hlist[(i)]->line)
#define histdata(i) (hlist[(i)]->data)

#define FREE_RLIST() \
	do { \
		for (rl = rlist; rl; ) { \
			REPL *r;	\
			r = rl->next; \
			if (rl->pat) \
				free (rl->pat); \
			if (rl->rep) \
				free (rl->rep); \
			free (rl); \
			rl = r; \
		} \
	} while (0)

static char *fc_dosubs (char *, REPL *);
static char *fc_gethist (char *, HIST_ENTRY **, int);
static int fc_gethnum (char *, HIST_ENTRY **, int);
static int fc_number (WORD_LIST *);
static void fc_replhist (char *);

static void
Shell::set_verbose_flag ()
{
  echo_input_at_read = verbose_flag;
}

/* String to execute on a file that we want to edit. */
#define FC_EDIT_COMMAND "${FCEDIT:-${EDITOR:-vi}}"
#if defined (STRICT_POSIX)
#  define POSIX_FC_EDIT_COMMAND "${FCEDIT:-ed}"
#else
#  define POSIX_FC_EDIT_COMMAND "${FCEDIT:-${EDITOR:-ed}}"
#endif

static void
unlink_and_free (void *arg)
{
  char *fn = (char *)arg;
  unlink(fn);
  xfree(fn);
}

int
Shell::fc_builtin (WORD_LIST *list)
{
  int numbering, reverse, listing, execute;

  numbering = 1;
  reverse = listing = execute = 0;
  char *ename = (char *)NULL;

  /* Parse out the options and set which of the two forms we're in. */
  reset_internal_getopt ();
  lcurrent = list;		/* XXX */
  int opt;
  while (fc_number (loptend = lcurrent) == 0 &&
	 (opt = internal_getopt (list, ":e:lnrs")) != -1)
    {
      switch (opt)
	{
	case 'n':
	  numbering = 0;
	  break;

	case 'l':
	  listing = HN_LISTING;		/* for fc_gethnum */
	  break;

	case 'r':
	  reverse = 1;
	  break;

	case 's':
	  execute = 1;
	  break;

	case 'e':
	  ename = list_optarg;
	  break;

	CASE_HELPOPT;
	default:
	  builtin_usage ();
	  return EX_USAGE;
	}
    }

  list = loptend;

  if (ename && (*ename == '-') && (ename[1] == '\0'))
    execute = 1;

  /* The "execute" form of the command (re-run, with possible string
     substitutions). */
  if (execute)
    {
      REPL *rlist = (REPL *)NULL;
      REPL *rl;
      char *sep;
      while (list && ((sep = (char *)strchr (list->word->word, '=')) != NULL))
	{
	  *sep++ = '\0';
	  rl = (REPL *)xmalloc (sizeof (REPL));
	  rl->next = (REPL *)NULL;
	  rl->pat = savestring (list->word->word);
	  rl->rep = savestring (sep);

	  if (rlist == NULL)
	    rlist = rl;
	  else
	    {
	      rl->next = rlist;
	      rlist = rl;
	    }
	  list = (WORD_LIST *)list->next;
	}

      /* If we have a list of substitutions to do, then reverse it
	 to get the replacements in the proper order. */

      rlist = REVERSE_LIST (rlist, REPL *);

      HIST_ENTRY **hlist = history_list ();

      /* If we still have something in list, it is a command spec.
	 Otherwise, we use the most recent command in time. */
      char *command = fc_gethist (list ? list->word->word : (char *)NULL, hlist, 0);

      if (command == NULL)
	{
	  builtin_error (_("no command found"));
	  if (rlist)
	    FREE_RLIST ();

	  return EXECUTION_FAILURE;
	}

      if (rlist)
	{
	  char *newcom = fc_dosubs (command, rlist);
	  free (command);
	  FREE_RLIST ();
	  command = newcom;
	}

      fprintf (stderr, "%s\n", command);
      fc_replhist (command);	/* replace `fc -s' with command */
      /* Posix says that the re-executed commands should be entered into the
	 history. */
      return parse_and_execute (command, "fc", SEVAL_NOHIST);
    }

  /* This is the second form of the command (the list-or-edit-and-rerun
     form). */
  HIST_ENTRY **hlist = history_list ();
  if (hlist == 0)
    return EXECUTION_SUCCESS;

  int i;
  for (i = 0; hlist[i]; i++);

  /* With the Bash implementation of history, the current command line
     ("fc blah..." and so on) is already part of the history list by
     the time we get to this point.  This just skips over that command
     and makes the last command that this deals with be the last command
     the user entered before the fc.  We need to check whether the
     line was actually added (HISTIGNORE may have caused it to not be),
     so we check hist_last_line_added. */

  /* Even though command substitution through parse_and_execute turns off
     remember_on_history, command substitution in a shell when set -o history
     has been enabled (interactive or not) should use it in the last_hist
     calculation as if it were on. */
  int rh = remember_on_history || ((subshell_environment & SUBSHELL_COMSUB) && enable_history_list);
  int last_hist = i - rh - hist_last_line_added;

  /* Make sure that real_last is calculated the same way here and in
     fc_gethnum.  The return value from fc_gethnum is treated specially if
     it is == real_last and we are listing commands. */
  int real_last = i;
  /* back up from the end to the last non-null history entry */
  while (hlist[real_last] == 0 && real_last > 0)
    real_last--;

  /* XXX */
  if (i == last_hist && hlist[last_hist] == 0)
    while (last_hist >= 0 && hlist[last_hist] == 0)
      last_hist--;
  if (last_hist < 0)
    last_hist = 0;		/* per POSIX */

  int histbeg, histend;
  if (list)
    {
      histbeg = fc_gethnum (list->word->word, hlist, listing|HN_FIRST);
      list = (WORD_LIST *)list->next;

      if (list)
	histend = fc_gethnum (list->word->word, hlist, listing);
      else if (histbeg == real_last)
	histend = listing ? real_last : histbeg;
      else
	histend = listing ? last_hist : histbeg;
    }
  else
    {
      /* The default for listing is the last 16 history items. */
      if (listing)
	{
	  histend = last_hist;
	  histbeg = histend - 16 + 1;	/* +1 because loop below uses >= */
	  if (histbeg < 0)
	    histbeg = 0;
	}
      else
	/* For editing, it is the last history command. */
	histbeg = histend = last_hist;
    }

  if (histbeg == HIST_INVALID || histend == HIST_INVALID)
    {
      sh_erange ((char *)NULL, _("history specification"));
      return EXECUTION_FAILURE;
    }
  else if (histbeg == HIST_ERANGE || histend == HIST_ERANGE)
    {
      sh_erange ((char *)NULL, _("history specification"));
      return EXECUTION_FAILURE;
    }
  else if (histbeg == HIST_NOTFOUND || histend == HIST_NOTFOUND)
    {
      builtin_error (_("no command found"));
      return EXECUTION_FAILURE;
    }

  /* We don't throw an error for line specifications out of range, per POSIX */
  if (histbeg < 0)
    histbeg = 0;
  if (histend < 0)
    histend = 0;

  /* "When not listing, the fc command that caused the editing shall not be
     entered into the history list." */
  if (listing == 0 && hist_last_line_added)
    {
      bash_delete_last_history ();
      /* If we're editing a single command -- the last command in the
	 history -- and we just removed the dummy command added by
	 edit_and_execute_command (), we need to check whether or not we
	 just removed the last command in the history and need to back
	 the pointer up.  remember_on_history is off because we're running
	 in parse_and_execute(). */
      if (histbeg == histend && histend == last_hist && hlist[last_hist] == 0)
	last_hist = histbeg = --histend;

      if (hlist[last_hist] == 0)
	last_hist--;
      if (histend >= last_hist)
	histend = last_hist;
      else if (histbeg >= last_hist)
	histbeg = last_hist;
    }

  if (histbeg == HIST_INVALID || histend == HIST_INVALID)
    {
      sh_erange ((char *)NULL, _("history specification"));
      return EXECUTION_FAILURE;
    }
  else if (histbeg == HIST_ERANGE || histend == HIST_ERANGE)
    {
      sh_erange ((char *)NULL, _("history specification"));
      return EXECUTION_FAILURE;
    }
  else if (histbeg == HIST_NOTFOUND || histend == HIST_NOTFOUND)
    {
      builtin_error (_("no command found"));
      return EXECUTION_FAILURE;
    }

  /* We don't throw an error for line specifications out of range, per POSIX */
  if (histbeg < 0)
    histbeg = 0;
  if (histend < 0)
    histend = 0;

  if (histend < histbeg)
    {
      i = histend;
      histend = histbeg;
      histbeg = i;

      reverse = 1;
    }

  FILE *stream;
  char *fn;
  if (listing)
    stream = stdout;
  else
    {
      numbering = 0;
      stream = sh_mktmpfp ("bash-fc", MT_USERANDOM|MT_USETMPDIR, &fn);
      if (stream == 0)
	{
	  builtin_error (_("%s: cannot open temp file: %s"), fn ? fn : "", strerror (errno));
	  FREE (fn);
	  return EXECUTION_FAILURE;
	}
    }

  for (i = reverse ? histend : histbeg; reverse ? i >= histbeg : i <= histend; reverse ? i-- : i++)
    {
      QUIT;
      if (numbering)
	fprintf (stream, "%d", i + history_base);
      if (listing)
	{
	  if (posixly_correct)
	    fputs ("\t", stream);
	  else
	    fprintf (stream, "\t%c", histdata (i) ? '*' : ' ');
	}
      fprintf (stream, "%s\n", histline (i));
    }

  if (listing)
    return sh_chkwrite (EXECUTION_SUCCESS);

  fflush (stream);
  if (ferror (stream))
    {
      sh_wrerror ();
      fclose (stream);
      FREE (fn);
      return EXECUTION_FAILURE;
    }
  fclose (stream);

  /* Now edit the file of commands. */
  char *command;
  if (ename)
    {
      command = (char *)xmalloc (strlen (ename) + strlen (fn) + 2);
      sprintf (command, "%s %s", ename, fn);
    }
  else
    {
      const char *fcedit = posixly_correct ? POSIX_FC_EDIT_COMMAND : FC_EDIT_COMMAND;
      command = (char *)xmalloc (3 + strlen (fcedit) + strlen (fn));
      sprintf (command, "%s %s", fcedit, fn);
    }
  int retval = parse_and_execute (command, "fc", SEVAL_NOHIST);
  if (retval != EXECUTION_SUCCESS)
    {
      unlink (fn);
      free (fn);
      return EXECUTION_FAILURE;
    }

#if defined (READLINE)
  /* If we're executing as part of a dispatched readline command like
     {emacs,vi}_edit_and_execute_command, the readline state will indicate it.
     We could remove the partial command from the history, but ksh93 doesn't
     so we stay compatible. */
#endif

  /* Make sure parse_and_execute doesn't turn this off, even though a
     call to parse_and_execute farther up the function call stack (e.g.,
     if this is called by vi_edit_and_execute_command) may have already
     called bash_history_disable. */
  remember_on_history = true;

  /* Turn on the `v' flag while fc_execute_file runs so the commands
     will be echoed as they are read by the parser. */
  begin_unwind_frame ("fc builtin");
  add_unwind_protect_ptr (unlink_and_free, fn);
  add_unwind_protect (set_verbose_flag);
  unwind_protect_var (suppress_debug_trap_verbose);
  echo_input_at_read = true;
  suppress_debug_trap_verbose = true;

  retval = fc_execute_file (fn);
  run_unwind_frame ("fc builtin");

  return retval;
}

/* Return 1 if LIST->word->word is a legal number for fc's use. */
static int
fc_number (WORD_LIST *list)
{
  char *s;

  if (list == 0)
    return 0;
  s = list->word->word;
  if (*s == '-')
    s++;
  return legal_number (s, nullptr);
}

/* Return an absolute index into HLIST which corresponds to COMMAND.  If
   COMMAND is a number, then it was specified in relative terms.  If it
   is a string, then it is the start of a command line present in HLIST.
   MODE includes HN_LISTING if we are listing commands, and does not if we
   are executing them. If MODE includes HN_FIRST we are looking for the
   first history number specification. */
static int
fc_gethnum (char *command, HIST_ENTRY **hlist, int mode)
{
  int listing = mode & HN_LISTING;
  int sign = 1;
  /* Count history elements. */
  int i;
  for (i = 0; hlist[i]; i++);

  /* With the Bash implementation of history, the current command line
     ("fc blah..." and so on) is already part of the history list by
     the time we get to this point.  This just skips over that command
     and makes the last command that this deals with be the last command
     the user entered before the fc.  We need to check whether the
     line was actually added (HISTIGNORE may have caused it to not be),
     so we check hist_last_line_added.  This needs to agree with the
     calculation of last_hist in fc_builtin above. */
  /* Even though command substitution through parse_and_execute turns off
     remember_on_history, command substitution in a shell when set -o history
     has been enabled (interactive or not) should use it in the last_hist
     calculation as if it were on. */
  int rh = remember_on_history || ((subshell_environment & SUBSHELL_COMSUB) && enable_history_list);
  int last_hist = i - rh - hist_last_line_added;

  if (i == last_hist && hlist[last_hist] == 0)
    while (last_hist >= 0 && hlist[last_hist] == 0)
      last_hist--;
  if (last_hist < 0)
    return -1;

  int real_last = i;
  i = last_hist;

  /* No specification defaults to most recent command. */
  if (command == NULL)
    return i;

  /* back up from the end to the last non-null history entry */
  while (hlist[real_last] == 0 && real_last > 0)
    real_last--;

  /* Otherwise, there is a specification.  It can be a number relative to
     the current position, or an absolute history number. */
  char *s = command;

  /* Handle possible leading minus sign. */
  if (s && (*s == '-'))
    {
      sign = -1;
      s++;
    }

  if (s && DIGIT(*s))
    {
      int n = atoi (s);
      n *= sign;

      /* We want to return something that is an offset to HISTORY_BASE. */

      /* If the value is negative or zero, then it is an offset from
	 the current history item. */
      /* We don't use HN_FIRST here, so we don't return different values
	 depending on whether we're looking for the first or last in a
	 pair of range arguments, but nobody else does, either. */
      if (n < 0)
	{
	  n += i + 1;
	  return n < 0 ? 0 : n;
	}
      else if (n == 0)
	return (sign == -1) ? (listing ? real_last : HIST_INVALID) : i;
      else
	{
	  /* If we're out of range (greater than I (last history entry) or
	     less than HISTORY_BASE, we want to return different values
	     based on whether or not we are looking for the first or last
	     value in a desired range of history entries. */
	  n -= history_base;
	  if (n < 0)
	    return mode & HN_FIRST ? 0 : i;
	  else if (n >= i)
	    return mode & HN_FIRST ? 0 : i;
	  else
	    return n;
	}
    }

  int clen = strlen (command);
  for (int j = i; j >= 0; j--)
    {
      if (STREQN (command, histline (j), clen))
	return j;
    }
  return HIST_NOTFOUND;
}

/* Locate the most recent history line which begins with
   COMMAND in HLIST, and return a malloc()'ed copy of it.
   MODE is 1 if we are listing commands, 0 if we are executing them. */
static char *
fc_gethist (char *command, HIST_ENTRY **hlist, int mode)
{
  if (hlist == 0)
    return (char *)NULL;

  int i = fc_gethnum (command, hlist, mode);

  if (i >= 0)
    return savestring (histline (i));
  else
    return (char *)NULL;
}

/* Perform the SUBS on COMMAND.
   SUBS is a list of substitutions, and COMMAND is a simple string.
   Return a pointer to a malloc'ed string which contains the substituted
   command. */
static char *
fc_dosubs (char *command, REPL *subs)
{
  char *new_;
  REPL *r;

  for (new_ = savestring (command), r = subs; r; r = r->next)
    {
      char *t = strsub (new_, r->pat, r->rep, 1);
      free (new_);
      new_ = t;
    }
  return new_;
}

/* Use `command' to replace the last entry in the history list, which,
   by this time, is `fc blah...'.  The intent is that the new command
   become the history entry, and that `fc' should never appear in the
   history list.  This way you can do `r' to your heart's content. */
static void
fc_replhist (char *command)
{
  if (command == 0 || *command == '\0')
    return;

  int n = strlen (command);
  if (command[n - 1] == '\n')
    command[n - 1] = '\0';

  if (command && *command)
    {
      bash_delete_last_history ();
      maybe_add_history (command);	/* Obeys HISTCONTROL setting. */
    }
}

}  // namespace bash

#endif /* HISTORY */
