/* text.c -- text handling commands for readline. */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

#include <clocale>

#if defined(__EMX__)
#define INCL_DOSPROCESS
#include <os2.h>
#endif /* __EMX__ */

namespace readline
{

/* The largest chunk of text that can be inserted in one call to
   rl_insert_text.  Text blocks larger than this are divided. */
#define TEXT_COUNT_MAX 1024

/* **************************************************************** */
/*								    */
/*			Insert and Delete			    */
/*								    */
/* **************************************************************** */

/* Insert a string of text into the line at point.  This is the only
   way that you should do insertion.  _rl_insert_char () calls this
   function.  Returns the number of characters inserted. */
unsigned int
Readline::rl_insert_text (const std::string &str)
{
  unsigned int l = static_cast<unsigned int> (str.size ());
  if (l == 0)
    return 0;

  rl_line_buffer.insert (rl_point, str);

  /* Remember how to undo this if we aren't undoing something. */
  if (!_rl_doing_an_undo)
    {
      /* If possible and desirable, concatenate the undos. */
      if ((l == 1) && rl_undo_list
          && (rl_undo_list->back ().what == UNDO_INSERT)
          && (rl_undo_list->back ().end == rl_point)
          && (rl_undo_list->back ().end - rl_undo_list->back ().start < 20))
        rl_undo_list->back ().end++;
      else
        rl_add_undo (UNDO_INSERT, rl_point, rl_point + l, nullptr);
    }
  rl_point += l;
  return l;
}

/* Delete the string between FROM and TO.  FROM is inclusive, TO is not.
   Returns the number of characters deleted. */
unsigned int
Readline::rl_delete_text (unsigned int from, unsigned int to)
{
  /* Fix it if the caller is confused. */
  if (from > to)
    std::swap (from, to);

  /* fix boundaries */
  if (to > rl_end ())
    {
      to = rl_end ();
      if (from > to)
        from = to;
    }

  unsigned int diff = to - from;
  if (diff == 0)
    return 0;

  char *text = rl_copy_text (from, to);
  rl_line_buffer.erase (from, diff);

  /* Remember how to undo this delete. */
  if (!_rl_doing_an_undo)
    rl_add_undo (UNDO_DELETE, from, to, text);
  else
    delete[] text;

  _rl_fix_mark ();
  return diff;
}

/* Replace the contents of the line buffer between START and END with
   TEXT.  The operation is undoable.  To replace the entire line in an
   undoable mode, use _rl_replace_text(text, 0, rl_line_buffer.size ()); */
unsigned int
Readline::_rl_replace_text (const std::string &text, unsigned int start,
                            unsigned int end)
{
  unsigned int n = 0;
  rl_begin_undo_group ();
  if (start <= end)
    rl_delete_text (start, end + 1);
  rl_point = start;
  if (!text.empty ())
    n = rl_insert_text (text);
  rl_end_undo_group ();

  return n;
}

/* **************************************************************** */
/*								    */
/*			Readline character functions		    */
/*								    */
/* **************************************************************** */

/* This is not a gap editor, just a stupid line input routine.  No hair
   is involved in writing any of the functions, and none should be. */

/* Note that:

   rl_point is the place in the string where the cursor is.  Sometimes
   this is the same as rl_line_buffer.size ().

   Any command that is called interactively receives two arguments.
   The first is a count: the numeric arg passed to this command.
   The second is the key which invoked this command.
*/

/* **************************************************************** */
/*								    */
/*			Movement Commands			    */
/*								    */
/* **************************************************************** */

/* Note that if you `optimize' the display for these functions, you cannot
   use said functions in other functions which do not do optimizing display.
   I.e., you will have to update the data base for rl_redisplay, and you
   might as well let rl_redisplay do that job. */

/* Move forward COUNT bytes. */
int
Readline::rl_forward_byte (int count, int key)
{
  if (count < 0)
    return rl_backward_byte (-count, key);

  if (count > 0)
    {
      unsigned int end = rl_point + static_cast<unsigned int> (count);
      unsigned int lend;
#if defined(VI_MODE)
      lend = rl_end () > 0 ? rl_end () - (VI_COMMAND_MODE ()) : rl_end ();
#else
      lend = rl_end ();
#endif

      if (end > lend)
        {
          rl_point = lend;
          rl_ding ();
        }
      else
        rl_point = end;
    }

  return 0;
}

unsigned int
Readline::_rl_forward_char_internal (int count)
{
  unsigned int point;

#if defined(HANDLE_MULTIBYTE)
  point = _rl_find_next_mbchar (rl_line_buffer, rl_point, count,
                                MB_FIND_NONZERO);

#if defined(VI_MODE)
  if (point >= rl_end () && VI_COMMAND_MODE ())
    point = _rl_find_prev_mbchar (rl_line_buffer, rl_end (), MB_FIND_NONZERO);
#endif

#else
  point = rl_point + count;
#endif

  if (point > rl_end ())
    point = rl_end ();
  return point;
}

unsigned int
Readline::_rl_backward_char_internal (int count)
{
  unsigned int point = rl_point;

#if defined(HANDLE_MULTIBYTE)
  if (count > 0)
    {
      while (count > 0 && point > 0)
        {
          point
              = _rl_find_prev_mbchar (rl_line_buffer, point, MB_FIND_NONZERO);
          count--;
        }
      if (count > 0)
        return 0; /* XXX - rl_ding() here? */
    }
#else
  if (count > 0)
    point -= count;
#endif

  return point;
}

#if defined(HANDLE_MULTIBYTE)
/* Move forward COUNT characters. */
int
Readline::rl_forward_char (int count, int key)
{
  if (MB_CUR_MAX == 1 || rl_byte_oriented)
    return rl_forward_byte (count, key);

  if (count < 0)
    return rl_backward_char (-count, key);

  if (count > 0)
    {
      if (rl_point == rl_line_buffer.size () && EMACS_MODE ())
        {
          rl_ding ();
          return 0;
        }

      unsigned int point = _rl_forward_char_internal (count);

      if (rl_point == point)
        rl_ding ();

      rl_point = point;
    }

  return 0;
}
#else  /* !HANDLE_MULTIBYTE */
int
Readline::rl_forward_char (int count, int key)
{
  return rl_forward_byte (count, key);
}
#endif /* !HANDLE_MULTIBYTE */

/* Backwards compatibility. */
int
Readline::rl_forward (int count, int key)
{
  return rl_forward_char (count, key);
}

/* Move backward COUNT bytes. */
int
Readline::rl_backward_byte (int count, int key)
{
  if (count < 0)
    return rl_forward_byte (-count, key);

  if (count > 0)
    {
      if (rl_point < static_cast<size_t> (count))
        {
          rl_point = 0;
          rl_ding ();
        }
      else
        rl_point -= static_cast<size_t> (count);
    }

  return 0;
}

#if defined(HANDLE_MULTIBYTE)
/* Move backward COUNT characters. */
int
Readline::rl_backward_char (int count, int key)
{
  if (MB_CUR_MAX == 1 || rl_byte_oriented)
    return rl_backward_byte (count, key);

  if (count < 0)
    return rl_forward_char (-count, key);

  if (count > 0)
    {
      unsigned int point = rl_point;

      while (count > 0 && point > 0)
        {
          point
              = _rl_find_prev_mbchar (rl_line_buffer, point, MB_FIND_NONZERO);
          count--;
        }
      if (count > 0)
        {
          rl_point = 0;
          rl_ding ();
        }
      else
        rl_point = point;
    }

  return 0;
}
#else
int
Readline::rl_backward_char (int count, int key)
{
  return rl_backward_byte (count, key);
}
#endif

/* Backwards compatibility. */
int
Readline::rl_backward (int count, int key)
{
  return rl_backward_char (count, key);
}

/* Move to the beginning of the line. */
int
Readline::rl_beg_of_line (int, int)
{
  rl_point = 0;
  return 0;
}

/* Move to the end of the line. */
int
Readline::rl_end_of_line (int, int)
{
  rl_point = rl_end ();
  return 0;
}

/* Move forward a word.  We do what Emacs does.  Handles multibyte chars. */
int
Readline::rl_forward_word (int count, int key)
{
  if (count < 0)
    return rl_backward_word (-count, key);

  while (count)
    {
      if (rl_point > rl_end ())
        rl_point = rl_end ();

      if (rl_point == rl_end ())
        return 0;

      /* If we are not in a word, move forward until we are in one.
         Then, move forward until we hit a non-alphabetic character. */
      wchar_t c = _rl_char_value (rl_line_buffer, rl_point);

      if (_rl_walphabetic (c) == 0)
        {
          rl_point
              = MB_NEXTCHAR (rl_line_buffer, rl_point, 1, MB_FIND_NONZERO);
          while (rl_point < rl_end ())
            {
              c = _rl_char_value (rl_line_buffer, rl_point);
              if (_rl_walphabetic (c))
                break;
              rl_point
                  = MB_NEXTCHAR (rl_line_buffer, rl_point, 1, MB_FIND_NONZERO);
            }
        }

      if (rl_point > rl_end ())
        rl_point = rl_end ();

      if (rl_point == rl_end ())
        return 0;

      rl_point = MB_NEXTCHAR (rl_line_buffer, rl_point, 1, MB_FIND_NONZERO);
      while (rl_point < rl_end ())
        {
          c = _rl_char_value (rl_line_buffer, rl_point);
          if (_rl_walphabetic (c) == 0)
            break;
          rl_point
              = MB_NEXTCHAR (rl_line_buffer, rl_point, 1, MB_FIND_NONZERO);
        }

      --count;
    }

  return 0;
}

/* Move backward a word.  We do what Emacs does.  Handles multibyte chars. */
int
Readline::rl_backward_word (int count, int key)
{
  if (count < 0)
    return rl_forward_word (-count, key);

  while (count)
    {
      if (rl_point == 0)
        return 0;

      /* Like rl_forward_word (), except that we look at the characters
         just before point. */

      unsigned int p = MB_PREVCHAR (rl_line_buffer, rl_point, MB_FIND_NONZERO);
      wchar_t c = _rl_char_value (rl_line_buffer, p);

      if (_rl_walphabetic (c) == 0)
        {
          rl_point = p;
          while (rl_point > 0)
            {
              p = MB_PREVCHAR (rl_line_buffer, rl_point, MB_FIND_NONZERO);
              c = _rl_char_value (rl_line_buffer, p);
              if (_rl_walphabetic (c))
                break;
              rl_point = p;
            }
        }

      while (rl_point)
        {
          p = MB_PREVCHAR (rl_line_buffer, rl_point, MB_FIND_NONZERO);
          c = _rl_char_value (rl_line_buffer, p);
          if (_rl_walphabetic (c) == 0)
            break;
          else
            rl_point = p;
        }

      --count;
    }

  return 0;
}

/* Clear the current line.  Numeric argument to C-l does this. */
int
Readline::rl_refresh_line (int, int)
{
  _rl_refresh_line ();
  rl_display_fixed = true;
  return 0;
}

/* C-l typed to a line without quoting clears the screen, and then reprints
   the prompt and the current input line.  Given a numeric arg, redraw only
   the current line. */
int
Readline::rl_clear_screen (int count, int key)
{
  if (rl_explicit_arg)
    {
      rl_refresh_line (count, key);
      return 0;
    }

  _rl_clear_screen (false); /* calls termcap function to clear screen */
  rl_keep_mark_active ();
  rl_forced_update_display ();
  rl_display_fixed = true;
  return 0;
}

int
Readline::rl_clear_display (int, int)
{
  _rl_clear_screen (
      true); /* calls termcap function to clear screen and scrollback buffer */
  rl_forced_update_display ();
  rl_display_fixed = true;
  return 0;
}

int
Readline::rl_previous_screen_line (int, int key)
{
  int c = static_cast<int> (_rl_term_autowrap ? _rl_screenwidth
                                              : (_rl_screenwidth + 1));
  return rl_backward_char (c, key);
}

int
Readline::rl_next_screen_line (int, int key)
{
  int c = static_cast<int> (_rl_term_autowrap ? _rl_screenwidth
                                              : (_rl_screenwidth + 1));
  return rl_forward_char (c, key);
}

int
Readline::rl_skip_csi_sequence (int, int)
{
  int ch;

  RL_SETSTATE (RL_STATE_MOREINPUT);
  do
    ch = rl_read_key ();
  while (ch >= 0x20 && ch < 0x40);
  RL_UNSETSTATE (RL_STATE_MOREINPUT);

  return ch < 0;
}

int
Readline::rl_arrow_keys (int count, int)
{
  int ch;

  RL_SETSTATE (RL_STATE_MOREINPUT);
  ch = rl_read_key ();
  RL_UNSETSTATE (RL_STATE_MOREINPUT);
  if (ch < 0)
    return 1;

  switch (_rl_to_upper (ch))
    {
    case 'A':
      rl_get_previous_history (count, ch);
      break;

    case 'B':
      rl_get_next_history (count, ch);
      break;

    case 'C':
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
        rl_forward_char (count, ch);
      else
        rl_forward_byte (count, ch);
      break;

    case 'D':
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
        rl_backward_char (count, ch);
      else
        rl_backward_byte (count, ch);
      break;

    default:
      rl_ding ();
    }

  return 0;
}

/* **************************************************************** */
/*								    */
/*			Text commands				    */
/*								    */
/* **************************************************************** */

/* Insert the character C at the current location, moving point forward.
   If C introduces a multibyte sequence, we read the whole sequence and
   then insert the multibyte char into the line buffer. */
int
Readline::_rl_insert_char (int count, int c)
{
  if (count <= 0)
    return 0;

  std::string string;
#ifdef HANDLE_MULTIBYTE
  int string_size;
  char incoming[MB_LEN_MAX + 1];
  int incoming_length = 0;
  mbstate_t ps_back;
#endif

#if defined(HANDLE_MULTIBYTE)
  if (MB_CUR_MAX == 1 || rl_byte_oriented)
    {
      incoming[0] = static_cast<char> (c);
      incoming[1] = '\0';
      incoming_length = 1;
    }
  else if (_rl_utf8locale && (c & 0x80) == 0)
    {
      incoming[0] = static_cast<char> (c);
      incoming[1] = '\0';
      incoming_length = 1;
    }
  else
    {
      wchar_t wc;
      size_t ret;

      if (_rl_stored_count <= 0)
        _rl_stored_count = count;
      else
        count = _rl_stored_count;

      ps_back = _rl_pending_mbstate;
      _rl_pending_bytes[_rl_pending_bytes_length++] = static_cast<char> (c);

      ret = std::mbrtowc (&wc, _rl_pending_bytes,
                          static_cast<size_t> (_rl_pending_bytes_length),
                          &_rl_pending_mbstate);

      if (ret == static_cast<size_t> (-2))
        {
          /* Bytes too short to compose character, try to wait for next byte.
             Restore the state of the byte sequence, because in this case the
             effect of mbstate is undefined. */
          _rl_pending_mbstate = ps_back;
          return 1;
        }
      else if (ret == static_cast<size_t> (-1))
        {
          /* Invalid byte sequence for the current locale.  Treat first byte
             as a single character. */
          incoming[0] = _rl_pending_bytes[0];
          incoming[1] = '\0';
          incoming_length = 1;
          _rl_pending_bytes_length--;
          std::memmove (_rl_pending_bytes, _rl_pending_bytes + 1,
                        static_cast<size_t> (_rl_pending_bytes_length));
          /* Clear the state of the byte sequence, because in this case the
             effect of mbstate is undefined. */
          std::memset (&_rl_pending_mbstate, 0, sizeof (mbstate_t));
        }
      else if (ret == 0)
        {
          incoming[0] = '\0';
          incoming_length = 0;
          _rl_pending_bytes_length--;
          /* Clear the state of the byte sequence, because in this case the
             effect of mbstate is undefined. */
          std::memset (&_rl_pending_mbstate, 0, sizeof (mbstate_t));
        }
      else if (ret == 1)
        {
          incoming[0] = _rl_pending_bytes[0];
          incoming[incoming_length = 1] = '\0';
          _rl_pending_bytes_length = 0;
        }
      else
        {
          /* We successfully read a single multibyte character. */
          std::memcpy (incoming, _rl_pending_bytes,
                       static_cast<size_t> (_rl_pending_bytes_length));

          incoming[_rl_pending_bytes_length] = '\0';
          incoming_length = _rl_pending_bytes_length;
          _rl_pending_bytes_length = 0;
        }
    }
#endif /* HANDLE_MULTIBYTE */

  /* If we can optimize, then do it.  But don't let people crash
     readline because of extra large arguments. */
  if (count > 1 && count <= TEXT_COUNT_MAX)
    {
#if defined(HANDLE_MULTIBYTE)
      string_size = count * incoming_length;
      int i = 0;
      while (i < string_size)
        {
          if (incoming_length == 1)
            {
              string.push_back (*incoming);
              i++;
            }
          else
            {
              string.append (incoming, static_cast<size_t> (incoming_length));
              i += incoming_length;
            }
        }
      incoming_length = 0;
      _rl_stored_count = 0;
#else  /* !HANDLE_MULTIBYTE */
      int i;
      for (i = 0; i < count; i++)
        string.push_back (c);
#endif /* !HANDLE_MULTIBYTE */

      rl_insert_text (string);
      return 0;
    }

  if (count > TEXT_COUNT_MAX)
    {
      int decreaser;
#if defined(HANDLE_MULTIBYTE)
      string_size = incoming_length * TEXT_COUNT_MAX;

      int i = 0;
      while (i < string_size)
        {
          if (incoming_length == 1)
            {
              string.push_back (*incoming);
              i++;
            }
          else
            {
              string.append (incoming, static_cast<size_t> (incoming_length));
              i += incoming_length;
            }
        }

      while (count)
        {
          decreaser = (count > TEXT_COUNT_MAX) ? TEXT_COUNT_MAX : count;
          string.resize (static_cast<size_t> (decreaser * incoming_length));
          rl_insert_text (string);
          count -= decreaser;
        }

      incoming_length = 0;
      _rl_stored_count = 0;
#else  /* !HANDLE_MULTIBYTE */
      for (int i = 0; i < TEXT_COUNT_MAX; i++)
        string.push_back (c);

      while (count)
        {
          decreaser = (count > TEXT_COUNT_MAX ? TEXT_COUNT_MAX : count);
          string.resize (decreaser);
          rl_insert_text (str);
          count -= decreaser;
        }
#endif /* !HANDLE_MULTIBYTE */

      return 0;
    }

  if (MB_CUR_MAX == 1 || rl_byte_oriented)
    {
      /* We are inserting a single character.
         If there is pending input, then make a string of all of the
         pending characters that are bound to rl_insert, and insert
         them all.  Don't do this if we're current reading input from
         a macro. */
      if ((RL_ISSTATE (RL_STATE_MACROINPUT) == 0)
          && _rl_pushed_input_available ())
        _rl_insert_typein (c);
      else
        {
          /* Inserting a single character. */
          string.push_back (static_cast<char> (c));
          rl_insert_text (string);
        }
    }
#if defined(HANDLE_MULTIBYTE)
  else
    {
      rl_insert_text (incoming);
      _rl_stored_count = 0;
    }
#endif

  return 0;
}

/* Overwrite the character at point (or next COUNT characters) with C.
   If C introduces a multibyte character sequence, read the entire sequence
   before starting the overwrite loop. */
int
Readline::_rl_overwrite_char (int count, int c)
{
  int i;
#if defined(HANDLE_MULTIBYTE)
  char mbkey[MB_LEN_MAX];

  /* Read an entire multibyte character sequence to insert COUNT times. */
  if (count > 0 && MB_CUR_MAX > 1 && !rl_byte_oriented)
    (void)_rl_read_mbstring (c, mbkey, MB_LEN_MAX);
#endif

  rl_begin_undo_group ();

  for (i = 0; i < count; i++)
    {
#if defined(HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
        rl_insert_text (mbkey);
      else
#endif
        _rl_insert_char (1, c);

      if (rl_point < rl_line_buffer.size ())
        rl_delete (1, c);
    }

  rl_end_undo_group ();

  return 0;
}

int
Readline::rl_insert (int count, int c)
{
  int r = (rl_insert_mode == RL_IM_INSERT) ? _rl_insert_char (count, c)
                                           : _rl_overwrite_char (count, c);

  /* XXX -- attempt to batch-insert pending input that maps to self-insert */
  int x = 0;
  int n = -2;
  while (_rl_optimize_typeahead && rl_num_chars_to_read == 0
         && (RL_ISSTATE (RL_STATE_INPUTPENDING | RL_STATE_MACROINPUT) == 0)
         && _rl_pushed_input_available () == 0 && _rl_input_queued (0)
         && (n = rl_read_key ()) > 0 && _rl_keymap[n & 0xff].type == ISFUNC
         && _rl_keymap[n & 0xff].value.function == &Readline::rl_insert)
    {
      r = (rl_insert_mode == RL_IM_INSERT) ? _rl_insert_char (1, n)
                                           : _rl_overwrite_char (1, n);
      /* _rl_insert_char keeps its own set of pending characters to compose a
         complete multibyte character, and only returns 1 if it sees a
         character that's part of a multibyte character but too short to
         complete one.  We can try to read another character in the hopes that
         we will get the next one or just punt.  Right now we try to read
         another character. We don't want to call rl_insert_next if
         _rl_insert_char has already stored the character in the pending_bytes
         array because that will result in doubled input. */
      n = -2;
      x++;        /* count of bytes of typeahead read, currently unused */
      if (r == 1) /* read partial multibyte character */
        continue;
      if (rl_done || r != 0)
        break;
    }

  if (n != -2) /* -2 = sentinel value for having inserted N */
    {
      /* setting rl_pending_input inhibits setting rl_last_func so we do it
         ourselves here */
      rl_last_func = &Readline::rl_insert;
      _rl_reset_argument ();
      rl_executing_keyseq.clear ();
      rl_execute_next (n);
      return 0;
    }

  return r;
}

/* Insert the next typed character verbatim. */
int
Readline::_rl_insert_next (int count)
{
  int c;

  RL_SETSTATE (RL_STATE_MOREINPUT);
  c = rl_read_key ();
  RL_UNSETSTATE (RL_STATE_MOREINPUT);

  if (c < 0)
    return 1;

  if (RL_ISSTATE (RL_STATE_MACRODEF))
    _rl_add_macro_char (static_cast<char> (c));

#if defined(HANDLE_SIGNALS)
  if (RL_ISSTATE (RL_STATE_CALLBACK) == 0)
    _rl_restore_tty_signals ();
#endif

  return _rl_insert_char (count, c);
}

#if defined(READLINE_CALLBACKS)
int
Readline::_rl_insert_next_callback (_rl_callback_generic_arg *data)
{
  int count = data->count;
  int r = 0;

  if (count < 0)
    {
      data->count++;
      r = _rl_insert_next (1);
      _rl_want_redisplay = 1;
      /* If we should keep going, leave the callback function installed */
      if (data->count < 0 && r == 0)
        return r;
      count = 0; /* data->count == 0 || r != 0; force break below */
    }

  /* Deregister function, let rl_callback_read_char deallocate data */
  _rl_callback_func = nullptr;
  _rl_want_redisplay = true;

  if (count == 0)
    return r;

  return _rl_insert_next (count);
}
#endif

int
Readline::rl_quoted_insert (int count, int)
{
  /* Let's see...should the callback interface futz with signal handling? */
#if defined(HANDLE_SIGNALS)
  if (RL_ISSTATE (RL_STATE_CALLBACK) == 0)
    _rl_disable_tty_signals ();
#endif

#if defined(READLINE_CALLBACKS)
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      _rl_callback_data = new _rl_callback_generic_arg (count);
      _rl_callback_func = &Readline::_rl_insert_next_callback;
      return 0;
    }
#endif

  /* A negative count means to quote the next -COUNT characters. */
  if (count < 0)
    {
      int r;

      do
        r = _rl_insert_next (1);
      while (r == 0 && ++count < 0);
      return r;
    }

  return _rl_insert_next (count);
}

/* Insert a tab character. */
int
Readline::rl_tab_insert (int count, int)
{
  return _rl_insert_char (count, '\t');
}

/* What to do when a NEWLINE is pressed.  We accept the whole line.
   KEY is the key that invoked this command.  I guess it could have
   meaning in the future. */
int
Readline::rl_newline (int, int)
{
  if (rl_mark_active_p ())
    {
      rl_deactivate_mark ();
      ((*this).*rl_redisplay_function) ();
      _rl_want_redisplay = false;
    }

  rl_done = true;

  if (_rl_history_preserve_point)
    _rl_history_saved_point = (rl_point == rl_line_buffer.size ())
                                  ? static_cast<unsigned int> (-1)
                                  : rl_point;

  RL_SETSTATE (RL_STATE_DONE);

#if defined(VI_MODE)
  if (rl_editing_mode == vi_mode)
    {
      _rl_vi_done_inserting ();
      if (!_rl_vi_textmod_command (_rl_vi_last_command)) /* XXX */
        _rl_vi_reset_last ();
    }
#endif /* VI_MODE */

  /* If we've been asked to erase empty lines, suppress the final update,
     since _rl_update_final calls rl_crlf(). */
  if (rl_erase_empty_line && rl_point == 0 && rl_line_buffer.empty ())
    return 0;

  if (_rl_echoing_p)
    _rl_update_final ();

  return 0;
}

/* What to do for some uppercase characters, like meta characters,
   and some characters appearing in emacs_ctlx_keymap.  This function
   is just a stub, you bind keys to it and the code in _rl_dispatch ()
   is special cased. */
int
Readline::rl_do_lowercase_version (int, int)
{
  return 0;
}

/* This is different from what vi does, so the code's not shared.  Emacs
   rubout in overwrite mode has one oddity:  it replaces a control
   character that's displayed as two characters (^X) with two spaces. */
int
Readline::_rl_overwrite_rubout (int count, int key)
{
  if (rl_point == 0)
    {
      rl_ding ();
      return 1;
    }

  unsigned int opoint = rl_point;
  int i, l;

  /* L == number of spaces to insert */
  for (i = l = 0; i < count; i++)
    {
      rl_backward_char (1, key);
      l += rl_character_len (rl_line_buffer[rl_point],
                             rl_point); /* not exactly right */
    }

  rl_begin_undo_group ();

  if (count > 1 || rl_explicit_arg)
    rl_kill_text (opoint, rl_point);
  else
    rl_delete_text (opoint, rl_point);

  /* Emacs puts point at the beginning of the sequence of spaces. */
  if (rl_point < rl_line_buffer.size ())
    {
      opoint = rl_point;
      _rl_insert_char (l, ' ');
      rl_point = opoint;
    }

  rl_end_undo_group ();

  return 0;
}

/* Rubout the character behind point. */
int
Readline::rl_rubout (int count, int key)
{
  if (count < 0)
    return rl_delete (-count, key);

  if (!rl_point)
    {
      rl_ding ();
      return 1;
    }

  if (rl_insert_mode == RL_IM_OVERWRITE)
    return _rl_overwrite_rubout (count, key);

  return _rl_rubout_char (count, key);
}

int
Readline::_rl_rubout_char (int count, int key)
{
  /* Duplicated code because this is called from other parts of the library. */
  if (count < 0)
    return rl_delete (-count, key);

  if (rl_point == 0)
    {
      rl_ding ();
      return 1;
    }

  unsigned int orig_point = rl_point;
  if (count > 1 || rl_explicit_arg)
    {
      rl_backward_char (count, key);
      rl_kill_text (orig_point, rl_point);
    }
  else if (MB_CUR_MAX == 1 || rl_byte_oriented)
    {
      int c = static_cast<unsigned char> (rl_line_buffer[--rl_point]);
      rl_delete_text (rl_point, orig_point);
      /* The erase-at-end-of-line hack is of questionable merit now. */
      if (rl_point == rl_line_buffer.size () && std::isprint (c)
          && _rl_last_c_pos)
        {
          unsigned int l = rl_character_len (c, rl_point);
          _rl_erase_at_end_of_line (l);
        }
    }
  else
    {
      rl_point
          = _rl_find_prev_mbchar (rl_line_buffer, rl_point, MB_FIND_NONZERO);
      rl_delete_text (rl_point, orig_point);
    }

  return 0;
}

/* Delete the character under the cursor.  Given a numeric argument,
   kill that many characters instead. */
int
Readline::rl_delete (int count, int key)
{
  if (count < 0)
    return _rl_rubout_char (-count, key);

  if (rl_point == rl_line_buffer.size ())
    {
      rl_ding ();
      return 1;
    }

  if (count > 1 || rl_explicit_arg)
    {
      unsigned int xpoint = rl_point;
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
        rl_forward_char (count, key);
      else
        rl_forward_byte (count, key);

      rl_kill_text (xpoint, rl_point);
      rl_point = xpoint;
    }
  else
    {
      unsigned int xpoint
          = MB_NEXTCHAR (rl_line_buffer, rl_point, 1, MB_FIND_NONZERO);
      rl_delete_text (rl_point, xpoint);
    }
  return 0;
}

/* Delete the character under the cursor, unless the insertion
   point is at the end of the line, in which case the character
   behind the cursor is deleted.  COUNT is obeyed and may be used
   to delete forward or backward that many characters. */
int
Readline::rl_rubout_or_delete (int count, int key)
{
  if (rl_end () != 0 && rl_point == rl_end ())
    return _rl_rubout_char (count, key);
  else
    return rl_delete (count, key);
}

/* Delete all spaces and tabs around point. */
int
Readline::rl_delete_horizontal_space (int, int)
{
  while (rl_point && whitespace (rl_line_buffer[rl_point - 1]))
    rl_point--;

  unsigned int start = rl_point;

  while (rl_point < rl_end () && whitespace (rl_line_buffer[rl_point]))
    rl_point++;

  if (start != rl_point)
    {
      rl_delete_text (start, rl_point);
      rl_point = start;
    }

  return 0;
}

/* Like the tcsh editing function delete-char-or-list.  The eof character
   is caught before this is invoked, so this really does the same thing as
   delete-char-or-list-or-eof, as long as it's bound to the eof character. */
int
Readline::rl_delete_or_show_completions (int count, int key)
{
  size_t rl_end = rl_line_buffer.size ();
  if (rl_end != 0 && rl_point == rl_end)
    return rl_possible_completions (count, key);
  else
    return rl_delete (count, key);
}

#ifndef RL_COMMENT_BEGIN_DEFAULT
#define RL_COMMENT_BEGIN_DEFAULT "#"
#endif

/* Turn the current line into a comment in shell history.
   A K*rn shell style function. */
int
Readline::rl_insert_comment (int, int key)
{
  const char *rl_comment_text;

  rl_beg_of_line (1, key);
  rl_comment_text
      = _rl_comment_begin ? _rl_comment_begin : RL_COMMENT_BEGIN_DEFAULT;

  if (rl_explicit_arg == 0)
    rl_insert_text (rl_comment_text);
  else
    {
      unsigned int rl_comment_len
          = static_cast<unsigned int> (std::strlen (rl_comment_text));
      if (STREQN (rl_comment_text, rl_line_buffer.c_str (), rl_comment_len))
        rl_delete_text (rl_point, rl_point + rl_comment_len);
      else
        rl_insert_text (rl_comment_text);
    }

  ((*this).*rl_redisplay_function) ();
  rl_newline (1, '\n');

  return 0;
}

/* **************************************************************** */
/*								    */
/*			Changing Case				    */
/*								    */
/* **************************************************************** */

/* Uppercase the word at point. */
int
Readline::rl_upcase_word (int count, int)
{
  return rl_change_case (count, UpCase);
}

/* Lowercase the word at point. */
int
Readline::rl_downcase_word (int count, int)
{
  return rl_change_case (count, DownCase);
}

/* Upcase the first letter, downcase the rest. */
int
Readline::rl_capitalize_word (int count, int)
{
  return rl_change_case (count, CapCase);
}

/* The meaty function.
   Change the case of COUNT words, performing OP on them.
   OP is one of UpCase, DownCase, or CapCase.
   If a negative argument is given, leave point where it started,
   otherwise, leave it where it moves to. */
int
Readline::rl_change_case (int count, case_change_type op)
{
  unsigned int start, end;
#if defined(HANDLE_MULTIBYTE)
  wchar_t wc, nwc;
  char mb[MB_LEN_MAX + 1];
  size_t mlen, m;
  mbstate_t mps;
#endif

  start = rl_point;
  rl_forward_word (count, 0);
  end = rl_point;

  if (op != UpCase && op != DownCase && op != CapCase)
    {
      rl_ding ();
      return 1;
    }

  if (count < 0)
    std::swap (start, end);

#if defined(HANDLE_MULTIBYTE)
  std::memset (&mps, 0, sizeof (mbstate_t));
#endif

  /* We are going to modify some text, so let's prepare to undo it. */
  rl_modifying (start, end);

  bool inword = false;
  while (start < end)
    {
      wchar_t c = _rl_char_value (rl_line_buffer, start);

      /*  This assumes that the upper and lower case versions are the same
       * width. */
      unsigned int next
          = MB_NEXTCHAR (rl_line_buffer, start, 1, MB_FIND_NONZERO);

      if (_rl_walphabetic (c) == 0)
        {
          inword = false;
          start = next;
          continue;
        }

      case_change_type nop;
      if (op == CapCase)
        {
          nop = inword ? DownCase : UpCase;
          inword = true;
        }
      else
        nop = op;

      /* Can't check isascii here; some languages (e.g, Turkish) have
         multibyte upper and lower case equivalents of single-byte ascii
         characters */
      if (MB_CUR_MAX == 1 || rl_byte_oriented)
        {
          int nc = (nop == UpCase) ? _rl_to_upper (c) : _rl_to_lower (c);
          rl_line_buffer[start] = static_cast<char> (nc);
        }
#if defined(HANDLE_MULTIBYTE)
      else
        {
          m = std::mbrtowc (&wc, rl_line_buffer.c_str () + start, end - start,
                            &mps);
          if (MB_INVALIDCH (m))
            wc = static_cast<wchar_t> (rl_line_buffer[start]);
          else if (MB_NULLWCH (m))
            wc = L'\0';
          nwc = static_cast<wchar_t> (
              (nop == UpCase) ? _rl_to_wupper (static_cast<wint_t> (wc))
                              : _rl_to_wlower (static_cast<wint_t> (wc)));
          if (nwc != wc) /*  just skip unchanged characters */
            {
              mbstate_t ts;

              std::memset (&ts, 0, sizeof (mbstate_t));
              mlen = std::wcrtomb (mb, nwc, &ts);
              if (static_cast<ssize_t> (mlen) < 0)
                {
                  nwc = wc;
                  std::memset (&ts, 0, sizeof (mbstate_t));
                  mlen = std::wcrtomb (mb, nwc, &ts);
                  if (static_cast<ssize_t> (mlen) < 0) /* should not happen */
                    mlen = m = 0;
                }
              /* what to do if m != mlen? C++ makes replacement simple. */
              /* m == length of old char, mlen == length of new char */
              if (m == mlen)
                std::memcpy (&rl_line_buffer[start], mb, mlen);
              else
                rl_line_buffer.replace (start, m, mb, mlen);
            }
        }
#endif

      start = next;
    }

  rl_point = end;
  return 0;
}

/* **************************************************************** */
/*								    */
/*			Transposition				    */
/*								    */
/* **************************************************************** */

/* Transpose the words at point.  If point is at the end of the line,
   transpose the two words before point. */
int
Readline::rl_transpose_words (int count, int key)
{
  char *word1, *word2;
  unsigned int w1_beg, w1_end, w2_beg, w2_end;
  unsigned int orig_point = rl_point;

  if (!count)
    return 0;

  /* Find the two words. */
  rl_forward_word (count, key);
  w2_end = rl_point;
  rl_backward_word (1, key);
  w2_beg = rl_point;
  rl_backward_word (count, key);
  w1_beg = rl_point;
  rl_forward_word (1, key);
  w1_end = rl_point;

  /* Do some check to make sure that there really are two words. */
  if ((w1_beg == w2_beg) || (w2_beg < w1_end))
    {
      rl_ding ();
      rl_point = orig_point;
      return 1;
    }

  /* Get the text of the words. */
  word1 = rl_copy_text (w1_beg, w1_end);
  word2 = rl_copy_text (w2_beg, w2_end);

  /* We are about to do many insertions and deletions.  Remember them
     as one operation. */
  rl_begin_undo_group ();

  /* Do the stuff at word2 first, so that we don't have to worry
     about word1 moving. */
  rl_point = w2_beg;
  rl_delete_text (w2_beg, w2_end);
  rl_insert_text (word1);

  rl_point = w1_beg;
  rl_delete_text (w1_beg, w1_end);
  rl_insert_text (word2);

  /* This is exactly correct since the text before this point has not
     changed in length. */
  rl_point = w2_end;

  /* I think that does it. */
  rl_end_undo_group ();
  delete[] word1;
  delete[] word2;

  return 0;
}

/* Transpose the characters at point.  If point is at the end of the line,
   then transpose the characters before point. */
int
Readline::rl_transpose_chars (int count, int)
{
  if (count == 0)
    return 0;

  if (!rl_point || rl_line_buffer.size () < 2)
    {
      rl_ding ();
      return 1;
    }

  rl_begin_undo_group ();

  if (rl_point == rl_line_buffer.size ())
    {
      rl_point = MB_PREVCHAR (rl_line_buffer, rl_point, MB_FIND_NONZERO);
      count = 1;
    }

  unsigned int prev_point = rl_point;
  rl_point = MB_PREVCHAR (rl_line_buffer, rl_point, MB_FIND_NONZERO);

#if defined(HANDLE_MULTIBYTE)
  unsigned int char_length = prev_point - rl_point;
  char *dummy = new char[char_length + 1];
  unsigned int i;
  for (i = 0; i < char_length; i++)
    dummy[i] = rl_line_buffer[rl_point + i];
  dummy[i] = '\0';
#else
  unsigned int char_length = 1;
  char dummy[2] = { rl_line_buffer[rl_point], '\0' };
#endif

  rl_delete_text (rl_point, rl_point + char_length);

  rl_point = _rl_find_next_mbchar (rl_line_buffer, rl_point, count,
                                   MB_FIND_NONZERO);

  _rl_fix_point (0);
  rl_insert_text (dummy);
  rl_end_undo_group ();

#if defined(HANDLE_MULTIBYTE)
  delete[] dummy;
#endif

  return 0;
}

/* **************************************************************** */
/*								    */
/*			Character Searching			    */
/*								    */
/* **************************************************************** */

int
#if defined(HANDLE_MULTIBYTE)
Readline::_rl_char_search_internal (int count, int dir, char *smbchar,
                                    unsigned int len)
#else
Readline::_rl_char_search_internal (int count, int dir, int schar)
#endif
{
#if defined(HANDLE_MULTIBYTE)
  size_t prepos;
#endif

  if (dir == 0)
    return 1;

  unsigned int pos = rl_point;
  int inc = (dir < 0) ? -1 : 1;
  while (count)
    {
      if ((dir < 0 && pos == 0) || (dir > 0 && pos >= rl_end ()))
        {
          rl_ding ();
          return 1;
        }

#if defined(HANDLE_MULTIBYTE)
      pos = (inc > 0)
                ? _rl_find_next_mbchar (rl_line_buffer, pos, 1, MB_FIND_ANY)
                : _rl_find_prev_mbchar (rl_line_buffer, pos, MB_FIND_ANY);
#else
      pos += inc;
#endif
      do
        {
#if defined(HANDLE_MULTIBYTE)
          if (_rl_is_mbchar_matched (rl_line_buffer.c_str (), pos, rl_end (),
                                     smbchar, len))
#else
          if (rl_line_buffer[pos] == schar)
#endif
            {
              count--;
              if (dir < 0)
                rl_point = (dir == BTO) ? _rl_find_next_mbchar (
                               rl_line_buffer, pos, 1, MB_FIND_ANY)
                                        : pos;
              else
                rl_point = (dir == FTO) ? _rl_find_prev_mbchar (
                               rl_line_buffer, pos, MB_FIND_ANY)
                                        : pos;
              break;
            }
#if defined(HANDLE_MULTIBYTE)
          prepos = pos;
#endif
        }
#if defined(HANDLE_MULTIBYTE)
      while ((dir < 0) ? (pos = _rl_find_prev_mbchar (rl_line_buffer, pos,
                                                      MB_FIND_ANY))
                             != prepos
                       : (pos = _rl_find_next_mbchar (rl_line_buffer, pos, 1,
                                                      MB_FIND_ANY))
                             != prepos);
#else
      while ((dir < 0) ? pos-- : ++pos < rl_end);
#endif
    }
  return 0;
}

/* Search COUNT times for a character read from the current input stream.
   FDIR is the direction to search if COUNT is non-negative; otherwise
   the search goes in BDIR.  So much is dependent on HANDLE_MULTIBYTE
   that there are two separate versions of this function. */
#if defined(HANDLE_MULTIBYTE)
int
Readline::_rl_char_search (int count, int fdir, int bdir)
{
  char mbchar[MB_LEN_MAX];
  int mb_len;

  mb_len = _rl_read_mbchar (mbchar, MB_LEN_MAX);

  if (mb_len <= 0)
    return 1;

  if (count < 0)
    return _rl_char_search_internal (-count, bdir, mbchar,
                                     static_cast<unsigned int> (mb_len));
  else
    return _rl_char_search_internal (count, fdir, mbchar,
                                     static_cast<unsigned int> (mb_len));
}
#else  /* !HANDLE_MULTIBYTE */
static int
_rl_char_search (int count, int fdir, int bdir)
{
  int c;

  c = _rl_bracketed_read_key ();
  if (c < 0)
    return 1;

  if (count < 0)
    return _rl_char_search_internal (-count, bdir, c);
  else
    return _rl_char_search_internal (count, fdir, c);
}
#endif /* !HANDLE_MULTIBYTE */

#if defined(READLINE_CALLBACKS)
int
Readline::_rl_char_search_callback (_rl_callback_generic_arg *data)
{
  _rl_callback_func = nullptr;
  _rl_want_redisplay = true;

  return _rl_char_search (data->count, data->i1, data->i2);
}
#endif

int
Readline::rl_char_search (int count, int)
{
#if defined(READLINE_CALLBACKS)
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      _rl_callback_data = new _rl_callback_generic_arg (count);
      _rl_callback_data->i1 = FFIND;
      _rl_callback_data->i2 = BFIND;
      _rl_callback_func = &Readline::_rl_char_search_callback;
      return 0;
    }
#endif

  return _rl_char_search (count, FFIND, BFIND);
}

int
Readline::rl_backward_char_search (int count, int)
{
#if defined(READLINE_CALLBACKS)
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      _rl_callback_data = new _rl_callback_generic_arg (count);
      _rl_callback_data->i1 = BFIND;
      _rl_callback_data->i2 = FFIND;
      _rl_callback_func = &Readline::_rl_char_search_callback;
      return 0;
    }
#endif

  return _rl_char_search (count, BFIND, FFIND);
}

/* **************************************************************** */
/*								    */
/*		   The Mark and the Region.			    */
/*								    */
/* **************************************************************** */

/* A bindable command to set the mark. */
int
Readline::rl_set_mark (int count, int)
{
  return _rl_set_mark_at_pos (
      rl_explicit_arg ? static_cast<unsigned int> (count) : rl_point);
}

/* Exchange the position of mark and point. */
int
Readline::rl_exchange_point_and_mark (int, int)
{
  std::swap (rl_point, rl_mark);
  rl_activate_mark ();
  return 0;
}

} // namespace readline
