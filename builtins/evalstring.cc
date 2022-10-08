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

#include "config.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <signal.h>
#include <stdio.h>

#include <errno.h>

#include "filecntl.hh"

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

#define IS_BUILTIN(s) (builtin_address_internal (s, 0) != nullptr)

bool
Shell::should_optimize_fork (const COMMAND *command, bool subshell)
{
  const SIMPLE_COM *scm = dynamic_cast<const SIMPLE_COM *> (command);
  return scm != nullptr && running_trap == 0
         && signal_is_trapped (EXIT_TRAP) == 0
         && signal_is_trapped (ERROR_TRAP) == 0 && any_signals_trapped () < 0
         && (subshell
             || (command->redirects == nullptr
                 && scm->simple_redirects == nullptr))
         && (command->flags & (CMD_TIME_PIPELINE | CMD_INVERT_RETURN)) == 0;
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
  else
    {
      CONNECTION *connection = dynamic_cast<CONNECTION *> (command);
      if (connection
          && (connection->connector == parser::token::AND_AND
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
  GROUP_COM *gcm = dynamic_cast<GROUP_COM *> (command);
  COMMAND *fc = gcm ? gcm->command : command;

  if (fc->type == cm_simple && should_suppress_fork (fc))
    fc->flags |= CMD_NO_FORK;
  else
    {
      CONNECTION *connection = dynamic_cast<CONNECTION *> (command);
      if (connection && can_optimize_connection (connection)
          && should_suppress_fork (connection->second))
        connection->second->flags |= CMD_NO_FORK;
    }
}

bool
Shell::can_optimize_cat_file (const COMMAND *command)
{
  const SIMPLE_COM *scm = dynamic_cast<const SIMPLE_COM *> (command);

  return scm && command->redirects == nullptr
         && (command->flags & CMD_TIME_PIPELINE) == 0 && scm->words == nullptr
         && scm->simple_redirects && scm->simple_redirects->next () == nullptr
         && scm->simple_redirects->instruction == r_input_direction
         && scm->simple_redirects->redirector.r.dest == 0;
}

// This class replaces parse_prologue ().
class EvalStringUnwindHandler
{
public:
  EvalStringUnwindHandler (Shell &s, const char *string, parse_flags pf)
      : shell (s), flags (pf)
  {
    parse_and_execute_level = shell.parse_and_execute_level;
    indirection_level = shell.indirection_level;
    line_number = shell.line_number;
    line_number_for_err_trap = shell.line_number_for_err_trap;
    loop_level = shell.loop_level;
    executing_list = shell.executing_list;
    comsub_ignore_return = shell.comsub_ignore_return;
    if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
      interactive = shell.interactive;

#if defined(HISTORY)
    remember_on_history = shell.remember_on_history;
#if defined(BANG_HISTORY)
    history_expansion_inhibited = shell.history_expansion_inhibited;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

    if (shell.interactive_shell)
      {
        x = get_current_prompt_level ();
        add_unwind_protect (set_current_prompt_level, x);
      }

    if (the_printed_command_except_trap)
      {
        lastcom = savestring (the_printed_command_except_trap);
        add_unwind_protect (restore_lastcom, lastcom);
      }

    add_unwind_protect (pop_stream, (char *)NULL);
    if (parser_expanding_alias ())
      add_unwind_protect (parser_restore_alias, (char *)NULL);

    if (orig_string && ((flags & SEVAL_NOFREE) == 0))
      add_unwind_protect (xfree, orig_string);
    end_unwind_frame ();

    if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
      interactive = (flags & SEVAL_NONINT) ? 0 : 1;

#if defined(HISTORY)
    if (flags & SEVAL_NOHIST)
      bash_history_disable ();
#if defined(BANG_HISTORY)
    if (flags & SEVAL_NOHISTEXP)
      history_expansion_inhibited = 1;
#endif /* BANG_HISTORY */
#endif /* HISTORY */
  }

  ~EvalStringUnwindHandler ()
  {
    if (!unwound)
      unwind ();
  }

  // This doesn't check if it's been called already.
  void
  unwind ()
  {
    shell.parse_and_execute_level = parse_and_execute_level;
    shell.indirection_level = indirection_level;
    shell.line_number = line_number;
    shell.line_number_for_err_trap = line_number_for_err_trap;
    shell.loop_level = loop_level;
    shell.executing_list = executing_list;
    shell.comsub_ignore_return = comsub_ignore_return;
    if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
      shell.interactive = interactive;

#if defined(HISTORY)
    shell.remember_on_history = remember_on_history;
#if defined(BANG_HISTORY)
    shell.history_expansion_inhibited = history_expansion_inhibited;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

    unwound = true;
  }

private:
  Shell &shell;
  parse_flags flags;
  int parse_and_execute_level;
  int indirection_level;
  int line_number;
  int line_number_for_err_trap;
  int loop_level;
  int executing_list;
  bool unwound;
  bool comsub_ignore_return;
  bool interactive;
  char remember_on_history;
  bool history_expansion_inhibited;
};

/* Parse and execute the commands in STRING.  Returns whatever
   execute_command () returns.  This deletes STRING.  FLAGS is a
   flags word; look in common.h for the possible values.  Actions
   are:
        (flags & SEVAL_NONINT) -> interactive = 0;
        (flags & SEVAL_INTERACT) -> interactive = 1;
        (flags & SEVAL_NOHIST) -> call bash_history_disable ()
        (flags & SEVAL_NOFREE) -> don't free STRING when finished
        (flags & SEVAL_RESETLINE) -> reset line_number to 1
        (flags & SEVAL_NOHISTEXP) -> history_expansion_inhibited -> 1
*/
int
Shell::parse_and_execute (const std::string &string,
                          const std::string &from_file, parse_flags flags)
{
  //   int code, lreset;
  //   volatile int should_jump_to_top_level, last_result;
  //   COMMAND *volatile command;

  // This will restore the previous state on destruction or unwind ().
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

  code = should_jump_to_top_level = 0;
  last_result = EXECUTION_SUCCESS;

  /* We need to reset enough of the token state so we can start fresh. */
  if (current_token == parser::token::yacc_EOF)
    current_token = '\n'; /* reset_parser() ? */

  with_input_from_string (string, from_file);
  clear_shell_input_line ();
  while (*(bash_input.location.string) || parser_expanding_alias ())
    {
      command = nullptr;

      if (interrupt_state)
        {
          last_result = EXECUTION_FAILURE;
          break;
        }

      /* Provide a location for functions which `longjmp (top_level)' to
         jump to.  This prevents errors in substitution from restarting
         the reader loop directly, for example. */
      code = setjmp_nosigs (top_level);

      if (code)
        {
          should_jump_to_top_level = 0;
          switch (code)
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
                {
                  discard_unwind_frame ("pe_dispose");
                  reset_local_contexts (); /* not in a function */
                }
              should_jump_to_top_level = 1;
              goto out;
            case FORCE_EOF:
            case EXITPROG:
              if (command)
                run_unwind_frame ("pe_dispose");
              /* Remember to call longjmp (top_level) after the old
                 value for it is restored. */
              should_jump_to_top_level = 1;
              goto out;

            case EXITBLTIN:
              if (command)
                {
                  if (variable_context && signal_is_trapped (0))
                    {
                      /* Let's make sure we run the exit trap in the function
                         context, as we do when not running parse_and_execute.
                         The pe_dispose unwind frame comes before any unwind-
                         protects installed by the string we're evaluating, so
                         it will undo the current function scope. */
                      dispose_command (command);
                      discard_unwind_frame ("pe_dispose");
                    }
                  else
                    run_unwind_frame ("pe_dispose");
                }
              should_jump_to_top_level = 1;
              goto out;

            case DISCARD:
              if (command)
                run_unwind_frame ("pe_dispose");
              last_result = last_command_exit_value
                  = EXECUTION_FAILURE; /* XXX */
              set_pipestatus_from_exit (last_command_exit_value);
              if (subshell_environment)
                {
                  should_jump_to_top_level = 1;
                  goto out;
                }
              else
                {
#if 0
		  dispose_command (command);	/* pe_dispose does this */
#endif
#if defined(HAVE_POSIX_SIGNALS)
                  sigprocmask (SIG_SETMASK, &pe_sigmask, nullptr);
#endif
                  continue;
                }

            default:
              command_error ("parse_and_execute", CMDERR_BADJUMP, code);
              break;
            }
        }

      if (parse_command () == 0)
        {
          if ((flags & SEVAL_PARSEONLY)
              || (!interactive_shell && read_but_dont_execute))
            {
              last_result = EXECUTION_SUCCESS;
              delete global_command;
              global_command = nullptr;
            }
          else if (command = global_command)
            {
              struct fd_bitmap *bitmap;

              if (flags & SEVAL_FUNCDEF)
                {
                  const char *x;

                  /* If the command parses to something other than a straight
                     function definition, or if we have not consumed the entire
                     string, or if the parser has transformed the function
                     name (as parsing will if it begins or ends with shell
                     whitespace, for example), reject the attempt */
                  if (command->type != cm_function_def
                      || ((x = parser_remaining_input ()) && *x)
                      || (STREQ (from_file,
                                 command->value.Function_def->name->word)
                          == 0))
                    {
                      internal_warning (
                          _ ("%s: ignoring function definition attempt"),
                          from_file);
                      should_jump_to_top_level = 0;
                      last_result = last_command_exit_value = EX_BADUSAGE;
                      set_pipestatus_from_exit (last_command_exit_value);
                      reset_parser ();
                      break;
                    }
                }

              bitmap = new_fd_bitmap (FD_BITMAP_SIZE);
              begin_unwind_frame ("pe_dispose");
              add_unwind_protect (dispose_fd_bitmap, bitmap);
              add_unwind_protect (dispose_command, command); /* XXX */

              global_command = nullptr;

              if ((subshell_environment & SUBSHELL_COMSUB)
                  && comsub_ignore_return)
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
                {
                  command->flags |= CMD_NO_FORK;
                  command->value.Simple->flags |= CMD_NO_FORK;
                }

              /* Can't optimize forks out here execept for simple commands.
                 This knows that the parser sets up commands as left-side heavy
                 (&& and || are left-associative) and after the single parse,
                 if we are at the end of the command string, the last in a
                 series of connection commands is
                 connection->second. */
              else if (command->type == cm_connection
                       && can_optimize_connection (command))
                {
                  command->value.Connection->second->flags
                      |= CMD_TRY_OPTIMIZING;
                  command->value.Connection->second->value.Simple->flags
                      |= CMD_TRY_OPTIMIZING;
                }
#endif /* ONESHOT */

              /* See if this is a candidate for $( <file ). */
              if (startup_state == 2
                  && (subshell_environment & SUBSHELL_COMSUB)
                  && *bash_input.location.string == '\0'
                  && can_optimize_cat_file (command))
                {
                  int r;
                  r = cat_file (command->value.Simple->redirects);
                  last_result
                      = (r < 0) ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
                }
              else
                last_result = execute_command_internal (command, 0, NO_PIPE,
                                                        NO_PIPE, bitmap);
              dispose_command (command);
              dispose_fd_bitmap (bitmap);
              discard_unwind_frame ("pe_dispose");

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

          if (interactive_shell == 0 && this_shell_builtin
              && (this_shell_builtin == source_builtin
                  || this_shell_builtin == eval_builtin)
              && last_command_exit_value == EX_BADSYNTAX && posixly_correct
              && executing_command_builtin == 0)
            {
              should_jump_to_top_level = 1;
              code = ERREXIT;
              last_command_exit_value = EX_BADUSAGE;
            }

          /* Since we are shell compatible, syntax errors in a script
             abort the execution of the script.  Right? */
          break;
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

  if (should_jump_to_top_level)
    throw bash_exception (code);

  return (last_result);
}

/* Parse a command contained in STRING according to FLAGS and return the
   number of characters consumed from the string.  If non-NULL, set *ENDP
   to the position in the string where the parse ended.  Used to validate
   command substitutions during parsing to obey Posix rules about finding
   the end of the command and balancing parens. */
size_t
Shell::parse_string (const std::string &string, const std::string &from_file,
                     parse_flags flags, COMMAND **cmdp,
                     std::string::const_iterator *ep)
{
  bool should_jump_to_top_level;
  COMMAND *command, *oglobal;
  char *ostring;
  sigset_t ps_sigmask;

  // This will restore the previous state on destruction or unwind ().
  EvalStringUnwindHandler unwind_handler (*this, string, flags);

#if defined(HAVE_POSIX_SIGNALS)
  /* If we longjmp and are going to go on, use this to restore signal mask */
  sigemptyset ((sigset_t *)&ps_sigmask);
  sigprocmask (SIG_BLOCK, (sigset_t *)NULL, (sigset_t *)&ps_sigmask);
#endif

  /* Reset the line number if the caller wants us to.  If we don't reset the
     line number, we have to subtract one, because we will add one just
     before executing the next command (resetting the line number sets it to
     0; the first line number is 1). */
  push_stream (0);
  if (parser_expanding_alias ())
    /* push current shell_input_line */
    parser_save_alias ();

  should_jump_to_top_level = false;
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

      /* Provide a location for functions which `longjmp (top_level)' to
         jump to. */
      code = setjmp_nosigs (top_level);

      if (code)
        {
          internal_debug ("parse_string: exception caught: code = %d", code);

          should_jump_to_top_level = 0;
          switch (code)
            {
            case FORCE_EOF:
            case ERREXIT:
            case EXITPROG:
            case EXITBLTIN:
            case DISCARD: /* XXX */
              if (command)
                delete command;
              /* Remember to call longjmp (top_level) after the old
                 value for it is restored. */
              should_jump_to_top_level = 1;
              goto out;

            default:
#if defined(HAVE_POSIX_SIGNALS)
              sigprocmask (SIG_SETMASK, (sigset_t *)&ps_sigmask,
                           (sigset_t *)NULL);
#endif
              command_error ("parse_string", CMDERR_BADJUMP, code);
              break;
            }
        }

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
            {
              should_jump_to_top_level = 1;
              code = DISCARD;
            }
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

out:

  global_command = oglobal;
  nc = bash_input.location.string - ostring;
  if (endp)
    *endp = bash_input.location.string;

  unwind_handler.unwind ();

  /* If we return < 0, the caller (xparse_dolparen) will jump_to_top_level for
     us, after doing cleanup */
  if (should_jump_to_top_level)
    {
      if (parse_and_execute_level == 0)
        top_level_cleanup ();
      if (code == DISCARD)
        return -DISCARD;
      jump_to_top_level (code);
    }

  return (nc);
}

int
Shell::open_redir_file (REDIRECT *r, char **fnp)
{
  char *fn;
  int fd, rval;

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

  fd = ::open (fn, O_RDONLY);
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

  int rval = zcatfd (fd, 1, fn);

  delete[] fn;
  close (fd);

  return rval;
}

int
Shell::evalstring (char *string, const char *from_file, parse_flags flags)
{
  /* Are we running a trap when we execute this function? */
  int was_trap = running_trap;

  /* If we are in a place where `return' is valid, we have to catch
     `eval "... return"' and make sure parse_and_execute cleans up. Then
     we can trampoline to the previous saved return_catch location. */
  if (return_catch_flag)
    {
      int saved_return_catch_flag = return_catch_flag;
      return_catch_flag++; /* increment so we have a counter */

      try
        {
          /* Note that parse_and_execute () frees the string it is passed. */
          int r = parse_and_execute (string, from_file, flags);
          return_catch_flag = saved_return_catch_flag;
          return r;
        }
      catch (const bash_exception &)
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
          return_catch_flag = saved_return_catch_flag;
          throw;
        }
    }
  else
    return parse_and_execute (string, from_file, flags);
}

} // namespace bash
