/* Sample builtin to be dynamically loaded with enable -f and create a new
   builtin. */

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

#include "loadables.hh"

namespace bash
{

// Loadable class for "hello".
class ShellLoadable : public Shell
{
public:
  int hello_builtin (WORD_LIST *);

private:
};

/* A builtin `xxx' is normally implemented with an `xxx_builtin' function.
   If you're converting a command that uses the normal Unix argc/argv
   calling convention, use argv = make_builtin_argv (list, &argc) and call
   the original `main' something like `xxx_main'.  Look at cat.c for an
   example.

   Builtins should use internal_getopt to parse options.  It is the same as
   getopt(3), but it takes a WORD_LIST *.  Look at print.c for an example
   of its use.

   If the builtin takes no options, call no_options(list) before doing
   anything else.  If it returns a non-zero value, your builtin should
   immediately return EX_USAGE.  Look at logname.c for an example.

   A builtin command returns EXECUTION_SUCCESS for success and
   EXECUTION_FAILURE to indicate failure. */
int
ShellLoadable::hello_builtin (WORD_LIST *list)
{
  printf ("hello world\n");
  fflush (stdout);
  return (EXECUTION_SUCCESS);
}

int
hello_builtin_load (char *s)
{
  printf ("hello builtin loaded\n");
  fflush (stdout);
  return (1);
}

void
hello_builtin_unload (char *s)
{
  printf ("hello builtin unloaded\n");
  fflush (stdout);
}

/* An array of strings forming the `long' documentation for a builtin xxx,
   which is printed by `help xxx'.  It must end with a NULL.  By convention,
   the first line is a short description. */
static const char *const hello_doc[]
    = { "Sample builtin.", "",
        "this is the long doc for the sample hello builtin", nullptr };

Shell::builtin hello_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::hello_builtin),
    hello_doc, "hello", nullptr, BUILTIN_ENABLED);

} // namespace bash
