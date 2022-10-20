/* hashcmd.h - Common defines for hashing filenames. */

/* Copyright (C) 1993-2020 Free Software Foundation, Inc.

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

#if !defined(_HASHCMD_H_)
#define _HASHCMD_H_

#include "hashlib.hh"

namespace bash
{

constexpr int FILENAME_HASH_BUCKETS = 256; /* must be power of two */

enum phash_flags
{
  HASH_NOFLAGS = 0,
  HASH_RELPATH = 0x01, // this filename is a relative pathname.
  HASH_CHKDOT = 0x02   // check `.' since it was earlier in $PATH.
};

struct PATH_DATA
{
  PATH_DATA (const char *path_, phash_flags flags_)
      : path (path_), flags (flags_)
  {
  }

  const char *path; /* The full pathname of the file. */
  phash_flags flags;
};

} // namespace bash

#endif /* _HASHCMD_H */
