/* builtins.h -- What a builtin looks like, and where to find them. */

/* Copyright (C) 1987-2009 Free Software Foundation, Inc.

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

#ifndef BUILTINS_H
#define BUILTINS_H

#include "config.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

namespace bash
{

/* Flags describing various things about a builtin. */
enum builtin_flags
{
  BUILTIN_ENABLED = 0x01,    /* This builtin is enabled. */
  BUILTIN_DELETED = 0x02,    /* This has been deleted with enable -d. */
  STATIC_BUILTIN = 0x04,     /* This builtin is not dynamically loaded. */
  SPECIAL_BUILTIN = 0x08,    /* This is a Posix `special' builtin. */
  ASSIGNMENT_BUILTIN = 0x10, /* This builtin takes assignment statements. */
  POSIX_BUILTIN
  = 0x20, /* This builtin is special in the Posix command search order. */
  LOCALVAR_BUILTIN = 0x40 /* This builtin creates local variables */
};

const int BASE_INDENT = 4;

} // namespace bash

#endif /* BUILTINS_H */
