/* bashtypes.h -- Bash system types. */

/* Copyright (C) 1993-2009 Free Software Foundation, Inc.

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

#if !defined(_BASHTYPES_H_)
#define _BASHTYPES_H_

#include <sys/types.h>

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

// Fake C++11 keywords for older C++ compilers.
#if !defined(nullptr) && __cplusplus < 201103L
#define noexcept throw ()
#define override
#define nullptr NULL
#define constexpr const
#define CONSTEXPR
#else
#define CONSTEXPR constexpr
#endif

// Remove __attribute__ if we're not using GCC or Clang.
#ifndef __attribute__
#if !defined (__clang__) && !defined (__GNUC__)
#define __attribute__(x)
#endif
#endif

#endif /* _BASHTYPES_H_ */