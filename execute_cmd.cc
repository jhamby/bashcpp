/* execute_cmd.cc -- Execute a COMMAND structure. */

/* Copyright (C) 1987-2022 Free Software Foundation, Inc.

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

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include <fcntl.h>
#include <fnmatch.h>

namespace bash
{

/* Static functions defined and used in this file. */

#if defined(COMMAND_TIMING)
static void mkfmt (char *, int, bool, time_t, int);
#endif

static inline int builtin_status (int);
static inline char *getinterp (const char *, ssize_t, int *);
static inline void coproc_close (Coproc *);

/* A convenience macro to avoid resetting line_number_for_err_trap while
   running the ERR trap. */
#define SET_LINE_NUMBER(v)                                                    \
  do                                                                          \
    {                                                                         \
      line_number = v;                                                        \
      if (!signal_in_progress (ERROR_TRAP)                                    \
          && running_trap != (ERROR_TRAP + 1))                                \
        line_number_for_err_trap = line_number;                               \
    }                                                                         \
  while (0)

/* This can't be in executing_line_number() because that's used for LINENO
   and we want LINENO to reflect the line number of commands run during
   the ERR trap. Right now this is only used to push to BASH_LINENO. */
#define GET_LINE_NUMBER()                                                     \
  (signal_in_progress (ERROR_TRAP) && running_trap == ERROR_TRAP + 1)         \
      ? line_number_for_err_trap                                              \
      : executing_line_number ()

/* Return the line number of the currently executing command. */
int
Shell::executing_line_number ()
{
  if (executing && !showing_function_line
      && (variable_context == 0 || !interactive_shell)
      && currently_executing_command)
    {
      // XXX - should we only return this for some types of commands?
      return currently_executing_command->line;
    }
  else
    return line_number;
}

/* Execute the command passed in COMMAND.  COMMAND is exactly what
   read_command () places into GLOBAL_COMMAND.  See "command.h" for the
   details of the command structure.

   EXECUTION_SUCCESS or EXECUTION_FAILURE are the only possible
   return values.  Executing a command with nothing in it returns
   EXECUTION_SUCCESS. */
int
Shell::execute_command (COMMAND *command)
{
  current_fds_to_close = nullptr;
  fd_bitmap bitmap;

  /* Just do the command, but not asynchronously. */
  int result
      = execute_command_internal (command, false, NO_PIPE, NO_PIPE, &bitmap);

#if defined(PROCESS_SUBSTITUTION)
  /* don't unlink fifos if we're in a shell function; wait until the function
     returns. */
  if (variable_context == 0 && executing_list == 0)
    unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

  QUIT;
  return result;
}

/* Return true if TYPE is a shell control structure type. */

// Most command types are control structures, so the default is true.
bool
COMMAND::control_structure ()
{
  return true;
}

// Simple command isn't a control structure.
bool
SIMPLE_COM::control_structure ()
{
  return false;
}

// Subshell command isn't a control structure.
bool
SUBSHELL_COM::control_structure ()
{
  return false;
}

// Coprocess command isn't a control structure.
bool
COPROC_COM::control_structure ()
{
  return false;
}

#if 0
void
undo_partial_redirects ()
{
  if (redirection_undo_list)
    {
      cleanup_redirects (redirection_undo_list);
      redirection_undo_list = nullptr;
    }
}

/* Function to unwind_protect the redirections for functions and builtins. */
static void
cleanup_func_redirects (REDIRECT *list)
{
  do_redirections (list, RX_ACTIVE);
}

void
dispose_exec_redirects ()
{
  if (exec_redirection_undo_list)
    {
      dispose_redirects (exec_redirection_undo_list);
      exec_redirection_undo_list = nullptr;
    }
}

void
dispose_partial_redirects ()
{
  if (redirection_undo_list)
    {
      dispose_redirects (redirection_undo_list);
      redirection_undo_list = nullptr;
    }
}
#endif

#if defined(JOB_CONTROL)
#if 0
/* A function to restore the signal mask to its proper value when the shell
   is interrupted or errors occur while creating a pipeline. */
static void
restore_signal_mask (void *set)
{
  sigprocmask (SIG_SETMASK, (sigset_t *)set, nullptr);
}
#endif
#endif /* JOB_CONTROL */

#if defined(DEBUG) && 0
/* A debugging function that can be called from gdb, for instance. */
void
open_files ()
{
  int fd_table_size = getdtablesize ();

  fprintf (stderr, "pid %ld open files:", static_cast<long> (getpid ()));
  for (int i = 3; i < fd_table_size; i++)
    {
      int f;
      if ((f = fcntl (i, F_GETFD, 0)) != -1)
        fprintf (stderr, " %d (%s)", i, f ? "close" : "open");
    }
  fprintf (stderr, "\n");
}
#endif

void
Shell::async_redirect_stdin ()
{
  int fd;

  fd = open ("/dev/null", O_RDONLY);
  if (fd > 0)
    {
      dup2 (fd, 0);
      close (fd);
    }
  else if (fd < 0)
    internal_error (_ ("cannot redirect standard input from /dev/null: %s"),
                    strerror (errno));
}

#define DESCRIBE_PID(pid)                                                     \
  do                                                                          \
    {                                                                         \
      if (interactive)                                                        \
        describe_pid (pid);                                                   \
    }                                                                         \
  while (0)

/* Execute the command passed in COMMAND, perhaps doing it asynchronously.
   COMMAND is exactly what read_command () places into GLOBAL_COMMAND.
   ASYNCHRONOUS, if non-zero, says to do this command in the background.
   PIPE_IN and PIPE_OUT are file descriptors saying where input comes
   from and where it goes.  They can have the value of NO_PIPE, which means
   I/O is stdin/stdout.
   FDS_TO_CLOSE is a list of file descriptors to close once the child has
   been forked.  This list often contains the unusable sides of pipes, etc.

   EXECUTION_SUCCESS or EXECUTION_FAILURE are the only possible
   return values.  Executing a command with nothing in it returns
   EXECUTION_SUCCESS. */
int
Shell::execute_command_internal (COMMAND *command, bool asynchronous,
                                 int pipe_in, int pipe_out,
                                 fd_bitmap *fds_to_close)
{
#if defined(PROCESS_SUBSTITUTION)
  fifo_vector ofifo_list;
  bool saved_fifo;
#endif

  if (breaking || continuing || read_but_dont_execute)
    return last_command_exit_value;
  if (command == nullptr)
    return EXECUTION_SUCCESS;

  QUIT;
  run_pending_traps ();

  currently_executing_command = command;

  bool invert = (command->flags & CMD_INVERT_RETURN) != 0;

  /* If we're inverting the return value and `set -e' has been executed,
     we don't want a failing command to inadvertently cause the shell
     to exit. */
  if (exit_immediately_on_error && invert) /* XXX */
    command->flags |= CMD_IGNORE_RETURN;   /* XXX */

  bool is_subshell = (command->type == cm_subshell);

  /* If a command was being explicitly run in a subshell, or if it is
     a shell control-structure, and it has a pipe, then we do the command
     in a subshell. */
  if (is_subshell && (command->flags & CMD_NO_FORK))
    return execute_in_subshell (command, asynchronous, pipe_in, pipe_out,
                                fds_to_close);

#if defined(COPROCESS_SUPPORT)
  if (command->type == cm_coproc)
    return last_command_exit_value
           = execute_coproc (static_cast<COPROC_COM *> (command), pipe_in,
                             pipe_out, fds_to_close);
#endif

  bool user_subshell
      = is_subshell || ((command->flags & CMD_WANT_SUBSHELL) != 0);

#if defined(TIME_BEFORE_SUBSHELL)
  if ((command->flags & CMD_TIME_PIPELINE) && user_subshell && !asynchronous)
    {
      command->flags |= CMD_FORCE_SUBSHELL;
      int exec_result = time_command (command, asynchronous, pipe_in, pipe_out,
                                      fds_to_close);
      currently_executing_command = nullptr;
      return exec_result;
    }
#endif

  if (is_subshell
      || (command->flags & (CMD_WANT_SUBSHELL | CMD_FORCE_SUBSHELL))
      || (command->control_structure ()
          && (pipe_out != NO_PIPE || pipe_in != NO_PIPE || asynchronous)))
    {
      /* Fork a subshell, turn off the subshell bit, turn off job
         control and call execute_command () on the command again. */
      int save_line_number = line_number;

      if (is_subshell)
        SET_LINE_NUMBER (command->line); /* XXX - save value? */
      /* Otherwise we defer setting line_number */

      string tcmd (make_command_string (command));
      make_child_flags fork_flags = asynchronous ? FORK_ASYNC : FORK_SYNC;

      pid_t paren_pid = make_child (tcmd, fork_flags);

      if (user_subshell && signal_is_trapped (ERROR_TRAP)
          && !signal_in_progress (DEBUG_TRAP) && running_trap == 0)
        {
          the_printed_command_except_trap = the_printed_command;
        }

      if (paren_pid == 0)
        {
          /* We want to run the exit trap for forced {} subshells, and we
             want to note this before execute_in_subshell modifies the
             COMMAND struct.  Need to keep in mind that execute_in_subshell
             runs the exit trap for () subshells itself. */
          /* This handles { command; } & */
          bool s = !user_subshell && command->type == cm_group
                   && pipe_in == NO_PIPE && pipe_out == NO_PIPE
                   && asynchronous;
          /* run exit trap for : | { ...; } and { ...; } | : */
          /* run exit trap for : | ( ...; ) and ( ...; ) | : */
          s |= !user_subshell && command->type == cm_group
               && (pipe_in != NO_PIPE || pipe_out != NO_PIPE) && !asynchronous;

          last_command_exit_value = execute_in_subshell (
              command, asynchronous, pipe_in, pipe_out, fds_to_close);
          if (s)
            subshell_exit (last_command_exit_value);
          else
            sh_exit (last_command_exit_value);
          /* NOTREACHED */
        }
      else
        {
          close_pipes (pipe_in, pipe_out);

#if defined(PROCESS_SUBSTITUTION) && defined(HAVE_DEV_FD)
          if (variable_context == 0) /* wait until shell function completes */
            unlink_fifo_list ();
#endif
          /* If we are part of a pipeline, and not the end of the pipeline,
             then we should simply return and let the last command in the
             pipe be waited for.  If we are not in a pipeline, or are the
             last command in the pipeline, then we wait for the subshell
             and return its exit status as usual. */
          if (pipe_out != NO_PIPE)
            return EXECUTION_SUCCESS;

          stop_pipeline (asynchronous, nullptr);

          line_number = save_line_number;

          if (!asynchronous)
            {
              bool was_error_trap = signal_is_trapped (ERROR_TRAP)
                                    && signal_is_ignored (ERROR_TRAP) == 0;
              invert = (command->flags & CMD_INVERT_RETURN)
                       != 0; /* XXX - reload flag? */
              bool ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

              int exec_result = wait_for (paren_pid, 0);

              /* If we have to, invert the return value. */
              if (invert)
                exec_result
                    = ((exec_result == EXECUTION_SUCCESS) ? EXECUTION_FAILURE
                                                          : EXECUTION_SUCCESS);

              last_command_exit_value = exec_result;
              if (user_subshell && was_error_trap && !ignore_return && !invert
                  && exec_result != EXECUTION_SUCCESS)
                {
                  save_line_number = line_number;
                  line_number = line_number_for_err_trap;
                  run_error_trap ();
                  line_number = save_line_number;
                }

              if (user_subshell && !ignore_return && !invert
                  && exit_immediately_on_error
                  && exec_result != EXECUTION_SUCCESS)
                {
                  run_pending_traps ();
                  throw bash_exception (ERREXIT);
                }

              return last_command_exit_value;
            }
          else
            {
              DESCRIBE_PID (paren_pid);

              run_pending_traps ();

              /* Posix 2013 2.9.3.1: "the exit status of an asynchronous list
                 shall be zero." */
              last_command_exit_value = 0;
              return EXECUTION_SUCCESS;
            }
        }
    }

#if defined(COMMAND_TIMING)
  if (command->flags & CMD_TIME_PIPELINE)
    {
      int exec_result;
      if (asynchronous)
        {
          command->flags |= CMD_FORCE_SUBSHELL;
          exec_result = execute_command_internal (command, 1, pipe_in,
                                                  pipe_out, fds_to_close);
        }
      else
        {
          exec_result = time_command (command, asynchronous, pipe_in, pipe_out,
                                      fds_to_close);
#if 0
	  if (running_trap == 0)
#endif
          currently_executing_command = nullptr;
        }
      return exec_result;
    }
#endif /* COMMAND_TIMING */

  if (command->control_structure () && command->redirects)
    stdin_redir = stdin_redirects (command->redirects);

#if defined(PROCESS_SUBSTITUTION)
#if !defined(HAVE_DEV_FD)
  reap_procsubs ();
#endif

  /* XXX - also if sourcelevel != 0? */
  if (variable_context != 0 || executing_list)
    {
      ofifo_list = copy_fifo_list ();
      saved_fifo = true;
    }
  else
    saved_fifo = false;
#endif

  /* Handle WHILE FOR CASE etc. with redirections.  (Also '&' input
     redirection.)  */

  bool was_error_trap
      = signal_is_trapped (ERROR_TRAP) && !signal_is_ignored (ERROR_TRAP);

  bool ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

  if (do_redirections (command->redirects, RX_ACTIVE | RX_UNDOABLE) != 0)
    {
      undo_partial_redirects ();
      dispose_exec_redirects ();

      /* Handle redirection error as command failure if errexit set. */
      last_command_exit_value = EXECUTION_FAILURE;
      if (!ignore_return && !invert && pipe_in == NO_PIPE
          && pipe_out == NO_PIPE)
        {
          if (was_error_trap)
            {
              int save_line_number = line_number;
              line_number = line_number_for_err_trap;
              run_error_trap ();
              line_number = save_line_number;
            }
          if (exit_immediately_on_error)
            {
              run_pending_traps ();
              throw bash_exception (ERREXIT);
            }
        }
      return last_command_exit_value;
    }

  QUIT;

  REDIRECT *my_undo_list = redirection_undo_list;
  redirection_undo_list = nullptr;

  REDIRECT *exec_undo_list = exec_redirection_undo_list;
  exec_redirection_undo_list = nullptr;

  int exec_result = EXECUTION_SUCCESS;
  int save_line_number;

  try
    {
      switch (command->type)
        {
        case cm_simple:
          {
            save_line_number = line_number;
            /* We can't rely on variables retaining their values across a
               call to execute_simple_command if a throw occurs as the
               result of a `return' builtin.
             */
#if defined(RECYCLES_PIDS)
            last_made_pid = NO_PID;
#endif
            was_error_trap = signal_is_trapped (ERROR_TRAP)
                             && !signal_is_ignored (ERROR_TRAP);

            if (ignore_return)
              command->flags |= CMD_IGNORE_RETURN;

            SIMPLE_COM *simple_com = static_cast<SIMPLE_COM *> (command);

            SET_LINE_NUMBER (command->line);
            exec_result = execute_simple_command (
                simple_com, pipe_in, pipe_out, asynchronous, fds_to_close);
            line_number = save_line_number;

            /* The temporary environment should be used for only the simple
               command immediately following its definition. */
            dispose_used_env_vars ();

            /* If we forked to do the command, then we must wait_for ()
               the child. */

            /* XXX - this is something to watch out for if there are problems
               when the shell is compiled without job control.  Don't worry
               about whether or not last_made_pid == last_pid;
               already_making_children tells us whether or not there are
               unwaited-for children to wait for and reap. */
            if (already_making_children && pipe_out == NO_PIPE)
              {
                stop_pipeline (asynchronous, nullptr);

                if (asynchronous)
                  {
                    DESCRIBE_PID (last_made_pid);
                    exec_result = EXECUTION_SUCCESS;
                    invert = false; /* async commands always succeed */
                  }
                else
#if !defined(JOB_CONTROL)
                  /* Do not wait for asynchronous processes started from
                     startup files. */
                  if (last_made_pid != NO_PID
                      && last_made_pid != last_asynchronous_pid)
#else
                    if (last_made_pid != NO_PID)
#endif
                    /* When executing a shell function that executes other
                       commands, this causes the last simple command in
                       the function to be waited for twice.  This also causes
                       subshells forked to execute builtin commands (e.g., in
                       pipelines) to be waited for twice. */
                    exec_result = wait_for (last_made_pid, 0);
              }
          }

          /* 2009/02/13 -- pipeline failure is processed elsewhere.  This
             handles only the failure of a simple command. We don't want to run
             the error trap if the command run by the `command' builtin fails;
             we want to defer that until the command builtin itself returns
             failure. */
          /* 2020/07/14 -- this changes with how the command builtin is handled
           */
          if (was_error_trap && !ignore_return && !invert && pipe_in == NO_PIPE
              && pipe_out == NO_PIPE
              && (command->flags & CMD_COMMAND_BUILTIN) == 0
              && exec_result != EXECUTION_SUCCESS)
            {
              last_command_exit_value = exec_result;
              line_number = line_number_for_err_trap;
              run_error_trap ();
              line_number = save_line_number;
            }

          if (!ignore_return && !invert
              && ((posixly_correct && !interactive && special_builtin_failed)
                  || (exit_immediately_on_error && pipe_in == NO_PIPE
                      && pipe_out == NO_PIPE
                      && exec_result != EXECUTION_SUCCESS)))
            {
              last_command_exit_value = exec_result;
              run_pending_traps ();

              /* The catch block will undo the redirections and rethrow. */
              throw bash_exception (ERREXIT);
            }

          break;

        case cm_for:
          if (ignore_return)
            command->flags |= CMD_IGNORE_RETURN;
          exec_result
              = execute_for_command (static_cast<FOR_SELECT_COM *> (command));
          break;

#if defined(ARITH_FOR_COMMAND)
        case cm_arith_for:
          if (ignore_return)
            command->flags |= CMD_IGNORE_RETURN;
          exec_result = execute_arith_for_command (
              static_cast<ARITH_FOR_COM *> (command));
          break;
#endif

#if defined(SELECT_COMMAND)
        case cm_select:
          if (ignore_return)
            command->flags |= CMD_IGNORE_RETURN;
          exec_result = execute_select_command (
              static_cast<FOR_SELECT_COM *> (command));
          break;
#endif

        case cm_case:
          if (ignore_return)
            command->flags |= CMD_IGNORE_RETURN;
          exec_result
              = execute_case_command (static_cast<CASE_COM *> (command));
          break;

        case cm_while:
        case cm_until:
          if (ignore_return)
            command->flags |= CMD_IGNORE_RETURN;
          exec_result = execute_while_or_until (
              static_cast<UNTIL_WHILE_COM *> (command));
          break;

        case cm_if:
          if (ignore_return)
            command->flags |= CMD_IGNORE_RETURN;
          exec_result = execute_if_command (static_cast<IF_COM *> (command));
          break;

        case cm_group:

          /* This code can be executed from either of two paths: an explicit
             '{}' command, or via a function call.  If we are executed via a
             function call, we have already taken care of the function being
             executed in the background (down there in execute_simple_command
             ()), and this command should *not* be marked as asynchronous.  If
             we are executing a regular '{}' group command, and asynchronous ==
             1, we must want to execute the whole command in the background, so
             we need a subshell, and we want the stuff executed in that
             subshell (this group command) to be executed in the foreground of
             that subshell (i.e. there will not be *another* subshell forked).

             What we do is to force a subshell if asynchronous, and then call
             execute_command_internal again with asynchronous still set to 1,
             but with the original group command, so the printed command will
             look right.

             The code above that handles forking off subshells will note that
             both subshell and async are on, and turn off async in the child
             after forking the subshell (but leave async set in the parent, so
             the normal call to describe_pid is made).  This turning off
             async is *crucial*; if it is not done, this will fall into an
             infinite loop of executions through this spot in subshell after
             subshell until the process limit is exhausted. */

          if (asynchronous)
            {
              command->flags |= CMD_FORCE_SUBSHELL;
              exec_result = execute_command_internal (command, true, pipe_in,
                                                      pipe_out, fds_to_close);
            }
          else
            {
              GROUP_COM *group_com = static_cast<GROUP_COM *> (command);
              if (ignore_return && group_com->command)
                group_com->command->flags |= CMD_IGNORE_RETURN;

              exec_result
                  = execute_command_internal (group_com->command, asynchronous,
                                              pipe_in, pipe_out, fds_to_close);
            }
          break;

        case cm_connection:
          exec_result = execute_connection (
              static_cast<CONNECTION *> (command), asynchronous, pipe_in,
              pipe_out, fds_to_close);

          if (asynchronous)
            invert = false; /* XXX */
          break;

#if defined(DPAREN_ARITHMETIC)
        case cm_arith:
#endif
#if defined(COND_COMMAND)
        case cm_cond:
#endif
        case cm_function_def:
          was_error_trap = signal_is_trapped (ERROR_TRAP)
                           && !signal_is_ignored (ERROR_TRAP);
#if defined(DPAREN_ARITHMETIC)
          if (ignore_return && command->type == cm_arith)
            command->flags |= CMD_IGNORE_RETURN;
#endif
#if defined(COND_COMMAND)
          if (ignore_return && command->type == cm_cond)
            command->flags |= CMD_IGNORE_RETURN;
#endif

          line_number_for_err_trap = save_line_number = line_number; // XXX
#if defined(DPAREN_ARITHMETIC)
          if (command->type == cm_arith)
            exec_result
                = execute_arith_command (static_cast<ARITH_COM *> (command));
          else
#endif
#if defined(COND_COMMAND)
              if (command->type == cm_cond)
            exec_result
                = execute_cond_command (static_cast<COND_COM *> (command));
          else
#endif
              if (command->type == cm_function_def)
            {
              FUNCTION_DEF *fndef = static_cast<FUNCTION_DEF *> (command);
              exec_result = execute_intern_function (fndef->name, fndef);
            }

          line_number = save_line_number;

          if (was_error_trap && !ignore_return && !invert
              && exec_result != EXECUTION_SUCCESS)
            {
              last_command_exit_value = exec_result;
              save_line_number = line_number;
              line_number = line_number_for_err_trap;
              run_error_trap ();
              line_number = save_line_number;
            }

          if (!ignore_return && !invert && exit_immediately_on_error
              && exec_result != EXECUTION_SUCCESS)
            {
              last_command_exit_value = exec_result;
              run_pending_traps ();
              throw bash_exception (ERREXIT);
            }

          break;

        case cm_subshell:
        case cm_coproc:
        default:
          command_error ("execute_command", CMDERR_BADTYPE, command->type);
        }
    }
  catch (const std::exception &)
    {
      if (my_undo_list)
        cleanup_redirects (my_undo_list);

      if (exec_undo_list)
        delete exec_undo_list;

      throw;
    }

  if (my_undo_list)
    cleanup_redirects (my_undo_list);

  if (exec_undo_list)
    delete exec_undo_list;

#if defined(PROCESS_SUBSTITUTION)
  if (saved_fifo)
    close_new_fifos (ofifo_list);
#endif

  /* Invert the return value if we have to */
  if (invert)
    exec_result = (exec_result == EXECUTION_SUCCESS) ? EXECUTION_FAILURE
                                                     : EXECUTION_SUCCESS;

#if defined(DPAREN_ARITHMETIC) || defined(COND_COMMAND)

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

  /* This is where we set PIPESTATUS from the exit status of the appropriate
     compound commands (the ones that look enough like simple commands to
     cause confusion).  We might be able to optimize by not doing this if
     subshell_environment != 0. */
  switch (command->type)
    {
#if defined(DPAREN_ARITHMETIC)
    case cm_arith:
#endif
#if defined(COND_COMMAND)
    case cm_cond:
#endif
      set_pipestatus_from_exit (exec_result);
      break;
    default:
      break;
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif

  last_command_exit_value = exec_result;
  run_pending_traps ();
  currently_executing_command = nullptr;

  return last_command_exit_value;
}

#if defined(COMMAND_TIMING)

#if defined(HAVE_GETRUSAGE) && defined(HAVE_GETTIMEOFDAY)
extern struct timeval *difftimeval (struct timeval *, struct timeval *,
                                    struct timeval *);
extern struct timeval *addtimeval (struct timeval *, struct timeval *,
                                   struct timeval *);
extern int timeval_to_cpu (struct timeval *, struct timeval *,
                           struct timeval *);
#endif

#define POSIX_TIMEFORMAT "real %2R\nuser %2U\nsys %2S"
#define BASH_TIMEFORMAT "\nreal\t%3lR\nuser\t%3lU\nsys\t%3lS"

static const int precs[] = { 0, 100, 10, 1 };

/* Expand one `%'-prefixed escape sequence from a time format string. */
static void
mkfmt (char *buf, int prec, bool lng, time_t sec, int sec_fraction)
{
  time_t min;
  char abuf[INT_STRLEN_BOUND (time_t) + 1];
  int ind, aind;

  ind = 0;
  abuf[sizeof (abuf) - 1] = '\0';

  /* If LNG is non-zero, we want to decompose SEC into minutes and seconds. */
  if (lng)
    {
      min = sec / 60;
      sec %= 60;
      aind = sizeof (abuf) - 2;
      do
        abuf[aind--] = (min % 10) + '0';
      while (min /= 10);
      aind++;
      while (abuf[aind])
        buf[ind++] = abuf[aind++];
      buf[ind++] = 'm';
    }

  /* Now add the seconds. */
  aind = sizeof (abuf) - 2;
  do
    abuf[aind--] = (sec % 10) + '0';
  while (sec /= 10);
  aind++;
  while (abuf[aind])
    buf[ind++] = abuf[aind++];

  /* We want to add a decimal point and PREC places after it if PREC is
     nonzero.  PREC is not greater than 3.  SEC_FRACTION is between 0
     and 999. */
  if (prec != 0)
    {
      buf[ind++] = locale_decpoint ();
      for (aind = 1; aind <= prec; aind++)
        {
          buf[ind++] = static_cast<char> (sec_fraction / precs[aind]) + '0';
          sec_fraction %= precs[aind];
        }
    }

  if (lng)
    buf[ind++] = 's';
  buf[ind] = '\0';
}

/* Interpret the format string FORMAT, interpolating the following escape
   sequences:
                %[prec][l][RUS]

   where the optional `prec' is a precision, meaning the number of
   characters after the decimal point, the optional `l' means to format
   using minutes and seconds (MMmNN[.FF]s), like the `times' builtin',
   and the last character is one of

                R	number of seconds of `real' time
                U	number of seconds of `user' time
                S	number of seconds of `system' time

   An occurrence of `%%' in the format string is translated to a `%'.  The
   result is printed to FP, a pointer to a FILE.  The other variables are
   the seconds and thousandths of a second of real, user, and system time,
   resectively. */
void
Shell::print_formatted_time (FILE *fp, const char *format, time_t rs, int rsf,
                             time_t us, int usf, time_t ss, int ssf, int cpu)
{
  char ts[INT_STRLEN_BOUND (time_t) + sizeof ("mSS.FFFF")];
  time_t sum;
  int sum_frac;

  size_t len = strlen (format);
  size_t ssize = (len + 64) - (len % 64);

  string str;
  str.reserve (ssize);

  for (const char *s = format; *s; s++)
    {
      if (*s != '%' || s[1] == '\0')
        str.push_back (*s);
      else if (s[1] == '%')
        {
          s++;
          str.push_back (*s);
        }
      else if (s[1] == 'P')
        {
          s++;
#if 0
	  /* clamp CPU usage at 100% */
	  if (cpu > 10000)
	    cpu = 10000;
#endif
          sum = cpu / 100;
          sum_frac = (cpu % 100) * 10;
          mkfmt (ts, 2, 0, sum, sum_frac);
          str += ts;
        }
      else
        {
          int prec = 3;     // default is three places past the decimal point.
          bool lng = false; // default is to not use minutes or append `s'
          s++;
          if (c_isdigit (*s)) /* `precision' */
            {
              prec = *s++ - '0';
              if (prec > 3)
                prec = 3;
            }
          if (*s == 'l') /* `length extender' */
            {
              lng = true;
              s++;
            }
          if (*s == 'R' || *s == 'E')
            mkfmt (ts, prec, lng, rs, rsf);
          else if (*s == 'U')
            mkfmt (ts, prec, lng, us, usf);
          else if (*s == 'S')
            mkfmt (ts, prec, lng, ss, ssf);
          else
            {
              internal_error (_ ("TIMEFORMAT: `%c': invalid format character"),
                              *s);
              return;
            }
          str += ts;
        }
    }

  fprintf (fp, "%s\n", str.c_str ());
  fflush (fp);
}

int
Shell::time_command (COMMAND *command, bool asynchronous, int pipe_in,
                     int pipe_out, fd_bitmap *fds_to_close)
{
#if defined(HAVE_GETRUSAGE) && defined(HAVE_GETTIMEOFDAY)
  struct timeval real, user, sys;
  struct timeval before, after;
#if defined(HAVE_STRUCT_TIMEZONE)
  struct timezone dtz; /* posix doesn't define this */
#endif
  struct rusage selfb, selfa, kidsb, kidsa; /* a = after, b = before */
#else
#if defined(HAVE_TIMES)
  clock_t tbefore, tafter, real, user, sys;
  struct tms before, after;
#endif
#endif

#if defined(HAVE_GETRUSAGE) && defined(HAVE_GETTIMEOFDAY)
#if defined(HAVE_STRUCT_TIMEZONE)
  gettimeofday (&before, &dtz);
#else
  gettimeofday (&before, nullptr);
#endif /* !HAVE_STRUCT_TIMEZONE */
  getrusage (RUSAGE_SELF, &selfb);
  getrusage (RUSAGE_CHILDREN, &kidsb);
#else
#if defined(HAVE_TIMES)
  tbefore = times (&before);
#endif
#endif

  int old_subshell = subshell_environment;
  bool posix_time = command && (command->flags & CMD_TIME_POSIX);

  bool nullcmd = (command == nullptr);
  if (!nullcmd && command->type == cm_simple)
    {
      SIMPLE_COM *simple_com = static_cast<SIMPLE_COM *> (command);
      if (simple_com->words == nullptr
          && simple_com->simple_redirects == nullptr)
        nullcmd = true;
    }

  if (posixly_correct && nullcmd)
    {
#if defined(HAVE_GETRUSAGE)
      selfb.ru_utime.tv_sec = kidsb.ru_utime.tv_sec = selfb.ru_stime.tv_sec
          = kidsb.ru_stime.tv_sec = 0;
      selfb.ru_utime.tv_usec = kidsb.ru_utime.tv_usec = selfb.ru_stime.tv_usec
          = kidsb.ru_stime.tv_usec = 0;
      before = shellstart;
#else
      before.tms_utime = before.tms_stime = before.tms_cutime
          = before.tms_cstime = 0;
      tbefore = shell_start_time;
#endif
    }

  cmd_flags old_flags = command->flags;
  command->flags &= ~(CMD_TIME_PIPELINE | CMD_TIME_POSIX);

  int rv;
  bash_exception_t exc_type = NOEXCEPTION;
  try
    {
      rv = execute_command_internal (command, asynchronous, pipe_in, pipe_out,
                                     fds_to_close);
    }
  catch (const bash_exception &e)
    {
      exc_type = e.type;
    }

  command->flags = old_flags;

  /* If we're catching from a different subshell environment than we started,
     don't bother printing timing stats, just rethrow back to the original top
     level. */
  if (exc_type != NOEXCEPTION && subshell_environment
      && subshell_environment != old_subshell)
    throw bash_exception (exc_type);

  time_t rs = 0, us = 0, ss = 0;
  int rsf = 0, usf = 0, ssf = 0, cpu = 0;

#if defined(HAVE_GETRUSAGE) && defined(HAVE_GETTIMEOFDAY)
#if defined(HAVE_STRUCT_TIMEZONE)
  gettimeofday (&after, &dtz);
#else
  gettimeofday (&after, nullptr);
#endif /* !HAVE_STRUCT_TIMEZONE */
  getrusage (RUSAGE_SELF, &selfa);
  getrusage (RUSAGE_CHILDREN, &kidsa);

  difftimeval (&real, &before, &after);
  timeval_to_secs (&real, &rs, &rsf);

  addtimeval (&user, difftimeval (&after, &selfb.ru_utime, &selfa.ru_utime),
              difftimeval (&before, &kidsb.ru_utime, &kidsa.ru_utime));
  timeval_to_secs (&user, &us, &usf);

  addtimeval (&sys, difftimeval (&after, &selfb.ru_stime, &selfa.ru_stime),
              difftimeval (&before, &kidsb.ru_stime, &kidsa.ru_stime));
  timeval_to_secs (&sys, &ss, &ssf);

  cpu = timeval_to_cpu (&real, &user, &sys);
#else
#if defined(HAVE_TIMES)
  tafter = times (&after);

  real = tafter - tbefore;
  clock_t_to_secs (real, &rs, &rsf);

  user = (after.tms_utime - before.tms_utime)
         + (after.tms_cutime - before.tms_cutime);
  clock_t_to_secs (user, &us, &usf);

  sys = (after.tms_stime - before.tms_stime)
        + (after.tms_cstime - before.tms_cstime);
  clock_t_to_secs (sys, &ss, &ssf);

  cpu = (real == 0) ? 0 : ((user + sys) * 10000) / real;

#else
  rs = us = ss = 0;
  rsf = usf = ssf = cpu = 0;
#endif
#endif

  const char *time_format = nullptr;
  const string *time_str;
  if (posix_time)
    time_format = POSIX_TIMEFORMAT;
  else if ((time_str = get_string_value ("TIMEFORMAT")) == nullptr)
    {
      if (posixly_correct && nullcmd)
        time_format = "user\t%2lU\nsys\t%2lS";
      else
        time_format = BASH_TIMEFORMAT;
    }
  else
    time_format = time_str->c_str ();

  if (time_format && *time_format)
    print_formatted_time (stderr, time_format, rs, rsf, us, usf, ss, ssf, cpu);

  if (exc_type)
    throw bash_exception (exc_type);

  return rv;
}
#endif /* COMMAND_TIMING */

/* Execute a command that's supposed to be in a subshell.  This must be
   called after make_child and we must be running in the child process.
   The caller will return or exit() immediately with the value this returns. */
int
Shell::execute_in_subshell (COMMAND *command, bool asynchronous, int pipe_in,
                            int pipe_out, fd_bitmap *fds_to_close)
{
  subshell_level++;
  bool should_redir_stdin
      = (asynchronous && (command->flags & CMD_STDIN_REDIR)
         && pipe_in == NO_PIPE && stdin_redirects (command->redirects) == 0);

  bool invert = (command->flags & CMD_INVERT_RETURN) != 0;
  const bool user_subshell = command->type == cm_subshell
                             || ((command->flags & CMD_WANT_SUBSHELL) != 0);
  const bool user_coproc = command->type == cm_coproc;

  command->flags
      &= ~(CMD_FORCE_SUBSHELL | CMD_WANT_SUBSHELL | CMD_INVERT_RETURN);

  /* If a command is asynchronous in a subshell (like ( foo ) & or
     the special case of an asynchronous GROUP command where the
     the subshell bit is turned on down in case cm_group: below),
     turn off `asynchronous', so that two subshells aren't spawned.
     XXX - asynchronous used to be set to 0 in this block, but that
     means that setup_async_signals was never run.  Now it's set to
     0 after subshell_environment is set appropriately and setup_async_signals
     is run.

     This seems semantically correct to me.  For example,
     ( foo ) & seems to say ``do the command `foo' in a subshell
     environment, but don't wait for that subshell to finish'',
     and "{ foo ; bar ; } &" seems to me to be like functions or
     builtins in the background, which executed in a subshell
     environment.  I just don't see the need to fork two subshells. */

  /* Don't fork again, we are already in a subshell.  A `doubly
     async' shell is not interactive, however. */
  if (asynchronous)
    {
#if defined(JOB_CONTROL)
      /* If a construct like ( exec xxx yyy ) & is given while job
         control is active, we want to prevent exec from putting the
         subshell back into the original process group, carefully
         undoing all the work we just did in make_child. */
      original_pgrp = -1;
#endif /* JOB_CONTROL */
      bool ois = interactive_shell;
      interactive_shell = false;
      /* This test is to prevent alias expansion by interactive shells that
         run `(command) &' but to allow scripts that have enabled alias
         expansion with `shopt -s expand_alias' to continue to expand
         aliases. */
      if (ois != interactive_shell)
        expand_aliases = 0;
    }

  /* Subshells are neither login nor interactive. */
  login_shell = 0;
  interactive = false;

  /* And we're no longer in a loop. See Posix interp 842 (we are not in the
     "same execution environment"). */
  if (shell_compatibility_level > 44)
    loop_level = 0;

  if (user_subshell)
    {
      subshell_environment = SUBSHELL_PAREN; /* XXX */
      if (asynchronous)
        subshell_environment |= SUBSHELL_ASYNC;
    }
  else
    {
      subshell_environment = SUBSHELL_NOFLAGS; /* XXX */
      if (asynchronous)
        subshell_environment |= SUBSHELL_ASYNC;
      if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
        subshell_environment |= SUBSHELL_PIPE;
      if (user_coproc)
        subshell_environment |= SUBSHELL_COPROC;
    }

  QUIT;

  reset_terminating_signals (); /* in sig.cc */

  /* Cancel traps, in trap.cc. */
  /* Reset the signal handlers in the child, but don't free the
     trap strings.  Set a flag noting that we have to free the
     trap strings if we run trap to change a signal disposition. */
  clear_pending_traps ();
  reset_signal_handlers ();
  subshell_environment |= SUBSHELL_RESETTRAP;

  /* Note that signal handlers have been reset, so we should no longer
    reset the handler and resend trapped signals to ourselves. */
  subshell_environment &= ~SUBSHELL_IGNTRAP;

  /* We are in a subshell, so forget that we are running a trap handler or
     that the signal handler has changed (we haven't changed it!) */
  /* XXX - maybe do this for `real' signals and not ERR/DEBUG/RETURN/EXIT
     traps? */
  if (running_trap > 0)
    {
      run_trap_cleanup (running_trap - 1);
      running_trap = 0; /* XXX - maybe leave this */
    }

  /* Make sure restore_original_signals doesn't undo the work done by
     make_child to ensure that asynchronous children are immune to SIGINT
     and SIGQUIT.  Turn off asynchronous to make sure more subshells are
     not spawned. */
  if (asynchronous)
    {
      setup_async_signals ();
      asynchronous = false;
    }
  else
    set_sigint_handler ();

#if defined(JOB_CONTROL)
  set_sigchld_handler ();
#endif /* JOB_CONTROL */

  /* Delete all traces that there were any jobs running.  This is
     only for subshells. */
  without_job_control ();

  if (fds_to_close)
    close_fd_bitmap (*fds_to_close);

  do_piping (pipe_in, pipe_out);

#if defined(COPROCESS_SUPPORT)
  coproc_closeall ();
#endif

#if defined(PROCESS_SUBSTITUTION)
  clear_fifo_list (); /* XXX- we haven't created any FIFOs */
#endif

  /* If this is a user subshell, set a flag if stdin was redirected.
     This is used later to decide whether to redirect fd 0 to
     /dev/null for async commands in the subshell.  This adds more
     sh compatibility, but I'm not sure it's the right thing to do.
     Note that an input pipe to a compound command suffices to inhibit
     the implicit /dev/null redirection for asynchronous commands
     executed as part of that compound command. */
  if (user_subshell)
    {
      stdin_redir
          = (stdin_redirects (command->redirects) || pipe_in != NO_PIPE);
#if 0
      restore_default_signal (EXIT_TRAP); // XXX - reset_signal_handlers above
#endif
    }
  else if (command->control_structure () && pipe_in != NO_PIPE)
    stdin_redir = true;

  /* If this is an asynchronous command (command &), we want to
     redirect the standard input from /dev/null in the absence of
     any specific redirection involving stdin. */
  if (should_redir_stdin && !stdin_redir)
    async_redirect_stdin ();

#if defined(BUFFERED_INPUT)
  /* In any case, we are not reading our command input from stdin. */
  default_buffered_input = -1;
#endif

  /* We can't optimize away forks if one of the commands executed by the
     subshell sets an exit trap, so we set CMD_NO_FORK for simple commands
     and set CMD_TRY_OPTIMIZING for simple commands on the right side of an
     and-or or `;' list to test for optimizing forks when they are executed. */
  if (user_subshell && command->type == cm_subshell)
    optimize_subshell_command (static_cast<SUBSHELL_COM *> (command)->command);

  /* Do redirections, then dispose of them before recursive call. */
  if (command->redirects)
    {
      if (do_redirections (command->redirects, RX_ACTIVE) != 0)
        exit (invert ? EXECUTION_SUCCESS : EXECUTION_FAILURE);

      delete command->redirects;
      command->redirects = nullptr;
    }

  COMMAND *tcom;
  if (command->type == cm_subshell)
    tcom = static_cast<SUBSHELL_COM *> (command)->command;
  else if (user_coproc)
    tcom = static_cast<COPROC_COM *> (command)->command;
  else
    tcom = command;

  if (command->flags & CMD_TIME_PIPELINE)
    tcom->flags |= CMD_TIME_PIPELINE;
  if (command->flags & CMD_TIME_POSIX)
    tcom->flags |= CMD_TIME_POSIX;

  /* Make sure the subshell inherits any CMD_IGNORE_RETURN flag. */
  if ((command->flags & CMD_IGNORE_RETURN) && tcom != command)
    tcom->flags |= CMD_IGNORE_RETURN;

  /* If this is a simple command, tell execute_disk_command that it
     might be able to get away without forking and simply exec.
     This means things like ( sleep 10 ) will only cause one fork.
     If we're timing the command or inverting its return value, however,
     we cannot do this optimization. */
  if ((user_subshell || user_coproc)
      && (tcom->type == cm_simple || tcom->type == cm_subshell)
      && ((tcom->flags & (CMD_TIME_PIPELINE | CMD_INVERT_RETURN)) == 0))
    {
      tcom->flags |= CMD_NO_FORK;
    }

  invert = (tcom->flags & CMD_INVERT_RETURN) != 0;
  tcom->flags &= ~CMD_INVERT_RETURN;

  int return_code;

  try
    {
      return_code = execute_command_internal (tcom, asynchronous, NO_PIPE,
                                              NO_PIPE, fds_to_close);
    }
  catch (const bash_exception &e)
    {
      /* If we're going to exit the shell, we don't want to invert the return
         status. */
      if (e.type == EXITPROG || e.type == EXITBLTIN)
        {
          invert = false;
          return_code = last_command_exit_value;
        }
      else if (e.type)
        return_code = (last_command_exit_value == EXECUTION_SUCCESS)
                          ? EXECUTION_FAILURE
                          : last_command_exit_value;
    }

  /* If we are asked to, invert the return value. */
  if (invert)
    return_code = (return_code == EXECUTION_SUCCESS) ? EXECUTION_FAILURE
                                                     : EXECUTION_SUCCESS;

  /* If we were explicitly placed in a subshell with (), we need
     to do the `shell cleanup' things, such as running traps[0]. */
  if (user_subshell && signal_is_trapped (0))
    {
      last_command_exit_value = return_code;
      return_code = run_exit_trap ();
    }

#if 0
  subshell_level--;		/* don't bother, caller will just exit */
#endif
  return return_code;
}

#if defined(COPROCESS_SUPPORT)

struct cplist_t
{
  vector<Coproc *> list;
  int lock;
};

/* Functions to manage the list of coprocs */

#if MULTIPLE_COPROCS
static void
cpl_reap ()
{
  struct cpelement *p, *next, *nh, *nt;

  /* Build a new list by removing dead coprocs and fix up the coproc_list
     pointers when done. */
  nh = nt = next = (struct cpelement *)0;
  for (p = coproc_list.head; p; p = next)
    {
      next = p->next;
      if (p->coproc->c_flags & COPROC_DEAD)
        {
          coproc_list
              .ncoproc--; /* keep running count, fix up pointers later */

          internal_debug ("cpl_reap: deleting %d", p->coproc->c_pid);

          coproc_dispose (p->coproc);
          cpe_dispose (p);
        }
      else if (nh == 0)
        nh = nt = p;
      else
        {
          nt->next = p;
          nt = nt->next;
        }
    }

  if (coproc_list.ncoproc == 0)
    coproc_list.head = coproc_list.tail = 0;
  else
    {
      if (nt)
        nt->next = 0;
      coproc_list.head = nh;
      coproc_list.tail = nt;
      if (coproc_list.ncoproc == 1)
        coproc_list.tail = coproc_list.head; /* just to make sure */
    }
}

/* Clear out the list of saved statuses */
static void
cpl_flush ()
{
  struct cpelement *cpe, *p;

  for (cpe = coproc_list.head; cpe;)
    {
      p = cpe;
      cpe = cpe->next;

      coproc_dispose (p->coproc);
      cpe_dispose (p);
    }

  coproc_list.head = coproc_list.tail = 0;
  coproc_list.ncoproc = 0;
}

static void
cpl_closeall ()
{
  struct cpelement *cpe;

  for (cpe = coproc_list.head; cpe; cpe = cpe->next)
    coproc_close (cpe->coproc);
}

static void
cpl_fdchk (int fd)
{
  struct cpelement *cpe;

  for (cpe = coproc_list.head; cpe; cpe = cpe->next)
    coproc_checkfd (cpe->coproc, fd);
}

/* Search for PID in the list of coprocs; return the cpelement struct if
   found.  If not found, return nullptr. */
static struct cpelement *
cpl_search (pid_t pid)
{
  struct cpelement *cpe;

  for (cpe = coproc_list.head; cpe; cpe = cpe->next)
    if (cpe->coproc->c_pid == pid)
      return cpe;
  return nullptr;
}

/* Search for the coproc named NAME in the list of coprocs; return the
   cpelement struct if found.  If not found, return NULL. */
static struct cpelement *
cpl_searchbyname (const char *name)
{
  struct cpelement *cp;

  for (cp = coproc_list.head; cp; cp = cp->next)
    if (STREQ (cp->coproc->c_name, name))
      return cp;
  return nullptr;
}

static pid_t
cpl_firstactive ()
{
  struct cpelement *cpe;

  for (cpe = coproc_list.head; cpe; cpe = cpe->next)
    if ((cpe->coproc->c_flags & COPROC_DEAD) == 0)
      return cpe->coproc->c_pid;
  return (pid_t)NO_PID;
}
#endif

/* These currently use a single global "shell coproc" but are written in a
   way to not preclude additional coprocs later. */

Coproc *
Shell::getcoprocbypid (pid_t pid)
{
#if MULTIPLE_COPROCS
  return cpl_search (pid);
#else
  return pid == sh_coproc.c_pid ? &sh_coproc : nullptr;
#endif
}

Coproc *
Shell::getcoprocbyname (const string &name)
{
#if MULTIPLE_COPROCS
  return cpl_searchbyname (name);
#else
  return (sh_coproc.c_name == name) ? &sh_coproc : nullptr;
#endif
}

static inline void
coproc_init (Coproc *cp)
{
  cp->c_name.clear ();
  cp->c_pid = NO_PID;
  cp->c_rfd = cp->c_wfd = -1;
  cp->c_rsave = cp->c_wsave = -1;
  cp->c_flags = COPROC_UNSET;
  cp->c_status = COPROC_UNSET;
  cp->c_lock = 0;
}

Coproc *
Shell::coproc_alloc (const string &name, pid_t pid)
{
  Coproc *cp;

#if MULTIPLE_COPROCS
  cp = new Coproc ();
#else
  cp = &sh_coproc;
#endif
  coproc_init (cp);
  cp->c_lock = 2;

  cp->c_pid = pid;
  cp->c_name = name;
#if MULTIPLE_COPROCS
  cpl_add (cp);
#endif
  cp->c_lock = 0;
  return cp;
}

void
Shell::coproc_dispose (Coproc *cp)
{
  sigset_t set, oset;

  if (cp == nullptr)
    return;

  BLOCK_SIGNAL (SIGCHLD, set, oset);
  cp->c_lock = 3;
  coproc_unsetvars (cp);
  cp->c_name.clear ();
  coproc_close (cp);
#if MULTIPLE_COPROCS
  coproc_free (cp);
#else
  coproc_init (cp);
  cp->c_lock = 0;
#endif
  UNBLOCK_SIGNAL (oset);
}

/* Placeholder for now.  Will require changes for multiple coprocs */
void
Shell::coproc_flush ()
{
#if MULTIPLE_COPROCS
  cpl_flush ();
#else
  coproc_dispose (&sh_coproc);
#endif
}

static inline void
coproc_close (Coproc *cp)
{
  if (cp->c_rfd >= 0)
    {
      close (cp->c_rfd);
      cp->c_rfd = -1;
    }
  if (cp->c_wfd >= 0)
    {
      close (cp->c_wfd);
      cp->c_wfd = -1;
    }
  cp->c_rsave = cp->c_wsave = -1;
}

void
Shell::coproc_closeall ()
{
#if MULTIPLE_COPROCS
  cpl_closeall ();
#else
  /* XXX - will require changes for multiple coprocs */
  coproc_close (&sh_coproc);
#endif
}

void
Shell::coproc_reap ()
{
#if MULTIPLE_COPROCS
  cpl_reap ();
#else
  Coproc *cp;

  cp = &sh_coproc; /* XXX - will require changes for multiple coprocs */
  if (cp && (cp->c_flags & COPROC_DEAD))
    coproc_dispose (cp);
#endif
}

void
Shell::coproc_checkfd (Coproc *cp, int fd)
{
  bool update = false;

  if (cp->c_rfd >= 0 && cp->c_rfd == fd)
    {
      cp->c_rfd = -1;
      update = true;
    }
  if (cp->c_wfd >= 0 && cp->c_wfd == fd)
    {
      cp->c_wfd = -1;
      update = true;
    }
  if (update)
    coproc_setvars (cp);
}

void
Shell::coproc_fdchk (int fd)
{
#if MULTIPLE_COPROCS
  cpl_fdchk (fd);
#else
  coproc_checkfd (&sh_coproc, fd);
#endif
}

static inline void
coproc_setstatus (Coproc *cp, coproc_status status)
{
  cp->c_lock = 4;
  cp->c_status = status;
  cp->c_flags = COPROC_DEAD;
  /* Don't dispose the coproc or unset the COPROC_XXX variables because
     this is executed in a signal handler context.  Wait until coproc_reap
     takes care of it. */
  cp->c_lock = 0;
}

void
Shell::coproc_pidchk (pid_t pid, coproc_status status)
{
  Coproc *cp;

#if MULTIPLE_COPROCS
  struct cpelement *cpe;

  /* We're not disposing the coproc because this is executed in a signal
     handler context */
  cpe = cpl_search (pid);
  cp = cpe ? cpe->coproc : 0;
#else
  cp = getcoprocbypid (pid);
#endif
  if (cp)
    coproc_setstatus (cp, status);
}

pid_t
Shell::coproc_active ()
{
#if MULTIPLE_COPROCS
  return cpl_firstactive ();
#else
  return (sh_coproc.c_flags & COPROC_DEAD) ? NO_PID : sh_coproc.c_pid;
#endif
}

void
Shell::coproc_setvars (Coproc *cp)
{
  SHELL_VAR *v;
#if defined(ARRAY_VARS)
  arrayind_t ind;
#endif

  if (cp->c_name.empty ())
    return;

  /* We could do more here but right now we only check the name, warn if it's
     not a valid identifier, and refuse to create variables with invalid names
     if a coproc with such a name is supplied. */

  WORD_DESC w (cp->c_name);

  if (!check_identifier (&w, true))
    return;

  string t, namevar;

#if defined(ARRAY_VARS)
  v = find_variable (cp->c_name);

  /* This is the same code as in find_or_make_array_variable */
  if (v == nullptr)
    {
      try
        {
          v = find_variable_nameref_for_create (cp->c_name, 1);
        }
      catch (const invalid_nameref_value &)
        {
          return;
        }

      if (v && v->nameref ())
        {
          cp->c_name = *(v->str_value ());
          v = make_new_array_variable (cp->c_name);
        }
    }

  if (v && (v->readonly () || v->noassign ()))
    {
      if (v->readonly ())
        err_readonly (cp->c_name.c_str ());
      return;
    }

  if (v == nullptr)
    v = make_new_array_variable (cp->c_name);

  if (!v->array ())
    (void)convert_var_to_array (v);

  t = itos (cp->c_rfd);
  ind = 0;
  (void)bind_array_variable (cp->c_name, ind, t);

  t = itos (cp->c_wfd);
  ind = 1;
  (void)bind_array_variable (cp->c_name, ind, t);
#else
  namevar = cp->c_name;
  namevar += "_READ";
  t = itos (cp->c_rfd);
  bind_variable (namevar, t);
  free (t);
  namevar = cp->c_name;
  namevar += "_WRITE";
  t = itos (cp->c_wfd);
  bind_variable (namevar, t);
  free (t);
#endif

  namevar = cp->c_name;
  namevar += "_PID";
  t = itos (cp->c_pid);
  (void)bind_variable (namevar, t);
}

void
Shell::coproc_unsetvars (Coproc *cp)
{
  if (cp->c_name.empty ())
    return;

  string namevar (cp->c_name);
  namevar += "_PID";

  unbind_variable_noref (namevar);

#if defined(ARRAY_VARS)
  check_unbind_variable (cp->c_name);
#else
  namevar = cp->c_name;
  namevar += "_READ";
  unbind_variable (namevar);
  namevar = cp->c_name;
  namevar += "_WRITE";
  unbind_variable (namevar);
#endif
}

int
Shell::execute_coproc (COPROC_COM *command, int pipe_in, int pipe_out,
                       fd_bitmap *fds_to_close)
{
  /* XXX -- can be removed after changes to handle multiple coprocs */
#if !MULTIPLE_COPROCS
  if (sh_coproc.c_pid != NO_PID
      && (sh_coproc.c_rfd >= 0 || sh_coproc.c_wfd >= 0))
    internal_warning (_ ("execute_coproc: coproc [%d:%s] still exists"),
                      sh_coproc.c_pid, sh_coproc.c_name.c_str ());
  coproc_init (&sh_coproc);
#endif

  bool invert = (command->flags & CMD_INVERT_RETURN);

  // expand name without splitting - could make this dependent on a shopt
  // option
  string name (expand_string_unsplit_to_string (command->name, 0));

  /* Optional check -- could be relaxed */
  if (!legal_identifier (name))
    {
      internal_error (_ ("`%s': not a valid identifier"), name.c_str ());
      return invert ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
    }
  else
    command->name = name;

  the_printed_command.clear ();
  string tcmd (make_command_string (command));

  int rpipe[2], wpipe[2];
  sh_openpipe (rpipe); /* 0 = parent read, 1 = child write */
  sh_openpipe (wpipe); /* 0 = child read, 1 = parent write */

  sigset_t set, oset;
  BLOCK_SIGNAL (SIGCHLD, set, oset);

  pid_t coproc_pid = make_child (tcmd, FORK_ASYNC);

  if (coproc_pid == 0)
    {
      close (rpipe[0]);
      close (wpipe[1]);

      UNBLOCK_SIGNAL (oset);
      int estat = execute_in_subshell (command, true, wpipe[0], rpipe[1],
                                       fds_to_close);

      fflush (stdout);
      fflush (stderr);

      exit (estat);
    }

  close (rpipe[1]);
  close (wpipe[0]);

  Coproc *cp = coproc_alloc (command->name, coproc_pid);
  cp->c_rfd = rpipe[0];
  cp->c_wfd = wpipe[1];

  cp->c_flags = COPROC_RUNNING;

  SET_CLOSE_ON_EXEC (cp->c_rfd);
  SET_CLOSE_ON_EXEC (cp->c_wfd);

  coproc_setvars (cp);

  UNBLOCK_SIGNAL (oset);

#if 0
  itrace ("execute_coproc (%s): [%d] %s", command->name.c_str (), coproc_pid,
          the_printed_command.c_str ());
#endif

  close_pipes (pipe_in, pipe_out);
#if defined(PROCESS_SUBSTITUTION) && defined(HAVE_DEV_FD)
  unlink_fifo_list ();
#endif
  stop_pipeline (1, nullptr);
  DESCRIBE_PID (coproc_pid);
  run_pending_traps ();

  return invert ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
}
#endif

/* If S == -1, it's a special value saying to close stdin */
static void
restore_stdin (int s)
{
  if (s == -1)
    close (0);
  else
    {
      dup2 (s, 0);
      close (s);
    }
}

#if 0
/* Catch-all cleanup function for lastpipe code for unwind-protects */
static int
lastpipe_cleanup (int s)
{
  set_jobs_list_frozen (s);
  return 0; // unused
}
#endif

int
Shell::execute_pipeline (CONNECTION *command, bool asynchronous, int pipe_in,
                         int pipe_out, fd_bitmap *fds_to_close)
{
#if defined(JOB_CONTROL)
  sigset_t set, oset;
  BLOCK_CHILD (set, oset);
#endif /* JOB_CONTROL */

  bool ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

  bool stdin_valid = sh_validfd (0);

  int prev = pipe_in;
  CONNECTION *cmd = command;

  while (cmd && cmd->connector == '|')
    {
      /* Make a pipeline between the two commands. */
      int fildes[2];
      if (pipe (fildes) < 0)
        {
          sys_error (_ ("pipe error"));
#if defined(JOB_CONTROL)
          terminate_current_pipeline ();
          kill_current_pipeline ();
          UNBLOCK_CHILD (oset);
#endif /* JOB_CONTROL */
          last_command_exit_value = EXECUTION_FAILURE;
          /* The unwind-protects installed below will take care
             of closing all of the open file descriptors. */
          throw_to_top_level ();
          return EXECUTION_FAILURE; /* XXX */
        }

      /* Here is a problem: with the new file close-on-exec
         code, the read end of the pipe (fildes[0]) stays open
         in the first process, so that process will never get a
         SIGPIPE.  There is no way to signal the first process
         that it should close fildes[0] after forking, so it
         remains open.  No SIGPIPE is ever sent because there
         is still a file descriptor open for reading connected
         to the pipe.  We take care of that here.  This passes
         around a bitmap of file descriptors that must be
         closed after making a child process in execute_simple_command. */

      /* Now copy the old information into the new bitmap. */
      fd_bitmap saved_bitmap (*fds_to_close);

      /* Resize to fit the new pipe file descriptor, if necessary. */
      size_t new_size = static_cast<size_t> (fildes[0] + 1);
      if (saved_bitmap.size () < new_size)
        saved_bitmap.resize (new_size);

      /* And mark the pipe file descriptors to be closed. */
      saved_bitmap[new_size - 1] = true;

      try
        {
          if (ignore_return && cmd->first)
            cmd->first->flags |= CMD_IGNORE_RETURN;

          execute_command_internal (cmd->first, asynchronous, prev, fildes[1],
                                    &saved_bitmap);

          if (prev >= 0)
            close (prev);

          prev = fildes[0];
          close (fildes[1]);
        }
      catch (const std::exception &)
        {
#if defined(JOB_CONTROL)
          sigprocmask (SIG_SETMASK, &oset, nullptr);
#endif

          if (prev >= 0)
            close (prev);

          prev = fildes[0];
          close (fildes[1]);

          /* In case there are pipe or out-of-processes errors, we
             want all these file descriptors to be closed when
             exceptions are thrown. */
          close_fd_bitmap (saved_bitmap);

          throw;
        }

      // Continue to the next connection, if any.
      if (cmd->second && cmd->second->type == cm_connection)
        cmd = static_cast<CONNECTION *> (cmd->second);
      else
        break;
    }

  pid_t lastpid = last_made_pid;

  /* Now execute the rightmost command in the pipeline.  */
  if (ignore_return && cmd)
    cmd->flags |= CMD_IGNORE_RETURN;

  bool lastpipe_flag = false;

  int lstdin = -2; /* -1 is special, meaning fd 0 is closed */

  int lastpipe_jid = 0, old_frozen = 0;

  /* If the `lastpipe' option is set with shopt, and job control is not
     enabled, execute the last element of non-async pipelines in the
     current shell environment. */

  /* prev can be 0 if fd 0 was closed when this function was executed. prev
     will never be 0 at this point if fd 0 was valid when this function was
     executed (though we check above). */
  if (lastpipe_opt && !job_control && !asynchronous && pipe_out == NO_PIPE
      && prev >= 0)
    {
      /* -1 is a special value meaning to close stdin */
      lstdin = (prev > 0 && stdin_valid) ? move_to_high_fd (0, 1, -1) : -1;
      if (lstdin > 0 || lstdin == -1)
        {
          do_piping (prev, pipe_out);
          prev = NO_PIPE;
          lastpipe_flag = true;
          old_frozen = freeze_jobs_list ();
          lastpipe_jid = stop_pipeline (0, nullptr); /* XXX */
#if defined(JOB_CONTROL)
          UNBLOCK_CHILD (oset); /* XXX */
#endif
        }
      if (cmd)
        cmd->flags |= CMD_LASTPIPE;
    }

  int exec_result;
  try
    {
      exec_result = execute_command_internal (cmd, asynchronous, prev,
                                              pipe_out, fds_to_close);
    }
  catch (const std::exception &)
    {
      if (prev >= 0)
        close (prev);

      set_jobs_list_frozen (old_frozen);

      if (lstdin > 0 || lstdin == -1)
        restore_stdin (lstdin);

      throw;
    }

  if (prev >= 0)
    close (prev);

  if (lstdin > 0 || lstdin == -1)
    restore_stdin (lstdin);

#if defined(JOB_CONTROL)
  UNBLOCK_CHILD (oset);
#endif

  QUIT;

  if (lastpipe_flag)
    {
#if defined(JOB_CONTROL)
      if (INVALID_JOB (lastpipe_jid) == 0)
        {
          append_process (savestring (the_printed_command_except_trap),
                          dollar_dollar_pid, exec_result, lastpipe_jid);

          lstdin = wait_for (lastpid, 0);
        }
      else
        {
          lstdin = wait_for_single_pid (lastpid, 0); /* checks bgpids list */
          if (lstdin > 256)                          // error sentinel
            lstdin = 127;
        }
#else
      lstdin = wait_for (lastpid, 0);
#endif

#if defined(JOB_CONTROL)
      /* If wait_for removes the job from the jobs table, use result of last
         command as pipeline's exit status as usual.  The jobs list can get
         frozen and unfrozen at inconvenient times if there are multiple
         pipelines running simultaneously. */
      if (INVALID_JOB (lastpipe_jid) == 0)
        exec_result = job_exit_status (lastpipe_jid);
      else if (pipefail_opt)
        exec_result |= lstdin; /* XXX */
                               /* otherwise we use exec_result */
#endif

      set_jobs_list_frozen (old_frozen);
    }

  return exec_result;
}

int
Shell::execute_connection (CONNECTION *command, bool asynchronous, int pipe_in,
                           int pipe_out, fd_bitmap *fds_to_close)
{
  COMMAND *tc, *second;
  int exec_result;
  int save_line_number;

  bool ignore_return = (command->flags & CMD_IGNORE_RETURN) != 0;

  switch (command->connector)
    {
    /* Do the first command asynchronously. */
    case '&':
      tc = command->first;
      if (tc == nullptr)
        return EXECUTION_SUCCESS;

      if (ignore_return)
        tc->flags |= CMD_IGNORE_RETURN;
      tc->flags |= CMD_AMPERSAND;

      /* If this shell was compiled without job control support,
         if we are currently in a subshell via `( xxx )', or if job
         control is not active then the standard input for an
         asynchronous command is forced to /dev/null. */
#if defined(JOB_CONTROL)
      if ((subshell_environment || !job_control) && !stdin_redir)
#else
      if (!stdin_redir)
#endif /* JOB_CONTROL */
        tc->flags |= CMD_STDIN_REDIR;

      exec_result = execute_command_internal (tc, true, pipe_in, pipe_out,
                                              fds_to_close);

      QUIT;

      if (tc->flags & CMD_STDIN_REDIR)
        tc->flags &= ~CMD_STDIN_REDIR;

      second = command->second;
      if (second)
        {
          if (ignore_return)
            second->flags |= CMD_IGNORE_RETURN;

          exec_result = execute_command_internal (
              second, asynchronous, pipe_in, pipe_out, fds_to_close);
        }

      break;

    /* Just call execute command on both sides. */
    case ';':
    case '\n': /* special case, happens in command substitutions */
      if (ignore_return)
        {
          if (command->first)
            command->first->flags |= CMD_IGNORE_RETURN;
          if (command->second)
            command->second->flags |= CMD_IGNORE_RETURN;
        }
      executing_list++;
      QUIT;

#if 1
      execute_command (command->first);
#else
      execute_command_internal (command->first, asynchronous, pipe_in,
                                pipe_out, fds_to_close);
#endif

      QUIT;
      optimize_connection_fork (command); /* XXX */
      exec_result = execute_command_internal (command->second, asynchronous,
                                              pipe_in, pipe_out, fds_to_close);
      executing_list--;
      break;

    case '|':
      {
        bool was_error_trap = signal_is_trapped (ERROR_TRAP)
                              && !signal_is_ignored (ERROR_TRAP);

        bool invert = (command->flags & CMD_INVERT_RETURN);
        ignore_return = (command->flags & CMD_IGNORE_RETURN);

        SET_LINE_NUMBER (line_number); /* XXX - save value? */
        exec_result = execute_pipeline (command, asynchronous, pipe_in,
                                        pipe_out, fds_to_close);

        if (asynchronous)
          {
            exec_result = EXECUTION_SUCCESS;
            invert = 0;
          }

        if (was_error_trap && !ignore_return && !invert
            && exec_result != EXECUTION_SUCCESS)
          {
            last_command_exit_value = exec_result;
            save_line_number = line_number;
            line_number = line_number_for_err_trap;
            run_error_trap ();
            line_number = save_line_number;
          }

        if (!ignore_return && !invert && exit_immediately_on_error
            && exec_result != EXECUTION_SUCCESS)
          {
            last_command_exit_value = exec_result;
            run_pending_traps ();
            throw bash_exception (ERREXIT);
          }

        break;
      }

    case parser::token::AND_AND:
    case parser::token::OR_OR:
      if (asynchronous)
        {
          /* If we have something like `a && b &' or `a || b &', run the
             && or || stuff in a subshell.  Force a subshell and just call
             execute_command_internal again.  Leave asynchronous on
             so that we get a report from the parent shell about the
             background job. */
          command->flags |= CMD_FORCE_SUBSHELL;
          exec_result = execute_command_internal (command, 1, pipe_in,
                                                  pipe_out, fds_to_close);
          break;
        }

      /* Execute the first command.  If the result of that is successful
         and the connector is AND_AND, or the result is not successful
         and the connector is OR_OR, then execute the second command,
         otherwise return. */

      executing_list++;
      if (command->first)
        command->first->flags |= CMD_IGNORE_RETURN;

#if 1
      exec_result = execute_command (command->first);
#else
      exec_result = execute_command_internal (command->first, 0, NO_PIPE,
                                              NO_PIPE, fds_to_close);
#endif
      QUIT;
      if (((command->connector == parser::token::AND_AND)
           && (exec_result == EXECUTION_SUCCESS))
          || ((command->connector == parser::token::OR_OR)
              && (exec_result != EXECUTION_SUCCESS)))
        {
          optimize_connection_fork (command);

          second = command->second;
          if (ignore_return && second)
            second->flags |= CMD_IGNORE_RETURN;

          exec_result = execute_command (second);
        }
      executing_list--;
      break;

    default:
      command_error ("execute_connection", CMDERR_BADCONN, command->connector);
      /* NOTREACHED */
    }

  return exec_result;
}

/* The test used to be only for interactive_shell, but we don't want to report
   job status when the shell is not interactive or when job control isn't
   enabled. */
#define REAP()                                                                \
  do                                                                          \
    {                                                                         \
      if (!job_control || !interactive_shell)                                 \
        reap_dead_jobs ();                                                    \
    }                                                                         \
  while (0)

/* Execute a FOR command.  The syntax is: FOR word_desc IN word_list;
   DO command; DONE */
int
Shell::execute_for_command (FOR_SELECT_COM *for_command)
{
  int save_line_number = line_number;
  if (!check_identifier (for_command->name, 1))
    {
      if (posixly_correct && !interactive_shell)
        {
          last_command_exit_value = EX_BADUSAGE;
          throw bash_exception (ERREXIT);
        }
      return EXECUTION_FAILURE;
    }

  loop_level++;

  string &identifier = for_command->name->word;
  line_number = for_command->line; // for expansion error messages

  WORD_LIST *list = expand_words_no_vars (for_command->map_list);
  WORD_LIST *releaser = list;

  int retval = EXECUTION_SUCCESS;

  try
    {
      if (for_command->flags & CMD_IGNORE_RETURN)
        for_command->action->flags |= CMD_IGNORE_RETURN;

      for (; list; list = list->next ())
        {
          QUIT;

          line_number = for_command->line;

          /* Remember what this command looks like, for debugger. */
          the_printed_command.clear ();
          for_command->print_head (this);

          if (echo_command_at_execute)
            xtrace_print_for_command_head (for_command);

          /* Save this command unless it's a trap command and we're not running
             a debug trap. */
          if (!signal_in_progress (DEBUG_TRAP) && running_trap == 0)
            the_printed_command_except_trap = the_printed_command;

          retval = run_debug_trap ();

#if defined(DEBUGGER)
          /* In debugging mode, if the DEBUG trap returns a non-zero status, we
             skip the command. */
          if (debugging_mode && retval != EXECUTION_SUCCESS)
            continue;
#endif

          this_command_name.clear ();

          /* XXX - special ksh93 for command index variable handling */
          SHELL_VAR *v = find_variable_last_nameref (identifier, 1);

          if (v && v->nameref ())
            {
              if (valid_nameref_value (list->word->word, VA_NOEXPAND) == 0)
                {
                  sh_invalidid (list->word->word.c_str ());
                  v = nullptr;
                }
              else if (v->readonly ())
                err_readonly (v->name ().c_str ());
              else
                v = bind_variable_value (v, list->word->word, ASS_NAMEREF);
            }
          else
            v = bind_variable (identifier, list->word->word);

          if (v == nullptr || v->readonly () || v->noassign ())
            {
              line_number = save_line_number;
              if (v && v->readonly () && !interactive_shell && posixly_correct)
                {
                  last_command_exit_value = EXECUTION_FAILURE;
                  throw bash_exception (FORCE_EOF);
                }
              else
                {
                  delete releaser;
                  loop_level--;
                  return EXECUTION_FAILURE;
                }
            }

          if (ifsname (identifier))
            setifs (v);
          else
            stupidly_hack_special_variables (identifier);

          retval = execute_command (for_command->action);

          REAP ();
          QUIT;

          if (breaking)
            {
              breaking--;
              break;
            }

          if (continuing)
            {
              continuing--;
              if (continuing)
                break;
            }
        }

      loop_level--;
      line_number = save_line_number;
    }
  catch (const std::exception &)
    {
      loop_level--;
      delete releaser;
      throw;
    }

  delete releaser;
  return retval;
}

#if defined(ARITH_FOR_COMMAND)
/* Execute an arithmetic for command.  The syntax is

        for (( init ; step ; test ))
        do
                body
        done

   The execution should be exactly equivalent to

        eval \(\( init \)\)
        while eval \(\( test \)\) ; do
                body;
                eval \(\( step \)\)
        done
*/
int64_t
Shell::eval_arith_for_expr (WORD_LIST *l, bool *okp)
{
  int64_t expresult;

  string expr (l->next () ? string_list (l) : l->word->word);
  string temp (expand_arith_string (expr, Q_DOUBLE_QUOTES | Q_ARITH));
  WORD_LIST *new_list = new WORD_LIST (make_word (temp), nullptr);

  if (echo_command_at_execute)
    xtrace_print_arith_cmd (new_list);

  the_printed_command.clear ();
  print_arith_command (new_list);

  if (!signal_in_progress (DEBUG_TRAP) && running_trap == 0)
    the_printed_command_except_trap = the_printed_command;

  int r = run_debug_trap ();

  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  eval_flags eflag
      = (shell_compatibility_level > 51) ? EXP_NOFLAGS : EXP_EXPANDED;

  this_command_name = "(("; // for expression error messages

#if defined(DEBUGGER)
  if (!debugging_mode || r == EXECUTION_SUCCESS)
    expresult = evalexp (new_list->word->word, eflag, okp);
  else
    {
      expresult = 0;
      if (okp)
        *okp = true;
    }
#else
  expresult = evalexp (new_list->word->word, eflag, okp);
#endif
  delete new_list;

  return expresult;
}

int
Shell::execute_arith_for_command (ARITH_FOR_COM *arith_for_command)
{
  int body_status = EXECUTION_SUCCESS;
  loop_level++;
  int save_lineno = line_number;

  if (arith_for_command->flags & CMD_IGNORE_RETURN)
    arith_for_command->action->flags |= CMD_IGNORE_RETURN;

  this_command_name = "(("; // for expression error messages

  /* save the starting line number of the command so we can reset
     line_number before executing each expression -- for $LINENO
     and the DEBUG trap. */

  int arith_lineno = arith_for_command->line;
  line_number = arith_lineno;

  if (variable_context && interactive_shell && sourcelevel == 0)
    {
      /* line numbers in a function start at 1 */
      line_number -= function_line_number - 1;
      if (line_number <= 0)
        line_number = 1;
    }

  /* Evaluate the initialization expression. */

  bool expok;
  int64_t expresult = eval_arith_for_expr (arith_for_command->init, &expok);
  if (!expok)
    {
      line_number = save_lineno;
      return EXECUTION_FAILURE;
    }

  while (1)
    {
      /* Evaluate the test expression. */
      line_number = arith_lineno;
      expresult = eval_arith_for_expr (arith_for_command->test, &expok);
      line_number = save_lineno;

      if (!expok)
        {
          body_status = EXECUTION_FAILURE;
          break;
        }

      REAP ();

      if (expresult == 0)
        break;

      /* Execute the body of the arithmetic for command. */
      QUIT;
      body_status = execute_command (arith_for_command->action);
      QUIT;

      /* Handle any `break' or `continue' commands executed by the body. */
      if (breaking)
        {
          breaking--;
          break;
        }

      if (continuing)
        {
          continuing--;
          if (continuing)
            break;
        }

      /* Evaluate the step expression. */
      line_number = arith_lineno;
      expresult = eval_arith_for_expr (arith_for_command->step, &expok);
      line_number = save_lineno;

      if (!expok)
        {
          body_status = EXECUTION_FAILURE;
          break;
        }
    }

  loop_level--;
  line_number = save_lineno;

  return body_status;
}
#endif

#if defined(SELECT_COMMAND)

#define tabsize 8

#define RP_SPACE ") "
#define RP_SPACE_LEN 2

/* XXX - does not handle numbers > 1000000 at all. */
#define NUMBER_LEN(s)                                                         \
  ((s < 10)                                                                   \
       ? 1                                                                    \
       : ((s < 100)                                                           \
              ? 2                                                             \
              : ((s < 1000) ? 3                                               \
                            : ((s < 10000) ? 4 : ((s < 100000) ? 5 : 6)))))

static int
displen (const char *s)
{
#if defined(HANDLE_MULTIBYTE)
  wchar_t *wcstr = nullptr;
  size_t slen = mbstowcs (wcstr, s, 0);
  if (slen == static_cast<size_t> (-1))
    slen = 0;

  wcstr = new wchar_t[slen + 1];
  mbstowcs (wcstr, s, slen + 1);
  int wclen = wcswidth (wcstr, slen);
  delete[] wcstr;

  return wclen < 0 ? static_cast<int> (strlen (s)) : wclen;
#else
  return static_cast<int> (strlen (s));
#endif
}

static inline int
print_index_and_element (int len, int ind, WORD_LIST *list)
{
  WORD_LIST *l;
  int i;

  if (list == nullptr)
    return 0;

  for (i = ind, l = list; l && --i; l = l->next ())
    ;

  if (l == nullptr) /* don't think this can happen */
    return 0;

  const char *word = l->word->word.c_str ();

  fprintf (stderr, "%*d%s%s", len, ind, RP_SPACE, word);
  return displen (word);
}

static inline void
indent (int from, int to)
{
  while (from < to)
    {
      if ((to / tabsize) > (from / tabsize))
        {
          putc ('\t', stderr);
          from += tabsize - from % tabsize;
        }
      else
        {
          putc (' ', stderr);
          from++;
        }
    }
}

void
Shell::print_select_list (WORD_LIST *list, int max_elem_len, int indices_len)
{
  if (list == nullptr)
    {
      putc ('\n', stderr);
      return;
    }

  int cols = max_elem_len ? select_cols / max_elem_len : 1;
  if (cols == 0)
    cols = 1;

  int list_len = static_cast<int> (list->size ());

  int rows = list_len ? list_len / cols + (list_len % cols != 0) : 1;
  cols = list_len ? list_len / rows + (list_len % rows != 0) : 1;

  if (rows == 1)
    {
      rows = cols;
      cols = 1;
    }

  int first_column_indices_len = NUMBER_LEN (rows);
  int other_indices_len = indices_len;

  for (int row = 0; row < rows; row++)
    {
      int ind = row;
      int pos = 0;
      while (1)
        {
          indices_len
              = (pos == 0) ? first_column_indices_len : other_indices_len;
          int elem_len = print_index_and_element (indices_len, ind + 1, list);
          elem_len += indices_len + RP_SPACE_LEN;
          ind += rows;
          if (ind >= list_len)
            break;
          bash::indent (pos + elem_len, pos + max_elem_len);
          pos += max_elem_len;
        }
      putc ('\n', stderr);
    }
}

/* Print the elements of LIST, one per line, preceded by an index from 1 to
   LIST_LEN.  Then display PROMPT and wait for the user to enter a number.
   If the number is between 1 and LIST_LEN, return that selection.  If EOF
   is read, return a null string.  If a blank line is entered, or an invalid
   number is entered, the loop is executed again. */
string *
Shell::select_query (WORD_LIST *list, const char *prompt, bool print_menu)
{
  select_cols = default_columns ();

  int max_elem_len = 0;
  for (WORD_LIST *l = list; l; l = l->next ())
    {
      int len = displen (l->word->word.c_str ());
      if (len > max_elem_len)
        max_elem_len = len;
    }

  int indices_len = NUMBER_LEN (list->size ());
  max_elem_len += indices_len + RP_SPACE_LEN + 2;

  while (1)
    {
      if (print_menu)
        print_select_list (list, max_elem_len, indices_len);

      fprintf (stderr, "%s", prompt);
      fflush (stderr);
      QUIT;

      int oe = executing_builtin;
      executing_builtin = 1;
      int r = read_builtin (nullptr);
      executing_builtin = oe;

      if (r != EXECUTION_SUCCESS)
        {
          putchar ('\n');
          return nullptr;
        }

      const string *repl_string = get_string_value ("REPLY");

      if (repl_string == nullptr)
        return nullptr;

      if (repl_string->empty ())
        {
          print_menu = true;
          continue;
        }

      int64_t reply;
      if (!legal_number (repl_string->c_str (), &reply))
        return nullptr;

      if (reply < 1 || reply > static_cast<int64_t> (list->size ()))
        return nullptr;

      WORD_LIST *l;
      for (l = list; l && --reply; l = l->next ())
        ;

      return &l->word->word;
    }
}

/* Execute a SELECT command.  The syntax is:
   SELECT word IN list DO command_list DONE
   Only `break' or `return' in command_list will terminate
   the command. */
int
Shell::execute_select_command (FOR_SELECT_COM *select_command)
{
  if (!check_identifier (select_command->name, true))
    return EXECUTION_FAILURE;

  int save_line_number = line_number;
  line_number = select_command->line;

  the_printed_command.clear ();
  select_command->print_head (this);

  if (echo_command_at_execute)
    xtrace_print_select_command_head (select_command);

#if 0
  if (!signal_in_progress (DEBUG_TRAP)
      && (this_command_name == 0 || (STREQ (this_command_name, "trap") == 0)))
#else
  if (!signal_in_progress (DEBUG_TRAP) && running_trap == 0)
#endif
  {
    the_printed_command_except_trap = the_printed_command;
  }

  int retval = run_debug_trap ();

#if defined(DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    return EXECUTION_SUCCESS;
#endif

  this_command_name.clear ();

  loop_level++;
  const string &identifier = select_command->name->word;

  /* command and arithmetic substitution, parameter and variable expansion,
     word splitting, pathname expansion, and quote removal. */

  WORD_LIST *releaser, *list;
  list = releaser = expand_words_no_vars (select_command->map_list);
  if (list == nullptr || list->size () == 0)
    {
      delete list;
      line_number = save_line_number;
      return EXECUTION_SUCCESS;
    }

  if (select_command->flags & CMD_IGNORE_RETURN)
    select_command->action->flags |= CMD_IGNORE_RETURN;

  retval = EXECUTION_SUCCESS;
  bool show_menu = true;

  try
    {
      while (1)
        {
          line_number = select_command->line;
          const string *ps3_prompt_str = get_string_value ("PS3");
          const char *ps3_prompt
              = ps3_prompt_str ? ps3_prompt_str->c_str () : "#? ";

          QUIT;
          string *selection = select_query (list, ps3_prompt, show_menu);
          QUIT;

          if (selection == nullptr)
            {
              /* select_query returns EXECUTION_FAILURE if the read builtin
                 fails, so we want to return failure in this case. */
              retval = EXECUTION_FAILURE;
              break;
            }

          SHELL_VAR *v = bind_variable (identifier, *selection);
          if (v == nullptr || v->readonly () || v->noassign ())
            {
              if (v && v->readonly () && !interactive_shell && posixly_correct)
                {
                  last_command_exit_value = EXECUTION_FAILURE;
                  throw bash_exception (FORCE_EOF);
                }
              else
                {
                  delete releaser;
                  loop_level--;
                  line_number = save_line_number;
                  return EXECUTION_FAILURE;
                }
            }

          stupidly_hack_special_variables (identifier);

          retval = execute_command (select_command->action);

          REAP ();
          QUIT;

          if (breaking)
            {
              breaking--;
              break;
            }

          if (continuing)
            {
              continuing--;
              if (continuing)
                break;
            }

#if defined(KSH_COMPATIBLE_SELECT)
          show_menu = false;
          const string *reply_string = get_string_value ("REPLY");
          if (reply_string && reply_string->empty ())
            show_menu = true;
#endif
        }
    }
  catch (const std::exception &)
    {
      loop_level--;
      line_number = save_line_number;
      delete releaser;
      throw;
    }

  loop_level--;
  line_number = save_line_number;

  delete releaser;
  return retval;
}
#endif /* SELECT_COMMAND */

/* Execute a CASE command.  The syntax is: CASE word_desc IN pattern_list ESAC.
   The pattern_list is a linked list of pattern clauses; each clause contains
   some patterns to compare word_desc against, and an associated command to
   execute. */
int
Shell::execute_case_command (CASE_COM *case_command)
{
  int save_line_number = line_number;
  line_number = case_command->line;

  the_printed_command.clear ();
  case_command->print_head (this);

  if (echo_command_at_execute)
    xtrace_print_case_command_head (case_command);

#if 0
  if (!signal_in_progress (DEBUG_TRAP)
      && (this_command_name == nullptr
          || (STREQ (this_command_name, "trap") == 0)))
#else
  if (!signal_in_progress (DEBUG_TRAP) && running_trap == 0)
#endif
  {
    the_printed_command_except_trap = the_printed_command;
  }

  int retval = run_debug_trap ();

#if defined(DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    {
      line_number = save_line_number;
      return EXECUTION_SUCCESS;
    }
#endif

  /* Use the same expansions (the ones POSIX specifies) as the patterns;
     dequote the resulting string (as POSIX specifies) since the quotes in
     patterns are handled specially below. We have to do it in this order
     because we're not supposed to perform word splitting. */
  WORD_LIST *wlist = expand_word_leave_quoted (case_command->word, 0);

  string word;
  if (wlist)
    word = dequote_string (string_list (wlist));

  delete wlist;

  retval = EXECUTION_SUCCESS;
  bool ignore_return = (case_command->flags & CMD_IGNORE_RETURN);

#define EXIT_CASE() goto exit_case_command

  PATTERN_LIST *clauses;
  const char *word_cstr = word.c_str ();
  for (clauses = case_command->clauses; clauses; clauses = clauses->next ())
    {
      QUIT;
      for (WORD_LIST *list = clauses->patterns; list; list = list->next ())
        {
          WORD_LIST *es = expand_word_leave_quoted (list->word, 0);

          char *pattern;
          if (es && es->word && !es->word->word.empty ())
            {
              /* Convert quoted null strings into empty strings. */
              qglob_flags qflags = QGLOB_CVTNULL;

              /* We left CTLESC in place quoting CTLESC and CTLNUL after the
                 call to expand_word_leave_quoted; tell
                 quote_string_for_globbing to remove those here. This works for
                 both unquoted portions of the word (which call quote_escapes)
                 and quoted portions (which call quote_string). */
              qflags |= QGLOB_CTLESC;
              pattern = quote_string_for_globbing (es->word->word.c_str (),
                                                   qflags);
            }
          else
            {
              pattern = new char[1];
              pattern[0] = '\0';
            }

          /* Since the pattern does not undergo quote removal (as per
             Posix.2, section 3.9.4.3), the fnmatch () call must be able
             to recognize backslashes as escape characters. */
          bool match = (fnmatch (pattern, word_cstr,
                                 FNMATCH_EXTFLAG | FNMATCH_IGNCASE)
                        != FNM_NOMATCH);

          delete[] pattern;
          delete es;

          if (match)
            {
              do
                {
                  if (clauses->action && ignore_return)
                    clauses->action->flags |= CMD_IGNORE_RETURN;
                  retval = execute_command (clauses->action);
                }
              while ((clauses->flags & CASEPAT_FALLTHROUGH)
                     && (clauses = clauses->next ()));
              if (clauses == nullptr
                  || (clauses->flags & CASEPAT_TESTNEXT) == 0)
                EXIT_CASE ();
              else
                break;
            }

          QUIT;
        }
    }

exit_case_command:
  line_number = save_line_number;
  return retval;
}

/* The body for both while and until. The only difference between the
   two is that the test value is treated differently. The return value for both
   commands should be EXECUTION_SUCCESS if no commands in the body are
   executed, and the status of the last command executed in the body otherwise.
 */
int
Shell::execute_while_or_until (UNTIL_WHILE_COM *while_command)
{
  int return_value, body_status;

  body_status = EXECUTION_SUCCESS;
  loop_level++;

  while_command->test->flags |= CMD_IGNORE_RETURN;
  if (while_command->flags & CMD_IGNORE_RETURN)
    while_command->action->flags |= CMD_IGNORE_RETURN;

  while (1)
    {
      return_value = execute_command (while_command->test);
      REAP ();

      /* Need to handle `break' in the test when we would break out of the
         loop.  The job control code will set `breaking' to loop_level
         when a job in a loop is stopped with SIGTSTP.  If the stopped job
         is in the loop test, `breaking' will not be reset unless we do
         this, and the shell will cease to execute commands.  The same holds
         true for `continue'. */
      if (while_command->type == cm_while && return_value != EXECUTION_SUCCESS)
        {
          if (breaking)
            breaking--;
          if (continuing)
            continuing--;
          break;
        }
      if (while_command->type == cm_until && return_value == EXECUTION_SUCCESS)
        {
          if (breaking)
            breaking--;
          if (continuing)
            continuing--;
          break;
        }

      QUIT;
      body_status = execute_command (while_command->action);
      QUIT;

      if (breaking)
        {
          breaking--;
          break;
        }

      if (continuing)
        {
          continuing--;
          if (continuing)
            break;
        }
    }
  loop_level--;

  return body_status;
}

/* IF test THEN command [ELSE command].
   IF also allows ELIF in the place of ELSE IF, but
   the parser makes *that* stupidity transparent. */
int
Shell::execute_if_command (IF_COM *if_command)
{
  int return_value, save_line_number;

  save_line_number = line_number;
  if_command->test->flags |= CMD_IGNORE_RETURN;
  return_value = execute_command (if_command->test);
  line_number = save_line_number;

  if (return_value == EXECUTION_SUCCESS)
    {
      QUIT;

      if (if_command->true_case && (if_command->flags & CMD_IGNORE_RETURN))
        if_command->true_case->flags |= CMD_IGNORE_RETURN;

      return execute_command (if_command->true_case);
    }
  else
    {
      QUIT;

      if (if_command->false_case && (if_command->flags & CMD_IGNORE_RETURN))
        if_command->false_case->flags |= CMD_IGNORE_RETURN;

      return execute_command (if_command->false_case);
    }
}

#if defined(DPAREN_ARITHMETIC)
int
Shell::execute_arith_command (ARITH_COM *arith_command)
{
  int64_t expresult = 0;

  int save_line_number = line_number;
  this_command_name = "((";

  SET_LINE_NUMBER (arith_command->line);

  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell && sourcelevel == 0)
    {
      /* line numbers in a function start at 1 */
      line_number -= function_line_number - 1;
      if (line_number <= 0)
        line_number = 1;
    }

  the_printed_command.clear ();
  print_arith_command (arith_command->exp);

  if (!signal_in_progress (DEBUG_TRAP) && running_trap == 0)
    the_printed_command_except_trap = the_printed_command;

  /* Run the debug trap before each arithmetic command, but do it after we
     update the line number information and before we expand the various
     words in the expression. */
  int retval = run_debug_trap ();

#if defined(DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    {
      line_number = save_line_number;
      return EXECUTION_SUCCESS;
    }
#endif

  this_command_name = "((";
  WORD_LIST *new_list = arith_command->exp;

  string exp (new_list->next () ? string_list (new_list)
                                : new_list->word->word);
  exp = expand_arith_string (exp, Q_DOUBLE_QUOTES | Q_ARITH);

  /* If we're tracing, make a new word list with `((' at the front and `))'
     at the back and print it. Change xtrace_print_arith_cmd to take a string
     when I change eval_arith_for_expr to use expand_arith_string(). */
  if (echo_command_at_execute)
    {
      new_list = new WORD_LIST (make_word (exp), nullptr);
      xtrace_print_arith_cmd (new_list);
      delete new_list;
    }

  bool expok = false;
  if (!exp.empty ())
    {
      eval_flags eflag
          = shell_compatibility_level > 51 ? EXP_NOFLAGS : EXP_EXPANDED;
      expresult = evalexp (exp, eflag, &expok);
      line_number = save_line_number;
    }
  else
    {
      expresult = 0;
      expok = true;
    }

  if (!expok)
    return EXECUTION_FAILURE;

  return expresult == 0 ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
}
#endif /* DPAREN_ARITHMETIC */

#if defined(COND_COMMAND)

/* XXX - can COND ever be NULL when this is called? */
int
Shell::execute_cond_node (COND_COM *cond)
{
  bool invert = (cond->flags & CMD_INVERT_RETURN);
  bool ignore = (cond->flags & CMD_IGNORE_RETURN);

  if (ignore)
    {
      if (cond->left)
        cond->left->flags |= CMD_IGNORE_RETURN;
      if (cond->right)
        cond->right->flags |= CMD_IGNORE_RETURN;
    }

  int result;
  if (cond->cond_type == COND_EXPR)
    result = execute_cond_node (cond->left);
  else if (cond->cond_type == COND_OR)
    {
      result = execute_cond_node (cond->left);
      if (result != EXECUTION_SUCCESS)
        result = execute_cond_node (cond->right);
    }
  else if (cond->cond_type == COND_AND)
    {
      result = execute_cond_node (cond->left);
      if (result == EXECUTION_SUCCESS)
        result = execute_cond_node (cond->right);
    }
  else if (cond->cond_type == COND_UNARY)
    {
      if (ignore)
        comsub_ignore_return++;

      bool varop = (cond->op->word == "-v");

      string arg1 (cond_expand_word (cond->left->op, varop ? 3 : 0));

      if (ignore)
        comsub_ignore_return--;

      if (echo_command_at_execute)
        xtrace_print_cond_term (cond->cond_type, invert, cond->op,
                                arg1.c_str (), nullptr);

#if defined(ARRAY_VARS)
      if (varop && shell_compatibility_level > 51)
        {
          char oa = assoc_expand_once;
          result = unary_test (cond->op->word.c_str (), arg1.c_str ())
                       ? EXECUTION_SUCCESS
                       : EXECUTION_FAILURE;
          assoc_expand_once = oa;
        }
      else
#endif
        result = unary_test (cond->op->word.c_str (), arg1.c_str ())
                     ? EXECUTION_SUCCESS
                     : EXECUTION_FAILURE;
    }
  else if (cond->cond_type == COND_BINARY)
    {
      string &op = cond->op->word;

      bool patmatch = (op == "==" || op == "!=" || op == "=");

#if defined(COND_REGEXP)
      bool rmatch = (op == "=~");
#endif

      bool arith = (op == "-eq" || op == "-ne" || op == "-lt" || op == "-le"
                    || op == "-gt" || op == "-ge");

      int mode;
      if (arith)
        mode = 3;
      else if (rmatch && shell_compatibility_level > 31)
        mode = 2;
      else if (patmatch)
        mode = 1;
      else
        mode = 0;

      if (ignore)
        comsub_ignore_return++;

      string arg1 (cond_expand_word (cond->left->op, arith ? mode : 0));
      string arg2 (cond_expand_word (cond->right->op, mode));

      if (ignore)
        comsub_ignore_return--;

      if (echo_command_at_execute)
        xtrace_print_cond_term (cond->cond_type, invert, cond->op,
                                arg1.c_str (), arg2.c_str ());

#if defined(COND_REGEXP)
      if (rmatch)
        {
          int mflags = SHMAT_PWARN;
#if defined(ARRAY_VARS)
          mflags |= SHMAT_SUBEXP;
#endif

          result = sh_regmatch (arg1, arg2, mflags);
        }
      else
#endif /* COND_REGEXP */
        {
          bool oe = extended_glob;
          extended_glob = true;
          result = binary_test (cond->op->word.c_str (), arg1.c_str (),
                                arg2.c_str (),
                                (TEST_PATMATCH | TEST_ARITHEXP | TEST_LOCALE))
                       ? EXECUTION_SUCCESS
                       : EXECUTION_FAILURE;
          extended_glob = oe;
        }
    }
  else
    {
      command_error ("execute_cond_node", CMDERR_BADTYPE, cond->type);
      /* NOT REACHED */
    }

  if (invert)
    result = (result == EXECUTION_SUCCESS) ? EXECUTION_FAILURE
                                           : EXECUTION_SUCCESS;

  return result;
}

int
Shell::execute_cond_command (COND_COM *cond_command)
{
  int save_line_number = line_number;
  SET_LINE_NUMBER (cond_command->line);

  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell && sourcelevel == 0)
    {
      /* line numbers in a function start at 1 */
      line_number -= function_line_number - 1;
      if (line_number <= 0)
        line_number = 1;
    }

  the_printed_command.clear ();
  cond_command->print (this);

  if (!signal_in_progress (DEBUG_TRAP) && running_trap == 0)
    the_printed_command_except_trap.clear ();

  /* Run the debug trap before each conditional command, but do it after we
     update the line number information. */
  int retval = run_debug_trap ();

#if defined(DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && retval != EXECUTION_SUCCESS)
    {
      line_number = save_line_number;
      return EXECUTION_SUCCESS;
    }
#endif

  this_command_name = "[[";

#if 0
  debug_print_cond_command (cond_command);
#endif

  last_command_exit_value = retval = execute_cond_node (cond_command);
  line_number = save_line_number;
  return retval;
}
#endif /* COND_COMMAND */

/* Execute a null command.  Fork a subshell if the command uses pipes or is
   to be run asynchronously.  This handles all the side effects that are
   supposed to take place. */
int
Shell::execute_null_command (REDIRECT *redirects, int pipe_in, int pipe_out,
                             bool async)
{
  bool forcefork;
  REDIRECT *rd;

  for (forcefork = false, rd = redirects; rd; rd = rd->next ())
    {
      forcefork |= rd->rflags & REDIR_VARASSIGN;
      /* Safety */
      forcefork |= (rd->redirector.r.dest == 0
                    || fd_is_bash_input (rd->redirector.r.dest))
                   && (INPUT_REDIRECT (rd->instruction)
                       || TRANSLATE_REDIRECT (rd->instruction)
                       || rd->instruction == r_close_this);
    }

  if (forcefork || pipe_in != NO_PIPE || pipe_out != NO_PIPE || async)
    {
      /* We have a null command, but we really want a subshell to take
         care of it.  Just fork, do piping and redirections, and exit. */
      make_child_flags fork_flags = async ? FORK_ASYNC : FORK_SYNC;
      if (make_child (string (), fork_flags) == 0)
        {
          /* Cancel traps, in trap.c. */
          restore_original_signals (); /* XXX */

          do_piping (pipe_in, pipe_out);

#if defined(COPROCESS_SUPPORT)
          coproc_closeall ();
#endif

          interactive = false; /* XXX */

          subshell_environment = SUBSHELL_NOFLAGS;

          if (async)
            subshell_environment |= SUBSHELL_ASYNC;

          if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
            subshell_environment |= SUBSHELL_PIPE;

          if (do_redirections (redirects, RX_ACTIVE) == 0)
            exit (EXECUTION_SUCCESS);
          else
            exit (EXECUTION_FAILURE);
        }
      else
        {
          close_pipes (pipe_in, pipe_out);
#if defined(PROCESS_SUBSTITUTION) && defined(HAVE_DEV_FD)
          if (pipe_out == NO_PIPE)
            unlink_fifo_list ();
#endif
          return EXECUTION_SUCCESS;
        }
    }
  else
    {
      /* Even if there aren't any command names, pretend to do the
         redirections that are specified.  The user expects the side
         effects to take place.  If the redirections fail, then return
         failure.  Otherwise, if a command substitution took place while
         expanding the command or a redirection, return the value of that
         substitution.  Otherwise, return EXECUTION_SUCCESS. */

      int r = do_redirections (redirects, RX_ACTIVE | RX_UNDOABLE);
      cleanup_redirects (redirection_undo_list);
      redirection_undo_list = nullptr;

      if (r != 0)
        return EXECUTION_FAILURE;
      else if (last_command_subst_pid != NO_PID)
        return last_command_exit_value;
      else
        return EXECUTION_SUCCESS;
    }
}

/* This is a hack to suppress word splitting for assignment statements
   given as arguments to builtins with the ASSIGNMENT_BUILTIN flag set. */
void
Shell::fix_assignment_words (WORD_LIST *words)
{
  if (words == nullptr)
    return;

  builtin *b = nullptr;
  bool assoc = false, global = false, array = false;

  /* Skip over assignment statements preceding a command name */
  WORD_LIST *wcmd;
  for (wcmd = words; wcmd; wcmd = wcmd->next ())
    if ((wcmd->word->flags & W_ASSIGNMENT) == 0)
      break;

  /* Posix (post-2008) says that `command' doesn't change whether
     or not the builtin it shadows is a `declaration command', even
     though it removes other special builtin properties.  In Posix
     mode, we skip over one or more instances of `command' and
     deal with the next word as the assignment builtin. */
  while (posixly_correct && wcmd && wcmd->word
         && wcmd->word->word == "command")
    wcmd = wcmd->next ();

  for (WORD_LIST *w = wcmd; w; w = w->next ())
    if (w->word->flags & W_ASSIGNMENT)
      {
        /* Lazy builtin lookup, only do it if we find an assignment */
        if (b == nullptr)
          {
            b = builtin_address_internal (wcmd->word->word, false);
            if (b == nullptr || (b->flags & ASSIGNMENT_BUILTIN) == 0)
              return;
            else if (b && (b->flags & ASSIGNMENT_BUILTIN))
              wcmd->word->flags |= W_ASSNBLTIN;
          }
        w->word->flags |= (W_NOSPLIT | W_NOGLOB | W_TILDEEXP | W_ASSIGNARG);
#if defined(ARRAY_VARS)
        if (assoc)
          w->word->flags |= W_ASSIGNASSOC;
        if (array)
          w->word->flags |= W_ASSIGNARRAY;
#endif
        if (global)
          w->word->flags |= W_ASSNGLOBAL;

        /* If we have an assignment builtin that does not create local
           variables, make sure we create global variables even if we
           internally call `declare'.  The CHKLOCAL flag means to set
           attributes or values on an existing local variable, if there is one.
         */
        if (b
            && ((b->flags & (ASSIGNMENT_BUILTIN | LOCALVAR_BUILTIN))
                == ASSIGNMENT_BUILTIN))
          w->word->flags |= (W_ASSNGLOBAL | W_CHKLOCAL);
        else if (b && (b->flags & ASSIGNMENT_BUILTIN)
                 && (b->flags & LOCALVAR_BUILTIN) && variable_context)
          w->word->flags |= W_FORCELOCAL;
      }
#if defined(ARRAY_VARS)
    /* Note that we saw an associative array option to a builtin that takes
       assignment statements.  This is a bit of a kludge. */
    else if (w->word->word[0] == '-'
             && (strpbrk (w->word->word.c_str () + 1, "Aag") != nullptr))
#else
    else if (w->word->word[0] == '-'
             && w->word->word.find ('g', 1) != string::npos)
#endif
      {
        if (b == nullptr)
          {
            b = builtin_address_internal (wcmd->word->word, false);
            if (b == nullptr || (b->flags & ASSIGNMENT_BUILTIN) == 0)
              return;
            else if (b && (b->flags & ASSIGNMENT_BUILTIN))
              wcmd->word->flags |= W_ASSNBLTIN;
          }
        if ((wcmd->word->flags & W_ASSNBLTIN)
            && w->word->word.find ('A', 1) != string::npos)
          assoc = true;
        else if ((wcmd->word->flags & W_ASSNBLTIN)
                 && w->word->word.find ('a', 1) != string::npos)
          array = true;
        if ((wcmd->word->flags & W_ASSNBLTIN)
            && w->word->word.find ('g', 1) != string::npos)
          global = true;
      }
}

#if defined(ARRAY_VARS)
/* Set W_ARRAYREF on words that are valid array references to a builtin that
   accepts them. This is intended to completely replace assoc_expand_once in
   time. */
void
Shell::fix_arrayref_words (WORD_LIST *words)
{
  if (words == nullptr)
    return;

  builtin *b = nullptr;

  /* Skip over assignment statements preceding a command name */
  WORD_LIST *wcmd = words;
  for (wcmd = words; wcmd; wcmd = wcmd->next ())
    if ((wcmd->word->flags & W_ASSIGNMENT) == 0)
      break;

  /* Skip over `command' */
  while (wcmd && wcmd->word && wcmd->word->word == "command")
    wcmd = wcmd->next ();

  if (wcmd == nullptr)
    return;

  /* If it's not an array reference builtin, we have nothing to do. */
  b = builtin_address_internal (wcmd->word->word, false);

  if (b == nullptr || (b->flags & ARRAYREF_BUILTIN) == 0)
    return;

  for (WORD_LIST *w = wcmd->next (); w; w = w->next ())
    {
      if (w->word && !w->word->word.empty ()
          && valid_array_reference (w->word->word, 0))
        w->word->flags |= W_ARRAYREF;
    }
}
#endif

#ifndef ISOPTION
#define ISOPTION(s, c) (s[0] == '-' && s[1] == c && s[2] == 0)
#endif

#define RETURN_NOT_COMMAND()                                                  \
  do                                                                          \
    {                                                                         \
      if (typep)                                                              \
        *typep = 0;                                                           \
      return words;                                                           \
    }                                                                         \
  while (0)

/* Make sure we have `command [-p] command_name [args]', and handle skipping
   over the usual `--' that ends the options.  Returns the updated WORDS with
   the command and options stripped and sets *TYPEP to a non-zero value. If
   any other options are supplied, or there is not a command_name, we punt
   and return a zero value in *TYPEP without updating WORDS. */
WORD_LIST *
Shell::check_command_builtin (WORD_LIST *words, int *typep)
{
  int type;
  WORD_LIST *w;

  w = words->next ();
  type = 1;

  if (w && ISOPTION (w->word->word, 'p')) /* command -p */
    {
#if defined(RESTRICTED_SHELL)
      if (restricted)
        RETURN_NOT_COMMAND ();
#endif
      w = w->next ();
      type = 2;
    }

  if (w && ISOPTION (w->word->word, '-')) /* command [-p] -- */
    w = w->next ();
  else if (w && w->word->word[0] == '-') /* any other option */
    RETURN_NOT_COMMAND ();

  if (w == nullptr || w->word->word.empty ()) /* must have a command_name */
    RETURN_NOT_COMMAND ();

  if (typep)
    *typep = type;

  return w;
}

/* The meaty part of all the executions.  We have to start hacking the
   real execution of commands here.  Fork a process, set things up,
   execute the command. */
int
Shell::execute_simple_command (SIMPLE_COM *simple_command, int pipe_in,
                               int pipe_out, bool async,
                               fd_bitmap *fds_to_close)
{
  QUIT;

  /* If we're in a function, update the line number information. */
  if (variable_context && interactive_shell && sourcelevel == 0)
    {
      /* line numbers in a function start at 1 */
      line_number -= function_line_number - 1;
      if (line_number <= 0)
        line_number = 1;
    }

  /* Remember what this command line looks like at invocation. */
  the_printed_command.clear ();
  simple_command->print (this);

  if (!signal_in_progress (DEBUG_TRAP) && running_trap == 0)
    {
      the_printed_command_except_trap = the_printed_command;
    }

  /* Run the debug trap before each simple command, but do it after we
     update the line number information. */
  int result = run_debug_trap ();
#if defined(DEBUGGER)
  /* In debugging mode, if the DEBUG trap returns a non-zero status, we
     skip the command. */
  if (debugging_mode && result != EXECUTION_SUCCESS)
    return EXECUTION_SUCCESS;
#endif

  cmd_flags cmdflags = simple_command->flags;

  bool first_word_quoted
      = simple_command->words ? (simple_command->words->word->flags & W_QUOTED)
                              : false;

  last_command_subst_pid = NO_PID;
  pid_t old_last_async_pid = last_asynchronous_pid;

  bool already_forked = false;

  /* If we're in a pipeline or run in the background, set DOFORK so we
     make the child early, before word expansion.  This keeps assignment
     statements from affecting the parent shell's environment when they
     should not. */
  bool dofork = pipe_in != NO_PIPE || pipe_out != NO_PIPE || async;

  /* Something like `%2 &' should restart job 2 in the background, not cause
     the shell to fork here. */
  if (dofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE
      && simple_command->words && simple_command->words->word
      && !simple_command->words->word->word.empty ()
      && (simple_command->words->word->word[0] == '%'))
    dofork = false;

  if (dofork)
    {
      /* Do this now, because execute_disk_command will do it anyway in the
         vast majority of cases. */
      maybe_make_export_env ();

      /* Don't let a DEBUG trap overwrite the command string to be saved with
         the process/job associated with this child. */
      make_child_flags fork_flags = async ? FORK_ASYNC : FORK_SYNC;
      if (make_child (the_printed_command_except_trap, fork_flags) == 0)
        {
          already_forked = true;
          cmdflags |= CMD_NO_FORK;

          /* We redo some of what make_child() does with SUBSHELL_IGNTRAP */
          subshell_environment = SUBSHELL_FORK | SUBSHELL_IGNTRAP; /* XXX */

          if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
            subshell_environment |= SUBSHELL_PIPE;

          if (async)
            subshell_environment |= SUBSHELL_ASYNC;

          // We need to do this before piping to handle some really
          // pathological cases where one of the pipe file descriptors is < 2.
          if (fds_to_close)
            close_fd_bitmap (*fds_to_close);

          /* If we fork because of an input pipe, note input pipe for later to
             inhibit async commands from redirecting stdin from /dev/null */
          stdin_redir |= (pipe_in != NO_PIPE);

          do_piping (pipe_in, pipe_out);
          pipe_in = pipe_out = NO_PIPE;

#if defined(COPROCESS_SUPPORT)
          coproc_closeall ();
#endif

          last_asynchronous_pid = old_last_async_pid;

          if (async)
            subshell_level++; /* not for pipes yet */
        }
      else
        {
          /* Don't let simple commands that aren't the last command in a
             pipeline change $? for the rest of the pipeline (or at all). */
          if (pipe_out != NO_PIPE)
            result = last_command_exit_value;
          close_pipes (pipe_in, pipe_out);
          return result;
        }
    }

  QUIT; /* XXX */

  WORD_LIST *words;

  /* If we are re-running this as the result of executing the `command'
     builtin, do not expand the command words a second time. */
  if ((cmdflags & CMD_INHIBIT_EXPANSION) == 0)
    {
      current_fds_to_close = fds_to_close;
      fix_assignment_words (simple_command->words);
#if defined(ARRAY_VARS)
      fix_arrayref_words (simple_command->words);
#endif

      /* Pass the ignore return flag down to command substitutions */
      if (cmdflags & CMD_IGNORE_RETURN) /* XXX */
        comsub_ignore_return++;

      words = expand_words (simple_command->words);

      if (cmdflags & CMD_IGNORE_RETURN)
        comsub_ignore_return--;

      current_fds_to_close = nullptr;
    }
  else
    words = new WORD_LIST (*simple_command->words);

  /* It is possible for WORDS not to have anything left in it.
     Perhaps all the words consisted of `$foo', and there was
     no variable `$foo'. */
  if (words == nullptr)
    {
      this_command_name.clear ();
      result = execute_null_command (simple_command->redirects, pipe_in,
                                     pipe_out, already_forked ? 0 : async);
      if (already_forked)
        sh_exit (result);
      else
        {
          bind_lastarg (nullptr);
          set_pipestatus_from_exit (result);
          return result;
        }
    }

  if (echo_command_at_execute && (cmdflags & CMD_COMMAND_BUILTIN) == 0)
    xtrace_print_word_list (words, 1);

  sh_builtin_func_t builtin = nullptr;
  SHELL_VAR *func = nullptr;
  bool builtin_is_special = false;

  /* This test is still here in case we want to change the command builtin
     handler code below to recursively call execute_simple_command (after
     modifying the simple_command struct). */
  if ((cmdflags & CMD_NO_FUNCTIONS) == 0)
    {
      /* Posix.2 says special builtins are found before functions.  We
         don't set builtin_is_special anywhere other than here, because
         this path is followed only when the `command' builtin is *not*
         being used, and we don't want to exit the shell if a special
         builtin executed with `command builtin' fails.  `command' is not
         a special builtin. */
      if (posixly_correct)
        {
          builtin = find_special_builtin (words->word->word);
          if (builtin)
            builtin_is_special = true;
        }
      if (builtin == nullptr)
        func = find_function (words->word->word);
    }

  /* What happens in posix mode when an assignment preceding a command name
     fails.  This should agree with the code in execute_cmd.c:
     do_assignment_statements(), even though I don't think it's executed any
     more. */
  if (posixly_correct && tempenv_assign_error)
    {
#if defined(DEBUG)
      /* I don't know if this clause is ever executed, so let's check */
      itrace ("execute_simple_command: posix mode tempenv assignment error");
#endif
      last_command_exit_value = EXECUTION_FAILURE;
#if defined(STRICT_POSIX)
      throw bash_exception (interactive_shell ? DISCARD : FORCE_EOF);
#else
      if (!interactive_shell && builtin_is_special)
        throw bash_exception (FORCE_EOF);
      else if (!interactive_shell)
        throw bash_exception (DISCARD); /* XXX - maybe change later */
      else
        throw bash_exception (DISCARD);
#endif
    }
  tempenv_assign_error = false; /* don't care about this any more */

  /* This is where we handle the command builtin as a pseudo-reserved word
     prefix. This allows us to optimize away forks if we can. */
  int old_command_builtin = -1;
  if (builtin == nullptr && func == nullptr)
    {
      WORD_LIST *disposer, *l;
      int cmdtype;

      builtin = find_shell_builtin (words->word->word);
      while (builtin == &Shell::command_builtin)
        {
          disposer = words;
          cmdtype = 0;
          words = check_command_builtin (words, &cmdtype);
          if (cmdtype > 0) /* command -p [--] words */
            {
              for (l = disposer; l->next () != words; l = l->next ())
                ;
              l->set_next (nullptr);
              delete disposer;

              cmdflags |= (CMD_COMMAND_BUILTIN | CMD_NO_FUNCTIONS);
              if (cmdtype == 2)
                cmdflags |= CMD_STDPATH;
              builtin = find_shell_builtin (words->word->word);
            }
          else
            break;
        }
      if (cmdflags & CMD_COMMAND_BUILTIN)
        {
          old_command_builtin = executing_command_builtin;
          executing_command_builtin = true;
        }
      builtin = nullptr;
    }

  const char *command_line = nullptr;
  const string *temp = nullptr;
  string *lastarg = nullptr;
  int old_builtin = 0;

  // Start a try block to handle unwinding on errors.
  try
    {
      QUIT;

      /* Bind the last word in this command to "$_" after execution. */
      WORD_LIST *lastword;
      for (lastword = words; lastword->next (); lastword = lastword->next ())
        ;

      lastarg = &(lastword->word->word);

#if defined(JOB_CONTROL)
      /* Is this command a job control related thing? */
      if (words->word->word[0] == '%' && !already_forked)
        {
          this_command_name = async ? "bg" : "fg";
          last_shell_builtin = this_shell_builtin;
          this_shell_builtin = builtin_address (this_command_name);
          result = ((*this).*this_shell_builtin) (words);
          goto return_result;
        }

      /* One other possibililty.  The user may want to resume an existing
         job. If they do, find out whether this word is a candidate for a
         running job. */
      if (job_control && !already_forked && !async && !first_word_quoted
          && !words->next () && !words->word->word.empty ()
          && !simple_command->redirects && pipe_in == NO_PIPE
          && pipe_out == NO_PIPE && (temp = get_string_value ("auto_resume")))
        {
          get_job_flags jflags = JM_STOPPED | JM_FIRSTMATCH;

          if (*temp == "exact")
            jflags |= JM_EXACT;
          else if (*temp == "substring")
            jflags |= JM_SUBSTRING;
          else
            jflags |= JM_PREFIX;

          int job = get_job_by_name (words->word->word, jflags);
          if (job != NO_JOB)
            {
              delete words;
              if (builtin)
                executing_command_builtin = old_command_builtin;

              this_command_name = "fg";
              last_shell_builtin = this_shell_builtin;
              this_shell_builtin = builtin_address ("fg");

              int started_status = start_job (job, true);
              return (started_status < 0) ? EXECUTION_FAILURE : started_status;
            }
        }
#endif /* JOB_CONTROL */

    run_builtin:
      /* Remember the name of this command globally. */
      this_command_name = words->word->word;

      QUIT;

      /* This command could be a shell builtin or a user-defined function.
         We have already found special builtins by this time, so we do not
         set builtin_is_special.  If this is a function or builtin, and we
         have pipes, then fork a subshell in here.  Otherwise, just execute
         the command directly. */
      if (func == nullptr && builtin == nullptr)
        builtin = find_shell_builtin (this_command_name);

      last_shell_builtin = this_shell_builtin;
      this_shell_builtin = builtin;

      if (builtin || func)
        {
          if (builtin)
            {
              old_builtin = executing_builtin;
              if (old_command_builtin == -1) /* sentinel, can be set above */
                old_command_builtin = executing_command_builtin;
            }
          if (already_forked)
            {
              /* reset_terminating_signals (); */ /* XXX */
              /* Reset the signal handlers in the child, but don't free the
                 trap strings.  Set a flag noting that we have to free the
                 trap strings if we run trap to change a signal
                 disposition. */
              reset_signal_handlers ();
              subshell_environment |= SUBSHELL_RESETTRAP;
              subshell_environment &= ~SUBSHELL_IGNTRAP;

              if (async)
                {
                  if ((cmdflags & CMD_STDIN_REDIR) && pipe_in == NO_PIPE
                      && (stdin_redirects (simple_command->redirects) == 0))
                    async_redirect_stdin ();
                  setup_async_signals ();
                }

              if (!async)
                subshell_level++;

              // This doesn't return.
              execute_subshell_builtin_or_function (
                  words, simple_command->redirects, builtin, func, pipe_in,
                  pipe_out, async, fds_to_close, cmdflags);
            }
          else
            {
              result = execute_builtin_or_function (words, builtin, func,
                                                    simple_command->redirects,
                                                    fds_to_close, cmdflags);
              if (builtin)
                {
                  if (result > EX_SHERRBASE)
                    {
                      switch (result)
                        {
                        case EX_REDIRFAIL:
                        case EX_BADASSIGN:
                        case EX_EXPFAIL:
                          // These errors cause non-interactive posix mode
                          // shells to exit
                          if (posixly_correct && builtin_is_special
                              && !interactive_shell)
                            {
                              last_command_exit_value = EXECUTION_FAILURE;
                              throw bash_exception (ERREXIT);
                            }
                          break;
                        case EX_DISKFALLBACK:
                          /* XXX - experimental */
                          executing_builtin = old_builtin;
                          executing_command_builtin = old_command_builtin;
                          builtin = nullptr;

                          /* The redirections have already been `undone',
                             so this will have to do them again. But piping
                             is forever. */
                          pipe_in = pipe_out = -1;
                          goto execute_from_filesystem;
                        }
                      result = builtin_status (result);

                      /* XXX - take command builtin into account? */
                      if (builtin_is_special)
                        special_builtin_failed = true;
                    }

                  /* In POSIX mode, if there are assignment statements
                     preceding a special builtin, they persist after the
                     builtin completes. */
                  if (posixly_correct && builtin_is_special && temporary_env)
                    merge_temporary_env ();
                }
              else /* function */
                {
                  if (result == EX_USAGE)
                    result = EX_BADUSAGE;
                  else if (result > EX_SHERRBASE)
                    result = builtin_status (result);
                }

              set_pipestatus_from_exit (result);

              goto return_result;
            }
        }

      if (autocd && interactive && words->word
          && is_dirname (words->word->word.c_str ()))
        {
          words = new WORD_LIST (make_word ("--"), words);
          words = new WORD_LIST (make_word ("cd"), words);
          xtrace_print_word_list (words, 0);

          func = find_function ("cd");
          goto run_builtin;
        }

    execute_from_filesystem:

      if (command_line == nullptr)
        command_line = savestring (the_printed_command_except_trap);

#if defined(PROCESS_SUBSTITUTION)
      /* The old code did not test already_forked and only did this if
         subshell_environment & SUBSHELL_COMSUB != 0 (comsubs and
         procsubs). Other uses of the no-fork optimization left FIFOs in
         $TMPDIR */
      if (!already_forked && (cmdflags & CMD_NO_FORK) && fifos_pending () > 0)
        cmdflags &= ~CMD_NO_FORK;
#endif

      result = execute_disk_command (words, simple_command->redirects,
                                     command_line, pipe_in, pipe_out, async,
                                     fds_to_close, cmdflags);
    }
  catch (const std::exception &)
    {
      delete[] command_line;
      delete words;

      if (builtin)
        {
          executing_builtin = old_builtin;
          executing_command_builtin = old_command_builtin;
        }

      this_command_name.clear ();
      throw;
    }

return_result:
  if (lastarg)
    bind_lastarg (*lastarg);

  delete[] command_line;
  delete words;

  if (builtin)
    {
      executing_builtin = old_builtin;
      executing_command_builtin = old_command_builtin;
    }

  this_command_name.clear ();
  return result;
}

/* Translate the special builtin exit statuses.  We don't really need a
   function for this; it's a placeholder for future work. */
static inline int
builtin_status (int result)
{
  int r;

  switch (result)
    {
    case EX_USAGE:
    case EX_BADSYNTAX:
      r = EX_BADUSAGE;
      break;
    case EX_REDIRFAIL:
    case EX_BADASSIGN:
    case EX_EXPFAIL:
      r = EXECUTION_FAILURE;
      break;
    default:
      /* other special exit statuses not yet defined */
      r = (result > EX_SHERRBASE) ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
      break;
    }
  return r;
}

int
Shell::execute_builtin (sh_builtin_func_t builtin, WORD_LIST *words,
                        cmd_flags flags, bool subshell)
{
  /* The eval builtin calls parse_and_execute, which does not know about
     the setting of flags, and always calls the execution functions with
     flags that will exit the shell on an error if -e is set.  If the
     eval builtin is being called, and we're supposed to ignore the exit
     value of the command, we turn the -e flag off ourselves and disable
     the ERR trap, then restore them when the command completes.  This is
     also a problem (as below) for the command and source/. builtins. */
  if (!subshell && (flags & CMD_IGNORE_RETURN)
      && (builtin == &Shell::eval_builtin || (flags & CMD_COMMAND_BUILTIN)
          || builtin == &Shell::source_builtin))
    {
      char saved_exit_on_error = exit_immediately_on_error;
      bool ignexit_flag = builtin_ignoring_errexit;
      const char *error_trap = TRAP_STRING (ERROR_TRAP);
      try
        {
          exit_immediately_on_error = false;
          builtin_ignoring_errexit = true;
          int result = execute_builtin2 (builtin, words, flags, subshell);
          builtin_ignoring_errexit = ignexit_flag;

          exit_immediately_on_error
              = builtin_ignoring_errexit ? 0 : errexit_flag;

          if (error_trap)
            set_error_trap (error_trap);

          return result;
        }
      catch (const std::exception &)
        {
          if (error_trap)
            set_error_trap (error_trap);

          builtin_ignoring_errexit = ignexit_flag;
          exit_immediately_on_error = saved_exit_on_error;
          throw;
        }
    }
  else
    return execute_builtin2 (builtin, words, flags, subshell);
}

// This is execute_builtin, part 2 (after the eval_builtin try/catch block).
int
Shell::execute_builtin2 (sh_builtin_func_t builtin, WORD_LIST *words,
                         cmd_flags flags, bool subshell)
{
  /* The temporary environment for a builtin is supposed to apply to
     all commands executed by that builtin.  Currently, this is a
     problem only with the `unset', `source' and `eval' builtins.
     `mapfile' is a special case because it uses evalstring (same as
     eval or source) to run its callbacks. */
  bool isbltinenv
      = (builtin == &Shell::source_builtin || builtin == &Shell::eval_builtin
         || builtin == &Shell::unset_builtin
         || builtin == &Shell::mapfile_builtin);

  /* SHOULD_KEEP is for the pop_scope call below; it only matters when
     posixly_correct is set, but we should propagate the temporary
     environment to the enclosing environment only for special builtins. */
  bool should_keep = isbltinenv && builtin != &Shell::mapfile_builtin;

#if defined(HISTORY) && defined(READLINE)
  if (builtin == &Shell::fc_builtin || builtin == &Shell::read_builtin)
    {
      isbltinenv = true;
      should_keep = false;
    }
#endif

  if (isbltinenv)
    {
      bool saved_temp_env = false;
      if (temporary_env)
        {
          push_scope (VC_BLTNENV, temporary_env);
          saved_temp_env = true;

          if (flags & CMD_COMMAND_BUILTIN)
            should_keep = false;

          temporary_env = nullptr;
        }

      if (!subshell)
        {
          int saved_evalnest = evalnest;
          int saved_sourcenest = sourcenest;
          try
            {
              if (builtin == &Shell::eval_builtin)
                {
                  if (evalnest_max > 0 && evalnest >= evalnest_max)
                    {
                      internal_error (
                          _ ("eval: maximum eval nesting level exceeded (%d)"),
                          evalnest);
                      evalnest = 0;
                      throw bash_exception (DISCARD);
                    }

                  /* execute_subshell_builtin_or_function sets this to 0 */
                  evalnest++;
                }
              else if (builtin == &Shell::source_builtin)
                {
                  if (sourcenest_max > 0 && sourcenest >= sourcenest_max)
                    {
                      internal_error (
                          _ ("%s: maximum source nesting level exceeded (%d)"),
                          this_command_name.c_str (), sourcenest);
                      sourcenest = 0;
                      throw bash_exception (DISCARD);
                    }

                  /* execute_subshell_builtin_or_function sets this to 0 */
                  sourcenest++;
                }

              return execute_builtin3 (builtin, words, flags, subshell);
            }
          catch (std::exception &)
            {
              // It's easier (and faster?) to always restore both values.
              evalnest = saved_evalnest;
              sourcenest = saved_sourcenest;
              if (saved_temp_env)
                pop_scope (should_keep);
              throw;
            }
        }
    }

  // Call the same method as above, but without the try/catch block.
  return execute_builtin3 (builtin, words, flags, subshell);
}

// This is execute_builtin, part 3 (execute the builtin).
int
Shell::execute_builtin3 (sh_builtin_func_t builtin, WORD_LIST *words,
                         cmd_flags flags, bool subshell)
{
  executing_builtin++;
  executing_command_builtin |= (builtin == &Shell::command_builtin);
  int result;

  /* `return' does a throw to a saved environment in execute_function.
     If a variable assignment list preceded the command, and the shell is
     running in POSIX mode, we need to merge that into the shell_variables
     table, since `return' is a POSIX special builtin. We don't do this if
     it's being run by the `command' builtin, since that's supposed to
     inhibit the special builtin properties. */
  if (posixly_correct && !subshell && builtin == &Shell::return_builtin
      && (flags & CMD_COMMAND_BUILTIN) == 0 && temporary_env)
    {
      try
        {
          result = (((*this).*builtin) (words->next ()));
        }
      catch (const std::exception &)
        {
          merge_temporary_env ();
          throw;
        }
    }
  else
    result = (((*this).*builtin) (words->next ()));

  /* This shouldn't happen, but in case `return' comes back instead of
     throwing, we'll return the value. */
  return result;
}

#if defined(ARRAY_VARS)
void
Shell::restore_funcarray_state (func_array_state *fa)
{
  SHELL_VAR *nfv;
  ARRAY *funcname_a;

  fa->source_a->pop ();
  fa->lineno_a->pop ();

  GET_ARRAY_FROM_VAR ("FUNCNAME", nfv, funcname_a);
  if (nfv == fa->funcname_v)
    funcname_a->pop ();

  delete fa;
}
#endif

// Unwind handler class for execute_function () to restore the shell state
// before returning or throwing.
class ExecFunctionUnwindHandler
{
public:
  // Unwind handler constructor stores the state to be restored.
  ExecFunctionUnwindHandler (bool subshell, Shell *s, SHELL_VAR *var,
                             COMMAND *tc, sh_getopt_state_t *gs,
                             const char *debug_trap, const char *error_trap,
                             const char *return_trap)
  {
    if (!subshell)
      {
        shell = s;
        saved_var = var;
        saved_command = tc;
        saved_getopt_state = gs;
        saved_line_number = s->line_number;
        saved_line_number_for_err_trap = s->line_number_for_err_trap;
        saved_function_line_number = s->function_line_number;
        saved_return_catch_flag = s->return_catch_flag;
        saved_this_shell_function = s->this_shell_function;
        saved_funcnest = s->funcnest;
        saved_loop_level = s->loop_level;
        saved_debugging_mode = s->debugging_mode;

        // function_trace_mode != 0 means that all functions inherit the DEBUG
        // trap. If the function has the trace attribute set, it inherits the
        // DEBUG trap.
        if (debug_trap && (!var->trace () && s->function_trace_mode == 0))
          debug_trap_copy = savestring (debug_trap);
        else
          debug_trap_copy = nullptr;

        // error_trace_mode != 0 means that functions inherit the ERR trap.
        if (error_trap && s->error_trace_mode == 0)
          error_trap_copy = savestring (error_trap);
        else
          error_trap_copy = nullptr;

        // Shell functions inherit the RETURN trap if function tracing is on
        // globally or on individually for this function.
        if (return_trap
            && (s->signal_in_progress (DEBUG_TRAP)
                || (!var->trace () && s->function_trace_mode == 0)))
          return_trap_copy = savestring (return_trap);
        else
          return_trap_copy = nullptr;
      }
  }

  // Destructor to delete temporary data and restore the original values.
  ~ExecFunctionUnwindHandler ()
  {
    if (shell)
      {
        if (saved_debugging_mode)
          shell->pop_args ();

#if defined(ARRAY_VARS)
        if (fa_state)
          shell->restore_funcarray_state (fa_state);
#endif

        shell->loop_level = saved_loop_level;
        shell->funcnest = saved_funcnest;
        shell->this_shell_function = saved_this_shell_function;
        shell->return_catch_flag = saved_return_catch_flag;
        shell->function_line_number = saved_function_line_number;
        shell->line_number_for_err_trap = saved_line_number_for_err_trap;
        shell->line_number = saved_line_number;

        shell->pop_context ();

        delete saved_command; /* XXX - C version doesn't do this? */

        /* This has to be after the pop_context(), because the unwinding of
           local variables may cause the restore of a local declaration of
           OPTIND to force a getopts state reset. */
        // If we have a local copy of OPTIND and it's at the right (current)
        // context, then we restore getopt's internal state. If not, we just
        // let it go. We know there is a local OPTIND if gs->gs_flags & 1.
        // This is set below in execute_function() before the context is run.
        if (saved_getopt_state->gs_flags & 1)
          shell->sh_getopt_restore_istate (saved_getopt_state);
        else
          delete saved_getopt_state;
      }
  }

#if defined(ARRAY_VARS)
  // Add this after construction, if we have array variables.
  void
  add_funcarray_state (func_array_state &fa)
  {
    fa_state = &fa;
  }
#endif

private:
  Shell *shell;
  SHELL_VAR *saved_var;
  COMMAND *saved_command;
  SHELL_VAR *saved_this_shell_function;
  const char *debug_trap_copy;
  const char *error_trap_copy;
  const char *return_trap_copy;
  sh_getopt_state_t *saved_getopt_state;
#if defined(ARRAY_VARS)
  func_array_state *fa_state;
#endif

  int saved_line_number;
  int saved_line_number_for_err_trap;
  int saved_function_line_number;
  int saved_return_catch_flag;
  int saved_funcnest;
  int saved_loop_level;

  char saved_debugging_mode;
};

int
Shell::execute_function (SHELL_VAR *var, WORD_LIST *words, int flags,
                         fd_bitmap *fds_to_close, bool async, bool subshell)
{
#if defined(ARRAY_VARS)
  SHELL_VAR *funcname_v, *bash_source_v, *bash_lineno_v;
  ARRAY *funcname_a;
  ARRAY *bash_source_a;
  ARRAY *bash_lineno_a;
#endif

  if (funcnest_max > 0 && funcnest >= funcnest_max)
    {
      internal_error (_ ("%s: maximum function nesting level exceeded (%d)"),
                      var->name ().c_str (), funcnest);

      funcnest = 0; /* XXX - should we reset it somewhere else? */
      throw bash_exception (DISCARD);
    }

#if defined(ARRAY_VARS)
  GET_ARRAY_FROM_VAR ("FUNCNAME", funcname_v, funcname_a);
  GET_ARRAY_FROM_VAR ("BASH_SOURCE", bash_source_v, bash_source_a);
  GET_ARRAY_FROM_VAR ("BASH_LINENO", bash_lineno_v, bash_lineno_a);
#endif

  /* Be sure to free this copy of the command. */
  COMMAND *tc = var->func_value ()->clone ();

  if (tc && (flags & CMD_IGNORE_RETURN))
    tc->flags |= CMD_IGNORE_RETURN;

  /* A limited attempt at optimization: shell functions at the end of command
     substitutions that are already marked NO_FORK. */
  if (tc && (flags & CMD_NO_FORK) && (subshell_environment & SUBSHELL_COMSUB))
    optimize_shell_function (tc);

  sh_getopt_state_t *gs = sh_getopt_save_istate ();

  /* If the shell is in posix mode, this will push the variables in
     the temporary environment to the "current shell environment" (the
     global scope), and dispose the temporary env before setting it to
     NULL later. This behavior has disappeared from the latest edition
     of the standard, so I will eventually remove it from variables.c:
     push_var_context. */
  push_context (var->name (), subshell, temporary_env);

  temporary_env = nullptr;

  this_shell_function = var;
  make_funcname_visible (true);

  const char *debug_trap = TRAP_STRING (DEBUG_TRAP);
  const char *error_trap = TRAP_STRING (ERROR_TRAP);
  const char *return_trap = TRAP_STRING (RETURN_TRAP);

  // This handler will free memory and restore state on return or throw.
  ExecFunctionUnwindHandler unwind_handler (
      subshell, this, var, tc, gs, debug_trap, error_trap, return_trap);

  /* function_trace_mode != 0 means that all functions inherit the DEBUG
     trap. if the function has the trace attribute set, it inherits the DEBUG
     trap */
  if (debug_trap && (!var->trace () && function_trace_mode == 0))
    restore_default_signal (DEBUG_TRAP);

  /* error_trace_mode != 0 means that functions inherit the ERR trap. */
  if (error_trap && error_trace_mode == 0)
    restore_default_signal (ERROR_TRAP);

  /* Shell functions inherit the RETURN trap if function tracing is on
     globally or on individually for this function. */
  if (return_trap
      && (signal_in_progress (DEBUG_TRAP)
          || (!var->trace () && function_trace_mode == 0)))
    restore_default_signal (RETURN_TRAP);

  funcnest++;

#if defined(ARRAY_VARS)
  /* This is quite similar to the code in shell.cc and elsewhere. */
  FUNCTION_DEF *shell_fn = find_function_def (this_shell_function->name ());

  string sfile;
  if (shell_fn)
    sfile = shell_fn->source_file;

  funcname_a->push (this_shell_function->name ());
  bash_source_a->push (sfile);

  int lineno = GET_LINE_NUMBER ();
  string t (itos (lineno));
  bash_lineno_a->push (t);

  /* restore_funcarray_state() will delete this. */
  func_array_state *fa = new func_array_state ();
  fa->source_a = bash_source_a;
  fa->source_v = bash_source_v;
  fa->lineno_a = bash_lineno_a;
  fa->lineno_v = bash_lineno_v;
  fa->funcname_a = funcname_a;
  fa->funcname_v = funcname_v;

  if (!subshell)
    unwind_handler.add_funcarray_state (*fa);
#endif

  /* The temporary environment for a function is supposed to apply to
     all commands executed within the function body. */

  /* Initialize BASH_ARGC and BASH_ARGV before we blow away the positional
     parameters */
  if (debugging_mode || shell_compatibility_level <= 44)
    init_bash_argv ();

  remember_args (words->next (), true);

  /* Update BASH_ARGV and BASH_ARGC */
  if (debugging_mode)
    push_args (words->next ());

  /* Number of the line on which the function body starts. */
  line_number = function_line_number = tc->line;

#if defined(JOB_CONTROL)
  if (subshell)
    stop_pipeline (async, nullptr);
#endif

  if (shell_compatibility_level > 43)
    loop_level = 0;

  return_catch_flag++;
  int result;

  try
    {
      /* Run the debug trap here so we can trap at the start of a function's
         execution rather than the execution of the body's first command. */
      showing_function_line = true;
      COMMAND *save_current = currently_executing_command;
      result = run_debug_trap ();

#if defined(DEBUGGER)
      /* In debugging mode, if the DEBUG trap returns a non-zero status, we
         skip the command. */
      if (debugging_mode == 0 || result == EXECUTION_SUCCESS)
        {
          showing_function_line = false;
          currently_executing_command = save_current;
          result = execute_command_internal (tc, false, NO_PIPE, NO_PIPE,
                                             fds_to_close);

          /* Run the RETURN trap in the function's context */
          save_current = currently_executing_command;
          run_return_trap ();
          currently_executing_command = save_current;
        }
#else
      result = execute_command_internal (tc, false, NO_PIPE, NO_PIPE,
                                         fds_to_close);

      save_current = currently_executing_command;
      run_return_trap ();
      currently_executing_command = save_current;
#endif
      showing_function_line = false;
    }
  catch (const return_catch_exception &e)
    {
      result = e.return_catch_value;
      /* Run the RETURN trap in the function's context. */
      COMMAND *save_current = currently_executing_command;
      run_return_trap ();
      currently_executing_command = save_current;
    }

  // If we have a local copy of OPTIND, note it in the saved getopts state.
  SHELL_VAR *gv = find_variable ("OPTIND");
  if (gv && gv->context == variable_context)
    gs->gs_flags |= 1;

  if (subshell)
    {
#if defined(ARRAY_VARS)
      restore_funcarray_state (fa);
#endif
      /* Restore BASH_ARGC and BASH_ARGV */
      if (debugging_mode)
        pop_args ();
      /* Fix memory leak. */
      delete tc;
    }

  if (variable_context == 0 || this_shell_function == nullptr)
    {
      make_funcname_visible (false);
#if defined(PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif
    }

  return result;
}

/* Execute a shell builtin or function in a subshell environment.  This
   routine does not return; it only calls exit().  If BUILTIN is non-null,
   it points to a function to call to execute a shell builtin; otherwise
   VAR points at the body of a function to execute.  WORDS is the arguments
   to the command, REDIRECTS specifies redirections to perform before the
   command is executed. */
void
Shell::execute_subshell_builtin_or_function (
    WORD_LIST *words, REDIRECT *redirects, sh_builtin_func_t builtin,
    SHELL_VAR *var, int pipe_in, int pipe_out, bool async,
    fd_bitmap *fds_to_close, cmd_flags flags)
{
#if defined(JOB_CONTROL)
  bool jobs_hack = (builtin == &Shell::jobs_builtin)
                   && ((subshell_environment & SUBSHELL_ASYNC) == 0
                       || pipe_out != NO_PIPE);
#endif

  /* A subshell is neither a login shell nor interactive. */
  login_shell = interactive = 0;

  if (builtin == &Shell::eval_builtin)
    evalnest = 0;
  else if (builtin == &Shell::source_builtin)
    sourcenest = 0;

  if (async)
    subshell_environment |= SUBSHELL_ASYNC;

  if (pipe_in != NO_PIPE || pipe_out != NO_PIPE)
    subshell_environment |= SUBSHELL_PIPE;

  maybe_make_export_env (); /* XXX - is this needed? */

#if defined(JOB_CONTROL)
  /* Eradicate all traces of job control after we fork the subshell, so
     all jobs begun by this subshell are in the same process group as
     the shell itself. */

  /* Allow the output of `jobs' to be piped. */
  if (jobs_hack)
    kill_current_pipeline ();
  else
    without_job_control ();

  set_sigchld_handler ();
#else
  without_job_control ();
#endif /* JOB_CONTROL */

  set_sigint_handler ();

  if (fds_to_close)
    close_fd_bitmap (*fds_to_close);

  do_piping (pipe_in, pipe_out);

  if (do_redirections (redirects, RX_ACTIVE) != 0)
    exit (EXECUTION_FAILURE);

  if (builtin)
    {
      /* Give builtins a place to jump back to on failure, so we don't go back
       * up to main(). */
      try
        {
          int r = execute_builtin (builtin, words, flags, true);
          fflush (stdout);
          if (r == EX_USAGE)
            r = EX_BADUSAGE;
          /* XXX - experimental */
          else if (r == EX_DISKFALLBACK)
            {
              r = execute_disk_command (
                  words, nullptr, the_printed_command_except_trap, -1, -1,
                  async, nullptr, flags | CMD_NO_FORK);
            }
          subshell_exit (r);
        }
      catch (const return_catch_exception &e)
        {
          /* Give the return builtin a place to jump to when executed in a
           * subshell or pipeline. */
          subshell_exit (e.return_catch_value);
        }
      catch (const bash_exception &e)
        {
          if (e.type == EXITPROG || e.type == EXITBLTIN)
            subshell_exit (last_command_exit_value);
          else
            subshell_exit (EXECUTION_FAILURE);
        }
    }
  else
    {
      int r = execute_function (var, words, flags, fds_to_close, async, true);
      fflush (stdout);
      subshell_exit (r);
    }
}

/* Execute a builtin or function in the current shell context.  If BUILTIN
   is non-null, it is the builtin command to execute, otherwise VAR points
   to the body of a function.  WORDS are the command's arguments, REDIRECTS
   are the redirections to perform.  FDS_TO_CLOSE is the usual bitmap of
   file descriptors to close.

   If BUILTIN is exec_builtin, the redirections specified in REDIRECTS are
   not undone before this function returns. */
int
Shell::execute_builtin_or_function (WORD_LIST *words,
                                    sh_builtin_func_t builtin, SHELL_VAR *var,
                                    REDIRECT *redirects,
                                    fd_bitmap *fds_to_close, cmd_flags flags)
{
  int result;
  REDIRECT *saved_undo_list;

#if defined(PROCESS_SUBSTITUTION)
  int ofifo = num_fifos ();
  fifo_vector ofifo_list = copy_fifo_list ();
#endif

  if (do_redirections (redirects, RX_ACTIVE | RX_UNDOABLE) != 0)
    {
      undo_partial_redirects ();
      dispose_exec_redirects ();
      return EX_REDIRFAIL; /* was EXECUTION_FAILURE */
    }

  saved_undo_list = redirection_undo_list;

  /* Calling the "exec" builtin changes redirections forever. */
  if (builtin == &Shell::exec_builtin)
    {
      delete saved_undo_list;
      saved_undo_list = exec_redirection_undo_list;
      exec_redirection_undo_list = nullptr;
    }
  else
    dispose_exec_redirects ();

  redirection_undo_list = nullptr;

  try
    {
      if (builtin)
        result = execute_builtin (builtin, words, flags, false);
      else
        result
            = execute_function (var, words, flags, fds_to_close, false, false);
    }
  catch (const std::exception &)
    {
      cleanup_redirects (saved_undo_list);
      throw;
    }

  /* We do this before undoing the effects of any redirections. */
  fflush (stdout);
  if (ferror (stdout))
    clearerr (stdout);

  /* If we are executing the `command' builtin, but this_shell_builtin is
     set to `exec_builtin', we know that we have something like
     `command exec [redirection]', since otherwise `exec' would have
     overwritten the shell and we wouldn't get here.  In this case, we
     want to behave as if the `command' builtin had not been specified
     and preserve the redirections. */
  if (builtin == &Shell::command_builtin
      && this_shell_builtin == &Shell::exec_builtin)
    {
      if (saved_undo_list)
        delete saved_undo_list;

      redirection_undo_list = exec_redirection_undo_list;
      saved_undo_list = exec_redirection_undo_list = nullptr;
    }

  if (saved_undo_list)
    redirection_undo_list = saved_undo_list;

  undo_partial_redirects ();

#if defined(PROCESS_SUBSTITUTION)
  /* Close any FIFOs created by this builtin or function. */
  int nfifo = num_fifos ();
  if (nfifo > ofifo)
    close_new_fifos (ofifo_list);
#endif

  return result;
}

/* Name of a shell function to call when a command name is not found. */
#ifndef NOTFOUND_HOOK
#define NOTFOUND_HOOK "command_not_found_handle"
#endif

/* Execute a simple command that is hopefully defined in a disk file
   somewhere.

   1) fork ()
   2) connect pipes
   3) look up the command
   4) do redirections
   5) execve ()
   6) If the execve failed, see if the file has executable mode set.
   If so, and it isn't a directory, then execute its contents as
   a shell script.

   Note that the filename hashing stuff has to take place up here,
   in the parent.  This is probably why the Bourne style shells
   don't handle it, since that would require them to go through
   this gnarly hair, for no good reason.

   NOTE: callers expect this to fork or exit(). */
int
Shell::execute_disk_command (WORD_LIST *words, REDIRECT *redirects,
                             const string &command_line, int pipe_in,
                             int pipe_out, bool async, fd_bitmap *fds_to_close,
                             cmd_flags cmdflags)
{
  bool stdpath = (cmdflags & CMD_STDPATH); // use command -p path
  bool nofork = (cmdflags & CMD_NO_FORK); // Don't fork, just exec, if no pipes

  const char *pathname = words->word->word.c_str ();

  char *command;
  int result = EXECUTION_SUCCESS;

#if defined(RESTRICTED_SHELL)
  command = nullptr;
  if (restricted && mbschr (pathname, '/'))
    {
      internal_error (
          _ ("%s: restricted: cannot specify `/' in command names"), pathname);

      result = last_command_exit_value = EXECUTION_FAILURE;

      /* If we're not going to fork below, we must already be in a child
         process or a context in which it's safe to call exit(2).  */
      if (nofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE)
        exit (last_command_exit_value);
      else
        goto parent_return;
    }
#endif /* RESTRICTED_SHELL */

  // If we want to change this so `command -p' (CMD_STDPATH) does not insert
  // any pathname it finds into the hash table, it should read:
  //   command = search_for_command (pathname, stdpath ? CMDSRCH_STDPATH
  //                                                   : CMDSRCH_HASH);
  command = search_for_command (
      pathname, CMDSRCH_HASH | (stdpath ? CMDSRCH_STDPATH : CMDSRCH_NOFLAGS));

  QUIT;

  if (command)
    {
      /* If we're optimizing out the fork (implicit `exec'), decrement the
         shell level like `exec' would do. Don't do this if we are already
         in a pipeline environment, assuming it's already been done. */
      if (nofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE
          && (subshell_environment & SUBSHELL_PIPE) == 0)
        adjust_shell_level (-1);

      maybe_make_export_env ();
      put_command_name_into_env (command);
    }

  /* We have to make the child before we check for the non-existence
     of COMMAND, since we want the error messages to be redirected. */
  /* If we can get away without forking and there are no pipes to deal with,
     don't bother to fork, just directly exec the command. */
  pid_t pid;
  if (nofork && pipe_in == NO_PIPE && pipe_out == NO_PIPE)
    pid = 0;
  else
    {
      make_child_flags fork_flags = async ? FORK_ASYNC : FORK_SYNC;
      pid = make_child (command_line, fork_flags);
    }

  if (pid == 0)
    {
      bool old_interactive;

      reset_terminating_signals (); /* XXX */
      /* Cancel traps, in trap.c. */
      restore_original_signals ();
      subshell_environment &= ~SUBSHELL_IGNTRAP;

      /* restore_original_signals may have undone the work done
         by make_child to ensure that SIGINT and SIGQUIT are ignored
         in asynchronous children. */
      if (async)
        {
          if ((cmdflags & CMD_STDIN_REDIR) && pipe_in == NO_PIPE
              && (stdin_redirects (redirects) == 0))
            async_redirect_stdin ();
          setup_async_signals ();
        }

      /* This functionality is now provided by close-on-exec of the
         file descriptors manipulated by redirection and piping.
         Some file descriptors still need to be closed in all children
         because of the way bash does pipes; fds_to_close is a
         bitmap of all such file descriptors. */
      if (fds_to_close)
        close_fd_bitmap (*fds_to_close);

      do_piping (pipe_in, pipe_out);

      old_interactive = interactive;
      if (async)
        interactive = false;

      subshell_environment |= SUBSHELL_FORK; /* XXX - was just = */

#if defined(PROCESS_SUBSTITUTION) && !defined(HAVE_DEV_FD)
      clear_fifo_list (); /* XXX - we haven't created any FIFOs */
#endif

      /* reset shell_pgrp to pipeline_pgrp here for word expansions performed
         by the redirections here? */
      if (redirects && (do_redirections (redirects, RX_ACTIVE) != 0))
        {
#if defined(PROCESS_SUBSTITUTION)
          /* Try to remove named pipes that may have been created as the
             result of redirections. */
          unlink_all_fifos ();
#endif /* PROCESS_SUBSTITUTION */
          exit (EXECUTION_FAILURE);
        }

#if defined(PROCESS_SUBSTITUTION) && !defined(HAVE_DEV_FD)
      /* This should only contain FIFOs created as part of redirection
         expansion. */
      unlink_all_fifos ();
#endif

      if (async)
        interactive = old_interactive;

      if (command == nullptr)
        {
          SHELL_VAR *hookf = find_function (NOTFOUND_HOOK);
          if (hookf == nullptr)
            {
              /* Make sure filenames are displayed using printable characters
               */
              string printname (printable_filename (pathname, 0));
              internal_error (_ ("%s: command not found"), printname.c_str ());
              exit (EX_NOTFOUND); /* Posix.2 says the exit status is 127 */
            }

          /* We don't want to manage process groups for processes we start
             from here, so we turn off job control and don't attempt to
             manipulate the terminal's process group. */
          without_job_control ();

#if defined(JOB_CONTROL)
          set_sigchld_handler ();
#endif

          WORD_LIST *wl = new WORD_LIST (make_word (NOTFOUND_HOOK), words);
          exit (execute_shell_function (hookf, wl));
        }

      /* Execve expects the command name to be in args[0].  So we
         leave it there, in the same format that the user used to
         type it in. */
      char **args = strvec_from_word_list (words, 0, nullptr);
      exit (shell_execve (command, args, export_env));
    }
  else
    {
    parent_return:
      QUIT;

      /* Make sure that the pipes are closed in the parent. */
      close_pipes (pipe_in, pipe_out);
#if defined(PROCESS_SUBSTITUTION) && defined(HAVE_DEV_FD)
#if 0
      if (variable_context == 0)
        unlink_fifo_list ();
#endif
#endif
      delete[] command;
      return result;
    }
}

/* CPP defines to decide whether a particular index into the #! line
   corresponds to a valid interpreter name or argument character, or
   whitespace.  The MSDOS define is to allow \r to be treated the same
   as \n. This is also useful for OpenVMS. */

#if !defined(MSDOS) && !defined(__VMS)
#define STRINGCHAR(ind)                                                       \
  (ind < sample_len && !whitespace (sample[ind]) && sample[ind] != '\n')
#if !defined(HAVE_HASH_BANG_EXEC)
#define WHITECHAR(ind) (ind < sample_len && whitespace (sample[ind]))
#endif
#else /* MSDOS || __VMS */
#define STRINGCHAR(ind)                                                       \
  (ind < sample_len && !whitespace (sample[ind]) && sample[ind] != '\n'       \
   && sample[ind] != '\r')
#if !defined(HAVE_HASH_BANG_EXEC)
#define WHITECHAR(ind) (ind < sample_len && whitespace (sample[ind]))
#endif
#endif /* !MSDOS && !__VMS */

static inline char *
getinterp (const char *sample, ssize_t sample_len, int *endp)
{
  int i;
  char *execname;
  int start;

  /* Find the name of the interpreter to exec. */
  for (i = 2; i < sample_len && whitespace (sample[i]); i++)
    ;

  for (start = i; STRINGCHAR (i); i++)
    ;

  execname = substring (sample, static_cast<size_t> (start),
                        static_cast<size_t> (i));

  if (endp)
    *endp = i;
  return execname;
}

#if !defined(HAVE_HASH_BANG_EXEC)
/* If the operating system on which we're running does not handle
   the #! executable format, then help out.  SAMPLE is the text read
   from the file, SAMPLE_LEN characters.  COMMAND is the name of
   the script; it and ARGS, the arguments given by the user, will
   become arguments to the specified interpreter.  ENV is the environment
   to pass to the interpreter.

   The word immediately following the #! is the interpreter to execute.
   A single argument to the interpreter is allowed. */
int
Shell::execute_shell_script (char *sample, ssize_t sample_len,
                             const char *command, char **args, char **env)
{
  int i, start;
  char *firstarg = nullptr;

  /* Find the name of the interpreter to exec. */
  string execname (getinterp (sample, sample_len, &i));
  int size_increment = 1;

  /* Now the argument, if any. */
  for (start = i; WHITECHAR (i); i++)
    ;

  /* If there is more text on the line, then it is an argument for the
     interpreter. */

  if (STRINGCHAR (i))
    {
      for (start = i; STRINGCHAR (i); i++)
        ;
      firstarg = substring (sample, static_cast<size_t> (start),
                            static_cast<size_t> (i));
      size_increment = 2;
    }

  int argc = static_cast<int> (strvec_len (args));
  int larry = argc + size_increment;
  char **new_args = new char *[static_cast<size_t> (larry + 1)];
  memcpy (new_args + size_increment, args, static_cast<size_t> (argc + 1));
  delete[] args;
  args = new_args;

  for (i = larry - 1; i; i--)
    args[i] = args[i - size_increment];

  args[0] = const_cast<char *> (execname.c_str ());
  if (firstarg)
    {
      args[1] = firstarg;
      args[2] = const_cast<char *> (command);
    }
  else
    args[1] = const_cast<char *> (command);

  args[larry] = nullptr;

  return shell_execve (execname.c_str (), args, env);
}
#endif /* !HAVE_HASH_BANG_EXEC */

#undef STRINGCHAR
#undef WHITECHAR

void
Shell::initialize_subshell ()
{
#if defined(ALIAS)
  /* Forget about any aliases that we knew of.  We are in a subshell. */
  delete_all_aliases ();
#endif /* ALIAS */

#if defined(HISTORY)
  /* Forget about the history lines we have read.  This is a non-interactive
     subshell. */
  history_lines_this_session = 0;
#endif

  /* Forget about the way job control was working. We are in a subshell. */
  without_job_control ();

#if defined(JOB_CONTROL)
  set_sigchld_handler ();
  init_job_stats ();
#endif /* JOB_CONTROL */

  /* Reset the values of the shell flags and options. */
  reset_shell_flags ();
  reset_shell_options ();
  reset_shopt_options ();

  /* Zero out builtin_env, since this could be a shell script run from a
     sourced file with a temporary environment supplied to the `source/.'
     builtin.  Such variables are not supposed to be exported (empirical
     testing with sh and ksh).  Just throw it away; don't worry about a
     memory leak. */
  if (shell_variables->builtin_env ())
    shell_variables = shell_variables->down;

  /* XXX -- are there other things we should be resetting here? */
  parse_and_execute_level = 0; /* nothing left to restore it */

  /* We're no longer inside a shell function. */
  variable_context = return_catch_flag = funcnest = evalnest = sourcenest = 0;

  executing_list = 0; /* XXX */

  /* If we're not interactive, close the file descriptor from which we're
     reading the current shell script. */
  if (!interactive_shell)
    unset_bash_input (0);
}

#define HASH_BANG_BUFSIZ 128

#define READ_SAMPLE_BUF(file, buf, len)                                       \
  do                                                                          \
    {                                                                         \
      fd = open (file, O_RDONLY);                                             \
      if (fd >= 0)                                                            \
        {                                                                     \
          len = read (fd, buf, HASH_BANG_BUFSIZ);                             \
          close (fd);                                                         \
        }                                                                     \
      else                                                                    \
        len = -1;                                                             \
    }                                                                         \
  while (0)

/* Call execve (), handling interpreting shell scripts, and handling
   exec failures. */
int
Shell::shell_execve (const char *command, char **args, char **env)
{
  int i, fd;
  char sample[HASH_BANG_BUFSIZ];
  ssize_t sample_len;

  execve (command, args, env);
  i = errno; /* error from execve() */
  CHECK_TERMSIG;

  /* If we get to this point, then start checking out the file.
     Maybe it is something we can hack ourselves. */
  if (i != ENOEXEC)
    {
      /* make sure this is set correctly for file_error/report_error */
      last_command_exit_value
          = (i == ENOENT)
                ? EX_NOTFOUND
                : EX_NOEXEC; /* XXX Posix.2 says that exit status is 126 */

      if (file_isdir (command))
#if defined(EISDIR)
        internal_error (_ ("%s: %s"), command, strerror (EISDIR));
#else
        internal_error (_ ("%s: is a directory"), command);
#endif
      else if (!executable_file (command))
        {
          errno = i;
          file_error (command);
        }
      /* errors not involving the path argument to execve. */
      else if (i == E2BIG || i == ENOMEM)
        {
          errno = i;
          file_error (command);
        }
      else if (i == ENOENT)
        {
          errno = i;
          internal_error (_ ("%s: cannot execute: required file not found"),
                          command);
        }
      else
        {
          /* The file has the execute bits set, but the kernel refuses to
             run it for some reason.  See why. */
#if defined(HAVE_HASH_BANG_EXEC)
          READ_SAMPLE_BUF (command, sample, sample_len);
          if (sample_len > 0)
            sample[sample_len - 1] = '\0';
          if (sample_len > 2 && sample[0] == '#' && sample[1] == '!')
            {
              string interp (getinterp (sample, sample_len, nullptr));
              size_t ilen = interp.size ();
              errno = i;
              if (interp[ilen - 1] == '\r')
                {
                  interp[ilen - 1] = '^';
                  interp.push_back ('M');
                }
              sys_error (_ ("%s: %s: bad interpreter"), command,
                         interp.c_str ());
              return EX_NOEXEC;
            }
#endif
          errno = i;
          file_error (command);
        }
      return last_command_exit_value;
    }

  /* This file is executable.
     If it begins with #!, then help out people with losing operating
     systems.  Otherwise, check to see if it is a binary file by seeing
     if the contents of the first line (or up to 128 characters) are in the
     ASCII set.  If it's a text file, execute the contents as shell commands,
     otherwise return 126 (EX_BINARY_FILE). */
  READ_SAMPLE_BUF (command, sample, sample_len);

  if (sample_len == 0)
    return EXECUTION_SUCCESS;

  /* Is this supposed to be an executable script?
     If so, the format of the line is "#! interpreter [argument]".
     A single argument is allowed.  The BSD kernel restricts
     the length of the entire line to 32 characters (32 bytes
     being the size of the BSD exec header), but we allow up to 128
     characters. */
  if (sample_len > 0)
    {
#if !defined(HAVE_HASH_BANG_EXEC)
      if (sample_len > 2 && sample[0] == '#' && sample[1] == '!')
        return execute_shell_script (sample, sample_len, command, args, env);
      else
#endif
          if (check_binary_file (sample, static_cast<size_t> (sample_len)))
        {
          internal_error (_ ("%s: cannot execute binary file: %s"), command,
                          strerror (i));
          errno = i;
          return EX_BINARY_FILE;
        }
    }

  /* We have committed to attempting to execute the contents of this file
     as shell commands. */

  reset_parser ();
  initialize_subshell ();

  set_sigint_handler ();

  /* Insert the name of this shell into the argument list. */
  size_t larray = strvec_len (args) + 1;
  char **new_args = new char *[larray + 1];
  memcpy (new_args + 1, args, larray);
  delete[] args;
  args = new_args;

  args[0] = const_cast<char *> (shell_name);
  args[1] = const_cast<char *> (command);
  args[larray] = nullptr;

  if (args[0][0] == '-')
    args[0]++;

#if defined(RESTRICTED_SHELL)
  if (restricted)
    change_flag ('r', FLAG_OFF);
#endif

  if (subshell_argv)
    {
      /* Can't free subshell_argv[0]; that is shell_name. */
      for (i = 1; i < subshell_argc; i++)
        delete[] subshell_argv[i];

      delete[] subshell_argv;
    }

  delete currently_executing_command; /* XXX */
  currently_executing_command = nullptr;

  subshell_argc = static_cast<int> (larray);
  subshell_argv = args;
  subshell_envp = env;

  unbind_args (); /* remove the positional parameters */

#if defined(PROCESS_SUBSTITUTION) && defined(HAVE_DEV_FD)
  clear_fifo_list (); /* pipe fds are what they are now */
#endif

  throw bash_exception (FORCE_EOF);
}

int
Shell::execute_intern_function (WORD_DESC *name, FUNCTION_DEF *funcdef)
{
  SHELL_VAR *var;

  if (!check_identifier (name, posixly_correct))
    {
      if (posixly_correct && !interactive_shell)
        {
          last_command_exit_value = EX_BADUSAGE;
          throw bash_exception (ERREXIT);
        }
      return EXECUTION_FAILURE;
    }

  if (name->word.find (CTLESC) != string::npos) /* WHY? */
    {
      name->word = dequote_escapes (name->word);
    }

  /* Posix interpretation 383 */
  if (posixly_correct && find_special_builtin (name->word))
    {
      internal_error (_ ("`%s': is a special builtin"), name->word.c_str ());
      last_command_exit_value = EX_BADUSAGE;
      throw bash_exception (interactive_shell ? DISCARD : ERREXIT);
    }

  var = find_function (name->word);
  if (var && (var->readonly () || var->noassign ()))
    {
      if (var->readonly ())
        internal_error (_ ("%s: readonly function"), var->name ().c_str ());
      return EXECUTION_FAILURE;
    }

#if defined(DEBUGGER)
  bind_function_def (name->word, funcdef, true);
#endif

  bind_function (name->word, funcdef->command);
  return EXECUTION_SUCCESS;
}

/* Redirect input and output to be from and to the specified pipes.
   NO_PIPE and REDIRECT_BOTH are handled correctly. */
void
Shell::do_piping (int pipe_in, int pipe_out)
{
  if (pipe_in != NO_PIPE)
    {
      if (dup2 (pipe_in, 0) < 0)
        dup_error (pipe_in, 0);
      if (pipe_in > 0)
        close (pipe_in);
#ifdef __CYGWIN__
      /* Let stdio know the fd may have changed from text to binary mode. */
      freopen (nullptr, "r", stdin);
#endif /* __CYGWIN__ */
    }
  if (pipe_out != NO_PIPE)
    {
      if (pipe_out != REDIRECT_BOTH)
        {
          if (dup2 (pipe_out, 1) < 0)
            dup_error (pipe_out, 1);
          if (pipe_out == 0 || pipe_out > 1)
            close (pipe_out);
        }
      else
        {
          if (dup2 (1, 2) < 0)
            dup_error (1, 2);
        }
#ifdef __CYGWIN__
      /* Let stdio know the fd may have changed from text to binary mode, and
         make sure to preserve stdout line buffering. */
      freopen (nullptr, "w", stdout);
      sh_setlinebuf (stdout);
#endif /* __CYGWIN__ */
    }
}

} // namespace bash
