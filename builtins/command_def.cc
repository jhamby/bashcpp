// This file is command_def.cc
// It implements the builtin "command" in Bash.

// Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

// $BUILTIN command
// $FUNCTION command_builtin
// $SHORT_DOC command [-pVv] command [arg ...]
// Execute a simple command or display information about commands.

// Runs COMMAND with ARGS suppressing  shell function lookup, or display
// information about the specified COMMANDs.  Can be used to invoke commands
// on disk when a function with the same name exists.

// Options:
//   -p    use a default value for PATH that is guaranteed to find all of
//         the standard utilities
//   -v    print a description of COMMAND similar to the `type' builtin
//   -V    print a more verbose description of each COMMAND

// Exit Status:
// Returns exit status of COMMAND, or failure if COMMAND is not found.
// $END

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "common.hh"
#include "flags.hh"
#include "shell.hh"

namespace bash
{

#if defined(_CS_PATH) && defined(HAVE_CONFSTR) && !HAVE_DECL_CONFSTR
extern size_t confstr (int, char *, size_t);
#endif

/* Run the commands mentioned in LIST without paying attention to shell
   functions. */
int
Shell::command_builtin (WORD_LIST *list)
{
  int result, verbose, use_standard_path, opt;
  COMMAND *command;

  verbose = use_standard_path = 0;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "pvV")) != -1)
    {
      switch (opt)
        {
        case 'p':
          use_standard_path = CDESC_STDPATH;
          break;
        case 'V':
          verbose = CDESC_SHORTDESC
                    | CDESC_ABSPATH; /* look in common.h for constants */
          break;
        case 'v':
          verbose = CDESC_REUSABLE; /* ditto */
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  if (list == 0)
    return EXECUTION_SUCCESS;

#if defined(RESTRICTED_SHELL)
  if (use_standard_path && restricted)
    {
      sh_restricted ("-p");
      return EXECUTION_FAILURE;
    }
#endif

  if (verbose)
    {
      int found, any_found;

      for (any_found = 0; list; list = list->next ())
        {
          found = describe_command (list->word->word,
                                    verbose | use_standard_path);

          if (found == 0 && verbose != CDESC_REUSABLE)
            sh_notfound (list->word->word);

          any_found += found;
        }

      return any_found ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
    }

  begin_unwind_frame ("command_builtin");

#define COMMAND_BUILTIN_FLAGS                                                 \
  (CMD_NO_FUNCTIONS | CMD_INHIBIT_EXPANSION | CMD_COMMAND_BUILTIN             \
   | (use_standard_path ? CMD_STDPATH : 0))

  internal_debug ("command_builtin: running execute_command for `%s'",
                  list->word->word);

  /* We don't want this to be reparsed (consider command echo 'foo &'), so
     just make a simple_command structure and call execute_command with it. */
  command = make_bare_simple_command ();
  command->value.Simple->words = (WORD_LIST *)copy_word_list (list);
  command->value.Simple->redirects = (REDIRECT *)NULL;
  command->flags |= COMMAND_BUILTIN_FLAGS;
  command->value.Simple->flags |= COMMAND_BUILTIN_FLAGS;

  add_unwind_protect_ptr (dispose_command, command);
  result = execute_command (command);

  run_unwind_frame ("command_builtin");

  return result;
}

} // namespace bash
