/* mbschr.c - strchr(3) that handles multibyte characters. */

/* Copyright (C) 2002 Free Software Foundation, Inc.

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

#include <cstdlib>

#include "general.hh"
#include "shell.hh"
#include "shmbutil.hh"

namespace bash
{

/* In some locales, the non-first byte of some multibyte characters have
   the same value as some ascii character.  Faced with these strings, a
   legacy strchr() might return the wrong value. */

const char *
Shell::mbschr (const char *s, int c)
{
#if HANDLE_MULTIBYTE
  const char *pos;
  mbstate_t state;

  if (locale_utf8locale && c < 0x80)
    return utf8_mbschr (s, c); /* XXX */

  /* The locale encodings with said weird property are BIG5, BIG5-HKSCS,
     GBK, GB18030, SHIFT_JIS, and JOHAB.  They exhibit the problem only
     when c >= 0x30.  We can therefore use the faster bytewise search if
     c <= 0x30. */
  if (static_cast<unsigned char> (c) >= '0' && locale_mb_cur_max > 1)
    {
      pos = s;
      std::memset (&state, '\0', sizeof (mbstate_t));
      size_t strlength = std::strlen (s);

      while (strlength > 0)
        {
          size_t mblength;
          if (is_basic (*pos))
            mblength = 1;
          else
            {
              mblength = std::mbrlen (pos, strlength, &state);
              if (mblength == static_cast<size_t> (-2)
                  || mblength == static_cast<size_t> (-1) || mblength == 0)
                mblength = 1;
            }

          if (mblength == 1 && c == static_cast<unsigned char> (*pos))
            return pos;

          strlength -= mblength;
          pos += mblength;
        }

      return nullptr;
    }
  else
#endif
    return std::strchr (s, c);
}

} // namespace bash
