/* evalstring.cc - evaluate a string as one or more shell commands. */

/* Copyright (C) 1996-2022 Free Software Foundation, Inc.

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

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "bashintl.hh"
#include "builtins.hh"
#include "flags.hh"
#include "input.hh"
#include "jobs.hh"
#include "parser.hh"
#include "shell.hh"
#include "trap.hh"

#include "builtext.hh"
#include "common.hh"

namespace bash
{

bool
Shell::should_optimize_fork (const COMMAND *command, bool subshell)
{
  if (command->type != cm_simple)
    return false;

  const SIMPLE_COM *scm = static_cast<const SIMPLE_COM *> (command);

  return running_trap == 0 && !signal_is_trapped (EXIT_TRAP)
         && !signal_is_trapped (ERROR_TRAP) && any_signals_trapped () < 0
         && (subshell
             || (scm->redirects == nullptr
                 && scm->simple_redirects == nullptr))
         && (scm->flags & (CMD_TIME_PIPELINE | CMD_INVERT_RETURN)) == 0;
}

/* This has extra tests to account for STARTUP_STATE == 2, which is for
   -c command but has been extended to command and process substitution
   (basically any time you call parse_and_execute in a subshell). */
bool
Shell::should_suppress_fork (const COMMAND *command)
{
  bool subshell;

  subshell = subshell_environment & SUBSHELL_PROCSUB; /* salt to taste */
  return startup_state == 2 && parse_and_execute_level == 1
         && *bash_input.location.string == '\0' && !parser_expanding_alias ()
         && should_optimize_fork (command, subshell);
}

bool
Shell::can_optimize_connection (const CONNECTION *connection)
{
  return *bash_input.location.string == '\0' && !parser_expanding_alias ()
         && (connection->connector == parser::token::AND_AND
             || connection->connector == parser::token::OR_OR
             || connection->connector == ';')
         && connection->second->type == cm_simple;
}

void
Shell::optimize_connection_fork (CONNECTION *connection)
{
  if ((connection->connector == parser::token::AND_AND
       || connection->connector == parser::token::OR_OR
       || connection->connector == ';')
      && (connection->second->flags & CMD_TRY_OPTIMIZING)
      && ((startup_state == 2 && should_suppress_fork (connection->second))
          || ((subshell_environment & SUBSHELL_PAREN)
              && should_optimize_fork (connection->second, false))))
    {
      connection->second->flags |= CMD_NO_FORK;
    }
}

void
Shell::optimize_subshell_command (COMMAND *command)
{
  if (should_optimize_fork (command, false))
    command->flags |= CMD_NO_FORK;
  else if (command->type == cm_connection)
    {
      CONNECTION *connection = static_cast<CONNECTION *> (command);
      if ((connection->connector == parser::token::AND_AND
           || connection->connector == parser::token::OR_OR
           || connection->connector == ';')
          && connection->second->type == cm_simple
          && !parser_expanding_alias ())
        {
          connection->second->flags |= CMD_TRY_OPTIMIZING;
        }
    }
}

void
Shell::optimize_shell_function (COMMAND *command)
{
  COMMAND *fc;
  if (command->type == cm_group)
    fc = static_cast<GROUP_COM *> (command)->command;
  else
    fc = command;

  if (fc->type == cm_simple && should_suppress_fork (fc))
    fc->flags |= CMD_NO_FORK;
  else if (fc->type == cm_connection)
    {
      CONNECTION *connection = static_cast<CONNECTION *> (command);
      if (can_optimize_connection (connection)
          && should_suppress_fork (connection->second))
        connection->second->flags |= CMD_NO_FORK;
    }
}

bool
Shell::can_optimize_cat_file (const COMMAND *command)
{
  if (command->type != cm_simple)
    return false;

  const SIMPLE_COM *scm = static_cast<const SIMPLE_COM *> (command);

  return command->redirects == nullptr
         && (command->flags & CMD_TIME_PIPELINE) == 0 && scm->words == nullptr
         && scm->simple_redirects && scm->simple_redirects->next () == nullptr
         && scm->simple_redirects->instruction == r_input_direction
         && scm->simple_redirects->redirector.r.dest == 0;
}

// Unwind handler class for parse_and_execute () to restore shell state before
// return or during a throw.
class EvalStringUnwindHandler
{
public:
  // This constructor replaces parse_prologue (). It initializes the shell,
  // local parse_flags, and a copy of the_printed_command_except_trap.
  EvalStringUnwindHandler (Shell &s, char *string, parse_flags pf)
      : shell (s), the_printed_command_except_trap (
                       shell.the_printed_command_except_trap),
        flags (pf)
  {
    parse_and_execute_level = shell.parse_and_execute_level;
    indirection_level = shell.indirection_level;
    line_number = shell.line_number;
    line_number_for_err_trap = shell.line_number_for_err_trap;
    loop_level = shell.loop_level;
    executing_list = shell.executing_list;
    comsub_ignore_return = shell.comsub_ignore_return;

    if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
      {
        interactive = shell.interactive;
        shell.interactive = (flags & SEVAL_INTERACT);
      }

#if defined(HISTORY)
    remember_on_history = shell.remember_on_history;
#if defined(BANG_HISTORY)
    history_expansion_inhibited = shell.history_expansion_inhibited;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

    interactive_shell = shell.interactive_shell;
    if (interactive_shell)
      current_prompt_level = shell.get_current_prompt_level ();

    if (shell.parser_expanding_alias ())
      parser_expanding_alias = true;

    if (string && ((flags & SEVAL_NOFREE) == 0))
      orig_string_to_free = string;

#if defined(HISTORY)
    if (flags & SEVAL_NOHIST)
      shell.bash_history_disable ();

#if defined(BANG_HISTORY)
    if (flags & SEVAL_NOHISTEXP)
      shell.history_expansion_inhibited = true;
#endif /* BANG_HISTORY */
#endif /* HISTORY */
  }

  ~EvalStringUnwindHandler () { unwind (); }

  // Unwind all values stored in this object. May be called more than once.
  // These are restored in the opposite order from how they were saved.
  void
  unwind ()
  {
    if (unwound)
      return;

    // Delete the original string if this is non-nullptr.
    delete[] orig_string_to_free;

    if (parser_expanding_alias)
      shell.parser_restore_alias ();

    // Note: this call doesn't have a corresponding push in the constructor.
    shell.pop_stream ();

    if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
      shell.interactive = interactive;

    if (!the_printed_command_except_trap.empty ())
      shell.the_printed_command_except_trap = the_printed_command_except_trap;

    shell.comsub_ignore_return = comsub_ignore_return;
    shell.executing_list = executing_list;
    shell.loop_level = loop_level;
    shell.line_number_for_err_trap = line_number_for_err_trap;
    shell.line_number = line_number;
    shell.indirection_level = indirection_level;
    shell.parse_and_execute_level = parse_and_execute_level;

#if defined(HISTORY)
    if (parse_and_execute_level == 0)
      shell.remember_on_history = shell.enable_history_list;
    else
      shell.remember_on_history = remember_on_history;
#if defined(BANG_HISTORY)
    shell.history_expansion_inhibited = history_expansion_inhibited;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

    unwound = true;
  }

private:
  Shell &shell;
  std::string the_printed_command_except_trap;
  char *orig_string_to_free;
  parse_flags flags;
  int parse_and_execute_level;
  int indirection_level;
  int line_number;
  int line_number_for_err_trap;
  int loop_level;
  int executing_list;
  int current_prompt_level;
  bool unwound;
  bool comsub_ignore_return;
  bool interactive;
  char remember_on_history;
  bool history_expansion_inhibited;
  bool interactive_shell;
  bool parser_expanding_alias;
};

/* Parse and execute the commands in C string STRING.  Returns whatever
   execute_command () returns. This deletes STRING. FLAGS is a flags word; look
   in common.hh for the possible values. Actions are:

   (flags & SEVAL_NONINT) -> interactive = 0;
   (flags & SEVAL_INTERACT) -> interactive = 1;
   (flags & SEVAL_NOHIST) -> call bash_history_disable ()
   (flags & SEVAL_NOFREE) -> don't free STRING when finished
   (flags & SEVAL_RESETLINE) -> reset line_number to 1
   (flags & SEVAL_NOHISTEXP) -> history_expansion_inhibited -> 1
*/
int
Shell::parse_and_execute (char *string, string_view from_file,
                          parse_flags flags)
{
  // Make a local unwind handler to delete the string and restore variable and
  // parser state on return or throw.
  EvalStringUnwindHandler unwind_handler (*this, string, flags);

  parse_and_execute_level++;

  parse_flags lreset = flags & SEVAL_RESETLINE;

#if defined(HAVE_POSIX_SIGNALS)
  /* If we throw and are going to go on, use this to restore signal mask */
  sigset_t pe_sigmask;
  sigemptyset (&pe_sigmask);
  sigprocmask (SIG_BLOCK, nullptr, &pe_sigmask);
#endif

  /* Reset the line number if the caller wants us to.  If we don't reset the
     line number, we have to subtract one, because we will add one just
     before executing the next command (resetting the line number sets it to
     0; the first line number is 1). */
  push_stream (lreset);

  if (parser_expanding_alias ())
    /* push current shell_input_line */
    parser_save_alias ();

  if (lreset == 0)
    line_number--;

  indirection_level++;

  int last_result = EXECUTION_SUCCESS;
  bash_exception_t exception_code = NOEXCEPTION;

  /* We need to reset enough of the token state so we can start fresh. */
  if (current_token == parser::token::yacc_EOF)
    current_token = static_cast<parser::token_kind_type> ('\n');

  // Call reset_parser ()?

  with_input_from_string (string, from_file);
  clear_shell_input_line ();

  while (*(bash_input.location.string) || parser_expanding_alias ())
    {
      COMMAND *command = nullptr;

      if (interrupt_state)
        {
          last_result = EXECUTION_FAILURE;
          break;
        }

      // Catch and handle some top-level exceptions here. This prevents, e.g.
      // errors in substitution from restarting the reader loop directly.
      try
        {
          if (parse_command () == 0)
            {
              if ((flags & SEVAL_PARSEONLY)
                  || (!interactive_shell && read_but_dont_execute))
                {
                  last_result = EXECUTION_SUCCESS;
                  delete global_command;
                  global_command = nullptr;
                }
              else
                {
                  if (flags & SEVAL_FUNCDEF)
                    {
                      std::string::iterator x;

                      // If the command parses to something other than a
                      // straight function definition, or if we have not
                      // consumed the entire string, or if the parser has
                      // transformed the function name (as parsing will if it
                      // begins or ends with shell whitespace, for example),
                      // reject the attempt.
                      if (command->type != cm_function_def
                          || ((x = parser_remaining_input ())
                              != shell_input_line.end ())
                          || (from_file
                              != static_cast<FUNCTION_DEF *> (command)
                                     ->name->word))
                        {
                          internal_warning (
                              _ ("%s: ignoring function definition attempt"),
                              to_string (from_file).c_str ());

                          last_result = last_command_exit_value = EX_BADUSAGE;
                          set_pipestatus_from_exit (last_command_exit_value);
                          reset_parser ();
                          break;
                        }
                    }

                  try_execute_command ();

                  if (flags & SEVAL_ONECMD)
                    {
                      reset_parser ();
                      break;
                    }
                }
            }
          else
            {
              last_result = EX_BADUSAGE; /* was EXECUTION_FAILURE */

              if (!interactive_shell
                  && (this_shell_builtin == &Shell::source_builtin
                      || this_shell_builtin == &Shell::eval_builtin)
                  && last_command_exit_value == EX_BADSYNTAX && posixly_correct
                  && !executing_command_builtin)
                {
                  exception_code = ERREXIT;
                  last_command_exit_value = EX_BADUSAGE;
                }

              /* Since we are shell compatible, syntax errors in a script
                 abort the execution of the script.  Right? */
              break;
            }
        }
      catch (const bash_exception &e)
        {
          switch (e.type)
            {
            case ERREXIT:
              /* variable_context -> 0 is what eval.c:reader_loop() does in
                 these circumstances.  Don't bother with cleanup here because
                 we don't want to run the function execution cleanup stuff
                 that will cause pop_context and other functions to run.
                 We call reset_local_contexts() instead, which just frees
                 context memory.
                 XXX - change that if we want the function context to be
                 unwound. */
              if (exit_immediately_on_error && variable_context)
                reset_local_contexts (); /* not in a function */
              exception_code = e.type;
              goto out;

            case FORCE_EOF:
            case EXITPROG:
              exception_code = e.type;
              goto out;

            case EXITBLTIN:
              exception_code = e.type;
              goto out;

            case DISCARD:
              last_result = last_command_exit_value
                  = EXECUTION_FAILURE; /* XXX */
              set_pipestatus_from_exit (last_command_exit_value);
              if (subshell_environment)
                {
                  exception_code = e.type;
                  goto out;
                }
              else
                {
#if defined(HAVE_POSIX_SIGNALS)
                  sigprocmask (SIG_SETMASK, &pe_sigmask, nullptr);
#endif
                  break;
                }

            case NOEXCEPTION:
            default:
              command_error ("parse_and_execute", CMDERR_BADJUMP, e.type);
              /* NOTREACHED */
            }
        }
    }

out:

  unwind_handler.unwind ();

  if (interrupt_state && parse_and_execute_level == 0)
    {
      /* An interrupt during non-interactive execution in an
         interactive shell (e.g. via $PROMPT_COMMAND) should
         not cause the shell to exit. */
      interactive = interactive_shell;
      throw_to_top_level ();
    }

  CHECK_TERMSIG;

  if (exception_code != NOEXCEPTION)
    throw bash_exception (exception_code);

  return last_result;
}

int
Shell::try_execute_command ()
{
  fd_bitmap *bitmap = new fd_bitmap ();
  COMMAND *command = global_command;
  global_command = nullptr;

  if ((subshell_environment & SUBSHELL_COMSUB) && comsub_ignore_return)
    command->flags |= CMD_IGNORE_RETURN;

#if defined(ONESHOT)
  /*
   * IF
   *   we were invoked as `bash -c' (startup_state == 2) AND
   *   parse_and_execute has not been called recursively AND
   *   we're not running a trap AND
   *   we have parsed the full command (string == '\0') AND
   *   we're not going to run the exit trap AND
   *   we have a simple command without redirections AND
   *   the command is not being timed AND
   *   the command's return status is not being inverted AND
   *   there aren't any traps in effect
   * THEN
   *   tell the execution code that we don't need to fork
   */
  if (should_suppress_fork (command))
    command->flags |= CMD_NO_FORK;

  // Can't optimize forks out here except for simple commands. This knows that
  // the parser sets up commands as left-side heavy (&& and || are
  // left-associative) and after the single parse, if we are at the end of the
  // command string, the last in a series of connection commands is
  // connection->second.
  else if (command->type == cm_connection)
    {
      CONNECTION *connection = static_cast<CONNECTION *> (command);
      if (can_optimize_connection (connection))
        connection->second->flags |= CMD_TRY_OPTIMIZING;
    }

#endif /* ONESHOT */

  try
    {
      int result;

      /* See if this is a candidate for $( <file ). */
      if (startup_state == 2 && (subshell_environment & SUBSHELL_COMSUB)
          && *bash_input.location.string == '\0'
          && can_optimize_cat_file (command))
        {
          SIMPLE_COM *simple_com = static_cast<SIMPLE_COM *> (command);
          int r = cat_file (simple_com->simple_redirects);
          result = (r < 0) ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
        }
      else
        {
          result = execute_command_internal (command, 0, NO_PIPE, NO_PIPE,
                                             bitmap);
        }

      delete command;
      delete bitmap;
      return result;
    }
  catch (const std::exception &)
    {
      delete command;
      delete bitmap;
      throw;
    }
}

/* Parse a command contained in STRING according to FLAGS and return the
   number of characters consumed from the string.  If non-NULL, set *ENDP
   to the position in the string where the parse ended.  Used to validate
   command substitutions during parsing to obey Posix rules about finding
   the end of the command and balancing parens. */
size_t
Shell::parse_string (char *string, string_view from_file, parse_flags flags,
                     COMMAND **cmdp, char **endp)
{
  COMMAND *command, *oglobal;
  char *ostring;
  sigset_t ps_sigmask;

  // This will restore the previous state on destruction or unwind ().
  EvalStringUnwindHandler unwind_handler (*this, string, flags);

#if defined(HAVE_POSIX_SIGNALS)
  /* If we throw and are going to go on, use this to restore signal mask */
  sigemptyset (&ps_sigmask);
  sigprocmask (SIG_BLOCK, nullptr, &ps_sigmask);
#endif

  /* Reset the line number if the caller wants us to.  If we don't reset the
     line number, we have to subtract one, because we will add one just
     before executing the next command (resetting the line number sets it to
     0; the first line number is 1). */
  push_stream (0);

  if (parser_expanding_alias ())
    /* push current shell_input_line */
    parser_save_alias ();

  bash_exception_t exception_code = NOEXCEPTION;
  oglobal = global_command;

  with_input_from_string (string, from_file);
  ostring = bash_input.location.string;

  while (*(bash_input.location.string)) /* XXX - parser_expanding_alias () ? */
    {
      command = nullptr;

#if 0
      if (interrupt_state)
	break;
#endif

      // Catch and handle some top-level exceptions here. This prevents, e.g.
      // errors in substitution from restarting the reader loop directly.
      try
        {
          if (parse_command () == 0)
            {
              if (cmdp)
                *cmdp = global_command;
              else
                delete global_command;
              global_command = nullptr;
            }
          else
            {
              if ((flags & SEVAL_NOTHROW) == 0)
                exception_code = DISCARD;
              else
                reset_parser (); /* XXX - sets token_to_read */
              break;
            }

          if (current_token == parser::token::yacc_EOF
              || current_token == shell_eof_token)
            {
              if (current_token == shell_eof_token)
                rewind_input_string ();
              break;
            }
        }
      catch (const bash_exception &e)
        {
          internal_debug ("parse_string: exception caught: code = %d", e.type);

          switch (e.type)
            {
            case FORCE_EOF:
            case ERREXIT:
            case EXITPROG:
            case EXITBLTIN:
            case DISCARD: /* XXX */
              if (command)
                delete command;

              exception_code = e.type;
              goto out;

            case NOEXCEPTION:
            default:
#if defined(HAVE_POSIX_SIGNALS)
              sigprocmask (SIG_SETMASK, &ps_sigmask, nullptr);
#endif
              command_error ("parse_string", CMDERR_BADJUMP, e.type);
              /* NOTREACHED */
            }
        }
    }

out:

  global_command = oglobal;
  size_t nc = static_cast<size_t> (bash_input.location.string - ostring);

  if (endp)
    *endp = bash_input.location.string;

  /* If we throw DISCARD, the caller (xparse_dolparen) will catch and rethrow
     for us, after doing cleanup */
  if (exception_code != NOEXCEPTION)
    throw bash_exception (exception_code);

  return nc;
}

int
Shell::open_redir_file (REDIRECT *r, char **fnp)
{
  char *fn;
  int fd;

  if (r->instruction != r_input_direction)
    return -1;

  /* Get the filename. */
  if (posixly_correct && !interactive_shell)
    disallow_filename_globbing++;
  fn = redirection_expand (r->redirectee.r.filename);
  if (posixly_correct && !interactive_shell)
    disallow_filename_globbing--;

  if (fn == nullptr)
    {
      redirection_error (r, AMBIGUOUS_REDIRECT, fn);
      return -1;
    }

  fd = open (fn, O_RDONLY);
  if (fd < 0)
    {
      file_error (fn);
      delete[] fn;
      if (fnp)
        *fnp = nullptr;
      return -1;
    }

  if (fnp)
    *fnp = fn;
  return fd;
}

/* Handle a $( < file ) command substitution.  This expands the filename,
   returning errors as appropriate, then just cats the file to the standard
   output. */
int
Shell::cat_file (REDIRECT *r)
{
  char *fn;

  int fd = open_redir_file (r, &fn);
  if (fd < 0)
    return -1;

  int rval = static_cast<int> (zcatfd (fd, 1, fn));

  delete[] fn;
  close (fd);

  return rval;
}

int
Shell::evalstring (char *string, string_view from_file, parse_flags flags)
{
  /* Are we running a trap when we execute this function? */
  int was_trap = running_trap;

  int rflag = return_catch_flag;

  /* If we are in a place where `return' is valid, we have to catch
     `eval "... return"' and make sure parse_and_execute cleans up. Then
     we can trampoline to the previous saved return_catch location. */
  if (rflag)
    {
      return_catch_flag++; /* increment so we have a counter */
      try
        {
          /* Note that parse_and_execute () frees the string it is passed. */
          int r = parse_and_execute (string, from_file, flags);
          return_catch_flag = rflag;
          return r;
        }
      catch (const return_catch_exception &)
        {
          /* We care about whether or not we are running the same trap we were
             when we entered this function. */
          if (running_trap > 0)
            {
              /* We assume if we have a different value for running_trap than
                 when we started (the only caller that cares is evalstring()),
                 the original caller will perform the cleanup, and we should
                 not step on them. */
              if (running_trap != was_trap)
                run_trap_cleanup (running_trap - 1);
              unfreeze_jobs_list ();
            }

          return_catch_flag = rflag;
          throw;
        }
    }
  else
    return parse_and_execute (string, from_file, flags);
}

} // namespace bash
