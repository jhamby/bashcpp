/* complete.c -- filename completion for readline. */

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

#include "readline.hh"
#include "rlprivate.hh"

#include <sys/types.h>
#include <fcntl.h>
#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif

#include "posixstat.hh"

#if defined (COLOR_SUPPORT)
#  include "colors.hh"
#endif

/* Most systems don't declare getpwent in <pwd.h> if _POSIX_SOURCE is
   defined. */
#if defined (HAVE_GETPWENT) && (!defined (HAVE_GETPW_DECLS) || defined (_POSIX_SOURCE))
extern "C" struct passwd *getpwent ();
#endif /* HAVE_GETPWENT && (!HAVE_GETPW_DECLS || _POSIX_SOURCE) */

namespace readline
{

#ifdef HAVE_LSTAT
#  define LSTAT ::lstat
#else
#  define LSTAT ::stat
#endif

/* Unix version of a hidden file.  Could be different on other systems. */
#define HIDDEN_FILE(fname)	((fname)[0] == '.')

#if defined (VISIBLE_STATS) || defined (COLOR_SUPPORT)
#  if !defined (X_OK)
#    define X_OK 1
#  endif
#endif

/* **************************************************************** */
/*								    */
/*	Completion matching, from readline's point of view.	    */
/*								    */
/* **************************************************************** */

/*************************************/
/*				     */
/*    Bindable completion functions  */
/*				     */
/*************************************/

/* Complete the word at or before point.  You have supplied the function
   that does the initial simple matching selection algorithm (see
   rl_completion_matches ()).  The default is to do filename completion. */
int
Readline::rl_complete (int count, int invoking_key)
{
  rl_completion_invoking_key = invoking_key;

  if (rl_inhibit_completion)
    return _rl_insert_char (count, invoking_key);
  else if (rl_last_func == &Readline::rl_complete && _rl_cmpl_changed_buffer == 0)
    return rl_complete_internal ('?');
  else if (_rl_complete_show_all)
    return rl_complete_internal ('!');
  else if (_rl_complete_show_unmodified)
    return rl_complete_internal ('@');
  else
    return rl_complete_internal (TAB);
}

/* List the possible completions.  See description of rl_complete (). */
int
Readline::rl_possible_completions (int, int invoking_key)
{
  rl_completion_invoking_key = invoking_key;
  return rl_complete_internal ('?');
}

int
Readline::rl_insert_completions (int, int invoking_key)
{
  rl_completion_invoking_key = invoking_key;
  return rl_complete_internal ('*');
}

/* Return the correct value to pass to rl_complete_internal performing
   the same tests as rl_complete.  This allows consecutive calls to an
   application's completion function to list possible completions and for
   an application-specific completion function to honor the
   show-all-if-ambiguous readline variable. */
int
Readline::rl_completion_mode (rl_command_func_t cfunc)
{
  if (rl_last_func == cfunc && !_rl_cmpl_changed_buffer)
    return '?';
  else if (_rl_complete_show_all)
    return '!';
  else if (_rl_complete_show_unmodified)
    return '@';
  else
    return TAB;
}

/************************************/
/*				    */
/*    Completion utility functions  */
/*				    */
/************************************/

/* Set default values for readline word completion.  These are the variables
   that application completion functions can change or inspect. */
void
Readline::set_completion_defaults (int what_to_do)
{
  /* Only the completion entry function can change these. */
  rl_filename_completion_desired = 0;
  rl_filename_quoting_desired = 1;
  rl_completion_type = what_to_do;
  rl_completion_suppress_append = rl_completion_suppress_quote = 0;
  rl_completion_append_character = ' ';

  /* The completion entry function may optionally change this. */
  rl_completion_mark_symlink_dirs = _rl_complete_mark_symlink_dirs;

  /* Reset private state. */
  _rl_complete_display_matches_interrupt = 0;
}

/* The user must press "y" or "n". Non-zero return means "y" pressed. */
int
Readline::get_y_or_n (bool for_pager)
{
  int c;

  /* For now, disable pager in callback mode, until we later convert to state
     driven functions.  Have to wait until next major version to add new
     state definition, since it will change value of RL_STATE_DONE. */
#if defined (READLINE_CALLBACKS)
  if (RL_ISSTATE (RL_STATE_CALLBACK))
    return 1;
#endif

  for (;;)
    {
      RL_SETSTATE(RL_STATE_MOREINPUT);
      c = rl_read_key ();
      RL_UNSETSTATE(RL_STATE_MOREINPUT);

      if (c == 'y' || c == 'Y' || c == ' ')
	return 1;
      if (c == 'n' || c == 'N' || c == RUBOUT)
	return 0;
      if (c == ABORT_CHAR || c < 0)
	_rl_abort_internal ();
      if (for_pager && (c == NEWLINE || c == RETURN))
	return 2;
      if (for_pager && (c == 'q' || c == 'Q'))
	return 0;
      rl_ding ();
    }
}

int
Readline::_rl_internal_pager (int lines)
{
  std::fprintf (rl_outstream, "--More--");
  std::fflush (rl_outstream);

  int i = get_y_or_n (1);
  _rl_erase_entire_line ();
  if (i == 0)
    return -1;
  else if (i == 2)
    return lines - 1;
  else
    return 0;
}

static inline bool
path_isdir (const char *filename)
{
  struct stat finfo;

  return ::stat (filename, &finfo) == 0 && S_ISDIR (finfo.st_mode);
}

#if defined (VISIBLE_STATS)
/* Return the character which best describes FILENAME.
     `@' for symbolic links
     `/' for directories
     `*' for executables
     `=' for sockets
     `|' for FIFOs
     `%' for character special devices
     `#' for block special devices */
int
Readline::stat_char (const char *filename)
{
  struct stat finfo;
  int character, r;
  const char *fn;

  /* Short-circuit a //server on cygwin, since that will always behave as
     a directory. */
#if defined (__CYGWIN__)
  if (filename[0] == '/' && filename[1] == '/' && std::strchr (filename+2, '/') == 0)
    return '/';
#endif

  char *f = nullptr;
  if (rl_filename_stat_hook)
    {
      std::string tmp (filename);
      ((*this).*rl_filename_stat_hook) (tmp);
      f = savestring (tmp);
      fn = f;
    }
  else
    fn = filename;

#if defined (HAVE_LSTAT) && defined (S_ISLNK)
  r = ::lstat (fn, &finfo);
#else
  r = ::stat (fn, &finfo);
#endif

  if (r == -1)
    {
      delete[] f;
      return 0;
    }

  character = 0;
  if (S_ISDIR (finfo.st_mode))
    character = '/';
#if defined (S_ISCHR)
  else if (S_ISCHR (finfo.st_mode))
    character = '%';
#endif /* S_ISCHR */
#if defined (S_ISBLK)
  else if (S_ISBLK (finfo.st_mode))
    character = '#';
#endif /* S_ISBLK */
#if defined (S_ISLNK)
  else if (S_ISLNK (finfo.st_mode))
    character = '@';
#endif /* S_ISLNK */
#if defined (S_ISSOCK)
  else if (S_ISSOCK (finfo.st_mode))
    character = '=';
#endif /* S_ISSOCK */
#if defined (S_ISFIFO)
  else if (S_ISFIFO (finfo.st_mode))
    character = '|';
#endif
  else if (S_ISREG (finfo.st_mode))
    {
#if defined (_WIN32) && !defined (__CYGWIN__)
      char *ext;

      /* Windows doesn't do access and X_OK; check file extension instead */
      ext = std::strrchr (fn, '.');
      if (ext && (_rl_stricmp (ext, ".exe") == 0 ||
		  _rl_stricmp (ext, ".cmd") == 0 ||
		  _rl_stricmp (ext, ".bat") == 0 ||
		  _rl_stricmp (ext, ".com") == 0))
	character = '*';
#else
      if (::access (filename, X_OK) == 0)
	character = '*';
#endif
    }

  delete[] f;
  return character;
}
#endif /* VISIBLE_STATS */

/* Return the portion of PATHNAME that should be output when listing
   possible completions.  If we are hacking filename completion, we
   are only interested in the basename, the portion following the
   final slash.  Otherwise, we return what we were passed.  Since
   printing empty strings is not very informative, if we're doing
   filename completion, and the basename is the empty string, we look
   for the previous slash and return the portion following that.  If
   there's no previous slash, we just return what we were passed. */
char *
Readline::printable_part (char *pathname)
{
  char *temp, *x;

  if (rl_filename_completion_desired == 0)	/* don't need to do anything */
    return pathname;

  temp = std::strrchr (pathname, '/');
#if defined (__MSDOS__) || defined (_WIN32)
  if (temp == 0 && ISALPHA ((unsigned char)pathname[0]) && pathname[1] == ':')
    temp = pathname + 1;
#endif

  if (temp == nullptr || *temp == '\0')
    return pathname;
  else if (temp[1] == 0 && temp == pathname)
    return pathname;
  /* If the basename is NULL, we might have a pathname like '/usr/src/'.
     Look for a previous slash and, if one is found, return the portion
     following that slash.  If there's no previous slash, just return the
     pathname we were passed. */
  else if (temp[1] == '\0')
    {
      for (x = temp - 1; x > pathname; x--)
        if (*x == '/')
          break;
      return (*x == '/') ? x + 1 : pathname;
    }
  else
    return ++temp;
}

/* Compute width of STRING when displayed on screen by print_filename */
static int
fnwidth (const char *string)
{
  int width, pos;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  size_t left;
  size_t clen;
  wchar_t wc;

  left = std::strlen (string) + 1;
  std::memset (&ps, 0, sizeof (mbstate_t));
#endif

  width = pos = 0;
  while (string[pos])
    {
      if (CTRL_CHAR (string[pos]) || string[pos] == RUBOUT)
	{
	  width += 2;
	  pos++;
	}
      else
	{
#if defined (HANDLE_MULTIBYTE)
	  clen = std::mbrtowc (&wc, string + pos, left - static_cast<size_t> (pos), &ps);
	  if (MB_INVALIDCH (clen))
	    {
	      width++;
	      pos++;
	      std::memset (&ps, 0, sizeof (mbstate_t));
	    }
	  else if (MB_NULLWCH (clen))
	    break;
	  else
	    {
	      pos += clen;
	      int w = WCWIDTH (wc);
	      width += (w >= 0) ? w : 1;
	    }
#else
	  width++;
	  pos++;
#endif
	}
    }

  return width;
}

#define ELLIPSIS_LEN	3

int
Readline::fnprint (const char *to_print, int prefix_bytes, const char *real_pathname)
{
  int printed_len, w;
  const char *s;
  int common_prefix_len, print_len;
#if defined (HANDLE_MULTIBYTE)
  mbstate_t ps;
  const char *end;
  int width;
  wchar_t wc;

  print_len = static_cast<int> (std::strlen (to_print));
  end = to_print + print_len + 1;
  std::memset (&ps, 0, sizeof (mbstate_t));
#else
  print_len = std::strlen (to_print);
#endif

  printed_len = common_prefix_len = 0;

  /* Don't print only the ellipsis if the common prefix is one of the
     possible completions.  Only cut off prefix_bytes if we're going to be
     printing the ellipsis, which takes precedence over coloring the
     completion prefix (see print_filename() below). */
  if (_rl_completion_prefix_display_length > 0 && prefix_bytes >= print_len)
    prefix_bytes = 0;

#if defined (COLOR_SUPPORT)
  if (_rl_colored_stats && (prefix_bytes == 0 || _rl_colored_completion_prefix <= 0))
    colored_stat_start (real_pathname);
#endif

  if (prefix_bytes && _rl_completion_prefix_display_length > 0)
    {
      char ellipsis;

      ellipsis = (to_print[prefix_bytes] == '.') ? '_' : '.';
      for (w = 0; w < ELLIPSIS_LEN; w++)
	std::putc (ellipsis, rl_outstream);
      printed_len = ELLIPSIS_LEN;
    }
#if defined (COLOR_SUPPORT)
  else if (prefix_bytes && _rl_colored_completion_prefix > 0)
    {
      common_prefix_len = prefix_bytes;
      prefix_bytes = 0;
      /* XXX - print color indicator start here */
      colored_prefix_start ();
    }
#endif

  s = to_print + prefix_bytes;
  while (*s)
    {
      if (CTRL_CHAR (*s))
        {
          std::putc ('^', rl_outstream);
          std::putc (UNCTRL (*s), rl_outstream);
          printed_len += 2;
          s++;
#if defined (HANDLE_MULTIBYTE)
	  std::memset (&ps, 0, sizeof (mbstate_t));
#endif
        }
      else if (*s == RUBOUT)
	{
	  std::putc ('^', rl_outstream);
	  std::putc ('?', rl_outstream);
	  printed_len += 2;
	  s++;
#if defined (HANDLE_MULTIBYTE)
	  std::memset (&ps, 0, sizeof (mbstate_t));
#endif
	}
      else
	{
#if defined (HANDLE_MULTIBYTE)
	  size_t tlen = std::mbrtowc (&wc, s, static_cast<size_t> (end - s), &ps);
	  if (MB_INVALIDCH (tlen))
	    {
	      tlen = 1;
	      width = 1;
	      std::memset (&ps, 0, sizeof (mbstate_t));
	    }
	  else if (MB_NULLWCH (tlen))
	    break;
	  else
	    {
	      w = WCWIDTH (wc);
	      width = (w >= 0) ? w : 1;
	    }
	  std::fwrite (s, 1, tlen, rl_outstream);
	  s += tlen;
	  printed_len += width;
#else
	  std::putc (*s, rl_outstream);
	  s++;
	  printed_len++;
#endif
	}
      if (common_prefix_len > 0 && (s - to_print) >= common_prefix_len)
	{
#if defined (COLOR_SUPPORT)
	  /* printed bytes = s - to_print */
	  /* printed bytes should never be > but check for paranoia's sake */
	  colored_prefix_end ();
	  if (_rl_colored_stats)
	    colored_stat_start (real_pathname);		/* XXX - experiment */
#endif
	  common_prefix_len = 0;
	}
    }

#if defined (COLOR_SUPPORT)
  /* XXX - unconditional for now */
  if (_rl_colored_stats)
    colored_stat_end ();
#endif

  return printed_len;
}

/* Output TO_PRINT to rl_outstream.  If VISIBLE_STATS is defined and we
   are using it, check for and output a single character for `special'
   filenames.  Return the number of characters we output. */

int
Readline::print_filename (char *to_print, const char *full_pathname, int prefix_bytes)
{
  int extension_char = 0;
  int printed_len = 0;

#if defined (COLOR_SUPPORT)
  /* Defer printing if we want to prefix with a color indicator */
  if (_rl_colored_stats == 0 || rl_filename_completion_desired == 0)
#endif
    printed_len = fnprint (to_print, prefix_bytes, to_print);

  if (rl_filename_completion_desired && (
#if defined (VISIBLE_STATS)
     rl_visible_stats ||
#endif
#if defined (COLOR_SUPPORT)
     _rl_colored_stats ||
#endif
     _rl_complete_mark_directories))
    {
      /* If to_print != full_pathname, to_print is the basename of the
	 path passed.  In this case, we try to expand the directory
	 name before checking for the stat character. */
      if (to_print != full_pathname)
	{
	  /* Terminate the directory name. */
	  char c = to_print[-1];
	  to_print[-1] = '\0';

	  /* If setting the last slash in full_pathname to a NUL results in
	     full_pathname being the empty string, we are trying to complete
	     files in the root directory.  If we pass a null string to the
	     bash directory completion hook, for example, it will expand it
	     to the current directory.  We just want the `/'. */
	  const char *dn;
	  if (full_pathname == nullptr || *full_pathname == '\0')
	    dn = "/";
	  else if (full_pathname[0] != '/')
	    dn = full_pathname;
	  else if (full_pathname[1] == 0)
	    dn = "//";		/* restore trailing slash to `//' */
	  else if (full_pathname[1] == '/' && full_pathname[2] == 0)
	    dn = "/";		/* don't turn /// into // */
	  else
	    dn = full_pathname;
	  char *tmp = tilde_expand (dn);
	  std::string s (tmp);
	  delete[] tmp;
	  if (rl_directory_completion_hook)
	    ((*this).*rl_directory_completion_hook) (s);

	  size_t slen = s.size ();
	  size_t tlen = std::strlen (to_print);
	  char *new_full_pathname = new char[slen + tlen + 2];
	  std::strcpy (new_full_pathname, s.data ());
	  if (s[slen - 1] == '/')
	    slen--;
	  else
	    new_full_pathname[slen] = '/';
	  std::strcpy (new_full_pathname + slen + 1, to_print);

#if defined (VISIBLE_STATS)
	  if (rl_visible_stats)
	    extension_char = stat_char (new_full_pathname);
	  else
#endif
	  if (_rl_complete_mark_directories)
	    {
	      if (rl_directory_completion_hook == nullptr && rl_filename_stat_hook)
		{
		  std::string new_pathname_copy (new_full_pathname);
		  ((*this).*rl_filename_stat_hook) (new_pathname_copy);
		  delete[] new_full_pathname;
		  new_full_pathname = savestring (new_pathname_copy);
		}
	      if (path_isdir (new_full_pathname))
		extension_char = '/';
	    }

	  /* Move colored-stats code inside fnprint() */
#if defined (COLOR_SUPPORT)
	  if (_rl_colored_stats)
	    printed_len = fnprint (to_print, prefix_bytes, new_full_pathname);
#endif

	  delete[] new_full_pathname;
	  to_print[-1] = c;
	}
      else
	{
	  char *s = tilde_expand (full_pathname);
#if defined (VISIBLE_STATS)
	  if (rl_visible_stats)
	    extension_char = stat_char (s);
	  else
#endif
	    if (_rl_complete_mark_directories && path_isdir (s))
	      extension_char = '/';

	  /* Move colored-stats code inside fnprint() */
#if defined (COLOR_SUPPORT)
	  if (_rl_colored_stats)
	    printed_len = fnprint (to_print, prefix_bytes, s);
#endif
	  delete[] s;
	}

      if (extension_char)
	{
	  std::putc (extension_char, rl_outstream);
	  printed_len++;
	}
    }

  return printed_len;
}

char *
Readline::rl_quote_filename (const char *s, replace_type, unsigned char *qcp)
{
  char *r = new char[std::strlen (s) + 2];
  *r = *rl_completer_quote_characters;
  std::strcpy (r + 1, s);

  if (qcp)
    *qcp = static_cast<unsigned char> (*rl_completer_quote_characters);
  return r;
}

/* Find the bounds of the current word for completion purposes, and leave
   rl_point set to the end of the word.  This function skips quoted
   substrings (characters between matched pairs of characters in
   rl_completer_quote_characters).  First we try to find an unclosed
   quoted substring on which to do matching.  If one is not found, we use
   the word break characters to find the boundaries of the current word.
   We call an application-specific function to decide whether or not a
   particular word break character is quoted; if that function returns a
   non-zero result, the character does not break a word.  This function
   returns the opening quote character if we found an unclosed quoted
   substring, '\0' otherwise.  FP, if non-null, is set to a value saying
   which (shell-like) quote characters we found (single quote, double
   quote, or backslash) anywhere in the string.  DP, if non-null, is set to
   the value of the delimiter character that caused a word break. */

unsigned char
Readline::_rl_find_completion_word (rl_qf_flags *fp, unsigned char *dp)
{
//   int scan, end, found_quote, delimiter, pass_next, isbrk;
//   char quote_char, *brkchars;

  unsigned int scan;
  unsigned int end = rl_point;
  rl_qf_flags found_quote = RL_QF_NONE;
  unsigned char delimiter = '\0';
  unsigned char quote_char = '\0';
  bool pass_next = false;

  const char *brkchars = nullptr;
  if (rl_completion_word_break_hook)
    brkchars = ((*this).*rl_completion_word_break_hook) ();
  if (brkchars == nullptr)
    brkchars = rl_completer_word_break_characters;

  if (rl_completer_quote_characters)
    {
      /* We have a list of characters which can be used in pairs to
	 quote substrings for the completer.  Try to find the start
	 of an unclosed quoted substring. */
      /* FOUND_QUOTE is set so we know what kind of quotes we found. */
      for (scan = 0; scan < end;
	   scan = MB_NEXTCHAR (rl_line_buffer, scan, 1, MB_FIND_ANY))
	{
	  if (pass_next)
	    {
	      pass_next = false;
	      continue;
	    }

	  /* Shell-like semantics for single quotes -- don't allow backslash
	     to quote anything in single quotes, especially not the closing
	     quote.  If you don't like this, take out the check on the value
	     of quote_char. */
	  if (quote_char != '\'' && rl_line_buffer[scan] == '\\')
	    {
	      pass_next = true;
	      found_quote |= RL_QF_BACKSLASH;
	      continue;
	    }

	  if (quote_char != '\0')
	    {
	      /* Ignore everything until the matching close quote char. */
	      if (rl_line_buffer[scan] == quote_char)
		{
		  /* Found matching close.  Abandon this substring. */
		  quote_char = '\0';
		  rl_point = end;
		}
	    }
	  else if (std::strchr (rl_completer_quote_characters, rl_line_buffer[scan]))
	    {
	      /* Found start of a quoted substring. */
	      quote_char = static_cast<unsigned char> (rl_line_buffer[scan]);
	      rl_point = scan + 1;
	      /* Shell-like quoting conventions. */
	      if (quote_char == '\'')
		found_quote |= RL_QF_SINGLE_QUOTE;
	      else if (quote_char == '"')
		found_quote |= RL_QF_DOUBLE_QUOTE;
	      else
		found_quote |= RL_QF_OTHER_QUOTE;
	    }
	}
    }

  if (rl_point == end && quote_char == '\0')
    {
      /* We didn't find an unclosed quoted substring upon which to do
         completion, so use the word break characters to find the
         substring on which to complete. */
      while ((rl_point = MB_PREVCHAR (rl_line_buffer, rl_point, MB_FIND_ANY)))
	{
	  char c = rl_line_buffer[rl_point];

	  if (std::strchr (brkchars, c) == nullptr)
	    continue;

	  /* Call the application-specific function to tell us whether
	     this word break character is quoted and should be skipped. */
	  if (rl_char_is_quoted_p && found_quote &&
	      ((*this).*rl_char_is_quoted_p) (rl_line_buffer,
					      static_cast<int> (rl_point)))
	    continue;

	  /* Convoluted code, but it avoids an n^2 algorithm with calls
	     to char_is_quoted. */
	  break;
	}
    }

  /* If we are at an unquoted word break, then advance past it. */
  char c = rl_line_buffer[rl_point];

  /* If there is an application-specific function to say whether or not
     a character is quoted and we found a quote character, let that
     function decide whether or not a character is a word break, even
     if it is found in rl_completer_word_break_characters.  Don't bother
     if we're at the end of the line, though. */
  if (c)
    {
      bool isbrk;
      if (rl_char_is_quoted_p)
	isbrk = (found_quote == 0 ||
		((*this).*rl_char_is_quoted_p) (rl_line_buffer,
			static_cast<int> (rl_point)) == 0) &&
			std::strchr (brkchars, c) != nullptr;
      else
	isbrk = std::strchr (brkchars, c) != nullptr;

      if (isbrk)
	{
	  /* If the character that caused the word break was a quoting
	     character, then remember it as the delimiter. */
	  if (rl_basic_quote_characters &&
	      std::strchr (rl_basic_quote_characters, c) &&
	      (end - rl_point) > 1)
	    delimiter = static_cast<unsigned char> (c);

	  /* If the character isn't needed to determine something special
	     about what kind of completion to perform, then advance past it. */
	  if (rl_special_prefixes == nullptr ||
	      std::strchr (rl_special_prefixes, c) == nullptr)
	    rl_point++;
	}
    }

  if (fp)
    *fp = found_quote;
  if (dp)
    *dp = delimiter;

  return quote_char;
}

char **
Readline::gen_completion_matches (const char *text, unsigned int start,
				  unsigned int end,
				  rl_compentry_func_t our_func,
				  rl_qf_flags found_quote,
				  unsigned char quote_char)
{
  char **matches;

  rl_completion_found_quote = found_quote;
  rl_completion_quote_character = quote_char;

  /* If the user wants to TRY to complete, but then wants to give
     up and use the default completion function, they set the
     variable rl_attempted_completion_function. */
  if (rl_attempted_completion_function)
    {
      matches = ((*this).*rl_attempted_completion_function) (text, start, end);
      if (RL_SIG_RECEIVED())
	{
	  _rl_free_match_list (matches);
	  matches = nullptr;
	  RL_CHECK_SIGNALS ();
	}

      if (matches || rl_attempted_completion_over)
	{
	  rl_attempted_completion_over = 0;
	  return matches;
	}
    }

  /* XXX -- filename dequoting moved into rl_filename_completion_function */

  /* rl_completion_matches will check for signals as well to avoid a long
     delay while reading a directory. */
  matches = rl_completion_matches (text, our_func);
  if (RL_SIG_RECEIVED())
    {
      _rl_free_match_list (matches);
      matches = nullptr;
      RL_CHECK_SIGNALS ();
    }
  return matches;
}

/* Filter out duplicates in MATCHES.  This frees up the strings in
   MATCHES. */
char **
Readline::remove_duplicate_matches (char **matches)
{
  char *lowest_common;
  unsigned int i, j, newlen;
  char dead_slot;
  char **temp_array;

  /* Sort the items. */
  for (i = 0; matches[i]; i++)
    ;

  /* Sort the array without matches[0], since we need it to
     stay in place no matter what. */
  if (i && rl_sort_completion_matches)
    std::qsort (matches + 1, i - 1, sizeof (char *), &_rl_qsort_string_compare);

  /* Remember the lowest common denominator for it may be unique. */
  lowest_common = savestring (matches[0]);

  for (i = newlen = 0; matches[i + 1]; i++)
    {
      if (std::strcmp (matches[i], matches[i + 1]) == 0)
	{
	  delete[] matches[i];
	  matches[i] = &dead_slot;
	}
      else
	newlen++;
    }

  /* We have marked all the dead slots with (char *)&dead_slot.
     Copy all the non-dead entries into a new array. */
  temp_array = new char*[3 + newlen];
  for (i = j = 1; matches[i]; i++)
    {
      if (matches[i] != &dead_slot)
	temp_array[j++] = matches[i];
    }
  temp_array[j] = nullptr;

  if (matches[0] != &dead_slot)
    delete[] matches[0];

  /* Place the lowest common denominator back in [0]. */
  temp_array[0] = lowest_common;

  /* If there is one string left, and it is identical to the
     lowest common denominator, then the LCD is the string to
     insert. */
  if (j == 2 && std::strcmp (temp_array[0], temp_array[1]) == 0)
    {
      delete[] temp_array[1];
      temp_array[1] = nullptr;
    }
  return temp_array;
}

/* Find the common prefix of the list of matches, and put it into
   matches[0]. */
int
Readline::compute_lcd_of_matches (char **match_list, int matches, const char *text)
{
  size_t low = 100000;			/* Count of max-matched characters. */
  char *dtext;				/* dequoted TEXT, if needed */
#if defined (HANDLE_MULTIBYTE)
  size_t v1, v2;
  mbstate_t ps1, ps2;
  wchar_t wc1, wc2;
#endif

  /* If only one match, just use that.  Otherwise, compare each
     member of the list with the next, finding out where they
     stop matching. */
  if (matches == 1)
    {
      match_list[0] = match_list[1];
      match_list[1] = nullptr;
      return 1;
    }

  size_t si;
  char c1, c2;
  for (int i = 1; i < matches; i++)
    {
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
	{
	  std::memset (&ps1, 0, sizeof (mbstate_t));
	  std::memset (&ps2, 0, sizeof (mbstate_t));
	}
#endif
      for (si = 0; (c1 = match_list[i][si]) && (c2 = match_list[i + 1][si]); si++)
	{
	    if (_rl_completion_case_fold)
	      {
	        c1 = static_cast<char> (_rl_to_lower (c1));
	        c2 = static_cast<char> (_rl_to_lower (c2));
	      }
#if defined (HANDLE_MULTIBYTE)
	    if (MB_CUR_MAX > 1 && !rl_byte_oriented)
	      {
		v1 = std::mbrtowc (&wc1, match_list[i] + si,
				   std::strlen (match_list[i] + si), &ps1);
		v2 = std::mbrtowc (&wc2, match_list[i + 1] + si,
				   std::strlen (match_list[i + 1] + si), &ps2);
		if (MB_INVALIDCH (v1) || MB_INVALIDCH (v2))
		  {
		    if (c1 != c2)	/* do byte comparison */
		      break;
		    continue;
		  }
		if (_rl_completion_case_fold)
		  {
		    wc1 = static_cast<wchar_t> (std::towlower (static_cast<wint_t> (wc1)));
		    wc2 = static_cast<wchar_t> (std::towlower (static_cast<wint_t> (wc2)));
		  }
		if (wc1 != wc2)
		  break;
		else if (v1 > 1)
		  si += v1 - 1;
	      }
	    else
#endif
	    if (c1 != c2)
	      break;
	}

      if (low > si)
	low = si;
    }

  /* If there were multiple matches, but none matched up to even the
     first character, and the user typed something, use that as the
     value of matches[0]. */
  if (low == 0 && text && *text)
    {
      match_list[0] = new char[std::strlen (text) + 1];
      std::strcpy (match_list[0], text);
    }
  else
    {
      match_list[0] = new char[low + 1];

      /* XXX - this might need changes in the presence of multibyte chars */

      /* If we are ignoring case, try to preserve the case of the string
	 the user typed in the face of multiple matches differing in case. */
      if (_rl_completion_case_fold)
	{
	  /* We're making an assumption here:
		IF we're completing filenames AND
		   the application has defined a filename dequoting function AND
		   we found a quote character AND
		   the application has requested filename quoting
		THEN
		   we assume that TEXT was dequoted before checking against
		   the file system and needs to be dequoted here before we
		   check against the list of matches
		FI */
	  dtext = nullptr;
	  if (rl_filename_completion_desired &&
	      rl_filename_dequoting_function &&
	      rl_completion_found_quote &&
	      rl_filename_quoting_desired)
	    {
	      dtext = ((*this).*rl_filename_dequoting_function)
			(const_cast<char*> (text), rl_completion_quote_character);
	      text = dtext;
	    }

	  /* sort the list to get consistent answers. */
	  if (rl_sort_completion_matches)
	    std::qsort (match_list + 1, static_cast<size_t> (matches),
			sizeof (char *), &_rl_qsort_string_compare);

	  si = std::strlen (text);
	  size_t lx = (si <= low) ? si : low;	/* check shorter of text and matches */
	  /* Try to preserve the case of what the user typed in the presence of
	     multiple matches: check each match for something that matches
	     what the user typed taking case into account; use it up to common
	     length of matches if one is found.  If not, just use first match. */
	  int i;
	  for (i = 1; i <= matches; i++)
	    if (std::strncmp (match_list[i], text, lx) == 0)
	      {
		std::strncpy (match_list[0], match_list[i], low);
		break;
	      }
	  /* no casematch, use first entry */
	  if (i > matches)
	    std::strncpy (match_list[0], match_list[1], low);

	  delete[] dtext;
	}
      else
        std::strncpy (match_list[0], match_list[1], low);

      match_list[0][low] = '\0';
    }

  return matches;
}

int
Readline::postprocess_matches (char ***matchesp, bool matching_filenames)
{
  char *t, **matches, **temp_matches;
  int nmatch, i;

  matches = *matchesp;

  if (matches == nullptr)
    return 0;

  /* It seems to me that in all the cases we handle we would like
     to ignore duplicate possibilities.  Scan for the text to
     insert being identical to the other completions. */
  if (rl_ignore_completion_duplicates)
    {
      temp_matches = remove_duplicate_matches (matches);
      delete[] matches;
      matches = temp_matches;
    }

  /* If we are matching filenames, then here is our chance to
     do clever processing by re-examining the list.  Call the
     ignore function with the array as a parameter.  It can
     munge the array, deleting matches as it desires. */
  if (rl_ignore_some_completions_function && matching_filenames)
    {
      for (nmatch = 1; matches[nmatch]; nmatch++)
	;
      (void)((*this).*rl_ignore_some_completions_function) (matches);
      if (matches == nullptr || matches[0] == nullptr)
	{
	  delete[] matches;
	  *matchesp = nullptr;
	  return 0;
        }
      else
	{
	  /* If we removed some matches, recompute the common prefix. */
	  for (i = 1; matches[i]; i++)
	    ;
	  if (i > 1 && i < nmatch)
	    {
	      t = matches[0];
	      compute_lcd_of_matches (matches, i - 1, t);
	      delete[] t;
	    }
	}
    }

  *matchesp = matches;
  return 1;
}

unsigned int
Readline::complete_get_screenwidth ()
{
  int cols;
  char *envcols;

  cols = _rl_completion_columns;
  if (cols >= 0 && cols <= static_cast<int> (_rl_screenwidth))
    return static_cast<unsigned int> (cols);
  envcols = std::getenv ("COLUMNS");
  if (envcols && *envcols)
    cols = std::atoi (envcols);
  if (cols >= 0 && cols <= static_cast<int> (_rl_screenwidth))
    return static_cast<unsigned int> (cols);
  return _rl_screenwidth;
}

/* A convenience function for displaying a list of strings in
   columnar format on readline's output stream.  MATCHES is the list
   of strings, in argv format, LEN is the number of strings in MATCHES,
   and MAX is the length of the longest string in MATCHES. */
void
Readline::rl_display_match_list (char **matches, int len, int max)
{
  int count, limit, printed_len;
  int common_length, sind;
  char *temp, *t;

  /* Find the length of the prefix common to all items: length as displayed
     characters (common_length) and as a byte index into the matches (sind) */
  common_length = sind = 0;
  if (_rl_completion_prefix_display_length > 0)
    {
      t = printable_part (matches[0]);
      /* check again in case of /usr/src/ */
      temp = rl_filename_completion_desired ? std::strrchr (t, '/') : nullptr;
      common_length = temp ? fnwidth (temp) : fnwidth (t);
      sind = static_cast<int> (temp ? std::strlen (temp) : std::strlen (t));
      if (common_length > max || sind > max)
	common_length = sind = 0;

      if (common_length > static_cast<int> (_rl_completion_prefix_display_length) &&
	  common_length > ELLIPSIS_LEN)
	max -= (common_length - ELLIPSIS_LEN);
      else
	common_length = sind = 0;
    }
#if defined (COLOR_SUPPORT)
  else if (_rl_colored_completion_prefix > 0)
    {
      t = printable_part (matches[0]);
      temp = rl_filename_completion_desired ? std::strrchr (t, '/') : nullptr;
      common_length = temp ? fnwidth (temp) : fnwidth (t);
      /* want portion after final slash */
      sind = static_cast<int> (temp ? std::strlen (temp + 1) : std::strlen (t));
      if (common_length > max || sind > max)
	common_length = sind = 0;
    }
#endif

  /* How many items of MAX length can we fit in the screen window? */
  unsigned int cols = complete_get_screenwidth ();
  max += 2;
  limit = static_cast<int> (cols) / max;
  if (limit != 1 && (limit * max == static_cast<int> (cols)))
    limit--;

  /* If cols == 0, limit will end up -1 */
  if (cols < _rl_screenwidth && limit < 0)
    limit = 1;

  /* Avoid a possible floating exception.  If max > cols,
     limit will be 0 and a divide-by-zero fault will result. */
  if (limit == 0)
    limit = 1;

  /* How many iterations of the printing loop? */
  count = (len + (limit - 1)) / limit;

  /* Watch out for special case.  If LEN is less than LIMIT, then
     just do the inner printing loop.
	   0 < len <= limit  implies  count = 1. */

  /* Sort the items if they are not already sorted. */
  if (!rl_ignore_completion_duplicates && rl_sort_completion_matches)
    std::qsort (matches + 1, static_cast<size_t> (len),
		sizeof (char *), &_rl_qsort_string_compare);

  rl_crlf ();

  int lines = 0;
  if (_rl_print_completions_horizontally == 0)
    {
      /* Print the sorted items, up-and-down alphabetically, like ls. */
      for (int i = 1; i <= count; i++)
	{
	  for (int j = 0, l = i; j < limit; j++)
	    {
	      if (l > len || matches[l] == nullptr)
		break;
	      else
		{
		  temp = printable_part (matches[l]);
		  printed_len = print_filename (temp, matches[l], sind);

		  if (j + 1 < limit)
		    {
		      if (max <= printed_len)
			putc (' ', rl_outstream);
		      else
			for (int k = 0; k < max - printed_len; k++)
			  putc (' ', rl_outstream);
		    }
		}
	      l += count;
	    }
	  rl_crlf ();
#if defined (SIGWINCH)
	  if (RL_SIG_RECEIVED () && RL_SIGWINCH_RECEIVED() == 0)
#else
	  if (RL_SIG_RECEIVED ())
#endif
	    return;
	  lines++;
	  if (_rl_page_completions &&
	      static_cast<unsigned int> (lines) >= (_rl_screenheight - 1) && i < count)
	    {
	      lines = _rl_internal_pager (lines);
	      if (lines < 0)
		return;
	    }
	}
    }
  else
    {
      /* Print the sorted items, across alphabetically, like ls -x. */
      for (int i = 1; matches[i]; i++)
	{
	  temp = printable_part (matches[i]);
	  printed_len = print_filename (temp, matches[i], static_cast<int> (sind));
	  /* Have we reached the end of this line? */
#if defined (SIGWINCH)
	  if (RL_SIG_RECEIVED () && RL_SIGWINCH_RECEIVED() == 0)
#else
	  if (RL_SIG_RECEIVED ())
#endif
	    return;
	  if (matches[i + 1])
	    {
	      if (limit == 1 || (i && (limit > 1) && (i % limit) == 0))
		{
		  rl_crlf ();
		  lines++;
		  if (_rl_page_completions &&
		      static_cast<unsigned int> (lines) >= _rl_screenheight - 1)
		    {
		      lines = _rl_internal_pager (lines);
		      if (lines < 0)
			return;
		    }
		}
	      else if (max <= printed_len)
		std::putc (' ', rl_outstream);
	      else
		for (int k = 0; k < max - printed_len; k++)
		  std::putc (' ', rl_outstream);
	    }
	}
      rl_crlf ();
    }
}

/* Display MATCHES, a list of matching filenames in argv format.  This
   handles the simple case -- a single match -- first.  If there is more
   than one match, we compute the number of strings in the list and the
   length of the longest string, which will be needed by the display
   function.  If the application wants to handle displaying the list of
   matches itself, it sets RL_COMPLETION_DISPLAY_MATCHES_HOOK to the
   address of a function, and we just call it.  If we're handling the
   display ourselves, we just call rl_display_match_list.  We also check
   that the list of matches doesn't exceed the user-settable threshold,
   and ask the user if he wants to see the list if there are more matches
   than RL_COMPLETION_QUERY_ITEMS. */
void
Readline::display_matches (char **matches)
{
  int len, max, i;
  char *temp;

  /* Move to the last visible line of a possibly-multiple-line command. */
  _rl_move_vert (_rl_vis_botlin);

  /* Handle simple case first.  What if there is only one answer? */
  if (matches[1] == nullptr)
    {
      temp = printable_part (matches[0]);
      rl_crlf ();
      print_filename (temp, matches[0], 0);
      rl_crlf ();

      rl_forced_update_display ();
      rl_display_fixed = 1;

      return;
    }

  /* There is more than one answer.  Find out how many there are,
     and find the maximum printed length of a single entry. */
  for (max = 0, i = 1; matches[i]; i++)
    {
      temp = printable_part (matches[i]);
      len = fnwidth (temp);

      if (len > max)
	max = len;
    }

  len = i - 1;

  /* If the caller has defined a display hook, then call that now. */
  if (rl_completion_display_matches_hook)
    {
      ((*this).*rl_completion_display_matches_hook) (matches, len, max);
      return;
    }

  /* If there are many items, then ask the user if she really wants to
     see them all. */
  if (rl_completion_query_items > 0 && len >= rl_completion_query_items)
    {
      rl_crlf ();
      std::fprintf (rl_outstream, "Display all %d possibilities? (y or n)", len);
      std::fflush (rl_outstream);
      if (get_y_or_n (0) == 0)
	{
	  rl_crlf ();

	  rl_forced_update_display ();
	  rl_display_fixed = true;

	  return;
	}
    }

  rl_display_match_list (matches, len, max);

  rl_forced_update_display ();
  rl_display_fixed = true;
}

/* qc == pointer to quoting character, if any */
char *
Readline::make_quoted_replacement (char *match, replace_type mtype,
				   unsigned char *qc)
{
  bool should_quote;
  char *replacement;

  /* If we are doing completion on quoted substrings, and any matches
     contain any of the completer_word_break_characters, then auto-
     matically prepend the substring with a quote character (just pick
     the first one from the list of such) if it does not already begin
     with a quote string.  FIXME: Need to remove any such automatically
     inserted quote character when it no longer is necessary, such as
     if we change the string we are completing on and the new set of
     matches don't require a quoted substring. */
  replacement = match;

  should_quote = match && rl_completer_quote_characters &&
			rl_filename_completion_desired &&
			rl_filename_quoting_desired;

  if (should_quote)
    should_quote = should_quote && (!qc || !*qc ||
		     (rl_completer_quote_characters &&
		      std::strchr (rl_completer_quote_characters, *qc)));

  if (should_quote)
    {
      /* If there is a single match, see if we need to quote it.
         This also checks whether the common prefix of several
	 matches needs to be quoted. */
      should_quote = rl_filename_quote_characters
			? (_rl_strpbrk (match, rl_filename_quote_characters) != nullptr)
			: 0;

      replace_type do_replace = should_quote ? mtype : NO_MATCH;
      /* Quote the replacement, since we found an embedded
	 word break character in a potential match. */
      if (do_replace != NO_MATCH && rl_filename_quoting_function)
	replacement = ((*this).*rl_filename_quoting_function) (match, do_replace, qc);
    }
  return replacement;
}

void
Readline::insert_match (char *match, unsigned int start, replace_type mtype, unsigned char *qc)
{
  char *replacement, *r;
  unsigned char oqc;
  unsigned int end, rlen;

  oqc = qc ? *qc : '\0';
  replacement = make_quoted_replacement (match, mtype, qc);

  /* Now insert the match. */
  if (replacement)
    {
      rlen = static_cast<unsigned int> (std::strlen (replacement));
      /* Don't double an opening quote character. */
      if (qc && *qc && start && rl_line_buffer[start - 1] == *qc &&
	    replacement[0] == *qc)
	start--;
      /* If make_quoted_replacement changed the quoting character, remove
	 the opening quote and insert the (fully-quoted) replacement. */
      else if (qc && (*qc != oqc) && start && rl_line_buffer[start - 1] == oqc &&
	    replacement[0] != oqc)
	start--;
      end = rl_point - 1;
      /* Don't double a closing quote character */
      if (qc && *qc && end && rl_line_buffer[rl_point] == *qc && replacement[rlen - 1] == *qc)
        end++;
      if (_rl_skip_completed_text)
	{
	  r = replacement;
	  while (start < rl_end () && *r && rl_line_buffer[start] == *r)
	    {
	      start++;
	      r++;
	    }
	  if (start <= end || *r)
	    _rl_replace_text (r, start, end);
	  rl_point = start + static_cast<unsigned int> (std::strlen (r));
	}
      else
	_rl_replace_text (replacement, start, end);
      if (replacement != match)
        delete[] replacement;
    }
}

/* Append any necessary closing quote and a separator character to the
   just-inserted match.  If the user has specified that directories
   should be marked by a trailing `/', append one of those instead.  The
   default trailing character is a space.  Returns the number of characters
   appended.  If NONTRIVIAL_MATCH is set, we test for a symlink (if the OS
   has them) and don't add a suffix for a symlink to a directory.  A
   nontrivial match is one that actually adds to the word being completed.
   The variable rl_completion_mark_symlink_dirs controls this behavior
   (it's initially set to the what the user has chosen, indicated by the
   value of _rl_complete_mark_symlink_dirs, but may be modified by an
   application's completion function). */
int
Readline::append_to_match (char *text, unsigned char delimiter,
			   unsigned char quote_char, bool nontrivial_match)
{
  char temp_string[4], *filename;
  int temp_string_index, s;
  struct stat finfo;

  temp_string_index = 0;
  if (quote_char && rl_point && rl_completion_suppress_quote == 0 &&
      rl_line_buffer[rl_point - 1] != static_cast<char> (quote_char))
    temp_string[temp_string_index++] = static_cast<char> (quote_char);

  if (delimiter)
    temp_string[temp_string_index++] = static_cast<char> (delimiter);
  else if (rl_completion_suppress_append == 0 && rl_completion_append_character)
    temp_string[temp_string_index++] = static_cast<char> (rl_completion_append_character);

  temp_string[temp_string_index++] = '\0';

  if (rl_filename_completion_desired)
    {
      filename = tilde_expand (text);
      if (rl_filename_stat_hook)
        {
          std::string fn (filename);
	  ((*this).*rl_filename_stat_hook) (fn);
	  delete[] filename;
	  filename = savestring (fn);
        }
      s = (nontrivial_match && rl_completion_mark_symlink_dirs == 0)
		? LSTAT (filename, &finfo)
		: stat (filename, &finfo);
      if (s == 0 && S_ISDIR (finfo.st_mode))
	{
	  if (_rl_complete_mark_directories /* && rl_completion_suppress_append == 0 */)
	    {
	      /* This is clumsy.  Avoid putting in a double slash if point
		 is at the end of the line and the previous character is a
		 slash. */
	      if (rl_point && rl_line_buffer.size () == rl_point &&
		  rl_line_buffer[rl_point - 1] == '/')
		;
	      else if (rl_line_buffer[rl_point] != '/')
		rl_insert_text ("/");
	    }
	}
#ifdef S_ISLNK
      /* Don't add anything if the filename is a symlink and resolves to a
	 directory. */
      else if (s == 0 && S_ISLNK (finfo.st_mode) && path_isdir (filename))
	;
#endif
      else
	{
	  if (rl_point == rl_line_buffer.size () && temp_string_index)
	    rl_insert_text (temp_string);
	}
      delete[] filename;
    }
  else
    {
      if (rl_point == rl_line_buffer.size () && temp_string_index)
	rl_insert_text (temp_string);
    }

  return temp_string_index;
}

void
Readline::insert_all_matches (char **matches, unsigned int point, unsigned char *qc)
{
  int i;
  char *rp;

  rl_begin_undo_group ();
  /* remove any opening quote character; make_quoted_replacement will add
     it back. */
  if (qc && *qc && point && rl_line_buffer[point - 1] == *qc)
    point--;
  rl_delete_text (point, rl_point);
  rl_point = point;

  if (matches[1])
    {
      for (i = 1; matches[i]; i++)
	{
	  rp = make_quoted_replacement (matches[i], SINGLE_MATCH, qc);
	  rl_insert_text (rp);
	  rl_insert_text (" ");
	  if (rp != matches[i])
	    delete[] rp;
	}
    }
  else
    {
      rp = make_quoted_replacement (matches[0], SINGLE_MATCH, qc);
      rl_insert_text (rp);
      rl_insert_text (" ");
      if (rp != matches[0])
	delete[] rp;
    }
  rl_end_undo_group ();
}

/* Compare a possibly-quoted filename TEXT from the line buffer and a possible
   MATCH that is the product of filename completion, which acts on the dequoted
   text. */
int
Readline::compare_match (const char *text, const char *match)
{
  if (rl_filename_completion_desired && rl_filename_quoting_desired &&
      rl_completion_found_quote && rl_filename_dequoting_function)
    {
      char *temp = ((*this).*rl_filename_dequoting_function)
			(text, rl_completion_quote_character);
      int r = std::strcmp (temp, match);
      delete[] temp;
      return r;
    }
  return std::strcmp (text, match);
}

/* Complete the word at or before point.
   WHAT_TO_DO says what to do with the completion.
   `?' means list the possible completions.
   TAB means do standard completion.
   `*' means insert all of the possible completions.
   `!' means to do standard completion, and list all possible completions if
   there is more than one.
   `@' means to do standard completion, and list all possible completions if
   there is more than one and partial completion is not possible. */
int
Readline::rl_complete_internal (int what_to_do)
{
  unsigned int start, end;
  unsigned char delimiter;
  rl_qf_flags found_quote;
  unsigned char quote_char;

  RL_SETSTATE(RL_STATE_COMPLETING);

  bool saved_last_completion_failed = last_completion_failed;

  set_completion_defaults (what_to_do);

  char *saved_line_buffer = rl_line_buffer.empty () ? nullptr : savestring (rl_line_buffer);

  rl_compentry_func_t our_func = rl_completion_entry_function
				  ? rl_completion_entry_function
				  : &Readline::rl_filename_completion_function;

  /* We now look backwards for the start of a filename/variable word. */
  end = rl_point;
  found_quote = RL_QF_NONE;
  delimiter = '\0';
  quote_char = '\0';

  if (rl_point)
    /* This (possibly) changes rl_point.  If it returns a non-zero char,
       we know we have an open quote. */
    quote_char = _rl_find_completion_word (&found_quote, &delimiter);

  start = rl_point;
  rl_point = end;

  char *text = rl_copy_text (start, end);
  char **matches = gen_completion_matches (text, start, end, our_func,
					   found_quote, quote_char);
  /* nontrivial_lcd is set if the common prefix adds something to the word
     being completed. */
  bool nontrivial_lcd = matches && compare_match (text, matches[0]) != 0;

  size_t tlen = 0;
  if (what_to_do == '!' || what_to_do == '@')
    tlen = std::strlen (text);

  delete[] text;

  if (matches == nullptr)
    {
      rl_ding ();
      delete[] saved_line_buffer;
      _rl_cmpl_changed_buffer = false;
      last_completion_failed = true;
      RL_UNSETSTATE(RL_STATE_COMPLETING);
      _rl_reset_completion_state ();
      return 0;
    }

  /* If we are matching filenames, the attempted completion function will
     have set rl_filename_completion_desired to a non-zero value.  The basic
     rl_filename_completion_function does this. */
  if (postprocess_matches (&matches, rl_filename_completion_desired) == 0)
    {
      rl_ding ();
      delete[] saved_line_buffer;
      _rl_cmpl_changed_buffer = false;
      last_completion_failed = true;
      RL_UNSETSTATE(RL_STATE_COMPLETING);
      _rl_reset_completion_state ();
      return 0;
    }

  if (matches && matches[0] && *matches[0])
    last_completion_failed = false;

  switch (what_to_do)
    {
    case TAB:
    case '!':
    case '@':
      /* Insert the first match with proper quoting. */
      if (what_to_do == TAB)
        {
          if (*matches[0])
	    insert_match (matches[0], start, matches[1] ? MULT_MATCH : SINGLE_MATCH,
			  &quote_char);
        }
      else if (*matches[0] && matches[1] == nullptr)
	/* should we perform the check only if there are multiple matches? */
	insert_match (matches[0], start, matches[1] ? MULT_MATCH : SINGLE_MATCH,
		      &quote_char);
      else if (*matches[0])	/* what_to_do != TAB && multiple matches */
	{
	  size_t mlen = *matches[0] ? strlen (matches[0]) : 0;
	  if (mlen >= tlen)
	    insert_match (matches[0], start, matches[1] ? MULT_MATCH : SINGLE_MATCH,
			  &quote_char);
	}

      /* If there are more matches, ring the bell to indicate.
	 If we are in vi mode, Posix.2 says to not ring the bell.
	 If the `show-all-if-ambiguous' variable is set, display
	 all the matches immediately.  Otherwise, if this was the
	 only match, and we are hacking files, check the file to
	 see if it was a directory.  If so, and the `mark-directories'
	 variable is set, add a '/' to the name.  If not, and we
	 are at the end of the line, then add a space.  */
      if (matches[1])
	{
	  if (what_to_do == '!')
	    {
	      display_matches (matches);
	      break;
	    }
	  else if (what_to_do == '@')
	    {
	      if (!nontrivial_lcd)
		display_matches (matches);
	      break;
	    }
	  else if (rl_editing_mode != vi_mode)
	    rl_ding ();	/* There are other matches remaining. */
	}
      else
	append_to_match (matches[0], delimiter, quote_char, nontrivial_lcd);

      break;

    case '*':
      insert_all_matches (matches, start, &quote_char);
      break;

    case '?':
      /* Let's try to insert a single match here if the last completion failed
	 but this attempt returned a single match. */
      if (saved_last_completion_failed && matches[0] && *matches[0] &&
	  matches[1] == nullptr)
	{
	  insert_match (matches[0], start, matches[1] ? MULT_MATCH : SINGLE_MATCH,
			&quote_char);
	  append_to_match (matches[0], delimiter, quote_char, nontrivial_lcd);
	  break;
	}

      if (rl_completion_display_matches_hook == nullptr)
	{
	  _rl_sigcleanup = &Readline::_rl_complete_sigcleanup;
	  _rl_sigcleanarg = matches;
	  _rl_complete_display_matches_interrupt = false;
	}
      display_matches (matches);
      if (_rl_complete_display_matches_interrupt)
        {
          matches = nullptr;		/* already freed by rl_complete_sigcleanup */
          _rl_complete_display_matches_interrupt = false;
	  if (rl_signal_event_hook)
	    ((*this).*rl_signal_event_hook) ();		/* XXX */
        }
      _rl_sigcleanup = nullptr;
      _rl_sigcleanarg = nullptr;
      break;

    default:
      _rl_ttymsg ("bad value %d for what_to_do in rl_complete", what_to_do);
      rl_ding ();
      delete[] saved_line_buffer;
      RL_UNSETSTATE(RL_STATE_COMPLETING);
      _rl_free_match_list (matches);
      _rl_reset_completion_state ();
      return 1;
    }

  _rl_free_match_list (matches);

  /* Check to see if the line has changed through all of this manipulation. */
  if (saved_line_buffer)
    {
      _rl_cmpl_changed_buffer =
	std::strcmp (rl_line_buffer.c_str (), saved_line_buffer) != 0;

      delete[] saved_line_buffer;
    }

  RL_UNSETSTATE(RL_STATE_COMPLETING);
  _rl_reset_completion_state ();

  RL_CHECK_SIGNALS ();
  return 0;
}

/***************************************************************/
/*							       */
/*  Application-callable completion match generator functions  */
/*							       */
/***************************************************************/

/* Return an array of (char *) which is a list of completions for TEXT.
   If there are no completions, return a NULL pointer.
   The first entry in the returned array is the substitution for TEXT.
   The remaining entries are the possible completions.
   The array is terminated with a NULL pointer.

   ENTRY_FUNCTION is a function of two args, and returns a (char *).
     The first argument is TEXT.
     The second is a state argument; it should be zero on the first call, and
     non-zero on subsequent calls.  It returns a NULL pointer to the caller
     when there are no more matches.
 */
char **
Readline::rl_completion_matches (const char *text, rl_compentry_func_t entry_function)
{
  /* The list of matches. */
  std::vector<char *> match_list;

  /* Number of matches actually found. */
  int matches;

  /* Temporary string binder. */
  char *string;

  matches = 0;
  match_list.push_back (nullptr);	// empty first entry

  while ((string = ((*this).*entry_function) (text, matches)))
    {
      if (RL_SIG_RECEIVED ())
	{
	  /* Start at 1 because we don't set matches[0] in this function.
	     Only free the list members if we're building match list from
	     rl_filename_completion_function, since we know that doesn't
	     free the strings it returns. */
	  if (entry_function == &Readline::rl_filename_completion_function)
	    {
	      for (size_t i = 1; match_list[i]; i++)
		delete[] match_list[i];
	    }
	  match_list.clear ();
	  matches = 0;
	  match_list.push_back (nullptr);	// empty first entry
	  RL_CHECK_SIGNALS ();
	}

      match_list.push_back (string);
    }

  /* If there were any matches, then look through them finding out the
     lowest common denominator.  That then becomes match_list[0]. */
  if (matches)
    {
      char** match_array = new char*[match_list.size ()];
      std::memcpy (match_array, match_list.data (), match_list.size () * sizeof (char *));
      compute_lcd_of_matches (match_array, matches, text);
      return match_array;
    }
  else				/* There were no matches. */
    {
      return nullptr;
    }
}

/* A completion function for usernames.
   TEXT contains a partial username preceded by a random
   character (usually `~').  */
char *
Readline::rl_username_completion_function (const char *text, int state)
{
#if defined (__WIN32__) || defined (__OPENNT)
  return nullptr;
#else /* !__WIN32__ && !__OPENNT) */
  if (state == 0)
    {
      _rl_ucmp_first_char = *text;
      _rl_ucmp_first_char_loc = _rl_ucmp_first_char == '~';

      _rl_ucmp_username = &text[_rl_ucmp_first_char_loc];
#if defined (HAVE_GETPWENT)
      ::setpwent ();
#endif
    }

#if defined (HAVE_GETPWENT)
  while ((_rl_ucmp_pwentry = ::getpwent ()))
    {
      /* Null usernames should result in all users as possible completions. */
      if (_rl_ucmp_username.empty () || _rl_ucmp_username != _rl_ucmp_pwentry->pw_name)
	break;
    }
#endif

  if (_rl_ucmp_pwentry == nullptr)
    {
#if defined (HAVE_GETPWENT)
      ::endpwent ();
#endif
      return nullptr;
    }
  else
    {
      char *value = new char[2 + std::strlen (_rl_ucmp_pwentry->pw_name)];

      *value = *text;

      std::strcpy (value + _rl_ucmp_first_char_loc, _rl_ucmp_pwentry->pw_name);

      if (_rl_ucmp_first_char == '~')
	rl_filename_completion_desired = true;

      return value;
    }
#endif /* !__WIN32__ && !__OPENNT */
}

/* Return non-zero if CONVFN matches FILENAME up to the length of FILENAME
   (FILENAME_LEN).  If _rl_completion_case_fold is set, compare without
   regard to the alphabetic case of characters.  If
   _rl_completion_case_map is set, make `-' and `_' equivalent.  CONVFN is
   the possibly-converted directory entry; FILENAME is what the user typed. */
bool
Readline::complete_fncmp (const char *convfn, unsigned int convlen, const char *filename,
			  unsigned int filename_len)
{
#if defined (HANDLE_MULTIBYTE)
  size_t v1, v2;
  mbstate_t ps1, ps2;
  wchar_t wc1, wc2;
#endif

#if defined (HANDLE_MULTIBYTE)
  std::memset (&ps1, 0, sizeof (mbstate_t));
  std::memset (&ps2, 0, sizeof (mbstate_t));
#endif

  if (filename_len == 0)
    return true;
  if (convlen < filename_len)
    return false;

  size_t len = filename_len;
  const char *s1 = convfn;
  const char *s2 = filename;

  /* Otherwise, if these match up to the length of filename, then
     it is a match. */
  if (_rl_completion_case_fold && _rl_completion_case_map)
    {
      /* Case-insensitive comparison treating _ and - as equivalent */
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
	{
	  do
	    {
	      v1 = std::mbrtowc (&wc1, s1, convlen, &ps1);
	      v2 = std::mbrtowc (&wc2, s2, filename_len, &ps2);
	      if (v1 == 0 && v2 == 0)
		return true;
	      else if (MB_INVALIDCH (v1) || MB_INVALIDCH (v2))
		{
		  if (*s1 != *s2)		/* do byte comparison */
		    return false;
		  else if ((*s1 == '-' || *s1 == '_') && (*s2 == '-' || *s2 == '_'))
		    return false;
		  s1++; s2++; len--;
		  continue;
		}
	      wc1 = static_cast<wchar_t> (std::towlower (static_cast<wint_t> (wc1)));
	      wc2 = static_cast<wchar_t> (std::towlower (static_cast<wint_t> (wc2)));
	      s1 += v1;
	      s2 += v1;
	      len -= v1;
	      if ((wc1 == L'-' || wc1 == L'_') && (wc2 == L'-' || wc2 == L'_'))
	        continue;
	      if (wc1 != wc2)
		return false;
	    }
	  while (len != 0);
	}
      else
#endif
	{
	do
	  {
	    bool d = _rl_to_lower (*s1) ^ _rl_to_lower (*s2);
	    /* *s1 == [-_] && *s2 == [-_] */
	    if ((*s1 == '-' || *s1 == '_') && (*s2 == '-' || *s2 == '_'))
	      d = false;
	    if (d)
	      return false;
	    s1++; s2++;	/* already checked convlen >= filename_len */
	  }
	while (--len != 0);
	}

      return true;
    }
  else if (_rl_completion_case_fold)
    {
#if defined (HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1 && !rl_byte_oriented)
	{
	  do
	    {
	      v1 = std::mbrtowc (&wc1, s1, convlen, &ps1);
	      v2 = std::mbrtowc (&wc2, s2, filename_len, &ps2);
	      if (v1 == 0 && v2 == 0)
		return true;
	      else if (MB_INVALIDCH (v1) || MB_INVALIDCH (v2))
		{
		  if (*s1 != *s2)		/* do byte comparison */
		    return false;
		  s1++; s2++; len--;
		  continue;
		}
	      wc1 = static_cast<wchar_t> (std::towlower (static_cast<wint_t> (wc1)));
	      wc2 = static_cast<wchar_t> (std::towlower (static_cast<wint_t> (wc2)));
	      if (wc1 != wc2)
		return false;
	      s1 += v1;
	      s2 += v1;
	      len -= v1;
	    }
	  while (len != 0);
	  return true;
	}
      else
#endif
      if ((_rl_to_lower (convfn[0]) == _rl_to_lower (filename[0])) &&
	  (convlen >= filename_len) &&
	  (_rl_strnicmp (filename, convfn, filename_len) == 0))
	return true;
    }
  else
    {
      if ((convfn[0] == filename[0]) &&
	  (convlen >= filename_len) &&
	  (std::strncmp (filename, convfn, filename_len) == 0))
	return true;
    }
  return false;
}

/* Okay, now we write the entry_function for filename completion.  In the
   general case.  Note that completion in the shell is a little different
   because of all the pathnames that must be followed when looking up the
   completion for a command. */
char *
Readline::rl_filename_completion_function (const char *text, int state)
{
  unsigned int dirlen, dentlen, convlen;
  struct dirent *entry;

  /* If we don't have any state, then do some initialization. */
  if (state == 0)
    {
      /* If we were interrupted before closing the directory or reading
	 all of its contents, close it. */
      if (_rl_fcmp_directory)
	{
	  ::closedir (_rl_fcmp_directory);
	  _rl_fcmp_directory = nullptr;
	}
      _rl_fcmp_users_dirname.clear ();

      _rl_fcmp_filename = text;
      if (*text == 0)
	text = ".";
      _rl_fcmp_dirname = text;

      const char *temp = std::strrchr (_rl_fcmp_dirname.c_str (), '/');

#if defined (__MSDOS__) || defined (_WIN32)
      /* special hack for //X/... */
      if (dirname[0] == '/' && dirname[1] == '/' &&
	  std::isalpha (dirname[2]) && dirname[3] == '/')
        temp = std::strrchr (dirname + 3, '/');
#endif

      if (temp)
	{
	  _rl_fcmp_filename = ++temp;
	}
#if defined (__MSDOS__) || (defined (_WIN32) && !defined (__CYGWIN__))
      /* searches from current directory on the drive */
      else if (std::isalpha (dirname[0]) && dirname[1] == ':')
        {
          _rl_fcmp_filename = (dirname + 2);
        }
#endif
      else
	{
	  _rl_fcmp_dirname = '.';
	}

      /* We aren't done yet.  We also support the "~user" syntax. */

      /* Save the version of the directory that the user typed, dequoting
	 it if necessary. */
      if (rl_completion_found_quote && rl_filename_dequoting_function)
	{
	  char *tmp2 = ((*this).*rl_filename_dequoting_function)
			(_rl_fcmp_dirname.c_str (), rl_completion_quote_character);
	  _rl_fcmp_users_dirname = tmp2;
	  delete[] tmp2;
	}
      else
	_rl_fcmp_users_dirname = _rl_fcmp_dirname;

      bool tilde_dirname = false;
      if (_rl_fcmp_dirname[0] == '~')
	{
	  temp = tilde_expand (_rl_fcmp_dirname.c_str ());
	  _rl_fcmp_dirname = temp;
	  tilde_dirname = true;
	}

      /* We have saved the possibly-dequoted version of the directory name
	 the user typed.  Now transform the directory name we're going to
	 pass to opendir(2).  The directory rewrite hook modifies only the
	 directory name; the directory completion hook modifies both the
	 directory name passed to opendir(2) and the version the user
	 typed.  Both the directory completion and rewrite hooks should perform
	 any necessary dequoting.  The hook functions return 1 if they modify
	 the directory name argument.  If either hook returns 0, it should
	 not modify the directory name pointer passed as an argument. */
      if (rl_directory_rewrite_hook)
	((*this).*rl_directory_rewrite_hook) (_rl_fcmp_dirname);
      else if (rl_directory_completion_hook &&
		((*this).*rl_directory_completion_hook) (_rl_fcmp_dirname))
	{
	  _rl_fcmp_users_dirname = _rl_fcmp_dirname;
	}
      else if (!tilde_dirname && rl_completion_found_quote && rl_filename_dequoting_function)
	{
	  /* delete single and double quotes */
	  _rl_fcmp_dirname = _rl_fcmp_users_dirname;
	}

      _rl_fcmp_directory = ::opendir (_rl_fcmp_dirname.c_str ());

      /* Now dequote a non-null filename.  FILENAME will not be NULL, but may
	 be empty. */
      if (!_rl_fcmp_filename.empty () && rl_completion_found_quote && rl_filename_dequoting_function)
	{
	  /* delete single and double quotes */
	  temp = ((*this).*rl_filename_dequoting_function)
			(_rl_fcmp_filename.c_str (), rl_completion_quote_character);

	  _rl_fcmp_filename = temp;
	}

      rl_filename_completion_desired = true;
    }

  /* At this point we should entertain the possibility of hacking wildcarded
     filenames, like /usr/man/man<WILD>/te<TAB>.  If the directory name
     contains globbing characters, then build an array of directories, and
     then map over that list while completing. */
  /* *** UNIMPLEMENTED *** */

  /* Now that we have some state, we can read the directory. */

  entry = nullptr;
  char *dentry = nullptr, *convfn = nullptr;
  while (_rl_fcmp_directory && (entry = ::readdir (_rl_fcmp_directory)))
    {
      convfn = dentry = entry->d_name;
      convlen = dentlen = D_NAMLEN (entry);

      if (rl_filename_rewrite_hook)
	{
	  convfn = ((*this).*rl_filename_rewrite_hook) (dentry, dentlen);
	  convlen = (convfn == dentry) ? dentlen
				       : static_cast<unsigned int> (std::strlen (convfn));
	}

      /* Special case for no filename.  If the user has disabled the
         `match-hidden-files' variable, skip filenames beginning with `.'.
	 All other entries except "." and ".." match. */
      if (_rl_fcmp_filename.empty ())
	{
	  if (_rl_match_hidden_files == 0 && HIDDEN_FILE (convfn))
	    continue;

	  if (convfn[0] != '.' ||
	       (convfn[1] && (convfn[1] != '.' || convfn[2])))
	    break;
	}
      else
	{
	  if (complete_fncmp (convfn, convlen, _rl_fcmp_filename.c_str (),
			      static_cast<unsigned int> (_rl_fcmp_filename.size ())))
	    break;
	}
    }

  if (entry == nullptr)
    {
      if (_rl_fcmp_directory)
	{
	  ::closedir (_rl_fcmp_directory);
	  _rl_fcmp_directory = nullptr;
	}
      _rl_fcmp_dirname.clear ();
      _rl_fcmp_filename.clear ();
      _rl_fcmp_users_dirname.clear ();

      return nullptr;
    }
  else
    {
      char *temp;
      /* dirname && (strcmp (dirname, ".") != 0) */
      if (!_rl_fcmp_dirname.empty () && (_rl_fcmp_dirname[0] != '.' || _rl_fcmp_dirname[1]))
	{
	  if (rl_complete_with_tilde_expansion && _rl_fcmp_users_dirname[0] == '~')
	    {
	      dirlen = static_cast<unsigned int> (_rl_fcmp_dirname.size ());
	      temp = new char[2 + dirlen + D_NAMLEN (entry)];
	      std::strcpy (temp, _rl_fcmp_dirname.data ());
	      /* Canonicalization cuts off any final slash present.  We
		 may need to add it back. */
	      if (_rl_fcmp_dirname[dirlen - 1] != '/')
	        {
	          temp[dirlen++] = '/';
	          temp[dirlen] = '\0';
	        }
	    }
	  else
	    {
	      dirlen = static_cast<unsigned int> (_rl_fcmp_users_dirname.size ());
	      temp = new char[2 + dirlen + D_NAMLEN (entry)];
	      std::strcpy (temp, _rl_fcmp_users_dirname.data ());
	      /* Make sure that temp has a trailing slash here. */
	      if (_rl_fcmp_users_dirname[dirlen - 1] != '/')
		temp[dirlen++] = '/';
	    }

	  std::strcpy (temp + dirlen, convfn);
	}
      else
	temp = savestring (convfn);

      if (convfn != dentry)
	delete[] convfn;

      return temp;
    }
}

/* An initial implementation of a menu completion function a la tcsh.  The
   first time (if the last readline command was not rl_old_menu_complete), we
   generate the list of matches.  This code is very similar to the code in
   rl_complete_internal -- there should be a way to combine the two.  Then,
   for each item in the list of matches, we insert the match in an undoable
   fashion, with the appropriate character appended (this happens on the
   second and subsequent consecutive calls to rl_old_menu_complete).  When we
   hit the end of the match list, we restore the original unmatched text,
   ring the bell, and reset the counter to zero. */
int
Readline::rl_old_menu_complete (int count, int invoking_key)
{
  /* The first time through, we generate the list of matches and set things
     up to insert them. */
  if (rl_last_func != &Readline::rl_old_menu_complete)
    {
      /* Clean up from previous call, if any. */
      delete[] _rl_omenu_orig_text;
      _rl_omenu_orig_text = nullptr;

      if (_rl_omenu_matches)
	_rl_free_match_list (_rl_omenu_matches);

      _rl_omenu_match_list_index = _rl_omenu_match_list_size = 0;
      _rl_omenu_matches = nullptr;

      rl_completion_invoking_key = invoking_key;

      RL_SETSTATE(RL_STATE_COMPLETING);

      /* Only the completion entry function can change these. */
      set_completion_defaults ('%');

      rl_compentry_func_t our_func = rl_menu_completion_entry_function;
      if (our_func == nullptr)
	our_func = rl_completion_entry_function
			? rl_completion_entry_function
			: &Readline::rl_filename_completion_function;

      /* We now look backwards for the start of a filename/variable word. */
      _rl_omenu_orig_end = static_cast<unsigned int> (rl_point);

      rl_qf_flags found_quote = RL_QF_NONE;
      _rl_omenu_delimiter = '\0';
      _rl_omenu_quote_char = '\0';

      if (rl_point)
	/* This (possibly) changes rl_point.  If it returns a non-zero char,
	   we know we have an open quote. */
	_rl_omenu_quote_char = _rl_find_completion_word (&found_quote, &_rl_omenu_delimiter);

      _rl_omenu_orig_start = static_cast<unsigned int> (rl_point);
      rl_point = _rl_omenu_orig_end;

      _rl_omenu_orig_text = rl_copy_text (_rl_omenu_orig_start, _rl_omenu_orig_end);
      _rl_omenu_matches = gen_completion_matches (_rl_omenu_orig_text, _rl_omenu_orig_start, _rl_omenu_orig_end,
					our_func, found_quote, _rl_omenu_quote_char);

      /* If we are matching filenames, the attempted completion function will
	 have set rl_filename_completion_desired to a non-zero value.  The basic
	 rl_filename_completion_function does this. */
      bool matching_filenames = rl_filename_completion_desired;

      if (_rl_omenu_matches == nullptr ||
	  postprocess_matches (&_rl_omenu_matches, matching_filenames) == 0)
	{
	  rl_ding ();
	  delete[] _rl_omenu_matches;
	  _rl_omenu_matches = nullptr;

	  delete[] _rl_omenu_orig_text;
	  _rl_omenu_orig_text = nullptr;

	  _rl_cmpl_changed_buffer = false;
	  RL_UNSETSTATE(RL_STATE_COMPLETING);
	  return 0;
	}

      RL_UNSETSTATE(RL_STATE_COMPLETING);

      for (_rl_omenu_match_list_size = 0; _rl_omenu_matches[_rl_omenu_match_list_size];
	   _rl_omenu_match_list_size++)
        ;
      /* matches[0] is lcd if match_list_size > 1, but the circular buffer
	 code below should take care of it. */

      if (_rl_omenu_match_list_size > 1 && _rl_complete_show_all)
	display_matches (_rl_omenu_matches);
    }

  /* Now we have the list of matches.  Replace the text between
     rl_line_buffer[orig_start] and rl_line_buffer[rl_point] with
     matches[match_list_index], and add any necessary closing char. */

  if (_rl_omenu_matches == nullptr || _rl_omenu_match_list_size == 0)
    {
      rl_ding ();
      delete[] _rl_omenu_matches;
      _rl_omenu_matches = nullptr;
      _rl_cmpl_changed_buffer = false;
      return 0;
    }

  _rl_omenu_match_list_index += count;
  if (_rl_omenu_match_list_index < 0)
    {
      while (_rl_omenu_match_list_index < 0)
	_rl_omenu_match_list_index += _rl_omenu_match_list_size;
    }
  else
    _rl_omenu_match_list_index %= _rl_omenu_match_list_size;

  if (_rl_omenu_match_list_index == 0 && _rl_omenu_match_list_size > 1)
    {
      rl_ding ();
      insert_match (_rl_omenu_orig_text, _rl_omenu_orig_start, MULT_MATCH,
		    &_rl_omenu_quote_char);
    }
  else
    {
      insert_match (_rl_omenu_matches[_rl_omenu_match_list_index],
		    _rl_omenu_orig_start, SINGLE_MATCH, &_rl_omenu_quote_char);

      append_to_match (_rl_omenu_matches[_rl_omenu_match_list_index],
		       _rl_omenu_delimiter, _rl_omenu_quote_char,
		       compare_match (_rl_omenu_orig_text,
				      _rl_omenu_matches[_rl_omenu_match_list_index]));
    }

  _rl_cmpl_changed_buffer = true;
  return 0;
}

/* The current version of menu completion.
   The differences between this function and the original are:

1. It honors the maximum number of completions variable (completion-query-items)
2. It appends to the word as usual if there is only one match
3. It displays the common prefix if there is one, and makes it the first menu
   choice if the menu-complete-display-prefix option is enabled
*/

int
Readline::rl_menu_complete (int count, int)
{
  /* The first time through, we generate the list of matches and set things
     up to insert them. */
  if ((rl_last_func != &Readline::rl_menu_complete &&
	rl_last_func != &Readline::rl_backward_menu_complete) || _rl_menu_full_completion)
    {
      /* Clean up from previous call, if any. */
      delete[] _rl_menu_orig_text;
      if (_rl_menu_matches)
	_rl_free_match_list (_rl_menu_matches);

      _rl_menu_match_list_index = _rl_menu_match_list_size = 0;
      _rl_menu_matches = nullptr;

      _rl_menu_full_completion = false;

      RL_SETSTATE(RL_STATE_COMPLETING);

      /* Only the completion entry function can change these. */
      set_completion_defaults ('%');

      rl_compentry_func_t our_func = rl_menu_completion_entry_function;
      if (our_func == nullptr)
	our_func = rl_completion_entry_function
			? rl_completion_entry_function
			: &Readline::rl_filename_completion_function;

      /* We now look backwards for the start of a filename/variable word. */
      _rl_menu_orig_end = static_cast<unsigned int> (rl_point);

      rl_qf_flags found_quote = RL_QF_NONE;
      _rl_menu_delimiter = '\0';
      _rl_menu_quote_char = '\0';

      if (rl_point)
	/* This (possibly) changes rl_point.  If it returns a non-zero char,
	   we know we have an open quote. */
	_rl_menu_quote_char = _rl_find_completion_word (&found_quote, &_rl_menu_delimiter);

      _rl_menu_orig_start = static_cast<unsigned int> (rl_point);
      rl_point = _rl_menu_orig_end;

      _rl_menu_orig_text = rl_copy_text (_rl_menu_orig_start, _rl_menu_orig_end);
      _rl_menu_matches = gen_completion_matches (_rl_menu_orig_text, _rl_menu_orig_start,
						 _rl_menu_orig_end, our_func,
						 found_quote, _rl_menu_quote_char);

      _rl_menu_nontrivial_lcd = _rl_menu_matches && compare_match (_rl_menu_orig_text, _rl_menu_matches[0]) != 0;

      /* If we are matching filenames, the attempted completion function will
	 have set rl_filename_completion_desired to a non-zero value.  The basic
	 rl_filename_completion_function does this. */
      bool matching_filenames = rl_filename_completion_desired;

      if (_rl_menu_matches == nullptr ||
	  postprocess_matches (&_rl_menu_matches, matching_filenames) == 0)
	{
	  rl_ding ();
	  delete[] _rl_menu_matches;
	  _rl_menu_matches = nullptr;

	  delete[] _rl_menu_orig_text;
	  _rl_menu_orig_text = nullptr;

	  _rl_cmpl_changed_buffer = false;
	  RL_UNSETSTATE(RL_STATE_COMPLETING);
	  return 0;
	}

      RL_UNSETSTATE(RL_STATE_COMPLETING);

      for (_rl_menu_match_list_size = 0; _rl_menu_matches[_rl_menu_match_list_size];
	   _rl_menu_match_list_size++)
        ;

      if (_rl_menu_match_list_size == 0)
	{
	  rl_ding ();
	  delete[] _rl_menu_matches;
	  _rl_menu_matches = nullptr;
	  _rl_menu_match_list_index = 0;
	  _rl_cmpl_changed_buffer = false;
	  return 0;
        }

      /* matches[0] is lcd if match_list_size > 1, but the circular buffer
	 code below should take care of it. */
      if (*_rl_menu_matches[0])
	{
	  insert_match (_rl_menu_matches[0], _rl_menu_orig_start,
			_rl_menu_matches[1] ? MULT_MATCH : SINGLE_MATCH,
			&_rl_menu_quote_char);
	  _rl_menu_orig_end = _rl_menu_orig_start +
		static_cast<unsigned int> (std::strlen (_rl_menu_matches[0]));
	  _rl_cmpl_changed_buffer = !STREQ (_rl_menu_orig_text, _rl_menu_matches[0]);
	}

      if (_rl_menu_match_list_size > 1 && _rl_complete_show_all)
	{
	  display_matches (_rl_menu_matches);
	  /* If there are so many matches that the user has to be asked
	     whether or not he wants to see the matches, menu completion
	     is unwieldy. */
	  if (rl_completion_query_items > 0 &&
	      _rl_menu_match_list_size >= rl_completion_query_items)
	    {
	      rl_ding ();
	      delete[] _rl_menu_matches;
	      _rl_menu_matches = nullptr;

	      _rl_menu_full_completion = true;
	      return 0;
	    }
	  else if (_rl_menu_complete_prefix_first)
	    {
	      rl_ding ();
	      return 0;
	    }
	}
      else if (_rl_menu_match_list_size <= 1)
	{
	  append_to_match (_rl_menu_matches[0], _rl_menu_delimiter,
			   _rl_menu_quote_char, _rl_menu_nontrivial_lcd);
	  _rl_menu_full_completion = true;
	  return 0;
	}
      else if (_rl_menu_complete_prefix_first && _rl_menu_match_list_size > 1)
	{
	  rl_ding ();
	  return 0;
	}
    }

  /* Now we have the list of matches.  Replace the text between
     rl_line_buffer[orig_start] and rl_line_buffer[rl_point] with
     matches[match_list_index], and add any necessary closing char. */

  if (_rl_menu_matches == nullptr || _rl_menu_match_list_size == 0)
    {
      rl_ding ();
      delete[] _rl_menu_matches;
      _rl_menu_matches = nullptr;

      _rl_cmpl_changed_buffer = false;
      return 0;
    }

  _rl_menu_match_list_index += count;
  if (_rl_menu_match_list_index < 0)
    {
      while (_rl_menu_match_list_index < 0)
	_rl_menu_match_list_index += _rl_menu_match_list_size;
    }
  else
    _rl_menu_match_list_index %= _rl_menu_match_list_size;

  if (_rl_menu_match_list_index == 0 && _rl_menu_match_list_size > 1)
    {
      rl_ding ();
      insert_match (_rl_menu_matches[0], _rl_menu_orig_start, MULT_MATCH,
		    &_rl_menu_quote_char);
    }
  else
    {
      insert_match (_rl_menu_matches[_rl_menu_match_list_index],
		    _rl_menu_orig_start, SINGLE_MATCH, &_rl_menu_quote_char);

      append_to_match (_rl_menu_matches[_rl_menu_match_list_index],
		       _rl_menu_delimiter, _rl_menu_quote_char,
		       compare_match (_rl_menu_orig_text,
				      _rl_menu_matches[_rl_menu_match_list_index]));
    }

  _rl_cmpl_changed_buffer = true;
  return 0;
}

int
Readline::rl_backward_menu_complete (int count, int key)
{
  /* Positive arguments to backward-menu-complete translate into negative
     arguments for menu-complete, and vice versa. */
  return rl_menu_complete (-count, key);
}

}  // namespace readline
