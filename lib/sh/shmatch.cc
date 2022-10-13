// shmatch.cc -- shell interface to posix regular expression matching.

/* Copyright (C) 2003-2022 Free Software Foundation, Inc.

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

#include "config.h"

#include "config-top.hh"
#include "config-bot.hh"

#if defined(HAVE_POSIX_REGEXP)

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cstdio>
#include <regex.h>

#include "externs.hh"
#include "shell.hh"
#include "variables.hh"

namespace bash
{

#if defined(ARRAY_VARS)
extern SHELL_VAR *builtin_find_indexed_array (char *, int);
#endif

int
Shell::sh_regmatch (const char *string, const char *pattern, int flags)
{
  regex_t regex;
  regmatch_t *matches;
  int rflags;
#if defined(ARRAY_VARS)
  SHELL_VAR *rematch;
  ARRAY *amatch;
  char *subexp_str;
  int subexp_len;
#endif
  int result;

#if defined(ARRAY_VARS)
  rematch = nullptr;
#endif

  rflags = REG_EXTENDED;
  if (match_ignore_case)
    rflags |= REG_ICASE;
#if !defined(ARRAY_VARS)
  rflags |= REG_NOSUB;
#endif

  if (regcomp (&regex, pattern, rflags))
    return 2; /* flag for printing a warning here. */

#if defined(ARRAY_VARS)
  matches = new regmatch_t[regex.re_nsub + 1];
#else
  matches = nullptr;
#endif

  /* man regexec: nullptr PMATCH ignored if NMATCH == 0 */
  if (regexec (&regex, string, matches ? regex.re_nsub + 1 : 0, matches, 0))
    result = EXECUTION_FAILURE;
  else
    result = EXECUTION_SUCCESS; /* match */

#if defined(ARRAY_VARS)
  subexp_len = strlen (string) + 10;
  subexp_str = new char[subexp_len + 1];

  /* Store the parenthesized subexpressions in the array BASH_REMATCH.
     Element 0 is the portion that matched the entire regexp.  Element 1
     is the part that matched the first subexpression, and so on. */
#if 1
  unbind_global_variable_noref ("BASH_REMATCH");
  rematch = make_new_array_variable ("BASH_REMATCH");
#else
  /* TAG:bash-5.3 */
  rematch = builtin_find_indexed_array ("BASH_REMATCH", 1);
#endif
  amatch = rematch ? array_cell (rematch) : (ARRAY *)0;

  if (matches && amatch && (flags & SHMAT_SUBEXP)
      && result == EXECUTION_SUCCESS && subexp_str)
    {
      for (size_t subexp_ind = 0; subexp_ind <= regex.re_nsub; subexp_ind++)
        {
          memset (subexp_str, 0, subexp_len);
          strncpy (subexp_str, string + matches[subexp_ind].rm_so,
                        matches[subexp_ind].rm_eo - matches[subexp_ind].rm_so);
          array_insert (amatch, subexp_ind, subexp_str);
        }
    }

  delete[] subexp_str;
  delete[] matches;
#endif /* ARRAY_VARS */

  regfree (&regex);

  return result;
}

} // namespace bash

#endif /* HAVE_POSIX_REGEXP */
