/* random.cc -- Functions for managing 16-bit and 32-bit random numbers. */

/* Copyright (C) 2020 Free Software Foundation, Inc.

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

#include "bashtypes.hh"

#if defined(HAVE_SYS_RANDOM_H)
#include <sys/random.h>
#endif

#include <unistd.h>

#include <cstdio>
#include <sys/time.h>

#include "shell.hh"

namespace bash
{

static uint32_t intrand32 (uint32_t);

/* Returns a 32-bit pseudo-random number. */
static inline uint32_t
intrand32 (uint32_t last)
{
  /* Minimal Standard generator from
     "Random number generators: good ones are hard to find",
     Park and Miller, Communications of the ACM, vol. 31, no. 10,
     October 1988, p. 1195. Filtered through FreeBSD.

     x(n+1) = 16807 * x(n) mod (m).

     We split up the calculations to avoid overflow.

     h = last / q; l = x - h * q; t = a * l - h * r
     m = 2147483647, a = 16807, q = 127773, r = 2836

     There are lots of other combinations of constants to use; look at
     https://www.gnu.org/software/gsl/manual/html_node/Other-random-number-generators.html#Other-random-number-generators
   */

  int32_t h, l, t;
  uint32_t ret;

  /* Can't seed with 0. */
  ret = (last == 0) ? 123459876 : last;
  h = ret / 127773;
  l = static_cast<int32_t> (ret) - (127773 * h);
  t = 16807 * l - 2836 * h;
  ret = static_cast<uint32_t> ((t < 0) ? t + 0x7fffffff : t);

  return ret;
}

uint32_t
Shell::genseed ()
{
  struct timeval tv;
  uint32_t iv;

  ::gettimeofday (&tv, nullptr);
  iv = static_cast<uint32_t> (reinterpret_cast<uintptr_t> (
      &intrand32)); // include a function ptr in the seed
  iv = static_cast<uint32_t> (tv.tv_sec ^ tv.tv_usec ^ ::getpid ()
                              ^ ::getppid () ^ current_user.uid ^ iv);
  return iv;
}

#define BASH_RAND_MAX 32767 /* 0x7fff - 16 bits */

/* Returns a pseudo-random number between 0 and 32767. */
int
Shell::brand ()
{
  unsigned int ret;

  rseed = intrand32 (rseed);
  if (shell_compatibility_level > 50)
    ret = (rseed >> 16) ^ (rseed & 65535);
  else
    ret = rseed;
  return ret & BASH_RAND_MAX;
}

#define BASH_RAND32_MAX 0x7fffffff /* 32 bits */

/* Returns a 32-bit pseudo-random number between 0 and 4294967295. */
uint32_t
Shell::brand32 ()
{
  rseed32 = intrand32 (rseed32);
  return rseed32 & BASH_RAND32_MAX;
}

#if !defined(HAVE_GETRANDOM)
/* Imperfect emulation of getrandom(2). */
#ifndef GRND_NONBLOCK
#define GRND_NONBLOCK 1
#define GRND_RANDOM 2
#endif

static ssize_t
getrandom (void *buf, size_t len, unsigned int flags)
{
  int oflags;
  ssize_t r;
  static int urand_unavail = 0;

#if HAVE_GETENTROPY
  r = getentropy (buf, len);
  return (r == 0) ? len : -1;
#endif

  if (urandfd == -1 && urand_unavail == 0)
    {
      oflags = O_RDONLY;
      if (flags & GRND_NONBLOCK)
        oflags |= O_NONBLOCK;
      urandfd = ::open ("/dev/urandom", oflags, 0);
      if (urandfd >= 0)
        SET_CLOSE_ON_EXEC (urandfd);
      else
        {
          urand_unavail = 1;
          return -1;
        }
    }
  if (urandfd >= 0 && (r = ::read (urandfd, buf, len)) == len)
    return r;
  return -1;
}
#endif

uint32_t
Shell::get_urandom32 ()
{
  uint32_t ret;

  if (getrandom (&ret, sizeof (ret), GRND_NONBLOCK) == sizeof (ret))
    return last_rand32 = ret;

#if defined(HAVE_ARC4RANDOM)
  ret = arc4random ();
#else
  if (subshell_environment)
    perturb_rand32 ();
  do
    ret = brand32 ();
  while (ret == last_rand32);
#endif
  return last_rand32 = ret;
}

} // namespace bash
