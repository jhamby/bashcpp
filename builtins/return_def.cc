// This file is return_def.cc.
// It implements the builtin "return" in Bash.

// Copyright (C) 1987-2015 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

// $BUILTIN return

// $FUNCTION return_builtin
// $SHORT_DOC return [n]
// Return from a shell function.

// Causes a function or sourced script to exit with the return value
// specified by N.  If N is omitted, the return status is that of the
// last command executed within the function or script.

// Exit Status:
// Returns N, or failure if the shell is not executing a function or script.
// $END

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "common.hh"
#include "shell.hh"

namespace bash
{

/* If we are executing a user-defined function then exit with the value
   specified as an argument.  if no argument is given, then the last
   exit status is used. */
int
Shell::return_builtin (WORD_LIST *list)
{
  CHECK_HELPOPT (list);

  return_catch_value = get_exitstat (list);

  if (return_catch_flag)
    sh_longjmp (return_catch, 1);
  else
    {
      builtin_error (
          _ ("can only `return' from a function or sourced script"));
      return EX_USAGE;
    }
}

} // namespace bash
