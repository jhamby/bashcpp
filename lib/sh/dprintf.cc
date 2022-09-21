/* dprintf -- printf to a file descriptor */

/* Copyright (C) 2008-2010 Free Software Foundation, Inc.

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

#ifdef HAVE_CONFIG_H
#include "config.hh"
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <cstdarg>
#include <cstdio>

int
dprintf (int fd, const char *format, ...)
{
  FILE *fp;
  int fd2, rc;
  va_list args;

  if ((fd2 = ::dup (fd)) < 0)
    return -1;

  fp = ::fdopen (fd2, "w");
  if (fp == nullptr)
    {
      ::close (fd2);
      return -1;
    }

  va_start (args, format);
  rc = std::vfprintf (fp, format, args);
  std::fflush (fp);
  va_end (args);

  (void)std::fclose (fp); /* check here */

  return rc;
}