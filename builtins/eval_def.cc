// This file is eval_def.cc.
// It implements the builtin "eval" in Bash.

// Copyright (C) 1987-2016 Free Software Foundation, Inc.

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

// $BUILTIN eval
// $FUNCTION eval_builtin
// $SHORT_DOC eval [arg ...]
// Execute arguments as a shell command.

// Combine ARGs into a single string, use the result as input to the shell,
// and execute the resulting commands.

// Exit Status:
// Returns exit status of command or success if command is null.
// $END

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "common.hh"
#include "shell.hh"

namespace bash
{

/* Parse the string that these words make, and execute the command found. */
int
Shell::eval_builtin (WORD_LIST *list)
{
  if (no_options (list))
    return EX_USAGE;
  list = loptend; /* skip over possible `--' */

  return list ? evalstring (savestring (string_list (list)), "eval",
                            SEVAL_NOHIST)
              : EXECUTION_SUCCESS;
}

} // namespace bash
