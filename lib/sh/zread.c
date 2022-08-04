/* zread - read data from file descriptor into buffer with retries */

/* Copyright (C) 1999-2020 Free Software Foundation, Inc.

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

#include <config.h>

#include <sys/types.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <csignal>
#include <cerrno>

#include "shell.h"

namespace bash
{

/* Read one character from FD and return it in CP.  Return values are as
   in read(2).  This does some local buffering to avoid many one-character
   calls to read(2), like those the `read' builtin performs. */

ssize_t
Shell::zreadc (int fd, char *cp)
{
  ssize_t nr;

  if (zread_lind == zread_lused || zread_lused == 0)
    {
      nr = zread (fd, zread_lbuf, sizeof (zread_lbuf));
      zread_lind = 0;
      if (nr <= 0)
	{
	  zread_lused = 0;
	  return nr;
	}
      zread_lused = nr;
    }
  if (cp)
    *cp = zread_lbuf[zread_lind++];
  return 1;
}

/* Don't mix calls to zreadc and zreadcintr in the same function, since they
   use the same local buffer. */
ssize_t
Shell::zreadcintr (int fd, char *cp)
{
  ssize_t nr;

  if (zread_lind == zread_lused || zread_lused == 0)
    {
      nr = zreadintr (fd, zread_lbuf, sizeof (zread_lbuf));
      zread_lind = 0;
      if (nr <= 0)
	{
	  zread_lused = 0;
	  return nr;
	}
      zread_lused = nr;
    }
  if (cp)
    *cp = zread_lbuf[zread_lind++];
  return 1;
}

/* Like zreadc, but read a specified number of characters at a time.  Used
   for `read -N'. */
ssize_t
Shell::zreadn (int fd, char *cp, size_t len)
{
  ssize_t nr;

  if (zread_lind == zread_lused || zread_lused == 0)
    {
      if (len > sizeof (zread_lbuf))
	len = sizeof (zread_lbuf);
      nr = zread (fd, zread_lbuf, len);
      zread_lind = 0;
      if (nr <= 0)
	{
	  zread_lused = 0;
	  return nr;
	}
      zread_lused = nr;
    }
  if (cp)
    *cp = zread_lbuf[zread_lind++];
  return 1;
}

}  // namespace bash
