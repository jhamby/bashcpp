/* misc.c -- miscellaneous bindable readline functions. */

/* Copyright (C) 1987-2019 Free Software Foundation, Inc.

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

#include "readline.h"
#include "history.h"
#include "rlprivate.h"

namespace readline
{

/* **************************************************************** */
/*								    */
/*			Numeric Arguments			    */
/*								    */
/* **************************************************************** */

int
Readline::_rl_arg_overflow ()
{
  if (rl_numeric_arg > 1000000)
    {
      _rl_argcxt = 0;
      rl_explicit_arg = rl_numeric_arg = 0;
      rl_ding ();
      rl_restore_prompt ();
      rl_clear_message ();
      RL_UNSETSTATE(RL_STATE_NUMERICARG);
      return 1;
    }
  return 0;
}

/* Process C as part of the current numeric argument.  Return -1 if the
   argument should be aborted, 0 if we should not read any more chars, and
   1 if we should continue to read chars. */
int
Readline::_rl_arg_dispatch (_rl_arg_cxt cxt, int c)
{
  int key, r;
  key = c;

  /* If we see a key bound to `universal-argument' after seeing digits,
      it ends the argument but is otherwise ignored. */
  if (c >= 0 && _rl_keymap[c].type == ISFUNC &&
      _rl_keymap[c].value.function == &Readline::rl_universal_argument)
    {
      if ((cxt & NUM_SAWDIGITS) == 0)
	{
	  rl_numeric_arg *= 4;
	  return 1;
	}
      else if (RL_ISSTATE (RL_STATE_CALLBACK))
        {
          _rl_argcxt |= NUM_READONE;
          return 0;	/* XXX */
        }
      else
	{
	  key = _rl_bracketed_read_key ();
	  rl_restore_prompt ();
	  rl_clear_message ();
	  RL_UNSETSTATE(RL_STATE_NUMERICARG);
	  if (key < 0)
	    return -1;
	  return _rl_dispatch (key, _rl_keymap);
	}
    }

  c = UNMETA (c);

  if (_rl_digit_p (c))
    {
      r = _rl_digit_value (c);
      rl_numeric_arg = rl_explicit_arg ? (rl_numeric_arg * 10) +  r : r;
      rl_explicit_arg = 1;
      _rl_argcxt |= NUM_SAWDIGITS;
    }
  else if (c == '-' && rl_explicit_arg == 0)
    {
      rl_numeric_arg = 1;
      _rl_argcxt |= NUM_SAWMINUS;
      rl_arg_sign = -1;
    }
  else
    {
      /* Make M-- command equivalent to M--1 command. */
      if ((_rl_argcxt & NUM_SAWMINUS) && rl_numeric_arg == 1 && rl_explicit_arg == 0)
	rl_explicit_arg = 1;
      rl_restore_prompt ();
      rl_clear_message ();
      RL_UNSETSTATE(RL_STATE_NUMERICARG);

      r = _rl_dispatch (key, _rl_keymap);
      if (RL_ISSTATE (RL_STATE_CALLBACK))
	{
	  /* At worst, this will cause an extra redisplay.  Otherwise,
	     we have to wait until the next character comes in. */
	  if (rl_done == 0)
	    ((*this).*rl_redisplay_function) ();
	  r = 0;
	}
      return r;
    }

  return 1;
}

/* Handle C-u style numeric args, as well as M--, and M-digits. */
int
Readline::rl_digit_loop ()
{
  while (1)
    {
      if (_rl_arg_overflow ())
	return 1;

      int c = _rl_arg_getchar ();

      if (c < 0)
	{
	  _rl_abort_internal ();
	  return -1;
	}

      int r = _rl_arg_dispatch (_rl_argcxt, c);
      if (r <= 0 || (RL_ISSTATE (RL_STATE_NUMERICARG) == 0))
	return r;
    }
}

/* Start a numeric argument with initial value KEY */
int
Readline::rl_digit_argument (int, int key)
{
  _rl_arg_init ();
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    {
      _rl_arg_dispatch (_rl_argcxt, key);
      rl_message ("(arg: %d) ", rl_arg_sign * rl_numeric_arg);
      return 0;
    }
  else
    {
      rl_execute_next (key);
      return rl_digit_loop ();
    }
}

/* C-u, universal argument.  Multiply the current argument by 4.
   Read a key.  If the key has nothing to do with arguments, then
   dispatch on it.  If the key is the abort character then abort. */
int
Readline::rl_universal_argument (int, int)
{
  _rl_arg_init ();
  rl_numeric_arg *= 4;

  return RL_ISSTATE (RL_STATE_CALLBACK) ? 0 : rl_digit_loop ();
}

int
Readline::_rl_arg_callback (_rl_arg_cxt cxt)
{
  int c, r;

  c = _rl_arg_getchar ();
  if (c < 0)
    return 1;		/* EOF */

  if (_rl_argcxt & NUM_READONE)
    {
      _rl_argcxt &= ~NUM_READONE;
      rl_restore_prompt ();
      rl_clear_message ();
      RL_UNSETSTATE(RL_STATE_NUMERICARG);
      rl_execute_next (c);
      return 0;
    }

  r = _rl_arg_dispatch (cxt, c);
  if (r > 0)
    rl_message ("(arg: %d) ", rl_arg_sign * rl_numeric_arg);
  return r != 1;
}

/* **************************************************************** */
/*								    */
/*			History Utilities			    */
/*								    */
/* **************************************************************** */

/* We already have a history library, and that is what we use to control
   the history features of readline.  This is our local interface to
   the history mechanism. */

/* Restore the _rl_saved_line_for_history if there is one. */
void
Readline::rl_maybe_unsave_line ()
{
  if (_rl_saved_line_for_history)
    {
      /* Can't call with `1' because rl_undo_list might point to an undo
	 list from a history entry, as in rl_replace_from_history() below. */
      rl_replace_line (_rl_saved_line_for_history->line, 0);
      rl_undo_list = dynamic_cast<UNDO_LIST *> (_rl_saved_line_for_history->data);
      delete _rl_saved_line_for_history;
      _rl_saved_line_for_history = nullptr;
      rl_point = rl_end ();	/* rl_replace_line changes length */
    }
  else
    rl_ding ();
}

void
Readline::_rl_history_set_point ()
{
  rl_point = (_rl_history_preserve_point &&
	      _rl_history_saved_point != static_cast<unsigned int> (-1))
		? _rl_history_saved_point
		: rl_end ();
  if (rl_point > rl_end ())
    rl_point = rl_end ();

#if defined (VI_MODE)
  if (rl_editing_mode == vi_mode && _rl_keymap != vi_insertion_keymap ())
    rl_point = 0;
#endif /* VI_MODE */

  if (rl_editing_mode == emacs_mode)
    rl_mark = (rl_point == rl_end () ? 0 : rl_end ());
}

/* Process and free undo lists attached to each history entry prior to the
   current entry, inclusive, reverting each line to its saved state.  This
   is destructive, and state about the current line is lost.  This is not
   intended to be called while actively editing, and the current line is
   not assumed to have been added to the history list. */
void
Readline::_rl_revert_previous_lines ()
{
  std::string lbuf (rl_line_buffer);
  UNDO_LIST *saved_undo_list = rl_undo_list;
  unsigned int hpos = where_history ();

  HIST_ENTRY *entry = (hpos == static_cast<unsigned int> (the_history.size ()))
			? previous_history () : current_history ();
  while (entry)
    {
      UNDO_LIST *ul;
      if ((ul = dynamic_cast<UNDO_LIST *> (entry->data)))
	{
	  if (ul == saved_undo_list)
	    saved_undo_list = nullptr;
	  /* Set up rl_line_buffer and other variables from history entry */
	  rl_replace_from_history (entry);	/* entry->line is now current */
	  entry->data = nullptr;		/* entry->data is now current undo list */
	  /* Undo all changes to this history entry */
	  while (rl_undo_list)
	    rl_do_undo ();
	  /* And copy the reverted line back to the history entry, preserving
	     the timestamp. */
	  entry->line = rl_line_buffer;
	}
      entry = previous_history ();
    }

  /* Restore history state */
  rl_undo_list = saved_undo_list;	/* may have been set to null */
  history_set_pos (hpos);

  /* reset the line buffer */
  rl_replace_line (lbuf, 0);
}

/* Free the history list, including private readline data and take care
   of pointer aliases to history data.  Resets rl_undo_list if it points
   to an UNDO_LIST * saved as some history entry's data member.  This
   should not be called while editing is active. */
void
Readline::rl_clear_history ()
{
  UNDO_LIST *ul, *saved_undo_list;

  saved_undo_list = rl_undo_list;
  std::vector<HIST_ENTRY*> &hlist = the_history;	/* reference, not copy */

  for (std::vector<HIST_ENTRY*>::iterator it = hlist.begin ();
       it != hlist.end (); ++it)
    {
      if ((ul = dynamic_cast<UNDO_LIST *> ((*it)->data)))
	{
	  if (ul == saved_undo_list)
	    saved_undo_list = nullptr;
	  delete ul;
	  (*it)->data = nullptr;
	}
      delete *it;
    }

  hlist.clear ();
  rl_undo_list = saved_undo_list;	/* should be NULL */
}

/* **************************************************************** */
/*								    */
/*			History Commands			    */
/*								    */
/* **************************************************************** */

/* Meta-< goes to the start of the history. */
int
Readline::rl_beginning_of_history (int, int key)
{
  return rl_get_previous_history (1 + static_cast<int> (where_history ()), key);
}

/* Meta-> goes to the end of the history.  (The current line). */
int
Readline::rl_end_of_history (int, int)
{
  rl_maybe_replace_line ();
  using_history ();
  rl_maybe_unsave_line ();
  return 0;
}

/* Move down to the next history line. */
int
Readline::rl_get_next_history (int count, int key)
{
  if (count < 0)
    return rl_get_previous_history (-count, key);

  if (count == 0)
    return 0;

  rl_maybe_replace_line ();

  /* either not saved by rl_newline or at end of line, so set appropriately. */
  size_t rl_end = rl_line_buffer.size ();
  if (_rl_history_saved_point == static_cast<unsigned int> (-1) && (rl_point || rl_end))
    _rl_history_saved_point = (rl_point == rl_end) ? static_cast<unsigned int> (-1) : rl_point;

  HIST_ENTRY *temp = nullptr;
  while (count)
    {
      temp = next_history ();
      if (!temp)
	break;
      --count;
    }

  if (temp == nullptr)
    rl_maybe_unsave_line ();
  else
    {
      rl_replace_from_history (temp);
      _rl_history_set_point ();
    }
  return 0;
}

/* Get the previous item out of our interactive history, making it the current
   line.  If there is no previous history, just ding. */
int
Readline::rl_get_previous_history (int count, int key)
{
  HIST_ENTRY *old_temp, *temp;

  if (count < 0)
    return rl_get_next_history (-count, key);

  if (count == 0 || the_history.empty ())
    return 0;

  /* either not saved by rl_newline or at end of line, so set appropriately. */
  size_t rl_end = rl_line_buffer.size ();
  if (_rl_history_saved_point == static_cast<unsigned int> (-1) && (rl_point || rl_end))
    _rl_history_saved_point = (rl_point == rl_end) ? static_cast<unsigned int> (-1) : rl_point;

  /* If we don't have a line saved, then save this one. */
  bool had_saved_line = (_rl_saved_line_for_history != nullptr);
  rl_maybe_save_line ();

  /* If the current line has changed, save the changes. */
  rl_maybe_replace_line ();

  temp = old_temp = nullptr;
  while (count)
    {
      temp = previous_history ();
      if (temp == nullptr)
	break;

      old_temp = temp;
      --count;
    }

  /* If there was a large argument, and we moved back to the start of the
     history, that is not an error.  So use the last value found. */
  if (!temp && old_temp)
    temp = old_temp;

  if (temp == nullptr)
    {
      if (!had_saved_line)
        _rl_free_saved_history_line ();
      rl_ding ();
    }
  else
    {
      rl_replace_from_history (temp);
      _rl_history_set_point ();
    }

  return 0;
}

/* The equivalent of the Korn shell C-o operate-and-get-next-history-line
   editing command. */

int
Readline::set_saved_history ()
{
  if (saved_history_logical_offset != static_cast<unsigned int> (-1))
    {
      int count = static_cast<int> (where_history ()) -
		  static_cast<int> (saved_history_logical_offset);
      rl_get_previous_history (count, 0);
    }
  saved_history_logical_offset = static_cast<unsigned int> (-1);
  _rl_internal_startup_hook = _rl_saved_internal_startup_hook;

  return 0;
}

int
Readline::rl_operate_and_get_next (int count, int c)
{
  /* Accept the current line. */
  rl_newline (1, c);

  saved_history_logical_offset = rl_explicit_arg ? static_cast<unsigned int> (count)
						 : (where_history () + 1);

  _rl_saved_internal_startup_hook = _rl_internal_startup_hook;
  _rl_internal_startup_hook = &Readline::set_saved_history;

  return 0;
}

/* **************************************************************** */
/*								    */
/*			    Editing Modes			    */
/*								    */
/* **************************************************************** */
/* How to toggle back and forth between editing modes. */
int
Readline::rl_vi_editing_mode (int, int key)
{
#if defined (VI_MODE)
  _rl_set_insert_mode (RL_IM_INSERT, 1);	/* vi mode ignores insert mode */
  rl_editing_mode = vi_mode;
  rl_vi_insert_mode (1, key);
#endif /* VI_MODE */

  return 0;
}

int
Readline::rl_emacs_editing_mode (int, int)
{
  rl_editing_mode = emacs_mode;
  _rl_set_insert_mode (RL_IM_INSERT, 1); /* emacs mode default is insert mode */
  _rl_keymap = emacs_standard_keymap ();

  if (_rl_show_mode_in_prompt)
    _rl_reset_prompt ();

  return 0;
}

/* Toggle overwrite mode.  A positive explicit argument selects overwrite
   mode.  A negative or zero explicit argument selects insert mode. */
int
Readline::rl_overwrite_mode (int count, int)
{
  if (rl_explicit_arg == 0)
    _rl_set_insert_mode (!rl_insert_mode, 0);
  else if (count > 0)
    _rl_set_insert_mode (RL_IM_OVERWRITE, 0);
  else
    _rl_set_insert_mode (RL_IM_INSERT, 0);

  return 0;
}

}  // namespace readline
