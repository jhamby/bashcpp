/* flags.cc -- Everything about flags except the `set' command.  That
   is in builtins.c */

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

namespace bash
{

/* **************************************************************** */
/*								    */
/*			The Flags ALIST.			    */
/*								    */
/* **************************************************************** */

// Called during startup
void
Shell::initialize_flags ()
{
  /* Standard sh flags. */
  shell_flags.push_back (flags_alist ('a', &mark_modified_vars));
#if defined(JOB_CONTROL)
  shell_flags.push_back (flags_alist ('b', &asynchronous_notification));
#endif /* JOB_CONTROL */
  shell_flags.push_back (flags_alist ('e', &errexit_flag));
  shell_flags.push_back (flags_alist ('f', &disallow_filename_globbing));
  shell_flags.push_back (flags_alist ('h', &hashing_enabled));
  shell_flags.push_back (flags_alist ('i', &forced_interactive));
  shell_flags.push_back (flags_alist ('k', &place_keywords_in_env));
#if defined(JOB_CONTROL)
  shell_flags.push_back (flags_alist ('m', &jobs_m_flag));
#endif /* JOB_CONTROL */
  shell_flags.push_back (flags_alist ('n', &read_but_dont_execute));
  shell_flags.push_back (flags_alist ('p', &privileged_mode));
#if defined(RESTRICTED_SHELL)
  shell_flags.push_back (flags_alist ('r', &restricted));
#endif /* RESTRICTED_SHELL */
  shell_flags.push_back (flags_alist ('t', &just_one_command));
  shell_flags.push_back (flags_alist ('u', &unbound_vars_is_error));
  shell_flags.push_back (flags_alist ('v', &verbose_flag));
  shell_flags.push_back (flags_alist ('x', &echo_command_at_execute));

/* New flags that control non-standard things. */
#if defined(BRACE_EXPANSION)
  shell_flags.push_back (flags_alist ('B', &brace_expansion));
#endif
  shell_flags.push_back (flags_alist ('C', &noclobber));
  shell_flags.push_back (flags_alist ('E', &error_trace_mode));
#if defined(BANG_HISTORY)
  shell_flags.push_back (flags_alist ('H', &histexp_flag));
#endif /* BANG_HISTORY */
  shell_flags.push_back (flags_alist ('P', &no_symbolic_links));
  shell_flags.push_back (flags_alist ('T', &function_trace_mode));
}

char optflags[NUM_SHELL_FLAGS + 4] = { '+' };

char *
Shell::find_flag (int name)
{
  int i;
  for (i = 0; shell_flags[i].name; i++)
    {
      if (shell_flags[i].name == name)
        return shell_flags[i].value;
    }
  return NULL;
}

/* Change the state of a flag, and return its original value, or return
   FLAG_ERROR if there is no flag FLAG.  ON_OR_OFF must be either
   FLAG_ON or FLAG_OFF. */
char
Shell::change_flag (int flag, int on_or_off)
{
  char *value, old_value;

#if defined(RESTRICTED_SHELL)
  /* Don't allow "set +r" in a shell which is `restricted'. */
  if (restricted && flag == 'r' && on_or_off == FLAG_OFF)
    return FLAG_ERROR;
#endif /* RESTRICTED_SHELL */

  value = find_flag (flag);

  if ((value == NULL) || (on_or_off != FLAG_ON && on_or_off != FLAG_OFF))
    return FLAG_ERROR;

  old_value = *value;
  *value = (on_or_off == FLAG_ON) ? 1 : 0;

  /* Special cases for a few flags. */
  switch (flag)
    {
#if defined(BANG_HISTORY)
    case 'H':
      history_expansion = histexp_flag;
      if (on_or_off == FLAG_ON)
        bash_initialize_history ();
      break;
#endif

#if defined(JOB_CONTROL)
    case 'm':
      set_job_control (on_or_off == FLAG_ON);
      break;
#endif /* JOB_CONTROL */

    case 'e':
      if (builtin_ignoring_errexit == 0)
        exit_immediately_on_error = errexit_flag;
      break;

    case 'n':
      if (interactive_shell)
        read_but_dont_execute = 0;
      break;

    case 'p':
      if (on_or_off == FLAG_OFF)
        disable_priv_mode ();
      break;

#if defined(RESTRICTED_SHELL)
    case 'r':
      if (on_or_off == FLAG_ON && shell_initialized)
        maybe_make_restricted (shell_name);
      break;
#endif

    case 'v':
      echo_input_at_read = verbose_flag;
      break;
    }

  return old_value;
}

/* Return a string which is the names of all the currently
   set shell flags. */
char *
which_set_flags ()
{
  char *temp;
  int i, string_index;

  temp = (char *)xmalloc (1 + NUM_SHELL_FLAGS + read_from_stdin
                          + want_pending_command);
  for (i = string_index = 0; shell_flags[i].name; i++)
    if (*(shell_flags[i].value))
      temp[string_index++] = shell_flags[i].name;

  if (want_pending_command)
    temp[string_index++] = 'c';
  if (read_from_stdin)
    temp[string_index++] = 's';

  temp[string_index] = '\0';
  return temp;
}

char *
get_current_flags ()
{
  char *temp;
  int i;

  temp = (char *)xmalloc (1 + NUM_SHELL_FLAGS);
  for (i = 0; shell_flags[i].name; i++)
    temp[i] = *(shell_flags[i].value);
  temp[i] = '\0';
  return temp;
}

void
set_current_flags (const char *bitmap)
{
  int i;

  if (bitmap == 0)
    return;
  for (i = 0; shell_flags[i].name; i++)
    *(shell_flags[i].value) = bitmap[i];
}

void
Shell::reset_shell_flags ()
{
  mark_modified_vars = disallow_filename_globbing = false;
  place_keywords_in_env = read_but_dont_execute = just_one_command = false;
  noclobber = unbound_vars_is_error = false;
  echo_command_at_execute = jobs_m_flag = forced_interactive = false;
  no_symbolic_links = false;
  privileged_mode = pipefail_opt = false;

  error_trace_mode = function_trace_mode = false;

  exit_immediately_on_error = errexit_flag = false;
  echo_input_at_read = verbose_flag = false;

  hashing_enabled = interactive_comments = true;

#if defined(JOB_CONTROL)
  asynchronous_notification = false;
#endif

#if defined(BANG_HISTORY)
  histexp_flag = false;
#endif

#if defined(BRACE_EXPANSION)
  brace_expansion = true;
#endif

#if defined(RESTRICTED_SHELL)
  restricted = false;
#endif
}

void
Shell::initialize_flags ()
{
  int i;

  for (i = 0; shell_flags[i].name; i++)
    optflags[i + 1] = shell_flags[i].name;
  optflags[++i] = 'o';
  optflags[++i] = ';';
  optflags[i + 1] = '\0';
}

} // namespace bash
