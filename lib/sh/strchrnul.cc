/* Searching in a string.
   Copyright (C) 2012 Free Software Foundation, Inc.

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

#include "config.h"

#include <cstdio>

/* Specification.  */
#include <cstring>

#if !defined(HAVE_STRCHRNUL)

/* Find the first occurrence of C in S or the final NUL byte.  */
const char *
strchrnul (const char *s, int c_in) noexcept
{
  char c;
  char *s1;

  for (c = c_in, s1 = const_cast<char *> (s); s1 && *s1 && *s1 != c; s1++)
    ;
  return s1;
}

#endif // HAVE_STRCHRNUL
