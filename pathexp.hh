/* pathexp.h -- The shell interface to the globbing library. */

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

#if !defined(_PATHEXP_H_)
#define _PATHEXP_H_

namespace bash
{

// #define GLOB_FAILED(glist) (glist) == (char **)&glob_error_return

/* Flag values for quote_string_for_globbing */
enum qglob_flags
{
  QGLOB_CVTNULL = 0x01,  /* convert QUOTED_NULL strings to '\0' */
  QGLOB_FILENAME = 0x02, /* do correct quoting for matching filenames */
  QGLOB_REGEXP = 0x04,   /* quote an ERE for regcomp/regexec */
  QGLOB_CTLESC = 0x08,   /* turn CTLESC CTLESC into CTLESC for BREs */
  QGLOB_DEQUOTE = 0x10   /* like dequote_string but quote glob chars */
};

static inline qglob_flags &
operator|= (qglob_flags &a, const qglob_flags &b)
{
  a = static_cast<qglob_flags> (static_cast<uint32_t> (a)
                                | static_cast<uint32_t> (b));
  return a;
}

#if defined(EXTENDED_GLOB)
/* Flags to OR with other flag args to fnmatch() to enabled the extended
   pattern matching. */
#define FNMATCH_EXTFLAG (extended_glob ? FNM_EXTMATCH : 0)
#else
#define FNMATCH_EXTFLAG 0
#endif /* !EXTENDED_GLOB */

#define FNMATCH_IGNCASE (match_ignore_case ? FNM_CASEFOLD : 0)
#define FNMATCH_NOCASEGLOB (glob_ignore_case ? FNM_CASEFOLD : 0)

} // namespace bash

#endif
