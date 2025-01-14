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

#include "config-bot.hh"
#include "config-top.hh"

#include <sys/types.h>

// First, include common C++ wrappers for standard C headers.
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>

// Now, include either Gnulib overrides or system headers.
#include <stdint.h>
#include <string.h>
#include <unistd.h> /* for _POSIX_VERSION */

// Include Gnulib versions of C type functions.
#include "c-ctype.h"

// Next, include some common C++ library headers.
#include <algorithm>
#include <exception>
#include <string>
#include <vector>

// Use C++17 std::string_view or our own lite version.
#if __cplusplus >= 201703L
#include <nonstd/string_view.hpp>
#include <string_view>
namespace bash
{
using nonstd::to_string;
using nonstd::to_string_view;
using std::string_view;
}
#else
#include <nonstd/string_view.hpp>
namespace bash
{
using nonstd::string_view;
using nonstd::to_string;
using nonstd::to_string_view;
}
#endif

// Prefer unlocked I/O where available (include after other files).
#include "unlocked-io.h"

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
#if !defined(__clang__) && !defined(__GNUC__)
#define __attribute__(x)
#endif
#endif

#endif /* _BASHTYPES_H_ */
