/* history.h -- the names of functions that you can call in history. */

/* Copyright (C) 1989-2015 Free Software Foundation, Inc.

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

#ifndef _HISTORY_H_
#define _HISTORY_H_

#include "rldefs.h"
#include "rlshell.h"

#include <ctime>		/* XXX - for history timestamp code */

namespace readline
{

/* Data stored with list. This is used for undo lists. */
class hist_data {
public:
  virtual ~hist_data ();
};

typedef hist_data *histdata_t;

/* The class used to store a history entry. */
class HIST_ENTRY {
public:
  HIST_ENTRY (const char *entry, const char *ts) :
		line (entry), timestamp (ts) {}

  HIST_ENTRY (const std::string &entry, const std::string &ts, histdata_t d) :
		line (entry), timestamp (ts), data(d) {}

  HIST_ENTRY (const std::string &entry) :
		line (entry) {}

  ~HIST_ENTRY ()
  {
    delete data;
  }

  std::string line;
  std::string timestamp;
  histdata_t data;

private:
};

/* Possible definitions for history starting point specification. */
enum hist_search_flags {
  NON_ANCHORED_SEARCH =		0,
  ANCHORED_SEARCH =		0x01,
  PATTERN_SEARCH =		0x02
};

// defaults for the History constructor below.
#define HISTORY_EVENT_DELIMITERS	"^$*%-"

#define HISTORY_WORD_DELIMITERS		" \t\n;&()|<>"


/* history file version; currently unused */
static const int history_file_version = 1;

// Parent class for Readline and Shell.

class History : public ReadlineShell {
public:
  virtual ~History () override;

  /* Generic function that takes a character buffer (which could be the readline
     line buffer) and an index into it (which could be rl_point) and returns
     an int. Also used for the inhibit history expansion callback. */
  typedef unsigned int (History::*rl_linebuf_func_t) (const std::string &, int);

  /* Initialization and state management. */

  // This constructor has parameters for all of the defaults that bash overrides.
  History (const char *search_delimiter_chars = nullptr,
	   const char *event_delimiter_chars = HISTORY_EVENT_DELIMITERS,
	   rl_linebuf_func_t inhibit_expansion_function = nullptr,
	   bool quotes_inhibit_expansion = false) :
		history_no_expand_chars (" \t\n\r="),
		history_word_delimiters (HISTORY_WORD_DELIMITERS),
		history_search_delimiter_chars(search_delimiter_chars),
		history_event_delimiter_chars(event_delimiter_chars),
		history_inhibit_expansion_function(inhibit_expansion_function),
		history_base (1),
		history_expansion_char ('!'),
		history_subst_char ('^'),
		history_quotes_inhibit_expansion(quotes_inhibit_expansion),
#if defined (HANDLE_MULTIBYTE)
		rl_byte_oriented (false)
#else
		rl_byte_oriented (true)
#endif
		{}

  /* Begin a session in which the history functions might be used.  This
     just sets the offset to the end of the list. Called frequently. */
  inline void using_history ()
  {
    history_offset = history_index_end;
  }

  /* Manage the history list. */

  /* Place STRING at the end of the history list.
     The associated data field (if any) is set to nullptr. */
  void add_history (const char *);

  /* Change the timestamp associated with the most recent history entry to
     STRING. */
  void add_history_time (const char *);

  /* Remove an entry from the history list.  WHICH is the magic number that
     tells us which element to delete.  The elements are numbered from 0. */
  HIST_ENTRY *remove_history (unsigned int);

  /* Remove a set of entries from the history list: FIRST to LAST, inclusive */
  HIST_ENTRY **remove_history_range (unsigned int, unsigned int);

  /* Make the history entry at WHICH have LINE and DATA.  This returns
     the old entry so you can dispose of the data.  In the case of an
     invalid WHICH, nullptr is returned. */
  HIST_ENTRY *replace_history_entry (unsigned int, const std::string &, histdata_t);

  /* Clear the history list and start over. */
  inline void
  clear_history ()
  {
    the_history.clear();
    history_offset = history_index_end = 0;
    history_base = 1;		/* reset history base to default */
  }

  /* Stifle the history list, remembering only MAX number of entries. */
  void stifle_history (unsigned int);

  /* Stop stifling the history. */
  inline void
  unstifle_history ()
  {
    history_stifled = false;

    // rearrange looped ring buffer for simpler handling
    if (history_index_end != the_history.size ())
      {
	size_t move_count = the_history.size () - history_index_end;
	the_history.insert (the_history.begin (), move_count, nullptr);
	std::copy (the_history.end () - static_cast<ssize_t> (move_count), the_history.end (),
		   the_history.begin ());
	the_history.resize (history_index_end);
      }
  }

  /* Return true if the history is stifled, false if it is not. */
  inline bool
  history_is_stifled () const {
    return history_stifled;
  }

  /* Information about the history list. */

  /* Returns the magic number which says what history element we are
     looking at, as the index relative to the oldest entry. */
  inline unsigned int
  where_history () const {
    unsigned int tmp = history_offset;
    unsigned int size = history_length ();
    if (history_index_end != size)
      {
	if (tmp < size)
	  tmp += size;
	tmp -= history_index_end;
      }
    return tmp;
  }

  /* Return the history entry at the specified index, relative to the
     oldest entry. This handles wrapping around in ring buffer mode. */
  inline HIST_ENTRY *
  get_history (unsigned int pos) const
  {
    unsigned int size = history_length ();
    if (history_index_end != size)
      {
	pos += history_index_end;
	if (pos >= size)
	  pos -= size;
      }
    return the_history.at (pos);	// check range, just in case
  }

  /* Return the history entry at the current position, as determined by
     history_offset.  If there is no entry there, return nullptr. */
  inline HIST_ENTRY *
  current_history () const {
    return (history_offset == the_history.size ())
		? nullptr : the_history[history_offset];
  }

  inline unsigned int
  history_length () const {
    return static_cast<unsigned int> (the_history.size ());
  }

  /* Helper to calculate the index of the `count`'th previous entry,
     wrapping around when we're in ring buffer mode. */
  inline unsigned int
  where_previous_history (unsigned int count)
  {
    unsigned int ret = history_index_end;
    while (ret < count)
      ret += history_length ();

    return ret - count;
  }

  /* Helper to find the index of the oldest entry in the list. */
  inline unsigned int
  where_oldest_history ()
  {
    if (history_stifled)
      return where_previous_history (history_length ());
    else
      return 0;
  }

  /* Return the timestamp associated with the HIST_ENTRY * passed as an
     argument */
  time_t history_get_time (const HIST_ENTRY *);

  /* Moving around the history list. */

  /* Make the current history item be the one at POS, a logical index.
     Returns false if POS is out of range, else true. */
  inline bool
  history_set_pos (unsigned int pos) {
    if (pos > the_history.size ())
      return false;

    history_offset = pos;
    return true;
  }

  /* Back up history_offset to the previous history entry, and return
     a pointer to that entry.  If there is no previous entry then return
     nullptr. */
  inline HIST_ENTRY *
  previous_history () {
    if (history_offset == where_oldest_history ())
      return nullptr;

    unsigned int tmp;
    if (history_offset == 0)
      tmp = history_length () - 1;
    else
      tmp = history_offset - 1;

    history_offset = tmp;
    return the_history[tmp];
  }

  /* Move history_offset forward to the next history entry, and return
     a pointer to that entry.  If there is no next entry then return
     nullptr. */
  inline HIST_ENTRY *
  next_history () {
    if (history_offset == history_index_end)
      return nullptr;

    unsigned int tmp;
    if (history_offset == history_length () - 1)
      tmp = 0;
    else
      tmp = history_offset + 1;

    history_offset = tmp;
    return the_history[tmp];
  }

  /* Searching the history list. */

  /* Search the history for STRING, starting at history_offset.
     If DIRECTION < 0, then the search is through previous entries,
     else through subsequent.  If the string is found, then
     current_history () is the history entry, and the value of this function
     is the offset in the line of that history entry that the string was
     found in.  Otherwise, nothing is changed, and a -1 is returned. */
  unsigned int history_search (const std::string &str, int direction) {
    return static_cast<unsigned int>
	(history_search_internal (str, direction, NON_ANCHORED_SEARCH));
  }

  /* Search the history for STRING, starting at history_offset.
     The search is anchored: matching lines must begin with string.
     DIRECTION is as in history_search(). */
  unsigned int history_search_prefix (const std::string &str, int direction) {
    return static_cast<unsigned int>
	(history_search_internal (str, direction, ANCHORED_SEARCH));
  }

  /* Search for STRING in the history list, starting at POS, an
     absolute index into the list.  DIR, if negative, says to search
     backwards from POS, else forwards.
     Returns the absolute index of the history element where STRING
     was found, or -1 otherwise. */
  unsigned int history_search_pos (const std::string &, int, unsigned int);

  /* Managing the history file. */

  /* Add the contents of FILENAME to the history list, a line at a time.
     If FILENAME is nullptr, then read from ~/.history.  Returns 0 if
     successful, or errno if not. */
  int read_history (const char *filename) {
    return read_history_range (filename, 0, static_cast<unsigned int> (-1));
  }

  /* Read a range of lines from FILENAME, adding them to the history list.
     Start reading at the FROM'th line and end at the TO'th.  If FROM
     is zero, start at the beginning.  If TO is (unsigned int)(-1)), read
     until the end of the file.  If FILENAME is nullptr, then read from
     ~/.history.  Returns 0 if successful, or errno if not. */
  int read_history_range (const char *, unsigned int, unsigned int);

  /* Overwrite FILENAME with the current history.  If FILENAME is nullptr,
     then write the history list to ~/.history.  Values returned
     are as in read_history ().*/
  int write_history (const char *filename) {
    return history_do_write (filename, history_length(), true);
  }

  /* Append NELEMENT entries to FILENAME.  The entries appended are from
     the end of the list minus NELEMENTs up to the end of the list. */
  int append_history (unsigned int nelements, const char *filename) {
    return history_do_write (filename, nelements, false);
  }

  /* Truncate the history file, leaving only the last NLINES lines. */
  int history_truncate_file (const char *, unsigned int);

  /* History expansion. */

  /* Expand the string STRING, placing the result into OUTPUT, a pointer
     to a string.  Returns:

     0) If no expansions took place (or, if the only change in
	the text was the de-slashifying of the history expansion
	character)
     1) If expansions did take place
    -1) If there was an error in expansion.
     2) If the returned line should just be printed.

    If an error occurred in expansion, then OUTPUT contains a descriptive
    error message. */
  int history_expand (const char *, char **);

  /* Extract a string segment consisting of the FIRST through LAST
     arguments present in STRING.  Arguments are broken up as in
     the shell. */
  char *history_arg_extract (int, int, const char *);

  /* Return the text of the history event beginning at the current
     offset into STRING.  Pass STRING with *INDEX equal to the
     history_expansion_char that begins this specification.
     DELIMITING_QUOTE is a character that is allowed to end the string
     specification for what to search for in addition to the normal
     characters `:', ` ', `\t', `\n', and sometimes `?'. */
  const char *get_history_event (const char *, unsigned int *, char);

  /* Return an array of tokens, much as the shell might.  The tokens are
     parsed out of STRING. */
  inline char **
  history_tokenize (const char *str) {
    return history_tokenize_internal (str, static_cast<unsigned int> (-1), nullptr);
  }

  // functions used by history and readline

  /* compare the specified two characters. If the characters matched,
     return true. Otherwise return false. */
  bool
  _rl_compare_chars (const char *buf1, unsigned int pos1, mbstate_t *ps1,
		     const char *buf2, unsigned int pos2, mbstate_t *ps2)
  {
    int w1, w2;

    if ((w1 = _rl_get_char_len (&buf1[pos1], ps1)) <= 0 ||
	(w2 = _rl_get_char_len (&buf2[pos2], ps2)) <= 0 ||
	(w1 != w2) ||
	(buf1[pos1] != buf2[pos2]))
      return false;

    for (unsigned int i = 1; i < static_cast<unsigned int> (w1); i++)
      if (buf1[pos1 + i] != buf2[pos2 + i])
	return false;

    return true;
  }

  unsigned int _rl_adjust_point (const std::string &string, unsigned int point, mbstate_t *ps);

  unsigned int _hs_history_patsearch (const char *string, int direction, hist_search_flags flags);

  wchar_t _rl_char_value (const std::string &buf, unsigned int ind);

  /* mbutil.c */

  /* Find next `count' characters started byte point of the specified seed.
     If `find_non_zero` is true, we look for non-zero-width multibyte
     characters. */
  inline unsigned int
  _rl_find_next_mbchar (const std::string &string, unsigned int seed, int count,
			find_mbchar_flags find_non_zero)
  {
#if defined (HANDLE_MULTIBYTE)
    return _rl_find_next_mbchar_internal (string, seed, count, find_non_zero);
#else
    return seed + count;
#endif
  }

  /* Find previous character started byte point of the specified seed.
     Returned point will be point <= seed.  If `find_non_zero` is true,
     we look for non-zero-width multibyte characters. */
  inline unsigned int
  _rl_find_prev_mbchar (const std::string &string, unsigned int seed,
			find_mbchar_flags find_non_zero)
  {
#if defined (HANDLE_MULTIBYTE)
    return _rl_find_prev_mbchar_internal (string, seed, find_non_zero);
#else
    return (seed == 0) ? seed : (seed - 1);
#endif
  }

  void _hs_replace_history_data (unsigned int which, histdata_t old, histdata_t new_);

  int _rl_get_char_len (const std::string &src, mbstate_t *ps);

private:

  // Private member functions

  /* Does S look like the beginning of a history timestamp entry?  Placeholder
     for more extensive tests. */
  inline bool HIST_TIMESTAMP_START(const char *s) {
    return *s == history_comment_char && std::isdigit (static_cast<unsigned char> (s[1]));
  }

  // histexpand.c
  int history_expand_internal (const char *string, unsigned int start, char qc,
				unsigned int *end_index_ptr, char **ret_string,
				const char *current_line);

  void postproc_subst_rhs ();

  char *get_history_word_specifier (const char *spec, const char *from, unsigned int *caller_index);

  char **history_tokenize_internal (const char *string, unsigned int wind, unsigned int *indp);

  char *get_subst_pattern (const char *, unsigned int *, char, bool, unsigned int *);

  unsigned int history_tokenize_word (const char *, unsigned int);

  char *history_find_word (const char *, unsigned int);

  // histfile.c
  char *history_filename (const char *filename);
  int history_do_write (const char *filename, unsigned int nelements, bool overwrite);

  // history.c

  /* Append LINE to the history line at offset WHICH, adding a newline to the
     end of the current line first.  This can be used to construct multi-line
     history entries while reading lines from the history file. */
  void _hs_append_history_line (unsigned int which, const char *line);

  char *hist_inittime ();

  // histsearch.c

  unsigned int history_search_internal (const std::string &string, int direction,
					hist_search_flags flags);

  // mbutil.c

  // Find next multi-byte char in std::string.
  unsigned int _rl_find_next_mbchar_internal (const std::string &string, unsigned int seed,
					      int count, find_mbchar_flags find_non_zero);

  // Find next multi-byte char in std::string.
  unsigned int _rl_find_prev_mbchar_internal (const std::string &string, unsigned int seed,
					      find_mbchar_flags find_non_zero);

public:

  /* ************************************************************** */
  /*	      Public history Variables (pointer types)		    */
  /* ************************************************************** */

  /* The list of characters which inhibit the expansion of text if found
     immediately following history_expansion_char. */
  const char *history_no_expand_chars;

  /* Used to split words by history_tokenize_internal. */
  const char *history_word_delimiters;

  /* The list of alternate characters that can delimit a history search
     string. */
  const char *history_search_delimiter_chars;

  // Ring buffer of history entry ptrs, with optional ptrs to histdata.
  std::vector<HIST_ENTRY*> the_history;

private:

  /* ************************************************************** */
  /*	    Protected history Variables (pointer types)		    */
  /* ************************************************************** */

  /* histexpand.c */

  /* Characters that delimit history event specifications and separate event
     specifications from word designators. */
  const char *history_event_delimiter_chars;

  // Optional callback function to inhibit expansion.
  rl_linebuf_func_t history_inhibit_expansion_function;

  /* The last string searched for by a !?string? search. */
  char *history_search_string;

  /* The last string matched by a !?string? search. */
  char *history_search_match;

  char *history_subst_lhs;
  char *history_subst_rhs;

  void *_pad[2];	// for future object expansion

public:

  /* ************************************************************** */
  /*		Public history Variables (32-bit types)		    */
  /* ************************************************************** */

  /* Max number of history entries, or zero for unlimited. */
  unsigned int history_max_entries;

  /* Immediately after a call to read_history() or read_history_range(), this
     will return the number of lines just read from the history file in that
     call. */
  unsigned int history_lines_read_from_file;

  /* Immediately after a call to write_history() or history_do_write(), this
     will return the number of lines just written to the history file in that
     call.  This also works with history_truncate_file. */
  unsigned int history_lines_written_to_file;

protected:

  /* ************************************************************** */
  /*	    Protected history Variables (32-bit types)		    */
  /* ************************************************************** */

  /* The logical `base' of the history array.  It defaults to 1. */
  unsigned int history_base;

private:

  /* ************************************************************** */
  /*	    Private history Variables (32-bit types)		    */
  /* ************************************************************** */

  /* The location where the next history entry will be inserted. */
  unsigned int history_offset;

  /* The location after the most recent entry in the list. */
  unsigned int history_index_end;

  unsigned int history_subst_lhs_len;
  unsigned int history_subst_rhs_len;

public:

  /* ************************************************************** */
  /*		Public history Variables (8-bit types)		    */
  /* ************************************************************** */

  /* The character that represents the start of a history expansion
     request.  This is usually `!'. */
  char history_expansion_char;

  /* The character that invokes word substitution if found at the start of
     a line.  This is usually `^'. */
  char history_subst_char;

  /* During tokenization, if this character is seen as the first character
     of a word, then it, and all subsequent characters up to a newline are
     ignored.  For a Bourne shell, this should be '#'.  Bash special cases
     the interactive comment character to not be a comment delimiter. */
  char history_comment_char;

  // The shell will change this from '\0' to either '\'' or '"' when quoting.
  char history_quoting_state;

  bool history_quotes_inhibit_expansion;

  /* If true, we write timestamps to the history file in history_do_write() */
  bool history_write_timestamps;

  // UTF-8 locale flag, shared between history and readline.
  bool _rl_utf8locale;

  // If true, the character set is byte-oriented (not multibyte).
  bool rl_byte_oriented;

private:

  /* True means that we have enforced a limit on the amount of
     history that we save. */
  bool history_stifled;

  /* If true, we assume that a history file that starts with a timestamp
     uses timestamp-delimited entries and can include multi-line history
     entries. Used by read_history_range */
  bool history_multiline_entries;

  char _pad2[6];		// for alignment and future expansion
};

}  // namespace readline

#endif /* !_HISTORY_H_ */
