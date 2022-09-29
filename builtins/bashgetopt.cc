/* bashgetopt.cc -- `getopt' for use by the builtins. */

/* Copyright (C) 1992-2021 Free Software Foundation, Inc.

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
  bool plus; /* true means to handle +option */

  plus = (*opts == '+');
  if (plus)
    opts++;

  if (list == nullptr)
    {
      list_optarg = nullptr;
      list_optflags = W_NOFLAGS;
      loptend = nullptr; /* No non-option arguments */
      return -1;
    }

  if (list != lhead || lhead == nullptr)
    {
      /* Hmmm.... called with a different word list.  Reset. */
      getopt_sp = 1;
      lcurrent = lhead = list;
      loptend = nullptr;
    }

  const char *current_word = lcurrent->word->word.c_str ();

  if (getopt_sp == 1)
    {
      if (lcurrent == 0 || NOTOPT (current_word))
        {
          lhead = nullptr;
          loptend = lcurrent;
          return -1;
        }
      else if (ISHELP (current_word))
        {
          lhead = nullptr;
          loptend = lcurrent;
          return GETOPT_HELP;
        }
      else if (current_word[0] == '-' && current_word[1] == '-'
               && current_word[2] == 0)
        {
          lhead = nullptr;
          loptend = lcurrent->next ();
          return -1;
        }
      getopt_errstr[0] = list_opttype = current_word[0];
    }

  int c;
  const char *cp;
  list_optopt = c = current_word[getopt_sp];

  if (c == ':' || (cp = strchr (opts, c)) == NULL)
    {
      getopt_errstr[1] = c;
      sh_invalidopt (getopt_errstr);
      if (current_word[++getopt_sp] == '\0')
        {
          lcurrent = lcurrent->next ();
          current_word = lcurrent->word->word.c_str ();
          getopt_sp = 1;
        }
      list_optarg = nullptr;
      list_optflags = W_NOFLAGS;
      if (lcurrent)
        loptend = lcurrent->next ();
      return '?';
    }

  if (*++cp == ':' || *cp == ';')
    {
      /* `:': Option requires an argument. */
      /* `;': option argument may be missing */
      /* We allow -l2 as equivalent to -l 2 */
      if (current_word[getopt_sp + 1])
        {
          list_optarg = current_word + getopt_sp + 1;
          list_optflags = W_NOFLAGS;
          lcurrent = lcurrent->next ();
          /* If the specifier is `;', don't set optarg if the next
             argument looks like another option. */
        }
      else if (lcurrent->next ()
               && (*cp == ':'
                   || NOTOPT ((lcurrent->next ())->word->word.c_str ())))
        {
          lcurrent = lcurrent->next ();
          list_optarg = lcurrent->word->word.c_str ();
          list_optflags = W_NOFLAGS;
          lcurrent = lcurrent->next ();
        }
      else if (*cp == ';')
        {
          list_optarg = nullptr;
          list_optflags = W_NOFLAGS;
          lcurrent = lcurrent->next ();
        }
      else
        { /* lcurrent->next == NULL */
          getopt_errstr[1] = c;
          sh_needarg (getopt_errstr);
          getopt_sp = 1;
          list_optarg = nullptr;
          list_optflags = W_NOFLAGS;
          return ('?');
        }
      getopt_sp = 1;
    }
  else if (*cp == '#')
    {
      /* option requires a numeric argument */
      if (lcurrent->word->word[getopt_sp + 1])
        {
          if (std::isdigit (lcurrent->word->word[getopt_sp + 1]))
            {
              list_optarg = lcurrent->word->word.c_str () + getopt_sp + 1;
              list_optflags = W_NOFLAGS;
              lcurrent = lcurrent->next ();
            }
          else
            {
              list_optarg = nullptr;
              list_optflags = W_NOFLAGS;
            }
        }
      else
        {
          if (lcurrent->next ()
              && legal_number ((lcurrent->next ())->word->word.c_str (), nullptr))
            {
              lcurrent = lcurrent->next ();
              list_optarg = lcurrent->word->word.c_str ();
              list_optflags = W_NOFLAGS;
              lcurrent = lcurrent->next ();
            }
          else
            {
              getopt_errstr[1] = c;
              sh_neednumarg (getopt_errstr);
              getopt_sp = 1;
              list_optarg = nullptr;
              list_optflags = W_NOFLAGS;
              return '?';
            }
        }
    }
  else
    {
      /* No argument, just return the option. */
      if (lcurrent->word->word[++getopt_sp] == '\0')
        {
          getopt_sp = 1;
          lcurrent = lcurrent->next ();
        }
      list_optarg = nullptr;
      list_optflags = W_NOFLAGS;
    }

  return c;
}

/*
 * reset_internal_getopt -- force the in[ft]ernal getopt to reset
 */

void
Shell::reset_internal_getopt ()
{
  lhead = lcurrent = loptend = nullptr;
  getopt_sp = 1;
}

} // namespace bash
