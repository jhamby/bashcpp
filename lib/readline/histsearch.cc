/* histsearch.c -- searching the history list. */

/* Copyright (C) 1989, 1992-2009,2017 Free Software Foundation, Inc.

   This file contains the GNU History Library (History), a set of
   routines for managing the text of previously typed lines.

   History is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   History is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with History.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "config.hh"

#include "history.hh"

#if defined (HAVE_FNMATCH)
#  include <fnmatch.h>
#endif

namespace readline
{

/* Search the history for STRING, starting at history_offset.
   If DIRECTION < 0, then the search is through previous entries, else
   through subsequent.  If ANCHORED is non-zero, the string must
   appear at the beginning of a history line, otherwise, the string
   may appear anywhere in the line.  If the string is found, then
   current_history () is the history entry, and the value of this
   function is the offset in the line of that history entry that the
   string was found in.  Otherwise, nothing is changed, and a -1 is
   returned. */

unsigned int
History::history_search_internal (const std::string &string, int direction, hist_search_flags flags)
{
  int i = static_cast<int> (where_history ());
  bool reverse = (direction < 0);
  int anchored = (flags & ANCHORED_SEARCH);
#if defined (HAVE_FNMATCH)
  bool patsearch = (flags & PATTERN_SEARCH);
#else
  bool patsearch = false;
#endif

  /* Take care of trivial cases first. */
  if (string.empty ())
    return static_cast<unsigned int> (-1);

  if (!history_length () || ((i >= static_cast<int> (history_length ())) && !reverse))
    return static_cast<unsigned int> (-1);

#define NEXT_LINE() do { if (reverse) i--; else i++; } while (0)

  while (1)
    {
      /* Search each line in the history list for STRING. */

      /* At limit for direction? */
      if ((reverse && i < 0) || (!reverse && i == static_cast<int> (history_length ())))
	return static_cast<unsigned int> (-1);

      HIST_ENTRY *entry = get_history (static_cast<unsigned int> (i));
      const char *line = entry->line.c_str ();
      int line_index = static_cast<int> (entry->line.size ());

      /* If STRING is longer than line, no match. */
      if (!patsearch && (static_cast<int> (string.size ()) > line_index))
	{
	  NEXT_LINE ();
	  continue;
	}

      /* Handle anchored searches first. */
      if (anchored == ANCHORED_SEARCH)
	{
#if defined (HAVE_FNMATCH)
	  if (patsearch)
	    {
	      if (::fnmatch (string.c_str (), line, 0) == 0)
		{
		  history_offset = static_cast<unsigned int> (i);
		  return 0;
		}
	    }
	  else
#endif
	  if (STREQN (string.c_str (), line, string.size ()))
	    {
	      history_offset = static_cast<unsigned int> (i);
	      return 0;
	    }

	  NEXT_LINE ();
	  continue;
	}

      /* Do substring search. */
      if (reverse)
	{
	  line_index -= (!patsearch) ? string.size () : 1;

	  while (line_index >= 0)
	    {
#if defined (HAVE_FNMATCH)
	      if (patsearch)
		{
		  if (::fnmatch (string.c_str (), line + line_index, 0) == 0)
		    {
		      history_offset = static_cast<unsigned int> (i);
		      return static_cast<unsigned int> (line_index);
		    }
		}
	      else
#endif
	      if (STREQN (string.c_str (), line + line_index, string.size ()))
		{
		  history_offset = static_cast<unsigned int> (i);
		  return static_cast<unsigned int> (line_index);
		}
	      line_index--;
	    }
	}
      else
	{
	  int limit;

	  limit = line_index - static_cast<int> (string.size ()) + 1;
	  line_index = 0;

	  while (line_index < limit)
	    {
#if defined (HAVE_FNMATCH)
	      if (patsearch)
		{
		  if (::fnmatch (string.c_str (), line + line_index, 0) == 0)
		    {
		      history_offset = static_cast<unsigned int> (i);
		      return static_cast<unsigned int> (line_index);
		    }
		}
	      else
#endif
	      if (STREQN (string.c_str (), line + line_index, string.size ()))
		{
		  history_offset = static_cast<unsigned int> (i);
		  return static_cast<unsigned int> (line_index);
		}
	      line_index++;
	    }
	}
      NEXT_LINE ();
    }
}

unsigned int
History::_hs_history_patsearch (const char *string, int direction, hist_search_flags flags)
{
  size_t len, start;
  unsigned int ret;
  bool unescaped_backslash;

#if defined (HAVE_FNMATCH)
  /* Assume that the string passed does not have a leading `^' and any
     anchored search request is captured in FLAGS */
  len = std::strlen (string);
  ret = static_cast<unsigned int> (len - 1);
  /* fnmatch is required to reject a pattern that ends with an unescaped
     backslash */
  if ((unescaped_backslash = (string[ret] == '\\')))
    {
      while (ret > 0 && string[--ret] == '\\')
	unescaped_backslash = !unescaped_backslash;
    }
  if (unescaped_backslash)
    return static_cast<unsigned int> (-1);

  char *pat = new char[len + 3];
  /* If the search string is not anchored, we'll be calling fnmatch (assuming
     we have it). Prefix a `*' to the front of the search string so we search
     anywhere in the line. */
  if ((flags & ANCHORED_SEARCH) == 0 && string[0] != '*')
    {
      pat[0] = '*';
      start = 1;
      len++;
    }
  else
    {
      start = 0;
    }

  /* Attempt to reduce the number of searches by tacking a `*' onto the end
     of a pattern that doesn't have one.  Assume a pattern that ends in a
     backslash contains an even number of trailing backslashes; we check
     above */
  std::strcpy (pat + start, string);
  if (pat[len - 1] != '*')
    {
      pat[len] = '*';		/* XXX */
      pat[len+1] = '\0';
    }
#else
  pat = const_cast<char *> (string);
#endif

  ret = history_search_internal (pat, direction, static_cast<hist_search_flags> (flags | PATTERN_SEARCH));

  if (pat != string)
    delete[] pat;
  return ret;
}

/* Search for STRING in the history list.  DIR is < 0 for searching
   backwards.  POS is an absolute index into the history list at
   which point to begin searching. */
unsigned int
History::history_search_pos (const std::string &string, int dir, unsigned int pos)
{
  unsigned int ret, old;

  old = where_history ();
  history_set_pos (pos);
  if (history_search (string, dir) == static_cast<unsigned int> (-1))
    {
      history_set_pos (old);
      return static_cast<unsigned int> (-1);
    }
  ret = where_history ();
  history_set_pos (old);
  return ret;
}

}  // namespace readline
