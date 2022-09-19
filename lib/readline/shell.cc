/* shell.c -- readline utility functions that are normally provided by
              bash when readline is linked as part of the shell. */

/* Copyright (C) 1997-2009,2017 Free Software Foundation, Inc.

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

#include "config.hh"

/* System-specific feature definitions and include files. */
#include "rldefs.hh"

#include <sys/types.h>

#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#include "rlshell.hh"

#if defined(HAVE_GETPWUID) && !defined(HAVE_GETPW_DECLS)
extern "C" struct passwd *getpwuid (uid_t);
#endif /* HAVE_GETPWUID && !HAVE_GETPW_DECLS */

namespace readline
{

// Define the virtual destructor.
ReadlineShell::~ReadlineShell () {}

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

/* Nonzero if the integer type T is signed.  */
#define TYPE_SIGNED(t) (!(static_cast<t> (0) < static_cast<t> (-1)))

/* Bound on length of the string representing an integer value of type T.
   Subtract one for the sign bit if T is signed;
   302 / 1000 is log10 (2) rounded up;
   add one for integer division truncation;
   add one more for a minus sign if t is signed.  */
#define INT_STRLEN_BOUND(t)                                                   \
  ((sizeof (t) * CHAR_BIT - TYPE_SIGNED (t)) * 302 / 1000 + 1                 \
   + TYPE_SIGNED (t))

/* All of these functions are resolved from bash if we are linking readline
   as part of bash. */

/* Does shell-like quoting using single quotes. */
std::string
ReadlineShell::sh_single_quote (const std::string &string)
{
  std::string result;
  result.reserve (2 + string.size ());
  result.push_back ('\'');

  std::string::const_iterator it;
  for (it = string.begin (); it != string.end (); ++it)
    {
      result.push_back (*it);

      if (*it == '\'')
        {
          result.push_back ('\\'); /* insert escaped single quote */
          result.push_back ('\'');
          result.push_back ('\''); /* start new quoted string */
        }
    }

  result.push_back ('\'');
  return result;
}

/* Set the environment variables LINES and COLUMNS to lines and cols,
   respectively. */
#if defined(HAVE_SETENV)
static char setenv_buf[INT_STRLEN_BOUND (int) + 1];
#else /* !HAVE_SETENV */
#if defined(HAVE_PUTENV)
static char
    putenv_buf1[INT_STRLEN_BOUND (int) + 6 + 1]; /* sizeof("LINES=") == 6 */
static char
    putenv_buf2[INT_STRLEN_BOUND (int) + 8 + 1]; /* sizeof("COLUMNS=") == 8 */
#endif /* HAVE_PUTENV */
#endif /* !HAVE_SETENV */

void
ReadlineShell::sh_set_lines_and_columns (unsigned int lines, unsigned int cols)
{
#if defined(HAVE_SETENV)
  std::snprintf (setenv_buf, sizeof (setenv_buf), "%u", lines);
  ::setenv ("LINES", setenv_buf, 1);

  std::snprintf (setenv_buf, sizeof (setenv_buf), "%u", cols);
  ::setenv ("COLUMNS", setenv_buf, 1);
#else /* !HAVE_SETENV */
#if defined(HAVE_PUTENV)
  std::snprintf (putenv_buf1, sizeof (putenv_buf1), "LINES=%u", lines);
  ::putenv (putenv_buf1);

  std::snprintf (putenv_buf2, sizeof (putenv_buf2), "COLUMNS=%u", cols);
  ::putenv (putenv_buf2);
#endif /* HAVE_PUTENV */
#endif /* !HAVE_SETENV */
}

const char *
ReadlineShell::sh_get_env_value (const char *varname)
{
  return std::getenv (varname);
}

std::string
ReadlineShell::sh_get_home_dir ()
{
  static std::string home_dir; // Note: static cache variable!
  struct passwd *entry;

  if (!home_dir.empty ())
    return home_dir;

#if defined(HAVE_GETPWUID)
#if defined(__TANDEM)
  entry = ::getpwnam (getlogin ());
#else
  entry = ::getpwuid (getuid ());
#endif
  if (entry)
    home_dir = entry->pw_dir;
#endif

#if defined(HAVE_GETPWENT)
  ::endpwent (); /* some systems need this */
#endif

  return home_dir;
}

#if !defined(O_NDELAY)
#if defined(FNDELAY)
#define O_NDELAY FNDELAY
#endif
#endif

int
ReadlineShell::sh_unset_nodelay_mode (int fd)
{
  int flags, bflags;

  if ((flags = ::fcntl (fd, F_GETFL, 0)) < 0)
    return -1;

  bflags = 0;

#ifdef O_NONBLOCK
  bflags |= O_NONBLOCK;
#endif

#ifdef O_NDELAY
  bflags |= O_NDELAY;
#endif

  if (flags & bflags)
    {
      flags &= ~bflags;
      return ::fcntl (fd, F_SETFL, flags);
    }

  return 0;
}

} // namespace readline
