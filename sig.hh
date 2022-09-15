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

/* Here is a definition for set_signal_handler () which simply expands to
   a call to signal () for non-Posix systems.  The code for set_signal_handler
   in the Posix case resides in general.c. */
#if !defined(HAVE_POSIX_SIGNALS)
#define set_signal_handler(sig, handler) signal (sig, handler)
#else
SigHandler set_signal_handler (int, SigHandler); /* in sig.cc */
#endif /* _POSIX_VERSION */

#if !defined(SIGCHLD) && defined(SIGCLD)
#define SIGCHLD SIGCLD
#endif

#if !defined(HAVE_POSIX_SIGNALS) && !defined(sigmask)
#define sigmask(x) (1 << ((x)-1))
#endif /* !HAVE_POSIX_SIGNALS && !sigmask */

#if !defined(HAVE_POSIX_SIGNALS)
#if !defined(SIG_BLOCK)
#define SIG_BLOCK 2
#define SIG_SETMASK 3
#endif /* SIG_BLOCK */

/* sigset_t defined in config.h */

/* Make sure there is nothing inside the signal set. */
#define sigemptyset(set) (*(set) = 0)

/* Initialize the signal set to hold all signals. */
#define sigfillset(set) (*set) = sigmask (NSIG) - 1

/* Add SIG to the contents of SET. */
#define sigaddset(set, sig) *(set) |= sigmask (sig)

/* Delete SIG from signal set SET. */
#define sigdelset(set, sig) *(set) &= ~sigmask (sig)

/* Is SIG a member of the signal set SET? */
#define sigismember(set, sig) ((*(set)&sigmask (sig)) != 0)

/* Suspend the process until the reception of one of the signals
   not present in SET. */
#define sigsuspend(set) sigpause (*(set))
#endif /* !HAVE_POSIX_SIGNALS */

/* These definitions are used both in POSIX and non-POSIX implementations. */

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

#if defined(HAVE_POSIX_SIGNALS)
#define BLOCK_CHILD(nvar, ovar) BLOCK_SIGNAL (SIGCHLD, nvar, ovar)
#define UNBLOCK_CHILD(ovar) UNBLOCK_SIGNAL (ovar)
#else /* !HAVE_POSIX_SIGNALS */
#define BLOCK_CHILD(nvar, ovar) ovar = sigblock (sigmask (SIGCHLD))
#define UNBLOCK_CHILD(ovar) sigsetmask (ovar)
#endif /* !HAVE_POSIX_SIGNALS */

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
