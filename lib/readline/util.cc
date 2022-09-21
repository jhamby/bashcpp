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

#include <fcntl.h>
#include <sys/types.h>

#if defined(TIOCSTAT_IN_SYS_IOCTL)
#include <sys/ioctl.h>
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

  RL_UNSETSTATE (RL_STATE_MULTIKEY); /* XXX */

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
#if defined(TIOCSTAT)
Readline::rl_tty_status (int count, int key)
#else
Readline::rl_tty_status (int, int)
#endif
{
#if defined(TIOCSTAT)
  ::ioctl (1, TIOCSTAT, reinterpret_cast<char *> (0));
  rl_refresh_line (count, key);
#else
  rl_ding ();
#endif
  return 0;
}

/* Return a copy of the string between FROM and TO.
   FROM is inclusive, TO is not. Free with delete[]. */
char *
Readline::rl_copy_text (size_t from, size_t to)
{
  /* Fix it if the caller is confused. */
  if (from > to)
    std::swap (from, to);

  size_t length = to - from;
  char *copy = new char[1 + length];
  std::strncpy (copy, &rl_line_buffer[from], length);
  copy[length] = '\0';
  return copy;
}

/* A function for simple tilde expansion. */
int
Readline::rl_tilde_expand (int, int)
{
  size_t end = rl_point;
  size_t start = end ? (end - 1) : 0;

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
  while (whitespace (rl_line_buffer[end]) == 0
         && end < rl_line_buffer.size ());

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

/* Stupid comparison routine for qsort () ing strings. */
int
_rl_qsort_string_compare (const void *v1, const void *v2)
{
  const char *const *s1 = reinterpret_cast<const char *const *> (v1);
  const char *const *s2 = reinterpret_cast<const char *const *> (v2);

#if defined (HAVE_STRCOLL)
  return (strcoll (*s1, *s2));
#else
  int result;

  result = **s1 - **s2;
  if (result == 0)
    result = strcmp (*s1, *s2);

  return result;
#endif
}

} // namespace readline
