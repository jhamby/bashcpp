/* eval.c -- reading and evaluating commands. */

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

#include <csignal>

#include "bashintl.hh"

#include "flags.hh"
#include "parser.hh"
#include "shell.hh"
#include "trap.hh"

#include "builtins/common.hh"

#include "input.hh"

namespace bash
{

static void alrm_catcher (int);

/* Read and execute commands until EOF is reached.  This assumes that
   the input source has already been initialized. */
int
Shell::reader_loop ()
{
  int our_indirection_level;

  COMMAND *current_command = nullptr;

  our_indirection_level = ++indirection_level;

  if (just_one_command)
    reset_readahead_token ();

  while (!EOF_Reached)
    {
#if defined(PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

      /* XXX - why do we set this every time through the loop?  And why do
         it if SIGINT is trapped in an interactive shell? */
      if (interactive_shell && signal_is_ignored (SIGINT) == 0
          && signal_is_trapped (SIGINT) == 0)
        set_signal_handler (SIGINT, &sigint_sighandler_global);

      try
        {
          executing = 0;
          if (temporary_env)
            dispose_used_env_vars ();

          if (read_command () == 0)
            {
              if (interactive_shell == 0 && read_but_dont_execute)
                {
                  set_exit_status (EXECUTION_SUCCESS);
                  delete global_command;
                  global_command = nullptr;
                }
              else if ((current_command = global_command))
                {
                  global_command = nullptr;

                  /* If the shell is interactive, expand and display $PS0 after
                     reading a command (possibly a list or pipeline) and before
                     executing it. */
                  if (interactive && ps0_prompt)
                    {
                      char *ps0_string;

                      ps0_string = decode_prompt_string (ps0_prompt);
                      if (ps0_string && *ps0_string)
                        {
                          fprintf (stderr, "%s", ps0_string);
                          fflush (stderr);
                        }
                      free (ps0_string);
                    }

                  current_command_number++;

                  executing = true;
                  stdin_redir = false;

                  execute_command (current_command);
                  goto exec_done;
                }
            }
          else
            {
              /* Parse error, maybe discard rest of stream if not interactive.
               */
              if (!interactive)
                EOF_Reached = true;
            }
          if (just_one_command)
            EOF_Reached = true;

          return last_command_exit_value;
        }
      catch (const bash_exception &e)
        {
          indirection_level = our_indirection_level;

          switch (e.type)
            {
              /* Some kind of throw to top_level has occurred. */
            case FORCE_EOF:
            case ERREXIT:
            case EXITPROG:
              current_command = nullptr;
              if (exit_immediately_on_error)
                variable_context = 0; /* not in a function */
              EOF_Reached = true;
              goto exec_done;

            case DISCARD:
              /* Make sure the exit status is reset to a non-zero value, but
                 leave existing non-zero values (e.g., > 128 on signal)
                 alone. */
              if (last_command_exit_value == 0)
                set_exit_status (EXECUTION_FAILURE);
              if (subshell_environment)
                {
                  current_command = nullptr;
                  EOF_Reached = true;
                  goto exec_done;
                }
              /* Obstack free command elements, etc. */
              if (current_command)
                {
                  delete current_command;
                  current_command = nullptr;
                }

              restore_sigmask ();
              break;

            case NOEXCEPTION:
            default:
              command_error ("reader_loop", CMDERR_BADJUMP, e.type);
            }
          return last_command_exit_value;
        }
    }
exec_done:
  QUIT;

  if (current_command)
    {
      delete current_command;
      current_command = nullptr;
    }

  indirection_level--;
  return last_command_exit_value;
}

/* Pretty print shell scripts */
int
Shell::pretty_print_loop ()
{
  COMMAND *current_command;
  const char *command_to_print;
  bool global_posix_mode, last_was_newline;

  global_posix_mode = posixly_correct;
  last_was_newline = false;
  try
    {
      while (!EOF_Reached)
        {
          if (read_command () == 0)
            {
              current_command = global_command;
              global_command = nullptr;
              posixly_correct = true; /* print posix-conformant */
              if (current_command
                  && (command_to_print
                      = make_command_string (current_command)))
                {
                  printf ("%s\n", command_to_print); /* for now */
                  last_was_newline = 0;
                }
              else if (!last_was_newline)
                {
                  printf ("\n");
                  last_was_newline = true;
                }
              posixly_correct = global_posix_mode;
              delete current_command;
            }
          else
            return EXECUTION_FAILURE;
        }
    }
  catch (const bash_exception &)
    {
      return EXECUTION_FAILURE;
    }

  return EXECUTION_SUCCESS;
}

static void
alrm_catcher (int)
{
  const char *msg;

  msg = _ ("\007timed out waiting for input: auto-logout\n");
  (void)::write (1, msg, std::strlen (msg));

  the_shell->bash_logout (); /* run ~/.bash_logout if this is a login shell */
  terminating_signal = SIGKILL; // XXX - throw EXITPROG as soon as possible
}

/* Send an escape sequence to emacs term mode to tell it the
   current working directory. */
void
Shell::send_pwd_to_eterm ()
{
  char *f = nullptr;
  char *pwd = get_string_value ("PWD");
  if (pwd == nullptr)
    f = pwd = get_working_directory ("eterm");
  std::fprintf (stderr, "\032/%s\n", pwd);
  delete[] f;
}

#if defined(ARRAY_VARS)
/* Caller ensures that A has a non-zero number of elements */
void
Shell::execute_array_command (ARRAY *a, const char *tag)
{
  int argc = 0;
  char **argv = array_to_argv (a, &argc);
  for (int i = 0; i < argc; i++)
    {
      if (argv[i] && argv[i][0])
        execute_variable_command (argv[i], tag);
    }
  strvec_dispose (argv);
}
#endif

void
Shell::execute_prompt_command ()
{
  char *command_to_execute;
  SHELL_VAR *pcv;
#if defined(ARRAY_VARS)
  ARRAY *pcmds;
#endif

  pcv = find_variable ("PROMPT_COMMAND");
  if (pcv == nullptr || !pcv->is_set () || pcv->invisible ())
    return;
#if defined(ARRAY_VARS)
  if (pcv->array ())
    {
      if ((pcmds = pcv->array_value ()) && array_num_elements (pcmds) > 0)
        execute_array_command (pcmds, "PROMPT_COMMAND");
      return;
    }
  else if (pcv->assoc ())
    return; /* currently don't allow associative arrays here */
#endif

  command_to_execute = pcv->str_value ();
  if (command_to_execute && *command_to_execute)
    execute_variable_command (command_to_execute, "PROMPT_COMMAND");
}

/* Call the Bison-generated parser and return the status of the parse.
   Input is read from the current input stream (bash_input).  yyparse
   leaves the parsed command in the global variable GLOBAL_COMMAND.
   This is where PROMPT_COMMAND is executed. */
int
Shell::parse_command ()
{
  need_here_doc = 0;
  run_pending_traps ();

  /* Allow the execution of a random command just before the printing
     of each primary prompt.  If the shell variable PROMPT_COMMAND
     is set then its value (array or string) is the command(s) to execute. */
  /* The tests are a combination of SHOULD_PROMPT() and prompt_again()
     from parse.y, which are the conditions under which the prompt is
     actually printed. */
  if (interactive && bash_input.type != st_string
      && parser_expanding_alias () == 0)
    {
#if defined(READLINE)
      if (no_line_editing
          || (bash_input.type == st_stdin && parser_will_prompt ()))
#endif
        execute_prompt_command ();

      if (running_under_emacs == 2)
        send_pwd_to_eterm (); /* Yuck */
    }

  current_command_line_count = 0;
  int r = yyparse ();

  if (need_here_doc)
    gather_here_documents ();

  return r;
}

/* Read and parse a command, returning the status of the parse.  The command
   is left in the globval variable GLOBAL_COMMAND for use by reader_loop.
   This is where the shell timeout code is executed. */
int
Shell::read_command ()
{
  SHELL_VAR *tmout_var;
  int tmout_len, result;
  SigHandler old_alrm;

  set_current_prompt_level (1);
  global_command = nullptr;

  /* Only do timeouts if interactive. */
  tmout_var = nullptr;
  tmout_len = 0;
  old_alrm = nullptr;

  if (interactive)
    {
      tmout_var = find_variable ("TMOUT");

      if (tmout_var && tmout_var->is_set ())
        {
          tmout_len = ::atoi (tmout_var->str_value ());
          if (tmout_len > 0)
            {
              old_alrm = set_signal_handler (SIGALRM, &alrm_catcher);
              ::alarm (static_cast<unsigned int> (tmout_len));
            }
        }
    }

  QUIT;

  current_command_line_count = 0;
  result = parse_command ();

  if (interactive && tmout_var && (tmout_len > 0))
    {
      ::alarm (0);
      set_signal_handler (SIGALRM, old_alrm);
    }

  return result;
}

} // namespace bash
