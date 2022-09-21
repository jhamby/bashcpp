/* mbutil.c -- readline multibyte character utility functions */

/* Copyright (C) 2001-2020 Free Software Foundation, Inc.

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

#include <fcntl.h>
#include <sys/types.h>

#if defined(TIOCSTAT_IN_SYS_IOCTL)
#include <sys/ioctl.h>
#endif /* TIOCSTAT_IN_SYS_IOCTL */

namespace readline
{

/* **************************************************************** */
/*								    */
/*		Multibyte Character Utility Functions		    */
/*								    */
/* **************************************************************** */

#if defined(HANDLE_MULTIBYTE)

size_t
History::_rl_find_next_mbchar_internal (const std::string &string, size_t seed,
                                        int count,
                                        find_mbchar_flags find_non_zero)
{
  mbstate_t ps;
  size_t point;
  wchar_t wc;

  size_t tmp = 0;

  std::memset (&ps, 0, sizeof (mbstate_t));
  if (count <= 0)
    return seed;

  point = seed + _rl_adjust_point (string, seed, &ps);
  /* if _rl_adjust_point returns -1, the character or string is invalid.
     treat as a byte. */
  if (point == seed - 1) /* invalid */
    return seed + 1;

  /* if this is true, means that seed was not pointing to a byte indicating
     the beginning of a multibyte character.  Correct the point and consume
     one char. */
  if (seed < point)
    count--;

  while (count > 0)
    {
      size_t len = string.size () - point;
      if (len == 0)
        break;
      if (_rl_utf8locale && UTF8_SINGLEBYTE (string[point]))
        {
          tmp = 1;
          wc = static_cast<wchar_t> (string[point]);
          std::memset (&ps, 0, sizeof (mbstate_t));
        }
      else
        tmp = std::mbrtowc (&wc, string.c_str () + point, len, &ps);
      if (MB_INVALIDCH (tmp))
        {
          /* invalid bytes. assume a byte represents a character */
          point++;
          count--;
          /* reset states. */
          std::memset (&ps, 0, sizeof (mbstate_t));
        }
      else if (MB_NULLWCH (tmp))
        break; /* found wide '\0' */
      else
        {
          /* valid bytes */
          point += tmp;
          if (find_non_zero)
            {
              if (WCWIDTH (wc) == 0)
                continue;
              else
                count--;
            }
          else
            count--;
        }
    }

  if (find_non_zero)
    {
      tmp = std::mbrtowc (&wc, string.c_str () + point, string.size () - point,
                          &ps);
      while (MB_NULLWCH (tmp) == 0 && MB_INVALIDCH (tmp) == 0
             && WCWIDTH (wc) == 0)
        {
          point += tmp;
          tmp = std::mbrtowc (&wc, string.c_str () + point,
                              string.size () - point, &ps);
        }
    }

  return point;
}

/* experimental -- needs to handle zero-width characters better */
size_t
History::_rl_find_prev_utf8char (const std::string &string, size_t seed,
                                 find_mbchar_flags find_non_zero)
{
  unsigned char b;
  int save, prev;
  size_t len = string.size ();

  prev = static_cast<int> (seed - 1);
  while (prev >= 0)
    {
      b = static_cast<unsigned char> (string[static_cast<size_t> (prev)]);
      if (UTF8_SINGLEBYTE (b))
        return static_cast<size_t> (prev);

      save = prev;

      /* Move back until we're not in the middle of a multibyte char */
      if (UTF8_MBCHAR (b))
        {
          while (prev > 0
                 && (b = static_cast<unsigned char> (
                         string[static_cast<size_t> (--prev)]))
                 && UTF8_MBCHAR (b))
            ;
        }

      if (UTF8_MBFIRSTCHAR (b))
        {
          if (find_non_zero)
            {
              if (_rl_test_nonzero (string.c_str (),
                                    static_cast<size_t> (prev), len))
                return static_cast<size_t> (prev);
              else /* valid but WCWIDTH (wc) == 0 */
                --prev;
            }
          else
            return static_cast<size_t> (prev);
        }
      else
        return static_cast<size_t> (save); // invalid utf-8 multibyte sequence
    }

  return (prev < 0) ? 0 : static_cast<size_t> (prev);
}

size_t
History::_rl_find_prev_mbchar_internal (const std::string &string, size_t seed,
                                        find_mbchar_flags find_non_zero)
{
  mbstate_t ps;
  size_t prev, non_zero_prev, point;
  size_t tmp;
  wchar_t wc;

  if (_rl_utf8locale)
    return _rl_find_prev_utf8char (string, seed, find_non_zero);

  std::memset (&ps, 0, sizeof (mbstate_t));
  size_t length = string.size ();

  if (length < seed)
    return length;

  prev = non_zero_prev = point = 0;
  while (point < seed)
    {
      if (_rl_utf8locale && UTF8_SINGLEBYTE (string[point]))
        {
          tmp = 1;
          wc = static_cast<wchar_t> (string[point]);
          std::memset (&ps, 0, sizeof (mbstate_t));
        }
      else
        tmp = std::mbrtowc (&wc, &string[point], length - point, &ps);
      if (MB_INVALIDCH (tmp))
        {
          /* in this case, bytes are invalid or too short to compose
             multibyte char, so assume that the first byte represents
             a single character anyway. */
          tmp = 1;
          /* clear the state of the byte sequence, because
             in this case effect of mbstate is undefined  */
          std::memset (&ps, 0, sizeof (mbstate_t));

          /* Since we're assuming that this byte represents a single
             non-zero-width character, don't forget about it. */
          prev = point;
        }
      else if (MB_NULLWCH (tmp))
        break; /* Found '\0' char.  Can this happen? */
      else
        {
          if (find_non_zero)
            {
              if (WCWIDTH (wc) != 0)
                prev = point;
            }
          else
            prev = point;
        }

      point += tmp;
    }

  return prev;
}

/* return the number of bytes parsed from the multibyte sequence starting
   at src, if a non-L'\0' wide character was recognized. It returns 0,
   if a L'\0' wide character was recognized. It returns -1 if an invalid
   multibyte sequence was encountered. It returns -2 if it couldn't parse
   a complete multibyte character. */
ssize_t
History::_rl_get_char_len (const std::string &src, mbstate_t *ps)
{
  /* Look at no more than MB_CUR_MAX characters */
  size_t l = src.size ();

  size_t tmp;
  if (l == 0)
    tmp = 0;
  else if (_rl_utf8locale && UTF8_SINGLEBYTE (src[0]))
    tmp = 1;
  else
    {
      size_t mb_cur_max = MB_CUR_MAX;
      tmp = std::mbrlen (src.c_str (), (l < mb_cur_max) ? l : mb_cur_max, ps);
    }

  if (tmp == static_cast<size_t> (-2))
    {
      /* too short to compose multibyte char */
      if (ps)
        std::memset (ps, 0, sizeof (mbstate_t));
      return -2;
    }
  else if (tmp == static_cast<size_t> (-1))
    {
      /* invalid to compose multibyte char */
      /* initialize the conversion state */
      if (ps)
        std::memset (ps, 0, sizeof (mbstate_t));
      return -1;
    }
  else
    return static_cast<int> (tmp);
}

/* adjust pointed byte and find mbstate of the point of string.
   adjusted point will be point <= adjusted_point, and returns
   differences of the byte(adjusted_point - point).
   if point is invalid (greater than string length),
   it returns (size_t)(-1). */
size_t
History::_rl_adjust_point (const std::string &string, size_t point,
                           mbstate_t *ps)
{
  size_t length = string.size ();

  if (point > length)
    return static_cast<size_t> (-1);

  size_t pos = 0;
  while (pos < point)
    {
      size_t tmp;
      if (_rl_utf8locale && UTF8_SINGLEBYTE (string[pos]))
        tmp = 1;
      else
        tmp = std::mbrlen (string.c_str () + pos, length - pos, ps);

      if (MB_INVALIDCH (tmp))
        {
          /* in this case, bytes are invalid or too short to compose
             multibyte char, so assume that the first byte represents
             a single character anyway. */
          pos++;
          /* clear the state of the byte sequence, because
             in this case effect of mbstate is undefined  */
          if (ps)
            std::memset (ps, 0, sizeof (mbstate_t));
        }
      else if (MB_NULLWCH (tmp))
        pos++;
      else
        pos += tmp;
    }

  return pos - point;
}

wchar_t
History::_rl_char_value (const std::string &buf, size_t ind)
{
  if (MB_LEN_MAX == 1 || rl_byte_oriented)
    return static_cast<wchar_t> (buf[ind]);

  if (_rl_utf8locale && UTF8_SINGLEBYTE (buf[ind]))
    return static_cast<wchar_t> (buf[ind]);

  size_t l = buf.size ();
  if (ind >= l - 1)
    return static_cast<wchar_t> (buf[ind]);

  mbstate_t ps;
  std::memset (&ps, 0, sizeof (mbstate_t));

  wchar_t wc;
  size_t tmp = std::mbrtowc (&wc, buf.c_str () + ind, l - ind, &ps);

  if (MB_INVALIDCH (tmp) || MB_NULLWCH (tmp))
    return static_cast<wchar_t> (buf[ind]);

  return wc;
}
#endif /* HANDLE_MULTIBYTE */

} // namespace readline
