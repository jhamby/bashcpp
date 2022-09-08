/* input.c -- character input functions for readline. */

/* Copyright (C) 1994-2017 Free Software Foundation, Inc.

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
#include <fcntl.h>
#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#include "posixselect.hh"

#if defined (FIONREAD_IN_SYS_IOCTL)
#  include <sys/ioctl.h>
#endif

/* What kind of non-blocking I/O do we have? */
#if !defined (O_NDELAY) && defined (O_NONBLOCK)
#  define O_NDELAY O_NONBLOCK	/* Posix style */
#endif

namespace readline
{

#if 0
static int ibuffer_space ();
static int rl_get_char (int *);
static int rl_gather_tyi ();
#endif

/* Windows isatty returns true for every character device, including the null
   device, so we need to perform additional checks. */
#if defined (_WIN32) && !defined (__CYGWIN__)
#include <io.h>
#include <conio.h>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

static inline int
win32_isatty (int fd)
{
  if (_isatty(fd))
    {
      HANDLE h;
      DWORD ignored;

      if ((h = (HANDLE) _get_osfhandle (fd)) == INVALID_HANDLE_VALUE)
	{
	  errno = EBADF;
	  return 0;
	}
      if (GetConsoleMode (h, &ignored) != 0)
	return 1;
    }
  errno = ENOTTY;
  return 0;
}

#define isatty(x)	win32_isatty(x)
#endif

/* **************************************************************** */
/*								    */
/*			Character Input Buffering       	    */
/*								    */
/* **************************************************************** */

/* If a character is available to be read, then read it and stuff it into
   IBUFFER.  Otherwise, just return.  Returns number of characters read
   (0 if none available) and -1 on error (EIO). */
int
Readline::rl_gather_tyi ()
{
  int tty;
  int result;
  int chars_avail, k;
  char input;
#if defined(HAVE_SELECT)
  fd_set readfds, exceptfds;
  struct timeval timeout;
#endif

  chars_avail = 0;
  input = 0;
  tty = ::fileno (rl_instream);

#if defined (HAVE_SELECT)
  FD_ZERO (&readfds);
  FD_ZERO (&exceptfds);
  FD_SET (tty, &readfds);
  FD_SET (tty, &exceptfds);
  USEC_TO_TIMEVAL (_keyboard_input_timeout, timeout);
  result = ::select (tty + 1, &readfds, nullptr, &exceptfds, &timeout);
  if (result <= 0)
    return 0;	/* Nothing to read. */
#endif

  result = -1;
  errno = 0;
#if defined (FIONREAD)
  result = ::ioctl (tty, FIONREAD, &chars_avail);
  if (result == -1 && errno == EIO)
    return -1;
  if (result == -1)
    chars_avail = 0;
#endif

#if defined (O_NDELAY)
  if (result == -1)
    {
      int tem = ::fcntl (tty, F_GETFL, 0);

      ::fcntl (tty, F_SETFL, (tem | O_NDELAY));
      chars_avail = static_cast<int> (::read (tty, &input, 1));
      ::fcntl (tty, F_SETFL, tem);

      if (chars_avail == -1 && errno == EAGAIN)
	return 0;
      if (chars_avail == -1 && errno == EIO)
	return -1;
      if (chars_avail == 0)	/* EOF */
	{
	  rl_stuff_char (EOF);
	  return 0;
	}
    }
#endif /* O_NDELAY */

#if defined (__MINGW32__)
  /* Use getch/_kbhit to check for available console input, in the same way
     that we read it normally. */
   chars_avail = ::isatty (tty) ? ::_kbhit () : 0;
   result = 0;
#endif

  /* If there's nothing available, don't waste time trying to read
     something. */
  if (chars_avail <= 0)
    return 0;

  int tem = static_cast<int> (ibuffer_space ());

  if (chars_avail > tem)
    chars_avail = tem;

  /* One cannot read all of the available input.  I can only read a single
     character at a time, or else programs which require input can be
     thwarted.  If the buffer is larger than one character, I lose.
     Damn! */

  if (result != -1)
    {
      while (chars_avail--)
	{
	  RL_CHECK_SIGNALS ();
	  k = ((*this).*rl_getc_function) (rl_instream);
	  if (rl_stuff_char (k) == 0)
	    break;			/* some problem; no more room */
	  if (k == NEWLINE || k == RETURN)
	    break;
	}
    }
  else
    {
      if (chars_avail)
	rl_stuff_char (input);
    }

  return 1;
}

int
Readline::rl_set_keyboard_input_timeout (int u)
{
  int o = _keyboard_input_timeout;
  if (u >= 0)
    _keyboard_input_timeout = u;
  return o;
}

/* Is there input available to be read on the readline input file
   descriptor?  Only works if the system has select(2) or FIONREAD.
   Uses the value of _keyboard_input_timeout as the timeout; if another
   readline function wants to specify a timeout and not leave it up to
   the user, it should use _rl_input_queued(timeout_value_in_microseconds)
   instead. */
int
Readline::_rl_input_available ()
{
#if defined(HAVE_SELECT)
  fd_set readfds, exceptfds;
  struct timeval timeout;
#endif
#if !defined (HAVE_SELECT) && defined(FIONREAD)
  int chars_avail;
#endif
  int tty;

  if (rl_input_available_hook)
    return ((*this).*rl_input_available_hook) ();

  tty = ::fileno (rl_instream);

#if defined (HAVE_SELECT)
  FD_ZERO (&readfds);
  FD_ZERO (&exceptfds);
  FD_SET (tty, &readfds);
  FD_SET (tty, &exceptfds);
  USEC_TO_TIMEVAL (_keyboard_input_timeout, timeout);
  return ::select (tty + 1, &readfds, nullptr, &exceptfds, &timeout) > 0;
#elif defined (FIONREAD)
  if (::ioctl (tty, FIONREAD, &chars_avail) == 0)
    return chars_avail;
#elif defined (__MINGW32__)
  if (::isatty (tty))
    return ::_kbhit ();
#else
  return 0;
#endif
}

int
Readline::_rl_nchars_available ()
{
  int chars_avail, fd, result;

  chars_avail = 0;

#if defined (FIONREAD)
  fd = ::fileno (rl_instream);
  errno = 0;
  result = ::ioctl (fd, FIONREAD, &chars_avail);
  if (result == -1 && errno == EIO)
    return -1;
#endif

  return chars_avail;
}

void
Readline::_rl_insert_typein (int c)
{
  std::string string;
  string.push_back (static_cast<char> (c));

  int key = 0, t;

  while ((t = rl_get_char (&key)) &&
	 _rl_keymap[key].type == ISFUNC &&
	 _rl_keymap[key].value.function == &Readline::rl_insert)
    string.push_back (static_cast<char> (t));

  if (t)
    _rl_unget_char (key);

  rl_insert_text (string);
}

/* Add KEY to the buffer of characters to be read.  Returns true if the
   character was stuffed correctly; false otherwise. */
bool
Readline::rl_stuff_char (int key)
{
  if (ibuffer_space () == 0)
    return false;

  if (key == EOF)
    {
      key = NEWLINE;
      rl_pending_input = EOF;
      RL_SETSTATE (RL_STATE_INPUTPENDING);
    }

  _rl_ibuffer[_rl_push_index++] = static_cast<unsigned char> (key);

  if (_rl_push_index >= static_cast<int> (sizeof (_rl_ibuffer)))
    _rl_push_index = 0;

  return true;
}

/* **************************************************************** */
/*								    */
/*			     Character Input			    */
/*								    */
/* **************************************************************** */

/* Read a key, including pending input. */
int
Readline::rl_read_key ()
{
  int c, r;

  if (rl_pending_input)
    {
      c = rl_pending_input;	/* XXX - cast to unsigned char if > 0? */
      rl_clear_pending_input ();
    }
  else
    {
      /* If input is coming from a macro, then use that. */
      if ((c = _rl_next_macro_key ()))
	return static_cast<unsigned char> (c);

      /* If the user has an event function, then call it periodically. */
      if (rl_event_hook)
	{
	  while (rl_event_hook)
	    {
	      if (rl_get_char (&c) != 0)
		break;

	      if ((r = rl_gather_tyi ()) < 0)	/* XXX - EIO */
		{
		  rl_done = 1;
		  return errno == EIO ? (RL_ISSTATE (RL_STATE_READCMD) ? READERR : EOF) : '\n';
		}
	      else if (r > 0)			/* read something */
		continue;

	      RL_CHECK_SIGNALS ();
	      if (rl_done)		/* XXX - experimental */
		return '\n';
	      ((*this).*rl_event_hook) ();
	    }
	}
      else
	{
	  if (rl_get_char (&c) == 0)
	    c = ((*this).*rl_getc_function) (rl_instream);
/* fprintf(stderr, "rl_read_key: calling RL_CHECK_SIGNALS: _rl_caught_signal = %d\r\n", _rl_caught_signal); */
	  RL_CHECK_SIGNALS ();
	}
    }

  return c;
}

int
Readline::rl_getc (FILE *stream)
{
  int result;
#if defined (HAVE_PSELECT)
#  if !defined (HANDLE_SIGNALS)
  sigset_t empty_set;
#  endif
  fd_set readfds;
#endif

  while (1)
    {
      RL_CHECK_SIGNALS ();

      /* We know at this point that _rl_caught_signal == 0 */

#if defined (__MINGW32__)
      if (::isatty (::fileno (stream)))
	return ::_getch ();	/* "There is no error return." */
#endif
      result = 0;
#if defined (HAVE_PSELECT)
      FD_ZERO (&readfds);
      FD_SET (::fileno (stream), &readfds);
#  if defined (HANDLE_SIGNALS)
      result = ::pselect (::fileno (stream) + 1, &readfds, nullptr, nullptr,
			  nullptr, &_rl_orig_sigset);
#  else
      sigemptyset (&empty_set);
      sigprocmask (SIG_BLOCK, NULL, &empty_set);
      result = ::pselect (::fileno (stream) + 1, &readfds, nullptr, nullptr,
			  nullptr, &empty_set);
#  endif /* HANDLE_SIGNALS */
#endif
      unsigned char c = 0;
      if (result >= 0)
	result = static_cast<int> (::read (::fileno (stream), &c, sizeof (unsigned char)));

      if (result == 1)
	return c;

      /* If zero characters are returned, then the file that we are
	 reading from is empty!  Return EOF in that case. */
      if (result == 0)
	return EOF;

#if defined (EWOULDBLOCK)
#  define X_EWOULDBLOCK EWOULDBLOCK
#else
#  define X_EWOULDBLOCK -99
#endif

#if defined (EAGAIN)
#  define X_EAGAIN EAGAIN
#else
#  define X_EAGAIN -99
#endif

      if (errno == X_EWOULDBLOCK || errno == X_EAGAIN)
	{
	  if (sh_unset_nodelay_mode (::fileno (stream)) < 0)
	    return EOF;
	  continue;
	}

#undef X_EWOULDBLOCK
#undef X_EAGAIN

/* fprintf(stderr, "rl_getc: result = %d errno = %d\n", result, errno); */

      /* If the error that we received was EINTR, then try again,
	 this is simply an interrupted system call to read ().  We allow
	 the read to be interrupted if we caught SIGHUP, SIGTERM, or any
	 of the other signals readline treats specially. If the
	 application sets an event hook, call it for other signals.
	 Otherwise (not EINTR), some error occurred, also signifying EOF. */
      if (errno != EINTR)
	return RL_ISSTATE (RL_STATE_READCMD) ? READERR : EOF;
      /* fatal signals of interest */
#if defined (SIGHUP)
      else if (_rl_caught_signal == SIGHUP || _rl_caught_signal == SIGTERM)
#else
      else if (_rl_caught_signal == SIGTERM)
#endif
	return RL_ISSTATE (RL_STATE_READCMD) ? READERR : EOF;
      /* keyboard-generated signals of interest */
#if defined (SIGQUIT)
      else if (_rl_caught_signal == SIGINT || _rl_caught_signal == SIGQUIT)
#else
      else if (_rl_caught_signal == SIGINT)
#endif
        RL_CHECK_SIGNALS ();
#if defined (SIGTSTP)
      else if (_rl_caught_signal == SIGTSTP)
	RL_CHECK_SIGNALS ();
#endif
      /* non-keyboard-generated signals of interest */
#if defined (SIGWINCH)
      else if (_rl_caught_signal == SIGWINCH)
	RL_CHECK_SIGNALS ();
#endif /* SIGWINCH */
#if defined (SIGALRM)
      else if (_rl_caught_signal == SIGALRM
#  if defined (SIGVTALRM)
		|| _rl_caught_signal == SIGVTALRM
#  endif
	      )
        RL_CHECK_SIGNALS ();
#endif  /* SIGALRM */

      if (rl_signal_event_hook)
	((*this).*rl_signal_event_hook) ();
    }
}

#if defined (HANDLE_MULTIBYTE)
/* read multibyte char */
int
Readline::_rl_read_mbchar (char *mbchar, int size)
{
  int mb_len, c;
  size_t mbchar_bytes_length;
  wchar_t wc;
  mbstate_t ps, ps_back;

  std::memset(&ps, 0, sizeof (mbstate_t));
  std::memset(&ps_back, 0, sizeof (mbstate_t));

  mb_len = 0;
  while (mb_len < size)
    {
      c = (mb_len == 0) ? _rl_bracketed_read_key () : rl_read_key ();

      if (c < 0)
	break;

      mbchar[mb_len++] = static_cast<char> (c);

      mbchar_bytes_length = std::mbrtowc (&wc, mbchar, static_cast<size_t> (mb_len), &ps);
      if (mbchar_bytes_length == static_cast<size_t> (-1))
	break;		/* invalid byte sequence for the current locale */
      else if (mbchar_bytes_length == static_cast<size_t> (-2))
	{
	  /* shorted bytes */
	  ps = ps_back;
	  continue;
	}
      else if (mbchar_bytes_length == 0)
	{
	  mbchar[0] = '\0';	/* null wide character */
	  mb_len = 1;
	  break;
	}
      else if (mbchar_bytes_length > 0)
	break;
    }

  return mb_len;
}

/* Read a multibyte-character string whose first character is FIRST into
   the buffer MB of length MLEN.  Returns the last character read, which
   may be FIRST.  Used by the search functions, among others.  Very similar
   to _rl_read_mbchar. */
int
Readline::_rl_read_mbstring (int first, char *mb, int mlen)
{
  int i, c, n;
  mbstate_t ps;

  c = first;
  std::memset (mb, 0, static_cast<size_t> (mlen));
  for (i = 0; c >= 0 && i < mlen; i++)
    {
      mb[i] = static_cast<char> (c);
      std::memset (&ps, 0, sizeof (mbstate_t));
      n = _rl_get_char_len (mb, &ps);
      if (n == -2)
	{
	  /* Read more for multibyte character */
	  RL_SETSTATE (RL_STATE_MOREINPUT);
	  c = rl_read_key ();
	  RL_UNSETSTATE (RL_STATE_MOREINPUT);
	}
      else
	break;
    }
  return c;
}
#endif /* HANDLE_MULTIBYTE */

}  // namespace readline
