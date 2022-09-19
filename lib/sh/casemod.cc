/* casemod.c -- functions to change case of strings */

/* Copyright (C) 2008-2020 Free Software Foundation, Inc.

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
#endif /* HAVE_UNISTD_H */

#include "bashintl.hh"
#include "bashtypes.hh"
#include "externs.hh"
#include "shell.hh"

#include "chartypes.hh"
#include "shmbchar.hh"
#include "shmbutil.hh"
#include "typemax.hh"

#include "strmatch.hh"

#include "externs.hh"

namespace bash
{

#if !defined(HANDLE_MULTIBYTE)
// XXX the non-multibyte code path will need updating
#define cval(s, i) ((s)[(i)])
#define iswalnum(c) isalnum (c)
#define TOGGLE(x)                                                             \
  (std::isupper (x) ? std::tolower ((unsigned char)x) : (std::toupper (x)))
#else

static inline wchar_t
_to_wupper (wchar_t x)
{
  wint_t wc = static_cast<wint_t> (x);
  return static_cast<wchar_t> (std::iswlower (wc) ? std::towupper (wc) : wc);
}

static inline wchar_t
_to_wlower (wchar_t x)
{
  wint_t wc = static_cast<wint_t> (x);
  return static_cast<wchar_t> (std::iswupper (wc) ? std::towlower (wc) : wc);
}

static inline wchar_t
TOGGLE (wchar_t x)
{
  wint_t wc = static_cast<wint_t> (x);
  if (std::iswupper (wc))
    return static_cast<wchar_t> (std::towlower (wc));
  else if (std::iswlower (wc))
    return static_cast<wchar_t> (std::towupper (wc));
  else
    return x;
}

#endif

#if defined(HANDLE_MULTIBYTE)
static inline wchar_t
cval (const std::string::const_iterator it,
      const std::string::const_iterator end)
{
  size_t tmp;
  wchar_t wc;
  size_t l;
  mbstate_t mps;

  if (MB_CUR_MAX == 1 || is_basic (static_cast<unsigned char> (*it)))
    return static_cast<wchar_t> (*it);

  std::memset (&mps, 0, sizeof (mbstate_t));
  tmp = std::mbrtowc (&wc, &(*it), static_cast<size_t> (end - it), &mps);

  if (MB_INVALIDCH (tmp) || MB_NULLWCH (tmp))
    return static_cast<wchar_t> (*it);

  return wc;
}
#endif

// Modify the case of characters in STRING matching PAT based on the value of
// FLAGS. If PAT is null, modify the case of each character.
std::string
Shell::sh_modcase (const std::string &string, const char *pat,
                   sh_modcase_flags flags)
{
#if defined(HANDLE_MULTIBYTE)
  char mb[MB_LEN_MAX + 1];
  mbstate_t state;
#endif

  if (string.empty ())
    {
      return std::string ();
    }

#if defined(HANDLE_MULTIBYTE)
  std::memset (&state, 0, sizeof (mbstate_t));
#endif

  std::string::const_iterator it = string.begin ();
  std::string::const_iterator end = string.end ();
  size_t mb_cur_max = MB_CUR_MAX;

  std::string ret;
  ret.reserve (string.size ());

  /* See if we are supposed to split on alphanumerics and operate on each word
   */
  bool usewords = (flags & CASE_USEWORDS);
  flags &= ~CASE_USEWORDS;

  bool inword = false;
  while (it != end)
    {
      wchar_t wc = cval (it, end);

      if (std::iswalnum (static_cast<wint_t> (wc)) == 0)
        inword = false;

      if (pat)
        {
          size_t next = start;
          ADVANCE_CHAR (string, end, next);

          char *s = substring (string, start, next);
          bool match = (strmatch (pat, s, FNM_EXTMATCH) != FNM_NOMATCH);
          delete[] s;

          if (!match)
            {
              /* copy unmatched portion */
              std::memcpy (ret + retind, string + start, next - start);
              retind += next - start;
              start = next;
              inword = true;
              continue;
            }
        }

      sh_modcase_flags nop;

      /* XXX - for now, the toggling operators work on the individual
         words in the string, breaking on alphanumerics.  Should I
         leave the capitalization operators to do that also? */
      if (flags == CASE_CAPITALIZE)
        {
          if (usewords)
            nop = inword ? CASE_LOWER : CASE_UPPER;
          else
            nop = (it != string.begin ()) ? CASE_LOWER : CASE_UPPER;
          inword = true;
        }
      else if (flags == CASE_UNCAP)
        {
          if (usewords)
            nop = inword ? CASE_UPPER : CASE_LOWER;
          else
            nop = (it != string.begin ()) ? CASE_UPPER : CASE_LOWER;
          inword = true;
        }
      else if (flags == CASE_UPFIRST)
        {
          if (usewords)
            nop = inword ? CASE_NOOP : CASE_UPPER;
          else
            nop = (it != string.begin ()) ? CASE_NOOP : CASE_UPPER;
          inword = true;
        }
      else if (flags == CASE_LOWFIRST)
        {
          if (usewords)
            nop = inword ? CASE_NOOP : CASE_LOWER;
          else
            nop = (it != string.begin ()) ? CASE_NOOP : CASE_LOWER;
          inword = true;
        }
      else if (flags == CASE_TOGGLE)
        {
          nop = inword ? CASE_NOOP : CASE_TOGGLE;
          inword = true;
        }
      else
        nop = flags;

      /* Can't short-circuit, some locales have multibyte upper and lower
         case equivalents of single-byte ascii characters (e.g., Turkish) */
      if (mb_cur_max == 1)
        {
        singlebyte:
          wchar_t nc;
          switch (nop)
            {
            default:
            case CASE_NOOP:
              nc = wc;
              break;
            case CASE_UPPER:
              nc = std::toupper (wc);
              break;
            case CASE_LOWER:
              nc = std::tolower (wc);
              break;
            case CASE_TOGGLEALL:
            case CASE_TOGGLE:
              nc = TOGGLE (wc);
              break;
            }
          ret.push_back (static_cast<char> (nc));
        }
#if defined(HANDLE_MULTIBYTE)
      else
        {
          size_t m = std::mbrtowc (&wc, string + start, end - start, &state);
          /* Have to go through wide case conversion even for single-byte
             chars, to accommodate single-byte characters where the
             corresponding upper or lower case equivalent is multibyte. */
          if (MB_INVALIDCH (m))
            {
              wc = static_cast<unsigned char> (string[start]);
              goto singlebyte;
            }
          else if (MB_NULLWCH (m))
            wc = L'\0';

          wchar_t nwc;
          switch (nop)
            {
            default:
            case CASE_NOOP:
              nwc = wc;
              break;
            case CASE_UPPER:
              nwc = _to_wupper (wc);
              break;
            case CASE_LOWER:
              nwc = _to_wlower (wc);
              break;
            case CASE_TOGGLEALL:
            case CASE_TOGGLE:
              nwc = TOGGLE (wc);
              break;
            }

          /* We don't have to convert `wide' characters that are in the
             unsigned char range back to single-byte `multibyte' characters. */
          if (static_cast<int> (nwc) <= UCHAR_MAX
              && is_basic (static_cast<char> (nwc)))
            ret.push_back (static_cast<char> (nwc));
          else
            {
              size_t mlen = std::wcrtomb (mb, nwc, &state);
              if (mlen > 0)
                mb[mlen] = '\0';
              /* Don't assume the same width */
              std::strncpy (ret + retind, mb, mlen);
              retind += mlen;
            }
        }
#endif

      ADVANCE_CHAR (string, end, start);
    }

  return ret;
}

} // namespace bash
