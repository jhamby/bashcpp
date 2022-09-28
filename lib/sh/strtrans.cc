/* strtrans.c - Translate and untranslate strings with ANSI-C escape sequences.
 */

/* Copyright (C) 2000-2015 Free Software Foundation, Inc.

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

#include "chartypes.hh"
#include "shell.hh"

#include "shmbchar.hh"
#include "shmbutil.hh"

namespace bash
{

#ifdef ESC
#undef ESC
#endif
#define ESC '\033' /* ASCII */

/* Convert STRING by expanding the escape sequences specified by the
   ANSI C standard.  If SAWC is non-null, recognize `\c' and use that
   as a string terminator.  If we see \c, set *SAWC to 1 before
   returning.  LEN is the length of STRING.  If (FLAGS & 1) is non-zero,
   that we're translating a string for `echo -e', and therefore should not
   treat a single quote as a character that may be escaped with a backslash.
   If (FLAGS & 2) is non-zero, we're expanding for the parser and want to
   quote CTLESC and CTLNUL with CTLESC.  If (flags & 4) is non-zero, we want
   to remove the backslash before any unrecognized escape sequence. */
std::string
Shell::ansicstr (const std::string &string, int flags, bool *sawc)
{
  int temp;
  uint32_t v;
#if defined(HANDLE_MULTIBYTE)
  wchar_t wc;
#endif

  if (string.empty ())
    return std::string ();

  size_t mb_cur_max = MB_CUR_MAX;
  std::string ret;

  std::string::const_iterator s;
  for (s = string.begin (); s != string.end ();)
    {
      unsigned char c = static_cast<unsigned char> (*s++);
      if (c != '\\' || s == string.end ())
        {
          size_t clen = 1;
#if defined(HANDLE_MULTIBYTE)
          if ((locale_utf8locale && (c & 0x80))
              || (locale_utf8locale == 0 && mb_cur_max > 0
                  && is_basic (c) == 0))
            {
              clen = std::mbrtowc (&wc, &(*(s - 1)), mb_cur_max, 0);
              if (MB_INVALIDCH (clen))
                clen = 1;
            }
#endif
          ret.push_back (static_cast<char> (c));
          for (--clen; clen > 0; clen--)
            ret.push_back (*s++);
        }
      else
        {
          switch (c = static_cast<unsigned char> (*s++))
            {
            case 'a':
              c = '\a';
              break;
            case 'v':
              c = '\v';
              break;
            case 'b':
              c = '\b';
              break;
            case 'e':
            case 'E': /* ESC -- non-ANSI */
              c = ESC;
              break;
            case 'f':
              c = '\f';
              break;
            case 'n':
              c = '\n';
              break;
            case 'r':
              c = '\r';
              break;
            case 't':
              c = '\t';
              break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
#if 1
              if (flags & 1)
                {
                  ret.push_back ('\\');
                  break;
                }
              __attribute__ ((fallthrough));
              /*FALLTHROUGH*/
#endif
            case '0':
              /* If (FLAGS & 1), we're translating a string for echo -e (or
                 the equivalent xpg_echo option), so we obey the SUSv3/
                 POSIX-2001 requirement and accept 0-3 octal digits after
                 a leading `0'. */
              temp = 2 + ((flags & 1) && (c == '0'));
              for (c -= '0'; isoctal (*s) && temp--; s++)
                c = (c * 8) + octvalue (*s);
              break;
            case 'x': /* Hex digit -- non-ANSI */
              if ((flags & 2) && *s == '{')
                {
                  flags |= 16; /* internal flag value */
                  s++;
                }
              /* Consume at least two hex characters */
              for (temp = 2, c = 0;
                   std::isxdigit ((unsigned char)*s) && temp--; s++)
                c = (c * 16) + hexvalue (*s);
              /* DGK says that after a `\x{' ksh93 consumes ISXDIGIT chars
                 until a non-xdigit or `}', so potentially more than two
                 chars are consumed. */
              if (flags & 16)
                {
                  for (; std::isxdigit (static_cast<unsigned char> (*s)); s++)
                    c = (c * 16) + hexvalue (*s);
                  flags &= ~16;
                  if (*s == '}')
                    s++;
                }
              /* \x followed by non-hex digits is passed through unchanged */
              else if (temp == 2)
                {
                  ret.push_back ('\\');
                  c = 'x';
                }
              break;
#if defined(HANDLE_MULTIBYTE)
            case 'u':
            case 'U':
              temp = (c == 'u') ? 4 : 8; /* \uNNNN \UNNNNNNNN */
              for (v = 0;
                   std::isxdigit (static_cast<unsigned char> (*s)) && temp--;
                   s++)
                v = (v * 16) + hexvalue (*s);
              if (temp == ((c == 'u') ? 4 : 8))
                {
                  ret.push_back ('\\'); /* c remains unchanged */
                  break;
                }
              else if (v <= 0x7f) /* <= 0x7f translates directly */
                {
                  c = v;
                  break;
                }
              else
                {
                  u32cconv (v, ret);
                  continue;
                }
#endif
            case '\\':
              break;
            case '\'':
            case '"':
            case '?':
              if (flags & 1)
                ret.push_back ('\\');
              break;
            case 'c':
              if (sawc)
                {
                  *sawc = true;
                  return std::string ();
                }
              else if ((flags & 1) == 0 && *s == 0)
                ; /* pass \c through */
              else if ((flags & 1) == 0 && (c = *s))
                {
                  s++;
                  if ((flags & 2) && c == '\\' && c == *s)
                    s++; /* Posix requires $'\c\\' do backslash escaping */
                  c = toctrl (c);
                  break;
                }
              __attribute__ ((fallthrough));
              /*FALLTHROUGH*/
            default:
              if ((flags & 4) == 0)
                ret.push_back ('\\');
              break;
            }
          if ((flags & 2) && (c == CTLESC || c == CTLNUL))
            ret.push_back (CTLESC);
          ret.push_back (static_cast<char> (c));
        }
    }

  return ret;
}

/* Take a string STR, possibly containing non-printing characters, and turn it
   into a $'...' ANSI-C style quoted string.  Returns a new string. */
std::string
ansic_quote (const std::string &str)
{
  unsigned char c;
  size_t clen;
#if defined(HANDLE_MULTIBYTE)
  wchar_t wc;
#endif

  if (str.empty ())
    return str;

  std::string ret;
  ret.reserve (str.size () + 3);

  ret.push_back ('$');
  ret.push_back ('\'');

  std::string::const_iterator it;
  for (it = str.begin (); it != str.end (); ++it)
    {
      c = static_cast<unsigned char> (*it);
      bool b = true, l = true; // 1 == add backslash; 0 == no backslash
      clen = 1;

      switch (c)
        {
        case ESC:
          c = 'E';
          break;
        case '\a':
          c = 'a';
          break;
        case '\v':
          c = 'v';
          break;

        case '\b':
          c = 'b';
          break;
        case '\f':
          c = 'f';
          break;
        case '\n':
          c = 'n';
          break;
        case '\r':
          c = 'r';
          break;
        case '\t':
          c = 't';
          break;
        case '\\':
        case '\'':
          break;
        default:
#if defined(HANDLE_MULTIBYTE)
          b = is_basic (c);
          /* XXX - clen comparison to 0 is dicey */
          if ((!b
               && ((clen = std::mbrtowc (&wc, &(*it), MB_CUR_MAX, 0))
                       == static_cast<size_t> (-1)
                   || MB_INVALIDCH (clen) || std::iswprint (wc) == 0))
              || (b && std::isprint (c) == 0))
#else
          if (std::isprint (c) == 0)
#endif
            {
              ret.push_back ('\\');
              ret.push_back ((c >> 6) & 07);
              ret.push_back ((c >> 3) & 07);
              ret.push_back (c & 07);
              continue;
            }
          l = 0;
          break;
        }
      if (b == 0 && clen == 0)
        break;

      if (l)
        ret.push_back ('\\');

      if (clen == 1)
        ret.push_back (c);
      else
        {
          for (b = 0; b < (int)clen; b++)
            *r++ = static_cast<unsigned char> (s[b]);
          s += clen - 1; /* -1 because of the increment above */
        }
    }

  ret.push_back ('\'');
  return ret;
}

#if defined(HANDLE_MULTIBYTE)
static inline bool
ansic_wshouldquote (const char *string)
{
  const wchar_t *wcs;
  wchar_t wcc;
  wchar_t *wcstr = NULL;
  size_t slen;

  slen = std::mbstowcs (wcstr, string, 0);

  if (slen == static_cast<size_t> (-1))
    return true;

  wcstr = new wchar_t[slen + 1];
  std::mbstowcs (wcstr, string, slen + 1);

  for (wcs = wcstr; (wcc = *wcs); wcs++)
    if (std::iswprint (wcc) == 0)
      {
        delete[] wcstr;
        return true;
      }

  delete[] wcstr;
  return false;
}
#endif

/* return 1 if we need to quote with $'...' because of non-printing chars. */
bool
ansic_shouldquote (const std::string &string)
{
  std::string::const_iterator s;
  for (s = string.begin (); s != string.end (); ++s)
    {
      unsigned char c = static_cast<unsigned char> (*s);
#if defined(HANDLE_MULTIBYTE)
      if (!is_basic (c))
        return ansic_wshouldquote (string.c_str () + (s - string.begin ()));
#endif
      if (std::isprint (c) == 0)
        return true;
    }

  return false;
}

// $'...' ANSI-C expand the portion of string between begin and end and return
// the result as a new string.
std::string
Shell::ansiexpand (std::string::const_iterator begin,
                   std::string::const_iterator end)
{
  char *temp, *t;
  unsigned int len, tlen;

  temp = new char[end + 1 - start];
  for (tlen = 0, len = start; len < end;)
    temp[tlen++] = string[len++];
  temp[tlen] = '\0';

  if (*temp)
    {
      t = ansicstr (temp, tlen, 2, nullptr, lenp);
      delete[] temp;
      return t;
    }
  else
    {
      if (lenp)
        *lenp = 0;
      return temp;
    }
}

} // namespace bash
