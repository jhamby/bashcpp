/* search.c - code for non-incremental searching in emacs and vi modes. */

/* Copyright (C) 1992-2020 Free Software Foundation, Inc.

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
#include "history.hh"
#include "rlprivate.hh"

#include <sys/types.h>

namespace readline
{

/* Make the data from the history entry ENTRY be the contents of the
   current line.  This doesn't do anything with rl_point; the caller
   must set it. */
void
Readline::make_history_line_current (HIST_ENTRY *entry)
{
  _rl_replace_text (entry->line, 0, rl_end ());
  _rl_fix_point (1);
#if defined (VI_MODE)
  if (rl_editing_mode == vi_mode)
    /* POSIX.2 says that the `U' command doesn't affect the copy of any
       command lines to the edit line.  We're going to implement that by
       making the undo list start after the matching line is copied to the
       current editing buffer. */
    rl_free_undo_list ();
#endif

  if (_rl_saved_line_for_history)
    delete _rl_saved_line_for_history;

  _rl_saved_line_for_history = nullptr;
}

/* Search the history list for STRING starting at absolute history position
   POS.  If STRING begins with `^', the search must match STRING at the
   beginning of a history line, otherwise a full substring match is performed
   for STRING.  DIR < 0 means to search backwards through the history list,
   DIR >= 0 means to search forward. */
unsigned int
Readline::noninc_search_from_pos (const char *string, unsigned int pos, int dir,
				  rl_search_flags flags, unsigned int *ncp)
{
  unsigned int ret;
  const char *s;

  unsigned int old = where_history ();
  if (history_set_pos (pos) == 0)
    return static_cast<unsigned int> (-1);

  RL_SETSTATE(RL_STATE_SEARCH);
  /* These functions return the match offset in the line; history_offset gives
     the matching line in the history list */
  if (flags & SF_PATTERN)
    {
      s = string;
      hist_search_flags sflags = NON_ANCHORED_SEARCH;
      if (*s == '^')
	{
	  sflags = ANCHORED_SEARCH;
	  s++;
	}
      ret = _hs_history_patsearch (s, dir, sflags);
    }
  else if (*string == '^')
    ret = history_search_prefix (string + 1, dir);
  else
    ret = history_search (string, dir);
  RL_UNSETSTATE(RL_STATE_SEARCH);

  // only update values on success
  if (ret != static_cast<unsigned int> (-1))
    {
      if (ncp)
	*ncp = ret;

      ret = where_history ();
    }

  history_set_pos (old);
  return ret;
}

/* Search for a line in the history containing STRING.  If DIR is < 0, the
   search is backwards through previous entries, else through subsequent
   entries.  Returns 1 if the search was successful, 0 otherwise. */
bool
Readline::noninc_dosearch (const char *string, int dir, rl_search_flags flags)
{
  if (string == nullptr || *string == '\0')
    {
      rl_ding ();
      return false;
    }

  unsigned int ind;
  unsigned int pos = noninc_search_from_pos (string, static_cast<unsigned int>
				    (static_cast<int> (noninc_history_pos) + dir),
				    dir, flags, &ind);
  if (pos == static_cast<unsigned int> (-1))
    {
      /* Search failed, current history position unchanged. */
      rl_maybe_unsave_line ();
      rl_clear_message ();
      rl_point = 0;
      rl_ding ();
      return false;
    }

  noninc_history_pos = static_cast<unsigned int> (pos);

  unsigned int oldpos = where_history ();
  history_set_pos (noninc_history_pos);
  HIST_ENTRY *entry = current_history ();  // will never be NULL after successful search

#if defined (VI_MODE)
  if (rl_editing_mode != vi_mode)
#endif
    history_set_pos (oldpos);

  make_history_line_current (entry);

  if (_rl_enable_active_region && ((flags & SF_PATTERN) == 0) &&
      ind > 0 && ind < rl_end ())
    {
      rl_point = ind;
      rl_mark = ind + static_cast<unsigned int> (std::strlen (string));
      if (rl_mark > rl_end ())
	rl_mark = rl_end ();	/* can't happen? */
      rl_activate_mark ();
    }
  else
    {
      rl_point = 0;
      rl_mark = rl_end ();
    }

  rl_clear_message ();
  return true;
}

Readline::_rl_search_cxt *
Readline::_rl_nsearch_init (int dir, char pchar)
{
  _rl_search_cxt *cxt;

  cxt = new _rl_search_cxt (RL_SEARCH_NSEARCH, SF_NONE, rl_point, rl_mark,
			    where_history (), _rl_keymap);

  if (dir < 0)
    /* not strictly needed */
    cxt->sflags |= SF_REVERSE;

#if defined (VI_MODE)
  if (VI_COMMAND_MODE() && (pchar == '?' || pchar == '/'))
    cxt->sflags |= SF_PATTERN;
#endif

  cxt->direction = dir;
  cxt->history_pos = cxt->save_line;

  rl_maybe_save_line ();

  /* Clear the undo list, since reading the search string should create its
     own undo list, and the whole list will end up being freed when we
     finish reading the search string. */
  rl_undo_list = nullptr;

  /* Use the line buffer to read the search string. */
  rl_line_buffer[0] = 0;
  rl_point = 0;

  char *p = _rl_make_prompt_for_search (pchar ? pchar : ':');
  rl_message ("%s", p);
  delete[] p;

  RL_SETSTATE(RL_STATE_NSEARCH);

  _rl_nscxt = cxt;

  return cxt;
}

int
Readline::_rl_nsearch_cleanup (_rl_search_cxt *cxt, int r)
{
  delete cxt;
  _rl_nscxt = nullptr;

  RL_UNSETSTATE(RL_STATE_NSEARCH);

  return r != 1;
}

void
Readline::_rl_nsearch_abort (_rl_search_cxt *cxt)
{
  rl_maybe_unsave_line ();
  rl_clear_message ();
  rl_point = cxt->save_point;
  rl_mark = cxt->save_mark;
  _rl_fix_point (1);
  rl_restore_prompt ();

  RL_UNSETSTATE (RL_STATE_NSEARCH);
}

/* Process just-read character C according to search context CXT.  Return -1
   if the caller should abort the search, 0 if we should break out of the
   loop, and 1 if we should continue to read characters. */
int
Readline::_rl_nsearch_dispatch (_rl_search_cxt *cxt, int c)
{
  int n;

  if (c < 0)
    c = CTRL ('C');

  switch (c)
    {
    case CTRL('W'):
      rl_unix_word_rubout (1, c);
      break;

    case CTRL('U'):
      rl_unix_line_discard (1, c);
      break;

    case RETURN:
    case NEWLINE:
      return 0;

    case CTRL('H'):
    case RUBOUT:
      if (rl_point == 0)
	{
	  _rl_nsearch_abort (cxt);
	  return -1;
	}
      _rl_rubout_char (1, c);
      break;

    case CTRL('C'):
    case CTRL('G'):
      rl_ding ();
      _rl_nsearch_abort (cxt);
      return -1;

    case ESC:
      /* XXX - experimental code to allow users to bracketed-paste into the
	 search string. Similar code is in isearch.c:_rl_isearch_dispatch().
	 The difference here is that the bracketed paste sometimes doesn't
	 paste everything, so checking for the prefix and the suffix in the
	 input queue doesn't work well. We just have to check to see if the
	 number of chars in the input queue is enough for the bracketed paste
	 prefix and hope for the best. */
      if (_rl_enable_bracketed_paste &&
	  ((n = _rl_nchars_available ()) >= (BRACK_PASTE_SLEN - 1)))
	{
	  if (_rl_read_bracketed_paste_prefix (c) == 1)
	    rl_bracketed_paste_begin (1, c);
	  else
	    {
	      c = rl_read_key ();	/* get the ESC that got pushed back */
	      _rl_insert_char (1, c);
	    }
        }
      else
        _rl_insert_char (1, c);
      break;

    default:
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
	rl_insert_text (cxt->mb);
      else
#endif
	_rl_insert_char (1, c);
      break;
    }

  ((*this).*rl_redisplay_function) ();
  rl_deactivate_mark ();
  return 1;
}

/* Perform one search according to CXT, using NONINC_SEARCH_STRING.  Return
   -1 if the search should be aborted, any other value means to clean up
   using _rl_nsearch_cleanup ().  Returns 1 if the search was successful,
   0 otherwise. */
int
Readline::_rl_nsearch_dosearch (_rl_search_cxt *cxt)
{
  rl_mark = cxt->save_mark;

  /* If rl_point == 0, we want to re-use the previous search string and
     start from the saved history position.  If there's no previous search
     string, punt. */
  if (rl_point == 0)
    {
      if (noninc_search_string == nullptr)
	{
	  rl_ding ();
	  rl_restore_prompt ();
	  RL_UNSETSTATE (RL_STATE_NSEARCH);
	  return -1;
	}
    }
  else
    {
      /* We want to start the search from the current history position. */
      noninc_history_pos = cxt->save_line;
      delete[] noninc_search_string;
      noninc_search_string = savestring (rl_line_buffer);

      /* If we don't want the subsequent undo list generated by the search
	 matching a history line to include the contents of the search string,
	 we need to clear rl_line_buffer here.  For now, we just clear the
	 undo list generated by reading the search string.  (If the search
	 fails, the old undo list will be restored by rl_maybe_unsave_line.) */
      rl_free_undo_list ();
    }

  rl_restore_prompt ();
  return noninc_dosearch (noninc_search_string, cxt->direction, (cxt->sflags & SF_PATTERN));
}

/* Search non-interactively through the history list.  DIR < 0 means to
   search backwards through the history of previous commands; otherwise
   the search is for commands subsequent to the current position in the
   history list.  PCHAR is the character to use for prompting when reading
   the search string; if not specified (0), it defaults to `:'. */
int
Readline::noninc_search (int dir, char pchar)
{
  _rl_search_cxt *cxt;
  int c, r;

  cxt = _rl_nsearch_init (dir, pchar);

  if (RL_ISSTATE (RL_STATE_CALLBACK))
    return 0;

  /* Read the search string. */
  r = 0;
  while (1)
    {
      c = _rl_search_getchar (cxt);

      if (c < 0)
	{
	  _rl_nsearch_abort (cxt);
	  return 1;
	}

      if (c == 0)
	break;

      r = _rl_nsearch_dispatch (cxt, c);
      if (r < 0)
        return 1;
      else if (r == 0)
	break;
    }

  r = _rl_nsearch_dosearch (cxt);
  return (r >= 0) ? _rl_nsearch_cleanup (cxt, r) : (r != 1);
}

/* Search forward through the history list for a string.  If the vi-mode
   code calls this, KEY will be `?'. */
int
Readline::rl_noninc_forward_search (int, int key)
{
  return noninc_search (1, (key == '?') ? '?' : 0);
}

/* Reverse search the history list for a string.  If the vi-mode code
   calls this, KEY will be `/'. */
int
Readline::rl_noninc_reverse_search (int, int key)
{
  return noninc_search (-1, (key == '/') ? '/' : 0);
}

/* Search forward through the history list for the last string searched
   for.  If there is no saved search string, abort.  If the vi-mode code
   calls this, KEY will be `N'. */
int
Readline::rl_noninc_forward_search_again (int, int key)
{
  int r;

  if (!noninc_search_string)
    {
      rl_ding ();
      return 1;
    }
#if defined (VI_MODE)
  if (VI_COMMAND_MODE() && key == 'N')
    r = noninc_dosearch (noninc_search_string, 1, SF_PATTERN);
  else
#endif
    r = noninc_dosearch (noninc_search_string, 1, SF_NONE);
  return r != 1;
}

/* Reverse search in the history list for the last string searched
   for.  If there is no saved search string, abort.  If the vi-mode code
   calls this, KEY will be `n'. */
int
Readline::rl_noninc_reverse_search_again (int, int key)
{
  int r;

  if (!noninc_search_string)
    {
      rl_ding ();
      return 1;
    }
#if defined (VI_MODE)
  if (VI_COMMAND_MODE() && key == 'n')
    r = noninc_dosearch (noninc_search_string, -1, SF_PATTERN);
  else
#endif
    r = noninc_dosearch (noninc_search_string, -1, SF_NONE);
  return r != 1;
}

#if defined (READLINE_CALLBACKS)
int
Readline::_rl_nsearch_callback (_rl_search_cxt *cxt)
{
  int c, r;

  c = _rl_search_getchar (cxt);
  if (c <= 0)
    {
      if (c < 0)
        _rl_nsearch_abort (cxt);
      return 1;
    }
  r = _rl_nsearch_dispatch (cxt, c);
  if (r != 0)
    return 1;

  r = _rl_nsearch_dosearch (cxt);
  return r >= 0 ? _rl_nsearch_cleanup (cxt, r) : (r != 1);
}
#endif

int
Readline::rl_history_search_internal (int count, int dir)
{
  rl_maybe_save_line ();
  HIST_ENTRY *temp = nullptr;
  unsigned int newcol = 0;

  /* Search COUNT times through the history for a line matching
     history_search_string.  If history_search_string[0] == '^', the
     line must match from the start; otherwise any substring can match.
     When this loop finishes, TEMP, if non-null, is the history line to
     copy into the line buffer. */
  while (count)
    {
      RL_CHECK_SIGNALS ();
      unsigned int new_pos = static_cast<unsigned int>
					(static_cast<int> (rl_history_search_pos) + dir);
      unsigned int ret = noninc_search_from_pos (history_search_string.c_str (),
						 new_pos, dir, SF_NONE, &newcol);
      if (ret == static_cast<unsigned int> (-1))
	break;

      /* Get the history entry we found. */
      rl_history_search_pos = ret;
      unsigned int oldpos = where_history ();
      history_set_pos (rl_history_search_pos);
      temp = current_history ();	/* will never be NULL after successful search */
      history_set_pos (oldpos);

      /* Don't find multiple instances of the same line. */
      if (prev_line_found && STREQ (prev_line_found, temp->line.c_str ()))
        continue;

      prev_line_found = temp->line.c_str ();	// XXX - could the ptr be freed too soon?
      count--;
    }

  /* If we didn't find anything at all, return. */
  if (temp == nullptr)
    {
      rl_maybe_unsave_line ();
      rl_ding ();
      /* If you don't want the saved history line (last match) to show up
         in the line buffer after the search fails, change the #if 0 to
         #if 1 */
      rl_point = rl_history_search_len;	/* rl_maybe_unsave_line changes it */
      rl_mark = rl_end ();
      return 1;
    }

  /* Copy the line we found into the current line buffer. */
  make_history_line_current (temp);

  /* decide where to put rl_point -- need to change this for pattern search */
  if (rl_history_search_flags & ANCHORED_SEARCH)
    rl_point = rl_history_search_len;	/* easy case */
  else
    {
      rl_point = newcol;	// XXX: always succeeds?
    }
  rl_mark = rl_end ();

  return 0;
}

void
Readline::rl_history_search_reinit (hist_search_flags flags)
{
  rl_history_search_pos = where_history ();
  rl_history_search_len = static_cast<unsigned int> (rl_point);
  rl_history_search_flags = flags;

  prev_line_found = nullptr;
  if (rl_point)
    {
      history_search_string.clear ();
      if (flags & ANCHORED_SEARCH)
	history_search_string.push_back ('^');

      history_search_string.append (rl_line_buffer, rl_point);
    }
  _rl_free_saved_history_line ();
}

/* Search forward in the history for the string of characters
   from the start of the line to rl_point.  This is a non-incremental
   search.  The search is anchored to the beginning of the history line. */
int
Readline::rl_history_search_forward (int count, int ignore)
{
  if (count == 0)
    return 0;

  if (rl_last_func != &Readline::rl_history_search_forward &&
      rl_last_func != &Readline::rl_history_search_backward)
    rl_history_search_reinit (ANCHORED_SEARCH);

  if (rl_history_search_len == 0)
    return rl_get_next_history (count, ignore);
  return rl_history_search_internal (std::abs (count), (count > 0) ? 1 : -1);
}

/* Search backward through the history for the string of characters
   from the start of the line to rl_point.  This is a non-incremental
   search. */
int
Readline::rl_history_search_backward (int count, int ignore)
{
  if (count == 0)
    return 0;

  if (rl_last_func != &Readline::rl_history_search_forward &&
      rl_last_func != &Readline::rl_history_search_backward)
    rl_history_search_reinit (ANCHORED_SEARCH);

  if (rl_history_search_len == 0)
    return rl_get_previous_history (count, ignore);
  return rl_history_search_internal (std::abs (count), (count > 0) ? -1 : 1);
}

/* Search forward in the history for the string of characters
   from the start of the line to rl_point.  This is a non-incremental
   search.  The search succeeds if the search string is present anywhere
   in the history line. */
int
Readline::rl_history_substr_search_forward (int count, int ignore)
{
  if (count == 0)
    return 0;

  if (rl_last_func != &Readline::rl_history_substr_search_forward &&
      rl_last_func != &Readline::rl_history_substr_search_backward)
    rl_history_search_reinit (NON_ANCHORED_SEARCH);

  if (rl_history_search_len == 0)
    return rl_get_next_history (count, ignore);
  return rl_history_search_internal (std::abs (count), (count > 0) ? 1 : -1);
}

/* Search backward through the history for the string of characters
   from the start of the line to rl_point.  This is a non-incremental
   search. */
int
Readline::rl_history_substr_search_backward (int count, int ignore)
{
  if (count == 0)
    return 0;

  if (rl_last_func != &Readline::rl_history_substr_search_forward &&
      rl_last_func != &Readline::rl_history_substr_search_backward)
    rl_history_search_reinit (NON_ANCHORED_SEARCH);

  if (rl_history_search_len == 0)
    return rl_get_previous_history (count, ignore);
  return rl_history_search_internal (std::abs (count), (count > 0) ? -1 : 1);
}

}  // namespace readline
