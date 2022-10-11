/* stringvec.cc - functions for managing C++ vectors of C strings. */

/* Copyright (C) 2000-2002 Free Software Foundation, Inc.
   Copyright 2022, Jake Hamby.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "shell.hh"

namespace bash
{

// Cons up a new string vector. The words are taken from LIST, which is a
// WORD_LIST *. Everything is new[]'d, so you should delete[] everything in
// this array when you are done. STARTING_INDEX says where to start filling in
// the returned list; it can be used to reserve space at the beginning.
STRINGLIST *
strlist_from_word_list (const WORD_LIST *list, size_t starting_index)
{
  size_t count = list->size ();

  STRINGLIST *sl = new STRINGLIST (count + starting_index);

  STRINGLIST::iterator it;
  for (it = sl->begin () + static_cast<ssize_t> (starting_index); list;
       ++it, list = list->next ())
    *it = savestring (list->word->word);

  return sl;
}

/* Convert a vector of strings into the form used internally by the shell.
   STARTING_INDEX says where in ARRAY to begin. */
WORD_LIST *
strlist_to_word_list (const STRINGLIST *sl, size_t starting_index)
{
  if (sl == nullptr)
    return nullptr;

  STRINGLIST::const_reverse_iterator it;
  WORD_LIST *w = nullptr;
  for (it = sl->rbegin ();
       it != sl->rend () + static_cast<ssize_t> (starting_index); ++it)
    {
      w = new WORD_LIST (new WORD_DESC (*it), w);
      w = w->next ();
    }

  return w; // the list will be in forward order
}

} // namespace bash
