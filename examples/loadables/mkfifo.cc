/* mkfifo - make FIFOs */

/* See Makefile for compilation details. */

/*
   Copyright (C) 1999-2020 Free Software Foundation, Inc.

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

// Loadable class for "mkfifo".
class ShellLoadable : public Shell
{
public:
  int mkfifo_builtin (WORD_LIST *);

private:
};

#define ISOCTAL(c) ((c) >= '0' && (c) <= '7')

extern int parse_symbolic_mode ();

static int original_umask;

int
mkfifo_builtin (WORD_LIST *list)
{
  int opt, mflag, omode, rval, nmode, basemode;
  char *mode;
  WORD_LIST *l;

  mflag = 0;
  mode = (char *)NULL;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "m:")) != -1)
    switch (opt)
      {
      case 'm':
        mflag = 1;
        mode = list_optarg;
        break;
        CASE_HELPOPT;
      default:
        builtin_usage ();
        return EX_USAGE;
      }
  list = loptend;

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  basemode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
  if (mode == NULL)
    omode = basemode;
  else if (ISOCTAL (*mode)) /* octal number */
    {
      omode = read_octal (mode);
      if (omode < 0)
        {
          builtin_error ("invalid file mode: %s", mode);
          return EXECUTION_FAILURE;
        }
    }
  else /* symbolic mode */
    {
      /* initial bits are a=rwx; the mode argument modifies them */
      omode = parse_symbolic_mode (mode, basemode);
      if (omode < 0)
        {
          builtin_error ("invalid file mode: %s", mode);
          return EXECUTION_FAILURE;
        }
    }

  /* Make the new mode */
  original_umask = umask (0);
  umask (original_umask);

  nmode = basemode & ~original_umask;
  /* Adjust new mode based on mode argument */
  nmode &= omode;

  for (rval = EXECUTION_SUCCESS, l = list; l; l = l->next)
    {
      if (mkfifo (l->word->word, nmode) < 0)
        {
          builtin_error ("cannot create FIFO `%s': %s", l->word->word,
                         strerror (errno));
          rval = EXECUTION_FAILURE;
        }
    }
  return rval;
}

const char *mkfifo_doc[]
    = { "Create FIFOs (named pipes).",
        "",
        "Make FIFOs.  Create the FIFOs named as arguments, in",
        "the order specified, using mode a=rw as modified by the current",
        "umask (see `help umask').  The -m option causes the file permission",
        "bits of the final FIFO to be MODE.  The MODE argument may be",
        "an octal number or a symbolic mode like that used by chmod(1).  If",
        "a symbolic mode is used, the operations are interpreted relative to",
        "an initial mode of \"a=rw\".  mkfifo returns 0 if the FIFOs are",
        "umask, plus write and search permissions for the owner.  mkdir",
        "created successfully, and non-zero if an error occurs.",
        nullptr };

Shell::builtin mkfifo_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::mkfifo_builtin),
    mkfifo_doc, "mkfifo [-m mode] fifo_name [fifo_name ...]", nullptr,
    BUILTIN_ENABLED);

} // namespace bash
