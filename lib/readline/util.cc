/* util.c -- readline utility functions */

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

#include "readline.hh"
#include "rlprivate.hh"

#include <sys/types.h>
#include <fcntl.h>

#if defined (TIOCSTAT_IN_SYS_IOCTL)
#  include <sys/ioctl.h>
#endif /* TIOCSTAT_IN_SYS_IOCTL */

namespace readline
{

/* **************************************************************** */
/*								    */
/*			Utility Functions			    */
/*								    */
/* **************************************************************** */

/* How to abort things. */
int
Readline::_rl_abort_internal ()
{
  rl_ding ();
  rl_clear_message ();
  _rl_reset_argument ();
  rl_clear_pending_input ();
  rl_deactivate_mark ();

  while (rl_executing_macro)
    _rl_pop_executing_macro ();

  _rl_kill_kbd_macro ();

  RL_UNSETSTATE (RL_STATE_MULTIKEY);	/* XXX */

  rl_last_func = nullptr;

  throw rl_exception ();
}

int
Readline::rl_abort (int, int)
{
  return _rl_abort_internal ();
}

int
Readline::_rl_null_function (int, int)
{
  return 0;
}

int
#if defined (TIOCSTAT)
Readline::rl_tty_status (int count, int key)
#else
Readline::rl_tty_status (int, int)
#endif
{
#if defined (TIOCSTAT)
  ::ioctl (1, TIOCSTAT, reinterpret_cast<char*> (0));
  rl_refresh_line (count, key);
#else
  rl_ding ();
#endif
  return 0;
}

/* Return a copy of the string between FROM and TO.
   FROM is inclusive, TO is not. Free with delete[]. */
char *
Readline::rl_copy_text (unsigned int from, unsigned int to)
{
  /* Fix it if the caller is confused. */
  if (from > to)
    std::swap (from, to);

  unsigned int length = to - from;
  char *copy = new char[1 + length];
  std::strncpy (copy, &rl_line_buffer[from], length);
  copy[length] = '\0';
  return copy;
}

/* A function for simple tilde expansion. */
int
Readline::rl_tilde_expand (int, int)
{
  unsigned int end = rl_point;
  unsigned int start = end ? (end - 1) : 0;

  if (rl_point == rl_line_buffer.size () && rl_line_buffer[rl_point] == '~')
    {
      char *homedir = tilde_expand ("~");
      _rl_replace_text (homedir, start, end);
      delete[] homedir;
      return 0;
    }
  else if (start > 0 && rl_line_buffer[start] != '~')
    {
      for (; start > 0 && !whitespace (rl_line_buffer[start]); start--)
        ;
    }

  end = start;
  do
    end++;
  while (whitespace (rl_line_buffer[end]) == 0 && end < rl_line_buffer.size ());

  if (whitespace (rl_line_buffer[end]) || end >= rl_line_buffer.size ())
    end--;

  /* If the first character of the current word is a tilde, perform
     tilde expansion and insert the result.  If not a tilde, do
     nothing. */
  if (rl_line_buffer[start] == '~')
    {
      size_t len = end + 1 - start;
      char *temp = new char[len + 1];
      std::strncpy (temp, &rl_line_buffer[start], len);
      temp[len] = '\0';
      char *homedir = tilde_expand (temp);
      delete[] temp;

      _rl_replace_text (homedir, start, end);
      delete[] homedir;
    }

  return 0;
}

void
Readline::_rl_ttymsg (const char *format, ...)
{
  va_list args;

  va_start (args, format);

  std::fprintf (stderr, "readline: ");
  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");
  std::fflush (stderr);

  va_end (args);

  rl_forced_update_display ();
}

void
Readline::_rl_errmsg (const char *format, ...)
{
  va_list args;

  va_start (args, format);

  std::fprintf (stderr, "readline: ");
  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");
  std::fflush (stderr);

  va_end (args);
}

/* **************************************************************** */
/*								    */
/*			String Utility Functions		    */
/*								    */
/* **************************************************************** */

#ifndef HAVE_STRPBRK
/* Find the first occurrence in STRING1 of any character from STRING2.
   Return a pointer to the character in STRING1. */
char *
_rl_strpbrk (const char *string1, const char *string2)
{
  const char *scan;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  int i, v;

  std::memset (&ps, 0, sizeof (mbstate_t));
#endif

  for (; *string1; string1++)
    {
      for (scan = string2; *scan; scan++)
	{
	  if (*string1 == *scan)
	    return const_cast<char*> (string1);
	}
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
	{
	  v = _rl_get_char_len (string1, &ps);
	  if (v > 1)
	    string1 += v - 1;	/* -1 to account for auto-increment in loop */
	}
#endif
    }
  return NULL;
}
#endif

#if defined (DEBUG)
#if defined (USE_VARARGS)
static FILE *_rl_tracefp;	// XXX static global variable for debug builds

static int _rl_tropen ();

void
_rl_trace (const char *format, ...)
{
  va_list args;
  va_start (args, format);

  if (_rl_tracefp == 0)
    _rl_tropen ();

  std::vfprintf (_rl_tracefp, format, args);
  std::fprintf (_rl_tracefp, "\n");
  std::fflush (_rl_tracefp);

  va_end (args);
}

static inline int
_rl_tropen ()
{
  char fnbuf[128], *x;

  if (_rl_tracefp)
    std::fclose (_rl_tracefp);
#if defined (_WIN32) && !defined (__CYGWIN__)
  x = sh_get_env_value ("TEMP");
  if (x == 0)
    x = ".";
#else
  x = "/var/tmp";
#endif
  std::snprintf (fnbuf, sizeof (fnbuf), "%s/rltrace.%ld", x, static_cast<long> (::getpid()));
  ::unlink(fnbuf);
  _rl_tracefp = std::fopen (fnbuf, "w+");
  return _rl_tracefp != 0;
}

int
_rl_trclose ()
{
  int r;

  r = std::fclose (_rl_tracefp);
  _rl_tracefp = 0;
  return r;
}

void
_rl_settracefp (FILE *fp)
{
  _rl_tracefp = fp;
}
#endif
#endif /* DEBUG */


#if HAVE_DECL_AUDIT_USER_TTY && defined (HAVE_LIBAUDIT_H) && defined (ENABLE_TTY_AUDIT_SUPPORT)
#include <sys/socket.h>
#include <libaudit.h>
#include <linux/audit.h>
#include <linux/netlink.h>

/* Report STRING to the audit system. */
void
_rl_audit_tty (char *string)
{
  struct audit_message req;
  struct sockaddr_nl addr;
  size_t size;
  int fd;

  fd = ::socket (PF_NETLINK, SOCK_RAW, NETLINK_AUDIT);
  if (fd < 0)
    return;
  size = std::strlen (string) + 1;

  if (NLMSG_SPACE (size) > MAX_AUDIT_MESSAGE_LENGTH)
    return;

  std::memset (&req, 0, sizeof(req));
  req.nlh.nlmsg_len = NLMSG_SPACE (size);
  req.nlh.nlmsg_type = AUDIT_USER_TTY;
  req.nlh.nlmsg_flags = NLM_F_REQUEST;
  req.nlh.nlmsg_seq = 0;
  if (size && string)
    std::memcpy (NLMSG_DATA(&req.nlh), string, size);
  std::memset (&addr, 0, sizeof(addr));

  addr.nl_family = AF_NETLINK;
  addr.nl_pid = 0;
  addr.nl_groups = 0;

  ::sendto (fd, &req, req.nlh.nlmsg_len, 0, (struct sockaddr*)&addr, sizeof(addr));
  ::close (fd);
}
#endif

}  // namespace readline
