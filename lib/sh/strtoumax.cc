/* strtoumax - convert string representation of a number into an uint64_t
 * value. */

/* Copyright 1999-2020 Free Software Foundation, Inc.

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

/* Written by Paul Eggert.  Modified by Chet Ramey for Bash. */

#include "config.hh"

#if !defined(HAVE_STRTOUMAX)

#if HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#if HAVE_STDINT_H
#include <stdint.h>
#endif

#include <cstdlib>

/* Verify a requirement at compile-time (unlike assert, which is runtime).  */
#define verify(name, assertion)                                               \
  struct name                                                                 \
  {                                                                           \
    char a[(assertion) ? 1 : -1];                                             \
  }

#ifndef HAVE_DECL_STRTOUL
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_STRTOUL
    extern unsigned long
    strtoul (const char *, char **, int);
#endif

#ifndef HAVE_DECL_STRTOULL
"this configure-time declaration test was not run"
#endif
#if !HAVE_DECL_STRTOULL
    extern unsigned long long
    strtoull (const char *, char **, int);
#endif

#ifdef strtoumax
#undef strtoumax
#endif

uint64_t
strtoumax (const char *ptr, char **endptr, int base)
{
  verify (size_is_that_of_unsigned_long_or_unsigned_long_long,
          (sizeof (uint64_t) == sizeof (unsigned long)
           || sizeof (uint64_t) == sizeof (unsigned long long)));

  if (sizeof (uint64_t) != sizeof (unsigned long))
    return strtoull (ptr, endptr, base);

  return strtoul (ptr, endptr, base);
}

#ifdef TESTING
#include <cstdio>
int
main ()
{
  char *p, *endptr;
  uint64_t x;
  unsigned long long y;
  unsigned long z;

  std::printf ("sizeof uint64_t: %d\n", sizeof (uint64_t));

  std::printf ("sizeof unsigned long long: %d\n", sizeof (unsigned long long));
  std::printf ("sizeof unsigned long: %d\n", sizeof (unsigned long));

  x = ::strtoumax ("42", &endptr, 10);
  y = ::strtoull ("42", &endptr, 10);
  z = ::strtoul ("42", &endptr, 10);

  std::printf ("%llu %llu %lu\n", x, y, z);

  std::exit (0);
}
#endif

#endif
