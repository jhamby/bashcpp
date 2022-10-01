/* rldefs.h -- an attempt to isolate some of the system-specific defines
   for readline.  This should be included after any files that define
   system-specific constants like _POSIX_VERSION or USG. */

/* Copyright (C) 1987-2011 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library (Readline), a library
   for reading lines of text with interactive input and history editing.

   Readline is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Readline is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(_RLDEFS_H_)
#define _RLDEFS_H_

#include "config.hh"

// First, include common C++ wrappers for standard C headers.
#include <cctype>
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwctype>

// Next, include some common C++ library headers.
#include <algorithm>
#include <exception>
#include <string>
#include <vector>

// Fake C++11 keywords for older C++ compilers.
#if !defined(nullptr) && __cplusplus < 201103L
#define noexcept throw ()
#define override
#define nullptr NULL
#define constexpr const
#endif

#if defined(HAVE_INTTYPES_H)
#include <inttypes.h>
#endif

#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h> /* for _POSIX_VERSION */
#endif

#if defined(_POSIX_VERSION) && !defined(TERMIOS_MISSING)
#define TERMIOS_TTY_DRIVER
#else
#if defined(HAVE_TERMIO_H)
#define TERMIO_TTY_DRIVER
#else
#if !defined(__MINGW32__)
#define NEW_TTY_DRIVER
#else
#define NO_TTY_DRIVER
#endif
#endif
#endif

// Note: on Linux, we must include <termios.h> before including
// <sys/ioctl.h> or else NCC may be defined as too small a value.

/* Posix systems use termios and the Posix signal functions. */
#if defined(TERMIOS_TTY_DRIVER)
#include <termios.h>
#endif /* TERMIOS_TTY_DRIVER */

/* System V machines use termio. */
#if defined(TERMIO_TTY_DRIVER)
#include <termio.h>
#if !defined(TCOON)
#define TCOON 1
#endif
#endif /* TERMIO_TTY_DRIVER */

/* Other (BSD) machines use sgtty. */
#if defined(NEW_TTY_DRIVER)
#include <sgtty.h>
#endif

/* Posix macro to check file in statbuf for directory-ness.
   This requires that <sys/stat.h> be included before this test. */
#if defined(S_IFDIR) && !defined(S_ISDIR)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

#ifdef malloc
#undef malloc
#endif
#define malloc(x) Error - use C++ new instead !

#ifdef free
#undef free
#endif
#define free(x) Error - Use C++ delete instead !

#ifdef calloc
#undef calloc
#endif
#define calloc(x, y) Error - Use C++ vector instead !

#ifdef realloc
#undef realloc
#endif
#define realloc(x, y) Error - Use C++ vector instead !

#ifdef strdup
#undef strdup
#endif
#define strdup(x) Error - Use savestring () instead !

namespace readline
{

// Primary readline/libhistory exception class.
class rl_exception : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

// Special exception thrown to handle "word not found" error.
class word_not_found : public rl_exception
{
public:
  virtual const char *what () const noexcept override;
};

enum editing_mode
{
  no_mode = -1,
  vi_mode = 0,
  emacs_mode = 1
};

#if !defined(RL_IM_INSERT)
#define RL_IM_INSERT true
#define RL_IM_OVERWRITE false
#
#define RL_IM_DEFAULT RL_IM_INSERT
#endif

/* Possible values for _rl_bell_preference. */

enum rl_bell_pref
{
  NO_BELL = 0,
  AUDIBLE_BELL = 1,
  VISIBLE_BELL = 2
};

/* Definitions used when searching the line for characters. */
/* NOTE: it is necessary that opposite directions are inverses */
const int FTO = 1;    /* forward to */
const int BTO = -1;   /* backward to */
const int FFIND = 2;  /* forward find */
const int BFIND = -2; /* backward find */

/* Possible values for the found_quote flags word used by the completion
   functions.  It says what kind of (shell-like) quoting we found anywhere
   in the line. */
enum rl_qf_flags
{
  RL_QF_NONE = 0,
  RL_QF_SINGLE_QUOTE = 0x01,
  RL_QF_DOUBLE_QUOTE = 0x02,
  RL_QF_BACKSLASH = 0x04,
  RL_QF_OTHER_QUOTE = 0x08
};

static inline rl_qf_flags &
operator|= (rl_qf_flags &a, const rl_qf_flags &b)
{
  a = static_cast<rl_qf_flags> (static_cast<uint32_t> (a)
                                | static_cast<uint32_t> (b));
  return a;
}

static inline rl_qf_flags
operator| (const rl_qf_flags &a, const rl_qf_flags &b)
{
  return static_cast<rl_qf_flags> (static_cast<uint32_t> (a)
                                   | static_cast<uint32_t> (b));
}

/* Default readline line buffer length. */
constexpr int DEFAULT_BUFFER_SIZE = 256;

#ifndef BRACKETED_PASTE_DEFAULT
#define BRACKETED_PASTE_DEFAULT 1 /* XXX - for now */
#endif

} // namespace readline

/* CONFIGURATION SECTION */
#include "rlconf.hh"

// Include multibyte support.
#include "rlmbutil.hh"

// Include some additional string macros and inline funcs.
#include "chardefs.hh"

#endif /* !_RLDEFS_H_ */
