/* zmapfd - read contents of file descriptor into a newly-allocated buffer */

/* Copyright (C) 2006-2020 Free Software Foundation, Inc.

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

#include <cerrno>
#include <string>

#include "command.h"
#include "general.h"
#include "shell.h"

namespace bash
{

#ifndef ZBUFSIZ
#  define ZBUFSIZ 4096
#endif

/* Dump contents of file descriptor FD to *OSTR.  FN is the filename for
   error messages (not used right now). */
int
Shell::zmapfd (int fd, char **ostr, const char *fn)
{
  ssize_t nr;
  char lbuf[ZBUFSIZ];

  std::string result;
  int rind = 0;

  while (1)
    {
      nr = zread (fd, lbuf, sizeof (lbuf));
      if (nr == 0)
	{
	  break;
	}
      else if (nr < 0)
	{
	  if (ostr)
	    *ostr = nullptr;
	  return -1;
	}

      result.append (lbuf, nr);
      rind += nr;
    }

  if (ostr)
    *ostr = savestring (result);

  return rind;
}

}  // namespace bash
