/*
 * netopen.cc -- functions to make tcp/udp connections
 *
 * Chet Ramey
 * chet@ins.CWRU.Edu
 */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

#include "config-bot.hh"
#include "config-top.hh"

#if defined(HAVE_NETWORK)

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "shell.hh"

namespace bash
{

/*
 * Open a TCP or UDP connection to HOST on port SERV.  Uses getaddrinfo(3)
 * which provides support for IPv6.  Returns the connected socket or -1
 * on error.
 */
int
Shell::_netopen6 (const char *host, const char *serv, int typ)
{
  struct addrinfo hints, *res, *res0;
  int gerr;

  memset (&hints, 0, sizeof (hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = (typ == 't') ? SOCK_STREAM : SOCK_DGRAM;

  gerr = getaddrinfo (host, serv, &hints, &res0);
  if (gerr)
    {
      if (gerr == EAI_SERVICE)
        internal_error ("%s: %s", serv, gai_strerror (gerr));
      else
        internal_error ("%s: %s", host, gai_strerror (gerr));
      errno = EINVAL;
      return -1;
    }

  int s = -1;
  for (res = res0; res; res = res->ai_next)
    {
      if ((s = socket (res->ai_family, res->ai_socktype, res->ai_protocol))
          < 0)
        {
          if (res->ai_next)
            continue;
          sys_error ("socket");
          freeaddrinfo (res0);
          return -1;
        }
      if (connect (s, res->ai_addr, res->ai_addrlen) < 0)
        {
          if (res->ai_next)
            {
              close (s);
              continue;
            }
          int e = errno;
          sys_error ("connect");
          close (s);
          freeaddrinfo (res0);
          errno = e;
          return -1;
        }
      freeaddrinfo (res0);
      break;
    }
  return s;
}

/*
 * Open a TCP or UDP connection given a path like `/dev/tcp/host/port' to
 * host `host' on port `port' and return the connected socket.
 */
int
Shell::netopen (const char *path)
{
  char *np, *s, *t;
  int fd;

  np = savestring (path);

  s = np + 9;
  t = strchr (s, '/');
  if (t == nullptr)
    {
      internal_error (_ ("%s: bad network path specification"), path);
      delete[] np;
      return -1;
    }
  *t++ = '\0';
  fd = _netopen (s, t, path[5]);
  delete[] np;

  return fd;
}

} // namespace bash

#endif /* !HAVE_NETWORK */
