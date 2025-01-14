/* netconn.cc -- is a particular file descriptor a network connection?. */

/* Copyright (C) 2002-2005 Free Software Foundation, Inc.

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

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include "posixstat.hh"

#include <cerrno>

#include <unistd.h>

#include <sys/socket.h>

namespace bash
{

/* Is FD a socket or network connection? */
bool
isnetconn (int fd)
{
#if defined(HAVE_SYS_SOCKET_H) && defined(HAVE_GETPEERNAME)
  int rv;
  socklen_t l;
  struct sockaddr sa;

  l = sizeof (sa);
  rv = getpeername (fd, &sa, &l);
  /* Posix.2 says getpeername can return these errors. */
  return (rv < 0
          && (errno == ENOTSOCK || errno == ENOTCONN || errno == EINVAL
              || errno == EBADF))
             ? false
             : true;
#else /* !HAVE_GETPEERNAME */
#if defined(SVR4) || defined(SVR4_2)
  /* Sockets on SVR4 and SVR4.2 are character special (streams) devices. */
  struct stat sb;

  if (isatty (fd))
    return false;
  if (fstat (fd, &sb) < 0)
    return false;
#if defined(S_ISFIFO)
  if (S_ISFIFO (sb.st_mode))
    return false;
#endif /* S_ISFIFO */
  return S_ISCHR (sb.st_mode);
#else  /* !SVR4 && !SVR4_2 */
#if defined(S_ISSOCK)
  struct stat sb;

  if (fstat (fd, &sb) < 0)
    return false;
  return S_ISSOCK (sb.st_mode);
#else  /* !S_ISSOCK */
  return false;
#endif /* !S_ISSOCK */
#endif /* !SVR4 && !SVR4_2 */
#endif /* !HAVE_GETPEERNAME */
}

} // namespace bash
