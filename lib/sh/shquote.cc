/* shquote - functions to quote and dequote strings */

/* Copyright (C) 1999-2020 Free Software Foundation, Inc.

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

#include "config.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <cstdio>
#include <cstdlib>

#include <string>

#include "syntax.hh"

#include "shmbchar.hh"
#include "shmbutil.hh"

#include "shell.hh"

namespace bash
{

// Default set of characters that should be backslash-quoted in strings.
static constexpr char bstab[256]
    = { 0, 0, 0, 0, 0, 0, 0, 0,

        0, 1, 1, 0, 0, 0, 0, 0, // TAB, NL

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

        1, 1, 1, 0, 1, 0, 1, 1, // SPACE, !, DQUOTE, DOL, AMP, SQUOTE

        1, 1, 1, 0, 1, 0, 0, 0, // LPAR, RPAR, STAR, COMMA

        0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 1, 1, 0, 1, 1, // SEMI, LESSTHAN, GREATERTHAN, QUEST

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 1, 1, 1, 1, 0, // LBRACK, BS, RBRACK, CARAT

        1, 0, 0, 0, 0, 0, 0, 0, // BACKQ

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 1, 1, 1, 0, 0, // LBRACE, BAR, RBRACE

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,

        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0 };

/* **************************************************************** */
/*								    */
/*	 Functions for quoting strings to be re-read as input	    */
/*								    */
/* **************************************************************** */

// Return a new string which is the single-quoted version of STRING.
// Used by alias and trap, among others.
std::string
sh_single_quote (const std::string &string)
{
  std::string result;
  result.reserve (2 + string.size ()); // result is at least this size

  if (string.size () == 1 && string[0] == '\'')
    {
      result.push_back ('\\');
      result.push_back ('\'');
      return result;
    }

  result.push_back ('\'');

  char c = 0;
  for (std::string::const_iterator it = string.begin ();
       (it != string.end ()) && (c = *it); ++it)
    {
      result.push_back (c);

      if (c == '\'')
        {
          result.push_back ('\\'); // insert escaped single quote
          result.push_back ('\'');
          result.push_back ('\''); // start new quoted string
        }
    }

  result.push_back ('\'');
  return result;
}

// Quote STRING using double quotes. Return a new string.
std::string
Shell::sh_double_quote (const std::string &string)
{
  DECLARE_MBSTATE;
  size_t mb_cur_max = MB_CUR_MAX;

  std::string result;
  result.reserve (2 + string.size ()); // result is at least this size

  result.push_back ('"');

  unsigned char c = 0;
  for (std::string::const_iterator it = string.begin ();
       (it != string.end ()) && (c = static_cast<unsigned char> (*it)); ++it)
    {
      // Backslash-newline disappears within double quotes, so don't add one.
      if ((sh_syntaxtab[c] & CBSDQUOTE) && c != '\n')
        result.push_back ('\\');

#if defined(HANDLE_MULTIBYTE)
      if ((locale_utf8locale && (c & 0x80))
          || (locale_utf8locale == 0 && mb_cur_max > 1 && is_basic (c) == 0))
        {
          append_char (result, it, string.end (), locale_mb_cur_max,
                       locale_utf8locale, state);
          continue;
        }
#endif

      // Assume that the string will not be further expanded, so no need to
      // add CTLESC to protect CTLESC or CTLNUL.
      result.push_back (static_cast<char> (c));
    }

  result.push_back ('"');
  return result;
}

// Turn S into a simple double-quoted string. If FLAGS is non-zero, quote
// double quote characters in S with backslashes.
std::string
Shell::sh_mkdoublequoted (const std::string &s, int flags)
{
  DECLARE_MBSTATE;
  size_t mb_cur_max = flags ? MB_CUR_MAX : 1;

  std::string result;
  result.reserve (2 + s.size ());

  result.push_back ('"');

  std::string::const_iterator it = s.begin ();
  while (it != s.end ())
    {
      if (flags && (*it) == '"')
        result.push_back ('\\');

#if defined(HANDLE_MULTIBYTE)
      if (flags
          && ((locale_utf8locale && ((*it) & 0x80))
              || (!locale_utf8locale && mb_cur_max > 1 && !is_basic (*it))))
        {
          append_char (result, it, s.end (), locale_mb_cur_max,
                       locale_utf8locale, state);
          continue;
        }
#endif
      result.push_back (*(it++));
    }

  result.push_back ('"');
  return result;
}

// Remove backslashes that are quoting characters that are special between
// double quotes. Return a new string.
//
// XXX - should this handle CTLESC and CTLNUL?
std::string
Shell::sh_un_double_quote (const std::string &string)
{
  std::string result;
  result.reserve (string.size ());

  bool pass_next = false;
  char c = 0;
  for (std::string::const_iterator it = string.begin ();
       (it != string.end ()) && (c = *it); ++it)
    {
      if (pass_next)
        {
          result.push_back (c);
          pass_next = false;
          continue;
        }
      if (c == '\\' && (it + 1 != string.end ())
          && (sh_syntaxtab[static_cast<unsigned char> (*(it + 1))]
              & CBSDQUOTE))
        {
          pass_next = true;
          continue;
        }
      result.push_back (c);
    }

  return result;
}

// Quote special characters in STRING using backslashes. Return a new
// string. TABLE, if set, points to a map of the ascii code set with char
// needing to be backslash-quoted if table[char] == 1. FLAGS, if 1, causes
// tildes to be quoted as well. If FLAGS & 2, backslash-quote other shell blank
// characters.
//
// NOTE: if the string is to be further expanded, we need a way to protect the
// CTLESC and CTLNUL characters. As I write this, the current callers will
// never cause the string to be expanded without going through the shell
// parser, which will protect the internal quoting characters.
std::string
Shell::sh_backslash_quote (const std::string &string, const char *table,
                           int flags)
{
  DECLARE_MBSTATE;
  size_t mb_cur_max = MB_CUR_MAX;

  std::string result;

  const char *backslash_table = table ? table : bstab;

  unsigned char c = 0;
  for (std::string::const_iterator it = string.begin ();
       (it != string.end ()) && (c = static_cast<unsigned char> (*it)); ++it)
    {
#if defined(HANDLE_MULTIBYTE)
      /* XXX - isascii, even if is_basic(c) == 0 - works in most cases. */
      if (c >= 0 && c <= 127
          && backslash_table[static_cast<unsigned char> (c)] == 1)
        {
          result.push_back ('\\');
          result.push_back (c);
          continue;
        }
      if ((locale_utf8locale && (c & 0x80))
          || (locale_utf8locale == 0 && mb_cur_max > 1 && is_basic (c) == 0))
        {
          append_char (result, it, string.end (), locale_mb_cur_max,
                       locale_utf8locale, state);
          it--; // compensate for auto-increment in loop above
          continue;
        }
#endif
      if (backslash_table[static_cast<unsigned char> (c)] == 1)
        result.push_back ('\\');
      else if (c == '#' && (it == string.begin ())) /* comment char */
        result.push_back ('\\');
      else if ((flags & 1) && c == '~'
               && ((it == string.begin ()) || (*(it - 1)) == ':'
                   || (*(it - 1)) == '='))
        // Tildes are special at the start of a word or after a `:' or `='
        // (technically unquoted, but it doesn't make a difference in practice)
        result.push_back ('\\');
      else if ((flags & 2) && shellblank (c))
        result.push_back ('\\');

      result.push_back (static_cast<char> (c));
    }

  return result;
}

#if defined(PROMPT_STRING_DECODE)
// Quote characters that get special treatment when in double quotes in STRING
// using backslashes. Return a new string.
std::string
Shell::sh_backslash_quote_for_double_quotes (const std::string &string)
{
  DECLARE_MBSTATE;
  size_t mb_cur_max = MB_CUR_MAX;

  std::string result;

  char c = 0;
  for (std::string::const_iterator it = string.begin ();
       it != string.end () && (c = *it); ++it)
    {
      // Backslash-newline disappears within double quotes, so don't add one.
      if ((sh_syntaxtab[static_cast<unsigned char> (c)] & CBSDQUOTE)
          && c != '\n')
        result.push_back ('\\');
      /* I should probably use the CSPECL flag for these in sh_syntaxtab[] */
      else if (c == CTLESC || c == CTLNUL)
        result.push_back (CTLESC); /* could be '\\'? */

#if defined(HANDLE_MULTIBYTE)
      if ((locale_utf8locale && (c & 0x80))
          || (!locale_utf8locale && mb_cur_max > 1 && !is_basic (c)))
        {
          append_char (result, it, string.end (), locale_mb_cur_max,
                       locale_utf8locale, state);
          it--; // compensate for auto-increment in loop above
          continue;
        }
#endif

      result.push_back (c);
    }

  return result;
}
#endif /* PROMPT_STRING_DECODE */

std::string
sh_quote_reusable (const std::string &s, int flags)
{
  if (s.empty ())
    {
      return std::string ("\'\'");
    }
  else if (ansic_shouldquote (s))
    return ansic_quote (s);
  else if (flags)
    return sh_backslash_quote (s, nullptr, 1);
  else
    return sh_single_quote (s);
}

bool
sh_contains_shell_metas (const std::string &string)
{
  for (std::string::const_iterator it = string.begin (); it != string.end ();
       ++it)
    {
      switch (*it)
        {
        case ' ':
        case '\t':
        case '\n': /* IFS white space */
        case '\'':
        case '"':
        case '\\': /* quoting chars */
        case '|':
        case '&':
        case ';': /* shell metacharacters */
        case '(':
        case ')':
        case '<':
        case '>':
        case '!':
        case '{':
        case '}': /* reserved words */
        case '*':
        case '[':
        case '?':
        case ']': /* globbing chars */
        case '^':
        case '$':
        case '`': /* expansion chars */
          return true;
        case '~': /* tilde expansion */
          if (it == string.begin () || *(it - 1) == '=' || *(it - 1) == ':')
            return true;
          break;
        case '#':
          if (it == string.begin ()) /* comment char */
            return true;
          __attribute__ ((fallthrough));
          /* FALLTHROUGH */
        default:
          break;
        }
    }

  return false;
}

} // namespace bash
