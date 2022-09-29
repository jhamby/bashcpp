// This file is source_def.cc.
// It implements the builtins "." and  "source" in Bash.

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

// $BUILTIN source
// $FUNCTION source_builtin
// $SHORT_DOC source filename [arguments]
// Execute commands from a file in the current shell.

// Read and execute commands from FILENAME in the current shell.  The
// entries in $PATH are used to find the directory containing FILENAME.
// If any ARGUMENTS are supplied, they become the positional parameters
// when FILENAME is executed.

// Exit Status:
// Returns the status of the last command executed in FILENAME; fails if
// FILENAME cannot be read.
// $END

// $BUILTIN .
// $DOCNAME dot
// $FUNCTION source_builtin
// $SHORT_DOC . filename [arguments]
// Execute commands from a file in the current shell.

// Read and execute commands from FILENAME in the current shell.  The
// entries in $PATH are used to find the directory containing FILENAME.
// If any ARGUMENTS are supplied, they become the positional parameters
// when FILENAME is executed.

// Exit Status:
// Returns the status of the last command executed in FILENAME; fails if
// FILENAME cannot be read.
// $END

#include "config.hh"

#include "bashtypes.hh"
#include "filecntl.hh"
#include "posixstat.hh"

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "common.hh"
#include "findcmd.hh"
#include "flags.hh"
#include "shell.hh"
#include "trap.hh"

namespace bash
{

#if 0
/* If non-zero, `.' uses $PATH to look up the script to be sourced. */
char source_uses_path = 1;

/* If non-zero, `.' looks in the current directory if the filename argument
   is not found in the $PATH. */
bool source_searches_cwd = true;
#endif

/* If this . script is supplied arguments, we save the dollar vars and
   replace them with the script arguments for the duration of the script's
   execution.  If the script does not change the dollar vars, we restore
   what we saved.  If the dollar vars are changed in the script, and we are
   not executing a shell function, we leave the new values alone and free
   the saved values. */
void
Shell::maybe_pop_dollar_vars ()
{
  if (variable_context == 0 && (dollar_vars_changed () & ARGS_SETBLTIN))
    dispose_saved_dollar_vars ();
  else
    pop_dollar_vars ();
  if (debugging_mode)
    pop_args (); /* restore BASH_ARGC and BASH_ARGV */
  set_dollar_vars_unchanged ();
  invalidate_cached_quoted_dollar_at (); /* just invalidate to be safe */
}

/* Read and execute commands from the file passed as argument.  Guess what.
   This cannot be done in a subshell, since things like variable assignments
   take place in there.  So, I open the file, place it into a large string,
   close the file, and then execute the string. */
int
Shell::source_builtin (WORD_LIST *list)
{
  int result;
  char *filename, *debug_trap, *x;

  if (no_options (list))
    return EX_USAGE;
  list = loptend;

  if (list == 0)
    {
      builtin_error (_ ("filename argument required"));
      builtin_usage ();
      return EX_USAGE;
    }

#if defined(RESTRICTED_SHELL)
  if (restricted && strchr (list->word->word, '/'))
    {
      sh_restricted (list->word->word);
      return EXECUTION_FAILURE;
    }
#endif

  filename = (char *)NULL;
  /* XXX -- should this be absolute_pathname? */
  if (posixly_correct && strchr (list->word->word, '/'))
    filename = savestring (list->word->word);
  else if (absolute_pathname (list->word->word))
    filename = savestring (list->word->word);
  else if (source_uses_path)
    filename = find_path_file (list->word->word);
  if (filename == 0)
    {
      if (source_searches_cwd == 0)
        {
          x = printable_filename (list->word->word, 0);
          builtin_error (_ ("%s: file not found"), x);
          if (x != list->word->word)
            free (x);
          if (posixly_correct && interactive_shell == 0
              && executing_command_builtin == 0)
            {
              last_command_exit_value = EXECUTION_FAILURE;
              jump_to_top_level (EXITPROG);
            }
          return EXECUTION_FAILURE;
        }
      else
        filename = savestring (list->word->word);
    }

  begin_unwind_frame ("source");
  add_unwind_protect_ptr (xfree, filename);

  if (list->next)
    {
      push_dollar_vars ();
      add_unwind_protect (maybe_pop_dollar_vars);
      if (debugging_mode || shell_compatibility_level <= 44)
        init_bash_argv (); /* Initialize BASH_ARGV and BASH_ARGC */
      remember_args ((WORD_LIST *)list->next, true);
      if (debugging_mode)
        push_args (
            (WORD_LIST *)list->next); /* Update BASH_ARGV and BASH_ARGC */
    }
  set_dollar_vars_unchanged ();

  /* Don't inherit the DEBUG trap unless function_trace_mode (overloaded)
     is set.  XXX - should sourced files inherit the RETURN trap?  Functions
     don't. */
  debug_trap = TRAP_STRING (DEBUG_TRAP);
  if (debug_trap && function_trace_mode == 0)
    {
      debug_trap = savestring (debug_trap);
      add_unwind_protect_ptr (xfree, debug_trap);
      add_unwind_protect_ptr (maybe_set_debug_trap, debug_trap);
      restore_default_signal (DEBUG_TRAP);
    }

  result = source_file (filename, (list && list->next));

  run_unwind_frame ("source");

  return result;
}

} // namespace bash
