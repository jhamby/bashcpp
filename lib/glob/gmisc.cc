/* gmisc.cc -- miscellaneous pattern matching utility functions for Bash.

   Copyright (C) 2010-2020 Free Software Foundation, Inc.

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

#include "config.h"

#include "bashtypes.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "chartypes.hh"
#include "shmbutil.hh"

#ifndef FNM_CASEFOLD
#include "strmatch.hh"
#endif
#include "glob.hh"

namespace bash
{

/* Make sure these names continue to agree with what's in smatch.c */
const unsigned char *glob_patscan (const unsigned char *,
                                   const unsigned char *, int);

/* Compile `gm_loop.hh' for single-byte characters. */
#define CHAR char
#define INT int
#define L(CS) CS
#define EXTGLOB_PATTERN_P extglob_pattern_p
#define MATCH_PATTERN_CHAR match_pattern_char
#define MATCHLEN umatchlen
#define FOLD(c)                                                               \
  ((flags & FNM_CASEFOLD) ? std::tolower ((unsigned char)c)                   \
                          : ((unsigned char)c))
#ifndef LPAREN
#define LPAREN '('
#define RPAREN ')'
#endif
#include "gm_loop.hh"

/* Compile `gm_loop.hh' again for multibyte characters. */
#if HANDLE_MULTIBYTE

#define CHAR wchar_t
#define INT wint_t
#define L(CS) L##CS
#define EXTGLOB_PATTERN_P wextglob_pattern_p
#define MATCH_PATTERN_CHAR match_pattern_wchar
#define MATCHLEN wmatchlen

#define FOLD(c)                                                               \
  ((flags & FNM_CASEFOLD) && std::iswupper (c) ? std::towlower (c) : (c))
#define LPAREN L'('
#define RPAREN L')'
#include "gm_loop.hh"

#endif /* HANDLE_MULTIBYTE */

#if defined(EXTENDED_GLOB)
/* Skip characters in PAT and return the final occurrence of DIRSEP.  This
   is only called when extended_glob is set, so we have to skip over extglob
   patterns x(...) */
const char *
glob_dirscan (const char *pat, char dirsep)
{
  const char *p, *d, *pe, *se;

  d = pe = se = 0;
  for (p = pat; p && *p; p++)
    {
      if (extglob_pattern_p (p))
        {
          if (se == 0)
            se = p + std::strlen (p) - 1;
          pe = (char *)glob_patscan ((unsigned char *)(p + 2),
                                     (unsigned char *)se, 0);
          if (pe == 0)
            continue;
          else if (*pe == 0)
            break;
          p = pe - 1; /* will do increment above */
          continue;
        }
      if (*p == dirsep)
        d = p;
    }
  return d;
}
#endif /* EXTENDED_GLOB */

} // namespace bash
