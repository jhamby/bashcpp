/* head - copy first part of files. */

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

#include "bashtypes.hh"
#include "posixstat.hh"

#include <unistd.h>

#include "chartypes.hh"

#include "builtins.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

// Loadable class for "head".
class ShellLoadable : public Shell
{
public:
  int head_builtin (WORD_LIST *);

private:
};

static void
munge_list (WORD_LIST *list)
{
  WORD_LIST *l, *nl;
  WORD_DESC *wd;
  char *arg;

  for (l = list; l; l = l->next)
    {
      arg = l->word->word;
      if (arg[0] != '-' || arg[1] == '-' || (DIGIT (arg[1]) == 0))
        return;
      /* We have -[0-9]* */
      wd = make_bare_word (arg + 1);
      nl = make_word_list (wd, l->next);
      l->word->word[1] = 'n';
      l->word->word[2] = '\0';
      l->next = nl;
      l = nl; /* skip over new argument */
    }
}

static int
file_head (FILE *fp, int cnt)
{
  int ch;

  while (cnt--)
    {
      while ((ch = getc (fp)) != EOF)
        {
          if (putchar (ch) == EOF)
            {
              builtin_error ("write error: %s", strerror (errno));
              return EXECUTION_FAILURE;
            }
          if (ch == '\n')
            break;
        }
    }
  return EXECUTION_SUCCESS;
}

int
ShellLoadable::head_builtin (WORD_LIST *list)
{
  int nline, opt, rval;
  WORD_LIST *l;
  FILE *fp;

  char *t;

  munge_list (list); /* change -num into -n num */

  reset_internal_getopt ();
  nline = 10;
  while ((opt = internal_getopt (list, "n:")) != -1)
    {
      switch (opt)
        {
        case 'n':
          nline = atoi (list_optarg);
          if (nline <= 0)
            {
              builtin_error ("bad line count: %s", list_optarg);
              return EX_USAGE;
            }
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  if (list == 0)
    return file_head (stdin, nline);

  for (rval = EXECUTION_SUCCESS, opt = 1, l = list; l; l = l->next ())
    {
      fp = fopen (l->word->word, "r");
      if (fp == NULL)
        {
          builtin_error ("%s: %s", l->word->word, strerror (errno));
          continue;
        }
      if (list->next ()) /* more than one file */
        {
          printf ("%s==> %s <==\n", opt ? "" : "\n", l->word->word);
          opt = 0;
        }
      rval = file_head (fp, nline);
      fclose (fp);
    }

  return rval;
}

static const char *const head_doc[]
    = { "Display lines from beginning of file.",
        "",
        "Copy the first N lines from the input files to the standard output.",
        "N is supplied as an argument to the `-n' option.  If N is not given,",
        "the first ten lines are copied.",
        nullptr };

Shell::builtin head_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::head_builtin),
    head_doc, "head [-n num] [file ...]", nullptr, BUILTIN_ENABLED);

} // namespace bash
