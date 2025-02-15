// This file is suspend_def.cc.
// It implements the builtin "suspend" in Bash.

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

// $BUILTIN suspend
// $DEPENDS_ON JOB_CONTROL
// $FUNCTION suspend_builtin
// $SHORT_DOC suspend [-f]
// Suspend shell execution.

// Suspend the execution of this shell until it receives a SIGCONT signal.
// Unless forced, login shells cannot be suspended.

// Options:
//   -f	force the suspend, even if the shell is a login shell

// Exit Status:
// Returns success unless job control is not enabled or an error occurs.
// $END

#include "config.h"

#if defined(JOB_CONTROL)

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashtypes.hh"

#include <csignal>

#include "bashintl.hh"
#include "common.hh"
#include "jobs.hh"
#include "shell.hh"

namespace bash
{

static void suspend_continue (int);

#if 0
static SigHandler *old_cont;
static SigHandler *old_stop;
#endif

/* Continue handler. */
static void
suspend_continue (int sig)
{
  set_signal_handler (SIGCONT, old_cont);
}

/* Suspending the shell.  If -f is the arg, then do the suspend
   no matter what.  Otherwise, complain if a login shell. */
int
Shell::suspend_builtin (WORD_LIST *list)
{
  int opt, force;

  reset_internal_getopt ();
  force = 0;
  while ((opt = internal_getopt (list, "f")) != -1)
    switch (opt)
      {
      case 'f':
        force++;
        break;
        CASE_HELPOPT;
      default:
        builtin_usage ();
        return EX_USAGE;
      }

  list = loptend;

  if (job_control == 0)
    {
      sh_nojobs (_ ("cannot suspend"));
      return EXECUTION_FAILURE;
    }

  if (force == 0)
    {
      no_args (list);

      if (login_shell)
        {
          builtin_error (_ ("cannot suspend a login shell"));
          return EXECUTION_FAILURE;
        }
    }

  /* XXX - should we put ourselves back into the original pgrp now?  If so,
     call end_job_control() here and do the right thing in suspend_continue
     (that is, call restart_job_control()). */
  old_cont = (SigHandler *)set_signal_handler (SIGCONT, suspend_continue);
#if 0
  old_stop = (SigHandler *)set_signal_handler (SIGSTOP, SIG_DFL);
#endif
  killpg (shell_pgrp, SIGSTOP);
  return EXECUTION_SUCCESS;
}

#endif /* JOB_CONTROL */

} // namespace bash
