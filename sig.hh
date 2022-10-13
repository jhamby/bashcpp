/* sig.h -- header file for signal handler definitions. */

/* Copyright (C) 1994-2013 Free Software Foundation, Inc.

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

/* Make sure that this is included *after* config.h! */

#if !defined(_SIG_H_)
#define _SIG_H_

#include <csignal> /* for sig_atomic_t */

#if !defined(SIGABRT) && defined(SIGIOT)
#define SIGABRT SIGIOT
#endif

namespace bash
{

extern "C"
{
  typedef void (*SigHandler) (int sig);
}

SigHandler set_signal_handler (int, SigHandler); /* in sig.cc */

#if !defined(SIGCHLD) && defined(SIGCLD)
#define SIGCHLD SIGCLD
#endif

static inline void
BLOCK_SIGNAL (int signum, sigset_t &nvar, sigset_t &ovar)
{
  sigemptyset (&nvar);
  sigaddset (&nvar, signum);
  sigemptyset (&ovar);
  sigprocmask (SIG_BLOCK, &nvar, &ovar);
}

static inline void
UNBLOCK_SIGNAL (sigset_t &ovar)
{
  sigprocmask (SIG_SETMASK, &ovar, nullptr);
}

#define BLOCK_CHILD(nvar, ovar) BLOCK_SIGNAL (SIGCHLD, nvar, ovar)
#define UNBLOCK_CHILD(ovar) UNBLOCK_SIGNAL (ovar)

// Declare these globals extern "C", for maximum ABI compatibility.
extern "C"
{
  /* Signal handlers from sig.c. */
  void termsig_sighandler_global (int);
  void sigint_sighandler_global (int);
  void sigwinch_sighandler_global (int);
  void sigterm_sighandler_global (int);

  /* Signal handler from trap.c. */
  void trap_handler_global (int);

  /* Extern variables */
  extern volatile sig_atomic_t sigwinch_received;
  extern volatile sig_atomic_t sigterm_received;

  // bool terminate_immediately;
}

} // namespace bash

#endif /* _SIG_H_ */
