/* setlinebuf.cc - line-buffer a stdio stream. */

/* Copyright (C) 1997,2022 Free Software Foundation, Inc.

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

#include <cstdio>

#include "externs.hh"

namespace bash
{

#define LBUF_BUFSIZE BUFSIZ

/* Cause STREAM to buffer lines as opposed to characters or blocks. */
int
sh_setlinebuf (FILE *stream)
{
#if !defined(HAVE_SETLINEBUF) && !defined(HAVE_SETVBUF)
  return 0;
#endif

#if defined(HAVE_SETVBUF)
  char *local_linebuf = nullptr;

  return (setvbuf (stream, local_linebuf, _IOLBF, LBUF_BUFSIZE));
#else /* !HAVE_SETVBUF */

  setlinebuf (stream);
  return 0;

#endif /* !HAVE_SETVBUF */
}

} // namespace bash
