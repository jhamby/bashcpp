/* bashgetopt.c -- `getopt' for use by the builtins. */

/* Copyright (C) 1992-2002 Free Software Foundation, Inc.

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

#include "config.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "chartypes.hh"

#include <cerrno>

#include "common.hh"
#include "shell.hh"

#include "bashgetopt.hh"

namespace bash
{

#define ISOPT(s) (((*(s) == '-') || (plus && *(s) == '+')) && (s)[1])
#define NOTOPT(s) (((*(s) != '-') && (!plus || *(s) != '+')) || (s)[1] == '\0')

// static int	sp;

// char    *list_optarg;
// int	list_optopt;
// int	list_opttype;

// static WORD_LIST *lhead = nullptr;
// WORD_LIST	*lcurrent = nullptr;
// WORD_LIST	*loptend;	/* Points to the first non-option argument in
// the list */

int
Shell::internal_getopt (WORD_LIST *list, const char *opts)
{
  int plus; /* nonzero means to handle +option */
  static char errstr[3] = { '-', '\0', '\0' };

  plus = *opts == '+';
  if (plus)
    opts++;

  if (list == 0)
    {
      list_optarg = nullptr;
      loptend = nullptr; /* No non-option arguments */
      return -1;
    }

  if (list != lhead || lhead == 0)
    {
      /* Hmmm.... called with a different word list.  Reset. */
      sp = 1;
      lcurrent = lhead = list;
      loptend = nullptr;
    }

  if (sp == 1)
    {
      if (lcurrent == 0 || NOTOPT (lcurrent->word->word))
        {
          lhead = nullptr;
          loptend = lcurrent;
          return (-1);
        }
      else if (ISHELP (lcurrent->word->word))
        {
          lhead = nullptr;
          loptend = lcurrent;
          return GETOPT_HELP;
        }
      else if (lcurrent->word->word[0] == '-' && lcurrent->word->word[1] == '-'
               && lcurrent->word->word[2] == 0)
        {
          lhead = nullptr;
          loptend = (WORD_LIST *)lcurrent->next;
          return (-1);
        }
      errstr[0] = list_opttype = lcurrent->word->word[0];
    }

  int c;
  const char *cp;
  list_optopt = c = lcurrent->word->word[sp];

  if (c == ':' || (cp = strchr (opts, c)) == NULL)
    {
      errstr[1] = c;
      sh_invalidopt (errstr);
      if (lcurrent->word->word[++sp] == '\0')
        {
          lcurrent = (WORD_LIST *)lcurrent->next;
          sp = 1;
        }
      list_optarg = NULL;
      if (lcurrent)
        loptend = (WORD_LIST *)lcurrent->next;
      return ('?');
    }

  if (*++cp == ':' || *cp == ';')
    {
      /* `:': Option requires an argument. */
      /* `;': option argument may be missing */
      /* We allow -l2 as equivalent to -l 2 */
      if (lcurrent->word->word[sp + 1])
        {
          list_optarg = lcurrent->word->word + sp + 1;
          lcurrent = (WORD_LIST *)lcurrent->next;
          /* If the specifier is `;', don't set optarg if the next
             argument looks like another option. */
#if 0
		} else if (lcurrent->next && (*cp == ':' || lcurrent->next->word->word[0] != '-')) {
#else
        }
      else if (lcurrent->next
               && (*cp == ':'
                   || NOTOPT (((WORD_LIST *)lcurrent->next)->word->word)))
        {
#endif
          lcurrent = (WORD_LIST *)lcurrent->next;
          list_optarg = lcurrent->word->word;
          lcurrent = (WORD_LIST *)lcurrent->next;
        }
      else if (*cp == ';')
        {
          list_optarg = nullptr;
          lcurrent = (WORD_LIST *)lcurrent->next;
        }
      else
        { /* lcurrent->next == NULL */
          errstr[1] = c;
          sh_needarg (errstr);
          sp = 1;
          list_optarg = nullptr;
          return ('?');
        }
      sp = 1;
    }
  else if (*cp == '#')
    {
      /* option requires a numeric argument */
      if (lcurrent->word->word[sp + 1])
        {
          if (DIGIT (lcurrent->word->word[sp + 1]))
            {
              list_optarg = lcurrent->word->word + sp + 1;
              lcurrent = (WORD_LIST *)lcurrent->next;
            }
          else
            list_optarg = nullptr;
        }
      else
        {
          if (lcurrent->next
              && legal_number (((WORD_LIST *)lcurrent->next)->word->word,
                               (int64_t *)0))
            {
              lcurrent = (WORD_LIST *)lcurrent->next;
              list_optarg = lcurrent->word->word;
              lcurrent = (WORD_LIST *)lcurrent->next;
            }
          else
            {
              errstr[1] = c;
              sh_neednumarg (errstr);
              sp = 1;
              list_optarg = nullptr;
              return '?';
            }
        }
    }
  else
    {
      /* No argument, just return the option. */
      if (lcurrent->word->word[++sp] == '\0')
        {
          sp = 1;
          lcurrent = (WORD_LIST *)lcurrent->next;
        }
      list_optarg = nullptr;
    }

  return (c);
}

/*
 * reset_internal_getopt -- force the in[ft]ernal getopt to reset
 */

void
Shell::reset_internal_getopt ()
{
  lhead = lcurrent = loptend = nullptr;
  sp = 1;
}

} // namespace bash