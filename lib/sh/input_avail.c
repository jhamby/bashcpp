/* input_avail.c -- check whether or not data is available for reading on a
		    specified file descriptor. */

/* Copyright (C) 2008,2009-2019 Free Software Foundation, Inc.

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

#if defined (__TANDEM)
#  include <floss.h>
#endif

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#include <sys/types.h>
#include <fcntl.h>
#if defined (HAVE_SYS_FILE_H)
#  include <sys/file.h>
#endif /* HAVE_SYS_FILE_H */

#if defined (HAVE_PSELECT) || defined (HAVE_SELECT)
#  include <csignal>
#endif

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "posixselect.h"

#if defined (FIONREAD_IN_SYS_IOCTL)
#  include <sys/ioctl.h>
#endif

#include <cstdio>
#include <cerrno>

#if !defined (O_NDELAY) && defined (O_NONBLOCK)
#  define O_NDELAY O_NONBLOCK	/* Posix style */
#endif

#include "externs.h"

namespace bash
{

/* Return >= 1 if select/FIONREAD indicates data available for reading on
   file descriptor FD; 0 if no data available.  Return -1 on error. */
int
input_avail (int fd)
{
  int result;
#if defined(HAVE_SELECT)
  fd_set readfds, exceptfds;
  struct timeval timeout;
#endif

  if (fd < 0)
    return -1;

#if defined (HAVE_SELECT)
  FD_ZERO (&readfds);
  FD_ZERO (&exceptfds);
  FD_SET (fd, &readfds);
  FD_SET (fd, &exceptfds);
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  result = ::select (fd + 1, &readfds, nullptr, &exceptfds, &timeout);
  return (result <= 0) ? 0 : 1;
#elif defined (FIONREAD)
  errno = 0;
  int chars_avail = 0;
  result = ::ioctl (fd, FIONREAD, &chars_avail);
  if (result == -1 && errno == EIO)
    return -1;
  return chars_avail;
#else
  return 0;
#endif
}

}  // namespace bash
