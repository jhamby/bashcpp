/* trap.h -- data structures used in the trap mechanism. */

/* Copyright (C) 1993-2013 Free Software Foundation, Inc.

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

#if !defined(_TRAP_H_)
#define _TRAP_H_

#include "bashtypes.hh"

#include <csignal>

#if !defined(NSIG)
#define NSIG 64
#endif /* !NSIG */

#define NO_SIG -1
#define DEFAULT_SIG SIG_DFL
#define IGNORE_SIG SIG_IGN

/* Special shell trap names. */
#define DEBUG_TRAP NSIG
#define ERROR_TRAP NSIG + 1
#define RETURN_TRAP NSIG + 2
#define EXIT_TRAP 0

/* system signals plus special bash traps */
#define BASH_NSIG NSIG + 3

/* Flags values for decode_signal() */
enum decode_signal_flags
{
  DSIG_NOFLAGS = 0,
  DSIG_SIGPREFIX = 0x01, /* don't allow `SIG' PREFIX */
  DSIG_NOCASE = 0x02     /* case-insensitive comparison */
};

namespace bash
{

#if 0
extern char *trap_list[];

extern int trapped_signal_received;
extern int wait_signal_received;
extern int running_trap;
extern int trap_saved_exit_value;
extern bool suppress_debug_trap_verbose;

/* Externally-visible functions declared in trap.c. */
extern void initialize_traps ();

extern void run_pending_traps ();

extern void queue_sigchld_trap (int);
extern void maybe_set_sigchld_trap (void *);
extern void set_impossible_sigchld_trap ();
extern void set_sigchld_trap (char *);

extern void set_debug_trap (char *);
extern void set_error_trap (void *);
extern void set_return_trap (char *);

extern void maybe_set_debug_trap (void *);
extern void maybe_set_error_trap (void *);
extern void maybe_set_return_trap (void *);

extern void set_sigint_trap (char *);
extern void set_signal (int, char *);

extern void restore_default_signal (int);
extern void ignore_signal (int);
extern int run_exit_trap ();
extern void run_trap_cleanup (int);
extern int run_debug_trap ();
extern void run_error_trap ();
extern void run_return_trap ();

extern void free_trap_strings ();
extern void reset_signal_handlers ();
extern void restore_original_signals ();

extern void get_original_signal (int);
extern void get_all_original_signals ();

extern const char *signal_name (int);

extern int decode_signal (const char *, int);
extern void run_interrupt_trap (int);
extern int maybe_call_trap_handler (int);
extern int signal_is_special (int);
extern int signal_is_trapped (int);
extern int signal_is_pending (int);
extern int signal_is_ignored (int);
extern int signal_is_hard_ignored (int);
extern void set_signal_hard_ignored (int);
extern void set_signal_ignored (int);
extern int signal_in_progress (int);

extern void set_trap_state (int);

extern int next_pending_trap (int);
extern int first_pending_trap ();
extern void clear_pending_traps ();
extern int any_signals_trapped ();
extern void check_signals ();
extern void check_signals_and_traps ();

/* Inline functions. */

static bool
is_signal_object(const char *x, int f)
{
  return decode_signal (x, f) != NO_SIG;
}

static const char *
trap_string(int s)
{
  return (signal_is_trapped (s) && signal_is_ignored (s) == 0)
	  ? trap_list[s] : NULL;
}
#endif

} // namespace bash

#endif /* _TRAP_H_ */
