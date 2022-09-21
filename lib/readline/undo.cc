/* undo.c - manage list of changes to lines, offering opportunity to undo them
 */

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

namespace readline
{

/* **************************************************************** */
/*								    */
/*			Undo, and Undoing			    */
/*								    */
/* **************************************************************** */

// Virtual destructor to free undo entries.
UNDO_LIST::~UNDO_LIST () {}

/* Undo the next thing in the list.  Return 0 if there
   is nothing to undo, or non-zero if there was. */
int
Readline::rl_do_undo ()
{
  int waiting_for_begin;
  size_t start, end;

#define TRANS(i)                                                              \
  ((i) == static_cast<size_t> (-1)                                            \
       ? rl_point                                                             \
       : ((i) == static_cast<size_t> (-2) ? rl_end () : (i)))

  start = end = waiting_for_begin = 0;
  do
    {
      if (!rl_undo_list)
        return 0;

      _rl_doing_an_undo = true;
      RL_SETSTATE (RL_STATE_UNDOING);

      UNDO_ENTRY entry = rl_undo_list->back ();

      /* To better support vi-mode, a start or end value of -1 means
         rl_point, and a value of -2 means rl_end. */
      if (entry.what == UNDO_DELETE || entry.what == UNDO_INSERT)
        {
          start = TRANS (entry.start);
          end = TRANS (entry.end);
        }

      switch (entry.what)
        {
        /* Undoing deletes means inserting some text. */
        case UNDO_DELETE:
          rl_point = start;
          _rl_fix_point (1);
          rl_insert_text (entry.text);
          entry.text.clear ();
          break;

        /* Undoing inserts means deleting some text. */
        case UNDO_INSERT:
          rl_delete_text (start, end);
          rl_point = start;
          _rl_fix_point (1);
          break;

        /* Undoing an END means undoing everything 'til we get to a BEGIN. */
        case UNDO_END:
          waiting_for_begin++;
          break;

        /* Undoing a BEGIN means that we are done with this group. */
        case UNDO_BEGIN:
          if (waiting_for_begin)
            waiting_for_begin--;
          else
            rl_ding ();
          break;
        }

      _rl_doing_an_undo = false;
      RL_UNSETSTATE (RL_STATE_UNDOING);

      rl_undo_list->pop_back ();
    }
  while (waiting_for_begin);

  return 1;
}
#undef TRANS

/* Save an undo entry for the text from START to END. */
void
Readline::rl_modifying (size_t start, size_t end)
{
  if (start > end)
    {
      std::swap (start, end);
    }

  if (start != end)
    {
      char *temp = rl_copy_text (start, end);
      rl_begin_undo_group ();
      rl_add_undo (UNDO_DELETE, start, end, temp);
      rl_add_undo (UNDO_INSERT, start, end);
      rl_end_undo_group ();
    }
}

/* Revert the current line to its previous state. */
int
Readline::rl_revert_line (int, int)
{
  if (!rl_undo_list)
    rl_ding ();
  else
    {
      while (rl_undo_list)
        rl_do_undo ();

#if defined(VI_MODE)
      if (rl_editing_mode == vi_mode)
        rl_point = rl_mark = 0;
#endif
    }
  return 0;
}

/* Do some undoing of things that were done. */
int
Readline::rl_undo_command (int count, int)
{
  if (count < 0)
    return 0; /* Nothing to do. */

  while (count)
    {
      if (rl_do_undo ())
        count--;
      else
        {
          rl_ding ();
          break;
        }
    }
  return 0;
}

} // namespace readline
