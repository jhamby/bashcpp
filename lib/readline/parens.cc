/* parens.c -- implementation of matching parentheses feature. */

/* Copyright (C) 1987, 1989, 1992-2015, 2017 Free Software Foundation, Inc.

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

#include "posixselect.hh"

namespace readline
{

static int find_matching_open (const std::string &, unsigned int, char,
                               const char *);

/* Change emacs_standard_keymap to have bindings for paren matching when
   ON_OR_OFF is 1, change them back to self_insert when ON_OR_OFF == 0. */
void
Readline::_rl_enable_paren_matching (bool on_or_off)
{
  if (on_or_off)
    {
      /* ([{ */
      rl_bind_key_in_map (')', &Readline::rl_insert_close,
                          emacs_standard_keymap ());
      rl_bind_key_in_map (']', &Readline::rl_insert_close,
                          emacs_standard_keymap ());
      rl_bind_key_in_map ('}', &Readline::rl_insert_close,
                          emacs_standard_keymap ());

#if defined(VI_MODE)
      /* ([{ */
      rl_bind_key_in_map (')', &Readline::rl_insert_close,
                          vi_insertion_keymap ());
      rl_bind_key_in_map (']', &Readline::rl_insert_close,
                          vi_insertion_keymap ());
      rl_bind_key_in_map ('}', &Readline::rl_insert_close,
                          vi_insertion_keymap ());
#endif
    }
  else
    {
      /* ([{ */
      rl_bind_key_in_map (')', &Readline::rl_insert, emacs_standard_keymap ());
      rl_bind_key_in_map (']', &Readline::rl_insert, emacs_standard_keymap ());
      rl_bind_key_in_map ('}', &Readline::rl_insert, emacs_standard_keymap ());

#if defined(VI_MODE)
      /* ([{ */
      rl_bind_key_in_map (')', &Readline::rl_insert, vi_insertion_keymap ());
      rl_bind_key_in_map (']', &Readline::rl_insert, vi_insertion_keymap ());
      rl_bind_key_in_map ('}', &Readline::rl_insert, vi_insertion_keymap ());
#endif
    }
}

int
Readline::rl_set_paren_blink_timeout (int u)
{
  int o;

  o = _paren_blink_usec;
  if (u > 0)
    _paren_blink_usec = u;
  return o;
}

int
Readline::rl_insert_close (int count, int invoking_key)
{
  if (rl_explicit_arg || !rl_blink_matching_paren)
    _rl_insert_char (count, invoking_key);
  else
    {
#if defined(HAVE_SELECT)
      _rl_insert_char (1, invoking_key);
      ((*this).*rl_redisplay_function) ();

      int match_point = find_matching_open (rl_line_buffer, rl_point - 2,
                                            static_cast<char> (invoking_key),
                                            rl_basic_quote_characters);

      /* Emacs might message or ring the bell here, but I don't. */
      if (match_point < 0)
        return 1;

      fd_set readfds;
      struct timeval timer;
      FD_ZERO (&readfds);
      FD_SET (::fileno (rl_instream), &readfds);
      USEC_TO_TIMEVAL (_paren_blink_usec, timer);

      unsigned int orig_point = rl_point;
      rl_point = static_cast<unsigned int> (match_point);
      ((*this).*rl_redisplay_function) ();
      (void)::select (1, &readfds, nullptr, nullptr, &timer);
      rl_point = orig_point;
#else  /* !HAVE_SELECT */
      _rl_insert_char (count, invoking_key);
#endif /* !HAVE_SELECT */
    }
  return 0;
}

static int
find_matching_open (const std::string &string, unsigned int from, char closer,
                    const char *quote_chars)
{
  char opener;

  switch (closer)
    {
    case ']':
      opener = '[';
      break;
    case '}':
      opener = '{';
      break;
    case ')':
      opener = '(';
      break;
    default:
      return -1;
    }

  int level = 1;      /* The closer passed in counts as 1. */
  char delimiter = 0; /* Delimited state unknown. */

  int i;
  for (i = static_cast<int> (from); i > -1; --i)
    {
      char ch = string[static_cast<unsigned int> (i)];

      if (delimiter && (ch == delimiter))
        delimiter = 0;
      else if (quote_chars && std::strchr (quote_chars, ch))
        delimiter = ch;
      else if (!delimiter && (ch == closer))
        level++;
      else if (!delimiter && (ch == opener))
        level--;

      if (!level)
        break;
    }
  return i;
}

} // namespace readline
