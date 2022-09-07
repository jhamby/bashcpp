/* zcatfd - copy contents of file descriptor to another */

/* Copyright (C) 2002-2020 Free Software Foundation, Inc.

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

#include "config.hh"

#include <sys/types.h>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <cerrno>

#include "shell.hh"

namespace bash
{

/* Dump contents of file descriptor FD to OFD.  FN is the filename for
   error messages (not used right now). */
ssize_t
Shell::zcatfd (int fd, int ofd, const char *)
{
  ssize_t nr;
  int rval;
  char lbuf[ZBUFSIZ];

  rval = 0;
  while (1)
    {
      nr = zread (fd, lbuf, sizeof (lbuf));
      if (nr == 0)
	break;
      else if (nr < 0)
	{
	  rval = -1;
	  break;
	}
      else if (zwrite (ofd, lbuf, static_cast<size_t> (nr)) < 0)
	{
	  rval = -1;
	  break;
	}
    }

  return rval;
}

}  // namespace bash
