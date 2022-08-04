/* Copyright (C) 1991-2020 Free Software Foundation, Inc.

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

#ifndef	_STRMATCH_H
#define	_STRMATCH_H	1

#include <config.h>

/* We #undef these before defining them because some losing systems
   (HP-UX A.08.07 for example) define these in <unistd.h>.  */
#undef  FNM_PATHNAME
#undef  FNM_NOESCAPE
#undef  FNM_PERIOD

/* Bits set in the FLAGS argument to `strmatch'.  */

/* standard flags are like fnmatch(3). */
#define	FNM_PATHNAME	(1 << 0) /* No wildcard can ever match `/'.  */
#define	FNM_NOESCAPE	(1 << 1) /* Backslashes don't quote special chars.  */
#define	FNM_PERIOD	(1 << 2) /* Leading `.' is matched only explicitly.  */

/* extended flags not available in most libc fnmatch versions, but we undef
   them to avoid any possible warnings. */
#undef FNM_LEADING_DIR
#undef FNM_CASEFOLD
#undef FNM_EXTMATCH

#define FNM_LEADING_DIR	(1 << 3) /* Ignore `/...' after a match. */
#define FNM_CASEFOLD	(1 << 4) /* Compare without regard to case. */
#define FNM_EXTMATCH	(1 << 5) /* Use ksh-like extended matching. */

#define FNM_FIRSTCHAR	(1 << 6) /* Match only the first character */

/* Value returned by `strmatch' if STRING does not match PATTERN.  */
#undef FNM_NOMATCH

#define	FNM_NOMATCH	1

namespace bash
{

extern int xstrmatch (const char *, const char *, int);
#if defined (HANDLE_MULTIBYTE)
extern int internal_wstrmatch (const wchar_t *, const wchar_t *, int);
#endif

/* Match STRING against the filename pattern PATTERN,
   returning zero if it matches, FNM_NOMATCH if not.  */
static inline int
strmatch (const char *pattern, const char *string, int flags)
{
  if (string == nullptr || pattern == nullptr)
    return FNM_NOMATCH;

  return xstrmatch (pattern, string, flags);
}

#if defined (HANDLE_MULTIBYTE)
static inline int
wcsmatch (const wchar_t *wpattern, const wchar_t *wstring, int flags)
{
  if (wstring == nullptr || wpattern == nullptr)
    return FNM_NOMATCH;

  return internal_wstrmatch (wpattern, wstring, flags);
}
#endif

char *mbsmbchar (const char *);

size_t xdupmbstowcs (wchar_t **destp, char ***indicesp, const char *src);

}  // namespace bash

#endif /* _STRMATCH_H */
