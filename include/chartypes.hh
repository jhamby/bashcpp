/* chartypes.h -- extend ctype.h */

/* Copyright (C) 2001 Free Software Foundation, Inc.

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

#ifndef _SH_CHARTYPES_H
#define _SH_CHARTYPES_H

#include <cctype>

namespace bash
{

// OpenVMS has C++03 except some C99 functions are in namespace std only.
#if __cplusplus >= 201103L || defined(__VMS)
using std::isblank;
#else
using ::isblank;
#endif

static inline bool
isword (char c)
{
  return std::isalpha (c) || std::isdigit (c) || (c == '_');
}

static inline int
hexvalue (char c)
{
  return (c >= 'a' && c <= 'f')   ? (c - 'a' + 10)
         : (c >= 'A' && c <= 'F') ? (c - 'A' + 10)
                                  : (c - '0');
}

static inline bool
isoctal (char c)
{
  return c >= '0' && c <= '7';
}

static inline int
octvalue (char c)
{
  return c - '0';
}

static inline int
todigit (char c)
{
  return c - '0';
}

static inline char
tochar (int i)
{
  return static_cast<char> (i) + '0';
}

/* letter to control char -- ASCII.  The TOUPPER is in there so \ce and
   \cE will map to the same character in $'...' expansions. */
static inline char
toctrl (char c)
{
  return (c == '?') ? 0x7f : (std::toupper (c) & 0x1f);
}

/* control char to letter -- ASCII */
static inline int
unctrl (char c)
{
  return std::toupper (c) ^ 0x40;
}

}

#endif /* _SH_CHARTYPES_H */
