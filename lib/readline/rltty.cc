/* rltty.c -- functions to prepare and restore the terminal for readline's
   use. */

/* Copyright (C) 1992-2017 Free Software Foundation, Inc.

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

#include "readline.hh"
#include "rlprivate.hh"

#include <sys/types.h>

#include "rltty.hh"

#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h> /* include for declaration of ioctl */
#endif

namespace readline
{

/* **************************************************************** */
/*								    */
/*		      Saving and Restoring the TTY	    	    */
/*								    */
/* **************************************************************** */

/* Non-zero means that the terminal is in a prepped state.  There are several
   flags that are OR'd in to denote whether or not we have sent various
   init strings to the terminal. */
#define TPX_PREPPED 0x01
#define TPX_BRACKPASTE 0x02
// #define TPX_METAKEY	0x04

/* Dummy call to force a backgrounded readline to stop before it tries
   to get the tty settings. */
static inline void
set_winsize (int tty)
{
#if defined(TIOCGWINSZ)
  struct winsize w;

  if (ioctl (tty, TIOCGWINSZ, &w) == 0)
    (void)ioctl (tty, TIOCSWINSZ, &w);
#endif /* TIOCGWINSZ */
}

#if defined(NO_TTY_DRIVER)
/* Nothing */
#else
#if !defined(VMIN)
#define VMIN VEOF
#endif

#if !defined(VTIME)
#define VTIME VEOL
#endif

#if defined(TERMIOS_TTY_DRIVER)
#define GETATTR(tty, tiop) (tcgetattr (tty, tiop))
#ifdef M_UNIX
#define SETATTR(tty, tiop) (tcsetattr (tty, TCSANOW, tiop))
#else
#define SETATTR(tty, tiop) (tcsetattr (tty, TCSADRAIN, tiop))
#endif /* !M_UNIX */
#else
#define GETATTR(tty, tiop) (ioctl (tty, TCGETA, tiop))
#define SETATTR(tty, tiop) (ioctl (tty, TCSETAW, tiop))
#endif /* !TERMIOS_TTY_DRIVER */

#if 0
static void save_tty_chars (TIOTYPE *);
static int _get_tty_settings (int, TIOTYPE *);
static int get_tty_settings (int, TIOTYPE *);
static int _set_tty_settings (int, TIOTYPE *);
static int set_tty_settings (int, TIOTYPE *);

static void prepare_terminal_settings (int, TIOTYPE, TIOTYPE *);

static void set_special_char (Keymap, TIOTYPE *, int, rl_command_func_t);
static void _rl_bind_tty_special_chars (Keymap, TIOTYPE);
#endif

#if defined(FLUSHO)
#define OUTPUT_BEING_FLUSHED(tp) (tp->c_lflag & FLUSHO)
#else
#define OUTPUT_BEING_FLUSHED(tp) 0
#endif

void
Readline::save_tty_chars (TIOTYPE *tiop)
{
  _rl_last_tty_chars = _rl_tty_chars;

  _rl_tty_chars.t_eof = tiop->c_cc[VEOF];
  _rl_tty_chars.t_eol = tiop->c_cc[VEOL];
#ifdef VEOL2
  _rl_tty_chars.t_eol2 = tiop->c_cc[VEOL2];
#endif
  _rl_tty_chars.t_erase = tiop->c_cc[VERASE];
#ifdef VWERASE
  _rl_tty_chars.t_werase = tiop->c_cc[VWERASE];
#endif
  _rl_tty_chars.t_kill = tiop->c_cc[VKILL];
#ifdef VREPRINT
  _rl_tty_chars.t_reprint = tiop->c_cc[VREPRINT];
#endif
  _rl_intr_char = _rl_tty_chars.t_intr = tiop->c_cc[VINTR];
  _rl_quit_char = _rl_tty_chars.t_quit = tiop->c_cc[VQUIT];
#ifdef VSUSP
  _rl_susp_char = _rl_tty_chars.t_susp = tiop->c_cc[VSUSP];
#endif
#ifdef VDSUSP
  _rl_tty_chars.t_dsusp = tiop->c_cc[VDSUSP];
#endif
#ifdef VSTART
  _rl_tty_chars.t_start = tiop->c_cc[VSTART];
#endif
#ifdef VSTOP
  _rl_tty_chars.t_stop = tiop->c_cc[VSTOP];
#endif
#ifdef VLNEXT
  _rl_tty_chars.t_lnext = tiop->c_cc[VLNEXT];
#endif
#ifdef VDISCARD
  _rl_tty_chars.t_flush = tiop->c_cc[VDISCARD];
#endif
#ifdef VSTATUS
  _rl_tty_chars.t_status = tiop->c_cc[VSTATUS];
#endif
}

int
Readline::_get_tty_settings (int tty, TIOTYPE *tiop)
{
  int ioctl_ret;

  while (1)
    {
      ioctl_ret = GETATTR (tty, tiop);
      if (ioctl_ret < 0)
        {
          if (errno != EINTR)
            return -1;
          else
            continue;
        }
      if (OUTPUT_BEING_FLUSHED (tiop))
        {
#if defined(FLUSHO)
          _rl_errmsg ("warning: turning off output flushing");
          tiop->c_lflag &= static_cast<tcflag_t> (~FLUSHO);
          break;
#else
          continue;
#endif
        }
      break;
    }

  return 0;
}

int
Readline::get_tty_settings (int tty, TIOTYPE *tiop)
{
  set_winsize (tty);

  errno = 0;
  if (_get_tty_settings (tty, tiop) < 0)
    return -1;

#if defined(_AIX)
  setopost (tiop);
#endif

  return 0;
}

static int
_set_tty_settings (int tty, TIOTYPE *tiop)
{
  while (SETATTR (tty, tiop) < 0)
    {
      if (errno != EINTR)
        return -1;
      errno = 0;
    }
  return 0;
}

static int
set_tty_settings (int tty, TIOTYPE *tiop)
{
  if (_set_tty_settings (tty, tiop) < 0)
    return -1;

  return 0;
}

void
Readline::prepare_terminal_settings (int meta_flag, TIOTYPE oldtio,
                                     TIOTYPE *tiop)
{
  int sc;
  Keymap kmap;

  _rl_echoing_p = (oldtio.c_lflag & ECHO);
#if defined(ECHOCTL)
  _rl_echoctl = (oldtio.c_lflag & ECHOCTL);
#endif

  tiop->c_lflag &= static_cast<tcflag_t> (~(ICANON | ECHO));

  if (static_cast<unsigned char> (oldtio.c_cc[VEOF])
      != static_cast<unsigned char> (_POSIX_VDISABLE))
    _rl_eof_char = oldtio.c_cc[VEOF];

#if defined(USE_XON_XOFF)
#if defined(IXANY)
  tiop->c_iflag &= ~(IXON | IXANY);
#else
  /* `strict' Posix systems do not define IXANY. */
  tiop->c_iflag &= ~IXON;
#endif /* IXANY */
#endif /* USE_XON_XOFF */

  /* Only turn this off if we are using all 8 bits. */
  if (((tiop->c_cflag & CSIZE) == CS8) || meta_flag)
    tiop->c_iflag &= static_cast<tcflag_t> (~(ISTRIP | INPCK));

  /* Make sure we differentiate between CR and NL on input. */
  tiop->c_iflag &= static_cast<tcflag_t> (~(ICRNL | INLCR));

#if !defined(HANDLE_SIGNALS)
  tiop->c_lflag &= ~ISIG;
#else
  tiop->c_lflag |= ISIG;
#endif

  tiop->c_cc[VMIN] = 1;
  tiop->c_cc[VTIME] = 0;

#if defined(FLUSHO)
  if (OUTPUT_BEING_FLUSHED (tiop))
    {
      tiop->c_lflag &= static_cast<tcflag_t> (~FLUSHO);
      oldtio.c_lflag &= static_cast<tcflag_t> (~FLUSHO);
    }
#endif

    /* Turn off characters that we need on Posix systems with job control,
       just to be sure.  This includes ^Y and ^V.  This should not really
       be necessary.  */
#if defined(TERMIOS_TTY_DRIVER) && defined(_POSIX_VDISABLE)

#if defined(VLNEXT)
  tiop->c_cc[VLNEXT] = _POSIX_VDISABLE;
#endif

#if defined(VDSUSP)
  tiop->c_cc[VDSUSP] = _POSIX_VDISABLE;
#endif

  /* Conditionally disable some other tty special characters if there is a
     key binding for them in the current keymap.  Readline ordinarily doesn't
     bind these characters, but an application or user might. */
#if defined(VI_MODE)
  kmap = (rl_editing_mode == vi_mode) ? vi_insertion_keymap () : _rl_keymap;
#else
  kmap = _rl_keymap;
#endif
#if defined(VDISCARD)
  sc = tiop->c_cc[VDISCARD];
  if (sc != _POSIX_VDISABLE
      && kmap[static_cast<unsigned char> (sc)].type == ISFUNC)
    tiop->c_cc[VDISCARD] = _POSIX_VDISABLE;
#endif /* VDISCARD */

#endif /* TERMIOS_TTY_DRIVER && _POSIX_VDISABLE */
}
#endif /* !NEW_TTY_DRIVER */

/* Put the terminal in CBREAK mode so that we can detect key presses. */
#if defined(NO_TTY_DRIVER)
void
Readline::rl_prep_terminal (int meta_flag)
{
  _rl_echoing_p = true;
}

void
Readline::rl_deprep_terminal ()
{
}

#else /* ! NO_TTY_DRIVER */
void
Readline::rl_prep_terminal (int meta_flag)
{
  int tty, nprep;
  TIOTYPE tio;

  if (terminal_prepped)
    return;

  /* Try to keep this function from being INTerrupted. */
  _rl_block_sigint ();

  tty = rl_instream ? fileno (rl_instream) : fileno (stdin);

  if (get_tty_settings (tty, &tio) < 0)
    {
#if defined(ENOTSUP)
      /* MacOS X and Linux, at least, lie about the value of errno if
         tcgetattr fails. */
      if (errno == ENOTTY || errno == EINVAL || errno == ENOTSUP)
#else
      if (errno == ENOTTY || errno == EINVAL)
#endif
        _rl_echoing_p = true; /* XXX */

      _rl_release_sigint ();
      return;
    }

  otio = tio;

  if (_rl_bind_stty_chars)
    {
#if defined(VI_MODE)
      /* If editing in vi mode, make sure we restore the bindings in the
         insertion keymap no matter what keymap we ended up in. */
      if (rl_editing_mode == vi_mode)
        rl_tty_unset_default_bindings (vi_insertion_keymap ());
      else
#endif
        rl_tty_unset_default_bindings (_rl_keymap);
    }
  save_tty_chars (&otio);
  RL_SETSTATE (RL_STATE_TTYCSAVED);
  if (_rl_bind_stty_chars)
    {
#if defined(VI_MODE)
      /* If editing in vi mode, make sure we set the bindings in the
         insertion keymap no matter what keymap we ended up in. */
      if (rl_editing_mode == vi_mode)
        _rl_bind_tty_special_chars (vi_insertion_keymap (), tio);
      else
#endif
        _rl_bind_tty_special_chars (_rl_keymap, tio);
    }

  prepare_terminal_settings (meta_flag, otio, &tio);

  if (set_tty_settings (tty, &tio) < 0)
    {
      _rl_release_sigint ();
      return;
    }

  if (_rl_enable_keypad)
    _rl_control_keypad (1);

  nprep = TPX_PREPPED;

  if (_rl_enable_bracketed_paste)
    {
      fprintf (rl_outstream, BRACK_PASTE_INIT);
      nprep |= TPX_BRACKPASTE;
    }

  fflush (rl_outstream);
  terminal_prepped = nprep;
  RL_SETSTATE (RL_STATE_TERMPREPPED);

  _rl_release_sigint ();
}

/* Restore the terminal's normal settings and modes. */
void
Readline::rl_deprep_terminal ()
{
  int tty;

  if (terminal_prepped == 0)
    return;

  /* Try to keep this function from being interrupted. */
  _rl_block_sigint ();

  tty = rl_instream ? fileno (rl_instream) : fileno (stdin);

  if (terminal_prepped & TPX_BRACKPASTE)
    {
      fprintf (rl_outstream, BRACK_PASTE_FINI);
      if (_rl_eof_found)
        fprintf (rl_outstream, "\n");
    }

  if (_rl_enable_keypad)
    _rl_control_keypad (0);

  fflush (rl_outstream);

  if (set_tty_settings (tty, &otio) < 0)
    {
      _rl_release_sigint ();
      return;
    }

  terminal_prepped = 0;
  RL_UNSETSTATE (RL_STATE_TERMPREPPED);

  _rl_release_sigint ();
}
#endif /* !NO_TTY_DRIVER */

/* Set readline's idea of whether or not it is echoing output to the terminal,
   returning the old value. */
int
Readline::rl_tty_set_echoing (int u)
{
  int o;

  o = _rl_echoing_p;
  _rl_echoing_p = u;
  return o;
}

/* **************************************************************** */
/*								    */
/*			Bogus Flow Control      		    */
/*								    */
/* **************************************************************** */

int
Readline::rl_restart_output (int, int)
{
#if defined(__MINGW32__)
  return 0;
#else /* !__MING32__ */

  int fildes = fileno (rl_outstream);
#if defined(TIOCSTART)
  ioctl (fildes, TIOCSTART, 0);

#else /* !TIOCSTART */
#if defined(TERMIOS_TTY_DRIVER)
  tcflow (fildes, TCOON); /* Simulate a ^Q. */
#else /* !TERMIOS_TTY_DRIVER */
#if defined(TCXONC)
  ioctl (fildes, TCXONC, TCOON);
#endif /* TCXONC */
#endif /* !TERMIOS_TTY_DRIVER */
#endif /* !TIOCSTART */

  return 0;
#endif /* !__MINGW32__ */
}

int
Readline::rl_stop_output (int, int)
{
#if defined(__MINGW32__)
  return 0;
#else

  int fildes = fileno (rl_instream);

#if defined(TIOCSTOP)
  ioctl (fildes, TIOCSTOP, 0);
#else /* !TIOCSTOP */
#if defined(TERMIOS_TTY_DRIVER)
  tcflow (fildes, TCOOFF);
#else
#if defined(TCXONC)
  ioctl (fildes, TCXONC, TCOON);
#endif /* TCXONC */
#endif /* !TERMIOS_TTY_DRIVER */
#endif /* !TIOCSTOP */

  return 0;
#endif /* !__MINGW32__ */
}

/* **************************************************************** */
/*								    */
/*			Default Key Bindings			    */
/*								    */
/* **************************************************************** */

#if !defined(NO_TTY_DRIVER)
#define SET_SPECIAL(sc, func) set_special_char (kmap, &ttybuff, sc, func)
#endif

#if defined(NO_TTY_DRIVER)

#define SET_SPECIAL(sc, func)
#define RESET_SPECIAL(c)

#else /* !NO_TTY_DRIVER */
static void
set_special_char (Readline::Keymap kmap, TIOTYPE *tiop, int sc,
                  Readline::rl_command_func_t func)
{
  unsigned char uc;

  uc = tiop->c_cc[sc];
  if (uc != static_cast<unsigned char> (_POSIX_VDISABLE)
      && kmap[uc].type == ISFUNC)
    kmap[uc].value.function = func;
}

/* used later */
static inline void
RESET_SPECIAL (Readline::Keymap kmap, unsigned char uc)
{
  if (uc != static_cast<unsigned char> (_POSIX_VDISABLE)
      && kmap[uc].type == ISFUNC)
    kmap[uc].value.function = &Readline::rl_insert;
}

void
Readline::_rl_bind_tty_special_chars (Keymap kmap, TIOTYPE ttybuff)
{
  SET_SPECIAL (VERASE, &Readline::rl_rubout);
  SET_SPECIAL (VKILL, &Readline::rl_unix_line_discard);

#if defined(VLNEXT) && defined(TERMIOS_TTY_DRIVER)
  SET_SPECIAL (VLNEXT, &Readline::rl_quoted_insert);
#endif /* VLNEXT && TERMIOS_TTY_DRIVER */

#if defined(VWERASE) && defined(TERMIOS_TTY_DRIVER)
#if defined(VI_MODE)
  if (rl_editing_mode == vi_mode)
    SET_SPECIAL (VWERASE, &Readline::rl_vi_unix_word_rubout);
  else
#endif
    SET_SPECIAL (VWERASE, &Readline::rl_unix_word_rubout);
#endif /* VWERASE && TERMIOS_TTY_DRIVER */
}

#endif /* !NEW_TTY_DRIVER */

/* New public way to set the system default editing chars to their readline
   equivalents. */
void
Readline::rl_tty_set_default_bindings (Keymap kmap)
{
#if !defined(NO_TTY_DRIVER)
  TIOTYPE ttybuff;
  int tty;

  tty = fileno (rl_instream);

  if (get_tty_settings (tty, &ttybuff) == 0)
    _rl_bind_tty_special_chars (kmap, ttybuff);
#endif
}

/* Rebind all of the tty special chars that readline worries about back
   to self-insert.  Call this before saving the current terminal special
   chars with save_tty_chars().  This only works on POSIX termios or termio
   systems. */
void
Readline::rl_tty_unset_default_bindings (Keymap kmap)
{
  /* Don't bother before we've saved the tty special chars at least once. */
  if (RL_ISSTATE (RL_STATE_TTYCSAVED) == 0)
    return;

  RESET_SPECIAL (kmap, _rl_tty_chars.t_erase);
  RESET_SPECIAL (kmap, _rl_tty_chars.t_kill);

#if defined(VLNEXT) && defined(TERMIOS_TTY_DRIVER)
  RESET_SPECIAL (kmap, _rl_tty_chars.t_lnext);
#endif /* VLNEXT && TERMIOS_TTY_DRIVER */

#if defined(VWERASE) && defined(TERMIOS_TTY_DRIVER)
  RESET_SPECIAL (kmap, _rl_tty_chars.t_werase);
#endif /* VWERASE && TERMIOS_TTY_DRIVER */
}

#if defined(HANDLE_SIGNALS)

#if defined(NO_TTY_DRIVER)
int
Readline::_rl_disable_tty_signals ()
{
  return 0;
}

int
Readline::_rl_restore_tty_signals ()
{
  return 0;
}
#else

static TIOTYPE sigstty, nosigstty;
static bool tty_sigs_disabled = false;

int
Readline::_rl_disable_tty_signals ()
{
  if (tty_sigs_disabled)
    return 0;

  if (_get_tty_settings (fileno (rl_instream), &sigstty) < 0)
    return -1;

  nosigstty = sigstty;

  nosigstty.c_lflag &= static_cast<tcflag_t> (~ISIG);
  nosigstty.c_iflag &= static_cast<tcflag_t> (~IXON);

  if (_set_tty_settings (fileno (rl_instream), &nosigstty) < 0)
    return _set_tty_settings (fileno (rl_instream), &sigstty);

  tty_sigs_disabled = true;
  return 0;
}

int
Readline::_rl_restore_tty_signals ()
{
  int r;

  if (!tty_sigs_disabled)
    return 0;

  r = _set_tty_settings (fileno (rl_instream), &sigstty);

  if (r == 0)
    tty_sigs_disabled = false;

  return r;
}
#endif /* !NEW_TTY_DRIVER */

#endif /* HANDLE_SIGNALS */

} // namespace readline
