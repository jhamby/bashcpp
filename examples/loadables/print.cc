/*
 * print -- loadable ksh-93 style print builtin
 */

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

#include "builtext.hh"
#include "builtins.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

// Loadable class for "print".
class ShellLoadable : public Shell
{
public:
  int print_builtin (WORD_LIST *);

private:
};

static int printargs ();

static FILE *ofp;

extern char *this_command_name;

static const char *const print_doc[]
    = { "Display arguments.",
        "",
        "Output the arguments.  The -f option means to use the argument as a",
        "format string as would be supplied to printf(1).  The rest of the",
        "options are as in ksh.",
        nullptr };

Shell::builtin print_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::print_builtin),
    print_doc, "print [-Rnprs] [-u unit] [-f format] [arguments]", nullptr,
    BUILTIN_ENABLED);

#ifndef ISOPTION
#define ISOPTION(s, c) (s[0] == '-' && s[2] == '\0' && s[1] == c)
#endif

int
ShellLoadable::print_builtin (WORD_LIST *list)
{
  int c, r, nflag, raw, ofd, sflag;
  int64_t lfd;
  char **v, *pfmt, *arg;
  WORD_LIST *l;

  nflag = raw = sflag = 0;
  ofd = 1;
  pfmt = 0;

  reset_internal_getopt ();
  while ((c = internal_getopt (list, "Rnprsu:f:")) != -1)
    {
      switch (c)
        {
        case 'R':
          raw = 2;
          loptend = lcurrent;
          if (loptend && ISOPTION (loptend->word->word, 'n'))
            {
              loptend = loptend->next;
              nflag = 1;
            }
          goto opt_end;
        case 'r':
          raw = 1;
          break;
        case 'n':
          nflag = 1;
          break;
        case 's':
          sflag = 1;
          break;
        case 'p':
          break; /* NOP */
        case 'u':
          if (all_digits (list_optarg) && legal_number (list_optarg, &lfd)
              && lfd == (int)lfd)
            ofd = lfd;
          else
            {
              for (l = list; l->next && l->next != lcurrent; l = l->next)
                ;
              lcurrent = loptend = l;
              goto opt_end;
            }
          break;
        case 'f':
          pfmt = list_optarg;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }

opt_end:
  list = loptend;

  ofp = (ofd == 1) ? stdout : fdopen (dup (ofd), "w");

  if (pfmt)
    {
      WORD_DESC *w;
      WORD_LIST *nlist;

      w = make_word (pfmt);
      nlist = make_word_list (w, list);
      r = printf_builtin (nlist);
      nlist->next = (WORD_LIST *)NULL;
      dispose_words (nlist);
      return r;
    }

  if (raw)
    {
      for (l = list; l; l = l->next)
        {
          fprintf (ofp, "%s", l->word->word);
          if (l->next)
            fprintf (ofp, " ");
        }
      if (nflag == 0)
        fprintf (ofp, "\n");
      fflush (ofp);
      return 0;
    }

  r = printargs (list, ofp);
  if (r && nflag == 0)
    fprintf (ofp, "\n");
  if (ofd != 1)
    fclose (ofp);
  return 0;
}

static int
printargs (WORD_LIST *list, FILE *ofp)
{
  WORD_LIST *l;
  char *ostr;
  int sawc;

  for (sawc = 0, l = list; l; l = l->next)
    {
      ostr = ansicstr (l->word->word, strlen (l->word->word), 0, &sawc,
                       (int *)0);
      if (ostr)
        fprintf (ofp, "%s", ostr);
      free (ostr);
      if (sawc)
        return 0;
      if (l->next)
        fprintf (ofp, " ");
    }
  return 1;
}

} // namespace bash
