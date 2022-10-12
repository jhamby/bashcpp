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

#include "config-top.hh"
#include "config-bot.hh"

#if defined(HAVE_NETWORK)

#include <sys/types.h>

#include <unistd.h>

#include <cstdio>

#include <sys/socket.h>

#include <netinet/in.h>

#if defined(HAVE_NETDB_H)
#include <netdb.h>
#endif

#include <arpa/inet.h>

#include "bashintl.hh"

#include <cerrno>

#include "shell.hh"

namespace bash
{

#if !defined(HAVE_INET_ATON)
extern int inet_aton (const char *, struct in_addr *);
#endif

#ifndef HAVE_GETADDRINFO
static int _getaddr (char *, struct in_addr *);
static int _getserv (char *, int, unsigned short *);
#endif

#ifndef HAVE_GETADDRINFO
/* Stuff the internet address corresponding to HOST into AP, in network
   byte order.  Return 1 on success, 0 on failure. */

static int
_getaddr (char *host, struct in_addr *ap)
{
  struct hostent *h;
  int r;

  r = 0;
  if (host[0] >= '0' && host[0] <= '9')
    {
      /* If the first character is a digit, guess that it's an
         Internet address and return immediately if inet_aton succeeds. */
      r = inet_aton (host, ap);
      if (r)
        return r;
    }
#if !defined(HAVE_GETHOSTBYNAME)
  return 0;
#else
  h = gethostbyname (host);
  if (h && h->h_addr)
    {
      bcopy (h->h_addr, (char *)ap, h->h_length);
      return 1;
    }
#endif
  return 0;
}

/* Return 1 if SERV is a valid port number and stuff the converted value into
   PP in network byte order. */
static int
_getserv (char *serv, int proto, unsigned short *pp)
{
  int64_t l;
  unsigned short s;

  if (legal_number (serv, &l))
    {
      s = (unsigned short)(l & 0xFFFF);
      if (s != l)
        return 0;
      s = htons (s);
      if (pp)
        *pp = s;
      return 1;
    }
  else
#if defined(HAVE_GETSERVBYNAME)
    {
      struct servent *se;

      se = getservbyname (serv, (proto == 't') ? "tcp" : "udp");
      if (se == 0)
        return 0;
      if (pp)
        *pp = se->s_port; /* ports returned in network byte order */
      return 1;
    }
#else  /* !HAVE_GETSERVBYNAME */
    return 0;
#endif /* !HAVE_GETSERVBYNAME */
}

/*
 * Open a TCP or UDP connection to HOST on port SERV.  Uses the
 * traditional BSD mechanisms.  Returns the connected socket or -1 on error.
 */
int
Shell::_netopen4 (const char *host, const char *serv, int typ)
{
  struct in_addr ina;
  struct sockaddr_in sin;
  unsigned short p;
  int s, e;

  if (_getaddr (host, &ina) == 0)
    {
      internal_error (_ ("%s: host unknown"), host);
      errno = EINVAL;
      return -1;
    }

  if (_getserv (serv, typ, &p) == 0)
    {
      internal_error (_ ("%s: invalid service"), serv);
      errno = EINVAL;
      return -1;
    }

  std::memset (&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_port = p;
  sin.sin_addr = ina;

  s = socket (AF_INET, (typ == 't') ? SOCK_STREAM : SOCK_DGRAM, 0);
  if (s < 0)
    {
      sys_error ("socket");
      return -1;
    }

  if (connect (s, (struct sockaddr *)&sin, sizeof (sin)) < 0)
    {
      e = errno;
      sys_error ("connect");
      close (s);
      errno = e;
      return -1;
    }

  return (s);
}
#endif /* ! HAVE_GETADDRINFO */

#ifdef HAVE_GETADDRINFO
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

  std::memset (&hints, 0, sizeof (hints));
  /* XXX -- if problems with IPv6, set to PF_INET for IPv4 only */
#ifdef DEBUG /* PF_INET is the one that works for me */
  hints.ai_family = PF_INET;
#else
  hints.ai_family = PF_UNSPEC;
#endif
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
#endif /* HAVE_GETADDRINFO */

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
  t = std::strchr (s, '/');
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

#else /* !HAVE_NETWORK */

namespace bash
{

int
Shell::netopen (const char *path)
{
  internal_error (_ ("network operations not supported"));
  return -1;
}

} // namespace bash

#endif /* !HAVE_NETWORK */
