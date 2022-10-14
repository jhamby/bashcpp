/* oslib.cc - functions present only in some unix versions. */

/* Copyright (C) 1995,2010 Free Software Foundation, Inc.

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

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#include <unistd.h>

#include <climits>

#include "posixstat.hh"

#if !defined(HAVE_KILLPG)
#include <csignal>
#endif

#include <cerrno>
#include <cstdio>

#include "chartypes.hh"
#include "shell.hh"

#if !defined(HAVE_KILLPG)
int
killpg (pid_t pgrp, int sig)
{
  return kill (-pgrp, sig);
}
#endif /* !HAVE_KILLPG */

namespace bash
{

#define DEFAULT_MAXGROUPS 64

int
getmaxgroups ()
{
  static int maxgroups = -1;

  if (maxgroups > 0)
    return maxgroups;

#if defined(HAVE_SYSCONF) && defined(_SC_NGROUPS_MAX)
  maxgroups = static_cast<int> (sysconf (_SC_NGROUPS_MAX));
#else
#if defined(NGROUPS_MAX)
  maxgroups = NGROUPS_MAX;
#else /* !NGROUPS_MAX */
#if defined(NGROUPS)
  maxgroups = NGROUPS;
#else  /* !NGROUPS */
  maxgroups = DEFAULT_MAXGROUPS;
#endif /* !NGROUPS */
#endif /* !NGROUPS_MAX */
#endif /* !HAVE_SYSCONF || !SC_NGROUPS_MAX */

  if (maxgroups <= 0)
    maxgroups = DEFAULT_MAXGROUPS;

  return maxgroups;
}

long
getmaxchild ()
{
  static long maxchild = -1L;

  if (maxchild > 0)
    return maxchild;

#if defined(HAVE_SYSCONF) && defined(_SC_CHILD_MAX)
  maxchild = sysconf (_SC_CHILD_MAX);
#else
#if defined(CHILD_MAX)
  maxchild = CHILD_MAX;
#else
#if defined(MAXUPRC)
  maxchild = MAXUPRC;
#endif /* MAXUPRC */
#endif /* CHILD_MAX */
#endif /* !HAVE_SYSCONF || !_SC_CHILD_MAX */

  return maxchild;
}

} // namespace bash
