/* common.cc - utility functions for all builtins */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "shell.hh"

#include "builtext.hh"

namespace bash
{

/* A standard error message to use when getcwd() returns nullptr. */
const char *const bash_getcwd_errstr
    = N_ ("getcwd: cannot access parent directories");

/* Used by some builtins and the mainline code. */
// sh_builtin_func_t *last_shell_builtin = (sh_builtin_func_t *)NULL;
// sh_builtin_func_t *this_shell_builtin = (sh_builtin_func_t *)NULL;

/* **************************************************************** */
/*								    */
/*	     Error reporting, usage, and option processing	    */
/*								    */
/* **************************************************************** */

// This is a lot like report_error (), but it is for shell builtins
// instead of shell control structures, and it won't ever exit the shell.
void
Shell::builtin_error_prolog ()
{
  const char *name;

  name = get_name_for_error ();
  fprintf (stderr, "%s: ", name);

  if (!interactive_shell)
    fprintf (stderr, _ ("line %d: "), executing_line_number ());

  if (!this_command_name.empty ())
    fprintf (stderr, "%s: ", to_string (this_command_name).c_str ());
}

void
Shell::builtin_error (const char *format, ...)
{
  va_list args;

  builtin_error_prolog ();

  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
}

void
Shell::builtin_warning (const char *format, ...)
{
  va_list args;

  builtin_error_prolog ();
  fprintf (stderr, _ ("warning: "));

  va_start (args, format);
  vfprintf (stderr, format, args);
  va_end (args);
  fprintf (stderr, "\n");
}

/* Print a usage summary for the currently-executing builtin command. */
void
Shell::builtin_usage ()
{
  if (!this_command_name.empty ())
    fprintf (stderr, _ ("%s: usage: "),
             to_string (this_command_name).c_str ());

  fprintf (stderr, "%s\n", _ (current_builtin->short_doc));
  fflush (stderr);
}

/* Return if LIST is NULL else barf and jump to top_level.  Used by some
   builtins that do not accept arguments. */
void
Shell::no_args (WORD_LIST *list)
{
  if (list)
    {
      builtin_error (_ ("too many arguments"));
      top_level_cleanup ();
      throw bash_exception (DISCARD);
    }
}

/* Check that no options were given to the currently-executing builtin,
   and return 0 if there were options. */
int
Shell::no_options (WORD_LIST *list)
{
  int opt;

  reset_internal_getopt ();
  if ((opt = internal_getopt (list, "")) != -1)
    {
      if (opt == GETOPT_HELP)
        {
          builtin_help ();
          return 2;
        }
      builtin_usage ();
      return 1;
    }
  return 0;
}

void
Shell::sh_needarg (const char *s)
{
  builtin_error (_ ("%s: option requires an argument"), s);
}

void
Shell::sh_neednumarg (const char *s)
{
  builtin_error (_ ("%s: numeric argument required"), s);
}

void
Shell::sh_notfound (const char *s)
{
  builtin_error (_ ("%s: not found"), s);
}

/* Function called when one of the builtin commands detects an invalid
   option. */
void
Shell::sh_invalidopt (const char *s)
{
  builtin_error (_ ("%s: invalid option"), s);
}

void
Shell::sh_invalidoptname (const char *s)
{
  builtin_error (_ ("%s: invalid option name"), s);
}

void
Shell::sh_invalidid (const char *s)
{
  builtin_error (_ ("`%s': not a valid identifier"), s);
}

void
Shell::sh_invalidnum (const char *s)
{
  const char *msg;

  if (*s == '0' && c_isdigit (s[1]))
    msg = _ ("invalid octal number");
  else if (*s == '0' && s[1] == 'x')
    msg = _ ("invalid hex number");
  else
    msg = _ ("invalid number");
  builtin_error ("%s: %s", s, msg);
}

void
Shell::sh_invalidsig (const char *s)
{
  builtin_error (_ ("%s: invalid signal specification"), s);
}

void
Shell::sh_badpid (const char *s)
{
  builtin_error (_ ("`%s': not a pid or valid job spec"), s);
}

void
Shell::sh_readonly (const char *s)
{
  builtin_error (_ ("%s: readonly variable"), s);
}

void
Shell::sh_noassign (const char *s)
{
  internal_error (_ ("%s: cannot assign"), s); /* XXX */
}

void
Shell::sh_erange (const char *s, const char *desc)
{
  if (s)
    builtin_error (_ ("%s: %s out of range"), s, desc ? desc : _ ("argument"));
  else
    builtin_error (_ ("%s out of range"), desc ? desc : _ ("argument"));
}

#if defined(JOB_CONTROL)
void
Shell::sh_badjob (const char *s)
{
  builtin_error (_ ("%s: no such job"), s);
}

void
Shell::sh_nojobs (const char *s)
{
  if (s)
    builtin_error (_ ("%s: no job control"), s);
  else
    builtin_error (_ ("no job control"));
}
#endif

#if defined(RESTRICTED_SHELL)
void
Shell::sh_restricted (const char *s)
{
  if (s)
    builtin_error (_ ("%s: restricted"), s);
  else
    builtin_error (_ ("restricted"));
}
#endif

void
Shell::sh_notbuiltin (const char *s)
{
  builtin_error (_ ("%s: not a shell builtin"), s);
}

void
Shell::sh_wrerror ()
{
#if defined(DONT_REPORT_BROKEN_PIPE_WRITE_ERRORS) && defined(EPIPE)
  if (errno != EPIPE)
#endif /* DONT_REPORT_BROKEN_PIPE_WRITE_ERRORS && EPIPE */
    builtin_error (_ ("write error: %s"), strerror (errno));
}

void
Shell::sh_ttyerror (int set)
{
  if (set)
    builtin_error (_ ("error setting terminal attributes: %s"),
                   strerror (errno));
  else
    builtin_error (_ ("error getting terminal attributes: %s"),
                   strerror (errno));
}

int
Shell::sh_chkwrite (int s)
{
  QUIT;
  fflush (stdout);
  QUIT;
  if (ferror (stdout))
    {
      sh_wrerror ();
      fpurge (stdout);
      clearerr (stdout);
      return EXECUTION_FAILURE;
    }
  return s;
}

/* **************************************************************** */
/*								    */
/*	     Shell positional parameter manipulation		    */
/*								    */
/* **************************************************************** */

/* Convert a WORD_LIST into a C-style argv.  Return the number of elements
   in the list in *IP, if IP is non-null.  A convenience function for
   loadable builtins; also used by `test'. */
char **
Shell::make_builtin_argv (WORD_LIST *list, size_t *ip)
{
  char **argv;

  argv = strvec_from_word_list (list, 1, ip);
  argv[0] = savestring (this_command_name);
  return argv;
}

/* Remember LIST in $1 ... $9, and REST_OF_ARGS.  If DESTRUCTIVE is
   non-zero, then discard whatever the existing arguments are, else
   only discard the ones that are to be replaced.  Set POSPARAM_COUNT
   to the number of args assigned (length of LIST). */
void
Shell::remember_args (WORD_LIST *list, bool destructive)
{
  posparam_count = 0;

  for (int i = 1; i < 10; i++)
    {
      if ((destructive || list) && dollar_vars[i])
        {
          delete[] dollar_vars[i];
          dollar_vars[i] = nullptr;
        }

      if (list)
        {
          dollar_vars[posparam_count = i] = savestring (list->word->word);
          list = list->next ();
        }
    }

  /* If arguments remain, assign them to REST_OF_ARGS.
     Note that copy_word_list (NULL) returns NULL, and
     that dispose_words (NULL) does nothing. */
  if (destructive || list)
    {
      delete rest_of_args;
      rest_of_args = copy_word_list (list);
      posparam_count += list_length (list);
    }

  if (destructive)
    set_dollar_vars_changed ();

  invalidate_cached_quoted_dollar_at ();
}

void
Shell::shift_args (int times)
{
  if (times <= 0) /* caller should check */
    return;

  while (times-- > 0)
    {
      if (dollar_vars[1])
        delete[] dollar_vars[1];

      for (int count = 1; count < 9; count++)
        dollar_vars[count] = dollar_vars[count + 1];

      if (rest_of_args)
        {
          WORD_LIST *temp = rest_of_args;
          dollar_vars[9] = savestring (temp->word->word);
          rest_of_args = rest_of_args->next ();
          temp->set_next (nullptr);
          delete temp;
        }
      else
        dollar_vars[9] = nullptr;

      posparam_count--;
    }
}

int
Shell::number_of_args ()
{
#if 0
  WORD_LIST *list;
  int n;

  for (n = 0; n < 9 && dollar_vars[n + 1]; n++)
    ;
  for (list = rest_of_args; list; list = list->next ())
    n++;

  if (n != posparam_count)
    itrace ("number_of_args: n (%d) != posparam_count (%d)", n,
            posparam_count);
#endif

  return posparam_count;
}

/* **************************************************************** */
/*								    */
/*		Validating numeric input and arguments		    */
/*								    */
/* **************************************************************** */

/* Read a numeric arg for this_command_name, the name of the shell builtin
   that wants it.  LIST is the word list that the arg is to come from.
   Accept only the numeric argument; report an error if other arguments
   follow.  If FATAL is 1, call throw_to_top_level, which exits the
   shell; if it's 2, call jump_to_top_level (DISCARD), which aborts the
   current command; if FATAL is 0, return an indication of an invalid
   number by setting *NUMOK == 0 and return -1. */
int
Shell::get_numeric_arg (WORD_LIST *list, int fatal, int64_t *count)
{
  char *arg;

  if (count)
    *count = 1;

  if (list && list->word && ISOPTION (list->word->word, '-'))
    list = list->next ();

  if (list)
    {
      arg = list->word->word;
      if (arg == 0 || (legal_number (arg, count) == 0))
        {
          sh_neednumarg (list->word->word ? list->word->word : "`'");
          if (fatal == 0)
            return 0;
          else if (fatal == 1) /* fatal == 1; abort */
            throw_to_top_level ();
          else /* fatal == 2; discard current command */
            {
              top_level_cleanup ();
              throw bash_exception (DISCARD);
            }
        }
      no_args (list->next ());
    }

  return 1;
}

/* Get an eight-bit status value from LIST */
int
Shell::get_exitstat (WORD_LIST *list)
{
  int status;
  int64_t sval;
  char *arg;

  if (list && list->word && ISOPTION (list->word->word, '-'))
    list = (WORD_LIST *)list->next;

  if (list == 0)
    {
      /* If we're not running the DEBUG trap, the return builtin, when not
         given any arguments, uses the value of $? before the trap ran.  If
         given an argument, return uses it.  This means that the trap can't
         change $?.  The DEBUG trap gets to change $?, though, since that is
         part of its reason for existing, and because the extended debug mode
         does things with the return value. */
      if (this_shell_builtin == return_builtin && running_trap > 0
          && running_trap != DEBUG_TRAP + 1)
        return trap_saved_exit_value;
      return last_command_exit_value;
    }

  arg = list->word->word;
  if (arg == 0 || legal_number (arg, &sval) == 0)
    {
      sh_neednumarg (list->word->word ? list->word->word : "`'");
      return EX_BADUSAGE;
    }
  no_args ((WORD_LIST *)list->next);

  status = sval & 255;
  return status;
}

/* Return the octal number parsed from STRING, or -1 to indicate
   that the string contained a bad number. */
int
Shell::read_octal (const char *string)
{
  int result, digits;

  result = digits = 0;
  while (*string && isoctal (*string))
    {
      digits++;
      result = (result * 8) + (*string++ - '0');
      if (result > 07777)
        return -1;
    }

  if (digits == 0 || *string)
    result = -1;

  return result;
}

/* **************************************************************** */
/*								    */
/*	     Manipulating the current working directory		    */
/*								    */
/* **************************************************************** */

/* Return a consed string which is the current working directory.
   FOR_WHOM is the name of the caller for error printing.  */
char *
Shell::get_working_directory (const char *for_whom)
{
  if (no_symbolic_links)
    the_current_working_directory = nullptr;

  if (the_current_working_directory == nullptr)
    {
      char current_dir_buf[PATH_MAX];
      if (getcwd (current_dir_buf, PATH_MAX) == nullptr)
        {
          fprintf (stderr,
                   _ ("%s: error retrieving current directory: %s: %s\n"),
                   !for_whom.empty () ? to_string (for_whom).c_str ()
                                      : get_name_for_error (),
                   _ (bash_getcwd_errstr), strerror (errno));
          return nullptr;
        }

      the_current_working_directory = new std::string (current_dir_buf);
      return the_current_working_directory;
    }
}

/* Make NAME our internal idea of the current working directory. */
void
Shell::set_working_directory (const char *name)
{
  delete the_current_working_directory;
  the_current_working_directory = savestring (name);
}

/* **************************************************************** */
/*								    */
/*	     	Job control support functions			    */
/*								    */
/* **************************************************************** */

#if defined(JOB_CONTROL)
int
Shell::get_job_by_name (string_view name, int flags)
{
  int job = NO_JOB;
  int wl = strlen (name);
  for (int i = js.j_jobslots - 1; i >= 0; i--)
    {
      JOB *j = get_job_by_jid (i);
      if (j == 0 || ((flags & JM_STOPPED) && J_JOBSTATE (j) != JSTOPPED))
        continue;

      PROCESS *p = j->pipe;
      do
        {
          int match;
          if (flags & JM_EXACT)
            {
              int cl = strlen (p->command);
              match = STREQN (p->command, name, cl);
            }
          else if (flags & JM_SUBSTRING)
            match = strcasestr (p->command, name) != (char *)0;
          else
            match = STREQN (p->command, name, wl);

          if (match == 0)
            {
              p = p->next;
              continue;
            }
          else if (flags & JM_FIRSTMATCH)
            return i; /* return first match */
          else if (job != NO_JOB)
            {
              if (this_shell_builtin)
                builtin_error (_ ("%s: ambiguous job spec"), name);
              else
                internal_error (_ ("%s: ambiguous job spec"), name);
              return DUP_JOB;
            }
          else
            job = i;
        }
      while (p != j->pipe);
    }

  return job;
}

/* Return the job spec found in LIST. */
int
Shell::get_job_spec (WORD_LIST *list)
{
  if (list == 0)
    return js.j_current;

  std::string &word = list->word->word;

  if (*word == '\0')
    return NO_JOB;

  if (*word == '%')
    word++;

  if (DIGIT (*word) && all_digits (word))
    {
      int job = atoi (word);
      return (job < 0 || job > js.j_jobslots) ? NO_JOB : job - 1;
    }

  int jflags = 0;
  switch (*word)
    {
    case 0:
    case '%':
    case '+':
      return js.j_current;

    case '-':
      return js.j_previous;

    case '?': /* Substring search requested. */
      jflags |= JM_SUBSTRING;
      word++;
      /* FALLTHROUGH */

    default:
      return get_job_by_name (word, jflags);
    }
}
#endif /* JOB_CONTROL */

/*
 * NOTE:  `kill' calls this function with forcecols == 0
 */
int
Shell::display_signal_list (WORD_LIST *list, int forcecols)
{
  int result = EXECUTION_SUCCESS;
  if (!list)
    {
      int column = 0;
      for (int i = 1; i < NSIG; i++)
        {
          const char *name = signal_name (i);
          if (STREQN (name, "SIGJUNK", 7) || STREQN (name, "Unknown", 7))
            continue;

          if (posixly_correct && !forcecols)
            {
              /* This is for the kill builtin.  POSIX.2 says the signal names
                 are displayed without the `SIG' prefix. */
              if (STREQN (name, "SIG", 3))
                name += 3;
              printf ("%s%s", name, (i == NSIG - 1) ? "" : " ");
            }
          else
            {
              printf ("%2d) %s", i, name);

              if (++column < 5)
                printf ("\t");
              else
                {
                  printf ("\n");
                  column = 0;
                }
            }
        }

      if ((posixly_correct && !forcecols) || column != 0)
        printf ("\n");
      return result;
    }

  /* List individual signal names or numbers. */
  while (list)
    {
      int64_t lsignum;
      if (legal_number (list->word->word, &lsignum))
        {
          /* This is specified by Posix.2 so that exit statuses can be
             mapped into signal numbers. */
          if (lsignum > 128)
            lsignum -= 128;
          if (lsignum < 0 || lsignum >= NSIG)
            {
              sh_invalidsig (list->word->word);
              result = EXECUTION_FAILURE;
              list = list->next ();
              continue;
            }

          int signum = lsignum;
          const char *name = signal_name (signum);
          if (STREQN (name, "SIGJUNK", 7) || STREQN (name, "Unknown", 7))
            {
              list = list->next ();
              continue;
            }
          /* POSIX.2 says that `kill -l signum' prints the signal name without
             the `SIG' prefix. */
          printf ("%s\n", (this_shell_builtin == kill_builtin && signum > 0)
                              ? name + 3
                              : name);
        }
      else
        {
          int dflags = DSIG_NOCASE;
          if (posixly_correct == 0 || this_shell_builtin != kill_builtin)
            dflags |= DSIG_SIGPREFIX;
          int signum = decode_signal (list->word->word, dflags);
          if (signum == NO_SIG)
            {
              sh_invalidsig (list->word->word);
              result = EXECUTION_FAILURE;
              list = list->next ();
              continue;
            }
          printf ("%d\n", signum);
        }
      list = list->next ();
    }
  return result;
}

/* **************************************************************** */
/*								    */
/*	    Finding builtin commands and their functions	    */
/*								    */
/* **************************************************************** */

#if !defined(HELP_BUILTIN)
void
builtin_help ()
{
  printf ("%s: %s\n", this_command_name,
          _ ("help not available in this version"));
}
#endif

/* **************************************************************** */
/*								    */
/*	    Variable assignments during builtin commands	    */
/*								    */
/* **************************************************************** */

/* Assign NAME=VALUE, passing FLAGS to the assignment functions. */
SHELL_VAR *
Shell::builtin_bind_variable (const char *name, const char *value, int flags)
{
  SHELL_VAR *v;

#if defined(ARRAY_VARS)
  if (valid_array_reference (
          name, assoc_expand_once ? (VA_NOEXPAND | VA_ONEWORD) : 0)
      == 0)
    v = bind_variable (name, value, flags);
  else
    v = assign_array_element (name, value,
                              flags | (assoc_expand_once ? ASS_NOEXPAND : 0));
#else  /* !ARRAY_VARS */
  v = bind_variable (name, value, flags);
#endif /* !ARRAY_VARS */

  if (v && readonly_p (v) == 0 && noassign_p (v) == 0)
    VUNSETATTR (v, att_invisible);

  return v;
}

/* Like check_unbind_variable, but for use by builtins (only matters for
   error messages). */
int
Shell::builtin_unbind_variable (const char *vname)
{
  SHELL_VAR *v;

  v = find_variable (vname);
  if (v && readonly_p (v))
    {
      builtin_error (_ ("%s: cannot unset: readonly %s"), vname, "variable");
      return -2;
    }
  else if (v && non_unsettable_p (v))
    {
      builtin_error (_ ("%s: cannot unset"), vname);
      return -2;
    }
  return unbind_variable (vname);
}

} // namespace bash
