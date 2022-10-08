/* shell.c -- GNU's idea of the POSIX shell specification. */

/* Copyright (C) 1987-2019 Free Software Foundation, Inc.

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

/*
  Birthdate:
  Sunday, January 10th, 1988.
  Initial author: Brian Fox
*/

#include "config.hh"

#include "bashtypes.hh"

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include "posixstat.hh"
#include "posixtime.hh"

#include "filecntl.hh"

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "builtins.hh"
#include "builtins/common.hh"
#include "flags.hh"
#include "parser.hh"
#include "shell.hh"
#include "trap.hh"

#include "jobs.hh"

#include "findcmd.hh"
#include "input.hh"

#include "strmatch.hh"

#if !defined(HAVE_GETPW_DECLS)
extern struct passwd *getpwuid ();
#endif /* !HAVE_GETPW_DECLS */

#if defined(NO_MAIN_ENV_ARG)
/* systems without third argument to main() */
int
main (int argc, char **argv)
{
  try
    {
      bash::the_shell = new bash::Shell ();
      bash::the_shell->start (argc, argv, bash::environ);
    }
  catch (const std::exception &e)
    {
      report_error (_ ("unhandled exception: %s"), e.what ());
      std::exit (2);
    }
}
#else  /* !NO_MAIN_ENV_ARG */
int
main (int argc, char **argv, char **env)
{
  try
    {
      bash::the_shell = new bash::Shell ();
      bash::the_shell->start_shell (argc, argv, env);
    }
  catch (const std::exception &e)
    {
      std::fprintf (stderr, _ ("unhandled exception: %s"), e.what ());
      std::exit (2);
    }
}
#endif /* !NO_MAIN_ENV_ARG */

// Start of the bash namespace
namespace bash
{

void
Shell::init_long_args ()
{
  long_args.push_back (LongArg ("debug", &debugging));
#if defined(DEBUGGER)
  long_args.push_back (LongArg ("debugger", &debugging_mode));
#endif
  long_args.push_back (LongArg ("dump-po-strings", &dump_po_strings));
  long_args.push_back (LongArg ("dump-strings", &dump_translatable_strings));
  long_args.push_back (LongArg ("help", &want_initial_help));
  long_args.push_back (LongArg ("init-file", &bashrc_file));
  long_args.push_back (LongArg ("login", &make_login_shell));
  long_args.push_back (LongArg ("noediting", &no_line_editing));
  long_args.push_back (LongArg ("noprofile", &no_profile));
  long_args.push_back (LongArg ("norc", &no_rc));
  long_args.push_back (LongArg ("posix", &posixly_correct));
  long_args.push_back (LongArg ("pretty-print", &pretty_print_mode));
#if defined(WORDEXP_OPTION)
  long_args.push_back (LongArg ("protected", &protected_mode));
#endif
  long_args.push_back (LongArg ("rcfile", &bashrc_file));
#if defined(RESTRICTED_SHELL)
  long_args.push_back (LongArg ("restricted", &restricted));
#endif
  long_args.push_back (LongArg ("verbose", &verbose_flag));
  long_args.push_back (LongArg ("version", &do_version));
#if defined(WORDEXP_OPTION)
  long_args.push_back (LongArg ("wordexp", &wordexp_only));
#endif
}

#ifdef __CYGWIN__
void
Shell::_cygwin32_check_tmp ()
{
  struct stat sb;

  if (stat ("/tmp", &sb) < 0)
    internal_warning (_ ("could not find /tmp, please create!"));
  else
    {
      if (S_ISDIR (sb.st_mode) == 0)
        internal_warning (_ ("/tmp must be a valid directory name"));
    }
}
#endif /* __CYGWIN__ */

// Initialize the simple shell variables to default values.
SimpleState::SimpleState ()
    : shell_tty (-1), shell_pgrp (NO_PID), terminal_pgrp (NO_PID),
      original_pgrp (NO_PID), last_made_pid (NO_PID),
      last_asynchronous_pid (NO_PID), current_command_number (1),
#if defined(BUFFERED_INPUT)
      default_buffered_input (-1),
#endif
      rseed (1), rseed32 (1073741823), last_command_subst_pid (NO_PID),
      current_command_subst_pid (NO_PID), eof_encountered_limit (10),
      word_top (-1), indentation_amount (4), xtrace_fd (-1), xattrfd (-1),
      array_needs_making (true),
#if defined(JOB_CONTROL)
      job_control (true),
#endif
      check_window_size (CHECKWINSIZE_DEFAULT), hashing_enabled (1),
#if defined(BANG_HISTORY)
      history_expansion (HISTEXPAND_DEFAULT),
#endif
      interactive_comments (1),
#if defined(RESTRICTED_SHELL)
      save_restricted (-1),
#endif
#if defined(BRACE_EXPANSION)
      brace_expansion (1),
#endif
      extended_quote (1), promptvars (1),
#ifdef HAVE_DEV_FD
      have_devfd (HAVE_DEV_FD),
#endif
#if !defined(READLINE)
      no_line_editing (1), /* can't have line editing without readline */
#endif
#if defined(STRICT_POSIX)
      posixly_correct (1)
#else
      posixly_correct (0)
#endif
{
  // XXX - remove this if Linux doesn't need this path
#if defined(PGRP_PIPE)
  // initialize the array here for strict C++03 compatibility
  pgrp_pipe[0] = -1;
  pgrp_pipe[1] = -1;
#endif

  // manual init for C++03 compatibility
  getopt_errstr[0] = '-';
  getopt_errstr[1] = '\0';
  getopt_errstr[2] = '\0';
}

Shell::Shell ()
    : SimpleState (), bashrc_file (DEFAULT_BASHRC), primary_prompt (PPROMPT),
      secondary_prompt (SPROMPT), source (0), redir (0), old_winch (SIG_DFL)
{
  // alloc this 4K read buffer from the heap to keep the class size small
  zread_lbuf = new char[ZBUFSIZ];

  init_long_args ();

  posix_vars[0] = &interactive_comments;
  posix_vars[1] = &source_uses_path;
  posix_vars[2] = &expand_aliases;
  posix_vars[3] = &inherit_errexit;
  posix_vars[4] = &print_shift_error;

  init_token_lists ();
}

// Virtual destructor for Shell.
Shell::~Shell () noexcept { delete[] zread_lbuf; }

// Everything happens when we call this method.
// Renamed from "start" to avoid accidentally appearing to be
// a local variable named "start".
void
Shell::start_shell (int argc, char **argv, char **env)
{
  early_init ();
  for (;;)
    {
      try
        {
          run_shell (argc, argv, env);
        }
      catch (const subshell_child_start &)
        {
          argc = subshell_argc;
          argv = subshell_argv;
          env = subshell_envp;
          sourced_env = false;
        }
    }
}

void
Shell::early_init ()
{
  xtrace_init ();

#ifdef __CYGWIN__
  _cygwin32_check_tmp ();
#endif /* __CYGWIN__ */

  /* Wait forever if we are debugging a login shell. */
  while (debugging_login_shell)
    ::sleep (3);

  set_default_locale ();

  running_setuid = uidget ();

  if (std::getenv ("POSIXLY_CORRECT") || std::getenv ("POSIX_PEDANTIC"))
    posixly_correct = true;
}

void
Shell::run_shell (int argc, char **argv, char **env)
{
#if defined(RESTRICTED_SHELL)
  int saverst;
#endif
  bool locally_skip_execution;

  shell_reinitialized = false;

  /* Initialize `local' variables for all `invocations' of main (). */
  int arg_index = 1;

  if (arg_index > argc)
    arg_index = argc;

  command_execution_string.clear ();
  shell_script_filename.clear ();
  want_pending_command = locally_skip_execution = read_from_stdin = false;
  default_input = stdin;

#if defined(BUFFERED_INPUT)
  default_buffered_input = -1;
#endif

  /* Fix for the `infinite process creation' bug when running shell scripts
     from startup files on System V. */
  login_shell = make_login_shell = 0;

  /* If this shell has already been run, then reinitialize it to a
     vanilla state. */
  if (shell_initialized || shell_name)
    {
      /* Make sure that we do not infinitely recurse as a login shell. */
      if (*shell_name == '-')
        shell_name++;

      shell_reinitialize ();
    }

  shell_environment = env;
  set_shell_name (argv[0]);

  ::gettimeofday (&shellstart, nullptr);
  shell_start_time = shellstart.tv_sec;

  /* Parse argument flags from the input line. */

  /* Find full word arguments first. */
  arg_index = parse_long_options (argv, arg_index, argc);

  if (want_initial_help)
    {
      show_shell_usage (stdout, true);
      std::exit (EXECUTION_SUCCESS);
    }

  if (do_version)
    {
      show_shell_version (1);
      std::exit (EXECUTION_SUCCESS);
    }

  echo_input_at_read = verbose_flag; /* --verbose given */

  /* All done with full word options; do standard shell option parsing.*/
  this_command_name = shell_name; /* for error reporting */
  arg_index = parse_shell_options (argv, arg_index, argc);

  /* If user supplied the "--login" (or -l) flag, then set and invert
     LOGIN_SHELL. */
  if (make_login_shell)
    {
      login_shell++;
      login_shell = -login_shell;
    }

  set_login_shell ("login_shell", login_shell != 0);

  if (dump_po_strings)
    dump_translatable_strings = true;

  if (dump_translatable_strings)
    read_but_dont_execute = 1;

  if (running_setuid && privileged_mode == 0)
    disable_priv_mode ();

  /* Need to get the argument to a -c option processed in the
     above loop.  The next arg is a command to execute, and the
     following args are $0...$n respectively. */
  if (want_pending_command)
    {
      command_execution_string = argv[arg_index];
      if (command_execution_string.empty ())
        {
          report_error (_ ("%s: option requires an argument"), "-c");
          std::exit (EX_BADUSAGE);
        }
      arg_index++;
    }
  this_command_name = nullptr;

  /* First, let the outside world know about our interactive status.
     A shell is interactive if the `-i' flag was given, or if all of
     the following conditions are met:
        no -c command
        no arguments remaining or the -s flag given
        standard input is a terminal
        standard error is a terminal
     Refer to Posix.2, the description of the `sh' utility. */

  if (forced_interactive ||                 /* -i flag */
      (command_execution_string.empty () && /* No -c command and ... */
       !wordexp_only &&                     /* No --wordexp and ... */
       ((arg_index == argc) ||              /*   no remaining args or... */
        read_from_stdin)
       &&                         /*   -s flag with args, and */
       isatty (fileno (stdin)) && /* Input is a terminal and */
       isatty (fileno (stderr)))) /* error output is a terminal. */
    init_interactive ();
  else
    init_noninteractive ();

  /* If we're in a strict Posix.2 mode, turn on interactive comments,
     alias expansion in non-interactive shells, and other Posix.2 things.
   */
  if (posixly_correct)
    {
      bind_variable ("POSIXLY_CORRECT", "y", 0);
      sv_strict_posix ("POSIXLY_CORRECT");
    }

  /* Now we run the shopt_alist and process the options. */
  if (!shopt_alist.empty ())
    run_shopt_alist ();

  /* From here on in, the shell must be a normal functioning shell.
     Variables from the environment are expected to be set, etc. */
  shell_initialize ();

  set_default_lang ();
  set_default_locale_vars ();

  /*
   * M-x term -> TERM=eterm-color INSIDE_EMACS='251,term:0.96' (eterm)
   * M-x shell -> TERM='dumb' INSIDE_EMACS='25.1,comint' (no line editing)
   *
   * Older versions of Emacs may set EMACS to 't' or to something like
   * '22.1 (term:0.96)' instead of (or in addition to) setting
   * INSIDE_EMACS. They may set TERM to 'eterm' instead of 'eterm-color'.
   * They may have a now-obsolete command that sets neither EMACS nor
   * INSIDE_EMACS: M-x terminal -> TERM='emacs-em7955' (line editing)
   */
  if (interactive_shell)
    {
      bool emacs_term, in_emacs;

      const char *term = get_string_value ("TERM");
      const char *emacs = get_string_value ("EMACS");
      const char *inside_emacs = get_string_value ("INSIDE_EMACS");

      if (inside_emacs)
        {
          emacs_term = strstr (inside_emacs, ",term:") != nullptr;
          in_emacs = true;
        }
      else if (emacs)
        {
          /* Infer whether we are in an older Emacs. */
          emacs_term = strstr (emacs, " (term:") != nullptr;
          in_emacs = emacs_term || STREQ (emacs, "t");
        }
      else
        in_emacs = emacs_term = false;

      /* Not sure any emacs terminal emulator sets TERM=emacs any more */
      if (term)
        {
          no_line_editing |= STREQ (term, "emacs");
          no_line_editing |= in_emacs && STREQ (term, "dumb");

          /* running_under_emacs == 2 for `eterm' */
          running_under_emacs = in_emacs || STREQN (term, "emacs", 5);
          running_under_emacs += emacs_term && STREQN (term, "eterm", 5);
        }

      if (running_under_emacs)
        gnu_error_format = 1;
    }

  int top_level_arg_index = arg_index;
  // Note: Infer says this write is a dead store, but the variable is
  // used in the catch block, so I think it's necessary to initialize it.
  char old_errexit_flag = exit_immediately_on_error;

  /* Give this shell a place to catch exceptions before executing the
     startup files.  This allows users to press C-c to abort the
     lengthy startup, which retries with locally_skip_execution set. */
  for (;;)
    {
      try
        {
          arg_index = top_level_arg_index;

          /* Execute the start-up scripts. */

          if (!interactive_shell)
            {
              unbind_variable ("PS1");
              unbind_variable ("PS2");
              interactive = false;
#if 0
      /* This has already been done by init_noninteractive */
      expand_aliases = posixly_correct;
#endif
            }
          else
            {
              change_flag ('i', FLAG_ON);
              interactive = true;
            }

#if defined(RESTRICTED_SHELL)
          /* Set restricted_shell based on whether the basename of $0 indicates
             that the shell should be restricted or if the `-r' option was
             supplied at startup. */
          restricted_shell = shell_is_restricted (shell_name);

          /* If the `-r' option is supplied at invocation, make sure that the
             shell is not in restricted mode when running the startup files. */
          saverst = restricted;
          restricted = 0;
#endif

          /* Set positional parameters before running startup files.
             top_level_arg_index holds the index of the current argument before
             setting the positional parameters, so any changes performed in the
             startup files won't affect later option processing. */
          if (wordexp_only)
            ; /* nothing yet */
          else if (!command_execution_string.empty ())
            (void)bind_args (argv, arg_index, argc, 0); /* $0 ... $n */
          else if (arg_index != argc && read_from_stdin == 0)
            {
              shell_script_filename = argv[arg_index++];
              (void)bind_args (argv, arg_index, argc, 1); /* $1 ... $n */
            }
          else
            (void)bind_args (argv, arg_index, argc, 1); /* $1 ... $n */

          /* The startup files are run with `set -e' temporarily disabled. */
          if (!locally_skip_execution && !running_setuid)
            {
              old_errexit_flag = exit_immediately_on_error;
              exit_immediately_on_error = 0;

              run_startup_files ();
              exit_immediately_on_error += old_errexit_flag;
            }

          /* If we are invoked as `sh', turn on Posix mode. */
          if (act_like_sh)
            {
              bind_variable ("POSIXLY_CORRECT", "y", 0);
              sv_strict_posix ("POSIXLY_CORRECT");
            }

#if defined(RESTRICTED_SHELL)
          /* Turn on the restrictions after executing the startup files.  This
             means that `bash -r' or `set -r' invoked from a startup file will
             turn on the restrictions after the startup files are executed. */
          restricted = saverst || restricted;
          if (!shell_reinitialized)
            maybe_make_restricted (shell_name);
#endif /* RESTRICTED_SHELL */

#if defined(WORDEXP_OPTION)
          if (wordexp_only)
            {
              startup_state = 3;
              last_command_exit_value
                  = run_wordexp (argv[top_level_arg_index]);
              exit_shell (last_command_exit_value);
            }
#endif

          if (!command_execution_string.empty ())
            {
              startup_state = 2;

              if (debugging_mode)
                start_debugger ();

#if defined(ONESHOT)
              executing = true;
              run_one_command (command_execution_string);
              exit_shell (last_command_exit_value);
#else  /* ONESHOT */
              with_input_from_string (command_execution_string, "-c");
              goto read_and_execute;
#endif /* !ONESHOT */
            }

          /* Get possible input filename and set up default_buffered_input or
             default_input as appropriate. */
          if (!shell_script_filename.empty ())
            open_shell_script (shell_script_filename.c_str ());
          else if (!interactive)
            {
              /* In this mode, bash is reading a script from stdin, which is a
                 pipe or redirected file. */
#if defined(BUFFERED_INPUT)
              default_buffered_input = ::fileno (stdin); /* == 0 */
#else
              std::setbuf (default_input, nullptr);
#endif /* !BUFFERED_INPUT */
              read_from_stdin = true;
            }
          else if (top_level_arg_index
                   == argc) /* arg index before startup files */
            /* "If there are no operands and the -c option is not specified,
               the -s option shall be assumed." */
            read_from_stdin = true;

          set_bash_input ();

          if (debugging_mode && locally_skip_execution == 0
              && running_setuid == 0
              && (reading_shell_script || interactive_shell == 0))
            start_debugger ();

          /* Do the things that should be done only for interactive shells. */
          if (interactive_shell)
            {
              /* Set up for checking for presence of mail. */
              reset_mail_timer ();
              init_mail_dates ();

#if defined(HISTORY)
              /* Initialize the interactive history stuff. */
              bash_initialize_history ();
              /* Don't load the history from the history file if we've already
                 saved some lines in this session (e.g., by putting `history -s
                 xx' into one of the startup files). */
              if (!shell_initialized && history_lines_this_session == 0)
                load_history ();
#endif /* HISTORY */

              /* Initialize terminal state for interactive shells after the
                 .bash_profile and .bashrc are interpreted. */
              get_tty_state ();
            }

#if !defined(ONESHOT)
        read_and_execute:
#endif /* !ONESHOT */

          shell_initialized = true;

          if (pretty_print_mode && interactive_shell)
            {
              internal_warning (
                  _ ("pretty-printing mode ignored in interactive shells"));
              pretty_print_mode = false;
            }
          if (pretty_print_mode)
            exit_shell (pretty_print_loop ());

          /* Read commands until exit condition. */
          reader_loop ();
          exit_shell (last_command_exit_value);
        }
      catch (const bash_exception &e)
        {
          if (e.type == EXITPROG || e.type == ERREXIT)
            exit_shell (last_command_exit_value);
          else
            {
#if defined(JOB_CONTROL)
              /* Reset job control, since run_startup_files turned it off. */
              set_job_control (interactive_shell);
#endif
              /* Reset value of `set -e', since it's turned off before running
                 the startup files. */
              exit_immediately_on_error += old_errexit_flag;
              locally_skip_execution = true;
            }
        }
    }
}

int
Shell::parse_long_options (char **argv, int arg_start, int arg_end)
{
  int arg_index, longarg;
  char *arg_string;

  arg_index = arg_start;
  while ((arg_index != arg_end) && (arg_string = argv[arg_index])
         && (*arg_string == '-'))
    {
      longarg = 0;

      /* Make --login equivalent to -login. */
      if (arg_string[1] == '-' && arg_string[2])
        {
          longarg = 1;
          arg_string++;
        }

      std::vector<LongArg>::const_iterator it;
      string_view this_arg (arg_string + 1); // skip '-'
      bool found = false;

      for (it = long_args.begin (); it != long_args.end (); ++it)
        {
          if (this_arg == (*it).name)
            {
              if ((*it).type == Bool)
                *(*it).value.bool_ptr = true;
              else if ((*it).type == Flag)
                *(*it).value.flag_ptr = 1;
              else if (argv[++arg_index] == nullptr)
                {
                  std::string name_cstr (to_string ((*it).name));
                  report_error (_ ("%s: option requires an argument"),
                                name_cstr.c_str ());
                  exit (EX_BADUSAGE);
                }
              else
                *(*it).value.str_ptr = argv[arg_index];

              found = true;
              break;
            }
        }
      if (!found)
        {
          if (longarg)
            {
              report_error (_ ("%s: invalid option"), argv[arg_index]);
              show_shell_usage (stderr, false);
              std::exit (EX_BADUSAGE);
            }
          break; /* No such argument.  Maybe flag arg. */
        }

      arg_index++;
    }

  return arg_index;
}

int
Shell::parse_shell_options (char **argv, int arg_start, int arg_end)
{
  char *arg_string;

  int arg_index = arg_start;
  while (arg_index != arg_end && (arg_string = argv[arg_index])
         && (*arg_string == '-' || *arg_string == '+'))
    {
      /* There are flag arguments, so parse them. */
      int next_arg = arg_index + 1;

      /* A single `-' signals the end of options.  From the 4.3 BSD sh.
         An option `--' means the same thing; this is the standard
         getopt(3) meaning. */
      if (arg_string[0] == '-'
          && (arg_string[1] == '\0'
              || (arg_string[1] == '-' && arg_string[2] == '\0')))
        return next_arg;

      int i = 1;
      char arg_character;
      char *o_option;

      char on_or_off = arg_string[0];
      while ((arg_character = arg_string[i++]))
        {
          switch (arg_character)
            {
            case 'c':
              want_pending_command = true;
              break;

            case 'l':
              make_login_shell = true;
              break;

            case 's':
              read_from_stdin = true;
              break;

            case 'o':
              o_option = argv[next_arg];
              if (o_option == nullptr)
                {
                  set_option_defaults ();
                  list_minus_o_opts (-1, (on_or_off == '-') ? 0 : 1);
                  reset_option_defaults ();
                  break;
                }
              if (set_minus_o_option (on_or_off, o_option)
                  != EXECUTION_SUCCESS)
                std::exit (EX_BADUSAGE);
              next_arg++;
              break;

            case 'O':
              /* Since some of these can be overridden by the normal
                 interactive/non-interactive shell initialization or
                 initializing posix mode, we save the options and process
                 them after initialization. */
              o_option = argv[next_arg];
              if (o_option == nullptr)
                {
                  shopt_listopt (o_option, (on_or_off == '-') ? 0 : 1);
                  break;
                }
              add_shopt_to_alist (o_option, on_or_off);
              next_arg++;
              break;

            case 'D':
              dump_translatable_strings = true;
              break;

            default:
              if (change_flag (arg_character, on_or_off) == FLAG_ERROR)
                {
                  report_error (_ ("%c%c: invalid option"), on_or_off,
                                arg_character);
                  show_shell_usage (stderr, false);
                  std::exit (EX_BADUSAGE);
                }
            }
        }
      /* Can't do just a simple increment anymore -- what about
         "bash -abouo emacs ignoreeof -hP"? */
      arg_index = next_arg;
    }

  return arg_index;
}

/* Exit the shell with status S. */
void
Shell::exit_shell (int s)
{
  std::fflush (stdout); /* XXX */
  std::fflush (stderr);

  /* Clean up the terminal if we are in a state where it's been modified. */
#if defined(READLINE)
  if (RL_ISSTATE (RL_STATE_TERMPREPPED) && rl_deprep_term_function)
    (*(dynamic_cast<Readline *> (this)).*rl_deprep_term_function) ();
#endif
  if (read_tty_modified ())
    read_tty_cleanup ();

  /* Do trap[0] if defined.  Allow it to override the exit status
     passed to us. */
  if (signal_is_trapped (0))
    s = run_exit_trap ();

#if defined(PROCESS_SUBSTITUTION)
  unlink_all_fifos ();
#endif /* PROCESS_SUBSTITUTION */

#if defined(HISTORY)
  if (remember_on_history)
    maybe_save_shell_history ();
#endif /* HISTORY */

#if defined(COPROCESS_SUPPORT)
  coproc_flush ();
#endif

#if defined(JOB_CONTROL)
  /* If the user has run `shopt -s huponexit', hangup all jobs when we exit
     an interactive login shell.  ksh does this unconditionally. */
  if (interactive_shell && login_shell && hup_on_exit)
    hangup_all_jobs ();

  /* If this shell is interactive, or job control is active, terminate all
     stopped jobs and restore the original terminal process group.  Don't do
     this if we're in a subshell and calling exit_shell after, for example,
     a failed word expansion.  We want to do this even if the shell is not
     interactive because we set the terminal's process group when job control
     is enabled regardless of the interactive status. */
  if (subshell_environment == 0)
    end_job_control ();
#endif /* JOB_CONTROL */

  /* Always return the exit status of the last command to our parent. */
  sh_exit (s);
}

/* Exit a subshell, which includes calling the exit trap.  We don't want to
   do any more cleanup, since a subshell is created as an exact copy of its
   parent. */
void
Shell::subshell_exit (int s)
{
  std::fflush (stdout);
  std::fflush (stderr);

  /* Do trap[0] if defined.  Allow it to override the exit status
     passed to us. */
  if (signal_is_trapped (0))
    s = run_exit_trap ();

  sh_exit (s);
}

/* Source the bash startup files.  If POSIXLY_CORRECT is non-zero, we obey
   the Posix.2 startup file rules:  $ENV is expanded, and if the file it
   names exists, that file is sourced.  The Posix.2 rules are in effect
   for interactive shells only. (section 4.56.5.3) */

/* Execute ~/.bashrc for most shells.  Never execute it if
   ACT_LIKE_SH is set, or if NO_RC is set.

   If the executable file "/usr/gnu/src/bash/foo" contains:

   #!/usr/gnu/bin/bash
   echo hello

   then:

         COMMAND	    EXECUTE BASHRC
         --------------------------------
         bash -c foo		NO
         bash foo		NO
         foo			NO
         rsh machine ls		YES (for rsh, which calls `bash -c')
         rsh machine foo	YES (for shell started by rsh) NO (for foo!)
         echo ls | bash		NO
         login			NO
         bash			YES
*/
void
Shell::execute_env_file (const char *env_file)
{
  if (env_file && *env_file)
    {
      std::string fn (
          expand_string_unsplit_to_string (env_file, Q_DOUBLE_QUOTES));
      if (!fn.empty ())
        maybe_execute_file (fn, true);
    }
}

void
Shell::run_startup_files ()
{
#if defined(JOB_CONTROL)
  bool old_job_control;
#endif
  bool sourced_login, run_by_ssh;

  /* get the rshd/sshd case out of the way first. */
  if (!interactive_shell && !no_rc && (login_shell == 0) && !act_like_sh
      && !command_execution_string.empty ())
    {
#ifdef SSH_SOURCE_BASHRC
      run_by_ssh = (find_variable ("SSH_CLIENT") != (SHELL_VAR *)0)
                   || (find_variable ("SSH2_CLIENT") != (SHELL_VAR *)0);
#else
      run_by_ssh = false;
#endif

      /* If we were run by sshd or we think we were run by rshd, execute
         ~/.bashrc if we are a top-level shell. */
      if ((run_by_ssh || isnetconn (fileno (stdin))) && shell_level < 2)
        {
#ifdef SYS_BASHRC
          maybe_execute_file (SYS_BASHRC, 1);
#endif
          maybe_execute_file (bashrc_file, 1);
          return;
        }
    }

#if defined(JOB_CONTROL)
  /* Startup files should be run without job control enabled. */
  old_job_control = interactive_shell ? set_job_control (false) : false;
#endif

  sourced_login = false;

  /* A shell begun with the --login (or -l) flag that is not in posix mode
     runs the login shell startup files, no matter whether or not it is
     interactive.  If NON_INTERACTIVE_LOGIN_SHELLS is defined, run the
     startup files if argv[0][0] == '-' as well. */
#if defined(NON_INTERACTIVE_LOGIN_SHELLS)
  if (login_shell && !posixly_correct)
#else
  if (login_shell < 0 && !posixly_correct)
#endif
    {
      /* We don't execute .bashrc for login shells. */
      no_rc = true;

      /* Execute /etc/profile and one of the personal login shell
         initialization files. */
      if (!no_profile)
        {
          maybe_execute_file (SYS_PROFILE, true);

          if (act_like_sh) /* sh */
            maybe_execute_file ("~/.profile", true);
          else if ((maybe_execute_file ("~/.bash_profile", true) == 0)
                   && (maybe_execute_file ("~/.bash_login", true)
                       == 0)) /* bash */
            maybe_execute_file ("~/.profile", true);
        }

      sourced_login = true;
    }

  /* A non-interactive shell not named `sh' and not in posix mode reads and
     executes commands from $BASH_ENV.  If `su' starts a shell with `-c cmd'
     and `-su' as the name of the shell, we want to read the startup files.
     No other non-interactive shells read any startup files. */
  if (!interactive_shell && !(su_shell && login_shell))
    {
      if (!posixly_correct && !act_like_sh && !privileged_mode && !sourced_env)
        {
          sourced_env = true;
          const char *bash_env = get_string_value ("BASH_ENV");
          if (bash_env)
            execute_env_file (bash_env);
        }
      return;
    }

  /* Interactive shell or `-su' shell. */
  if (!posixly_correct) /* bash, sh */
    {
      if (login_shell && !sourced_login)
        {
          // sourced_login = true; // not referenced again

          /* We don't execute .bashrc for login shells. */
          no_rc = true;

          /* Execute /etc/profile and one of the personal login shell
             initialization files. */
          if (!no_profile)
            {
              maybe_execute_file (SYS_PROFILE, true);

              if (act_like_sh) /* sh */
                maybe_execute_file ("~/.profile", true);
              else if ((maybe_execute_file ("~/.bash_profile", true) == 0)
                       && (maybe_execute_file ("~/.bash_login", true)
                           == 0)) /* bash */
                maybe_execute_file ("~/.profile", true);
            }
        }

      /* bash */
      if (!act_like_sh && !no_rc)
        {
#ifdef SYS_BASHRC
          maybe_execute_file (SYS_BASHRC, true);
#endif
          maybe_execute_file (bashrc_file, true);
        }
      /* sh */
      else if (act_like_sh && !privileged_mode && !sourced_env)
        {
          sourced_env = true;
          const char *env = get_string_value ("ENV");
          if (env)
            execute_env_file (env);
        }
    }
  else /* bash --posix, sh --posix */
    {
      /* bash and sh */
      if (interactive_shell && !privileged_mode && !sourced_env)
        {
          sourced_env = true;
          const char *env = get_string_value ("ENV");
          if (env)
            execute_env_file (env);
        }
    }

#if defined(JOB_CONTROL)
  set_job_control (old_job_control);
#endif
}

#if defined(RESTRICTED_SHELL)
/* Return 1 if the shell should be a restricted one based on NAME or the
   value of `restricted'.  Don't actually do anything, just return a
   boolean value. */
bool
Shell::shell_is_restricted (const char *name)
{
  if (restricted)
    return true;

  const char *temp = base_pathname (name);

  if (*temp == '-')
    temp++;

  return STREQ (temp, RESTRICTED_SHELL_NAME);
}

/* Perhaps make this shell a `restricted' one, based on NAME.  If the
   basename of NAME is "rbash", then this shell is restricted.  The
   name of the restricted shell is a configurable option, see config.h.
   In a restricted shell, PATH, SHELL, ENV, and BASH_ENV are read-only
   and non-unsettable.
   Do this also if `restricted' is already set to 1; maybe the shell was
   started with -r. */
bool
Shell::maybe_make_restricted (const char *name)
{
  const char *temp = base_pathname (name);

  if (*temp == '-')
    temp++;

  if (restricted || STREQ (temp, RESTRICTED_SHELL_NAME))
    {
#if defined(RBASH_STATIC_PATH_VALUE)
      bind_variable ("PATH" sv, RBASH_STATIC_PATH_VALUE, 0);
      stupidly_hack_special_variables ("PATH" sv); /* clear hash table */
#endif
      set_var_read_only ("PATH");
      set_var_read_only ("SHELL");
      set_var_read_only ("ENV");
      set_var_read_only ("BASH_ENV");
      set_var_read_only ("HISTFILE");
      restricted = true;
    }
  return restricted;
}
#endif /* RESTRICTED_SHELL */

/* Fetch the current set of uids and gids and return 1 if we're running
   setuid or setgid. */
bool
Shell::uidget ()
{
  uid_t u;

  u = ::getuid ();
  if (current_user.uid != u)
    {
      delete[] current_user.user_name;
      delete[] current_user.shell;
      delete[] current_user.home_dir;
      current_user.user_name = current_user.shell = current_user.home_dir
          = nullptr;
    }

  current_user.uid = u;
  current_user.gid = ::getgid ();
  current_user.euid = ::geteuid ();
  current_user.egid = ::getegid ();

  /* See whether or not we are running setuid or setgid. */
  return (current_user.uid != current_user.euid)
         || (current_user.gid != current_user.egid);
}

void
Shell::disable_priv_mode ()
{
#if HAVE_SETRESUID
  if (setresuid (current_user.uid, current_user.uid, current_user.uid) < 0)
#else
  if (setuid (current_user.uid) < 0)
#endif
    {
#if defined(EXIT_ON_SETUID_FAILURE)
      int e = errno;
#endif
      sys_error (_ ("cannot set uid to %d: effective uid %d"),
                 current_user.uid, current_user.euid);
#if defined(EXIT_ON_SETUID_FAILURE)
      if (e == EAGAIN)
        std::exit (e);
#endif
    }
#if HAVE_SETRESGID
  if (setresgid (current_user.gid, current_user.gid, current_user.gid) < 0)
#else
  if (setgid (current_user.gid) < 0)
#endif
    sys_error (_ ("cannot set gid to %d: effective gid %d"), current_user.gid,
               current_user.egid);

  current_user.euid = current_user.uid;
  current_user.egid = current_user.gid;
}

#if defined(WORDEXP_OPTION)
// XXX this code path hasn't been updated for C++ yet.
static int
run_wordexp (char *words)
{
  int code, nw, nb;
  WORD_LIST *wl, *tl, *result;

  code = setjmp_nosigs (top_level);

  if (code != NOT_JUMPED)
    {
      switch (code)
        {
          /* Some kind of throw to top_level has occurred. */
        case FORCE_EOF:
          return last_command_exit_value = 127;
        case EXITPROG:
        case ERREXIT:
        case SIGEXIT:
        case EXITBLTIN:
          return last_command_exit_value;
        case DISCARD:
          return last_command_exit_value = 1;
        default:
          command_error ("run_wordexp", CMDERR_BADJUMP, code, 0);
        }
    }

  /* Run it through the parser to get a list of words and expand them */
  if (words && *words)
    {
      with_input_from_string (words, "--wordexp");
      if (parse_command () != 0)
        return 126;
      if (global_command == 0)
        {
          printf ("0\n0\n");
          return 0;
        }
      if (global_command->type != cm_simple)
        return 126;
      wl = global_command->value.Simple->words;
      if (protected_mode)
        for (tl = wl; tl; tl = tl->next)
          tl->word->flags |= W_NOCOMSUB | W_NOPROCSUB;
      result = wl ? expand_words_no_vars (wl) : (WORD_LIST *)0;
    }
  else
    result = (WORD_LIST *)0;

  last_command_exit_value = 0;

  if (result == 0)
    {
      std::printf ("0\n0\n");
      return 0;
    }

  /* Count up the number of words and bytes, and print them.  Don't count
     the trailing NUL byte. */
  for (nw = nb = 0, wl = result; wl; wl = wl->next)
    {
      nw++;
      nb += std::strlen (wl->word->word);
    }
  std::printf ("%u\n%u\n", nw, nb);
  /* Print each word on a separate line.  This will have to be changed when
     the interface to glibc is completed. */
  for (wl = result; wl; wl = wl->next)
    std::printf ("%s\n", wl->word->word);

  return 0;
}
#endif

#if defined(ONESHOT)
/* Run one command, given as the argument to the -c option.  Tell
   parse_and_execute not to fork for a simple command. */
int
Shell::run_one_command (string_view command)
{
  try
    {
      return parse_and_execute (command, "-c", SEVAL_NOHIST | SEVAL_RESETLINE);
    }
  catch (const bash_exception &e)
    {
#if defined(PROCESS_SUBSTITUTION)
      unlink_fifo_list ();
#endif /* PROCESS_SUBSTITUTION */
      switch (e.type)
        {
          /* Some kind of throw to top_level has occurred. */
        case FORCE_EOF:
          return last_command_exit_value = 127;
        case EXITPROG:
        case ERREXIT:
        case SIGEXIT:
        case EXITBLTIN:
          return last_command_exit_value;
        case DISCARD:
          return last_command_exit_value = 1;
        case NOEXCEPTION:
        default:
          command_error ("run_one_command", CMDERR_BADJUMP, e.type);
        }
    }
}
#endif /* ONESHOT */

int
Shell::bind_args (char **argv, int arg_start, int arg_end, int start_index)
{
  int i;
  WORD_LIST *args, *tl;

  for (i = arg_start, args = tl = nullptr; i < arg_end; i++)
    {
      if (args == nullptr)
        args = tl = new WORD_LIST (make_word (argv[i]));
      else
        {
          tl->set_next (new WORD_LIST (make_word (argv[i])));
          tl = tl->next ();
        }
    }

  if (args)
    {
      if (start_index == 0) /* bind to $0...$n for sh -c command */
        {
          /* Posix.2 4.56.3 says that the first argument after sh -c command
             becomes $0, and the rest of the arguments become $1...$n */
          shell_name = savestring (args->word->word);
          delete[] dollar_vars[0];
          dollar_vars[0] = savestring (args->word->word);
          remember_args (args->next (), true);
          if (debugging_mode)
            {
              push_args (args->next ()); /* BASH_ARGV and BASH_ARGC */
              bash_argv_initialized = true;
            }
        }
      else /* bind to $1...$n for shell script */
        {
          remember_args (args, true);
          /* We do this unconditionally so something like -O extdebug doesn't
             do it first.  We're setting the definitive positional params
             here. */
          if (debugging_mode)
            {
              push_args (args); /* BASH_ARGV and BASH_ARGC */
              bash_argv_initialized = true;
            }
        }

      delete args;
    }

  return i;
}

void
Shell::start_debugger ()
{
#if defined(DEBUGGER) && defined(DEBUGGER_START_FILE)
  int old_errexit;
  int r;

  old_errexit = exit_immediately_on_error;
  exit_immediately_on_error = 0;

  r = force_execute_file (DEBUGGER_START_FILE, 1);
  if (r < 0)
    {
      internal_warning (_ ("cannot start debugger; debugging mode disabled"));
      debugging_mode = 0;
    }
  error_trace_mode = function_trace_mode = debugging_mode;

  set_shellopts ();
  set_bashopts ();

  exit_immediately_on_error += old_errexit;
#endif
}

int
Shell::open_shell_script (const char *script_name)
{
  char sample[80];
  int sample_len;
  struct stat sb;
#if defined(ARRAY_VARS)
  SHELL_VAR *funcname_v, *bash_source_v, *bash_lineno_v;
  ARRAY *funcname_a, *bash_source_a, *bash_lineno_a;
#endif

  std::string filename (script_name);

  int fd = open (filename.c_str (), O_RDONLY);
  if ((fd < 0) && (errno == ENOENT) && (!absolute_program (filename.c_str ())))
    {
      int e = errno;
      /* If it's not in the current directory, try looking through PATH
         for it. */
      std::string path_filename (find_path_file (script_name));
      if (!path_filename.empty ())
        {
          filename = path_filename;
          fd = open (filename.c_str (), O_RDONLY);
        }
      else
        errno = e;
    }

  if (fd < 0)
    {
      int e = errno;
      file_error (filename.c_str ());
#if defined(JOB_CONTROL)
      end_job_control (); /* just in case we were run as bash -i script */
#endif
      sh_exit ((e == ENOENT) ? EX_NOTFOUND : EX_NOINPUT);
    }

  delete[] dollar_vars[0];
  dollar_vars[0]
      = exec_argv0 ? savestring (exec_argv0) : savestring (script_name);

  if (exec_argv0)
    {
      delete[] exec_argv0;
      exec_argv0 = nullptr;
    }

  if (file_isdir (filename.c_str ()))
    {
#if defined(EISDIR)
      errno = EISDIR;
#else
      errno = EINVAL;
#endif
      file_error (filename.c_str ());
#if defined(JOB_CONTROL)
      end_job_control (); /* just in case we were run as bash -i script */
#endif
      sh_exit (EX_NOINPUT);
    }

#if defined(ARRAY_VARS)
  GET_ARRAY_FROM_VAR ("FUNCNAME", funcname_v, funcname_a);
  GET_ARRAY_FROM_VAR ("BASH_SOURCE", bash_source_v, bash_source_a);
  GET_ARRAY_FROM_VAR ("BASH_LINENO", bash_lineno_v, bash_lineno_a);

  bash_source_a->shift_element (filename);
  if (bash_lineno_a)
    bash_lineno_a->shift_element (itos (executing_line_number ()));

  funcname_a->shift_element ("main");
#endif

#ifdef HAVE_DEV_FD
  bool fd_is_tty = isatty (fd);
#else
  bool fd_is_tty = false;
#endif

  /* Only do this with non-tty file descriptors we can seek on. */
  if (!fd_is_tty && (::lseek (fd, 0L, 1) != -1))
    {
      /* Check to see if the `file' in `bash file' is a binary file
         according to the same tests done by execute_simple_command (),
         and report an error and exit if it is. */
      sample_len = static_cast<int> (::read (fd, sample, sizeof (sample)));
      if (sample_len < 0)
        {
          int e = errno;
          if ((::fstat (fd, &sb) == 0) && S_ISDIR (sb.st_mode))
            {
#if defined(EISDIR)
              errno = EISDIR;
              file_error (filename.c_str ());
#else
              internal_error (_ ("%s: Is a directory"), filename);
#endif
            }
          else
            {
              errno = e;
              file_error (filename.c_str ());
            }
#if defined(JOB_CONTROL)
          end_job_control (); /* just in case we were run as bash -i script */
#endif
          std::exit (EX_NOEXEC);
        }
      else if (sample_len > 0
               && (check_binary_file (sample,
                                      static_cast<size_t> (sample_len))))
        {
          internal_error (_ ("%s: cannot execute binary file"),
                          filename.c_str ());
#if defined(JOB_CONTROL)
          end_job_control (); /* just in case we were run as bash -i script */
#endif
          std::exit (EX_BINARY_FILE);
        }
      /* Now rewind the file back to the beginning. */
      ::lseek (fd, 0L, 0);
    }

  /* Open the script.  But try to move the file descriptor to a randomly
     large one, in the hopes that any descriptors used by the script will
     not match with ours. */
  fd = move_to_high_fd (fd, 1, -1);

#if defined(BUFFERED_INPUT)
  default_buffered_input = fd;
  SET_CLOSE_ON_EXEC (default_buffered_input);
#else  /* !BUFFERED_INPUT */
  default_input = fdopen (fd, "r");

  if (default_input == 0)
    {
      file_error (filename);
      std::exit (EX_NOTFOUND);
    }

  SET_CLOSE_ON_EXEC (fd);
  if (fileno (default_input) != fd)
    SET_CLOSE_ON_EXEC (fileno (default_input));
#endif /* !BUFFERED_INPUT */

  /* Just about the only way for this code to be executed is if something
     like `bash -i /dev/stdin' is executed. */
  if (interactive_shell && fd_is_tty)
    {
      ::dup2 (fd, 0);
      ::close (fd);
      fd = 0;
#if defined(BUFFERED_INPUT)
      default_buffered_input = 0;
#else
      std::fclose (default_input);
      default_input = stdin;
#endif
    }
  else if (forced_interactive && fd_is_tty == 0)
    /* But if a script is called with something like `bash -i scriptname',
       we need to do a non-interactive setup here, since we didn't do it
       before. */
    init_interactive_script ();

  reading_shell_script = true;
  return fd;
}

/* Initialize the input routines for the parser. */
void
Shell::set_bash_input ()
{
  /* Make sure the fd from which we are reading input is not in
     no-delay mode. */
#if defined(BUFFERED_INPUT)
  if (!interactive)
    sh_unset_nodelay_mode (default_buffered_input);
  else
#endif /* !BUFFERED_INPUT */
    sh_unset_nodelay_mode (fileno (stdin));

  /* with_input_from_stdin really means `with_input_from_readline' */
  if (interactive && !no_line_editing)
    with_input_from_stdin ();
#if defined(BUFFERED_INPUT)
  else if (!interactive)
    with_input_from_buffered_stream (default_buffered_input, dollar_vars[0]);
#endif /* BUFFERED_INPUT */
  else
    with_input_from_stream (default_input, dollar_vars[0]);
}

/* Close the current shell script input source and forget about it.  This is
   extern so execute_cmd.c:initialize_subshell() can call it.  If CHECK_ZERO
   is non-zero, we close default_buffered_input even if it's the standard
   input (fd 0). */
void
Shell::unset_bash_input (int check_zero)
{
#if defined(BUFFERED_INPUT)
  if ((check_zero && default_buffered_input >= 0)
      || (check_zero == 0 && default_buffered_input > 0))
    {
      close_buffered_fd (default_buffered_input);
      default_buffered_input = bash_input.location.buffered_fd = -1;
      bash_input.type = st_none; /* XXX */
    }
#else  /* !BUFFERED_INPUT */
  if (default_input)
    {
      std::fclose (default_input);
      default_input = nullptr;
    }
#endif /* !BUFFERED_INPUT */
}

#if !defined(PROGRAM)
#define PROGRAM "bash"
#endif

void
Shell::set_shell_name (const char *argv0)
{
  /* Here's a hack.  If the name of this shell is "sh", then don't do
     any startup files; just try to be more like /bin/sh. */
  shell_name = argv0 ? base_pathname (argv0) : PROGRAM;

  if (argv0 && *argv0 == '-')
    {
      if (*shell_name == '-')
        shell_name++;
      login_shell = 1;
    }

  if (shell_name[0] == 's' && shell_name[1] == 'h' && shell_name[2] == '\0')
    act_like_sh = true;

  if (shell_name[0] == 's' && shell_name[1] == 'u' && shell_name[2] == '\0')
    su_shell = true;

  shell_name = argv0 ? argv0 : PROGRAM;
  delete[] dollar_vars[0];
  dollar_vars[0] = savestring (shell_name);

  /* A program may start an interactive shell with
          "execl ("/bin/bash", "-", NULL)".
     If so, default the name of this shell to our name. */
  if (!shell_name || !*shell_name || (shell_name[0] == '-' && !shell_name[1]))
    shell_name = PROGRAM;
}

void
Shell::init_interactive ()
{
  expand_aliases = startup_state = 1;
  interactive_shell = interactive = true;
#if defined(HISTORY)
  if (enable_history_list == -1)
    enable_history_list = 1; /* set default  */
  remember_on_history = enable_history_list;
#if defined(BANG_HISTORY)
  histexp_flag = history_expansion; /* XXX */
#endif
#endif
}

void
Shell::init_noninteractive ()
{
#if defined(HISTORY)
  if (enable_history_list == -1) /* set default */
    enable_history_list = 0;
  bash_history_reinit (0);
#endif /* HISTORY */
  interactive_shell = interactive = false;
  startup_state = 0;
  expand_aliases = posixly_correct; /* XXX - was 0 not posixly_correct */
  no_line_editing = true;
#if defined(JOB_CONTROL)
  /* Even if the shell is not interactive, enable job control if the -i or
     -m option is supplied at startup. */
  set_job_control (forced_interactive || jobs_m_flag);
#endif /* JOB_CONTROL */
}

void
Shell::init_interactive_script ()
{
#if defined(HISTORY)
  if (enable_history_list == -1)
    enable_history_list = 1;
#endif
  init_noninteractive ();
  expand_aliases = startup_state = 1;
  interactive_shell = true;
#if defined(HISTORY)
  remember_on_history = enable_history_list; /* XXX */
#endif
}

void
Shell::get_current_user_info ()
{
  struct passwd *entry;

  /* Don't fetch this more than once. */
  if (current_user.user_name == nullptr)
    {
#if defined(__TANDEM)
      entry = ::getpwnam (getlogin ());
#else
      entry = ::getpwuid (current_user.uid);
#endif
      if (entry)
        {
          current_user.user_name = savestring (entry->pw_name);
          current_user.shell = (entry->pw_shell && entry->pw_shell[0])
                                   ? savestring (entry->pw_shell)
                                   : savestring ("/bin/sh");
          current_user.home_dir = savestring (entry->pw_dir);
        }
      else
        {
          current_user.user_name = savestring ("I have no name!");
          current_user.shell = savestring ("/bin/sh");
          current_user.home_dir = savestring ("/");
        }
#if defined(HAVE_GETPWENT)
      ::endpwent ();
#endif
    }
}

/* Do whatever is necessary to initialize the shell.
   Put new initializations in here. */
void
Shell::shell_initialize ()
{
  char hostname[256];
  int should_be_restricted;

  /* Line buffer output for stderr and stdout. */
  if (shell_initialized == 0)
    {
      sh_setlinebuf (stderr);
      sh_setlinebuf (stdout);
    }

  /* Sort the array of shell builtins so that the binary search in
     find_shell_builtin () works correctly. */
  initialize_shell_builtins ();

  /* Initialize the trap signal handlers before installing our own
     signal handlers.  traps.c:restore_original_signals () is responsible
     for restoring the original default signal handlers.  That function
     is called when we make a new child. */
  initialize_traps ();
  initialize_signals (false);

  /* It's highly unlikely that this will change. */
  if (current_host_name == nullptr)
    {
      /* Initialize current_host_name. */
      if (::gethostname (hostname, 255) < 0)
        current_host_name = "??host??";
      else
        current_host_name = savestring (hostname);
    }

  /* Initialize the stuff in current_user that comes from the password
     file.  We don't need to do this right away if the shell is not
     interactive. */
  if (interactive_shell)
    get_current_user_info ();

  /* Initialize our interface to the tilde expander. */
  tilde_initialize ();

#if defined(RESTRICTED_SHELL)
  should_be_restricted = shell_is_restricted (shell_name);
#endif

  /* Initialize internal and environment variables.  Don't import shell
     functions from the environment if we are running in privileged or
     restricted mode or if the shell is running setuid. */
#if defined(RESTRICTED_SHELL)
  initialize_shell_variables (shell_environment, privileged_mode || restricted
                                                     || should_be_restricted
                                                     || running_setuid);
#else
  initialize_shell_variables (shell_environment,
                              privileged_mode || running_setuid);
#endif

  /* Initialize the data structures for storing and running jobs. */
  initialize_job_control (jobs_m_flag);

  /* Initialize input streams to null. */
  initialize_bash_input ();

  initialize_flags ();

  /* Initialize the shell options.  Don't import the shell options
     from the environment variables $SHELLOPTS or $BASHOPTS if we are
     running in privileged or restricted mode or if the shell is running
     setuid. */
#if defined(RESTRICTED_SHELL)
  initialize_shell_options (privileged_mode || restricted
                            || should_be_restricted || running_setuid);
  initialize_bashopts (privileged_mode || restricted || should_be_restricted
                       || running_setuid);
#else
  initialize_shell_options (privileged_mode || running_setuid);
  initialize_bashopts (privileged_mode || running_setuid);
#endif
}

/* Function called by main () when it appears that the shell has already
   had some initialization performed.  This is supposed to reset the world
   back to a pristine state, as if we had been exec'ed. */
void
Shell::shell_reinitialize ()
{
  /* The default shell prompts. */
  primary_prompt = PPROMPT;
  secondary_prompt = SPROMPT;

  /* Things that get 1. */
  current_command_number = 1;

  /* We have decided that the ~/.bashrc file should not be executed
     for the invocation of each shell script.  If the variable $ENV
     (or $BASH_ENV) is set, its value is used as the name of a file
     to source. */
  no_rc = no_profile = true;

  /* Things that get 0. */
  login_shell = make_login_shell = interactive = executing = 0;
  debugging = do_version = false;
  line_number = last_command_exit_value = 0;
  forced_interactive = interactive_shell = 0;
  subshell_environment = running_in_background = 0;
  expand_aliases = 0;
  bash_argv_initialized = 0;

  /* XXX - should we set jobs_m_flag to 0 here? */

#if defined(HISTORY)
  bash_history_reinit ((enable_history_list = 0));
#endif /* HISTORY */

#if defined(RESTRICTED_SHELL)
  restricted = 0;
#endif /* RESTRICTED_SHELL */

  /* Ensure that the default startup file is used.  (Except that we don't
     execute this file for reinitialized shells). */
  bashrc_file = DEFAULT_BASHRC;

  /* Delete all variables and functions.  They will be reinitialized when
     the environment is parsed. */
  delete_all_contexts (shell_variables);
  delete_all_variables (shell_functions);

  reinit_special_variables ();

#if defined(READLINE)
  bashline_reinitialize ();
#endif

  shell_reinitialized = true;
}

void
Shell::show_shell_usage (FILE *fp, bool extra)
{
  char *s, *t;

  if (extra)
    std::fprintf (fp, _ ("GNU bash, version %s-(%s)\n"),
                  shell_version_string ().c_str (), MACHTYPE);
  std::fprintf (fp,
                _ ("Usage:\t%s [GNU long option] [option] ...\n\t%s [GNU long "
                   "option] [option] script-file ...\n"),
                shell_name, shell_name);
  std::fputs (_ ("GNU long options:\n"), fp);
  for (std::vector<LongArg>::iterator it = long_args.begin ();
       it != long_args.end (); ++it)
    {
      std::string name_str (to_string ((*it).name));
      std::fprintf (fp, "\t--%s\n", name_str.c_str ());
    }

  std::fputs (_ ("Shell options:\n"), fp);
  std::fputs (
      _ ("\t-ilrsD or -c command or -O shopt_option\t\t(invocation only)\n"),
      fp);

  char *set_opts = nullptr;
  std::map<string_view, Builtin>::iterator it = shell_builtins.find ("set");
  if (it != shell_builtins.end ())
    set_opts = savestring ((*it).second.short_doc);

  if (set_opts)
    {
      s = std::strchr (set_opts, '[');
      if (s == nullptr)
        s = set_opts;
      while (*++s == '-')
        ;
      t = std::strchr (s, ']');
      if (t)
        *t = '\0';
      std::fprintf (fp, _ ("\t-%s or -o option\n"), s);
      delete[] set_opts;
    }

  if (extra)
    {
      std::fprintf (fp,
                    _ ("Type `%s -c \"help set\"' for more information about "
                       "shell options.\n"),
                    shell_name);
      std::fprintf (fp,
                    _ ("Type `%s -c help' for more information about shell "
                       "builtin commands.\n"),
                    shell_name);
      std::fprintf (fp, _ ("Use the `bashbug' command to report bugs.\n"));
      std::fprintf (fp, "\n");
      std::fprintf (
          fp, _ ("bash home page: <http://www.gnu.org/software/bash>\n"));
      std::fprintf (fp, _ ("General help using GNU software: "
                           "<http://www.gnu.org/gethelp/>\n"));
    }
}

void
Shell::run_shopt_alist ()
{
  std::vector<STRING_INT_ALIST>::iterator it;

  for (it = shopt_alist.begin (); it != shopt_alist.end (); ++it)
    if (shopt_setopt ((*it).word, ((*it).token == '-')) != EXECUTION_SUCCESS)
      std::exit (EX_BADUSAGE);

  shopt_alist.clear ();
}

} // namespace bash
