/* stringlib.cc - Miscellaneous string functions. */

/* Copyright (C) 1996-2009 Free Software Foundation, Inc.

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

#include "bashtypes.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "chartypes.hh"

#include "pathexp.hh"
#include "shell.hh"

#include "glob.hh"

#if defined(EXTENDED_GLOB)
#include "strmatch.hh"
#endif

namespace bash
{

/* **************************************************************** */
/*								    */
/*		Functions to manage arrays of strings		    */
/*								    */
/* **************************************************************** */

/* Find STRING in ALIST, a list of string key/int value pairs.  If FLAGS
   is 1, STRING is treated as a pattern and matched using strmatch. */
int
find_string_in_alist (char *string, STRING_INT_ALIST *alist, int flags)
{
  int i;
  int r;

  for (i = r = 0; alist[i].word; i++)
    {
#if defined(EXTENDED_GLOB)
      if (flags)
        r = strmatch (alist[i].word, string, FNM_EXTMATCH) != FNM_NOMATCH;
      else
#endif
        r = STREQ (string, alist[i].word);

      if (r)
        return alist[i].token;
    }
  return -1;
}

int
find_index_in_alist (char *string, STRING_INT_ALIST *alist, int flags)
{
  int i;
  int r;

  for (i = r = 0; alist[i].word; i++)
    {
#if defined(EXTENDED_GLOB)
      if (flags)
        r = strmatch (alist[i].word, string, FNM_EXTMATCH) != FNM_NOMATCH;
      else
#endif
        r = STREQ (string, alist[i].word);

      if (r)
        return i;
    }

  return -1;
}

/* **************************************************************** */
/*								    */
/*		    String Management Functions			    */
/*								    */
/* **************************************************************** */

/* Replace occurrences of PAT with REP in STRING.  If GLOBAL is non-zero,
   replace all occurrences, otherwise replace only the first.
   This returns a new string; the caller should free it. */
char *
strsub (const char *str, const char *pat, const char *rep, bool global)
{
  size_t patlen = strlen (pat);
  std::string temp;

  int i;
  bool repl;
  for (i = 0, repl = true; str[i];)
    {
      if (repl && STREQN (str + i, pat, patlen))
        {
          for (const char *r = rep; *r;) /* can rep == "" */
            temp += *r++;

          i += patlen ? patlen : 1; /* avoid infinite recursion */
          repl = global != 0;
        }
      else
        {
          temp += str[i++];
        }
    }

  return savestring (temp);
}

/* Replace all instances of C in STRING with TEXT.  TEXT may be empty or
   NULL.  If DO_GLOB is non-zero, we quote the replacement text for
   globbing.  Backslash may be used to quote C. */
char *
strcreplace (const char *string, int c, const char *text, bool do_glob)
{
  size_t len = strlen (text);
  std::string ret;

  const char *p;
  int ind;
  for (p = string, ind = 0; p && *p; ++ind)
    {
      if (*p == c)
        {
          if (len)
            {
              if (do_glob
                  && (glob_pattern_p (text) || strchr (text, '\\')))
                {
                  char *t = quote_globbing_chars (text);
                  ret.append (t);
                  delete[] t;
                }
              else
                {
                  ret.append (text);
                }
            }
          p++;
          continue;
        }

      if (*p == '\\' && p[1] == c)
        p++;

      ret += *p++;
    }

  return savestring (ret);
}

/* Remove all trailing whitespace from STRING.  This includes
   newlines.  If NEWLINES_ONLY is non-zero, only trailing newlines
   are removed.  STRING should be terminated with a zero. */
void
strip_trailing (char *string, int len, bool newlines_only)
{
  while (len >= 0)
    {
      if ((newlines_only && string[len] == '\n')
          || (!newlines_only && whitespace (string[len])))
        len--;
      else
        break;
    }
  string[len + 1] = '\0';
}

} // namespace bash
