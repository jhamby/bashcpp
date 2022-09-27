/* evalstring.c - evaluate a string as one or more shell commands. */

/* Copyright (C) 1996-2020 Free Software Foundation, Inc.

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

#include <cerrno>
#include <csignal>
#include <cstdio>

#include "builtins/common.hh"
#include "filecntl.hh"
#include "parse.hh"
#include "shell.hh"

#include "builtext.hh"

namespace bash
{

#if 0
#define IS_BUILTIN(s)                                                         \
  (builtin_address_internal (s, 0) != (struct builtin *)NULL)

int parse_and_execute_level = 0;
#endif

#define PE_TAG "parse_and_execute top"
#define PS_TAG "parse_string top"

bool
Shell::should_suppress_fork (COMMAND *command)
{
#if 0 /* TAG: bash-5.2 */
  bool subshell;

  subshell = subshell_environment & SUBSHELL_PROCSUB;	/* salt to taste */
#endif
  SIMPLE_COM *scm = dynamic_cast<SIMPLE_COM *> (command);
  return (scm && startup_state == 2 && parse_and_execute_level == 1
          && running_trap == 0 && bash_input.location.string->empty ()
          && !parser_expanding_alias () && !signal_is_trapped (EXIT_TRAP)
          && !signal_is_trapped (ERROR_TRAP) && any_signals_trapped () < 0 &&
#if 0 /* TAG: bash-5.2 */
	  (subshell || (command->redirects == 0 && scm->simple_redirects == 0)) &&
#else
          command->redirects == 0 && scm->simple_redirects == 0 &&
#endif
          ((command->flags & CMD_TIME_PIPELINE) == 0)
          && ((command->flags & CMD_INVERT_RETURN) == 0));
}

bool
Shell::can_optimize_connection (CONNECTION *connection)
{
  return (bash_input.location.string->empty () && !parser_expanding_alias ()
          && (connection->connector == parser::token::AND_AND
              || connection->connector == parser::token::OR_OR
              || connection->connector == ';')
          && typeid (*(connection->second)) == typeid (CONNECTION));
}

void
Shell::optimize_fork (COMMAND *command)
{
  CONNECTION *connection = dynamic_cast<CONNECTION *> (command);
  if (connection
      && (connection->connector == parser::token::AND_AND
          || connection->connector == parser::token::OR_OR
          || connection->connector == ';')
      && (connection->second->flags & CMD_TRY_OPTIMIZING)
      && should_suppress_fork (connection->second))
    {
      connection->second->flags |= CMD_NO_FORK;
      SIMPLE_COM *second_com = dynamic_cast<SIMPLE_COM *> (connection->second);
      if (second_com)
        second_com->flags |= CMD_NO_FORK;
    }
}

void
Shell::optimize_subshell_command (COMMAND *command)
{
  SIMPLE_COM *scm = dynamic_cast<SIMPLE_COM *> (command);
  if (scm && running_trap == 0 && signal_is_trapped (EXIT_TRAP) == 0
      && signal_is_trapped (ERROR_TRAP) == 0 && any_signals_trapped () < 0
      && command->redirects == 0 && scm->simple_redirects == 0
      && ((command->flags & CMD_TIME_PIPELINE) == 0)
      && ((command->flags & CMD_INVERT_RETURN) == 0))
    {
      command->flags |= CMD_NO_FORK;
    }
  else
    {
      CONNECTION *connection = dynamic_cast<CONNECTION *> (command);
      if (connection
          && (connection->connector == parser::token::AND_AND
              || connection->connector == parser::token::OR_OR))
        optimize_subshell_command (connection->second);
    }
}

void
Shell::optimize_shell_function (COMMAND *command)
{
  GROUP_COM *gc = dynamic_cast<GROUP_COM *> (command);
  COMMAND *fc = gc ? gc->command : command;

  if (should_suppress_fork (fc))
    fc->flags |= CMD_NO_FORK;
  else
    {
      CONNECTION *connection = dynamic_cast<CONNECTION *> (fc);
      if (connection && can_optimize_connection (connection)
          && should_suppress_fork (connection->second))
        {
          connection->second->flags |= CMD_NO_FORK;
        }
    }
}

#if 0
static void
parse_prologue (const std::string &string, parse_flags flags,
                const std::string &tag)
{
  char *orig_string, *lastcom;
  int x;

  orig_string = string;
  /* Unwind protect this invocation of parse_and_execute (). */
  begin_unwind_frame (tag);
  unwind_protect_var (parse_and_execute_level);
  unwind_protect_var (top_level);
  unwind_protect_var (indirection_level);
  unwind_protect_var (line_number);
  unwind_protect_var (line_number_for_err_trap);
  unwind_protect_var (loop_level);
  unwind_protect_var (executing_list);
  unwind_protect_var (comsub_ignore_return);
  if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
    unwind_protect_var (interactive);

#if defined(HISTORY)
  if (parse_and_execute_level == 0)
    add_unwind_protect (set_history_remembering);
  else
    unwind_protect_var (remember_on_history); /* can be used in scripts */
#if defined(BANG_HISTORY)
  unwind_protect_var (history_expansion_inhibited);
#endif /* BANG_HISTORY */
#endif /* HISTORY */

  if (interactive_shell)
    {
      x = get_current_prompt_level ();
      add_unwind_protect_int (set_current_prompt_level, x);
    }

  if (the_printed_command_except_trap)
    {
      lastcom = savestring (the_printed_command_except_trap);
      add_unwind_protect_ptr (restore_lastcom, lastcom);
    }

  add_unwind_protect (pop_stream);
  if (parser_expanding_alias ())
    add_unwind_protect (parser_restore_alias);

  if (orig_string && ((flags & SEVAL_NOFREE) == 0))
    add_unwind_protect_ptr (xfree, orig_string);
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
/* How to force parse_and_execute () to clean up after itself. */
// TODO: move this into catch block
void
parse_and_execute_cleanup (int old_running_trap)
{
  if (running_trap > 0)
    {
      /* We assume if we have a different value for running_trap than when
         we started (the only caller that cares is evalstring ()), the
         original caller will perform the cleanup, and we should not step
         on them. */
      if (running_trap != old_running_trap)
        run_trap_cleanup (running_trap - 1);
      unfreeze_jobs_list ();
    }

  if (have_unwind_protects ())
    run_unwind_frame (PE_TAG);
  else
    parse_and_execute_level = 0; /* XXX */
}
#endif

// Parse and execute the commands in STRING. Returns whatever
// execute_command () returns. Actions are:
//
//        (flags & SEVAL_NONINT) -> interactive = 0;
//        (flags & SEVAL_INTERACT) -> interactive = 1;
//        (flags & SEVAL_NOHIST) -> call bash_history_disable ()
//        (flags & SEVAL_NOFREE) -> don't free STRING when finished
//        (flags & SEVAL_RESETLINE) -> reset line_number to 1
//        (flags & SEVAL_NOHISTEXP) -> history_expansion_inhibited -> 1
int
Shell::parse_and_execute (const std::string &string,
                          const std::string &from_file, parse_flags flags)
{
  //   int code, lreset;
  //   int should_jump_to_top_level, last_result;
  COMMAND *command;

  int saved_parse_and_exec_level = parse_and_execute_level;
  int saved_indirection_level = indirection_level;
  int saved_line_number = line_number;
  int saved_line_number_for_err_trap = line_number_for_err_trap;
  int saved_loop_level = loop_level;
  int saved_executing_list = executing_list;
  int saved_comsub_ignore_return = comsub_ignore_return;

  bool saved_interactive;
  if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
    saved_interactive = interactive;

#if defined(HISTORY)
  char saved_enable_history_list = enable_history_list;
#if defined(BANG_HISTORY)
  bool saved_history_expansion_inhibited = history_expansion_inhibited;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

  int saved_current_prompt_level;
  if (interactive_shell)
    saved_current_prompt_level = get_current_prompt_level ();

#if 0
  if (the_printed_command_except_trap)
    {
      lastcom = savestring (the_printed_command_except_trap);
      add_unwind_protect_ptr (restore_lastcom, lastcom);
    }

  add_unwind_protect (pop_stream);
  if (parser_expanding_alias ())
    add_unwind_protect (parser_restore_alias);

  parse_prologue (string, flags, PE_TAG);
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
#endif

  parse_and_execute_level++;

  bool lreset = (flags & SEVAL_RESETLINE);

#if defined(HAVE_POSIX_SIGNALS)
  sigset_t pe_sigmask;
  /* If we throw and are going to go on, use this to restore signal mask */
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

  if (!lreset)
    line_number--;

  indirection_level++;

  bool should_jump_to_top_level = false;
  int last_result = EXECUTION_SUCCESS;

  /* We need to reset enough of the token state so we can start fresh. */
  if (current_token == parser::token::yacc_EOF)
    current_token = '\n'; /* reset_parser() ? */

  with_input_from_string (string, from_file);
  clear_shell_input_line ();

  while (!bash_input.location.string->empty () || parser_expanding_alias ())
    {
      command = nullptr;

      if (interrupt_state)
        {
          last_result = EXECUTION_FAILURE;
          break;
        }

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
              else if ((command = global_command))
                {
                  struct fd_bitmap *bitmap;

                  if (flags & SEVAL_FUNCDEF)
                    {
                      const char *x;

                      /* If the command parses to something other than a
                         straight function definition, or if we have not
                         consumed the entire string, or if the parser has
                         transformed the function name (as parsing will if it
                         begins or ends with shell whitespace, for example),
                         reject the attempt */
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
                  add_unwind_protect_ptr (dispose_fd_bitmap, bitmap);
                  add_unwind_protect_ptr (dispose_command, command); /* XXX */

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
                    command->flags |= CMD_NO_FORK;

                  /* Can't optimize forks out here except for simple commands.
                     This knows that the parser sets up commands as left-side
                     heavy (&& and || are left-associative) and after the
                     single parse, if we are at the end of the command string,
                     the last in a series of connection commands is
                     connection->second. */
                  else
                    {
                      CONNECTION *connection
                          = dynamic_cast<CONNECTION *> (command);
                    }

                  if (command->type == cm_connection
                      && can_optimize_connection (command))
                    {
                      connection->second->flags |= CMD_TRY_OPTIMIZING;
                    }
#endif /* ONESHOT */

                  /* See if this is a candidate for $( <file ). */
                  if (startup_state == 2
                      && (subshell_environment & SUBSHELL_COMSUB)
                      && *bash_input.location.string == '\0'
                      && command->type == cm_simple && !command->redirects
                      && (command->flags & CMD_TIME_PIPELINE) == 0
                      && command->value.Simple->words == 0
                      && command->value.Simple->redirects
                      && command->value.Simple->redirects->next == 0
                      && command->value.Simple->redirects->instruction
                             == r_input_direction
                      && command->value.Simple->redirects->redirector.dest
                             == 0)
                    {
                      int r;
                      r = cat_file (command->value.Simple->redirects);
                      last_result
                          = (r < 0) ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
                    }
                  else
                    last_result = execute_command_internal (
                        command, 0, NO_PIPE, NO_PIPE, bitmap);
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

              if (!interactive_shell
                  && (this_shell_builtin == &Shell::source_builtin
                      || this_shell_builtin == &Shell::eval_builtin)
                  && last_command_exit_value == EX_BADSYNTAX && posixly_correct
                  && !executing_command_builtin)
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
      catch (const bash_exception &e)
        {
          if (e.type)
            {
              should_jump_to_top_level = 0;
              switch (e.type)
                {
                case ERREXIT:
                  /* variable_context -> 0 is what eval.c:reader_loop() does in
                     these circumstances.  Don't bother with cleanup here
                     because we don't want to run the function execution
                     cleanup stuff that will cause pop_context and other
                     functions to run.
                     XXX - change that if we want the function context to be
                     unwound. */
                  if (exit_immediately_on_error && variable_context)
                    {
                      discard_unwind_frame ("pe_dispose");
                      variable_context = 0; /* not in a function */
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

        out:

          run_unwind_frame (PE_TAG);
#endif
        }
      catch (std::exception &)
        {
          throw;
        }
    }

  if (interrupt_state && parse_and_execute_level == 0)
    {
      /* An interrupt during non-interactive execution in an
         interactive shell (e.g. via $PROMPT_COMMAND) should
         not cause the shell to exit. */
      interactive = interactive_shell;
      throw_to_top_level ();
    }

  if (should_jump_to_top_level)
    jump_to_top_level (code);

  return last_result;
}

/* Parse a command contained in STRING according to FLAGS and return the
   number of characters consumed from the string.  If non-NULL, set *ENDP
   to the position in the string where the parse ended.  Used to validate
   command substitutions during parsing to obey Posix rules about finding
   the end of the command and balancing parens. */
int
Shell::parse_string (const std::string &string, const std::string &from_file,
                     parse_flags flags, char **endp)
{
  int code, nc;
  volatile int should_jump_to_top_level;
  COMMAND *command, *oglobal;
  char *ostring;
  volatile sigset_t ps_sigmask;

  parse_prologue (string, flags, PS_TAG);

#if defined(HAVE_POSIX_SIGNALS)
  /* If we longjmp and are going to go on, use this to restore signal
   * mask */
  sigemptyset ((sigset_t *)&ps_sigmask);
  sigprocmask (SIG_BLOCK, nullptr, (sigset_t *)&ps_sigmask);
#endif

  /*itrace("parse_string: `%s'", string);*/
  /* Reset the line number if the caller wants us to.  If we don't reset
     the line number, we have to subtract one, because we will add one
     just before executing the next command (resetting the line number
     sets it to 0; the first line number is 1). */
  push_stream (0);
  if (parser_expanding_alias ())
    /* push current shell_input_line */
    parser_save_alias ();

  code = should_jump_to_top_level = 0;
  oglobal = global_command;
  ostring = string;

  with_input_from_string (string, from_file);
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
#if defined(DEBUG)
          itrace ("parse_string: longjmp executed: code = %d", code);
#endif
          should_jump_to_top_level = 0;
          switch (code)
            {
            case FORCE_EOF:
            case ERREXIT:
            case EXITPROG:
            case DISCARD: /* XXX */
              if (command)
                dispose_command (command);
              /* Remember to call longjmp (top_level) after the old
                 value for it is restored. */
              should_jump_to_top_level = 1;
              goto out;

            default:
#if defined(HAVE_POSIX_SIGNALS)
              sigprocmask (SIG_SETMASK, (sigset_t *)&ps_sigmask, nullptr);
#endif
              command_error ("parse_string", CMDERR_BADJUMP, code, 0);
              break;
            }
        }

      if (parse_command () == 0)
        {
          dispose_command (global_command);
          global_command = nullptr;
        }
      else
        {
          if ((flags & SEVAL_NOLONGJMP) == 0)
            {
              should_jump_to_top_level = 1;
              code = DISCARD;
            }
          else
            reset_parser (); /* XXX - sets token_to_read */
          break;
        }

      if (current_token == yacc_EOF || current_token == shell_eof_token)
        break;
    }

out:

  global_command = oglobal;
  nc = bash_input.location.string - ostring;
  if (endp)
    *endp = bash_input.location.string;

  run_unwind_frame (PS_TAG);

  /* If we return < 0, the caller (xparse_dolparen) will
     jump_to_top_level for us, after doing cleanup */
  if (should_jump_to_top_level)
    {
      if (parse_and_execute_level == 0)
        top_level_cleanup ();
      if (code == DISCARD)
        return -DISCARD;
      jump_to_top_level (code);
    }

  return nc;
}

/* Handle a $( < file ) command substitution.  This expands the filename,
   returning errors as appropriate, then just cats the file to the
   standard output. */
int
Shell::cat_file (REDIRECT *r)
{
  //   char *fn;
  int fd, rval;

  if (r->instruction != r_input_direction)
    return -1;

  /* Get the filename. */

  bool old_disallow_globbing = disallow_filename_globbing;

  if (posixly_correct && !interactive_shell)
    disallow_filename_globbing = true;

  fn = redirection_expand (r->redirectee.filename);

  if (posixly_correct && !interactive_shell)
    disallow_filename_globbing = old_disallow_globbing;

  if (fn == 0)
    {
      redirection_error (r, AMBIGUOUS_REDIRECT, fn);
      return -1;
    }

  fd = open (fn, O_RDONLY);
  if (fd < 0)
    {
      file_error (fn);
      free (fn);
      return -1;
    }

  rval = zcatfd (fd, 1, fn);

  free (fn);
  close (fd);

  return rval;
}

int
Shell::evalstring (const std::string &string, const std::string &from_file,
                   parse_flags flags)
{
  //   int r, rflag, rcatch;
  //   int was_trap;

  /* Are we running a trap when we execute this function? */
  int was_trap = running_trap;

  rcatch = 0;
  rflag = return_catch_flag;

  /* If we are in a place where `return' is valid, we have to catch
     `eval "... return"' and make sure parse_and_execute cleans up. Then
     we can trampoline to the previous saved return_catch location. */
  if (rflag)
    {
      begin_unwind_frame ("evalstring");

      unwind_protect_var (return_catch_flag);
      unwind_protect_var (return_catch);

      return_catch_flag++; /* increment so we have a counter */
      rcatch = setjmp_nosigs (return_catch);
    }

  if (rcatch)
    {
      /* We care about whether or not we are running the same trap we
         were when we entered this function. */
      parse_and_execute_cleanup (was_trap);
      r = return_catch_value;
    }
  else
    /* Note that parse_and_execute () frees the string it is passed. */
    r = parse_and_execute (string, from_file, flags);

  if (rflag)
    {
      run_unwind_frame ("evalstring");
      if (rcatch && return_catch_flag)
        {
          return_catch_value = r;
          sh_longjmp (return_catch, 1);
        }
    }

  return r;
}

} // namespace bash
