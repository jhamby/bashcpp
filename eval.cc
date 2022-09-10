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

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <csignal>

#include "bashintl.hh"

#include "shell.hh"
#include "parser.hh"
#include "flags.hh"
#include "trap.hh"

#include "builtins/common.hh"

#include "input.hh"
#include "execute_cmd.hh"

namespace bash
{

static void send_pwd_to_eterm ();
static SigHandler alrm_catcher (int);

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
#if defined (PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */

      /* XXX - why do we set this every time through the loop?  And why do
	 it if SIGINT is trapped in an interactive shell? */
      if (interactive_shell && signal_is_ignored (SIGINT) == 0 && signal_is_trapped (SIGINT) == 0)
	set_signal_handler (SIGINT, sigint_sighandler);

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
		  dispose_command (global_command);
		  global_command = nullptr;
		}
	      else if ((current_command = global_command))
		{
		  global_command = nullptr;

		  /* If the shell is interactive, expand and display $PS0 after reading a
		     command (possibly a list or pipeline) and before executing it. */
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

	    exec_done:
	      QUIT;

	      if (current_command)
		{
		  dispose_command (current_command);
		  current_command = nullptr;
		}
	       }
	    }
	  else
	    {
	      /* Parse error, maybe discard rest of stream if not interactive. */
	      if (!interactive)
		EOF_Reached = true;
	    }
	  if (just_one_command)
	    EOF_Reached = true;
	}
      catch (const bash_exception& e)
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
		variable_context = 0;	/* not in a function */
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
		  dispose_command (current_command);
		  current_command = nullptr;
		}

	      restore_sigmask ();
	      break;

	    default:
	      command_error ("reader_loop", CMDERR_BADJUMP, code, 0);
	    }
	}
    }
  indirection_level--;
  return last_command_exit_value;
}

/* Pretty print shell scripts */
int
Shell::pretty_print_loop ()
{
  COMMAND *current_command;
  char *command_to_print;
  bool global_posix_mode, last_was_newline;

  global_posix_mode = posixly_correct;
  last_was_newline = false;
  try
  {
    while (!EOF_Reached)
    {
      if (read_command() == 0)
	{
	  current_command = global_command;
	  global_command = nullptr;
	  posixly_correct = true;			/* print posix-conformant */
	  if (current_command && (command_to_print = make_command_string (current_command)))
	    {
	      printf ("%s\n", command_to_print);	/* for now */
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
  catch (const bash_exception &e)
    {
      return EXECUTION_FAILURE;
    }

  return EXECUTION_SUCCESS;
}

static SigHandler
alrm_catcher(int i)
{
  const char *msg;

  msg = _("\007timed out waiting for input: auto-logout\n");
  ssize_t ignored = ::write (1, msg, std::strlen (msg));

  bash_logout ();	/* run ~/.bash_logout if this is a login shell */
  throw bash_exception (EXITPROG);
}

/* Send an escape sequence to emacs term mode to tell it the
   current working directory. */
void
Shell::send_pwd_to_eterm ()
{
  char *pwd, *f;

  f = 0;
  pwd = get_string_value ("PWD");
  if (pwd == 0)
    f = pwd = get_working_directory ("eterm");
  std::fprintf (stderr, "\032/%s\n", pwd);
  free (f);
}

#if defined (ARRAY_VARS)
/* Caller ensures that A has a non-zero number of elements */
int
execute_array_command (ARRAY *a, const void *v)
{
  const char *tag;
  char **argv;
  int argc, i;

  tag = (const char *)v;
  argc = 0;
  argv = array_to_argv (a, &argc);
  for (i = 0; i < argc; i++)
    {
      if (argv[i] && argv[i][0])
	execute_variable_command (argv[i], tag);
    }
  strvec_dispose (argv);
  return 0;
}
#endif

void
Shell::execute_prompt_command ()
{
  char *command_to_execute;
  SHELL_VAR *pcv;
#if defined (ARRAY_VARS)
  ARRAY *pcmds;
#endif

  pcv = find_variable ("PROMPT_COMMAND");
  if (pcv  == 0 || var_isset (pcv) == 0 || invisible_p (pcv))
    return;
#if defined (ARRAY_VARS)
  if (array_p (pcv))
    {
      if ((pcmds = array_cell (pcv)) && array_num_elements (pcmds) > 0)
	execute_array_command (pcmds, "PROMPT_COMMAND");
      return;
    }
  else if (assoc_p (pcv))
    return;	/* currently don't allow associative arrays here */
#endif

  command_to_execute = value_cell (pcv);
  if (command_to_execute && *command_to_execute)
    execute_variable_command (command_to_execute, "PROMPT_COMMAND");
}

/* Call the YACC-generated parser and return the status of the parse.
   Input is read from the current input stream (bash_input).  yyparse
   leaves the parsed command in the global variable GLOBAL_COMMAND.
   This is where PROMPT_COMMAND is executed. */
int
Shell::parse_command ()
{
  int r;

  need_here_doc = 0;
  run_pending_traps ();

  /* Allow the execution of a random command just before the printing
     of each primary prompt.  If the shell variable PROMPT_COMMAND
     is set then its value (array or string) is the command(s) to execute. */
  /* The tests are a combination of SHOULD_PROMPT() and prompt_again()
     from parse.y, which are the conditions under which the prompt is
     actually printed. */
  if (interactive && bash_input.type != st_string && parser_expanding_alias() == 0)
    {
#if defined (READLINE)
      if (no_line_editing || (bash_input.type == st_stdin && parser_will_prompt ()))
#endif
        execute_prompt_command ();

      if (running_under_emacs == 2)
	send_pwd_to_eterm ();	/* Yuck */
    }

  current_command_line_count = 0;
  r = yyparse ();

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
  SigHandler *old_alrm;

  set_current_prompt_level (1);
  global_command = nullptr;

  /* Only do timeouts if interactive. */
  tmout_var = nullptr;
  tmout_len = 0;
  old_alrm = nullptr;

  if (interactive)
    {
      tmout_var = find_variable ("TMOUT");

      if (tmout_var && var_isset (tmout_var))
	{
	  tmout_len = atoi (value_cell (tmout_var));
	  if (tmout_len > 0)
	    {
	      old_alrm = set_signal_handler (SIGALRM, alrm_catcher);
	      alarm (tmout_len);
	    }
	}
    }

  QUIT;

  current_command_line_count = 0;
  result = parse_command ();

  if (interactive && tmout_var && (tmout_len > 0))
    {
      alarm(0);
      set_signal_handler (SIGALRM, old_alrm);
    }

  return result;
}

}  // namespace bash
