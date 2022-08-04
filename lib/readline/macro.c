/* macro.c -- keyboard macros for readline. */

/* Copyright (C) 1994-2009,2017 Free Software Foundation, Inc.

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

#include <sys/types.h>

namespace readline
{

#define MAX_MACRO_LEVEL 16

/* **************************************************************** */
/*								    */
/*			Hacking Keyboard Macros 		    */
/*								    */
/* **************************************************************** */

/* Set up to read subsequent input from STRING.
   STRING is deleted when we are done with it. */
void
Readline::_rl_with_macro_input (char *string)
{
  if (rl_macro_level > MAX_MACRO_LEVEL)
    {
      _rl_errmsg ("maximum macro execution nesting level exceeded");
      _rl_abort_internal ();
      return;
    }

  _rl_push_executing_macro ();
  rl_executing_macro = string;
  executing_macro_index = 0;
  RL_SETSTATE(RL_STATE_MACROINPUT);
}

/* Return the next character available from a macro, or 0 if
   there are no macro characters. */
int
Readline::_rl_next_macro_key ()
{
  int c;

  if (rl_executing_macro == nullptr)
    return 0;

  if (rl_executing_macro[executing_macro_index] == '\0')
    {
      _rl_pop_executing_macro ();
      return _rl_next_macro_key ();
    }

#if defined (READLINE_CALLBACKS)
  c = rl_executing_macro[executing_macro_index++];
  if (RL_ISSTATE (RL_STATE_CALLBACK) &&
      RL_ISSTATE (RL_STATE_READCMD | RL_STATE_MOREINPUT) &&
      rl_executing_macro[executing_macro_index] == 0)
      _rl_pop_executing_macro ();
  return c;
#else
  /* XXX - consider doing the same as the callback code, just not testing
     whether we're running in callback mode */
  return rl_executing_macro[executing_macro_index++];
#endif
}

int
Readline::_rl_peek_macro_key ()
{
  if (rl_executing_macro == nullptr)
    return 0;

  if (rl_executing_macro[executing_macro_index] == '\0' &&
      (rl_macro_list == nullptr || rl_macro_list->string == nullptr))
    return 0;

  if (rl_executing_macro[executing_macro_index] == 0 &&
      rl_macro_list && rl_macro_list->string)
    return rl_macro_list->string[0];

  return rl_executing_macro[executing_macro_index];
}

int
Readline::_rl_prev_macro_key ()
{
  if (rl_executing_macro == nullptr)
    return 0;

  if (executing_macro_index == 0)
    return 0;

  executing_macro_index--;
  return rl_executing_macro[executing_macro_index];
}

/* Save the currently executing macro on a stack of saved macros. */
void
Readline::_rl_push_executing_macro ()
{
  saved_macro *saver = new saved_macro ();
  saver->next = rl_macro_list;
  saver->sindex = executing_macro_index;
  saver->string = rl_executing_macro;

  rl_macro_list = saver;

  rl_macro_level++;
}

/* Discard the current macro, replacing it with the one
   on the top of the stack of saved macros. */
void
Readline::_rl_pop_executing_macro ()
{
  delete[] rl_executing_macro;
  rl_executing_macro = nullptr;
  executing_macro_index = 0;

  if (rl_macro_list)
    {
      saved_macro *macro = rl_macro_list;
      rl_executing_macro = rl_macro_list->string;
      executing_macro_index = rl_macro_list->sindex;
      rl_macro_list = rl_macro_list->next;
      delete macro;
    }

  rl_macro_level--;

  if (rl_executing_macro == nullptr)
    RL_UNSETSTATE(RL_STATE_MACROINPUT);
}

void
Readline::_rl_kill_kbd_macro ()
{
  rl_current_macro.clear ();

  delete[] rl_executing_macro;
  rl_executing_macro = nullptr;
  executing_macro_index = 0;

  RL_UNSETSTATE(RL_STATE_MACRODEF);
}

/* Begin defining a keyboard macro.
   Keystrokes are recorded as they are executed.
   End the definition with rl_end_kbd_macro ().
   If a numeric argument was explicitly typed, then append this
   definition to the end of the existing macro, and start by
   re-executing the existing macro. */
int
Readline::rl_start_kbd_macro (int, int)
{
  if (RL_ISSTATE (RL_STATE_MACRODEF))
    {
      _rl_abort_internal ();
      return 1;
    }

  if (rl_explicit_arg)
    {
      if (!rl_current_macro.empty ())
	_rl_with_macro_input (savestring (rl_current_macro));
    }
  else
    rl_current_macro.clear ();

  RL_SETSTATE(RL_STATE_MACRODEF);
  return 0;
}

/* Stop defining a keyboard macro.
   A numeric argument says to execute the macro right now,
   that many times, counting the definition as the first time. */
int
Readline::rl_end_kbd_macro (int count, int)
{
  if (RL_ISSTATE (RL_STATE_MACRODEF) == 0)
    {
      _rl_abort_internal ();
      return 1;
    }

  rl_current_macro.erase (rl_current_macro.size () - rl_executing_keyseq.size ());

  RL_UNSETSTATE(RL_STATE_MACRODEF);

  return rl_call_last_kbd_macro (--count, 0);
}

/* Execute the most recently defined keyboard macro.
   COUNT says how many times to execute it. */
int
Readline::rl_call_last_kbd_macro (int count, int)
{
  if (rl_current_macro.empty ())
    _rl_abort_internal ();

  if (RL_ISSTATE (RL_STATE_MACRODEF))
    {
      rl_ding ();		/* no recursive macros */
      // erase the last character
      rl_current_macro.erase (rl_current_macro.size () - 1, 1);
      return 0;
    }

  while (count--)
    _rl_with_macro_input (savestring (rl_current_macro));

  return 0;
}

int
Readline::rl_print_last_kbd_macro (int, int)
{
  char *m;

  if (rl_current_macro.empty ())
    {
      rl_ding ();
      return 0;
    }
  m = _rl_untranslate_macro_value (rl_current_macro.c_str (), 1);
  rl_crlf ();
  std::printf ("%s", m);
  std::fflush (stdout);
  rl_crlf ();
  delete[] m;
  rl_forced_update_display ();
  rl_display_fixed = true;

  return 0;
}

}  // namespace readline
