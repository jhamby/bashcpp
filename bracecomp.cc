/* bracecomp.cc -- Complete a filename with the possible completions enclosed
   in csh-style braces such that the list of completions is available to the
   shell. */

/* Original version by tromey@cns.caltech.edu,  Fri Feb  7 1992. */

/* Copyright (C) 1993-2020 Free Software Foundation, Inc.

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

#if defined(BRACE_EXPANSION) && defined(READLINE)

#include "readline.hh"
#include "shell.hh"

namespace bash
{

static int _strcompare (char **, char **);

/* Find greatest common prefix of two strings. */
static int
string_gcd (const char *s1, const char *s2)
{
  if (s1 == NULL || s2 == NULL)
    return 0;

  int i;
  for (i = 0; *s1 && *s2; ++s1, ++s2, ++i)
    {
      if (*s1 != *s2)
        break;
    }

  return i;
}

static char *
really_munge_braces (char **array, int real_start, int real_end, int gcd_zero)
{
  int start, end, gcd;
  char *subterm, *x;
  int tlen;

  bool flag = false;

  if (real_start == real_end)
    {
      x = array[real_start]
              ? sh_backslash_quote (array[real_start] + gcd_zero, 0, 0)
              : sh_backslash_quote (array[0], 0, 0);
      return x;
    }

  std::string result;

  for (start = real_start; start < real_end; start = end + 1)
    {
      gcd = strlen (array[start]);
      for (end = start + 1; end < real_end; end++)
        {
          int temp;

          temp = string_gcd (array[start], array[end]);

          if (temp <= gcd_zero)
            break;

          gcd = temp;
        }
      end--;

      if (gcd_zero == 0 && start == real_start && end != (real_end - 1))
        {
          /* In this case, add in a leading '{', because we are at
             top level, and there isn't a consistent prefix. */
          result.push_back ('{');
          flag = true;
        }

      /* Make sure we backslash quote every substring we insert into the
         resultant brace expression.  This is so the default filename
         quoting function won't inappropriately quote the braces. */
      if (start == end)
        {
          x = savestring (array[start] + gcd_zero);
          subterm = sh_backslash_quote (x, 0, 0);
          delete[] x;
        }
      else
        {
          /* If there is more than one element in the subarray,
             insert the (quoted) prefix and an opening brace. */
          tlen = gcd - gcd_zero;
          x = new char[tlen + 1];
          strncpy (x, array[start] + gcd_zero, tlen);
          x[tlen] = '\0';
          subterm = sh_backslash_quote (x, 0, 0);
          delete[] x;
          result.append (subterm);
          delete[] subterm;
          result.push_back ('{');
          subterm = really_munge_braces (array, start, end + 1, gcd);
          subterm[strlen (subterm) - 1] = '}';
        }

      result.append (subterm);
      result.push_back (',');
      delete[] subterm;
    }

  if (gcd_zero == 0 && flag)
    result.push_back ('}');

  return savestring (result);
}

static int
_strcompare (char **s1, char **s2)
{
  int result;

  result = **s1 - **s2;
  if (result == 0)
    result = strcmp (*s1, *s2);

  return result;
}

static int
hack_braces_completion (char **names)
{
  int i = strvec_len (names);
  if (MB_CUR_MAX > 1 && i > 2)
    qsort (names + 1, i - 1, sizeof (char *), (QSFUNC *)_strcompare);

  char *temp = really_munge_braces (names, 1, i, 0);

  for (i = 0; names[i]; ++i)
    {
      free (names[i]);
      names[i] = NULL;
    }
  names[0] = temp;
  return 0;
}

/* We handle quoting ourselves within hack_braces_completion, so we turn off
   rl_filename_quoting_desired and rl_filename_quoting_function. */
int
bash_brace_completion (int count, int ignore)
{
  rl_compignore_func_t *orig_ignore_func;
  rl_compentry_func_t *orig_entry_func;
  rl_quote_func_t *orig_quoting_func;
  rl_completion_func_t *orig_attempt_func;
  int orig_quoting_desired, r;

  orig_ignore_func = rl_ignore_some_completions_function;
  orig_attempt_func = rl_attempted_completion_function;
  orig_entry_func = rl_completion_entry_function;
  orig_quoting_func = rl_filename_quoting_function;
  orig_quoting_desired = rl_filename_quoting_desired;

  rl_completion_entry_function = rl_filename_completion_function;
  rl_attempted_completion_function = (rl_completion_func_t *)NULL;
  rl_ignore_some_completions_function = hack_braces_completion;
  rl_filename_quoting_function = (rl_quote_func_t *)NULL;
  rl_filename_quoting_desired = 0;

  r = rl_complete_internal (TAB);

  rl_ignore_some_completions_function = orig_ignore_func;
  rl_attempted_completion_function = orig_attempt_func;
  rl_completion_entry_function = orig_entry_func;
  rl_filename_quoting_function = orig_quoting_func;
  rl_filename_quoting_desired = orig_quoting_desired;

  return r;
}

} // namespace bash

#endif /* BRACE_EXPANSION && READLINE */
