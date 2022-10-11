/* fmtulong.cc -- Convert unsigned long int to string. */

/* Copyright (C) 1998-2011 Free Software Foundation, Inc.

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

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <climits>
#include <cstddef>
#include <cstring>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include <cerrno>

#include "bashintl.hh"
#include "chartypes.hh"
#include "externs.hh"

#include "typemax.hh"

namespace bash
{

static const char *x_digs = "0123456789abcdef";
static const char *X_digs = "0123456789ABCDEF";

/* XXX -- assumes uppercase letters, lowercase letters, and digits are
   contiguous */
#define FMTCHAR(x)                                                            \
  (static_cast<char> (((x) < 10)                                              \
      ? (x) + '0'                                                             \
      : (((x) < 36)                                                           \
             ? (x)-10 + 'a'                                                   \
             : (((x) < 62) ? (x)-36 + 'A' : (((x) == 62) ? '@' : '_')))))

#ifndef LONG
#define LONG long
#define UNSIGNED_LONG unsigned long
#endif

/* `unsigned long' (or unsigned long long) to string conversion for a given
   base. */
std::string
fmtulong (UNSIGNED_LONG ui, int base, fmt_flags flags)
{
  std::string buf;
  int sign;
  LONG si;

  if (base == 0)
    base = 10;

  if (base < 2 || base > 64)
    {
#if 1
      buf = _ ("invalid base");
      errno = EINVAL;
      return savestring (buf);
#else
      base = 10;
#endif
    }

  sign = 0;
  if ((flags & FL_UNSIGNED) == 0 && static_cast<LONG> (ui) < 0)
    {
      ui = -ui;
      sign = '-';
    }

  /* handle common cases explicitly */
  switch (base)
    {
    case 10:
      if (ui < 10)
        {
          buf.insert (0, 1, static_cast<char> (ui));
          break;
        }
      /* Favor signed arithmetic over unsigned arithmetic; it is faster on
         many machines. */
      if (static_cast<LONG> (ui) < 0)
        {
          buf.insert (0, 1, static_cast<char> (ui % 10));
          si = ui / 10;
        }
      else
        si = static_cast<LONG> (ui);
      do
        buf.insert (0, 1, static_cast<char> (si % 10));
      while (si /= 10);
      break;

    case 8:
      do
        buf.insert (0, 1, static_cast<char> (ui & 7));
      while (ui >>= 3);
      break;

    case 16:
      do
        buf.insert (0, 1,
                    (flags & FL_HEXUPPER) ? X_digs[ui & 15] : x_digs[ui & 15]);
      while (ui >>= 4);
      break;

    case 2:
      do
        buf.insert (0, 1, static_cast<char> (ui & 1));
      while (ui >>= 1);
      break;

    default:
      do
        buf.insert (0, 1, FMTCHAR (ui % static_cast<unsigned int> (base)));
      while (ui /= static_cast<unsigned int> (base));
      break;
    }

  if ((flags & FL_PREFIX) && (base == 8 || base == 16))
    {
      if (base == 16)
        {
          buf.insert (0, 1, (flags & FL_HEXUPPER) ? 'X' : 'x');
          buf.insert (0, 1, '0');
        }
      else if (buf[0] != '0')
        buf.insert (0, 1, '0');
    }
  else if ((flags & FL_ADDBASE) && base != 10)
    {
      buf.insert (0, 1, '#');
      buf.insert (0, 1, static_cast<char> (base % 10));
      if (base > 10)
        buf.insert (0, 1, static_cast<char> (base / 10));
    }

  if (sign)
    buf.insert (0, 1, '-');

  return savestring (buf);
}

} // namespace bash
