/* strftime - loadable builtin interface to strftime(3) */

/* See Makefile for compilation details. */

/*
   Copyright (C) 1999-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash.
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

#include "builtins.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

// Loadable class for "strftime".
class ShellLoadable : public Shell
{
public:
  int strftime_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::strftime_builtin (WORD_LIST *list)
{
  char *format, *tbuf;
  size_t tbsize, tsize;
  time_t secs;
  struct tm *t;
  int n;
  int64_t i;

  if (no_options (list))
    return EX_USAGE;
  list = loptend;

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  format = list->word->word;
  if (format == 0 || *format == 0)
    {
      printf ("\n");
      return EXECUTION_SUCCESS;
    }

  list = list->next;

  if (list && list->word->word)
    {
      n = legal_number (list->word->word, &i);
      if (n == 0 || i < 0 || i != (time_t)i)
        {
          sh_invalidnum (list->word->word);
          return EXECUTION_FAILURE;
        }
      else
        secs = i;
    }
  else
    secs = NOW;

  t = localtime (&secs);

  tbsize = strlen (format) * 4;
  tbuf = 0;

  /* Now try to figure out how big the buffer should really be.  strftime(3)
     will return the number of bytes placed in the buffer unless it's greater
     than MAXSIZE, in which case it returns 0. */
  for (n = 1; n <= 8; n++)
    {
      tbuf = xrealloc (tbuf, tbsize * n);
      tsize = strftime (tbuf, tbsize * n, format, t);
      if (tsize)
        break;
    }

  if (tsize)
    printf ("%s\n", tbuf);
  free (tbuf);

  return EXECUTION_SUCCESS;
}

/* An array of strings forming the `long' documentation for a builtin xxx,
   which is printed by `help xxx'.  It must end with a NULL. */
static const char *const strftime_doc[]
    = { "Display formatted time.",
        "",
        "Converts date and time format to a string and displays it on the",
        "standard output.  If the optional second argument is supplied, it",
        "is used as the number of seconds since the epoch to use in the",
        "conversion, otherwise the current time is used.",
        nullptr };

Shell::builtin strftime_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::strftime_builtin),
    strftime_doc, "strftime format [seconds]", nullptr, BUILTIN_ENABLED);

} // namespace bash
