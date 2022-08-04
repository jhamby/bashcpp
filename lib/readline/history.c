/* history.c -- standalone history library */

/* Copyright (C) 1989-2017 Free Software Foundation, Inc.

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

/* The goal is to make the implementation transparent, so that you
   don't have to know what data types are used, just what functions
   you can call.  I think I have done that. */

#include "history.h"

namespace readline
{

/* **************************************************************** */
/*								    */
/*			History Functions			    */
/*								    */
/* **************************************************************** */

/* Destructor to delete the allocated history entries. */
History::~History ()
{
  for (std::vector<HIST_ENTRY*>::iterator it = the_history.begin ();
       it != the_history.end (); ++it)
  {
    delete *it;
  }
}

/* empty virtual destructor for hist_data. */
hist_data::~hist_data () {}

time_t
History::history_get_time (const HIST_ENTRY *hist)
{
  const char *ts = hist->timestamp.c_str ();

  if (ts[0] != history_comment_char)
    return 0;

  errno = 0;

  time_t t = static_cast<time_t> (std::strtol (ts + 1, nullptr, 10));
  if (errno == ERANGE)
    return 0;

  return t;
}

char *
History::hist_inittime ()
{
  char ts[32], *ret;

  time_t t = ::time (nullptr);
#if defined (HAVE_VSNPRINTF)		/* assume snprintf if vsnprintf exists */
  std::snprintf (ts, sizeof (ts) - 1, "X%lu", static_cast<unsigned long> (t));
#else
  std::sprintf (ts, "X%lu", static_cast<unsigned long> (t));
#endif
  ret = savestring (ts);
  ret[0] = history_comment_char;

  return ret;
}

/* Place STRING at the end of the history list in a new HIST_ENTRY. */
void
History::add_history (const char *string)
{
  HIST_ENTRY *temp = new HIST_ENTRY (string, hist_inittime ());

  if (history_stifled && (history_length () == history_max_entries))
    {
      /* If the history is stifled, and history_length is zero,
	 and it equals history_max_entries, we don't save items. */
      if (the_history.empty ())
	return;

      /* Insert into the ring buffer, deleting oldest entry. */
      the_history[history_index_end++] = temp;

      if (history_index_end == history_length ())
	history_index_end = 0;

      ++history_base;
    }
  else
    {
      the_history.push_back(temp);
    }

  history_index_end++;
}

/* Change the time stamp of the most recent history entry to STRING. */
void
History::add_history_time (const char *string)
{
  HIST_ENTRY *hs;

  if (string == nullptr || the_history.empty ())
    return;

  hs = the_history.back();
  hs->timestamp = savestring (string);
}

/* Replace the history entry at index WHICH with new entry ENTRY. Returns
   the old entry so you can dispose of the data.  In the case of an
   invalid WHICH, nullptr is returned. */
HIST_ENTRY *
History::replace_history_entry (unsigned int which, const std::string &line, histdata_t data)
{
  HIST_ENTRY *temp, *old_value;

  if (which >= history_length ())
    return nullptr;

  old_value = the_history[which];
  temp = new HIST_ENTRY (line, old_value->timestamp, data);
  the_history[which] = temp;

  return old_value;
}

/* Append LINE to the history line at offset WHICH, adding a newline to the
   end of the current line first.  This can be used to construct multi-line
   history entries while reading lines from the history file. */
void
History::_hs_append_history_line (unsigned int which, const char *line)
{
  HIST_ENTRY *hent = the_history[which];
  hent->line.push_back ('\n');
  hent->line.append (line);
}

/* Replace the DATA in the specified history entries, replacing OLD with
   NEW.  WHICH says which one(s) to replace:  WHICH == -1 means to replace
   all of the history entries where entry->data == OLD; WHICH == -2 means
   to replace the `newest' history entry where entry->data == OLD; and
   WHICH >= 0 means to replace that particular history entry's data, as
   long as it matches OLD. */
void
History::_hs_replace_history_data (unsigned int which, histdata_t old, histdata_t new_)
{
  if (which < static_cast<unsigned int> (-2) || which >= the_history.size () ||
      the_history.empty ())
    return;

  if (static_cast<int> (which) >= 0)
    {
      HIST_ENTRY *entry = the_history[which];
      if (entry && entry->data == old)
	entry->data = new_;
      return;
    }

  int last = -1;
  for (unsigned int i = 0; i < history_length (); i++)
    {
      HIST_ENTRY *entry = the_history[i];
      if (entry == nullptr)
	continue;
      if (entry->data == old)
	{
	  last = static_cast<int> (i);
	  if (which == static_cast<unsigned int> (-1))
	    entry->data = new_;
	}
    }
  if (static_cast<int> (which) == -2 && last >= 0)
    {
      HIST_ENTRY *entry = the_history[static_cast<unsigned int> (last)];
      entry->data = new_;	/* XXX - we don't check entry->old */
    }
}

/* Remove history element WHICH from the history.  The removed
   element is returned to you so you can reuse or delete it. */
HIST_ENTRY *
History::remove_history (unsigned int which)
{
  if (which >= history_length () || the_history.empty ())
    return nullptr;

  HIST_ENTRY *return_value = the_history[which];

  the_history.erase (the_history.begin () + which);

  return return_value;
}

HIST_ENTRY **
History::remove_history_range (unsigned int first, unsigned int last)
{
  if (history_length () == 0)
    return nullptr;

  if (first >= history_length () || last >= history_length ())
    return nullptr;

  if (first > last)
    return nullptr;

  size_t nentries = last - first + 1;
  HIST_ENTRY **return_value = new HIST_ENTRY*[(nentries + 1) * sizeof (HIST_ENTRY *)];

  /* Return all the deleted entries in a list */
  unsigned int i;
  for (i = first ; i <= last; i++)
    return_value[i - first] = the_history[i];
  return_value[i - first] = nullptr;

  /* Erase the just-removed entries.  */
  the_history.erase (the_history.begin () + first, the_history.begin () + last + 1);

  return return_value;
}

/* Stifle the history list, remembering only MAX number of lines. */
void
History::stifle_history (unsigned int max)
{
  // Rearrange history if we were already in ring buffer mode and changed size.
  if (history_stifled && max != history_max_entries)
    // We also don't have to do anything if we can grow from the end.
    if (max < the_history.size () || history_index_end != the_history.size ())
      {
	// If the new length is longer, insert blank entries to expand into.
	if (max > the_history.size ())
	  {
	    the_history.insert (the_history.begin () + history_index_end,
				static_cast<size_t> (max - history_max_entries), nullptr);
	  }
	else
	  {
	    // Delete the oldest entries and increase the base index.
	    ssize_t diff = static_cast<ssize_t> (the_history.size () - max);
	    ssize_t diff1 = std::min (diff, static_cast<ssize_t> (the_history.size () - history_index_end));

	    for (std::vector<HIST_ENTRY *>::iterator it = the_history.begin () + history_index_end;
		 it != the_history.begin () + history_index_end + diff1; ++it)
	      delete *it;

	    the_history.erase (the_history.begin () + history_index_end,
			       the_history.begin () + history_index_end + diff1);
	    history_base += diff1;
	    diff -= diff1;

	    // We may need to remove entries from the left side as well.
	    if (diff)
	      {
		for (std::vector<HIST_ENTRY *>::iterator it = the_history.begin ();
		     it != the_history.begin () + diff; ++it)
		  delete *it;

		the_history.erase (the_history.begin (), the_history.begin () + diff);
		history_base += diff;
		history_index_end -= diff;
	      }
	  }
      }

  history_stifled = true;
  history_max_entries = max;
}

}  // namespace readline
