/* trap.cc -- Not the trap command, but useful functions for manipulating
   those objects.  The trap command is in builtins/trap.def. */

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

#include "shell.hh"

#include "builtins.hh"
#include "builtins/common.hh"

#include "builtext.hh"

#if defined(READLINE)
#include "readline.hh"
#endif

namespace bash
{

void
Shell::initialize_traps ()
{
  int i;

  trap_list[EXIT_TRAP] = trap_list[DEBUG_TRAP] = trap_list[ERROR_TRAP]
      = trap_list[RETURN_TRAP] = nullptr;
  sigmodes[EXIT_TRAP] = sigmodes[DEBUG_TRAP] = sigmodes[ERROR_TRAP]
      = sigmodes[RETURN_TRAP] = SIG_INHERITED;
  original_signals[EXIT_TRAP]
      = reinterpret_cast<SigHandler> (IMPOSSIBLE_TRAP_HANDLER);

  for (i = 1; i < NSIG; i++)
    {
      pending_traps[i] = 0;
      trap_list[i] = DEFAULT_SIG;
      sigmodes[i] = SIG_INHERITED; /* XXX - only set, not used */
      original_signals[i]
          = reinterpret_cast<SigHandler> (IMPOSSIBLE_TRAP_HANDLER);
    }

    /* Show which signals are treated specially by the shell. */
#if defined(SIGCHLD)
  GETORIGSIG (SIGCHLD);
  sigmodes[SIGCHLD] |= (SIG_SPECIAL | SIG_NO_TRAP);
#endif /* SIGCHLD */

  GETORIGSIG (SIGINT);
  sigmodes[SIGINT] |= SIG_SPECIAL;

  GETORIGSIG (SIGQUIT);
  sigmodes[SIGQUIT] |= SIG_SPECIAL;

  if (interactive)
    {
      GETORIGSIG (SIGTERM);
      sigmodes[SIGTERM] |= SIG_SPECIAL;
    }

  get_original_tty_job_signals ();
}

#ifdef DEBUG
/* Return a printable representation of the trap handler for SIG. */
const char *
Shell::trap_handler_string (int sig)
{
  if (trap_list[sig] == const_cast<char *> (DEFAULT_SIG))
    return "DEFAULT_SIG";
  else if (trap_list[sig] == const_cast<char *> (IGNORE_SIG))
    return "IGNORE_SIG";
  else if (trap_list[sig] == const_cast<char *> (IMPOSSIBLE_TRAP_HANDLER))
    return "IMPOSSIBLE_TRAP_HANDLER";
  else if (trap_list[sig])
    return trap_list[sig];
  else
    return "NULL";
}
#endif

/* Return the print name of this signal. */
static const char *
signal_name (int sig)
{
  const char *ret;

  /* on cygwin32, signal_names[sig] could be null */
  ret = (sig >= BASH_NSIG || sig < 0 || signal_names[sig] == nullptr)
            ? _ ("invalid signal number")
            : signal_names[sig];

  return ret;
}

/* Turn a string into a signal number, or a number into
   a signal number.  If STRING is "2", "SIGINT", or "INT",
   then (int)2 is returned.  Return NO_SIG if STRING doesn't
   contain a valid signal descriptor. */
int
decode_signal (const char *string, decode_signal_flags flags)
{
  int64_t sig;

  if (legal_number (string, &sig))
    return (sig >= 0 && sig < NSIG) ? static_cast<int> (sig) : NO_SIG;

#if defined(SIGRTMIN) && defined(SIGRTMAX)
  if (STREQN (string, "SIGRTMIN+", 9)
      || ((flags & DSIG_NOCASE) && strncasecmp (string, "SIGRTMIN+", 9) == 0))
    {
      if (legal_number (string + 9, &sig) && sig >= 0
          && sig <= SIGRTMAX - SIGRTMIN)
        return static_cast<int> (SIGRTMIN + sig);
      else
        return NO_SIG;
    }
  else if (STREQN (string, "RTMIN+", 6)
           || ((flags & DSIG_NOCASE)
               && strncasecmp (string, "RTMIN+", 6) == 0))
    {
      if (legal_number (string + 6, &sig) && sig >= 0
          && sig <= SIGRTMAX - SIGRTMIN)
        return static_cast<int> (SIGRTMIN + sig);
      else
        return NO_SIG;
    }
#endif /* SIGRTMIN && SIGRTMAX */

  /* A leading `SIG' may be omitted. */
  for (sig = 0; sig < BASH_NSIG; sig++)
    {
      const char *name = signal_names[sig];
      if (name == nullptr || name[0] == '\0')
        continue;

      /* Check name without the SIG prefix first case sensitively or
         insensitively depending on whether flags includes DSIG_NOCASE */
      if (STREQN (name, "SIG", 3))
        {
          name += 3;

          if ((flags & DSIG_NOCASE) && strcasecmp (string, name) == 0)
            return static_cast<int> (sig);
          else if ((flags & DSIG_NOCASE) == 0 && strcmp (string, name) == 0)
            return static_cast<int> (sig);
          /* If we can't use the `SIG' prefix to match, punt on this
             name now. */
          else if ((flags & DSIG_SIGPREFIX) == 0)
            continue;
        }

      /* Check name with SIG prefix case sensitively or insensitively
         depending on whether flags includes DSIG_NOCASE */
      name = signal_names[sig];
      if ((flags & DSIG_NOCASE) && strcasecmp (string, name) == 0)
        return static_cast<int> (sig);
      else if ((flags & DSIG_NOCASE) == 0 && strcmp (string, name) == 0)
        return static_cast<int> (sig);
    }

  return NO_SIG;
}

void
Shell::run_pending_traps ()
{
  int sig;
  int old_exit_value, x;
  int old_running;
  WORD_LIST *save_subst_varlist;
  HASH_TABLE *save_tempenv;
  sh_parser_state_t pstate;
#if defined(ARRAY_VARS)
  ARRAY *ps;
#endif

  if (catch_flag == 0) /* simple optimization */
    return;

  if (running_trap > 0)
    {
#if defined(DEBUG)
      internal_warning ("run_pending_traps: recursive invocation while "
                        "running trap for signal %d",
                        running_trap - 1);
#endif
#if defined(SIGWINCH)
      if (running_trap == SIGWINCH + 1 && pending_traps[SIGWINCH])
        return; /* no recursive SIGWINCH trap invocations */
#endif
      /* could check for running the trap handler for the same signal here
         (running_trap == sig+1) */
      if (evalnest_max > 0 && evalnest > evalnest_max)
        {
          internal_error (
              _ ("trap handler: maximum trap handler level exceeded (%d)"),
              evalnest_max);
          evalnest = 0;
          throw bash_exception (DISCARD);
        }
    }

  catch_flag = trapped_signal_received = 0;

  /* Preserve $? when running trap. */
  trap_saved_exit_value = old_exit_value = last_command_exit_value;
#if defined(ARRAY_VARS)
  ps = save_pipestatus_array ();
#endif
  old_running = running_trap;

  for (sig = 1; sig < NSIG; sig++)
    {
      /* XXX this could be made into a counter by using
         while (pending_traps[sig]--) instead of the if statement. */
      if (pending_traps[sig])
        {
          if (running_trap == sig + 1)
            /*continue*/;

          running_trap = sig + 1;

          if (sig == SIGINT)
            {
              pending_traps[sig] = 0; /* XXX */
              /* We don't modify evalnest here, since run_interrupt_trap()
                 calls _run_trap_internal, which does. */
              run_interrupt_trap (0);
              CLRINTERRUPT; /* interrupts don't stack */
            }
#if defined(JOB_CONTROL) && defined(SIGCHLD)
          else if (sig == SIGCHLD && trap_list[SIGCHLD] != IMPOSSIBLE_TRAP_NAME
                   && (sigmodes[SIGCHLD] & SIG_INPROGRESS) == 0)
            {
              sigmodes[SIGCHLD] |= SIG_INPROGRESS;
              /* We modify evalnest here even though run_sigchld_trap can run
                 the trap action more than once */
              evalnest++;
              x = pending_traps[sig];
              pending_traps[sig] = 0;
              run_sigchld_trap (x); /* use as counter */
              running_trap = 0;
              evalnest--;
              sigmodes[SIGCHLD] &= ~SIG_INPROGRESS;
              /* continue here rather than reset pending_traps[SIGCHLD] below
                 in case there are recursive calls to run_pending_traps and
                 children have been reaped while run_sigchld_trap was running.
               */
              continue;
            }
          else if (sig == SIGCHLD && trap_list[SIGCHLD] == IMPOSSIBLE_TRAP_NAME
                   && (sigmodes[SIGCHLD] & SIG_INPROGRESS) != 0)
            {
              /* This can happen when run_pending_traps is called while
                 running a SIGCHLD trap handler. */
              running_trap = 0;
              /* want to leave pending_traps[SIGCHLD] alone here */
              continue; /* XXX */
            }
          else if (sig == SIGCHLD && (sigmodes[SIGCHLD] & SIG_INPROGRESS))
            {
              /* whoops -- print warning? */
              running_trap = 0; /* XXX */
              /* want to leave pending_traps[SIGCHLD] alone here */
              continue;
            }
#endif
          else if (trap_list[sig] == const_cast<char *> (DEFAULT_SIG)
                   || trap_list[sig] == const_cast<char *> (IGNORE_SIG)
                   || trap_list[sig] == IMPOSSIBLE_TRAP_NAME)
            {
              /* This is possible due to a race condition.  Say a bash
                 process has SIGTERM trapped.  A subshell is spawned
                 using { list; } & and the parent does something and kills
                 the subshell with SIGTERM.  It's possible for the subshell
                 to set pending_traps[SIGTERM] to 1 before the code in
                 execute_cmd.c eventually calls restore_original_signals
                 to reset the SIGTERM signal handler in the subshell.  The
                 next time run_pending_traps is called, pending_traps[SIGTERM]
                 will be 1, but the trap handler in trap_list[SIGTERM] will
                 be invalid (probably DEFAULT_SIG, but it could be IGNORE_SIG).
                 Unless we catch this, the subshell will dump core when
                 trap_list[SIGTERM] == DEFAULT_SIG, because DEFAULT_SIG is
                 usually 0x0. */
              internal_warning (
                  _ ("run_pending_traps: bad value in trap_list[%d]: %p"), sig,
                  reinterpret_cast<const void *> (trap_list[sig]));
              if (trap_list[sig] == const_cast<char *> (DEFAULT_SIG))
                {
                  internal_warning (_ ("run_pending_traps: signal handler is "
                                       "SIG_DFL, resending %d (%s) to myself"),
                                    sig, signal_name (sig));
                  kill (getpid (), sig);
                }
            }
          else
            {
              /* XXX - should we use save_parser_state/restore_parser_state? */
              save_parser_state (&pstate);
              save_subst_varlist = subst_assign_varlist;
              subst_assign_varlist = nullptr;
              save_tempenv = temporary_env;
              /* traps shouldn't run with temporary env */
              temporary_env = nullptr;

#if defined(JOB_CONTROL)
              save_pipeline (1); /* XXX only provides one save level */
#endif
              /* XXX - set pending_traps[sig] = 0 here? */
              pending_traps[sig] = 0;
              evalnest++;
              evalstring (savestring (trap_list[sig]), "trap",
                          SEVAL_NONINT | SEVAL_NOHIST | SEVAL_RESETLINE);
              evalnest--;
#if defined(JOB_CONTROL)
              restore_pipeline (1);
#endif

              subst_assign_varlist = save_subst_varlist;
              restore_parser_state (&pstate);
              temporary_env = save_tempenv;
            }

          pending_traps[sig] = 0; /* XXX - move before evalstring? */
          running_trap = old_running;
        }
    }

#if defined(ARRAY_VARS)
  restore_pipestatus_array (ps);
#endif
  last_command_exit_value = old_exit_value;
}

/* Set the private state variables noting that we received a signal SIG
   for which we have a trap set. */
void
Shell::set_trap_state (int sig)
{
  catch_flag = true;
  pending_traps[sig]++;
  trapped_signal_received = sig;
}

extern "C" void
trap_handler_global (int sig)
{
  the_shell->trap_handler (sig);
}

void
Shell::trap_handler (int sig)
{
  int oerrno;

  if ((sigmodes[sig] & SIG_TRAPPED) == 0)
    {
#if defined(DEBUG)
      internal_warning ("trap_handler: signal %d: signal not trapped", sig);
#endif
      return;
    }

  /* This means we're in a subshell, but have not yet reset the handler for
     trapped signals. We're not supposed to execute the trap in this situation;
     we should restore the original signal and resend the signal to ourselves
     to preserve the Posix "signal traps that are not being ignored shall be
     set to the default action" semantics. */
  if ((subshell_environment & SUBSHELL_IGNTRAP)
      && trap_list[sig] != const_cast<char *> (IGNORE_SIG))
    {
      sigset_t mask;

      /* Paranoia */
      if (original_signals[sig]
          == reinterpret_cast<SigHandler> (IMPOSSIBLE_TRAP_HANDLER))
        original_signals[sig] = SIG_DFL;

      restore_signal (sig);

      /* Make sure we let the signal we just caught through */
      sigemptyset (&mask);
      sigprocmask (SIG_SETMASK, nullptr, &mask);
      sigdelset (&mask, sig);
      sigprocmask (SIG_SETMASK, &mask, nullptr);

      kill (getpid (), sig);

      return;
    }

  if ((sig >= NSIG) || (trap_list[sig] == const_cast<char *> (DEFAULT_SIG))
      || (trap_list[sig] == const_cast<char *> (IGNORE_SIG)))
    programming_error (_ ("trap_handler: bad signal %d"), sig);
  else
    {
      oerrno = errno;

      set_trap_state (sig);

      if (this_shell_builtin && (this_shell_builtin == &Shell::wait_builtin))
        {
          wait_signal_received = sig;
          if (waiting_for_child && wait_intr_flag)
            throw wait_interrupt ();
        }

#if defined(READLINE)
      /* Set the event hook so readline will call it after the signal handlers
         finish executing, so if this interrupted character input we can get
         quick response. */
      if (RL_ISSTATE (RL_STATE_SIGHANDLER))
        bashline_set_event_hook ();
#endif

      errno = oerrno;
    }
}

/* Reset the SIGINT handler so that subshells that are doing `shellsy'
   things, like waiting for command substitution or executing commands
   in explicit subshells ( ( cmd ) ), can catch interrupts properly. */
SigHandler
Shell::set_sigint_handler ()
{
  if (sigmodes[SIGINT] & SIG_HARD_IGNORE)
    return SIG_IGN;

  else if (sigmodes[SIGINT] & SIG_IGNORED)
    return set_signal_handler (SIGINT, SIG_IGN); /* XXX */

  else if (sigmodes[SIGINT] & SIG_TRAPPED)
    return set_signal_handler (SIGINT, &trap_handler_global);

  /* The signal is not trapped, so set the handler to the shell's special
     interrupt handler. */
  else if (interactive) /* XXX - was interactive_shell */
    return set_signal_handler (SIGINT, &sigint_sighandler_global);
  else
    return set_signal_handler (SIGINT, &termsig_sighandler_global);
}

/* Set SIG to call STRING as a command. */
void
Shell::set_signal (int sig, string_view string)
{
  sigset_t set, oset;

  if (SPECIAL_TRAP (sig))
    {
      change_signal (sig, savestring (string));
      if (sig == EXIT_TRAP && interactive == 0)
        initialize_terminating_signals ();
      return;
    }

  /* A signal ignored on entry to the shell cannot be trapped or reset, but
     no error is reported when attempting to do so.  -- Posix.2 */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* Make sure we have original_signals[sig] if the signal has not yet
     been trapped. */
  if ((sigmodes[sig] & SIG_TRAPPED) == 0)
    {
      /* If we aren't sure of the original value, check it. */
      if (original_signals[sig]
          == reinterpret_cast<SigHandler> (IMPOSSIBLE_TRAP_HANDLER))
        GETORIGSIG (sig);
      if (original_signals[sig] == SIG_IGN)
        return;
    }

  /* Only change the system signal handler if SIG_NO_TRAP is not set.
     The trap command string is changed in either case.  The shell signal
     handlers for SIGINT and SIGCHLD run the user specified traps in an
     environment in which it is safe to do so. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    {
      BLOCK_SIGNAL (sig, set, oset);
      change_signal (sig, savestring (string));
      set_signal_handler (sig, &trap_handler_global);
      UNBLOCK_SIGNAL (oset);
    }
  else
    change_signal (sig, savestring (string));
}

/* Restore the default action for SIG; i.e., the action the shell
   would have taken before you used the trap command.  This is called
   from trap_builtin (), which takes care to restore the handlers for
   the signals the shell treats specially. */
void
Shell::restore_default_signal (int sig)
{
  if (SPECIAL_TRAP (sig))
    {
      if ((sig != DEBUG_TRAP && sig != ERROR_TRAP && sig != RETURN_TRAP)
          || (sigmodes[sig] & SIG_INPROGRESS) == 0)
        free_trap_command (sig);
      trap_list[sig] = nullptr;
      sigmodes[sig] &= ~SIG_TRAPPED;
      if (sigmodes[sig] & SIG_INPROGRESS)
        sigmodes[sig] |= SIG_CHANGED;
      return;
    }

  GET_ORIGINAL_SIGNAL (sig);

  /* A signal ignored on entry to the shell cannot be trapped or reset, but
     no error is reported when attempting to do so.  Thanks Posix.2. */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* If we aren't trapping this signal, don't bother doing anything else. */
  /* We special-case SIGCHLD and IMPOSSIBLE_TRAP_HANDLER (see above) as a
     sentinel to determine whether or not disposition is reset to the default
     while the trap handler is executing. */
  if (((sigmodes[sig] & SIG_TRAPPED) == 0)
      && (sig != SIGCHLD || (sigmodes[sig] & SIG_INPROGRESS) == 0
          || trap_list[sig] != IMPOSSIBLE_TRAP_NAME))
    return;

  /* Only change the signal handler for SIG if it allows it. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    set_signal_handler (sig, original_signals[sig]);

  /* Change the trap command in either case. */
  change_signal (sig, const_cast<char *> (DEFAULT_SIG));

  /* Mark the signal as no longer trapped. */
  sigmodes[sig] &= ~SIG_TRAPPED;
}

/* Make this signal be ignored. */
void
Shell::ignore_signal (int sig)
{
  if (SPECIAL_TRAP (sig) && ((sigmodes[sig] & SIG_IGNORED) == 0))
    {
      change_signal (sig, const_cast<char *> (IGNORE_SIG));
      return;
    }

  GET_ORIGINAL_SIGNAL (sig);

  /* A signal ignored on entry to the shell cannot be trapped or reset.
     No error is reported when the user attempts to do so. */
  if (sigmodes[sig] & SIG_HARD_IGNORE)
    return;

  /* If already trapped and ignored, no change necessary. */
  if (sigmodes[sig] & SIG_IGNORED)
    return;

  /* Only change the signal handler for SIG if it allows it. */
  if ((sigmodes[sig] & SIG_NO_TRAP) == 0)
    set_signal_handler (sig, SIG_IGN);

  /* Change the trap command in either case. */
  change_signal (sig, const_cast<char *> (IGNORE_SIG));
}

/* Handle the calling of "trap 0".  The only sticky situation is when
   the command to be executed includes an "exit".  This is why we have
   to provide our own place for top_level to jump to. */
int
Shell::run_exit_trap ()
{
  char *trap_command;
  int retval;
#if defined(ARRAY_VARS)
  ARRAY *ps;
#endif

  trap_saved_exit_value = last_command_exit_value;
#if defined(ARRAY_VARS)
  ps = save_pipestatus_array ();
#endif

  /* Run the trap only if signal 0 is trapped and not ignored, and we are not
     currently running in the trap handler (call to exit in the list of
     commands given to trap 0). */
  if ((sigmodes[EXIT_TRAP] & SIG_TRAPPED)
      && (sigmodes[EXIT_TRAP] & (SIG_IGNORED | SIG_INPROGRESS)) == 0)
    {
      trap_command = savestring (trap_list[EXIT_TRAP]);
      sigmodes[EXIT_TRAP] &= ~SIG_TRAPPED;
      sigmodes[EXIT_TRAP] |= SIG_INPROGRESS;

      retval = trap_saved_exit_value;
      running_trap = 1;

      try
        {
          reset_parser ();
          parse_and_execute (trap_command, "exit trap",
                             SEVAL_NONINT | SEVAL_NOHIST | SEVAL_RESETLINE);
        }
      catch (const bash_exception &e)
        {
          if (e.type == ERREXIT || e.type == EXITPROG)
            retval = last_command_exit_value;
          else
            retval = trap_saved_exit_value;
        }
      catch (const return_exception &)
        {
          retval = return_catch_value;
        }

      running_trap = 0;
#if defined(ARRAY_VARS)
      array_dispose (ps);
#endif

      return retval;
    }

#if defined(ARRAY_VARS)
  restore_pipestatus_array (ps);
#endif
  return trap_saved_exit_value;
}

#define RECURSIVE_SIG(s) (SPECIAL_TRAP (s) == 0)

/* Run a trap command for SIG.  SIG is one of the signals the shell treats
   specially.  Returns the exit status of the executed trap command list. */
int
Shell::_run_trap_internal (int sig, const char *tag)
{
  const char *trap_command;
  int trap_exit_value;
  WORD_LIST *save_subst_varlist;
  HASH_TABLE *save_tempenv;
  sh_parser_state_t pstate;
#if defined(ARRAY_VARS)
  ARRAY *ps;
#endif

  trap_exit_value = 0;
  trap_saved_exit_value = last_command_exit_value;
  /* Run the trap only if SIG is trapped and not ignored, and we are not
     currently executing in the trap handler. */
  if ((sigmodes[sig] & SIG_TRAPPED) && ((sigmodes[sig] & SIG_IGNORED) == 0)
      && (trap_list[sig] != IMPOSSIBLE_TRAP_NAME) &&
#if 1
      /* Uncomment this to allow some special signals to recursively execute
         trap handlers. */
      (RECURSIVE_SIG (sig) || (sigmodes[sig] & SIG_INPROGRESS) == 0))
#else
      ((sigmodes[sig] & SIG_INPROGRESS) == 0))
#endif
    {
      char *old_trap = const_cast<char *> (trap_list[sig]);
      int old_modes = sigmodes[sig];
      int old_running = running_trap;

      sigmodes[sig] |= SIG_INPROGRESS;
      sigmodes[sig] &= ~SIG_CHANGED; /* just to be sure */
      trap_command = savestring (old_trap);

      running_trap = sig + 1;

      int old_int
          = interrupt_state; /* temporarily suppress pending interrupts */
      CLRINTERRUPT;

#if defined(ARRAY_VARS)
      ps = save_pipestatus_array ();
#endif

      save_parser_state (&pstate);
      save_subst_varlist = subst_assign_varlist;
      subst_assign_varlist = nullptr;
      save_tempenv = temporary_env;
      temporary_env = nullptr; /* traps should not run with temporary env */

#if defined(JOB_CONTROL)
      if (sig != DEBUG_TRAP) /* run_debug_trap does this */
        save_pipeline (1);   /* XXX only provides one save level */
#endif

      /* If we're in a function, make sure return exceptions come here, too. */
      int save_return_catch_flag = return_catch_flag;

      parse_flags flags = SEVAL_NONINT | SEVAL_NOHIST;
      if (sig != DEBUG_TRAP && sig != RETURN_TRAP && sig != ERROR_TRAP)
        flags |= SEVAL_RESETLINE;
      evalnest++;

      bool return_exception_thrown = false;
      try
        {
          parse_and_execute (trap_command, tag, flags);
          trap_exit_value = last_command_exit_value;
        }
      catch (const return_exception &)
        {
          trap_exit_value = return_catch_value;
          return_exception_thrown = true;
        }
      evalnest--;

#if defined(JOB_CONTROL)
      if (sig != DEBUG_TRAP) /* run_debug_trap does this */
        restore_pipeline (1);
#endif

      subst_assign_varlist = save_subst_varlist;
      restore_parser_state (&pstate);

#if defined(ARRAY_VARS)
      restore_pipestatus_array (ps);
#endif

      temporary_env = save_tempenv;

      if ((old_modes & SIG_INPROGRESS) == 0)
        sigmodes[sig] &= ~SIG_INPROGRESS;

      running_trap = old_running;
      interrupt_state = old_int;

      if (sigmodes[sig] & SIG_CHANGED)
        {
          delete[] old_trap;
          sigmodes[sig] &= ~SIG_CHANGED;

          CHECK_TERMSIG; /* some pathological conditions lead here */
        }

      if (save_return_catch_flag)
        {
          return_catch_flag = save_return_catch_flag;
          return_catch_value = trap_exit_value;
          if (return_exception_thrown)
            {
              throw return_exception ();
            }
        }
    }

  return trap_exit_value;
}

int
Shell::run_debug_trap ()
{
  int trap_exit_value;
  pid_t save_pgrp;
#if defined(PGRP_PIPE)
  int save_pipe[2];
#endif

  /* XXX - question:  should the DEBUG trap inherit the RETURN trap? */
  trap_exit_value = 0;
  if ((sigmodes[DEBUG_TRAP] & SIG_TRAPPED)
      && ((sigmodes[DEBUG_TRAP] & SIG_IGNORED) == 0)
      && ((sigmodes[DEBUG_TRAP] & SIG_INPROGRESS) == 0))
    {
#if defined(JOB_CONTROL)
      save_pgrp = pipeline_pgrp;
      pipeline_pgrp = 0;
      save_pipeline (1);
#if defined(PGRP_PIPE)
      save_pgrp_pipe (save_pipe, 1);
#endif
      stop_making_children ();
#endif

      char old_verbose = echo_input_at_read;
      echo_input_at_read
          = suppress_debug_trap_verbose ? 0 : echo_input_at_read;

      trap_exit_value = _run_trap_internal (DEBUG_TRAP, "debug trap");

      echo_input_at_read = old_verbose;

#if defined(JOB_CONTROL)
      pipeline_pgrp = save_pgrp;
      restore_pipeline (1);
#if defined(PGRP_PIPE)
      close_pgrp_pipe ();
      restore_pgrp_pipe (save_pipe);
#endif
      if (pipeline_pgrp > 0
          && ((subshell_environment & (SUBSHELL_ASYNC | SUBSHELL_PIPE)) == 0))
        give_terminal_to (pipeline_pgrp, 1);

      notify_and_cleanup ();
#endif

#if defined(DEBUGGER)
      /* If we're in the debugger and the DEBUG trap returns 2 while we're in
         a function or sourced script, we force a `return'. */
      if (debugging_mode && trap_exit_value == 2 && return_catch_flag)
        {
          return_catch_value = trap_exit_value;
          throw return_exception ();
        }
#endif
    }
  return trap_exit_value;
}

/* Free all the allocated strings in the list of traps and reset the trap
   values to the default.  Intended to be called from subshells that want
   to complete work done by reset_signal_handlers upon execution of a
   subsequent `trap' command that changes a signal's disposition.  We need
   to make sure that we duplicate the behavior of
   reset_or_restore_signal_handlers and not change the disposition of signals
   that are set to be ignored. */
void
Shell::free_trap_strings ()
{
  int i;

  for (i = 0; i < NSIG; i++)
    {
      if (trap_list[i] != const_cast<char *> (IGNORE_SIG))
        free_trap_string (i);
    }
  for (i = NSIG; i < BASH_NSIG; i++)
    {
      /* Don't free the trap string if the subshell inherited the trap */
      if ((sigmodes[i] & SIG_TRAPPED) == 0)
        {
          free_trap_string (i);
          trap_list[i] = nullptr;
        }
    }
}

void
Shell::reset_or_restore_signal_handlers (sh_resetsig_func_t reset)
{
  int i;

  /* Take care of the exit trap first */
  if (sigmodes[EXIT_TRAP] & SIG_TRAPPED)
    {
      sigmodes[EXIT_TRAP] &= ~SIG_TRAPPED; /* XXX - SIG_INPROGRESS? */
      if (reset != &Shell::reset_signal)
        {
          free_trap_command (EXIT_TRAP);
          trap_list[EXIT_TRAP] = nullptr;
        }
    }

  for (i = 1; i < NSIG; i++)
    {
      if (sigmodes[i] & SIG_TRAPPED)
        {
          if (trap_list[i] == const_cast<char *> (IGNORE_SIG))
            set_signal_handler (i, SIG_IGN);
          else
            ((*this).*reset) (i);
        }
      else if (sigmodes[i] & SIG_SPECIAL)
        ((*this).*reset) (i);
      pending_traps[i] = 0; /* XXX */
    }

  /* Command substitution and other child processes don't inherit the
     debug, error, or return traps.  If we're in the debugger, and the
     `functrace' or `errtrace' options have been set, then let command
     substitutions inherit them.  Let command substitution inherit the
     RETURN trap if we're in the debugger and tracing functions. */
  if (function_trace_mode == 0)
    {
      sigmodes[DEBUG_TRAP] &= ~SIG_TRAPPED;
      sigmodes[RETURN_TRAP] &= ~SIG_TRAPPED;
    }
  if (error_trace_mode == 0)
    sigmodes[ERROR_TRAP] &= ~SIG_TRAPPED;
}

} // namespace bash
