/*
 * mksyntax.cc - construct shell syntax table for fast char attribute lookup.
 */

/* Copyright (C) 2000-2009 Free Software Foundation, Inc.

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

#include "general.hh"

#include <unistd.h>

#include "syntax.hh"

namespace bash
{

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif

static struct wordflag
{
  char_flags flag;
  const char *fstr;
} wordflags[] = {
  { CWORD, "CWORD" },       { CSHMETA, "CSHMETA" },     { CSHBRK, "CSHBRK" },
  { CBACKQ, "CBACKQ" },     { CQUOTE, "CQUOTE" },       { CSPECL, "CSPECL" },
  { CEXP, "CEXP" },         { CBSDQUOTE, "CBSDQUOTE" }, { CBSHDOC, "CBSHDOC" },
  { CGLOB, "CGLOB" },       { CXGLOB, "CXGLOB" },       { CXQUOTE, "CXQUOTE" },
  { CSPECVAR, "CSPECVAR" }, { CSUBSTOP, "CSUBSTOP" },   { CBLANK, "CBLANK" },
};

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#define N_WFLAGS static_cast<int> (sizeof (wordflags) / sizeof (wordflags[0]))
#define SYNSIZE 256

static int lsyntax[SYNSIZE];
static int debug;
static char *progname;

static const char preamble[] = "\
/*\n\
 * This file was generated by mksyntax.  DO NOT EDIT.\n\
 */\n\
\n";

static const char includes[] = "\
#include \"config.h\"\n\
#include \"syntax.hh\"\n\
#include \"shell.hh\"\n\n\
namespace bash {\n\n";

static void __attribute__ ((__noreturn__)) usage ()
{
  fprintf (stderr, "%s: usage: %s [-d] [-o filename]\n", progname,
                progname);
  exit (2);
}

static const char *
cdesc (int i)
{
  static char xbuf[16];

  if (i == ' ')
    return "SPC";
  else if (c_isprint (i))
    {
      xbuf[0] = static_cast<char> (i);
      xbuf[1] = '\0';
      return xbuf;
    }
  else if (i == CTLESC)
    return "CTLESC";
  else if (i == CTLNUL)
    return "CTLNUL";
  else if (i == '\033') /* ASCII */
    return "ESC";

  xbuf[0] = '\\';
  xbuf[2] = '\0';

  switch (i)
    {
    case '\a':
      xbuf[1] = 'a';
      break;
    case '\v':
      xbuf[1] = 'v';
      break;
    case '\b':
      xbuf[1] = 'b';
      break;
    case '\f':
      xbuf[1] = 'f';
      break;
    case '\n':
      xbuf[1] = 'n';
      break;
    case '\r':
      xbuf[1] = 'r';
      break;
    case '\t':
      xbuf[1] = 't';
      break;
    default:
      sprintf (xbuf, "%d", i);
      break;
    }

  return xbuf;
}

static const char *
getcstr (int f)
{
  for (int i = 0; i < N_WFLAGS; i++)
    if (f == wordflags[i].flag)
      return wordflags[i].fstr;

  return nullptr;
}

static void
addcstr (const char *str, int flag)
{
  const char *s, *fstr;

  for (s = str; s && *s; s++)
    {
      unsigned char uc = static_cast<unsigned char> (*s);

      if (debug)
        {
          fstr = getcstr (flag);
          fprintf (stderr, "added %s for character %s\n", fstr,
                        cdesc (uc));
        }

      lsyntax[uc] |= flag;
    }
}

static void
addcchar (unsigned char c, int flag)
{
  const char *fstr;

  if (debug)
    {
      fstr = getcstr (flag);
      fprintf (stderr, "added %s for character %s\n", fstr, cdesc (c));
    }
  lsyntax[c] |= flag;
}

static void
addblanks ()
{
  for (int i = 0; i < SYNSIZE; i++)
    {
      unsigned char uc = static_cast<unsigned char> (i);

      /* Since we don't call setlocale(), this defaults to the "C" locale, and
         the default blank characters will be space and tab. */
      if (bash::isblank (uc))
        lsyntax[uc] |= CBLANK;
    }
}

/* load up the correct flag values in lsyntax */
static void
load_lsyntax ()
{
  /* shell metacharacters */
  addcstr (shell_meta_chars, CSHMETA);

  /* shell word break characters */
  addcstr (shell_break_chars, CSHBRK);

  addcchar ('`', CBACKQ);

  addcstr (shell_quote_chars, CQUOTE);

  addcchar (CTLESC, CSPECL);
  addcchar (CTLNUL, CSPECL);

  addcstr (shell_exp_chars, CEXP);

  addcstr (slashify_in_quotes, CBSDQUOTE);
  addcstr (slashify_in_here_document, CBSHDOC);

  addcstr (shell_glob_chars, CGLOB);

#if defined(EXTENDED_GLOB)
  addcstr (ext_glob_chars, CXGLOB);
#endif

  addcstr (shell_quote_chars, CXQUOTE);
  addcchar ('\\', CXQUOTE);

  addcstr ("@*#?-$!", CSPECVAR); /* omits $0...$9 and $_ */

  addcstr ("-=?+", CSUBSTOP); /* OP in ${paramOPword} */

  addblanks ();
}

static void
dump_lflags (FILE *fp, int ind)
{
  int xflags, first, i;

  xflags = lsyntax[ind];
  first = 1;

  if (xflags == 0)
    fputs (wordflags[0].fstr, fp);
  else
    {
      for (i = 1; i < N_WFLAGS; i++)
        if (xflags & wordflags[i].flag)
          {
            if (first)
              first = 0;
            else
              putc ('|', fp);
            fputs (wordflags[i].fstr, fp);
          }
    }
}

static void
wcomment (FILE *fp, int i)
{
  fputs ("\t\t/* ", fp);

  fprintf (fp, "%s", cdesc (i));

  fputs (" */", fp);
}

static void
dump_lsyntax (FILE *fp)
{
  int i;

  // fprintf (fp, "int sh_syntabsiz = %d;\n", SYNSIZE);	// declared
  // externally as const
  fprintf (fp, "char_flags Shell::sh_syntaxtab[%d] = {\n", SYNSIZE);

  for (i = 0; i < SYNSIZE; i++)
    {
      putc ('\t', fp);
      dump_lflags (fp, i);
      putc (',', fp);
      wcomment (fp, i);
      putc ('\n', fp);
    }

  fprintf (fp, "};\n");
}

} // namespace bash

int
main (int argc, char **argv)
{
  int opt, i;
  const char *filename;
  FILE *fp;

  if ((bash::progname = strrchr (argv[0], '/')) == nullptr)
    bash::progname = argv[0];
  else
    bash::progname++;

  filename = nullptr;
  bash::debug = 0;

  while ((opt = getopt (argc, argv, "do:")) != EOF)
    {
      switch (opt)
        {
        case 'd':
          bash::debug = 1;
          break;
        case 'o':
          filename = optarg;
          break;
        default:
          bash::usage ();
        }
    }

  argc -= optind;
  argv += optind;

  if (filename)
    {
      fp = fopen (filename, "w");
      if (fp == nullptr)
        {
          fprintf (stderr, "%s: %s: cannot open: %s\n", bash::progname,
                        filename, strerror (errno));
          exit (1);
        }
    }
  else
    {
      filename = "stdout";
      fp = stdout;
    }

  for (i = 0; i < SYNSIZE; i++)
    bash::lsyntax[i] = bash::CWORD;

  bash::load_lsyntax ();

  fprintf (fp, "%s\n", bash::preamble);
  fprintf (fp, "%s\n", bash::includes);

  bash::dump_lsyntax (fp);

  fprintf (fp, "}\n"); /* end namespace bash */

  if (fp != stdout)
    fclose (fp);
  exit (0);
}
