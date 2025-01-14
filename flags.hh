/* flags.h -- a list of all the flags that the shell knows about.  You add
   a flag to this program by adding the name here, and in flags.c. */

/* Copyright (C) 1993-2020 Free Software Foundation, Inc.

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

#if !defined(_FLAGS_H_)
#define _FLAGS_H_

/* Welcome to the world of Un*x, where everything is slightly backwards. */
const char FLAG_ON = '-';
const char FLAG_OFF = '+';

const char FLAG_ERROR = -1;
const char FLAG_UNKNOWN = 0;

/* The thing that we build the array of flags out of. */
struct flags_alist
{
  flags_alist (char name_, char *value_) : name (name_), value (value_) {}
  char name;
  char *value;
};

#if 0
extern const struct flags_alist shell_flags[];
extern char optflags[];

extern char
  mark_modified_vars, errexit_flag, exit_immediately_on_error,
  disallow_filename_globbing,
  place_keywords_in_env, read_but_dont_execute,
  just_one_command, unbound_vars_is_error, echo_input_at_read, verbose_flag,
  echo_command_at_execute, noclobber,
  hashing_enabled, forced_interactive, privileged_mode, jobs_m_flag,
  asynchronous_notification, interactive_comments, no_symbolic_links,
  function_trace_mode, error_trace_mode, pipefail_opt;

/* -c, -s invocation options -- not really flags, but they show up in $- */
extern bool want_pending_command, read_from_stdin;

#if 0
extern char lexical_scoping;
#endif

#if defined(BRACE_EXPANSION)
extern char brace_expansion;
#endif

#if defined(BANG_HISTORY)
extern char history_expansion;
extern char histexp_flag;
#endif /* BANG_HISTORY */

#if defined(RESTRICTED_SHELL)
extern char restricted;
extern char restricted_shell;
#endif /* RESTRICTED_SHELL */

extern char *find_flag (int);
extern char change_flag (int, int);
extern char *which_set_flags ();
extern void reset_shell_flags ();

extern char *get_current_flags ();
extern void set_current_flags (const char *);

extern void initialize_flags ();
#endif

/* A macro for efficiency. */
#define change_flag_char(flag, on_or_off) change_flag (flag, on_or_off)

#endif /* _FLAGS_H_ */
