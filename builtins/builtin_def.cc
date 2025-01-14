// This file is builtin_def.cc
// It implements the builtin "builtin" in Bash.

// Copyright (C) 1987-2017 Free Software Foundation, Inc.

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

// $BUILTIN builtin
// $FUNCTION builtin_builtin
// $SHORT_DOC builtin [shell-builtin [arg ...]]
// Execute shell builtins.

// Execute SHELL-BUILTIN with arguments ARGs without performing command
// lookup.  This is useful when you wish to reimplement a shell builtin
// as a shell function, but need to execute the builtin within the function.

// Exit Status:
// Returns the exit status of SHELL-BUILTIN, or false if SHELL-BUILTIN is
// not a shell builtin.
// $END

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "common.hh"
#include "shell.hh"

namespace bash
{

/* Run the command mentioned in list directly, without going through the
   normal alias/function/builtin/filename lookup process. */
int
Shell::builtin_builtin (WORD_LIST *list)
{
  sh_builtin_func_t function;

  if (no_options (list))
    return EX_USAGE;
  list = loptend; /* skip over possible `--' */

  if (list == 0)
    return EXECUTION_SUCCESS;

  const char *command = list->word->word;
#if defined(DISABLED_BUILTINS)
  function = builtin_address (command);
#else  /* !DISABLED_BUILTINS */
  function = find_shell_builtin (command);
#endif /* !DISABLED_BUILTINS */

  if (function == 0)
    {
      sh_notbuiltin (command);
      return EXECUTION_FAILURE;
    }
  else
    {
      this_command_name = command;
      this_shell_builtin = function; /* overwrite "builtin" as this builtin */
      list = list->next ();
      return (*function) (list);
    }
}

} // namespace bash
