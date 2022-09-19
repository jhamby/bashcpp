/* Copyright (C) 2001, 2006, 2009, 2010, 2012, 2015-2018 Free Software
   Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "config.hh"

#if defined(HANDLE_MULTIBYTE)
#include <climits>
#include <cstdlib>

#include <cerrno>

#include "general.hh"
#include "shell.hh"
#include "shmbchar.hh"
#include "shmbutil.hh"

namespace bash
{

#if defined(IS_BASIC_ASCII)

// Bit table of characters in the ISO C "basic character set".
constexpr unsigned int is_basic_table[UCHAR_MAX / 32 + 1] = {
  0x00001a00, // '\t' '\v' '\f'
  0xffffffef, // ' '...'#' '%'...'?'
  0xfffffffe, // 'A'...'Z' '[' '\\' ']' '^' '_'
  0x7ffffffe  // 'a'...'z' '{' '|' '}' '~'
  // The remaining bits are 0.
};

#endif /* IS_BASIC_ASCII */

// Count the number of characters in S, counting multi-byte characters as a
// single character.
size_t
mbstrlen (const char *s)
{
  size_t clen;
  bool f;

  mbstate_t mbs, mbsbak;
  std::memset (&mbs, '\0', sizeof (std::mbstate_t));
  std::memset (&mbsbak, '\0', sizeof (std::mbstate_t));

  size_t nc = 0;
  size_t mb_cur_max = MB_CUR_MAX;
  while (*s
         && (clen = (f = is_basic (static_cast<unsigned char> (*s)))
                        ? 1
                        : std::mbrlen (s, mb_cur_max, &mbs))
                != 0)
    {
      if (MB_INVALIDCH (clen))
        {
          clen = 1; /* assume single byte */
          mbs = mbsbak;
        }

      if (f == 0)
        mbsbak = mbs;

      s += clen;
      nc++;
    }
  return nc;
}

/* Return pointer to first multibyte char in S, or nullptr if none. */
/* XXX - if we know that the locale is UTF-8, we can just check whether or
   not any byte has the eighth bit turned on */
const char *
Shell::mbsmbchar (const char *s)
{
  if (locale_utf8locale)
    return utf8_mbsmbchar (s); /* XXX */

  mbstate_t mbs;
  std::memset (&mbs, '\0', sizeof (std::mbstate_t));

  const char *t;
  size_t clen;

  size_t mb_cur_max = MB_CUR_MAX;
  for (t = s; *t; t++)
    {
      if (is_basic (static_cast<unsigned char> (*t)))
        continue;

      if (locale_utf8locale) /* not used if above code active */
        clen = utf8_mblen (t, mb_cur_max);
      else
        clen = std::mbrlen (t, mb_cur_max, &mbs);

      if (clen == 0)
        return nullptr;

      if (MB_INVALIDCH (clen))
        continue;

      if (clen > 1)
        return t;
    }
  return nullptr;
}

} // namespace bash

#endif // HANDLE_MULTIBYTE
