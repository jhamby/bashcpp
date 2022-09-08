/* psize.c - Find pipe size. */

/* Copyright (C) 1987, 1991 Free Software Foundation, Inc.

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

/*  Write output in 128-byte chunks until we get a sigpipe or write gets an
    EPIPE.  Then report how many bytes we wrote.  We assume that this is the
    pipe size. */

#include "config.hh"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "bashtypes.hh"

#include <csignal>

#include "command.hh"
#include "general.hh"
#include "sig.hh"

static int nw;

static void sigpipe (int)  __attribute__((__noreturn__));

static void
sigpipe (int)
{
  std::fprintf (stderr, "%d\n", nw);
  std::exit (0);
}

int
main (int, char **)
{
  char buf[128];

  for (int i = 0; i < 128; i++)
    buf[i] = ' ';

  ::signal (SIGPIPE, &sigpipe);

  nw = 0;
  for (;;)
    {
      ssize_t n;
      n = ::write (1, buf, 128);
      nw += static_cast<int> (n);
    }
}
