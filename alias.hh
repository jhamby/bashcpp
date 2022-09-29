/* alias.h -- structure definitions. */

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

#if !defined(_ALIAS_H_)
#define _ALIAS_H_

#include "hashlib.hh"

namespace bash
{

/* Values for `flags' member of struct alias. */
enum alias_flags
{
  AL_NOFLAGS = 0,
  AL_EXPANDNEXT = 0x1,
  AL_BEINGEXPANDED = 0x2
};

static inline alias_flags &
operator|= (alias_flags &a, const alias_flags &b)
{
  a = static_cast<alias_flags> (static_cast<uint32_t> (a)
                                | static_cast<uint32_t> (b));
  return a;
}

static inline alias_flags &
operator&= (alias_flags &a, const alias_flags &b)
{
  a = static_cast<alias_flags> (static_cast<uint32_t> (a)
                                & static_cast<uint32_t> (b));
  return a;
}

static inline alias_flags
operator~(const alias_flags &a)
{
  return static_cast<alias_flags> (~static_cast<uint32_t> (a));
}

struct alias_t
{
  std::string name;
  std::string value;
  alias_flags flags;
};

} // namespace bash

#endif /* _ALIAS_H_ */
