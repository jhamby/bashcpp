// This file is kill_def.cc.
// It implements the builtin "kill" in Bash.

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

// $BUILTIN kill
// $FUNCTION kill_builtin
// $SHORT_DOC kill [-s sigspec | -n signum | -sigspec] pid | jobspec ... or kill -l [sigspec]
// Send a signal to a job.

// Send the processes identified by PID or JOBSPEC the signal named by
// SIGSPEC or SIGNUM.  If neither SIGSPEC nor SIGNUM is present, then
// SIGTERM is assumed.

// Options:
//   -s sig	SIG is a signal name
//   -n sig	SIG is a signal number
//   -l	list the signal names; if arguments follow `-l' they are
// 		assumed to be signal numbers for which names should be listed
//   -L	synonym for -l

// Kill is a shell builtin for two reasons: it allows job IDs to be used
// instead of process IDs, and allows processes to be killed if the limit
// on processes that you can create is reached.

// Exit Status:
// Returns success unless an invalid option is given or an error occurs.
// $END

#include "config.h"

#include <unistd.h>

#include "bashintl.hh"

#include "common.hh"
#include "jobs.hh"
#include "shell.hh"
#include "trap.hh"

namespace bash
{

static void kill_error (pid_t, int);

#if !defined(CONTINUE_AFTER_KILL_ERROR)
#define CONTINUE_OR_FAIL return (EXECUTION_FAILURE)
#else
#define CONTINUE_OR_FAIL goto continue_killing
#endif /* CONTINUE_AFTER_KILL_ERROR */

/* Here is the kill builtin.  We only have it so that people can type
   kill -KILL %1?  No, if you fill up the process table this way you
   can still kill some. */
int
Shell::kill_builtin (WORD_LIST *list)
{
  int sig, dflags;
  const char *sigspec, *word;
  pid_t pid;
  int64_t pid_value;

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  CHECK_HELPOPT (list);

  bool any_succeeded = false, listing = false, saw_signal = false;
  sig = SIGTERM;
  sigspec = "TERM";

  dflags = DSIG_NOCASE | ((posixly_correct == 0) ? DSIG_SIGPREFIX : 0);
  /* Process options. */
  while (list)
    {
      word = list->word->word;

      if (ISOPTION (word, 'l') || ISOPTION (word, 'L'))
        {
          listing = true;
          list = (WORD_LIST *)list->next;
        }
      else if (ISOPTION (word, 's') || ISOPTION (word, 'n'))
        {
          list = (WORD_LIST *)list->next;
          if (list)
            {
              sigspec = list->word->word;
            use_sigspec:
              if (sigspec[0] == '0' && sigspec[1] == '\0')
                sig = 0;
              else
                sig = decode_signal (sigspec, dflags);
              list = (WORD_LIST *)list->next;
              saw_signal = true;
            }
          else
            {
              sh_needarg (word);
              return EXECUTION_FAILURE;
            }
        }
      else if (word[0] == '-' && word[1] == 's' && ISALPHA (word[2]))
        {
          sigspec = word + 2;
          goto use_sigspec;
        }
      else if (word[0] == '-' && word[1] == 'n' && ISDIGIT (word[2]))
        {
          sigspec = word + 2;
          goto use_sigspec;
        }
      else if (ISOPTION (word, '-'))
        {
          list = (WORD_LIST *)list->next;
          break;
        }
      else if (ISOPTION (word, '?'))
        {
          builtin_usage ();
          return EX_USAGE;
        }
      /* If this is a signal specification then process it.  We only process
         the first one seen; other arguments may signify process groups (e.g,
         -num == process group num). */
      else if (*word == '-' && saw_signal == 0)
        {
          sigspec = word + 1;
          sig = decode_signal (sigspec, dflags);
          saw_signal = true;
          list = (WORD_LIST *)list->next;
        }
      else
        break;
    }

  if (listing)
    return display_signal_list (list, 0);

  /* OK, we are killing processes. */
  if (sig == NO_SIG)
    {
      sh_invalidsig (sigspec);
      return EXECUTION_FAILURE;
    }

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  while (list)
    {
      word = list->word->word;

      if (*word == '-')
        word++;

      /* Use the entire argument in case of minus sign presence. */
      if (*word && legal_number (list->word->word, &pid_value)
          && (pid_value == (pid_t)pid_value))
        {
          pid = (pid_t)pid_value;

          if (kill_pid (pid, sig, pid < -1) < 0)
            {
              if (errno == EINVAL)
                sh_invalidsig (sigspec);
              else
                kill_error (pid, errno);
              CONTINUE_OR_FAIL;
            }
          else
            any_succeeded = true;
        }
#if defined(JOB_CONTROL)
      else if (*list->word->word && *list->word->word != '%')
        {
          builtin_error (_ ("%s: arguments must be process or job IDs"),
                         list->word->word);
          CONTINUE_OR_FAIL;
        }
      else if (*word)
        /* Posix.2 says you can kill without job control active (4.32.4) */
        { /* Must be a job spec.  Check it out. */
          sigset_t set, oset;

          BLOCK_CHILD (set, oset);
          int job = get_job_spec (list);

          if (INVALID_JOB (job))
            {
              if (job != DUP_JOB)
                sh_badjob (list->word->word);
              UNBLOCK_CHILD (oset);
              CONTINUE_OR_FAIL;
            }

          JOB *j = get_job_by_jid (job);
          /* Job spec used.  Kill the process group. If the job was started
             without job control, then its pgrp == shell_pgrp, so we have
             to be careful.  We take the pid of the first job in the pipeline
             in that case. */
          pid = IS_JOBCONTROL (job) ? j->pgrp : j->pipe->pid;

          UNBLOCK_CHILD (oset);

          if (kill_pid (pid, sig, 1) < 0)
            {
              if (errno == EINVAL)
                sh_invalidsig (sigspec);
              else
                kill_error (pid, errno);
              CONTINUE_OR_FAIL;
            }
          else
            any_succeeded = true;
        }
#endif /* !JOB_CONTROL */
      else
        {
          sh_badpid (list->word->word);
          CONTINUE_OR_FAIL;
        }
    continue_killing:
      list = (WORD_LIST *)list->next;
    }

  return any_succeeded ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
}

static void
kill_error (pid_t pid, int e)
{
  const char *x;

  x = strerror (e);
  if (x == 0)
    x = _ ("Unknown error");
  builtin_error ("(%ld) - %s", (long)pid, x);
}

} // namespace bash
