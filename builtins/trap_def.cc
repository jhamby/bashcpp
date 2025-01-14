// This file is trap_def.cc.
// It implements the builtin "trap" in Bash.

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

// $BUILTIN trap
// $FUNCTION trap_builtin
// $SHORT_DOC trap [-lp] [[arg] signal_spec ...]
// Trap signals and other events.

// Defines and activates handlers to be run when the shell receives signals
// or other conditions.

// ARG is a command to be read and executed when the shell receives the
// signal(s) SIGNAL_SPEC.  If ARG is absent (and a single SIGNAL_SPEC
// is supplied) or `-', each specified signal is reset to its original
// value.  If ARG is the null string each SIGNAL_SPEC is ignored by the
// shell and by the commands it invokes.

// If a SIGNAL_SPEC is EXIT (0) ARG is executed on exit from the shell.  If
// a SIGNAL_SPEC is DEBUG, ARG is executed before every simple command.  If
// a SIGNAL_SPEC is RETURN, ARG is executed each time a shell function or a
// script run by the . or source builtins finishes executing.  A SIGNAL_SPEC
// of ERR means to execute ARG each time a command's failure would cause the
// shell to exit when the -e option is enabled.

// If no arguments are supplied, trap prints the list of commands associated
// with each signal.

// Options:
//   -l	print a list of signal names and their corresponding numbers
//   -p	display the trap commands associated with each SIGNAL_SPEC

// Each SIGNAL_SPEC is either a signal name in <signal.h> or a signal number.
// Signal names are case insensitive and the SIG prefix is optional.  A
// signal may be sent to the shell with "kill -signal $$".

// Exit Status:
// Returns success unless a SIGSPEC is invalid or an invalid option is given.
// $END

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashtypes.hh"

#include <csignal>

#include "common.hh"
#include "shell.hh"
#include "trap.hh"

namespace bash
{

// static void showtrap (int, int);
// static int display_traps (WORD_LIST *, int);

/* The trap command:

   trap <arg> <signal ...>
   trap <signal ...>
   trap -l
   trap -p [sigspec ...]
   trap [--]

   Set things up so that ARG is executed when SIGNAL(s) N is received.
   If ARG is the empty string, then ignore the SIGNAL(s).  If there is
   no ARG, then set the trap for SIGNAL(s) to its original value.  Just
   plain "trap" means to print out the list of commands associated with
   each signal number.  Single arg of "-l" means list the signal names. */

/* Possible operations to perform on the list of signals.*/
#define SET 0    /* Set this signal to first_arg. */
#define REVERT 1 /* Revert to this signals original value. */
#define IGNORE 2 /* Ignore this signal. */

int
Shell::trap_builtin (WORD_LIST *list)
{
  int list_signal_names, display, result, opt;

  list_signal_names = display = 0;
  result = EXECUTION_SUCCESS;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "lp")) != -1)
    {
      switch (opt)
        {
        case 'l':
          list_signal_names++;
          break;
        case 'p':
          display++;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  opt = DSIG_NOCASE | DSIG_SIGPREFIX; /* flags for decode_signal */

  if (list_signal_names)
    return sh_chkwrite (display_signal_list ((WORD_LIST *)NULL, 1));
  else if (display || list == 0)
    {
      initialize_terminating_signals ();
      get_all_original_signals ();
      return sh_chkwrite (display_traps (list, display && posixly_correct));
    }
  else
    {
      char *first_arg;
      int operation, sig, first_signal;

      operation = SET;
      first_arg = list->word->word;
      first_signal = first_arg && *first_arg && all_digits (first_arg)
                     && signal_object_p (first_arg, opt);

      /* Backwards compatibility.  XXX - question about whether or not we
         should throw an error if an all-digit argument doesn't correspond
         to a valid signal number (e.g., if it's `50' on a system with only
         32 signals).  */
      if (first_signal)
        operation = REVERT;
      /* When in posix mode, the historical behavior of looking for a
         missing first argument is disabled.  To revert to the original
         signal handling disposition, use `-' as the first argument. */
      else if (posixly_correct == 0 && first_arg && *first_arg
               && (*first_arg != '-' || first_arg[1])
               && signal_object_p (first_arg, opt) && list->next == 0)
        operation = REVERT;
      else
        {
          list = (WORD_LIST *)list->next;
          if (list == 0)
            {
              builtin_usage ();
              return EX_USAGE;
            }
          else if (*first_arg == '\0')
            operation = IGNORE;
          else if (first_arg[0] == '-' && !first_arg[1])
            operation = REVERT;
        }

      /* If we're in a command substitution, we haven't freed the trap strings
         (though we reset the signal handlers).  If we're setting a trap to
         handle a signal here, free the rest of the trap strings since they
         don't apply any more. */
      if (subshell_environment & SUBSHELL_RESETTRAP)
        {
          free_trap_strings ();
          subshell_environment &= ~SUBSHELL_RESETTRAP;
        }

      while (list)
        {
          sig = decode_signal (list->word->word, opt);

          if (sig == NO_SIG)
            {
              sh_invalidsig (list->word->word);
              result = EXECUTION_FAILURE;
            }
          else
            {
              switch (operation)
                {
                case SET:
                  set_signal (sig, first_arg);
                  break;

                case REVERT:
                  restore_default_signal (sig);

                  /* Signals that the shell treats specially need special
                     handling. */
                  switch (sig)
                    {
                    case SIGINT:
                      /* XXX - should we do this if original disposition
                         was SIG_IGN? */
                      if (interactive)
                        set_signal_handler (SIGINT, sigint_sighandler);
                      /* special cases for interactive == 0 */
                      else if (interactive_shell
                               && (sourcelevel || running_trap
                                   || parse_and_execute_level))
                        set_signal_handler (SIGINT, sigint_sighandler);
                      else
                        set_signal_handler (SIGINT, termsig_sighandler);
                      break;

                    case SIGQUIT:
                      /* Always ignore SIGQUIT. */
                      set_signal_handler (SIGQUIT, SIG_IGN);
                      break;
                    case SIGTERM:
#if defined(JOB_CONTROL)
                    case SIGTTIN:
                    case SIGTTOU:
                    case SIGTSTP:
#endif /* JOB_CONTROL */
                      if (interactive)
                        set_signal_handler (sig, SIG_IGN);
                      break;
                    }
                  break;

                case IGNORE:
                  ignore_signal (sig);
                  break;
                }
            }
          list = (WORD_LIST *)list->next;
        }
    }

  return result;
}

static void
showtrap (int i, int show_default)
{
  char *t, *p;

  p = trap_list[i];
  if (p == (char *)DEFAULT_SIG && signal_is_hard_ignored (i) == 0)
    {
      if (show_default)
        t = (char *)"-";
      else
        return;
    }
  else if (signal_is_hard_ignored (i))
    t = (char *)NULL;
  else
    t = (p == (char *)IGNORE_SIG) ? (char *)NULL : sh_single_quote (p);

  const char *sn = signal_name (i);
  /* Make sure that signals whose names are unknown (for whatever reason)
     are printed as signal numbers. */
  if (STREQN (sn, "SIGJUNK", 7) || STREQN (sn, "unknown", 7))
    printf ("trap -- %s %d\n", t ? t : "''", i);
  else if (posixly_correct)
    {
      if (STREQN (sn, "SIG", 3))
        printf ("trap -- %s %s\n", t ? t : "''", sn + 3);
      else
        printf ("trap -- %s %s\n", t ? t : "''", sn);
    }
  else
    printf ("trap -- %s %s\n", t ? t : "''", sn);

  if (!show_default)
    FREE (t);
}

static int
display_traps (WORD_LIST *list, int show_all)
{
  int result, i;

  if (list == 0)
    {
      for (i = 0; i < BASH_NSIG; i++)
        showtrap (i, show_all);
      return EXECUTION_SUCCESS;
    }

  for (result = EXECUTION_SUCCESS; list; list = list->next ())
    {
      i = decode_signal (list->word->word, DSIG_NOCASE | DSIG_SIGPREFIX);
      if (i == NO_SIG)
        {
          sh_invalidsig (list->word->word);
          result = EXECUTION_FAILURE;
        }
      else
        showtrap (i, show_all);
    }

  return result;
}

} // namespace bash
