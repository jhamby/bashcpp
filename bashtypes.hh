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

#include "config-top.hh"
#include "config-bot.hh"

#include <sys/types.h>

// Include the gnulib or system header first.
#include <inttypes.h>

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

// Include and undefine the Gnulib macros that break (at least) Haiku OS.
#include <stdlib.h>
#ifdef strtoll
#undef strtoll
#endif
#ifdef strtoull
#undef strtoull
#endif

// Undefine gnulib's static_assert macro from "assert.h",
// which breaks the system C++ headers on MINIX and Solaris.
#ifdef static_assert
#undef static_assert
#endif

// Use C++17 std::string_view or our own lite version.
#if __cplusplus >= 201703L
#include <string_view>
#include <nonstd/string_view.hpp>
namespace bash
{
using std::string_view;
using nonstd::to_string;
using nonstd::to_string_view;
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
