/* signals.c -- signal handling support for readline. */

/* Copyright (C) 1987-2017 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library (Readline), a library
   for reading lines of text with interactive input and history editing.

   Readline is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Readline is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "history.hh"
#include "readline.hh"
#include "rlprivate.hh"

#include <sys/types.h>

#if defined(GWINSZ_IN_SYS_IOCTL)
#include <sys/ioctl.h>
#endif /* GWINSZ_IN_SYS_IOCTL */

#if defined(HANDLE_SIGNALS)

namespace readline
{

// Signal handler callbacks should all have C linkage.
extern "C"
{

  // Main signal handler callback.
  void rl_signal_handler (int);

#if defined(SIGWINCH)
  // SIGWINCH signal handler callback.
  void rl_sigwinch_handler (int sig);
#endif

} // extern "C"

/* Global atomic signal caught flag. */
volatile sig_atomic_t _rl_caught_signal = 0;

// Static helper function to set a signal handler.
static void rl_maybe_set_sighandler (int, SigHandler, sighandler_cxt *);

// Static helper function to restore a signal handler.
static void rl_maybe_restore_sighandler (int, sighandler_cxt *);

/* **************************************************************** */
/*					        		    */
/*			   Signal Handling                          */
/*								    */
/* **************************************************************** */

/* Readline signal handler functions. */

// Extern "C" signal handler callback.
void
rl_signal_handler (int sig)
{
  _rl_caught_signal = sig;
}

/* Called from RL_CHECK_SIGNALS() macro to run signal handling code. */
void
Readline::_rl_signal_handler (int sig)
{
  _rl_caught_signal = 0; /* XXX */

#if defined(SIGWINCH)
  if (sig == SIGWINCH)
    {
      RL_SETSTATE (RL_STATE_SIGHANDLER);

      rl_resize_terminal ();
      /* XXX - experimental for now */
      /* Call a signal hook because though we called the original signal
         handler in rl_sigwinch_handler below, we will not resend the signal to
         ourselves. */
      if (rl_signal_event_hook)
        ((*this).*rl_signal_event_hook) ();

      RL_UNSETSTATE (RL_STATE_SIGHANDLER);
      return;
    }
#endif

  bool block_sig;

  sigset_t set, oset;

  RL_SETSTATE (RL_STATE_SIGHANDLER);

  /* If there's a sig cleanup function registered, call it and `deregister'
     the cleanup function to avoid multiple calls */
  if (_rl_sigcleanup)
    {
      ((*this).*_rl_sigcleanup) (sig, _rl_sigcleanarg);
      _rl_sigcleanup = nullptr;
      _rl_sigcleanarg = nullptr;
    }

  /* Get the current set of blocked signals. If we want to block a signal for
     the duration of the cleanup functions, make sure to add it to SET and
     set block_sig = 1 (see the SIGHUP case below). */
  block_sig = false; /* sentinel to block signals with sigprocmask */
  sigemptyset (&set);
  sigprocmask (SIG_BLOCK, nullptr, &set);

  switch (sig)
    {
    case SIGINT:
      _rl_reset_completion_state ();
      rl_free_line_state ();
#if defined(READLINE_CALLBACKS)
      rl_callback_sigcleanup ();
#endif

      /* FALLTHROUGH */
      __attribute__ ((fallthrough));

#if defined(SIGTSTP)
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
      /* Block SIGTTOU so we can restore the terminal settings to something
         sane without stopping on SIGTTOU if we have been placed into the
         background.  Even trying to get the current terminal pgrp with
         tcgetpgrp() will generate SIGTTOU, so we don't bother.  We still do
         this even if we've been stopped on SIGTTOU, since we handle signals
         when we have returned from the signal handler and the signal is no
         longer blocked. */
      sigaddset (&set, SIGTTOU);
      block_sig = true;
#endif /* SIGTSTP */

      /* FALLTHROUGH */
      __attribute__ ((fallthrough));

      /* Any signals that should be blocked during cleanup should go here. */
#if defined(SIGHUP)
    case SIGHUP:
#if defined(_AIX)
      if (!block_sig)
        {
          sigaddset (&set, sig);
          block_sig = true;
        }
#endif // _AIX
#endif

      /* FALLTHROUGH */
      __attribute__ ((fallthrough));

    /* Signals that don't require blocking during cleanup should go here. */
    case SIGTERM:
#if defined(SIGALRM)
    case SIGALRM:
#endif
#if defined(SIGQUIT)
    case SIGQUIT:
#endif

      if (block_sig)
        sigprocmask (SIG_BLOCK, &set, &oset);

      rl_echo_signal_char (sig);
      rl_cleanup_after_signal ();

      /* At this point, the application's signal handler, if any, is the
         current handler. */

      /* Unblock any signal(s) blocked above */
      if (block_sig)
        sigprocmask (SIG_UNBLOCK, &oset, nullptr);

        /* We don't have to bother unblocking the signal because we are not
           running in a signal handler context. */

#if defined(__EMX__)
      signal (sig, SIG_ACK);
#endif

#if defined(HAVE_KILL)
      kill (getpid (), sig);
#else
      raise (sig); /* assume we have raise */
#endif

      /* We don't need to modify the signal mask now that this is not run in
         a signal handler context. */
      rl_reset_after_signal ();
    }

  RL_UNSETSTATE (RL_STATE_SIGHANDLER);
}

#if defined(SIGWINCH)

// Extern "C" signal handler callback.
void
rl_sigwinch_handler (int sig)
{
  Readline::the_app->_rl_sigwinch_handler_internal (sig);
}

#if defined(__clang__)
// On Linux, 'sa_handler' is a recursive macro expansion.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif

void
Readline::_rl_sigwinch_handler_internal (int sig)
{
  SigHandler oh;

#if defined(MUST_REINSTALL_SIGHANDLERS)
  sighandler_cxt dummy_winch;

  /* We don't want to change old_winch -- it holds the state of SIGWINCH
     disposition set by the calling application.  We need this state
     because we call the application's SIGWINCH handler after updating
     our own idea of the screen size. */
  rl_set_sighandler (SIGWINCH, rl_sigwinch_handler, &dummy_winch);
#endif

  RL_SETSTATE (RL_STATE_SIGHANDLER);
  _rl_caught_signal = sig;

  /* If another sigwinch handler has been installed, call it. */
  oh = _rl_old_winch.sa_handler;
  if (oh && oh != SIG_IGN && oh != SIG_DFL)
    (*oh) (sig);

  RL_UNSETSTATE (RL_STATE_SIGHANDLER);
}
#endif /* SIGWINCH */

/* Functions to manage signal handling. */

/* Set up a readline-specific signal handler, saving the old signal
   information in OHANDLER.  Return the old signal handler, like
   signal(). */
static SigHandler
rl_set_sighandler (int sig, SigHandler handler, sighandler_cxt *ohandler)
{
  sighandler_cxt old_handler;
  struct sigaction act;

  act.sa_handler = handler;
#if defined(SIGWINCH)
  act.sa_flags = (sig == SIGWINCH) ? SA_RESTART : 0;
#else
  act.sa_flags = 0;
#endif /* SIGWINCH */
  sigemptyset (&act.sa_mask);
  sigemptyset (&ohandler->sa_mask);
  sigaction (sig, &act, &old_handler);

  /* If rl_set_signals is called twice in a row, don't set the old handler to
     rl_signal_handler, because that would cause infinite recursion. */
  if (handler != &rl_signal_handler
      || old_handler.sa_handler != &rl_signal_handler)
    memcpy (ohandler, &old_handler, sizeof (sighandler_cxt));

  return ohandler->sa_handler;
}

/* Set disposition of SIG to HANDLER, returning old state in OHANDLER.  Don't
   change disposition if OHANDLER indicates the signal was ignored. */
void
rl_maybe_set_sighandler (int sig, SigHandler handler, sighandler_cxt *ohandler)
{
  sighandler_cxt dummy;
  SigHandler oh;

  sigemptyset (&dummy.sa_mask);
  dummy.sa_flags = 0;
  oh = rl_set_sighandler (sig, handler, ohandler);
  if (oh == SIG_IGN)
    rl_sigaction (sig, ohandler, &dummy);
}

/* Set the disposition of SIG to HANDLER, if HANDLER->sa_handler indicates the
   signal was not being ignored.  MUST only be called for signals whose
   disposition was changed using rl_maybe_set_sighandler or for which the
   SIG_IGN check was performed inline (e.g., SIGALRM below). */
void
rl_maybe_restore_sighandler (int sig, sighandler_cxt *handler)
{
  sighandler_cxt dummy;

  sigemptyset (&dummy.sa_mask);
  dummy.sa_flags = 0;
  if (handler->sa_handler != SIG_IGN)
    rl_sigaction (sig, handler, &dummy);
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

int
Readline::rl_set_signals ()
{
  sighandler_cxt dummy;
  SigHandler oh;
  /* XXX move to class */
  static int sigmask_set = 0;
  static sigset_t bset;

  if (rl_catch_signals && sigmask_set == 0)
    {
      sigemptyset (&bset);

      sigaddset (&bset, SIGINT);
      sigaddset (&bset, SIGTERM);
#if defined(SIGHUP)
      sigaddset (&bset, SIGHUP);
#endif
#if defined(SIGQUIT)
      sigaddset (&bset, SIGQUIT);
#endif
#if defined(SIGALRM)
      sigaddset (&bset, SIGALRM);
#endif
#if defined(SIGTSTP)
      sigaddset (&bset, SIGTSTP);
#endif
#if defined(SIGTTIN)
      sigaddset (&bset, SIGTTIN);
#endif
#if defined(SIGTTOU)
      sigaddset (&bset, SIGTTOU);
#endif
      sigmask_set = 1;
    }

  if (rl_catch_signals && !signals_set_flag)
    {
      sigemptyset (&_rl_orig_sigset);
      sigprocmask (SIG_BLOCK, &bset, &_rl_orig_sigset);

      rl_maybe_set_sighandler (SIGINT, &rl_signal_handler, &old_int);
      rl_maybe_set_sighandler (SIGTERM, &rl_signal_handler, &old_term);
#if defined(SIGHUP)
      rl_maybe_set_sighandler (SIGHUP, &rl_signal_handler, &old_hup);
#endif
#if defined(SIGQUIT)
      rl_maybe_set_sighandler (SIGQUIT, &rl_signal_handler, &old_quit);
#endif

#if defined(SIGALRM)
      oh = rl_set_sighandler (SIGALRM, &rl_signal_handler, &old_alrm);
      if (oh == SIG_IGN)
        rl_sigaction (SIGALRM, &old_alrm, &dummy);
#if defined(SA_RESTART)
      /* If the application using readline has already installed a signal
         handler with SA_RESTART, SIGALRM will cause reads to be restarted
         automatically, so readline should just get out of the way.  Since
         we tested for SIG_IGN above, we can just test for SIG_DFL here. */
      if (oh != SIG_DFL && (old_alrm.sa_flags & SA_RESTART))
        rl_sigaction (SIGALRM, &old_alrm, &dummy);
#endif /* SA_RESTART */
#endif /* SIGALRM */

#if defined(SIGTSTP)
      rl_maybe_set_sighandler (SIGTSTP, rl_signal_handler, &old_tstp);
#endif /* SIGTSTP */

#if defined(SIGTTOU)
      rl_maybe_set_sighandler (SIGTTOU, rl_signal_handler, &old_ttou);
#endif /* SIGTTOU */

#if defined(SIGTTIN)
      rl_maybe_set_sighandler (SIGTTIN, rl_signal_handler, &old_ttin);
#endif /* SIGTTIN */

      signals_set_flag = true;

      sigprocmask (SIG_SETMASK, &_rl_orig_sigset, nullptr);
    }
  else if (rl_catch_signals == 0)
    {
      sigemptyset (&_rl_orig_sigset);
      sigprocmask (SIG_BLOCK, nullptr, &_rl_orig_sigset);
    }

#if defined(SIGWINCH)
  if (rl_catch_sigwinch && !sigwinch_set_flag)
    {
      rl_maybe_set_sighandler (SIGWINCH, rl_sigwinch_handler, &_rl_old_winch);
      sigwinch_set_flag = true;
    }
#endif /* SIGWINCH */

  return 0;
}

void
Readline::rl_clear_signals ()
{
  sighandler_cxt dummy;

  if (rl_catch_signals && signals_set_flag)
    {
      /* Since rl_maybe_set_sighandler doesn't override a SIG_IGN handler,
         we should in theory not have to restore a handler where
         old_xxx.sa_handler == SIG_IGN.  That's what
         rl_maybe_restore_sighandler does.  Fewer system calls should reduce
         readline's per-line overhead */
      rl_maybe_restore_sighandler (SIGINT, &old_int);
      rl_maybe_restore_sighandler (SIGTERM, &old_term);
#if defined(SIGHUP)
      rl_maybe_restore_sighandler (SIGHUP, &old_hup);
#endif
#if defined(SIGQUIT)
      rl_maybe_restore_sighandler (SIGQUIT, &old_quit);
#endif
#if defined(SIGALRM)
      rl_maybe_restore_sighandler (SIGALRM, &old_alrm);
#endif

#if defined(SIGTSTP)
      rl_maybe_restore_sighandler (SIGTSTP, &old_tstp);
#endif /* SIGTSTP */

#if defined(SIGTTOU)
      rl_maybe_restore_sighandler (SIGTTOU, &old_ttou);
#endif /* SIGTTOU */

#if defined(SIGTTIN)
      rl_maybe_restore_sighandler (SIGTTIN, &old_ttin);
#endif /* SIGTTIN */

      signals_set_flag = false;
    }

#if defined(SIGWINCH)
  if (rl_catch_sigwinch && sigwinch_set_flag)
    {
      sigemptyset (&dummy.sa_mask);
      rl_sigaction (SIGWINCH, &_rl_old_winch, &dummy);
      sigwinch_set_flag = false;
    }
#endif
}

#endif /* HANDLE_SIGNALS */

/* **************************************************************** */
/*								    */
/*			   SIGINT Management			    */
/*								    */
/* **************************************************************** */

/* Cause SIGWINCH to not be delivered until the corresponding call to
   release_sigwinch(). */
void
Readline::_rl_block_sigwinch ()
{
  if (sigwinch_blocked)
    return;

#if defined(SIGWINCH)
  sigemptyset (&sigwinch_set);
  sigemptyset (&sigwinch_oset);
  sigaddset (&sigwinch_set, SIGWINCH);
  sigprocmask (SIG_BLOCK, &sigwinch_set, &sigwinch_oset);
#endif /* SIGWINCH */

  sigwinch_blocked = true;
}

/* Allow SIGWINCH to be delivered. */
void
Readline::_rl_release_sigwinch ()
{
  if (!sigwinch_blocked)
    return;

#if defined(SIGWINCH)
  sigprocmask (SIG_SETMASK, &sigwinch_oset, nullptr);
#endif /* SIGWINCH */

  sigwinch_blocked = false;
}

/* **************************************************************** */
/*								    */
/*		Echoing special control characters		    */
/*								    */
/* **************************************************************** */
void
Readline::rl_echo_signal_char (int sig)
{
  char cstr[3];
  unsigned int cslen;
  int c;

  if (!_rl_echoctl || !_rl_echo_control_chars)
    return;

  switch (sig)
    {
    case SIGINT:
      c = _rl_intr_char;
      break;
#if defined(SIGQUIT)
    case SIGQUIT:
      c = _rl_quit_char;
      break;
#endif
#if defined(SIGTSTP)
    case SIGTSTP:
      c = _rl_susp_char;
      break;
#endif
    default:
      return;
    }

  if (CTRL_CHAR (c) || c == RUBOUT)
    {
      cstr[0] = '^';
      cstr[1] = CTRL_CHAR (c) ? UNCTRL (c) : '?';
      cstr[cslen = 2] = '\0';
    }
  else
    {
      cstr[0] = static_cast<char> (c);
      cstr[cslen = 1] = '\0';
    }

  _rl_output_some_chars (cstr, cslen);
}

} // namespace readline
