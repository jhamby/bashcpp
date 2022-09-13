/* bashversion.c -- Display bash version information. */

/* Copyright (C) 2001-2020 Free Software Foundation, Inc.

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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashtypes.h"
#include "conftypes.h"
#include "version.h"

#define RFLAG 0x0001
#define VFLAG 0x0002
#define MFLAG 0x0004
#define PFLAG 0x0008
#define SFLAG 0x0010
#define LFLAG 0x0020
#define XFLAG 0x0040

extern int optind;
extern char *optarg;

namespace bash
{

extern char *shell_version_string (void);
extern void show_shell_version (bool);

} // namespace bash

static const char *progname;

static void
usage ()
{
  std::fprintf (stderr, "%s: usage: %s [-hrvpmlsx]\n", progname, progname);
}

int
main (int argc, char **argv)
{
  int opt, oflags;
  char dv[128];
  char *rv = nullptr;

  if ((progname = std::strrchr (argv[0], '/')))
    progname++;
  else
    progname = argv[0];

  oflags = 0;
  while ((opt = ::getopt (argc, argv, "hrvmpslx")) != EOF)
    {
      switch (opt)
        {
        case 'h':
          usage ();
          std::exit (0);
        case 'r':
          oflags |= RFLAG; /* release */
          break;
        case 'v':
          oflags |= VFLAG; /* version */
          break;
        case 'm':
          oflags |= MFLAG; /* machtype */
          break;
        case 'p':
          oflags |= PFLAG; /* patchlevel */
          break;
        case 's': /* short version string */
          oflags |= SFLAG;
          break;
        case 'l': /* long version string */
          oflags |= LFLAG;
          break;
        case 'x': /* extended version information */
          oflags |= XFLAG;
          break;
        default:
          usage ();
          std::exit (2);
        }
    }

  argc -= optind;
  argv += optind;

  if (argc > 0)
    {
      usage ();
      std::exit (2);
    }

  /* default behavior */
  if (oflags == 0)
    oflags = SFLAG;

  if (oflags & (RFLAG | VFLAG))
    {
      std::strcpy (dv, bash::dist_version);
      rv = std::strchr (dv, '.');
      if (rv)
        *rv++ = '\0';
      else
        rv = const_cast<char *> ("00");
    }
  if (oflags & RFLAG)
    std::printf ("%s\n", dv);
  else if (oflags & VFLAG)
    std::printf ("%s\n", rv);
  else if (oflags & MFLAG)
    std::printf ("%s\n", MACHTYPE);
  else if (oflags & PFLAG)
    std::printf ("%d\n", bash::patch_level);
  else if (oflags & SFLAG)
    std::printf ("%s\n", bash::shell_version_string ());
  else if (oflags & LFLAG)
    bash::show_shell_version (0);
  else if (oflags & XFLAG)
    bash::show_shell_version (1);

  std::exit (0);
}
