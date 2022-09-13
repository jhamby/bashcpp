/* File-name wildcard pattern matching for GNU.
   Copyright (C) 1985-2020 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne-Again SHell.

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

#ifndef _GLOB_H_
#define _GLOB_H_

#include <exception>

namespace bash
{

enum glob_flags
{
  GX_NOFLAGS = 0,
  GX_MARKDIRS = 0x001,  /* mark directory names with trailing `/' */
  GX_NOCASE = 0x002,    /* ignore case */
  GX_MATCHDOT = 0x004,  /* match `.' literally */
  GX_MATCHDIRS = 0x008, /* match only directory names */
  GX_ALLDIRS = 0x010,   /* match all directory names, no others */
  GX_NULLDIR = 0x100,   /* internal -- no directory preceding pattern */
  GX_ADDCURDIR = 0x200, /* internal -- add passed directory name */
  GX_GLOBSTAR = 0x400,  /* turn on special handling of ** */
  GX_RECURSE = 0x800,   /* internal -- glob_filename called recursively */
  GX_SYMLINK = 0x1000   /* internal -- symlink to a directory */
};

extern bool glob_pattern_p (const char *);
extern char **glob_vector (const char *, const char *, int);
extern char **glob_filename (const char *, int);

extern bool extglob_pattern_p (const char *);

class glob_error : public std::exception
{
};

extern bool noglob_dot_filenames;
extern bool glob_ignore_case;

} // namespace bash

#endif /* _GLOB_H_ */
