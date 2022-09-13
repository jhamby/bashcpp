// This file is exec.def, from which is created exec.c.
// It implements the builtin "exec" in Bash.

// Copyright (C) 1987-2019 Free Software Foundation, Inc.

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

// $PRODUCES exec.c

// $BUILTIN exec
// $FUNCTION exec_builtin
// $SHORT_DOC exec [-cl] [-a name] [command [argument ...]] [redirection ...]
// Replace the shell with the given command.

// Execute COMMAND, replacing this shell with the specified program.
// ARGUMENTS become the arguments to COMMAND.  If COMMAND is not specified,
// any redirections take effect in the current shell.

// Options:
//   -a name	pass NAME as the zeroth argument to COMMAND
//   -c	execute COMMAND with an empty environment
//   -l	place a dash in the zeroth argument to COMMAND

// If the command cannot be executed, a non-interactive shell exits, unless
// the shell option `execfail' is set.

// Exit Status:
// Returns success unless COMMAND is not found or a redirection error occurs.
// $END

#include "config.hh"

#include "bashtypes.hh"
#include "posixstat.hh"

#include <cerrno>
#include <csignal>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <cstdio>

#include "bashintl.hh"

#include "execute_cmd.hh"
#include "findcmd.hh"
#include "shell.hh"

#if defined(JOB_CONTROL)
#include "jobs.hh"
#endif

#include "bashgetopt.hh"
#include "common.hh"
#include "flags.hh"
#include "input.hh"
#include "trap.hh"

namespace bash
{

extern REDIRECT *redirection_undo_list;
extern char *exec_argv0;

int no_exit_on_failed_exec;

/* If the user wants this to look like a login shell, then
   prepend a `-' onto NAME and return the new name. */
static char *
mkdashname (const char *name)
{
  char *ret;

  ret = (char *)xmalloc (2 + strlen (name));
  ret[0] = '-';
  strcpy (ret + 1, name);
  return ret;
}

int
Shell::exec_builtin (WORD_LIST *list)
{
  int exit_value = EXECUTION_FAILURE;
  int cleanenv, login, opt, orig_job_control;
  char *argv0, *command, **args, **env, *newname, *com2;

  cleanenv = login = 0;
  exec_argv0 = argv0 = (char *)NULL;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "cla:")) != -1)
    {
      switch (opt)
        {
        case 'c':
          cleanenv = 1;
          break;
        case 'l':
          login = 1;
          break;
        case 'a':
          argv0 = list_optarg;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  /* First, let the redirections remain. */
  dispose_redirects (redirection_undo_list);
  redirection_undo_list = (REDIRECT *)NULL;

  if (list == 0)
    return EXECUTION_SUCCESS;

#if defined(RESTRICTED_SHELL)
  if (restricted)
    {
      sh_restricted ((char *)NULL);
      return EXECUTION_FAILURE;
    }
#endif /* RESTRICTED_SHELL */

  args = strvec_from_word_list (list, 1, 0, (int *)NULL);
  env = (char **)0;

  /* A command with a slash anywhere in its name is not looked up in $PATH. */
  command
      = absolute_program (args[0]) ? args[0] : search_for_command (args[0], 1);

  if (command == 0)
    {
      if (file_isdir (args[0]))
        {
#if defined(EISDIR)
          builtin_error (_ ("%s: cannot execute: %s"), args[0],
                         strerror (EISDIR));
#else
          builtin_error (_ ("%s: cannot execute: %s"), args[0],
                         strerror (errno));
#endif
          exit_value = EX_NOEXEC;
        }
      else
        {
          sh_notfound (args[0]);
          exit_value = EX_NOTFOUND; /* As per Posix.2, 3.14.6 */
        }
      goto failed_exec;
    }

  com2 = full_pathname (command);
  if (com2)
    {
      if (command != args[0])
        free (command);
      command = com2;
    }

  if (argv0)
    {
      free (args[0]);
      args[0] = login ? mkdashname (argv0) : savestring (argv0);
      exec_argv0 = savestring (args[0]);
    }
  else if (login)
    {
      newname = mkdashname (args[0]);
      free (args[0]);
      args[0] = newname;
    }

  /* Decrement SHLVL by 1 so a new shell started here has the same value,
     preserving the appearance.  After we do that, we need to change the
     exported environment to include the new value.  If we've already forked
     and are in a subshell, we don't want to decrement the shell level,
     since we are `increasing' the level */

  if (cleanenv == 0 && (subshell_environment & SUBSHELL_PAREN) == 0)
    adjust_shell_level (-1);

  if (cleanenv)
    {
      env = strvec_create (1);
      env[0] = (char *)0;
    }
  else
    {
      maybe_make_export_env ();
      env = export_env;
    }

#if defined(HISTORY)
  if (interactive_shell && subshell_environment == 0)
    maybe_save_shell_history ();
#endif /* HISTORY */

  restore_original_signals ();

#if defined(JOB_CONTROL)
  orig_job_control = job_control; /* XXX - was also interactive_shell */
  if (subshell_environment == 0)
    end_job_control ();
  if (interactive || job_control)
    default_tty_job_signals (); /* undo initialize_job_signals */
#endif                          /* JOB_CONTROL */

#if defined(BUFFERED_INPUT)
  if (default_buffered_input >= 0)
    sync_buffered_stream (default_buffered_input);
#endif

  exit_value = shell_execve (command, args, env);

  /* We have to set this to NULL because shell_execve has called realloc()
     to stuff more items at the front of the array, which may have caused
     the memory to be freed by realloc().  We don't want to free it twice. */
  args = (char **)NULL;
  if (cleanenv == 0)
    adjust_shell_level (1);

  if (exit_value == EX_NOTFOUND) /* no duplicate error message */
    goto failed_exec;
  else if (executable_file (command) == 0)
    {
      builtin_error (_ ("%s: cannot execute: %s"), command, strerror (errno));
      exit_value = EX_NOEXEC; /* As per Posix.2, 3.14.6 */
    }
  else
    file_error (command);

failed_exec:
  FREE (command);

  if (subshell_environment
      || (interactive == 0 && no_exit_on_failed_exec == 0))
    exit_shell (exit_value);

  if (args)
    strvec_dispose (args);

  if (env && env != export_env)
    strvec_dispose (env);

  initialize_traps ();
  initialize_signals (1);

#if defined(JOB_CONTROL)
  if (orig_job_control)
    restart_job_control ();
#endif /* JOB_CONTROL */

  return exit_value;
}

} // namespace bash
