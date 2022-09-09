/* shell.h -- The data structures used by the shell */

/* Copyright (C) 1993-2020 Free Software Foundation, Inc.

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

#if !defined (_SHELL_H_)
#  define _SHELL_H_

#include "config.hh"

#include "bashtypes.hh"
#include "chartypes.hh"

#if defined (HAVE_PWD_H)
#  include <pwd.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <cerrno>

#include <vector>

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "posixstat.hh"
#include "bashintl.hh"

#include "command.hh"
#include "syntax.hh"
#include "error.hh"
#include "arrayfunc.hh"
#include "quit.hh"
#include "maxpath.hh"
#include "subst.hh"
#include "sig.hh"
#include "pathnames.hh"
#include "externs.hh"
#include "jobs.hh"
#include "shtty.hh"

#if defined (READLINE)
#include "readline.hh"
#endif

#if defined (HAVE_ICONV)
#  include <iconv.h>
#endif

namespace bash
{

/* Flag values for history_control */
enum hist_ctl_flags {
  HC_IGNSPACE =		0x01,
  HC_IGNDUPS =		0x02,
  HC_ERASEDUPS =	0x04,

  HC_IGNBOTH =	(HC_IGNSPACE | HC_IGNDUPS)
};

#if defined (STRICT_POSIX)
#  undef HISTEXPAND_DEFAULT
#  define HISTEXPAND_DEFAULT	0
#else
#  if !defined (HISTEXPAND_DEFAULT)
#    define HISTEXPAND_DEFAULT	1
#  endif /* !HISTEXPAND_DEFAULT */
#endif

const int NO_PIPE =		 -1;
const int REDIRECT_BOTH =	 -2;

const int NO_VARIABLE =		 -1;

/* Values that can be returned by execute_command (). */
const int EXECUTION_FAILURE =	  1;
const int EXECUTION_SUCCESS =	  0;

/* Usage messages by builtins result in a return status of 2. */
const int EX_BADUSAGE =		  2;

const int EX_MISCERROR =	  2;

/* Special exit statuses used by the shell, internally and externally. */
const int EX_RETRYFAIL =	124;
const int EX_WEXPCOMSUB =	125;
const int EX_BINARY_FILE =	126;
const int EX_NOEXEC =		126;
const int EX_NOINPUT =		126;
const int EX_NOTFOUND =		127;

const int EX_SHERRBASE =	256;	/* all special error values are > this. */

const int EX_BADSYNTAX =	257;	/* shell syntax error */
const int EX_USAGE =		258;	/* syntax error in usage */
const int EX_REDIRFAIL =	259;	/* redirection failed */
const int EX_BADASSIGN =	260;	/* variable assignment error */
const int EX_EXPFAIL =		261;	/* word expansion failed */
const int EX_DISKFALLBACK =	262;	/* fall back to disk command from builtin */

/* Flag values that control parameter pattern substitution. */
const int MATCH_ANY =		0x000;
const int MATCH_BEG =		0x001;
const int MATCH_END =		0x002;

const int MATCH_TYPEMASK =	0x003;

const int MATCH_GLOBREP =	0x010;
const int MATCH_QUOTED =	0x020;
const int MATCH_ASSIGNRHS =	0x040;
const int MATCH_STARSUB =	0x080;

/* Flags for skip_to_delim */

enum sd_flags {
  SD_NOTHROW =		0x001,	/* don't throw exception on fatal error. */
  SD_INVERT =		0x002,	/* look for chars NOT in passed set */
  SD_NOQUOTEDELIM =	0x004,	/* don't let single or double quotes act as delimiters */
  SD_NOSKIPCMD =	0x008,	/* don't skip over $(, <(, or >( command/process substitution; parse them as commands */
  SD_EXTGLOB =		0x010,	/* skip over extended globbing patterns if appropriate */
  SD_IGNOREQUOTE =	0x020,	/* single and double quotes are not special */
  SD_GLOB =		0x040,	/* skip over glob patterns like bracket expressions */
  SD_NOPROCSUB =	0x080,	/* don't parse process substitutions as commands */
  SD_COMPLETE =		0x100,	/* skip_to_delim during completion */
  SD_HISTEXP =		0x200,	/* skip_to_delim during history expansion */
  SD_ARITHEXP =		0x400	/* skip_to_delim during arithmetic expansion */
};

static inline sd_flags
operator | (const sd_flags &a, const sd_flags &b) {
  return static_cast<sd_flags> (static_cast<uint32_t> (a) | static_cast<uint32_t> (b));
}

/* Information about the current user. */
struct UserInfo {
  UserInfo() : uid(static_cast<uid_t> (-1)), euid(static_cast<uid_t> (-1)),
	       gid(static_cast<gid_t> (-1)), egid(static_cast<gid_t> (-1)) {}

  uid_t uid;
  uid_t euid;
  gid_t gid;
  gid_t egid;
  char *user_name;
  char *shell;		/* shell from the password file */
  char *home_dir;
};

/* Definitions moved from expr.cc to simplify definition of Shell class. */

/* The Tokens.  Singing "The Lion Sleeps Tonight". */

enum token_t {
  NONE =	0,
  EQEQ =	1,	/* "==" */
  NEQ =		2,	/* "!=" */
  LEQ =		3,	/* "<=" */
  GEQ =		4,	/* ">=" */
  STR =		5,	/* string */
  NUM =		6,	/* number */
  LAND =	7,	/* "&&" Logical AND */
  LOR =		8,	/* "||" Logical OR */
  LSH =		9,	/* "<<" Left SHift */
  RSH =		10,	/* ">>" Right SHift */
  OP_ASSIGN =	11,	/* op= expassign as in Posix.2 */
  COND =	12,	/* exp1 ? exp2 : exp3 */
  POWER =	13,	/* exp1**exp2 */
  PREINC =	14,	/* ++var */
  PREDEC =	15,	/* --var */
  POSTINC =	16,	/* var++ */
  POSTDEC =	17,	/* var-- */
  EQ =		'=',
  GT =		'>',
  LT =		'<',
  PLUS =	'+',
  MINUS =	'-',
  MUL =		'*',
  DIV =		'/',
  MOD =		'%',
  NOT =		'!',
  LPAR =	'(',
  RPAR =	')',
  BAND =	'&',	/* Bitwise AND */
  BOR =		'|',	/* Bitwise OR. */
  BXOR =	'^',	/* Bitwise eXclusive OR. */
  BNOT =	'~',	/* Bitwise NOT; Two's complement. */
  QUES =	'?',
  COL =		':',
  COMMA =	','
};

struct lvalue
{
  std::string tokstr;	/* possibly-rewritten lvalue if not nullptr */
  intmax_t tokval;	/* expression evaluated value */
  SHELL_VAR *tokvar;	/* variable described by array or var reference */
  intmax_t ind;		/* array index if not -1 */

  void init ()
  {
    tokstr.clear ();
    tokvar = nullptr;
    tokval = ind = -1;
  }
};

/* A struct defining a single expression context. */
struct EXPR_CONTEXT
{
  std::string expression, tp, lasttp;
  std::string tokstr;
  intmax_t tokval;
  struct lvalue lval;
  token_t curtok, lasttok;
  int noeval;
  int _pad;		// silence clang -Wpadded warning
};

/* Simple shell state: variables that can be memcpy'd to subshells. */
class SimpleState
{
public:
  // Initialize with default values (defined in shell.c).
  SimpleState ();

#if defined (VFORK_SUBSHELL)
  SimpleState (int fd);		// TODO: Load initial values from pipe
#endif

  // Everything below this is protected or private.

protected:

  /* ************************************************************** */
  /*		Bash Variables (array/struct/ptr types)		    */
  /* ************************************************************** */

  /* time in seconds when the shell was started */
  time_t shell_start_time;
  struct timeval shellstart;

  /* ************************************************************** */
  /*		Bash Variables (size_t: 32/64-bit)		    */
  /* ************************************************************** */

#if defined (HANDLE_MULTIBYTE)
  size_t ifs_firstc_len;
#endif

  /* ************************************************************** */
  /*		Bash Variables (32-bit int types)		    */
  /* ************************************************************** */

  /* from expr.cc: the OP in OP= */
  token_t assigntok;

  // from bashhist.c
  int history_lines_this_session;
  int history_lines_in_file;
  int history_control;

  /* The controlling tty for this shell. */
  int shell_tty;

  /* The shell's process group. */
  pid_t shell_pgrp;

  /* The terminal's process group. */
  pid_t terminal_pgrp;

  /* The process group of the shell's parent. */
  pid_t original_pgrp;

  /* The process group of the pipeline currently being made. */
  pid_t pipeline_pgrp;

#if defined (PGRP_PIPE)
  /* Pipes which each shell uses to communicate with the process group leader
     until all of the processes in a pipeline have been started.  Then the
     process leader is allowed to continue. */
  int pgrp_pipe[2];
#endif

  /* Last child made by the shell.  */
  volatile pid_t last_made_pid;

  /* Pid of the last asynchronous child. */
  volatile pid_t last_asynchronous_pid;

  /* Used to synchronize between wait_for and other functions and the SIGCHLD
     signal handler. */
  int sigchld;
  int queue_sigchld;

  /* The maximum bytes per char for this locale. */
  int locale_mb_cur_max;

  /* The number of shift states for this locale. */
  int locale_shiftstates;

  int subshell_environment;
  int shell_compatibility_level;

  /* The number of commands executed so far. */
  int current_command_number;

  /* Non-zero is the recursion depth for commands. */
  int indirection_level;

#if defined (BUFFERED_INPUT)
  /* The file descriptor from which the shell is reading input. */
  int default_buffered_input;
#endif

  int variable_context;

  int shell_level;

  /* variables from evalfile.c */
  int sourcelevel;

  /* variables from evalstring.c */
  int parse_and_execute_level;

  /* Variables declared in execute_cmd.c, used by many other files */
  int return_catch_flag;
  int return_catch_value;
  int last_command_exit_value;
  int last_command_exit_signal;
  int executing_builtin;
  int executing_list;		/* list nesting level */
  int comsub_ignore_return;	/* can be greater than 1 */
  int subshell_level;
  int funcnest;
  int funcnest_max;
  int evalnest;
  int evalnest_max;
  int sourcenest;
  int sourcenest_max;
  int line_number_for_err_trap;

  /* variables from break.def/continue.def */
  int breaking;
  int continuing;
  int loop_level;

  int posparam_count;
  pid_t dollar_dollar_pid;

  int subshell_argc;
  int shopt_ind;
  int shopt_len;

  /* The random number seed.  You can change this by setting RANDOM. */
  u_bits32_t rseed;
  u_bits32_t rseed32;
  u_bits32_t last_rand32;
  int last_random_value;

  /* Set to errno when a here document cannot be created for some reason.
     Used to print a reasonable error message. */
  int heredoc_errno;

  /* variables from subst.c */

  /* Process ID of the last command executed within command substitution. */
  pid_t last_command_subst_pid;
  pid_t current_command_subst_pid;

  /* variables from lib/sh/shtty.c */

  /* Variables used to keep track of the characters in IFS. */
  bool ifs_cmap[UCHAR_MAX + 1];
  bool ifs_is_set;
  bool ifs_is_null;

#if defined (HANDLE_MULTIBYTE)
  char ifs_firstc[MB_LEN_MAX];
#else
  char ifs_firstc;
#endif

  /* Set by expand_word_unsplit and several of the expand_string_XXX functions;
     used to inhibit splitting and re-joining $* on $IFS, primarily when doing
     assignment statements.  The idea is that if we're in a context where this
     is set, we're not going to be performing word splitting, so we use the same
     rules to expand $* as we would if it appeared within double quotes. */
  bool expand_no_split_dollar_star;

  /* ************************************************************** */
  /*		Bash Variables (8-bit bool/char types)		    */
  /* ************************************************************** */

  /* Is this locale a UTF-8 locale? */
  bool locale_utf8locale;

  /* variables from read.def */
  bool sigalrm_seen;

  /* variables from shift.def */
  char print_shift_error;

  /* variables from source.def */
  bool source_searches_cwd;
  char source_uses_path;

  /* variables from wait.def */
  bool wait_intr_flag;

  bool tempenv_assign_error;
  bool array_needs_making;

  /* Shared state from bashhist.c */
  bool remember_on_history;
  char enable_history_list;		/* value for `set -o history' */
  char literal_history;			/* controlled by `shopt lithist' */
  char force_append_history;
  char command_oriented_history;
  bool current_command_first_line_saved;
  bool current_command_first_line_comment;
  bool hist_last_line_added;
  bool hist_last_line_pushed;
  char dont_save_function_defs;

#if defined (BANG_HISTORY)
  bool history_expansion_inhibited;
  bool double_quotes_inhibit_history_expansion;
#endif /* BANG_HISTORY */

  /* Shared state from bashline.c */
  bool bash_readline_initialized;
  bool hostname_list_initialized;

  /* variables from execute_cmd.h */
  bool builtin_ignoring_errexit;
  bool match_ignore_case;
  bool executing_command_builtin;
  bool stdin_redir;

  /* these are controlled via shopt */

  /* If this is non-zero, do job control. */
  bool job_control;

  /* Are we running in background? (terminal_pgrp != shell_pgrp) */
  bool running_in_background;

  /* Call this when you start making children. */
  bool already_making_children;

  /* If this is non-zero, $LINES and $COLUMNS are reset after every process
     exits from get_tty_state(). */
  char check_window_size;

  /* EOF reached. */
  bool EOF_Reached;

  /* If non-zero, command substitution inherits the value of errexit option.
     Managed by builtins/shopt.def. */
  char inherit_errexit;

  /* Sentinel to tell when we are performing variable assignments preceding a
     command name and putting them into the environment.  Used to make sure
     we use the temporary environment when looking up variable values. */
  bool assigning_in_environment;

  /* Tell the expansion functions to not longjmp back to top_level on fatal
     errors.  Enabled when doing completion and prompt string expansion. */
  bool no_throw_on_fatal_error;

  /* Non-zero means to allow unmatched globbed filenames to expand to
     a null file. Managed by builtins/shopt.def. */
  char allow_null_glob_expansion;

  /* Non-zero means to throw an error when globbing fails to match anything.
     Managed by builtins/shopt.def. */
  char fail_glob_expansion;

  /* ************************************************************** */
  /*								    */
  /*		The Standard sh Flags (from flags.c).		    */
  /*								    */
  /* ************************************************************** */

  /* Non-zero means automatically mark variables which are modified or created
     as auto export variables. */
  char mark_modified_vars;

  /* Non-zero causes asynchronous job notification.  Otherwise, job state
     notification only takes place just before a primary prompt is printed. */
  char asynchronous_notification;

  /* Non-zero means exit immediately if a command exits with a non-zero
     exit status.  The first is what controls set -e; the second is what
     bash uses internally. */
  char errexit_flag;
  char exit_immediately_on_error;

  /* Non-zero means disable filename globbing. */
  char disallow_filename_globbing;

  /* Non-zero means that all keyword arguments are placed into the environment
     for a command, not just those that appear on the line before the command
     name. */
  char place_keywords_in_env;

  /* Non-zero means read commands, but don't execute them.  This is useful
     for debugging shell scripts that should do something hairy and possibly
     destructive. */
  char read_but_dont_execute;

  /* Non-zero means end of file is after one command. */
  char just_one_command;

  /* Non-zero means don't overwrite existing files while doing redirections. */
  char noclobber;

  /* Non-zero means trying to get the value of $i where $i is undefined
     causes an error, instead of a null substitution. */
  char unbound_vars_is_error;

  /* Non-zero means type out input lines after you read them. */
  char echo_input_at_read;
  char verbose_flag;

  /* Non-zero means type out the command definition after reading, but
     before executing. */
  char echo_command_at_execute;

  /* Non-zero means turn on the job control features. */
  char jobs_m_flag;

  /* Non-zero means this shell is interactive, even if running under a
     pipe. */
  char forced_interactive;

  /* By default, follow the symbolic links as if they were real directories
     while hacking the `cd' command.  This means that `cd ..' moves up in
     the string of symbolic links that make up the current directory, instead
     of the absolute directory.  The shell variable `nolinks' also controls
     this flag. */
  char no_symbolic_links;

  /* ************************************************************** */
  /*								    */
  /*		     Non-Standard Flags Follow Here.		    */
  /*								    */
  /* ************************************************************** */

  /* Non-zero means look up and remember command names in a hash table, */
  char hashing_enabled;

#if defined (BANG_HISTORY)
  /* Non-zero means that we are doing history expansion.  The default.
     This means !22 gets the 22nd line of history. */
  char history_expansion;
  char histexp_flag;
#endif /* BANG_HISTORY */

  /* Non-zero means that we allow comments to appear in interactive commands. */
  char interactive_comments;

#if defined (RESTRICTED_SHELL)
  /* Non-zero means that this shell is `restricted'.  A restricted shell
     disallows: changing directories, command or path names containing `/',
     unsetting or resetting the values of $PATH and $SHELL, and any type of
     output redirection. */
  char restricted;			/* currently restricted */
  char restricted_shell;		/* shell was started in restricted mode. */
#endif /* RESTRICTED_SHELL */

  /* Non-zero means that this shell is running in `privileged' mode.  This
     is required if the shell is to run setuid.  If the `-p' option is
     not supplied at startup, and the real and effective uids or gids
     differ, disable_priv_mode is called to relinquish setuid status. */
  char privileged_mode;

#if defined (BRACE_EXPANSION)
  /* Zero means to disable brace expansion: foo{a,b} -> fooa foob */
  char brace_expansion;
#endif

  /* Non-zero means that shell functions inherit the DEBUG trap. */
  char function_trace_mode;

  /* Non-zero means that shell functions inherit the ERR trap. */
  char error_trace_mode;

  /* Non-zero means that the rightmost non-zero exit status in a pipeline
     is the exit status of the entire pipeline.  If each processes exits
     with a 0 status, the status of the pipeline is 0. */
  char pipefail_opt;

  /* ************************************************************** */
  /*								    */
  /*		     Flags managed by shopt.def.		    */
  /*								    */
  /* ************************************************************** */

#if defined (ARRAY_VARS)
  char array_expand_once;
  char assoc_expand_once;
#endif
  char cdable_vars;
  char cdspelling;
  char check_hashed_filenames;
  char expand_aliases;
#if defined (EXTENDED_GLOB)
  char extended_glob;
#endif
  char extended_quote;
  char glob_asciirange;
  char glob_dot_filenames;
  char glob_ignore_case;
  char glob_star;
  char lastpipe_opt;
  char localvar_inherit;
  char localvar_unset;
  char mail_warning;
  char no_exit_on_failed_exec;
#if defined (PROGRAMMABLE_COMPLETION)
  char prog_completion_enabled;
  char progcomp_alias;
#endif
  char promptvars;
#if defined (SYSLOG_HISTORY)
  char syslog_history;
#endif
  char xpg_echo;

#if defined (READLINE)
  char complete_fullquote;
  char dircomplete_expand;
  char dircomplete_expand_relpath;
  char dircomplete_spelling;
  char force_fignore;
  char hist_verify;
  char history_reediting;
  char no_empty_command_completion;
  char perform_hostname_completion;
#endif

  // from shell.h

  // Set to 1 to use GNU error format.
  char gnu_error_format;

  /* Non-zero means that this shell has already been run; i.e. you should
     call shell_reinitialize () if you need to start afresh. */
  bool shell_initialized;

  // Non-zero means argv has been initialized.
  bool bash_argv_initialized;

  /* Non-zero means that this shell is a login shell.
     Specifically:
     0 = not login shell.
     1 = login shell from getty (or equivalent fake out)
    -1 = login shell from "--login" (or -l) flag.
    -2 = both from getty, and from flag.
  */
  char login_shell;

  /* Non-zero means that at this moment, the shell is interactive.  In
     general, this means that the shell is at this moment reading input
     from the keyboard. */
  bool interactive;

  /* Non-zero means that the shell was started as an interactive shell. */
  bool interactive_shell;

  /* Non-zero means to send a SIGHUP to all jobs when an interactive login
     shell exits. */
  char hup_on_exit;

  /* Non-zero means to list status of running and stopped jobs at shell exit */
  char check_jobs_at_exit;

  /* Non-zero means to change to a directory name supplied as a command name */
  char autocd;

  /* Tells what state the shell was in when it started:
	0 = non-interactive shell script
	1 = interactive
	2 = -c command
	3 = wordexp evaluation
     This is a superset of the information provided by interactive_shell.
  */
  char startup_state;

  bool reading_shell_script;

  /* Special debugging helper. */
  bool debugging_login_shell;

  /* Non-zero when we are executing a top-level command. */
  bool executing;

  /* Are we running in an emacs shell window? (== 2 for `eterm`) */
  char running_under_emacs;

  /* Do we have /dev/fd? */
  bool have_devfd;

  /* Non-zero means to act more like the Bourne shell on startup. */
  bool act_like_sh;

  /* Non-zero if this shell is being run by `su'. */
  bool su_shell;

  /* Non-zero if we have already expanded and sourced $ENV. */
  bool sourced_env;

  /* Is this shell running setuid? */
  bool running_setuid;

  /* Values for the long-winded argument names. */
  bool debugging;			/* Do debugging things. */
  bool no_rc;				/* Don't execute ~/.bashrc */
  bool no_profile;			/* Don't execute .profile */
  bool do_version;			/* Display interesting version info. */
  bool make_login_shell;		/* Make this shell be a `-bash' shell. */
  bool want_initial_help;		/* --help option */

#if defined (DEBUGGER)
  char debugging_mode;			/* In debugging mode with --debugger */
#endif
  bool no_line_editing;			/* non-zero -> don't do fancy line editing. */
  bool dump_translatable_strings;	/* Dump strings in $"...", don't execute. */
  bool dump_po_strings;			/* Dump strings in $"..." in po format */
  bool wordexp_only;			/* Do word expansion only */
  bool protected_mode;			/* No command substitution with --wordexp */

  bool pretty_print_mode;		/* pretty-print a shell script */

  char posixly_correct;			/* Non-zero means posix.2 superset. */

  bool read_from_stdin;			/* -s flag supplied */
  bool want_pending_command;		/* -c flag supplied */

  bool shell_reinitialized;

  /* Used by ttsave()/ttrestore() to check if there are valid saved settings. */
  bool ttsaved;

  /* from lib/sh/unicode.c */

  bool u32init;
  bool utf8locale;

  /* set to true if the expression has already been run through word expansion */
  bool already_expanded;

#ifndef HAVE_LOCALE_CHARSET
  char charsetbuf[40];
#endif

  char _pad[8];				// silence clang -Wpadded warning
};


#if defined (READLINE)
#define PARENT_CLASS , public readline::Readline
#define RL_OVERRIDE override
#else
#define PARENT_CLASS
#define RL_OVERRIDE
#endif


/* Shell class, containing global variables and the methods that use them. */
class Shell : public SimpleState PARENT_CLASS
{
public:

  Shell (int argc, char **argv, char **env);

  // public typedefs accessed from other classes

  typedef int (Shell::*sh_builtin_func_t) (WORD_LIST *);	/* sh_wlist_func_t */
  typedef void (Shell::*sh_vptrfunc_t) (void *);

  typedef char *(Shell::*tilde_hook_func_t) (char *);

  struct JOB {
    char *wd;			/* The working directory at time of invocation. */
    PROCESS *pipe;		/* The pipeline of processes that make up this job. */
#if defined (JOB_CONTROL)
    COMMAND *deferred;		/* Commands that will execute when this job is done. */
    sh_vptrfunc_t j_cleanup;	/* Cleanup function to call when job marked JDEAD */
    void *cleanarg;		/* Argument passed to (*j_cleanup)() */
#endif /* JOB_CONTROL */
    pid_t pgrp;			/* The process ID of the process group (necessary). */
    JOB_STATE state;		/* The state that this job is in. */
    job_flags flags;		/* Flags word: J_NOTIFIED, J_FOREGROUND, or J_JOBCONTROL. */
    int _pad;		// suppress clang -Wpadded warning
  };

// Define structs for jobs and collected job stats.

#define NO_JOB  -1	/* An impossible job array index. */
#define DUP_JOB -2	/* A possible return value for get_job_spec (). */
#define BAD_JOBSPEC -3	/* Bad syntax for job spec. */

  // Job stats struct.
  struct jobstats {
    jobstats() : c_childmax(-1), j_current (NO_JOB), j_previous (NO_JOB) {}

    /* limits */
    long c_childmax;
    /* */
    JOB *j_lastmade;	/* last job allocated by stop_pipeline */
    JOB *j_lastasync;	/* last async job allocated by stop_pipeline */
    /* child process statistics */
    int c_living;		/* running or stopped child processes */
    int c_reaped;		/* exited child processes still in jobs list */
    int c_injobs;		/* total number of child processes in jobs list */
    /* child process totals */
    int c_totforked;	/* total number of children this shell has forked */
    int c_totreaped;	/* total number of children this shell has reaped */
    /* job counters and indices */
    int j_lastj;		/* last (newest) job allocated */
    int j_firstj;		/* first (oldest) job allocated */
    int j_njobs;		/* number of non-NULL jobs in jobs array */
    int j_ndead;		/* number of JDEAD jobs in jobs array */
    /* */
    int j_current;	/* current job */
    int j_previous;	/* previous job */

    int _pad;
  };

protected:

  // typedefs moved from general.h so they can become method pointers to Shell.

  /* Map over jobs for job control. */
  typedef int (Shell::*sh_job_map_func_t) (JOB *, int, int, int);

  /* Stuff for hacking variables. */
  typedef int (Shell::*sh_var_map_func_t) (SHELL_VAR *);

  /* Shell function typedefs with prototypes */
  /* `Generic' function pointer typedefs (TODO: remove unused?) */

  typedef int (Shell::*sh_intfunc_t) (int);
  typedef int (Shell::*sh_ivoidfunc_t) ();
  typedef int (Shell::*sh_icpfunc_t) (char *);
  typedef int (Shell::*sh_icppfunc_t) (char **);
  typedef int (Shell::*sh_iptrfunc_t) (void *);

  typedef void (Shell::*sh_voidfunc_t) ();
  typedef void (Shell::*sh_vintfunc_t) (int);
  typedef void (Shell::*sh_vcpfunc_t) (char *);
  typedef void (Shell::*sh_vcppfunc_t) (char **);

  typedef int (Shell::*sh_wdesc_func_t) (WORD_DESC *);
  typedef int (Shell::*sh_wlist_func_t) (WORD_LIST *);

  typedef char *(Shell::*sh_string_func_t) (const char *);	/* like savestring, et al. */

  typedef int (Shell::*sh_msg_func_t) (const char *, ...);	/* printf(3)-like */
  typedef void (Shell::*sh_vmsg_func_t) (const char *, ...);	/* printf(3)-like */

  /* Specific function pointer typedefs.  Most of these could be done
     with #defines. */
  typedef void (Shell::*sh_sv_func_t) (const char *);		/* sh_vcpfunc_t */
  typedef void (Shell::*sh_free_func_t) (void *);			/* sh_vptrfunc_t */
  typedef void (Shell::*sh_resetsig_func_t) (int);		/* sh_vintfunc_t */

  typedef bool (Shell::*sh_ignore_func_t) (const char *);		/* sh_icpfunc_t */

  typedef int (Shell::*sh_assign_func_t) (const char *);
  typedef int (Shell::*sh_wassign_func_t) (WORD_DESC *, int);

  typedef int (Shell::*sh_load_func_t) (const char *);
  typedef void (Shell::*sh_unload_func_t) (const char *);

  // typedefs for builtins (public for access from builtins struct)

public:

#if defined (ALIAS)
  int alias_builtin (WORD_LIST *);
  int unalias_builtin (WORD_LIST *);
#endif /* ALIAS */

#if defined (READLINE)
  int bind_builtin (WORD_LIST *);
#endif /* READLINE */

  int break_builtin (WORD_LIST *);
  int continue_builtin (WORD_LIST *);
  int builtin_builtin (WORD_LIST *);

#if defined (DEBUGGER)
  int caller_builtin (WORD_LIST *);
#endif /* DEBUGGER */

  int cd_builtin (WORD_LIST *);
  int pwd_builtin (WORD_LIST *);
  int colon_builtin (WORD_LIST *);
  int false_builtin (WORD_LIST *);
  int command_builtin (WORD_LIST *);
  int declare_builtin (WORD_LIST *);
  int local_builtin (WORD_LIST *);
  int echo_builtin (WORD_LIST *);
  int enable_builtin (WORD_LIST *);
  int eval_builtin (WORD_LIST *);
  int getopts_builtin (WORD_LIST *);
  int exec_builtin (WORD_LIST *);
  int exit_builtin (WORD_LIST *);
  int logout_builtin (WORD_LIST *);

#if defined (HISTORY)
  int fc_builtin (WORD_LIST *);
#endif /* HISTORY */

#if defined (JOB_CONTROL)
  int fg_builtin (WORD_LIST *);
  int bg_builtin (WORD_LIST *);
#endif /* JOB_CONTROL */

  int hash_builtin (WORD_LIST *);

#if defined (HELP_BUILTIN)
  int help_builtin (WORD_LIST *);
#endif /* HELP_BUILTIN */

#if defined (HISTORY)
  int history_builtin (WORD_LIST *);
#endif /* HISTORY */

#if defined (JOB_CONTROL)
  int jobs_builtin (WORD_LIST *);
  int disown_builtin (WORD_LIST *);
#endif /* JOB_CONTROL */

  int kill_builtin (WORD_LIST *);
  int let_builtin (WORD_LIST *);
  int read_builtin (WORD_LIST *);
  int return_builtin (WORD_LIST *);
  int set_builtin (WORD_LIST *);
  int unset_builtin (WORD_LIST *);
  int export_builtin (WORD_LIST *);
  int readonly_builtin (WORD_LIST *);
  int shift_builtin (WORD_LIST *);
  int source_builtin (WORD_LIST *);

#if defined (JOB_CONTROL)
  int suspend_builtin (WORD_LIST *);
#endif /* JOB_CONTROL */

  int test_builtin (WORD_LIST *);
  int times_builtin (WORD_LIST *);
  int trap_builtin (WORD_LIST *);
  int type_builtin (WORD_LIST *);
#if !defined (_MINIX)
  int ulimit_builtin (WORD_LIST *);
#endif /* !_MINIX */
  int umask_builtin (WORD_LIST *);
  int wait_builtin (WORD_LIST *);

#if defined (PUSHD_AND_POPD)
  int pushd_builtin (WORD_LIST *);
  int popd_builtin (WORD_LIST *);
  int dirs_builtin (WORD_LIST *);
#endif /* PUSHD_AND_POPD */

  int shopt_builtin (WORD_LIST *);
  int printf_builtin (WORD_LIST *);

#if defined (PROGRAMMABLE_COMPLETION)
  int complete_builtin (WORD_LIST *);
  int compgen_builtin (WORD_LIST *);
  int compopt_builtin (WORD_LIST *);
#endif /* PROGRAMMABLE_COMPLETION */

  int mapfile_builtin (WORD_LIST *);

  // end of builtin definitions

  /* callback from lib/sh/getenv.cc */
  char *getenv (const char *);

protected:

  int internal_getopt (WORD_LIST *, const char *);
  void reset_internal_getopt ();

  void posix_readline_initialize (int);
  void reset_completer_word_break_chars ();
  int enable_hostname_completion (int);
  void initialize_readline ();
  void bashline_reset ();
  void bashline_reinitialize ();
  int bash_re_edit (const char *);

  void bashline_set_event_hook ();
  void bashline_reset_event_hook ();

  int bind_keyseq_to_unix_command (const char *);
  int bash_execute_unix_command (int, int);
  int print_unix_command_map ();
  int unbind_unix_command (const char *);

  char **bash_default_completion (const char *, int, int, int, int);

  void set_directory_hook ();

  /* Used by programmable completion code. */
  char *command_word_completion_function (const char *, int);
  char *bash_groupname_completion_function (const char *, int);
  char *bash_servicename_completion_function (const char *, int);

  char **get_hostname_list ();
  void clear_hostname_list ();

  char **bash_directory_completion_matches (const char *);
  char *bash_dequote_text (const char *);

  /* Functions from expr.c. */

  enum eval_flags {
    EXP_NONE =		0,
    EXP_EXPANDED =	0x01
  };

  intmax_t evalexp (const char *, eval_flags, bool *);
  intmax_t subexpr (const char *);
  intmax_t expcomma ();
  intmax_t expassign ();
  intmax_t expcond ();
  intmax_t explor ();
  intmax_t expland ();
  intmax_t expbor ();
  intmax_t expbxor ();
  intmax_t expband ();
  intmax_t exp5 ();
  intmax_t exp4 ();
  intmax_t expshift ();
  intmax_t exp3 ();
  intmax_t expmuldiv ();
  intmax_t exppower ();
  intmax_t exp1 ();
  intmax_t exp0 ();
  intmax_t expr_streval (const char *, int, struct lvalue *);

  void readtok ();

  intmax_t expr_streval (char *, int, struct lvalue *);

  void evalerror (const char *);
  intmax_t strlong (const char *);

  void pushexp ();
  void popexp ();
  void expr_unwind ();
  void expr_bind_variable (const char *, const char *);

#if defined (ARRAY_VARS)
  int expr_skipsubscript (const char *, char *);
  void expr_bind_array_element (const char *, arrayind_t, char *);
#endif

  /* set -x support */
  void xtrace_init ();
#ifdef NEED_XTRACE_SET_DECL
  void xtrace_set (int, FILE *);
#endif
  void xtrace_fdchk (int);
  void xtrace_reset ();
  char *indirection_level_string ();

/* Functions from shell.c. */
  void exit_shell (int) __attribute__((__noreturn__));
  void sh_exit (int) __attribute__((__noreturn__));
  void subshell_exit (int) __attribute__((__noreturn__));
  void set_exit_status (int);
  void disable_priv_mode ();
  void unbind_args ();

#if defined (RESTRICTED_SHELL)
  bool shell_is_restricted (const char *);
  bool maybe_make_restricted (const char *);
#endif

  void unset_bash_input (int);
  void get_current_user_info ();

  /* Functions from eval.c. */
  int reader_loop ();
  int pretty_print_loop ();
  int parse_command ();
  int read_command ();

  /* Functions from braces.c. */
#if defined (BRACE_EXPANSION)
  char **brace_expand (char *);
#endif

  int parser_will_prompt ();
  int parser_in_command_position ();

  void free_pushed_string_input ();

  int parser_expanding_alias ();
  void parser_save_alias ();
  void parser_restore_alias ();

  void clear_shell_input_line ();

  char *decode_prompt_string (const char *);

  int get_current_prompt_level ();
  int set_current_prompt_level (int);

#if defined (HISTORY)
  const char *history_delimiting_chars (const char *);
#endif

  /* Declarations for functions defined in locale.c */
  void set_default_locale ();
  void set_default_locale_vars ();
  bool set_locale_var (const char *, const char *);
  int set_lang (const char *, const char *);
  void set_default_lang ();
  const char *get_locale_var (const char *);
  char *localetrans (const char *, int, int *);
  char *mk_msgstr (char *, bool *);
  char *localeexpand (const char *, int, int, int, int *);

  /* Functions from jobs.h */

#if defined (JOB_CONTROL)

  inline void
  making_children ()
  {
    if (already_making_children)
      return;

    already_making_children = true;
    start_pipeline ();
  }

  inline void
  stop_making_children ()
  {
    already_making_children = false;
  }

  inline void
  cleanup_the_pipeline ()
  {
    PROCESS *disposer;
    sigset_t set, oset;

    BLOCK_CHILD (set, oset);
    disposer = the_pipeline;
    the_pipeline = nullptr;
    UNBLOCK_CHILD (oset);

    if (disposer)
      discard_pipeline (disposer);
  }

#else
#error write the nojobs.c version!
#endif

  void discard_last_procsub_child ();
  void save_pipeline (bool);
  PROCESS *restore_pipeline (bool);
  void start_pipeline ();
  int stop_pipeline (bool, COMMAND *);
  int discard_pipeline (PROCESS *);
  void append_process (char *, pid_t, int, int);

  void save_proc_status (pid_t, int);

  PROCESS *procsub_add (PROCESS *);
  PROCESS *procsub_search (pid_t);
  PROCESS *procsub_delete (pid_t);
  int procsub_waitpid (pid_t);
  void procsub_waitall ();
  void procsub_clear ();
  void procsub_prune ();

  void delete_job (int, int);
  void nohup_job (int);
  void delete_all_jobs (int);
  void nohup_all_jobs (int);

  int count_all_jobs ();

  void terminate_current_pipeline ();
  void terminate_stopped_jobs ();
  void hangup_all_jobs ();
  void kill_current_pipeline ();

  int get_job_by_pid (pid_t, int, PROCESS **);
  void describe_pid (pid_t);

  void list_one_job (JOB *, int, int, int);
  void list_all_jobs (int);
  void list_stopped_jobs (int);
  void list_running_jobs (int);

  pid_t make_child (char *, int);

  int get_tty_state ();
  int set_tty_state ();

  int job_exit_status (int);
  int job_exit_signal (int);

  int wait_for_single_pid (pid_t, int);
  void wait_for_background_pids (procstat *);
  int wait_for (pid_t, int);
  int wait_for_job (int, int, procstat *);
  int wait_for_any_job (int, procstat *);

  void wait_sigint_cleanup ();

  void notify_and_cleanup ();
  void reap_dead_jobs ();
  int start_job (int, bool);
  int kill_pid (pid_t, int, int);
  bool initialize_job_control (bool);
  void initialize_job_signals ();
  int give_terminal_to (pid_t, int);

  void run_sigchld_trap (int);

  int freeze_jobs_list ();
  void unfreeze_jobs_list ();
  void set_jobs_list_frozen (int);
  bool set_job_control (bool);
  void without_job_control ();
  void end_job_control ();
  void restart_job_control ();
  void set_sigchld_handler ();
  void ignore_tty_job_signals ();
  void default_tty_job_signals ();
  void get_original_tty_job_signals ();

  void init_job_stats ();

  void close_pgrp_pipe ();
  void save_pgrp_pipe (int *, int);
  void restore_pgrp_pipe (int *);

  void set_maxchild (int);

  /* Inline helper functions. */
  bool has_job_control() { return job_control; }

#if defined (JOB_CONTROL)
  void debug_print_pgrps ();

  int waitchld (pid_t, int);

  PROCESS *find_pid_in_pipeline (pid_t, PROCESS *, int);
  PROCESS *find_pipeline (pid_t, int, int *);
  PROCESS *find_process (pid_t, int, int *);

  const char *current_working_directory ();
  char *job_working_directory ();
  char *j_strsignal (int);
  const char *printable_job_status (int, PROCESS *, int);

  PROCESS *find_last_proc (int, int);
  pid_t find_last_pid (int, int);

  int set_new_line_discipline (int);
  int map_over_jobs (sh_job_map_func_t *, int, int);
  int job_last_stopped (int);
  int job_last_running (int);
  int most_recent_job_in_state (int, JOB_STATE);
  int find_job (pid_t, int, PROCESS **);
  int print_job (JOB *, int, int, int);
  int process_exit_status (WAIT);
  int process_exit_signal (WAIT);
  int set_job_status_and_cleanup (int);

  WAIT job_signal_status (int);
  WAIT raw_job_exit_status (int);

  void notify_of_job_status ();
  void reset_job_indices ();
  void cleanup_dead_jobs ();
  int processes_in_job (int);
  void realloc_jobs_list ();
  int compact_jobs_list (int);
  void add_process (char *, pid_t);
  void print_pipeline (PROCESS *, int, int, FILE *);
  void pretty_print_job (int, int, FILE *);
  void set_current_job (int);
  void reset_current ();
  void set_job_running (int);
  void setjstatus (int);
  int maybe_give_terminal_to (pid_t, pid_t, int);
  void mark_all_jobs_as_dead ();
  void mark_dead_jobs_as_notified (int);
  void restore_sigint_handler ();
#if defined (PGRP_PIPE)
  void pipe_read (int *);
#endif
#endif /* JOB_CONTROL */

  /* Hash table manipulation */

  ps_index_t *pshash_getbucket (pid_t);
  void pshash_delindex (ps_index_t);

  /* Saved background process status management */
  struct pidstat *bgp_add (pid_t, int);
  int bgp_delete (pid_t);
  void bgp_clear ();
  int bgp_search (pid_t);

  struct pipeline_saver *alloc_pipeline_saver ();

  ps_index_t bgp_getindex ();
  void bgp_resize ();	/* XXX */

  /* from lib/sh/casemod.cc */
  char *sh_modcase (const char *, const char *, sh_modcase_flags);

  /* from lib/sh/eaccess.cc */
  int sh_stataccess (const char *, int);

  /* from lib/sh/makepath.cc */
  char *sh_makepath (const char *, const char *, int);

  /* from lib/sh/mbschr.cc */
  const char *mbschr (const char *, int);

  /* from lib/sh/random.c */
  u_bits32_t genseed ();
  int brand ();
  u_bits32_t brand32 ();
  u_bits32_t get_urandom32 ();

  /* Set the random number generator seed to the least-significant 32 bits of SEED. */
  void
  sbrand (unsigned long seed)
  {
    rseed = static_cast<u_bits32_t> (seed);
    last_random_value = 0;
  }

  void
  seedrand ()
  {
    u_bits32_t iv = genseed ();
    sbrand (iv);
  }

  void
  sbrand32 (u_bits32_t seed)
  {
    last_rand32 = rseed32 = seed;
  }

  void
  seedrand32 ()
  {
    u_bits32_t iv = genseed ();
    sbrand32 (iv);
  }

  void
  perturb_rand32 ()
  {
    rseed32 ^= genseed ();
  }

  /* from lib/sh/shmatch.c */
  int sh_regmatch (const char *, const char *, int);
  int sh_eaccess (const char *, int);
  int sh_euidaccess (const char *, int);

  /* from lib/sh/shmbchar.c */
  const char *mbsmbchar (const char *);
  size_t sh_mbsnlen (const char *, size_t, size_t);

  /* from lib/sh/shquote.c */
  char *sh_double_quote (const char *);
  char *sh_mkdoublequoted (const char *, int, int);
  char *sh_un_double_quote (const char *);
  char *sh_backslash_quote (const char *, const char *, int);
  char *sh_backslash_quote_for_double_quotes (const char *);

  /* include all functions from lib/sh/shtty.c here: they're very small. */
  /* shtty.c -- abstract interface to the terminal, focusing on capabilities. */

  static inline int
  ttgetattr(int fd, TTYSTRUCT *ttp)
  {
#ifdef TERMIOS_TTY_DRIVER
    return ::tcgetattr(fd, ttp);
#else
#  ifdef TERMIO_TTY_DRIVER
    return ::ioctl(fd, TCGETA, ttp);
#  else
    return ::ioctl(fd, TIOCGETP, ttp);
#  endif
#endif
  }

  static inline int
  ttsetattr(int fd, TTYSTRUCT *ttp)
  {
#ifdef TERMIOS_TTY_DRIVER
    return ::tcsetattr(fd, TCSADRAIN, ttp);
#else
#  ifdef TERMIO_TTY_DRIVER
    return ::ioctl(fd, TCSETAW, ttp);
#  else
    return ::ioctl(fd, TIOCSETN, ttp);
#  endif
#endif
  }

  inline void
  ttsave()
  {
    if (ttsaved)
     return;
    (void) ttgetattr (0, &ttin);
    (void) ttgetattr (1, &ttout);
    ttsaved = true;
  }

  inline void
  ttrestore()
  {
    if (!ttsaved)
      return;
    (void) ttsetattr (0, &ttin);
    (void) ttsetattr (1, &ttout);
    ttsaved = false;
  }

  /* Retrieve the internally-saved attributes associated with tty fd FD. */
  inline TTYSTRUCT *
  ttattr (int fd)
  {
    if (!ttsaved)
      return nullptr;
    if (fd == 0)
      return &ttin;
    else if (fd == 1)
      return &ttout;
    else
      return nullptr;
  }

  /*
   * Change attributes in ttp so that when it is installed using
   * ttsetattr, the terminal will be in one-char-at-a-time mode.
   */
  static inline void
  tt_setonechar(TTYSTRUCT *ttp)
  {
#if defined (TERMIOS_TTY_DRIVER) || defined (TERMIO_TTY_DRIVER)

    /* XXX - might not want this -- it disables erase and kill processing. */
    ttp->c_lflag &= ~(static_cast<tcflag_t> (ICANON));

    ttp->c_lflag |= ISIG;
#  ifdef IEXTEN
    ttp->c_lflag |= IEXTEN;
#  endif

    ttp->c_iflag |= ICRNL;	/* make sure we get CR->NL on input */
    ttp->c_iflag &= ~(static_cast<tcflag_t> (INLCR));	/* but no NL->CR */

#  ifdef OPOST
    ttp->c_oflag |= OPOST;
#  endif
#  ifdef ONLCR
    ttp->c_oflag |= ONLCR;
#  endif
#  ifdef OCRNL
    ttp->c_oflag &= ~(static_cast<tcflag_t> (OCRNL));
#  endif
#  ifdef ONOCR
    ttp->c_oflag &= ~(static_cast<tcflag_t> (ONOCR));
#  endif
#  ifdef ONLRET
    ttp->c_oflag &= ~(static_cast<tcflag_t> (ONLRET));
#  endif

    ttp->c_cc[VMIN] = 1;
    ttp->c_cc[VTIME] = 0;

#else

    ttp->sg_flags |= CBREAK;

#endif
  }

  /* Set the tty associated with FD and TTP into one-character-at-a-time mode */
  static inline int
  ttfd_onechar (int fd, TTYSTRUCT *ttp)
  {
    tt_setonechar(ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into one-character-at-a-time mode */
  inline int
  ttonechar ()
  {
    TTYSTRUCT tt;

    if (!ttsaved)
      return -1;
    tt = ttin;
    return ttfd_onechar (0, &tt);
  }

  /*
   * Change attributes in ttp so that when it is installed using
   * ttsetattr, the terminal will be in no-echo mode.
   */
  static inline void
  tt_setnoecho(TTYSTRUCT *ttp)
  {
#if defined (TERMIOS_TTY_DRIVER) || defined (TERMIO_TTY_DRIVER)
    ttp->c_lflag &= ~(static_cast<tcflag_t> (ECHO | ECHOK | ECHONL));
#else
    ttp->sg_flags &= ~ECHO;
#endif
  }

  /* Set the tty associated with FD and TTP into no-echo mode */
  static inline int
  ttfd_noecho (int fd, TTYSTRUCT *ttp)
  {
    tt_setnoecho (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into no-echo mode */
  inline int
  ttnoecho ()
  {
    TTYSTRUCT tt;

    if (!ttsaved)
      return -1;
    tt = ttin;
    return ttfd_noecho (0, &tt);
  }

  /*
   * Change attributes in ttp so that when it is installed using
   * ttsetattr, the terminal will be in eight-bit mode (pass8).
   */
  static inline void
  tt_seteightbit (TTYSTRUCT *ttp)
  {
#if defined (TERMIOS_TTY_DRIVER) || defined (TERMIO_TTY_DRIVER)
    ttp->c_iflag &= ~(static_cast<tcflag_t> (ISTRIP));
    ttp->c_cflag |= CS8;
    ttp->c_cflag &= ~(static_cast<tcflag_t> (PARENB));
#else
    ttp->sg_flags |= ANYP;
#endif
  }

  /* Set the tty associated with FD and TTP into eight-bit mode */
  static inline int
  ttfd_eightbit (int fd, TTYSTRUCT *ttp)
  {
    tt_seteightbit (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into eight-bit mode */
  inline int
  tteightbit ()
  {
    TTYSTRUCT tt;

    if (!ttsaved)
      return -1;
    tt = ttin;
    return ttfd_eightbit (0, &tt);
  }

  /*
   * Change attributes in ttp so that when it is installed using
   * ttsetattr, the terminal will be in non-canonical input mode.
   */
  static inline void
  tt_setnocanon (TTYSTRUCT *ttp)
  {
#if defined (TERMIOS_TTY_DRIVER) || defined (TERMIO_TTY_DRIVER)
    ttp->c_lflag &= ~(static_cast<tcflag_t> (ICANON));
#endif
  }

  /* Set the tty associated with FD and TTP into non-canonical mode */
  static inline int
  ttfd_nocanon (int fd, TTYSTRUCT *ttp)
  {
    tt_setnocanon (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into non-canonical mode */
  inline int
  ttnocanon ()
  {
    TTYSTRUCT tt;

    if (!ttsaved)
      return -1;
    tt = ttin;
    return ttfd_nocanon (0, &tt);
  }

  /*
   * Change attributes in ttp so that when it is installed using
   * ttsetattr, the terminal will be in cbreak, no-echo mode.
   */
  static inline void
  tt_setcbreak(TTYSTRUCT *ttp)
  {
    tt_setonechar (ttp);
    tt_setnoecho (ttp);
  }

  /* Set the tty associated with FD and TTP into cbreak (no-echo,
     one-character-at-a-time) mode */
  static inline int
  ttfd_cbreak (int fd, TTYSTRUCT *ttp)
  {
    tt_setcbreak (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into cbreak (no-echo, one-character-at-a-time) mode */
  inline int
  ttcbreak ()
  {
    TTYSTRUCT tt;

    if (!ttsaved)
      return -1;
    tt = ttin;
    return ttfd_cbreak (0, &tt);
  }

  /* from lib/sh/strtrans.c */
  char *ansicstr (const char *, unsigned int, int, bool *, unsigned int *);
  char *ansiexpand (const char *, unsigned int, unsigned int, unsigned int *);

  /* from lib/sh/tmpfile.c */
  const char *get_tmpdir (int);

  /* from lib/sh/unicode.c */
  char *stub_charset ();
  void u32reset ();
  int u32cconv (u_bits32_t, char *);

  /* from lib/sh/winsize.c */
  void get_new_window_size (int, int *, int *);

  /* from lib/sh/zcatfd.c */
  ssize_t zcatfd (int, int, const char *);

  /* from lib/sh/zread.c */

  /* Read LEN bytes from FD into BUF.  Retry the read on EINTR.  Any other
     error causes the loop to break. */
  ssize_t
  zread (int fd, char *buf, size_t len)
  {
    ssize_t r;

    check_signals ();	/* check for signals before a blocking read */
    while ((r = ::read (fd, buf, len)) < 0 && errno == EINTR)
      {
	int t;
	t = errno;
	/* XXX - bash-5.0 */
	/* We check executing_builtin and run traps here for backwards compatibility */
	if (executing_builtin)
	  check_signals_and_traps ();	/* XXX - should it be check_signals()? */
	else
	  check_signals ();
	errno = t;
      }

    return r;
  }

  /* from lib/sh/zmapfd.c */

  int zmapfd (int, char **, const char *);

  /* Read LEN bytes from FD into BUF.  Retry the read on EINTR, up to three
     interrupts.  Any other error causes the loop to break. */

#ifdef NUM_INTR
#  undef NUM_INTR
#endif
#define NUM_INTR 3

  static ssize_t
  zreadretry (int fd, char *buf, size_t len)
  {
    for (int nintr = 0; ; )
      {
	ssize_t r = ::read (fd, buf, len);
	if (r >= 0)
	  return r;
	if (r == -1 && errno == EINTR)
	  {
	    if (++nintr >= NUM_INTR)
	      return -1;
	    continue;
	  }
	return r;
      }
  }

#undef NUM_INTR

  /* Call read(2) and allow it to be interrupted.  Just a stub for now. */
  ssize_t
  zreadintr (int fd, char *buf, size_t len)
  {
    check_signals ();
    return ::read (fd, buf, len);
  }

  void
  zreset () {
    zread_lind = zread_lused = 0;
  }

  /* Sync the seek pointer for FD so that the kernel's idea of the last char
     read is the last char returned by zreadc. */
  void
  zsyncfd (int fd)
  {
    off_t off, r;

    off = zread_lused - zread_lind;
    r = 0;
    if (off > 0)
      r = ::lseek (fd, -off, SEEK_CUR);

    if (r != -1)
      zread_lused = zread_lind = 0;
  }

  ssize_t zreadc (int, char *);
  ssize_t zreadcintr (int, char *);
  ssize_t zreadn (int, char *, size_t);

  /* function moved from lib/sh/zwrite.c */

  /* Write NB bytes from BUF to file descriptor FD, retrying the write if
     it is interrupted.  We retry three times if we get a zero-length
     write.  Any other signal causes this function to return prematurely. */
  static inline ssize_t
  zwrite (int fd, char *buf, size_t nb)
  {
    size_t n, nt;

    for (n = nb, nt = 0;;)
      {
	ssize_t i = ::write (fd, buf, n);
	if (i > 0)
	  {
	    n -= static_cast<size_t> (i);
	    if (n <= 0)
	      return static_cast<ssize_t> (nb);
	    buf += i;
	  }
	else if (i == 0)
	  {
	    if (++nt > 3)
	      return static_cast<ssize_t> (nb - n);
	  }
	else if (errno != EINTR)
	  return -1;
      }
  }

  /* from bashhist.c */
#if defined (BANG_HISTORY)
  bool bash_history_inhibit_expansion (const char *, int);
#endif

  /* Functions declared in execute_cmd.c, used by many other files */

  void
  close_fd_bitmap (std::vector<bool> *fdbp)
  {
    if (fdbp)
      {
	std::vector<bool>::iterator it = fdbp->begin ();
	while ((it = std::find (it, fdbp->end (), true)) != fdbp->end ())
	  {
	    int i = static_cast<int> (it - fdbp->begin ());
	    ::close (i);
	  }
	fdbp->clear ();
      }
  }

  int executing_line_number ();
  int execute_command (COMMAND *);
  int execute_command_internal (COMMAND *, bool, int, int, struct fd_bitmap *);
  int shell_execve (char *, char **, char **);
  void setup_async_signals ();
  void async_redirect_stdin ();

  void undo_partial_redirects ();
  void dispose_partial_redirects ();
  void dispose_exec_redirects ();

  int execute_shell_function (SHELL_VAR *, WORD_LIST *);

  Coproc *getcoprocbypid (pid_t);
  Coproc *getcoprocbyname (const char *);

  void coproc_init (Coproc *);
  Coproc *coproc_alloc (char *, pid_t);
  void coproc_dispose (Coproc *);
  void coproc_flush ();
  void coproc_close (Coproc *);
  void coproc_closeall ();
  void coproc_reap ();
  pid_t coproc_active ();

  void coproc_rclose (Coproc *, int);
  void coproc_wclose (Coproc *, int);
  void coproc_fdclose (Coproc *, int);

  void coproc_checkfd (Coproc *, int);
  void coproc_fdchk (int);

  void coproc_pidchk (pid_t, int);

  void coproc_fdsave (Coproc *);
  void coproc_fdrestore (Coproc *);

  void coproc_setvars (Coproc *);
  void coproc_unsetvars (Coproc *);

#if defined (PROCESS_SUBSTITUTION)
  void close_all_files ();
#endif

#if defined (ARRAY_VARS)
  void restore_funcarray_state (void *);
#endif

  /* Functions from mailcheck.c */
  int time_to_check_mail ();
  void reset_mail_timer ();
  void reset_mail_files ();
  void free_mail_files ();
  char *make_default_mailpath ();
  void remember_mail_dates ();
  void init_mail_dates ();
  void check_mail ();

  /* Functions from redir.c */

  /* Values for flags argument to do_redirections */
  enum redir_flags {
    RX_ACTIVE =		0x01,	/* do it; don't just go through the motions */
    RX_UNDOABLE =	0x02,	/* make a list to undo these redirections */
    RX_CLEXEC =		0x04,	/* set close-on-exec for opened fds > 2 */
    RX_INTERNAL =	0x08,
    RX_USER =		0x10,
    RX_SAVCLEXEC =	0x20,	/* set close-on-exec off in restored fd even though saved on has it on */
    RX_SAVEFD =		0x40	/* fd used to save another even if < SHELL_FD_BASE */
  };

  void redirection_error (REDIRECT *, int, const char *);
  int do_redirections (REDIRECT *, redir_flags);
  char *redirection_expand (WORD_DESC *);
  int stdin_redirects (REDIRECT *);

  /* Functions from trap.c */
  void check_signals ();
  void check_signals_and_traps ();

  /* ************************************************************** */
  /*		Private Shell Variables (ptr types)		    */
  /* ************************************************************** */

//   int subshell_argc;
//   char **subshell_argv;
//   char **subshell_envp;

  /* The current locale when the program begins */
  char *default_locale;

  /* The current domain for textdomain(3). */
  char *default_domain;
  char *default_dir;

  /* tracks the value of LC_ALL; used to override values for other locale
     categories */
  char *lc_all;

  /* tracks the value of LC_ALL; used to provide defaults for locale
     categories */
  char *lang;

  COMMAND *global_command;

  /* Information about the current user. */
  UserInfo current_user;

  /* The current host's name. */
  const char *current_host_name;

  /* The environment that the shell passes to other commands. */
  char **shell_environment;

  /* The name of this shell, as taken from argv[0]. */
  const char *shell_name;

  /* The name of the .(shell)rc file. */
  const char *bashrc_file;

  /* These are extern so execute_simple_command can set them, and then
     throw back to main to execute a shell script, instead of calling
     main () again and resulting in indefinite, possibly fatal, stack
     growth. */

  char **subshell_argv;
  char **subshell_envp;

  char *exec_argv0;

  const char *command_execution_string;	/* argument to -c option */
  const char *shell_script_filename; 	/* shell script */

  FILE *default_input;

  StringIntAlist *shopt_alist;

  // definitions from subst.h

  /* Remove backslashes which are quoting backquotes from STRING.  Modifies
   STRING. */
  void de_backslash (char *);

  /* Replace instances of \! in a string with !. */
  void unquote_bang (char *);

  /* Extract the $( construct in STRING, and return a new string.
     Start extracting at (SINDEX) as if we had just seen "$(".
     Make (SINDEX) get the position just after the matching ")".
     XFLAGS is additional flags to pass to other extraction functions, */
  char *extract_command_subst (const char *, size_t *, sx_flags);

  /* Extract the $[ construct in STRING, and return a new string.
     Start extracting at (SINDEX) as if we had just seen "$[".
     Make (SINDEX) get the position just after the matching "]". */
  inline char *
  extract_arithmetic_subst (const char *string, size_t *sindex)
  {
    return extract_delimited_string (string, sindex, "$[", "[", "]", SX_NOFLAGS); /*]*/
  }

#if defined (PROCESS_SUBSTITUTION)
  /* Extract the <( or >( construct in STRING, and return a new string.
     Start extracting at (SINDEX) as if we had just seen "<(".
     Make (SINDEX) get the position just after the matching ")". */
  inline char *
  extract_process_subst (const char *string, size_t *sindex, sx_flags xflags)
  {
    xflags |= (no_throw_on_fatal_error ? SX_NOTHROW : SX_NOFLAGS);
    return xparse_dolparen (string, const_cast<char *> (string + *sindex), sindex, xflags);
  }
#endif /* PROCESS_SUBSTITUTION */

  /* Return a single string of all the words present in LIST, separating
     each word with SEP. */
  char *string_list_internal (const WORD_LIST *, const char *);

  /* Return a single string of all the words present in LIST, separating
     each word with a space. */
  char *string_list (const WORD_LIST *);

  /* Turn $* into a single string, obeying POSIX rules. */
  char *string_list_dollar_star (const WORD_LIST *, int, int);

  /* Expand $@ into a single string, obeying POSIX rules. */
  char *string_list_dollar_at (WORD_LIST *, int, int);

  /* Turn the positional parameters into a string, understanding quoting and
     the various subtleties of using the first character of $IFS as the
     separator.  Calls string_list_dollar_at, string_list_dollar_star, and
     string_list as appropriate. */
  char *string_list_pos_params (char, WORD_LIST *, quoted_flags, param_flags);

  /* Perform quoted null character removal on each element of LIST.
     This modifies LIST. */
  void word_list_remove_quoted_nulls (WORD_LIST *);

  /* This performs word splitting and quoted null character removal on
     STRING. */
  WORD_LIST *list_string (const char *, const char *, quoted_flags);

  char *ifs_firstchar (int *);
  char *get_word_from_string (char **, const char *, char **);
  char *strip_trailing_ifs_whitespace (char *, const char *, bool);

  /* Given STRING, an assignment string, get the value of the right side
     of the `=', and bind it to the left side.  If EXPAND is true, then
     perform tilde expansion, parameter expansion, command substitution,
     and arithmetic expansion on the right-hand side.  Do not perform word
     splitting on the result of expansion. */
  int do_assignment (char *);
  int do_assignment_no_expand (char *);
  int do_word_assignment (WORD_DESC *, int);

  /* Append SOURCE to TARGET at INDEX.  SIZE is the current amount
     of space allocated to TARGET.  SOURCE can be NULL, in which
     case nothing happens.  Gets rid of SOURCE by free ()ing it.
     Returns TARGET in case the location has changed. */
  char *sub_append_string (char *, char *, int *, size_t *);

  /* Return the word list that corresponds to `$*'. */
  WORD_LIST *list_rest_of_args ();

  /* Expand STRING by performing parameter expansion, command substitution,
     and arithmetic expansion.  Dequote the resulting WORD_LIST before
     returning it, but do not perform word splitting.  The call to
     remove_quoted_nulls () is made here because word splitting normally
     takes care of quote removal. */
  WORD_LIST *expand_string_unsplit (const char *, int);

  /* Expand the rhs of an assignment statement. */
  WORD_LIST *expand_string_assignment (const char *, int);

  /* Expand a prompt string. */
  WORD_LIST *expand_prompt_string (const char *, int, int);

  /* Expand STRING just as if you were expanding a word.  This also returns
     a list of words.  Note that filename globbing is *NOT* done for word
     or string expansion, just when the shell is expanding a command.  This
     does parameter expansion, command substitution, arithmetic expansion,
     and word splitting.  Dequote the resultant WORD_LIST before returning. */
  WORD_LIST *expand_string (const char *, int);

  /* Convenience functions that expand strings to strings, taking care of
     converting the WORD_LIST * returned by the expand_string* functions
     to a string and deallocating the WORD_LIST *. */
  char *expand_string_to_string (const char *, int);
  char *expand_string_unsplit_to_string (const char *, int);
  char *expand_assignment_string_to_string (const char *, int);

  /* Expand an arithmetic expression string */
  char *expand_arith_string (const char *, int);

  /* De-quote quoted characters in STRING. */
  char *dequote_string (const char *);

  /* De-quote CTLESC-escaped CTLESC or CTLNUL characters in STRING. */
  char *dequote_escapes (const char *);

  WORD_DESC *dequote_word (WORD_DESC *);

  /* De-quote quoted characters in each word in LIST. */
  WORD_LIST *dequote_list (WORD_LIST *);

  /* Expand WORD, performing word splitting on the result.  This does
     parameter expansion, command substitution, arithmetic expansion,
     word splitting, and quote removal. */
  WORD_LIST *expand_word (WORD_DESC *, int);

  /* Expand WORD, but do not perform word splitting on the result.  This
     does parameter expansion, command substitution, arithmetic expansion,
     and quote removal. */
  WORD_LIST *expand_word_unsplit (WORD_DESC *, int);
  WORD_LIST *expand_word_leave_quoted (WORD_DESC *, int);

  /* Return the value of a positional parameter.  This handles values > 10. */
  char *get_dollar_var_value (intmax_t);

  /* Quote a string to protect it from word splitting. */
  char *quote_string (const char *);

  /* Quote escape characters (characters special to internals of expansion)
     in a string. */
  char *quote_escapes (const char *);

  /* And remove such quoted special characters. */
  char *remove_quoted_escapes (char *);

  /* Remove CTLNUL characters from STRING unless they are quoted with CTLESC. */
  char *remove_quoted_nulls (char *);

  /* Perform quote removal on STRING.  If QUOTED > 0, assume we are obeying the
     backslash quoting rules for within double quotes. */
  char *string_quote_removal (const char *, int);

  /* Perform quote removal on word WORD.  This allocates and returns a new
     WORD_DESC *. */
  WORD_DESC *word_quote_removal (WORD_DESC *, int);

  /* Perform quote removal on all words in LIST.  If QUOTED is non-zero,
     the members of the list are treated as if they are surrounded by
     double quotes.  Return a new list, or NULL if LIST is NULL. */
  WORD_LIST *word_list_quote_removal (WORD_LIST *, int);

  /* Called when IFS is changed to maintain some private variables. */
  void setifs (SHELL_VAR *);

  /* Return the value of $IFS, or " \t\n" if IFS is unset. */
  const char *getifs ();

  /* This splits a single word into a WORD LIST on $IFS, but only if the word
     is not quoted.  list_string () performs quote removal for us, even if we
     don't do any splitting. */
  WORD_LIST *word_split (WORD_DESC *, const char *);

  /* Take the list of words in LIST and do the various substitutions.  Return
     a new list of words which is the expanded list, and without things like
     variable assignments. */
  WORD_LIST *expand_words (WORD_LIST *);

  /* Same as expand_words (), but doesn't hack variable or environment
     variables. */
  WORD_LIST *expand_words_no_vars (WORD_LIST *);

  /* Perform the `normal shell expansions' on a WORD_LIST.  These are
     brace expansion, tilde expansion, parameter and variable substitution,
     command substitution, arithmetic expansion, and word splitting. */
  WORD_LIST *expand_words_shellexp (WORD_LIST *);

  WORD_DESC *command_substitute (char *, int, int);

  char *pat_subst (const char *, const char *, const char *, int);

#if defined (PROCESS_SUBSTITUTION)
  int fifos_pending ();
  int num_fifos ();
  void unlink_fifo_list ();
  void unlink_all_fifos ();
  void unlink_fifo (int);

  void *copy_fifo_list (int *);
  void close_new_fifos (void *, int);

  void clear_fifo_list ();

  int find_procsub_child (pid_t);
  void set_procsub_status (int, pid_t, int);

  void wait_procsubs ();
  void reap_procsubs ();
#endif

#if defined (ARRAY_VARS)
  char *extract_array_assignment_list (const char *, int *);
#endif

#if defined (COND_COMMAND)
  char *remove_backslashes (const char *);
  char *cond_expand_word (WORD_DESC *, int);
#endif

  size_t
  skip_to_delim (const char *, size_t, const char *, sd_flags);

#if defined (BANG_HISTORY)
  size_t
  skip_to_histexp (const char *, size_t, const char *, sd_flags);
#endif

#if defined (READLINE)
  unsigned int char_is_quoted (const std::string &, int);	/* rl_linebuf_func_t */
  bool unclosed_pair (const char *, int, const char *);
  WORD_LIST *split_at_delims (const char *, int, const char *, int, sd_flags, int *, int *);
#endif

  void invalidate_cached_quoted_dollar_at ();

  // private methods from subst.c

  char *
  string_extract (const char *, size_t *, const char *, sx_flags);

  char *
  string_extract_double_quoted (const char *, size_t *, sx_flags);

  size_t
  skip_double_quoted (const char *, size_t, size_t, sx_flags);

  char *
  string_extract_single_quoted (const char *, size_t *);

  size_t
  skip_single_quoted (const char *, size_t, size_t, sx_flags);

  char *
  string_extract_verbatim (const char *, size_t, size_t *, const char *, sx_flags);

  char *
  extract_delimited_string (const char *, size_t *, const char *,
			    const char *, const char *, sx_flags);

  char *
  extract_dollar_brace_string (const char *, size_t *, quoted_flags, sx_flags);

  size_t
  skip_matched_pair (const char *, size_t, char, char, valid_array_flags);

  void
  exp_throw_to_top_level (const std::exception &);

#if defined (ARRAY_VARS)
  /* Flags has 1 as a reserved value, since skip_matched_pair uses it for
     skipping over quoted strings and taking the first instance of the
     closing character. */
  size_t
  skipsubscript (const char *string, size_t start, valid_array_flags flags)
  {
    return skip_matched_pair (string, start, '[', ']', flags);
  }
#endif

  /* Evaluates to true if C is a character in $IFS. */
  inline bool isifs (char c) {
    return ifs_cmap[static_cast<unsigned char> (c)];
  }

  SHELL_VAR *
  do_compound_assignment (const char *, const char *, assign_flags);

  // from variables.c

  void validate_inherited_value (SHELL_VAR *, int);

  SHELL_VAR *set_if_not (const char *, const char *);

  virtual void sh_set_lines_and_columns (unsigned int, unsigned int) RL_OVERRIDE;
  void set_pwd ();
  void set_ppid ();
  void make_funcname_visible (bool);

  SHELL_VAR *var_lookup (const char *, VAR_CONTEXT *);

  SHELL_VAR *find_function_var (const char *);
  FUNCTION_DEF *find_function_def (const char *);
  SHELL_VAR *find_variable (const char *);
  SHELL_VAR *find_variable_noref (const char *);
  SHELL_VAR *find_var_last_nameref (const char *, int);
  SHELL_VAR *find_global_var_last_nameref (const char *, int);
  SHELL_VAR *find_var_nameref (SHELL_VAR *);
  SHELL_VAR *find_var_nameref_for_create (const char *, int);
  SHELL_VAR *find_var_nameref_for_assignment (const char *, int);
/* SHELL_VAR *find_internal (const char *, int); */
  SHELL_VAR *find_tempenv (const char *);
  SHELL_VAR *find_no_tempenv (const char *);
  SHELL_VAR *find_global (const char *);
  SHELL_VAR *find_global_noref (const char *);
  SHELL_VAR *find_shell (const char *);
  SHELL_VAR *find_no_invisible (const char *);
  SHELL_VAR *find_for_assignment (const char *);
  char *nameref_transform_name (const char *, int);
//   SHELL_VAR *copy_variable (SHELL_VAR *);
  SHELL_VAR *make_local (const char *, int);
  SHELL_VAR *bind (const char *, const char *, int);
  SHELL_VAR *bind_global (const char *, const char *, int);
  SHELL_VAR *bind_function (const char *, COMMAND *);

  void bind_function_def (const char *, FUNCTION_DEF *, int);

  SHELL_VAR **map_over (sh_var_map_func_t *, VAR_CONTEXT *);
  SHELL_VAR **map_over_funcs (sh_var_map_func_t *);

  SHELL_VAR **all_shell ();
  SHELL_VAR **all_shell_functions ();
  SHELL_VAR **all_visible ();
  SHELL_VAR **all_visible_functions ();
  SHELL_VAR **all_exported ();
  SHELL_VAR **local_exported ();
  SHELL_VAR **all_local (int);
#if defined (ARRAY_VARS)
  SHELL_VAR **all_array ();
#endif
  char **all_matching_prefix (const char *);

  char **make_var_array (HASH_TABLE *);
  char **add_or_supercede_exported_var (const char *, int);

  char *get_variable_value (SHELL_VAR *);
  char *get_string_value (const char *);
  char *sh_get_env_value (const char *) RL_OVERRIDE;
  char *make_variable_value (SHELL_VAR *, const char *, int);

  /* These three are virtual callbacks when Readline is used. */
  char *sh_single_quote (const char *) RL_OVERRIDE;
  char *sh_get_home_dir () RL_OVERRIDE;
  int sh_unset_nodelay_mode (int) RL_OVERRIDE;

  SHELL_VAR *bind_variable_value (SHELL_VAR *, const char *, int);
  SHELL_VAR *bind_int_variable (const char *, const char *, assign_flags);
  SHELL_VAR *bind_var_to_int (const char *, intmax_t);

  int assign_in_env (WORD_DESC *, int);

  int unbind_variable (const char *);
  int check_unbind_variable (const char *);
  int unbind_nameref (const char *);
  int unbind_variable_noref (const char *);
  int unbind_func (const char *);
  int unbind_function_def (const char *);
  int delete_var (const char *, VAR_CONTEXT *);
  int makunbound (const char *, VAR_CONTEXT *);
  int kill_local_var (const char *);
  void delete_all_vars (HASH_TABLE *);
  void delete_all_contexts (VAR_CONTEXT *);

  VAR_CONTEXT *new_var_context (const char *, int);
  void dispose_var_context (VAR_CONTEXT *);
  VAR_CONTEXT *push_var_context (const char *, int, HASH_TABLE *);
  void pop_var_context ();
  VAR_CONTEXT *push_scope (int, HASH_TABLE *);
  int pop_scope (int);

  void clear_dollar_vars ();

  void push_context (const char *, bool, HASH_TABLE *);
  void pop_context ();
  void push_dollar_vars ();
  void pop_dollar_vars ();
  void dispose_saved_dollar_vars ();

  void init_bash_argv ();
  void save_bash_argv ();
  void push_args (WORD_LIST *);
  void pop_args ();

  void adjust_shell_level (int);
  void non_unsettable (const char *);
  void dispose_variable (void *);
  void dispose_used_env_vars ();
  void dispose_function_env ();
  void dispose_builtin_env ();
  void merge_temporary_env ();
  void flush_temporary_env ();
  void merge_builtin_env ();
  void kill_all_local_variables ();

  void set_var_read_only (const char *);
  void set_func_read_only (const char *);
  void set_var_auto_export (const char *);
  void set_func_auto_export (const char *);

  void sort_variables (SHELL_VAR **);

  int chkexport (const char *);
  void maybe_make_export_env ();
  void update_export_env_inplace (const char *, int, const char *);
  void put_command_name_into_env (const char *);
  void put_gnu_argv_flags_into_env (intmax_t, const char *);

  void print_var_list (SHELL_VAR **);
  void print_func_list (SHELL_VAR **);
  void print_assignment (SHELL_VAR *);
  void print_var_value (SHELL_VAR *, int);
  void print_var_function (SHELL_VAR *);

#if defined (ARRAY_VARS)
  SHELL_VAR *make_new_array_variable (const char *);
  SHELL_VAR *make_local_array_variable (const char *, int);

  SHELL_VAR *make_new_assoc_variable (const char *);
  SHELL_VAR *make_local_assoc_variable (const char *, int);

  void set_pipestatus_array (int *, int);
  ARRAY *save_pipestatus_array ();
  void restore_pipestatus_array (ARRAY *);
#endif

  void set_pipestatus_from_exit (int);

  /* The variable in NAME has just had its state changed.  Check to see if it
     is one of the special ones where something special happens. */
  void stupidly_hack_special_vars (const char *);

  /* Reinitialize some special variables that have external effects upon unset
     when the shell reinitializes itself. */
  void reinit_special_vars ();

  int get_random_number ();

/* The `special variable' functions that get called when a particular
   variable is set. */
  void sv_ifs (const char *);
  void sv_path (const char *);
  void sv_mail (const char *);
  void sv_funcnest (const char *);
  void sv_execignore (const char *);
  void sv_globignore (const char *);
  void sv_ignoreeof (const char *);
  void sv_strict_posix (const char *);
  void sv_optind (const char *);
  void sv_opterr (const char *);
  void sv_locale (const char *);
  void sv_xtracefd (const char *);
  void sv_shcompat (const char *);

#if defined (READLINE)
  void sv_comp_wordbreaks (const char *);
  void sv_terminal (const char *);
  void sv_hostfile (const char *);
  void sv_winsize (const char *);
#endif

#if defined (__CYGWIN__)
  void sv_home (const char *);
#endif

#if defined (HISTORY)
  void sv_histsize (const char *);
  void sv_histignore (const char *);
  void sv_history_control (const char *);
#  if defined (BANG_HISTORY)
  void sv_histchars (const char *);
#  endif
  void sv_histtimefmt (const char *);
#endif /* HISTORY */

#if defined (HAVE_TZSET)
  void sv_tz (const char *);
#endif

#if defined (JOB_CONTROL)
  void sv_childmax (const char *);
#endif

  // Methods that were previously static in variables.c.
  void create_variable_tables ();

  /* The variable in NAME has just had its state changed.  Check to see if it
     is one of the special ones where something special happens. */
  void stupidly_hack_special_variables (const char *);

  // Methods in parse.y / y.tab.c.
  char *xparse_dolparen (const char *, char *, size_t *, sx_flags);

  // Methods in lib/tilde/tilde.c.
  size_t tilde_find_prefix (const char *, size_t *);
  char *tilde_expand_word (const char *);
  size_t tilde_find_suffix (const char *);
  char *tilde_expand (const char *);

private:

  // Here are the variables we can't trivially send to a different memory space.

  jobstats js;

  PROCESS *last_procsub_child;

//  ps_index_t *pidstat_table;	// FIXME: what type is this

  bgpids bgpids;

  procchain procsubs;

  /* The array of known jobs. */
  std::vector<JOB*> jobs;

  /* The pipeline currently being built. */
  PROCESS *the_pipeline;

#if defined (ARRAY_VARS)
  int *pstatuses;		/* list of pipeline statuses */
  size_t statsize;
#endif

  VAR_CONTEXT *global_variables;
  VAR_CONTEXT *shell_variables;

  HASH_TABLE *shell_functions;
  HASH_TABLE *temporary_env;

  char **dollar_vars;
  char **export_env;

  /* Special value for nameref with invalid value for creation or assignment. */
  SHELL_VAR invalid_nameref_value;

  /* variables from common.c */
  sh_builtin_func_t this_shell_builtin;
  sh_builtin_func_t last_shell_builtin;

  WORD_LIST *rest_of_args;

  char *the_current_working_directory;

  char *the_printed_command_except_trap;

  const char *this_command_name;
  SHELL_VAR *this_shell_function;

  char_flags *sh_syntaxtab;

  char *list_optarg;

  WORD_LIST *lcurrent;
  WORD_LIST *loptend;

  /* The arithmetic expression evaluator's stack of expression contexts. */
  std::vector<EXPR_CONTEXT *> expr_stack;

  /* The current context for the expression being evaluated. */
  EXPR_CONTEXT expr_current;

  lvalue lastlval;

  /* private variables from subst.c */

  /* Variables used to keep track of the characters in IFS. */
  SHELL_VAR *ifs_var;
  const char *ifs_value;

  /* Used to hold a list of variable assignments preceding a command.  Global
     so the SIGCHLD handler in jobs.c can unwind-protect it when it runs a
     SIGCHLD trap and so it can be saved and restored by the trap handlers. */
  WORD_LIST *subst_assign_varlist;

  WORD_LIST *cached_quoted_dollar_at;

  /* A WORD_LIST of words to be expanded by expand_word_list_internal,
     without any leading variable assignments. */
  WORD_LIST *garglist;

  /* variables from lib/sh/shtty.c */

  TTYSTRUCT ttin, ttout;

  /* local buffer for lib/sh/zread.c functions */
  char *zread_lbuf;

  /* moved from lib/tilde/tilde.c */

  /* If non-null, this contains the address of a function that the application
     wants called before trying the standard tilde expansions.  The function
     is called with the text sans tilde, and returns a malloc()'ed string
     which is the expansion, or a NULL pointer if the expansion fails. */
  tilde_hook_func_t tilde_expansion_preexpansion_hook;

  /* If non-null, this contains the address of a function to call if the
     standard meaning for expanding a tilde fails.  The function is called
     with the text (sans tilde, as in "foo"), and returns a malloc()'ed string
     which is the expansion, or a NULL pointer if there is no expansion. */
  tilde_hook_func_t tilde_expansion_failure_hook;

  /* When non-null, this is a nullptr-terminated array of strings which
     are duplicates for a tilde prefix.  Bash uses this to expand
     `=~' and `:~'. */
  char **tilde_additional_prefixes;

  /* When non-null, this is a nullptr-terminated array of strings which match
     the end of a username, instead of just "/".  Bash sets this to
     `:' and `=~'. */
  char **tilde_additional_suffixes;

#if defined (HAVE_ICONV)
  iconv_t localconv;
#endif

  // 32-bit variables here (before I move them to SimpleState)

  unsigned int zread_lind;	// read index in zread_lbuf
  unsigned int zread_lused;	// bytes used in zread_lbuf

  int list_optopt;
  int list_opttype;
};

// The global (constructed in shell.cc) pointer to the single shell object.
extern Shell *the_shell;

static constexpr int HEREDOC_MAX = 16;

struct ShellInputLineState {
  char *input_line;
  size_t input_line_index;
  size_t input_line_size;
  size_t input_line_len;
#if defined (HANDLE_MULTIBYTE)
  char *input_property;
  size_t input_propsize;
#endif
};

/* Structure in which to save partial parsing state when doing things like
   PROMPT_COMMAND and bash_execute_unix_command execution. */

struct ShellParserState {

  /* parsing state */
  int parser_state;
  int *token_state;

  char *token;
  int token_buffer_size;

  /* input line state -- line number saved elsewhere */
  int input_line_terminator;
  int eof_encountered;

#if defined (HANDLE_MULTIBYTE)
  /* Nothing right now for multibyte state, but might want something later. */
#endif

  const char **prompt_string_pointer;

  /* history state affecting or modified by the parser */
  int current_command_line_count;
#if defined (HISTORY)
  int remember_on_history;
  int history_expansion_inhibited;
#endif

  /* execution state possibly modified by the parser */
  int last_command_exit_value;
#if defined (ARRAY_VARS)
  ARRAY *pipestatus;
#endif

  Shell::sh_builtin_func_t last_shell_builtin;
  Shell::sh_builtin_func_t this_shell_builtin;

  /* flags state affecting the parser */
  int expand_aliases;
  int echo_input_at_read;
  int need_here_doc;
  int here_doc_first_line;

  /* structures affecting the parser */
  REDIRECT *redir_stack[HEREDOC_MAX];

  /* Let's try declaring these here. */
  const char *parser_remaining_input ();

  ShellParserState *save_parser_state (ShellParserState *);
  void restore_parser_state (ShellParserState *);

  ShellInputLineState *save_input_line_state (ShellInputLineState *);
  void ShellInputLineState (ShellInputLineState *);
};

}  // namespace bash

#endif /* _SHELL_H_ */
