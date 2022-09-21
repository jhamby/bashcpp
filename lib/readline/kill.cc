/* kill.c -- kill ring management. */

/* Copyright (C) 1994-2020 Free Software Foundation, Inc.

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

namespace readline
{

/* **************************************************************** */
/*								    */
/*			Killing Mechanism			    */
/*								    */
/* **************************************************************** */

/* Add TEXT to the kill ring, allocating a new kill ring slot as necessary.
   If APPEND is true, and the last command was a kill, the text is appended
   to the current kill ring slot, otherwise prepended. */
void
Readline::_rl_copy_to_kill_ring (const std::string &text, bool append)
{
  size_t slot;

  /* First, find the slot to work with. */
  if (!_rl_last_command_was_kill || rl_kill_ring_length == 0)
    {
      /* Get a new slot.  */
      if (rl_kill_ring_index == DEFAULT_MAX_KILLS - 1)
        slot = 0;
      else
        slot = rl_kill_ring_index + 1;

      if (rl_kill_ring_length < DEFAULT_MAX_KILLS)
        ++rl_kill_ring_length;
    }
  else
    slot = rl_kill_ring_index;

  /* If the last command was a kill, prepend or append. */
  if (_rl_last_command_was_kill && !rl_kill_ring[slot].empty ()
      && rl_editing_mode != vi_mode)
    {
      if (append)
        {
          rl_kill_ring[slot] += text;
        }
      else
        {
          rl_kill_ring[slot].insert (0, text);
        }
    }
  else
    rl_kill_ring[slot] = text;

  rl_kill_ring_index = slot;
}

/* The way to kill something.  This appends or prepends to the last
   kill, if the last command was a kill command.  if FROM is less
   than TO, then the text is appended, otherwise prepended.  If the
   last command was not a kill command, then a new slot is made for
   this kill. */
void
Readline::rl_kill_text (size_t from, size_t to)
{
  char *text;

  /* Is there anything to kill? */
  if (from == to)
    {
      // Note: readline_internal_char () also reads this flag.
      _rl_last_command_was_kill = true;
      return;
    }

  text = rl_copy_text (from, to);

  /* Delete the copied text from the line. */
  rl_delete_text (from, to);

  _rl_copy_to_kill_ring (text, from < to);
  delete[] text;

  _rl_last_command_was_kill = true;
}

/* Now REMEMBER!  In order to do prepending or appending correctly, kill
   commands always make rl_point's original position be the FROM argument,
   and rl_point's extent be the TO argument. */

/* **************************************************************** */
/*								    */
/*			Killing Commands			    */
/*								    */
/* **************************************************************** */

/* Delete the word at point, saving the text in the kill ring. */
int
Readline::rl_kill_word (int count, int key)
{
  if (count < 0)
    return rl_backward_kill_word (-count, key);
  else
    {
      size_t orig_point = rl_point;
      rl_forward_word (count, key);

      if (rl_point != orig_point)
        rl_kill_text (orig_point, rl_point);

      rl_point = orig_point;
      if (rl_editing_mode == emacs_mode)
        rl_mark = rl_point;
    }
  return 0;
}

/* Rubout the word before point, placing it on the kill ring. */
int
Readline::rl_backward_kill_word (int count, int key)
{
  if (count < 0)
    return rl_kill_word (-count, key);
  else
    {
      size_t orig_point = rl_point;
      rl_backward_word (count, key);

      if (rl_point != orig_point)
        rl_kill_text (orig_point, rl_point);

      if (rl_editing_mode == emacs_mode)
        rl_mark = rl_point;
    }
  return 0;
}

/* Kill from here to the end of the line.  If DIRECTION is negative, kill
   back to the line start instead. */
int
Readline::rl_kill_line (int direction, int key)
{
  if (direction < 0)
    return rl_backward_kill_line (1, key);
  else
    {
      size_t orig_point = rl_point;
      rl_end_of_line (1, key);
      if (orig_point != rl_point)
        rl_kill_text (orig_point, rl_point);
      rl_point = orig_point;
      if (rl_editing_mode == emacs_mode)
        rl_mark = rl_point;
    }
  return 0;
}

/* Kill backwards to the start of the line.  If DIRECTION is negative, kill
   forwards to the line end instead. */
int
Readline::rl_backward_kill_line (int direction, int key)
{
  if (direction < 0)
    return rl_kill_line (1, key);
  else
    {
      if (rl_point == 0)
        rl_ding ();
      else
        {
          size_t orig_point = rl_point;
          rl_beg_of_line (1, key);
          if (rl_point != orig_point)
            rl_kill_text (orig_point, rl_point);
          if (rl_editing_mode == emacs_mode)
            rl_mark = rl_point;
        }
    }
  return 0;
}

/* Kill the whole line, no matter where point is. */
int
Readline::rl_kill_full_line (int, int)
{
  rl_begin_undo_group ();
  rl_point = 0;
  rl_kill_text (rl_point, rl_end ());
  rl_mark = 0;
  rl_end_undo_group ();
  return 0;
}

/* The next two functions mimic unix line editing behaviour, except they
   save the deleted text on the kill ring.  This is safer than not saving
   it, and since we have a ring, nobody should get screwed. */

/* This does what C-w does in Unix.  We can't prevent people from
   using behaviour that they expect. */
int
Readline::rl_unix_word_rubout (int count, int)
{
  if (rl_point == 0)
    rl_ding ();
  else
    {
      size_t orig_point = rl_point;
      if (count <= 0)
        count = 1;

      while (count--)
        {
          while (rl_point && whitespace (rl_line_buffer[rl_point - 1]))
            rl_point--;

          while (rl_point && (whitespace (rl_line_buffer[rl_point - 1]) == 0))
            rl_point--; /* XXX - multibyte? */
        }

      rl_kill_text (orig_point, rl_point);
      if (rl_editing_mode == emacs_mode)
        rl_mark = rl_point;
    }

  return 0;
}

/* This deletes one filename component in a Unix pathname.  That is, it
   deletes backward to directory separator (`/') or whitespace.  */
int
Readline::rl_unix_filename_rubout (int count, int)
{
  if (rl_point == 0)
    rl_ding ();
  else
    {
      size_t orig_point = rl_point;
      if (count <= 0)
        count = 1;

      while (count--)
        {
          char c = rl_line_buffer[rl_point - 1];
          while (rl_point && (whitespace (c) || c == '/'))
            {
              rl_point--;
              c = rl_line_buffer[rl_point - 1];
            }

          while (rl_point && (whitespace (c) == 0) && c != '/')
            {
              rl_point--; /* XXX - multibyte? */
              c = rl_line_buffer[rl_point - 1];
            }
        }

      rl_kill_text (orig_point, rl_point);
      if (rl_editing_mode == emacs_mode)
        rl_mark = rl_point;
    }

  return 0;
}

/* Here is C-u doing what Unix does.  You don't *have* to use these
   key-bindings.  We have a choice of killing the entire line, or
   killing from where we are to the start of the line.  We choose the
   latter, because if you are a Unix weenie, then you haven't backspaced
   into the line at all, and if you aren't, then you know what you are
   doing. */
int
Readline::rl_unix_line_discard (int, int)
{
  if (rl_point == 0)
    rl_ding ();
  else
    {
      rl_kill_text (rl_point, 0);
      rl_point = 0;
      if (rl_editing_mode == emacs_mode)
        rl_mark = rl_point;
    }
  return 0;
}

/* Copy the text in the `region' to the kill ring.  If
   `delete_after_copy` is true, delete the text from the line as well. */
void
Readline::region_kill_internal (bool delete_after_copy)
{
  char *text;

  if (rl_mark != rl_point)
    {
      text = rl_copy_text (rl_point, rl_mark);

      if (delete_after_copy)
        rl_delete_text (rl_point, rl_mark);

      _rl_copy_to_kill_ring (text, rl_point < rl_mark);
      delete[] text;
    }

  _rl_fix_point (1);
  _rl_last_command_was_kill = true;
}

/* Copy the text in the region to the kill ring. */
int
Readline::rl_copy_region_to_kill (int, int)
{
  region_kill_internal (false);
  return 0;
}

/* Kill the text between the point and mark. */
int
Readline::rl_kill_region (int, int)
{
  size_t npoint;

  npoint = (rl_point < rl_mark) ? rl_point : rl_mark;
  region_kill_internal (true);

  rl_point = npoint;
  _rl_fix_point (1);
  return 0;
}

/* Copy COUNT words to the kill ring.  DIR says which direction we look
   to find the words. */
int
Readline::_rl_copy_word_as_kill (int count, int dir)
{
  size_t om = rl_mark;
  size_t op = rl_point;

  if (dir > 0)
    rl_forward_word (count, 0);
  else
    rl_backward_word (count, 0);

  rl_mark = rl_point;

  if (dir > 0)
    rl_backward_word (count, 0);
  else
    rl_forward_word (count, 0);

  region_kill_internal (0);

  rl_mark = om;
  rl_point = op;
  return 0;
}

int
Readline::rl_copy_forward_word (int count, int key)
{
  if (count < 0)
    return rl_copy_backward_word (-count, key);

  return _rl_copy_word_as_kill (count, 1);
}

int
Readline::rl_copy_backward_word (int count, int key)
{
  if (count < 0)
    return rl_copy_forward_word (-count, key);

  return _rl_copy_word_as_kill (count, -1);
}

/* Yank back the last killed text.  This ignores arguments. */
int
Readline::rl_yank (int, int)
{
  _rl_set_mark_at_pos (rl_point);
  rl_insert_text (rl_kill_ring[rl_kill_ring_index]);
  return 0;
}

/* If the last command was yank, or yank_pop, and the text just
   before point is identical to the current kill item, then
   delete that text from the line, rotate the index down, and
   yank back some other text. */
int
Readline::rl_yank_pop (int, int)
{
  // check for empty kill ring or unexpected last function.
  if (!rl_kill_ring_length
      || ((rl_last_func != &Readline::rl_yank_pop)
          && (rl_last_func != &Readline::rl_yank)))
    {
      _rl_abort_internal ();
      return 1;
    }

  size_t l
      = static_cast<size_t> (rl_kill_ring[rl_kill_ring_index].size ());
  size_t n = rl_point - l;

  if (rl_point >= l
      && STREQN (&rl_line_buffer[n], rl_kill_ring[rl_kill_ring_index].data (),
                 l))
    {
      rl_delete_text (n, rl_point);
      rl_point = n;
      if (rl_kill_ring_index == 0)
        rl_kill_ring_index = DEFAULT_MAX_KILLS - 1;
      else
        --rl_kill_ring_index;

      rl_yank (1, 0);
      return 0;
    }
  else
    {
      _rl_abort_internal ();
      return 1;
    }
}

#if defined(VI_MODE)
int
Readline::rl_vi_yank_pop (int, int)
{
  // check for empty kill ring or unexpected last function.
  if (!rl_kill_ring_length
      || ((rl_last_func != &Readline::rl_vi_yank_pop)
          && (rl_last_func != &Readline::rl_vi_put)))
    {
      _rl_abort_internal ();
      return 1;
    }

  size_t l
      = static_cast<size_t> (rl_kill_ring[rl_kill_ring_index].size ());
  size_t n = rl_point - l;
  if (rl_point >= l
      && STREQN (&rl_line_buffer[n], rl_kill_ring[rl_kill_ring_index].data (),
                 l))
    {
      rl_delete_text (n, rl_point);
      rl_point = n;
      if (rl_kill_ring_index == 0)
        rl_kill_ring_index = DEFAULT_MAX_KILLS - 1;
      else
        --rl_kill_ring_index;

      rl_vi_put (1, 'p');
      return 0;
    }
  else
    {
      _rl_abort_internal ();
      return 1;
    }
}
#endif /* VI_MODE */

/* Yank the COUNTh argument from the previous history line, skipping
   HISTORY_SKIP lines before looking for the `previous line'. */
int
Readline::rl_yank_nth_arg_internal (int count, int key, int history_skip)
{
  size_t pos = where_history ();

  if (history_skip)
    {
      for (int i = 0; i < history_skip; i++)
        (void)previous_history ();
    }

  HIST_ENTRY *entry = previous_history ();

  history_set_pos (pos);

  if (entry == nullptr)
    {
      rl_ding ();
      return 1;
    }

  char *arg = history_arg_extract (count, count, entry->line.c_str ());
  if (!arg || !*arg)
    {
      rl_ding ();
      delete[] arg;
      return 1;
    }

  rl_begin_undo_group ();

  _rl_set_mark_at_pos (rl_point);

#if defined(VI_MODE)
  /* Vi mode always inserts a space before yanking the argument, and it
     inserts it right *after* rl_point. */
  if (rl_editing_mode == vi_mode && _rl_keymap == vi_movement_keymap ())
    {
      rl_vi_append_mode (1, key);
      rl_insert_text (" ");
    }
#endif /* VI_MODE */

  rl_insert_text (arg);
  delete[] arg;

  rl_end_undo_group ();
  return 0;
}

/* Yank the COUNTth argument from the previous history line. */
int
Readline::rl_yank_nth_arg (int count, int key)
{
  return rl_yank_nth_arg_internal (count, key, 0);
}

/* Yank the last argument from the previous history line.  This `knows'
   how rl_yank_nth_arg treats a count of `$'.  With an argument, this
   behaves the same as rl_yank_nth_arg. */
int
Readline::rl_yank_last_arg (int count, int key)
{
  if (rl_last_func != &Readline::rl_yank_last_arg)
    {
      _rl_yank_history_skip = 0;
      _rl_yank_explicit_arg = rl_explicit_arg;
      _rl_yank_count_passed = count;
      _rl_yank_direction = 1;
    }
  else
    {
      if (_rl_yank_undo_needed)
        rl_do_undo ();

      if (count < 0) /* XXX - was < 1 */
        _rl_yank_direction = -_rl_yank_direction;

      _rl_yank_history_skip += _rl_yank_direction;
      if (_rl_yank_history_skip < 0)
        _rl_yank_history_skip = 0;
    }

  int retval;
  if (_rl_yank_explicit_arg)
    retval = rl_yank_nth_arg_internal (_rl_yank_count_passed, key,
                                       _rl_yank_history_skip);
  else
    retval = rl_yank_nth_arg_internal ('$', key, _rl_yank_history_skip);

  _rl_yank_undo_needed = (retval == 0);
  return retval;
}

/* Having read the special escape sequence denoting the beginning of a
   `bracketed paste' sequence, read the rest of the pasted input until the
   closing sequence and return the pasted text. */
std::string
Readline::_rl_bracketed_text ()
{
  int c;

  size_t len = 0;
  std::string buf;

  RL_SETSTATE (RL_STATE_MOREINPUT);
  while ((c = rl_read_key ()) >= 0)
    {
      if (RL_ISSTATE (RL_STATE_MACRODEF))
        _rl_add_macro_char (static_cast<char> (c));

      if (c == '\r') /* XXX */
        c = '\n';

      buf.push_back (static_cast<char> (c));

      if (len >= BRACK_PASTE_SLEN && c == BRACK_PASTE_LAST
          && STREQN (&buf[len - BRACK_PASTE_SLEN], BRACK_PASTE_SUFF,
                     BRACK_PASTE_SLEN))
        {
          buf.resize (buf.size () - BRACK_PASTE_SLEN);
          break;
        }
    }
  RL_UNSETSTATE (RL_STATE_MOREINPUT);

  return buf;
}

/* Having read the special escape sequence denoting the beginning of a
   `bracketed paste' sequence, read the rest of the pasted input until the
   closing sequence and insert the pasted text as a single unit without
   interpretation. Temporarily highlight the inserted text. */
int
Readline::rl_bracketed_paste_begin (int, int)
{
  std::string buf = _rl_bracketed_text ();
  rl_mark = rl_point;

  int retval = (rl_insert_text (buf) == buf.size ()) ? 0 : 1;
  if (_rl_enable_active_region)
    rl_activate_mark ();

  return retval;
}

int
Readline::_rl_read_bracketed_paste_prefix (int c)
{
  char pbuf[BRACK_PASTE_SLEN + 1];
  int key = 0, ind = 0;

  const char *pbpref = BRACK_PASTE_PREF; /* XXX - debugging */
  if (c != pbpref[0])
    return 0;
  pbuf[0] = static_cast<char> (c);

  while (ind < BRACK_PASTE_SLEN - 1
         && (RL_ISSTATE (RL_STATE_INPUTPENDING | RL_STATE_MACROINPUT) == 0)
         && !_rl_pushed_input_available () && _rl_input_queued (0))
    {
      key = rl_read_key (); /* XXX - for now */
      if (key < 0)
        break;
      pbuf[++ind] = static_cast<char> (key);
      if (pbuf[ind] != pbpref[ind])
        break;
    }

  if (ind < BRACK_PASTE_SLEN - 1) /* read incomplete sequence */
    {
      while (ind >= 0)
        _rl_unget_char (pbuf[ind--]);
      return key < 0 ? key : 0;
    }
  return key < 0 ? key : 1;
}

/* Get a character from wherever we read input, handling input in bracketed
   paste mode. If we don't have or use bracketed paste mode, this can be
   used in place of rl_read_key(). */
int
Readline::_rl_bracketed_read_key ()
{
  int c, r;
  char *pbuf;
  size_t pblen;

  RL_SETSTATE (RL_STATE_MOREINPUT);
  c = rl_read_key ();
  RL_UNSETSTATE (RL_STATE_MOREINPUT);

  if (c < 0)
    return -1;

  /* read pasted data with bracketed-paste mode enabled. */
  if (_rl_enable_bracketed_paste && c == ESC
      && (r = _rl_read_bracketed_paste_prefix (c)) == 1)
    {
      pbuf = _rl_bracketed_text (&pblen);
      if (pblen == 0)
        {
          delete[] pbuf;
          return 0; /* XXX */
        }
      c = static_cast<unsigned char> (pbuf[0]);
      if (pblen > 1)
        {
          while (--pblen > 0)
            _rl_unget_char (static_cast<unsigned char> (pbuf[pblen]));
        }
      delete[] pbuf;
    }

  return c;
}

/* Get a character from wherever we read input, handling input in bracketed
   paste mode. If we don't have or use bracketed paste mode, this can be
   used in place of rl_read_key(). */
int
Readline::_rl_bracketed_read_mbstring (char *mb, int mlen)
{
  int c = _rl_bracketed_read_key ();
  if (c < 0)
    return -1;

#if defined(HANDLE_MULTIBYTE)
  if (MB_CUR_MAX > 1 && !rl_byte_oriented)
    c = _rl_read_mbstring (c, mb, mlen);
  else
#endif
    mb[0] = static_cast<char> (c);

  mb[mlen] = '\0'; /* just in case */

  return c;
}

/* A special paste command for Windows users. */
#if defined(_WIN32)
#include <windows.h>

int
Readline::rl_paste_from_clipboard (int count, int key)
{
  char *data, *ptr;
  int len;

  if (OpenClipboard (NULL) == 0)
    return 0;

  data = static_cast<char *> (GetClipboardData (CF_TEXT));
  if (data)
    {
      ptr = std::strchr (data, '\r');
      if (ptr)
        {
          len = ptr - data;
          ptr = new char[len + 1];
          ptr[len] = '\0';
          std::strncpy (ptr, data, len);
        }
      else
        ptr = data;
      _rl_set_mark_at_pos (rl_point);
      rl_insert_text (ptr);
      if (ptr != data)
        delete[] ptr;
      CloseClipboard ();
    }
  return 0;
}
#endif /* _WIN32 */

} // namespace readline
