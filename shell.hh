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

#if !defined(_SHELL_H_)
#define _SHELL_H_

#include "config.hh"

#include "bashtypes.hh"
#include "chartypes.hh"

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <vector>

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"
#include "posixstat.hh"

#include "alias.hh"
#include "builtins.hh"
#include "command.hh"
#include "externs.hh"
#include "general.hh"
#include "jobs.hh"
#include "maxpath.hh"
#include "parser.hh"
#include "pathnames.hh"
#include "quit.hh"
#include "shtty.hh"
#include "sig.hh"
#include "subst.hh"
#include "syntax.hh"
#include "trap.hh"
#include "variables.hh"

#include "builtins/common.hh"

#ifdef DEBUG
#define YYDEBUG 1
#else
#define YYDEBUG 0
#endif

// include the generated Bison C++ header after suppressing some warnings
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wsuggest-destructor-override"
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

#include "parse.hh"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if defined(HISTORY)
#include "history.hh"
#endif

#if defined(READLINE)
#include "readline.hh"
#else
#include "tilde.hh"
#endif

#if defined(HAVE_ICONV)
#include <iconv.h>
#endif

namespace bash
{

/* Flag values for history_control */
enum hist_ctl_flags
{
  HC_IGNSPACE = 0x01,
  HC_IGNDUPS = 0x02,
  HC_ERASEDUPS = 0x04,

  HC_IGNBOTH = (HC_IGNSPACE | HC_IGNDUPS)
};

#if defined(STRICT_POSIX)
#undef HISTEXPAND_DEFAULT
#define HISTEXPAND_DEFAULT 0
#else
#if !defined(HISTEXPAND_DEFAULT)
#define HISTEXPAND_DEFAULT 1
#endif /* !HISTEXPAND_DEFAULT */
#endif

#define HEREDOC_MAX 16

#define NUM_POSIX_VARS 5

constexpr int NO_PIPE = -1;
constexpr int REDIRECT_BOTH = -2;

constexpr int NO_VARIABLE = -1;

/* Values that can be returned by execute_command (). */
constexpr int EXECUTION_FAILURE = 1;
constexpr int EXECUTION_SUCCESS = 0;

/* Usage messages by builtins result in a return status of 2. */
constexpr int EX_BADUSAGE = 2;

constexpr int EX_MISCERROR = 2;

/* Special exit statuses used by the shell, internally and externally. */
constexpr int EX_RETRYFAIL = 124;
constexpr int EX_WEXPCOMSUB = 125;
constexpr int EX_BINARY_FILE = 126;
constexpr int EX_NOEXEC = 126;
constexpr int EX_NOINPUT = 126;
constexpr int EX_NOTFOUND = 127;

constexpr int EX_SHERRBASE = 256; /* all special error values are > this. */

constexpr int EX_BADSYNTAX = 257; /* shell syntax error */
constexpr int EX_USAGE = 258;     /* syntax error in usage */
constexpr int EX_REDIRFAIL = 259; /* redirection failed */
constexpr int EX_BADASSIGN = 260; /* variable assignment error */
constexpr int EX_EXPFAIL = 261;   /* word expansion failed */
constexpr int EX_DISKFALLBACK
    = 262; /* fall back to disk command from builtin */

/* Flag values that control parameter pattern substitution. */
enum match_flags
{
  MATCH_ANY = 0x000,
  MATCH_BEG = 0x001,
  MATCH_END = 0x002,

  MATCH_TYPEMASK = 0x003,

  MATCH_GLOBREP = 0x010,
  MATCH_QUOTED = 0x020,
  MATCH_ASSIGNRHS = 0x040,
  MATCH_STARSUB = 0x080
};

/* Flags for skip_to_delim */

enum sd_flags
{
  SD_NOTHROW = 0x001, /* don't throw exception on fatal error. */
  SD_INVERT = 0x002,  /* look for chars NOT in passed set */
  SD_NOQUOTEDELIM
  = 0x004, /* don't let single or double quotes act as delimiters */
  SD_NOSKIPCMD = 0x008, /* don't skip over $(, <(, or >( command/process
                           substitution; parse them as commands */
  SD_EXTGLOB = 0x010, /* skip over extended globbing patterns if appropriate */
  SD_IGNOREQUOTE = 0x020, /* single and double quotes are not special */
  SD_GLOB = 0x040,      /* skip over glob patterns like bracket expressions */
  SD_NOPROCSUB = 0x080, /* don't parse process substitutions as commands */
  SD_COMPLETE = 0x100,  /* skip_to_delim during completion */
  SD_HISTEXP = 0x200,   /* skip_to_delim during history expansion */
  SD_ARITHEXP = 0x400   /* skip_to_delim during arithmetic expansion */
};

static inline sd_flags
operator| (const sd_flags &a, const sd_flags &b)
{
  return static_cast<sd_flags> (static_cast<uint32_t> (a)
                                | static_cast<uint32_t> (b));
}

/* Information about the current user. */
struct UserInfo
{
  UserInfo ()
      : uid (static_cast<uid_t> (-1)), euid (static_cast<uid_t> (-1)),
        gid (static_cast<gid_t> (-1)), egid (static_cast<gid_t> (-1))
  {
  }

  uid_t uid;
  uid_t euid;
  gid_t gid;
  gid_t egid;
  char *user_name;
  char *shell; /* shell from the password file */
  char *home_dir;
};

/* Definitions moved from expr.cc to simplify definition of Shell class. */

/* The Tokens.  Singing "The Lion Sleeps Tonight". */

enum token_t
{
  NONE = 0,
  EQEQ = 1,       /* "==" */
  NEQ = 2,        /* "!=" */
  LEQ = 3,        /* "<=" */
  GEQ = 4,        /* ">=" */
  STR = 5,        /* string */
  NUM = 6,        /* number */
  LAND = 7,       /* "&&" Logical AND */
  LOR = 8,        /* "||" Logical OR */
  LSH = 9,        /* "<<" Left SHift */
  RSH = 10,       /* ">>" Right SHift */
  OP_ASSIGN = 11, /* op= expassign as in Posix.2 */
  COND = 12,      /* exp1 ? exp2 : exp3 */
  POWER = 13,     /* exp1**exp2 */
  PREINC = 14,    /* ++var */
  PREDEC = 15,    /* --var */
  POSTINC = 16,   /* var++ */
  POSTDEC = 17,   /* var-- */
  EQ = '=',
  GT = '>',
  LT = '<',
  PLUS = '+',
  MINUS = '-',
  MUL = '*',
  DIV = '/',
  MOD = '%',
  NOT = '!',
  LPAR = '(',
  RPAR = ')',
  BAND = '&', /* Bitwise AND */
  BOR = '|',  /* Bitwise OR. */
  BXOR = '^', /* Bitwise eXclusive OR. */
  BNOT = '~', /* Bitwise NOT; Two's complement. */
  QUES = '?',
  COL = ':',
  COMMA = ','
};

struct lvalue
{
  std::string tokstr; /* possibly-rewritten lvalue if not nullptr */
  int64_t tokval;     /* expression evaluated value */
  SHELL_VAR *tokvar;  /* variable described by array or var reference */
  int64_t ind;        /* array index if not -1 */

  void
  init ()
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
  int64_t tokval;
  struct lvalue lval;
  token_t curtok, lasttok;
  int noeval;
};

/* Flags which describe the current handling state of a signal. */
enum sigmode_t
{
  SIG_INHERITED = 0x0,   /* Value inherited from parent. */
  SIG_TRAPPED = 0x1,     /* Currently trapped. */
  SIG_HARD_IGNORE = 0x2, /* Signal was ignored on shell entry. */
  SIG_SPECIAL = 0x4,     /* Treat this signal specially. */
  SIG_NO_TRAP = 0x8,     /* Signal cannot be trapped. */
  SIG_INPROGRESS = 0x10, /* Signal handler currently executing. */
  SIG_CHANGED = 0x20,    /* Trap value changed in trap handler. */
  SIG_IGNORED = 0x40     /* The signal is currently being ignored. */
};

static inline sigmode_t &
operator|= (sigmode_t &a, const sigmode_t &b)
{
  a = static_cast<sigmode_t> (static_cast<uint32_t> (a)
                              | static_cast<uint32_t> (b));
  return a;
}

static inline sigmode_t
operator| (const sigmode_t &a, const sigmode_t &b)
{
  return static_cast<sigmode_t> (static_cast<uint32_t> (a)
                                 | static_cast<uint32_t> (b));
}

static inline sigmode_t &
operator&= (sigmode_t &a, const sigmode_t &b)
{
  a = static_cast<sigmode_t> (static_cast<uint32_t> (a)
                              & static_cast<uint32_t> (b));
  return a;
}

static inline sigmode_t
operator~(const sigmode_t &a)
{
  return static_cast<sigmode_t> (~static_cast<uint32_t> (a));
}

#define SPECIAL_TRAP(s)                                                       \
  ((s) == EXIT_TRAP || (s) == DEBUG_TRAP || (s) == ERROR_TRAP                 \
   || (s) == RETURN_TRAP)

#define GETORIGSIG(sig)                                                       \
  do                                                                          \
    {                                                                         \
      original_signals[sig] = set_signal_handler (sig, SIG_DFL);              \
      set_signal_handler (sig, original_signals[sig]);                        \
      if (original_signals[sig] == SIG_IGN)                                   \
        sigmodes[sig] |= SIG_HARD_IGNORE;                                     \
    }                                                                         \
  while (0)

#define SETORIGSIG(sig, handler)                                              \
  do                                                                          \
    {                                                                         \
      original_signals[sig] = handler;                                        \
      if (original_signals[sig] == SIG_IGN)                                   \
        sigmodes[sig] |= SIG_HARD_IGNORE;                                     \
    }                                                                         \
  while (0)

#define GET_ORIGINAL_SIGNAL(sig)                                              \
  if (sig && sig < NSIG && original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER)  \
  GETORIGSIG (sig)

// The global (constructed in shell.cc) pointer to the single shell object.
extern Shell *the_shell;

// Define two invalid references to indicate an invalid entry.

#define IMPOSSIBLE_TRAP_HANDLER (&std::exit)
#define IMPOSSIBLE_TRAP_NAME (reinterpret_cast<char *> (the_shell))

enum print_flags
{
  FUNC_NOFLAGS = 0,
  FUNC_MULTILINE = 0x01,
  FUNC_EXTERNAL = 0x02
};

/* enum for sh_makepath function defined in lib/sh/makepath.c */
enum mp_flags
{
  MP_NOFLAGS = 0,
  MP_DOTILDE = 0x01,
  MP_DOCWD = 0x02,
  MP_RMDOT = 0x04,
  MP_IGNDOT = 0x08
};

static inline mp_flags
operator| (const mp_flags &a, const mp_flags &b)
{
  return static_cast<mp_flags> (static_cast<uint32_t> (a)
                                | static_cast<uint32_t> (b));
}

/*
 * Pushing and popping strings, implemented in parser.cc.
 */

enum push_string_flags
{
  PSH_NOFLAGS = 0,
  PSH_ALIAS = 0x01,
  PSH_DPAREN = 0x02,
  PSH_SOURCE = 0x04,
  PSH_ARRAY = 0x08
};

class STRING_SAVER : public GENERIC_LIST
{
public:
  STRING_SAVER () : GENERIC_LIST () {}

  STRING_SAVER *
  next ()
  {
    return static_cast<STRING_SAVER *> (next_);
  }

  std::string saved_line;
#if defined(ALIAS)
  alias_t *expander; /* alias that caused this line to be pushed. */
#endif
  size_t saved_line_index;
  int expand_alias; /* Value to set expand_alias to when string is popped. */
  int saved_line_terminator;
  push_string_flags flags;
};

/* Here is a generic structure for associating character strings
   with integers.  It is used in the parser for shell tokenization. */
struct STRING_INT_ALIST
{
  CONSTEXPR
  STRING_INT_ALIST (const char *word_, int token_)
      : word (word_), token (token_)
  {
  }

  const char *word;
  int token;
};

// Flags used when matching pairs of grouping constructs.
enum pgroup_flags
{
  P_NOFLAGS = 0,
  P_FIRSTCLOSE = 0x0001,
  P_ALLOWESC = 0x0002,
  P_DQUOTE = 0x0004,
  P_COMMAND = 0x0008,   // parsing a command, so look for comments
  P_BACKQUOTE = 0x0010, // parsing a backquoted command substitution
  P_ARRAYSUB = 0x0020,  // parsing a [...] array subscript for assignment
  P_DOLBRACE = 0x0040   // parsing a ${...} construct
};

static inline pgroup_flags
operator& (const pgroup_flags &a, const pgroup_flags &b)
{
  return static_cast<pgroup_flags> (static_cast<uint32_t> (a)
                                    & static_cast<uint32_t> (b));
}

static inline pgroup_flags
operator| (const pgroup_flags &a, const pgroup_flags &b)
{
  return static_cast<pgroup_flags> (static_cast<uint32_t> (a)
                                    | static_cast<uint32_t> (b));
}

/* Lexical state while parsing a grouping construct or $(...). */
enum lexical_state_flags
{
  LEX_NOFLAGS = 0,
  LEX_WASDOL = 0x0001,
  LEX_CKCOMMENT = 0x0002,
  LEX_INCOMMENT = 0x0004,
  LEX_PASSNEXT = 0x0008,
  LEX_RESWDOK = 0x0010,
  LEX_CKCASE = 0x0020,
  LEX_INCASE = 0x0040,
  LEX_INHEREDOC = 0x0080,
  LEX_HEREDELIM = 0x0100, // reading here-doc delimiter
  LEX_STRIPDOC = 0x0200,  // <<- strip tabs from here doc delim
  LEX_QUOTEDDOC = 0x0400, // here doc with quoted delim
  LEX_INWORD = 0x0800,
  LEX_GTLT = 0x1000
};

static inline lexical_state_flags &
operator|= (lexical_state_flags &a, const lexical_state_flags &b)
{
  a = static_cast<lexical_state_flags> (static_cast<uint32_t> (a)
                                        | static_cast<uint32_t> (b));
  return a;
}

static inline lexical_state_flags &
operator&= (lexical_state_flags &a, const lexical_state_flags &b)
{
  a = static_cast<lexical_state_flags> (static_cast<uint32_t> (a)
                                        & static_cast<uint32_t> (b));
  return a;
}

static inline lexical_state_flags
operator& (const lexical_state_flags &a, const lexical_state_flags &b)
{
  return static_cast<lexical_state_flags> (static_cast<uint32_t> (a)
                                           & static_cast<uint32_t> (b));
}

static inline lexical_state_flags
operator~(const lexical_state_flags &a)
{
  return static_cast<lexical_state_flags> (~static_cast<uint32_t> (a));
}

/* Simple shell state: variables that can be memcpy'd to subshells. */
class SimpleState
{
public:
  // Initialize with default values (defined in shell.c).
  SimpleState ();

#if defined(VFORK_SUBSHELL)
  SimpleState (int fd); // TODO: Load initial values from pipe
#endif

  // Start of protected variables, ordered by decreasing alignment.
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

#if defined(HANDLE_MULTIBYTE)
  size_t ifs_firstc_len;
#endif

  /* ************************************************************** */
  /*		Bash Variables (32-bit int types)		    */
  /* ************************************************************** */

  /* from expr.cc: the OP in OP= */
  token_t assigntok;

  // from bashhist.cc
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

#if defined(PGRP_PIPE)
  /* Pipes which each shell uses to communicate with the process group leader
     until all of the processes in a pipeline have been started.  Then the
     process leader is allowed to continue. */
  int pgrp_pipe[2];
#endif

  /* Last child made by the shell.  */
  pid_t last_made_pid;

  /* Pid of the last asynchronous child. */
  pid_t last_asynchronous_pid;

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

#if defined(BUFFERED_INPUT)
  /* The file descriptor from which the shell is reading input. */
  int default_buffered_input;

  int bash_input_fd_changed;
#endif

  /* The current variable context.  This is really a count of how deep into
     executing functions we are. */
  int variable_context;

  /* The number of times BASH has been executed.  This is set
     by initialize_variables (). */
  int shell_level;

  /* variables from evalfile.c */
  int sourcelevel;

  /* Variables declared in execute_cmd.c, used by many other files */
  int return_catch_flag;
  int return_catch_value;

  /* Whether or not the last command (corresponding to last_command_exit_value)
     was terminated by a signal, and, if so, which one. */
  int last_command_exit_signal;

  int executing_builtin;
  int executing_list;       /* list nesting level */
  int comsub_ignore_return; /* can be greater than 1 */
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

  /* The value of $$. */
  pid_t dollar_dollar_pid;

  int subshell_argc;

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

  // additional 32-bit variables

  unsigned int zread_lind;  // read index in zread_lbuf
  unsigned int zread_lused; // bytes used in zread_lbuf

  int list_optopt;
  int list_opttype;

  int export_env_index;
  int export_env_size;

#if defined(READLINE)
  int winsize_assignment; /* currently assigning to LINES or COLUMNS */
#endif

  /* The number of lines read from input while creating the current command. */
  int current_command_line_count;

  /* The number of lines in a command saved while we run parse_and_execute */
  int saved_command_line_count;

  /* A count of signals received for which we have trap handlers. */
  int pending_traps[NSIG];

  /* Set to the number of the signal we're running the trap for + 1.
     Used in execute_cmd.c and builtins/common.c to clean up when
     parse_and_execute does not return normally after executing the
     trap command (e.g., when `return' is executed in the trap command). */
  int running_trap;

  /* Set to last_command_exit_value before running a trap. */
  int trap_saved_exit_value;

  /* The (trapped) signal received while executing in the `wait' builtin */
  int wait_signal_received;

  int trapped_signal_received;

public:
  // The following are accessed by the parser.

  /* The token that currently denotes the end of parse. */
  int shell_eof_token;

  /* The token currently being read. */
  int current_token;

  /* variables from evalstring.c */
  int parse_and_execute_level;

  /* The value returned by the last synchronous command (possibly modified
     by the parser). */
  int last_command_exit_value;

  /* The current parser state. */
  pstate_flags parser_state;

  /* The line number in a script at which an arithmetic for command starts. */
  int arith_for_lineno;

  /* Do that silly `type "bye" to exit' stuff.  You know, "ignoreeof". */

  /* The number of times that we have encountered an EOF character without
     another character intervening.  When this gets above the limit, the
     shell terminates. */
  int eof_encountered;

  /* The limit for eof_encountered. */
  int eof_encountered_limit;

  /* The line number in a script where the word in a `case WORD', `select WORD'
     or `for WORD' begins.  This is a nested command maximum, since the array
     index is decremented after a case, select, or for command is parsed. */
#define MAX_CASE_NEST 128
  int word_lineno[MAX_CASE_NEST + 1];
  int word_top;

  /* The line number in a script on which a function definition starts. */
  int function_dstart;

  /* The line number in a script on which a function body starts. */
  int function_bstart;

  /* The current index into the redir_stack array. */
  int need_here_doc;

  /* If non-zero, it is the token that we want read_token to return
     regardless of what text is (or isn't) present to be read.  This
     is reset by read_token.  If token_to_read == WORD or
     ASSIGNMENT_WORD, yylval.word should be set to word_desc_to_read. */
  int token_to_read;

  // Current printing indentation level.
  int indentation;

  // Indentation amount.
  int indentation_amount;

  // Increase the printing indentation level.
  void
  add_indentation ()
  {
    indentation += indentation_amount;
  }

  // Decrease the printing indentation level.
  void
  sub_indentation ()
  {
    indentation -= indentation_amount;
  }

  /* The depth of the group commands that we are currently printing.  This
     includes the group command that is a function body. */
  int group_command_nesting;

  // When non-zero, skip the next indent.
  int skip_this_indent;

  /* Non-zero means stuff being printed is inside of a function def. */
  int inside_function_def;

  /* Non-zero means stuff being printed is inside of a connection. */
  int printing_connection;

  /* The globally known line number. */
  int line_number;

#if defined(COND_COMMAND)
  int cond_lineno;
  int cond_token;
#endif

protected:
  // Back to protected access.

  /* Either zero or EOF. */
  int shell_input_line_terminator;

  /* The last read token, or NULL.  read_token () uses this for context
     checking. */
  int last_read_token;

  /* The token read prior to last_read_token. */
  int token_before_that;

  /* The token read prior to token_before_that. */
  int two_tokens_ago;

  /* When non-zero, we have read the required tokens
     which allow ESAC to be the next one read. */
  int esacs_needed_count;

  /* When non-zero, we can read IN as an acceptable token, regardless of how
     many newlines we read. */
  int expecting_in_token;

  // File descriptor for xtrace output.
  int xtrace_fd;

  /* An array of sigmode_t flags, one for each signal, describing what the
     shell will do with a signal.  DEBUG_TRAP == NSIG; some code below
     assumes this. */
  sigmode_t sigmodes[BASH_NSIG];

  int ngroups;   // number of groups the user is a member of
  int maxgroups; // max number of supplemental groups

  /* This implements one-character lookahead/lookbehind across physical input
     lines, to avoid something being lost because it's pushed back with
     shell_ungetc when we're at the start of a line. */
  int eol_ungetc_lookahead;

  /* When non-zero, an open-brace used to create a group is awaiting a close
     brace partner. */
  int open_brace_count;

  /* ************************************************************** */
  /*		Bash Variables (8-bit bool/char types)		    */
  /* ************************************************************** */

public:
  // The following are accessed by the parser.

  /* A flag denoting whether or not ignoreeof is set. */
  char ignoreeof;

  /* Non-zero means that at this moment, the shell is interactive.  In
     general, this means that the shell is at this moment reading input
     from the keyboard. */
  bool interactive;

  bool was_heredoc;

protected:
  // Back to protected access.

  /* Variables used to keep track of the characters in IFS. */
  bool ifs_cmap[UCHAR_MAX + 1];
  bool ifs_is_set;
  bool ifs_is_null;

#if defined(HANDLE_MULTIBYTE)
  char ifs_firstc[MB_LEN_MAX];
#else
  char ifs_firstc;
#endif

  /* Set by expand_word_unsplit and several of the expand_string_XXX functions;
     used to inhibit splitting and re-joining $* on $IFS, primarily when doing
     assignment statements.  The idea is that if we're in a context where this
     is set, we're not going to be performing word splitting, so we use the
     same rules to expand $* as we would if it appeared within double quotes.
   */
  bool expand_no_split_dollar_star;

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

  /* Set to true if an assignment error occurs while putting variables
     into the temporary environment. */
  bool tempenv_assign_error;

  /* True means that we have to remake EXPORT_ENV. */
  bool array_needs_making;

  /* Shared state from bashhist.cc */
  bool remember_on_history;
  char enable_history_list; /* value for `set -o history' */
  char literal_history;     /* controlled by `shopt lithist' */
  char force_append_history;
  char command_oriented_history;
  bool current_command_first_line_saved;
  bool current_command_first_line_comment;
  bool hist_last_line_added;
  bool hist_last_line_pushed;
  char dont_save_function_defs;

#if defined(BANG_HISTORY)
  bool history_expansion_inhibited;
  bool double_quotes_inhibit_history_expansion;
#endif /* BANG_HISTORY */

  /* Shared state from bashline.c */
#if defined(READLINE)
  bool bash_readline_initialized;
#endif
  bool hostname_list_initialized;

  /* variables from execute_cmd.h */

  /* Are we currently ignoring the -e option for the duration of a builtin's
     execution? */
  bool builtin_ignoring_errexit;

  bool match_ignore_case;
  bool executing_command_builtin;

  /* Set to 1 if fd 0 was the subject of redirection to a subshell.  Global
     so that reader_loop can set it to zero before executing a command. */
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

  /* This becomes true when the end of file has been reached. */
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

  // We're waiting for a child.
  bool waiting_for_child;

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

#if defined(BANG_HISTORY)
  /* Non-zero means that we are doing history expansion.  The default.
     This means !22 gets the 22nd line of history. */
  char history_expansion;
  char histexp_flag;
#endif /* BANG_HISTORY */

  /* Non-zero means that we allow comments to appear in interactive commands.
   */
  char interactive_comments;

#if defined(RESTRICTED_SHELL)
  /* Non-zero means that this shell is `restricted'.  A restricted shell
     disallows: changing directories, command or path names containing `/',
     unsetting or resetting the values of $PATH and $SHELL, and any type of
     output redirection. */
  char restricted;       /* currently restricted */
  char restricted_shell; /* shell was started in restricted mode. */
#endif                   /* RESTRICTED_SHELL */

  /* Non-zero means that this shell is running in `privileged' mode.  This
     is required if the shell is to run setuid.  If the `-p' option is
     not supplied at startup, and the real and effective uids or gids
     differ, disable_priv_mode is called to relinquish setuid status. */
  char privileged_mode;

#if defined(BRACE_EXPANSION)
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

#if defined(ARRAY_VARS)
  char array_expand_once;
  char assoc_expand_once;
#endif

  char cdable_vars;
  char cdspelling;
  char check_hashed_filenames;

  /* Non-zero means we expand aliases in commands. */
  char expand_aliases;

#if defined(EXTENDED_GLOB)
  char extended_glob;
  char global_extglob;
#endif

  /* If non-zero, $'...' and $"..." are expanded when they appear within
     a ${...} expansion, even when the expansion appears within double
     quotes. */
  char extended_quote;

  char glob_asciirange;
  char glob_dot_filenames;
  char glob_ignore_case;
  char glob_star;
  char lastpipe_opt;

  /* If non-zero, local variables inherit values and attributes from a variable
     with the same name at a previous scope. */
  char localvar_inherit;

  /* If non-zero, calling `unset' on local variables in previous scopes marks
     them as invisible so lookups find them unset. This is the same behavior
     as local variables in the current local scope. */
  char localvar_unset;

  char mail_warning;
  char no_exit_on_failed_exec;

#if defined(PROGRAMMABLE_COMPLETION)
  char prog_completion_enabled;
  char progcomp_alias;
#endif

  /* If non-zero, the decoded prompt string undergoes parameter and
     variable substitution, command substitution, arithmetic substitution,
     string expansion, process substitution, and quote removal in
     decode_prompt_string. */
  char promptvars;

#if defined(SYSLOG_HISTORY)
  char syslog_history;
#endif

  char xpg_echo;

#if defined(READLINE)
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
  bool debugging;         /* Do debugging things. */
  bool no_rc;             /* Don't execute ~/.bashrc */
  bool no_profile;        /* Don't execute .profile */
  bool do_version;        /* Display interesting version info. */
  bool make_login_shell;  /* Make this shell be a `-bash' shell. */
  bool want_initial_help; /* --help option */

#if defined(DEBUGGER)
  char debugging_mode; /* In debugging mode with --debugger */
#endif
  bool no_line_editing; /* non-zero -> don't do fancy line editing. */
  bool dump_translatable_strings; /* Dump strings in $"...", don't execute. */
  bool dump_po_strings;           /* Dump strings in $"..." in po format */
  bool wordexp_only;              /* Do word expansion only */
  bool protected_mode;            /* No command substitution with --wordexp */

  bool pretty_print_mode; /* pretty-print a shell script */

  char posixly_correct; /* Non-zero means posix.2 superset. */

  bool read_from_stdin;      /* -s flag supplied */
  bool want_pending_command; /* -c flag supplied */

  bool shell_reinitialized;

  /* Set to true to suppress the effect of `set v' in the DEBUG trap. */
  bool suppress_debug_trap_verbose;

  // Used by ttsave()/ttrestore() to check if there are valid saved settings.
  bool ttsaved;

  /* from lib/sh/unicode.c */

  bool u32init;
  bool utf8locale;

  // Has the expression already been through word expansion?
  bool already_expanded;

  // Have the terminating signals been initialized?
  bool termsigs_initialized;

  /* When true, we call the terminating signal handler immediately. */
  bool terminate_immediately;

  // Are we currently handling a terminating signal?
  bool handling_termsig;

  /* True if we've caught a trapped signal. */
  bool catch_flag;

  // Have the tilde expander strings been initialized?
  bool tilde_strings_initialized;

  // True if an unquoted backslash was seen during parsing.
  bool unquoted_backslash;

  bool here_doc_first_line;

#ifndef HAVE_LOCALE_CHARSET
  char charsetbuf[40];
#endif
};

#if defined(READLINE)
#define PARENT_CLASS , public readline::Readline
#define RL_OVERRIDE override
#else
#define PARENT_CLASS
#define RL_OVERRIDE
#endif

struct sh_parser_state_t;

/* Shell class, containing global variables and the methods that use them. */
class Shell : public SimpleState PARENT_CLASS
{
public:
  Shell ();
  virtual ~Shell () noexcept override;

  // This start method replaces main() from the C version.
  void start_shell (int argc, char **argv, char **env)
      __attribute__ ((__noreturn__));

  // Early initialization before the subshell loop.
  void early_init ();

  // This is called from start () once, and possibly later for subshells.
  // It will either exit or possibly throw an uncaught exception.
  void run_shell (int argc, char **argv, char **env)
      __attribute__ ((__noreturn__));

  // public typedefs accessed from other classes

  typedef int (Shell::*sh_builtin_func_t) (WORD_LIST *); /* sh_wlist_func_t */
  typedef void (Shell::*sh_vptrfunc_t) (void *);

  typedef char *(Shell::*tilde_hook_func_t) (char *);

  struct JOB
  {
    char *wd;      /* The working directory at time of invocation. */
    PROCESS *pipe; /* The pipeline of processes that make up this job. */
#if defined(JOB_CONTROL)
    COMMAND *deferred; /* Commands that will execute when this job is done. */
    sh_vptrfunc_t
        j_cleanup;   /* Cleanup function to call when job marked JDEAD */
    void *cleanarg;  /* Argument passed to (*j_cleanup)() */
#endif               /* JOB_CONTROL */
    pid_t pgrp;      /* The process ID of the process group (necessary). */
    JOB_STATE state; /* The state that this job is in. */
    job_flags
        flags; /* Flags word: J_NOTIFIED, J_FOREGROUND, or J_JOBCONTROL. */
  };

  // Define structs for jobs and collected job stats.

#define NO_JOB -1      /* An impossible job array index. */
#define DUP_JOB -2     /* A possible return value for get_job_spec (). */
#define BAD_JOBSPEC -3 /* Bad syntax for job spec. */

  // Job stats struct.
  struct jobstats
  {
    jobstats () : c_childmax (-1), j_current (NO_JOB), j_previous (NO_JOB) {}

    /* limits */
    long c_childmax;
    /* */
    JOB *j_lastmade;  /* last job allocated by stop_pipeline */
    JOB *j_lastasync; /* last async job allocated by stop_pipeline */
    /* child process statistics */
    int c_living; /* running or stopped child processes */
    int c_reaped; /* exited child processes still in jobs list */
    int c_injobs; /* total number of child processes in jobs list */
    /* child process totals */
    int c_totforked; /* total number of children this shell has forked */
    int c_totreaped; /* total number of children this shell has reaped */
    /* job counters and indices */
    int j_lastj;  /* last (newest) job allocated */
    int j_firstj; /* first (oldest) job allocated */
    int j_njobs;  /* number of non-NULL jobs in jobs array */
    int j_ndead;  /* number of JDEAD jobs in jobs array */
    /* */
    int j_current;  /* current job */
    int j_previous; /* previous job */
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

  typedef char *(Shell::*sh_string_func_t) (
      const char *); /* like savestring, et al. */

  typedef int (Shell::*sh_msg_func_t) (const char *, ...); /* printf(3)-like */
  typedef void (Shell::*sh_vmsg_func_t) (const char *,
                                         ...); /* printf(3)-like */
  typedef void (Shell::*PFUNC) (const char *, ...);

  /* Specific function pointer typedefs.  Most of these could be done
     with #defines. */
  typedef void (Shell::*sh_sv_func_t) (const char *); /* sh_vcpfunc_t */
  typedef void (Shell::*sh_free_func_t) (void *);     /* sh_vptrfunc_t */
  typedef void (Shell::*sh_resetsig_func_t) (int);    /* sh_vintfunc_t */

  typedef bool (Shell::*sh_ignore_func_t) (const char *); /* sh_icpfunc_t */

  typedef int (Shell::*sh_assign_func_t) (const char *);
  typedef int (Shell::*sh_wassign_func_t) (WORD_DESC *, int);

  typedef int (Shell::*sh_load_func_t) (const char *);
  typedef void (Shell::*sh_unload_func_t) (const char *);

  typedef int (Shell::*sh_cget_func_t) ();      /* sh_ivoidfunc_t */
  typedef int (Shell::*sh_cunget_func_t) (int); /* sh_intfunc_t */

public:
  // public function typedefs for global setopt struct
  typedef int (Shell::*setopt_set_func_t) (int, const char *);
  typedef int (Shell::*setopt_get_func_t) (const char *);

  int set_edit_mode (int, const char *);
  int get_edit_mode (const char *);
  int bash_set_history (int, const char *);
  int set_ignoreeof (int, const char *);
  int set_posix_mode (int, const char *);

  typedef int (Shell::*shopt_set_func_t) (const char *, int);

  // Called from generated parser on EOF.
  void handle_eof_input_unit ();

  // Called from generated parser to rewind the input stream.
  void rewind_input_string ();

  // typedefs for builtins (public for access from builtins struct)

#if defined(ALIAS)
  int alias_builtin (WORD_LIST *);
  int unalias_builtin (WORD_LIST *);
#endif /* ALIAS */

#if defined(READLINE)
  int bind_builtin (WORD_LIST *);
#endif /* READLINE */

  int break_builtin (WORD_LIST *);
  int continue_builtin (WORD_LIST *);
  int builtin_builtin (WORD_LIST *);

#if defined(DEBUGGER)
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

#if defined(HISTORY)
  int fc_builtin (WORD_LIST *);
#endif /* HISTORY */

#if defined(JOB_CONTROL)
  int fg_builtin (WORD_LIST *);
  int bg_builtin (WORD_LIST *);
#endif /* JOB_CONTROL */

  int hash_builtin (WORD_LIST *);

#if defined(HELP_BUILTIN)
  int help_builtin (WORD_LIST *);
#endif /* HELP_BUILTIN */

#if defined(HISTORY)
  int history_builtin (WORD_LIST *);
#endif /* HISTORY */

#if defined(JOB_CONTROL)
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

#if defined(JOB_CONTROL)
  int suspend_builtin (WORD_LIST *);
#endif /* JOB_CONTROL */

  int test_builtin (WORD_LIST *);
  int times_builtin (WORD_LIST *);
  int trap_builtin (WORD_LIST *);
  int type_builtin (WORD_LIST *);
  int ulimit_builtin (WORD_LIST *);
  int umask_builtin (WORD_LIST *);
  int wait_builtin (WORD_LIST *);

  int parse_symbolic_mode (char *, int);

#if defined(PUSHD_AND_POPD)
  int pushd_builtin (WORD_LIST *);
  int popd_builtin (WORD_LIST *);
  int dirs_builtin (WORD_LIST *);
  char *get_dirstack_from_string (char *);
#endif /* PUSHD_AND_POPD */

  int shopt_builtin (WORD_LIST *);
  int printf_builtin (WORD_LIST *);

#if defined(PROGRAMMABLE_COMPLETION)
  int complete_builtin (WORD_LIST *);
  int compgen_builtin (WORD_LIST *);
  int compopt_builtin (WORD_LIST *);
#endif /* PROGRAMMABLE_COMPLETION */

  int mapfile_builtin (WORD_LIST *);

  // other functions moved from builtins/common.hh

  int set_login_shell (const char *, int);

  void initialize_shell_builtins ();

  /* Functions from exit.def */
  void bash_logout ();

  /* Functions from getopts.def */
  void getopts_reset (int);

  /* Functions from help.def */
  void builtin_help ();

  /* Functions from read.def */
  void read_tty_cleanup ();
  int read_tty_modified ();

  /* Functions from set.def */
  int minus_o_option_value (const char *);
  void list_minus_o_opts (int, int);
  char **get_minus_o_opts ();
  int set_minus_o_option (int, const char *);

  void set_shellopts ();
  void parse_shellopts (const char *);
  void initialize_shell_options (int);

  void reset_shell_options ();

  char *get_current_options ();
  void set_current_options (const char *);

  /* Functions from shopt.def */
  void reset_shopt_options ();
  char **get_shopt_options ();

  int shopt_setopt (const char *, int);
  int shopt_listopt (const char *, int);

  void set_bashopts ();
  void parse_bashopts (const char *);
  void initialize_bashopts (int);

  void set_compatibility_opts ();

  /* Functions from common.c */
  void builtin_error (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));
  void builtin_warning (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));
  void builtin_usage ();
  void no_args (WORD_LIST *);
  int no_options (WORD_LIST *);

  /* common error message functions */
  void sh_needarg (const char *);
  void sh_neednumarg (const char *);
  void sh_notfound (const char *);
  void sh_invalidopt (const char *);
  void sh_invalidoptname (const char *);
  void sh_invalidid (const char *);
  void sh_invalidnum (const char *);
  void sh_invalidsig (const char *);
  void sh_erange (const char *, const char *);
  void sh_badpid (const char *);
  void sh_badjob (const char *);
  void sh_readonly (const char *);
  void sh_nojobs (const char *);
  void sh_restricted (const char *);
  void sh_notbuiltin (const char *);
  void sh_wrerror ();
  void sh_ttyerror (int);
  int sh_chkwrite (int);

  char **make_builtin_argv (WORD_LIST *, int *);
  void remember_args (WORD_LIST *, bool);
  void shift_args (int);
  int number_of_args ();

  int dollar_vars_changed ();
  void set_dollar_vars_unchanged ();
  void set_dollar_vars_changed ();

  int get_numeric_arg (WORD_LIST *, int, int64_t *);
  int get_exitstat (WORD_LIST *);
  int read_octal (const char *);

#if defined(JOB_CONTROL)
  int get_job_by_name (const char *, int);
  int get_job_spec (WORD_LIST *);
#endif
  int display_signal_list (WORD_LIST *, int);

  /* Functions from evalstring.c */
  int parse_and_execute (const char *, const char *, parse_flags);
  int evalstring (char *, const char *, int);
  void parse_and_execute_cleanup (int);
  int parse_string (char *, const char *, int, char **);
  int should_suppress_fork (COMMAND *);
  int can_optimize_connection (COMMAND *);
  void optimize_fork (COMMAND *);
  void optimize_subshell_command (COMMAND *);
  void optimize_shell_function (COMMAND *);

  /* Functions from evalfile.c */
  int maybe_execute_file (const char *, bool);
  int force_execute_file (const char *, bool);
  int source_file (const char *, int);
  int fc_execute_file (const char *);

  /* Functions from flags.cc */
  int change_flag (int, int);
  void initialize_flags ();

  /* callback from lib/sh/getenv.cc */
  char *getenv (const char *);

  // lexer called from Bison parser
  parser::symbol_type yylex ();

  /* Functions from make_cmd.cc */

  WORD_DESC *
  make_word_flags (WORD_DESC *w, const std::string &string)
  {
    DECLARE_MBSTATE;

    std::string::const_iterator it = string.begin ();
    while (it != string.end ())
      {
        switch (*it)
          {
          case '$':
            w->flags |= W_HASDOLLAR;
            break;
          case '\\':
            break; /* continue the loop */
          case '\'':
          case '`':
          case '"':
            w->flags |= W_QUOTED;
            break;
          }

        advance_char (it, string.end (), locale_mb_cur_max, locale_utf8locale,
                      state);
      }

    return w;
  }

  WORD_DESC *
  make_word (const std::string &string)
  {
    WORD_DESC *temp = new WORD_DESC (string);
    return make_word_flags (temp, string);
  }

  WORD_DESC *
  make_word_from_token (int token)
  {
    char tokenizer[2];

    tokenizer[0] = static_cast<char> (token);
    tokenizer[1] = '\0';

    return make_word (tokenizer);
  }

  WORD_LIST *
  make_arith_for_expr (const std::string &s)
  {
    if (s.empty ())
      return nullptr;

    WORD_DESC *wd = make_word (s);
    wd->flags |= (W_NOGLOB | W_NOSPLIT | W_QUOTED
                  | W_DQUOTE); /* no word splitting or globbing */
#if defined(PROCESS_SUBSTITUTION)
    wd->flags |= W_NOPROCSUB; /* no process substitution */
#endif
    return new WORD_LIST (wd);
  }

  ARITH_FOR_COM *make_arith_for_command (WORD_LIST *exprs, COMMAND *action,
                                         int lineno);

  SIMPLE_COM *make_simple_command (ELEMENT element,
                                   COMMAND *command = nullptr);

  FUNCTION_DEF *make_function_def (WORD_DESC *, COMMAND *, int, int);

  void make_here_document (REDIRECT *, int);

  void
  push_heredoc (REDIRECT *r)
  {
    if (need_here_doc >= HEREDOC_MAX)
      {
        last_command_exit_value = EX_BADUSAGE;
        need_here_doc = 0;
        report_syntax_error (_ ("maximum here-document count exceeded"));
        reset_parser ();
        exit_shell (last_command_exit_value);
      }
    redir_stack[need_here_doc++] = r;
  }

  void gather_here_documents ();

  COMMAND *
  clean_simple_command (COMMAND *command)
  {
    SIMPLE_COM *simple_com = dynamic_cast<SIMPLE_COM *> (command);
    if (simple_com)
      {
        simple_com->words = simple_com->words->reverse ();
        simple_com->redirects = simple_com->redirects->reverse ();
      }
    else
      command_error ("clean_simple_command", CMDERR_BADTYPE, NOEXCEPTION);

    parser_state &= ~PST_REDIRLIST;
    return command;
  }

  // called from global signal handler functions
  void termsig_sighandler (int);
  void trap_handler (int);

#if defined(SELECT_COMMAND)
  void xtrace_print_select_command_head (FOR_SELECT_COM *);
#endif

  void xtrace_print_case_command_head (CASE_COM *);

#if defined(DPAREN_ARITHMETIC) || defined(ARITH_FOR_COMMAND)
  void xtrace_print_arith_cmd (WORD_LIST *);
#endif

  void make_command_string_internal (COMMAND *);

#if defined(COND_COMMAND)
  void print_cond_node (COND_COM *);
#ifdef DEBUG
  void debug_print_cond_command (COND_COM *cond);
#endif
  void xtrace_print_cond_term (cond_com_type, bool, WORD_DESC *, const char *,
                               const char *);
#endif

  void
  print_heredocs (REDIRECT *heredocs)
  {
    REDIRECT *hdtail;

    cprintf (" ");
    for (hdtail = heredocs; hdtail; hdtail = hdtail->next ())
      {
        print_redirection (hdtail);
        cprintf ("\n");
      }
    was_heredoc = true;
  }

  void
  print_heredoc_bodies (REDIRECT *heredocs)
  {
    REDIRECT *hdtail;

    cprintf ("\n");
    for (hdtail = heredocs; hdtail; hdtail = hdtail->next ())
      {
        print_heredoc_body (hdtail);
        cprintf ("\n");
      }
    was_heredoc = true;
  }

  void cprintf (const char *, ...) __attribute__ ((__format__ (printf, 2, 3)));
  void xprintf (const char *, ...) __attribute__ ((__format__ (printf, 2, 3)));

  void
  _print_word_list (WORD_LIST *list, const char *separator, PFUNC pfunc)
  {
    for (WORD_LIST *w = list; w; w = w->next ())
      ((*this).*pfunc) ("%s%s", w->word->word.c_str (),
                        w->next () ? separator : "");
  }

  void
  print_word_list (WORD_LIST *list, const char *separator)
  {
    _print_word_list (list, separator, &Shell::xprintf);
  }

  void xtrace_set (int, FILE *);

  void
  xtrace_init ()
  {
    xtrace_set (-1, stderr);
  }

  void
  xtrace_fdchk (int fd)
  {
    if (fd == xtrace_fd)
      xtrace_reset ();
  }

  void
  command_print_word_list (WORD_LIST *list, const char *separator)
  {
    _print_word_list (list, separator, &Shell::cprintf);
  }

  void
  newline (const char *string)
  {
    cprintf ("\n");
    indent (indentation);
    if (string && *string)
      cprintf ("%s", string);
  }

  void
  indent (int count)
  {
    indentation_string.assign (static_cast<size_t> (count), ' ');
    cprintf ("%s", indentation_string.c_str ());
  }

  void
  semicolon ()
  {
    size_t size = the_printed_command.size ();
    if (size
        && (the_printed_command[size - 1] == '&'
            || the_printed_command[size - 1] == '\n'))
      return;
    cprintf (";");
  }

  void
  PRINT_DEFERRED_HEREDOCS (const char *x)
  {
    if (deferred_heredocs)
      print_deferred_heredocs (x);
  }

  void print_redirection_list (REDIRECT *);

  void print_heredoc_header (REDIRECT *);

  void
  print_heredoc_body (REDIRECT *redirect)
  {
    /* Here doc body */
    cprintf ("%s%s", redirect->redirectee.r.filename->word.c_str (),
             redirect->here_doc_eof.c_str ());
  }

  void print_redirection (REDIRECT *);

  void print_function_def (FUNCTION_DEF *);

  char *named_function_string (const char *, COMMAND *, print_flags);

  void print_deferred_heredocs (const char *);

  // Protected methods below
protected:
  // Functions from parser.cc (previously in parse.y).

  // Report parser syntax error.
  void report_syntax_error (const char *);

  sh_parser_state_t *save_parser_state (sh_parser_state_t *);
  void restore_parser_state (sh_parser_state_t *);

#if defined(READLINE)

  int yy_readline_get ();

  int
  yy_readline_unget (int c)
  {
    if (current_readline_line_index && !current_readline_line.empty ())
      current_readline_line[--current_readline_line_index]
          = static_cast<char> (c);
    return c;
  }

  /* Will we be collecting another input line and printing a prompt? This uses
     different conditions than SHOULD_PROMPT(), since readline allows a user to
     embed a newline in the middle of the line it collects, which the parser
     will interpret as a line break and command delimiter. */
  bool
  parser_will_prompt ()
  {
    return current_readline_line.empty ();
  }

#endif

  int yy_string_get ();
  int yy_string_unget (int);

  void with_input_from_string (char *, const char *);

  int yy_stream_get ();
  int yy_stream_unget (int);

  void push_stream (int);
  void pop_stream ();

  /* Save the current token state and return it in a new array. */
  int *
  save_token_state ()
  {
    int *ret = new int[4];

    ret[0] = last_read_token;
    ret[1] = token_before_that;
    ret[2] = two_tokens_ago;
    ret[3] = current_token;
    return ret;
  }

  void
  restore_token_state (int *ts)
  {
    if (ts == nullptr)
      return;

    last_read_token = ts[0];
    token_before_that = ts[1];
    two_tokens_ago = ts[2];
    current_token = ts[3];
  }

  int find_reserved_word (const char *);

  // Functions provided by various builtins.

  int evalstring (char *, const char *, parse_flags);

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

  /* Functions from arrayfunc.cc */

  char *array_variable_name (const char *, int, char **, int *);

  /* Functions from expr.cc */

  enum eval_flags
  {
    EXP_NONE = 0,
    EXP_EXPANDED = 0x01
  };

  int64_t evalexp (const std::string &, eval_flags, bool *);
  int64_t subexpr (const std::string &);
  int64_t expcomma ();
  int64_t expassign ();
  int64_t expcond ();
  int64_t explor ();
  int64_t expland ();
  int64_t expbor ();
  int64_t expbxor ();
  int64_t expband ();
  int64_t exp5 ();
  int64_t exp4 ();
  int64_t expshift ();
  int64_t exp3 ();
  int64_t expmuldiv ();
  int64_t exppower ();
  int64_t exp1 ();
  int64_t exp0 ();
  int64_t expr_streval (const char *, int, struct lvalue *);

  void readtok ();

  void evalerror (const char *);
  int64_t strlong (const char *);

  void pushexp ();
  void popexp ();
  void expr_unwind ();
  void expr_bind_variable (const std::string &, const std::string &);

#if defined(ARRAY_VARS)
  int expr_skipsubscript (const char *, char *);
  void expr_bind_array_element (const std::string &, arrayind_t,
                                const std::string &);
#endif

  /* set -x support */
#ifdef NEED_XTRACE_SET_DECL
  void xtrace_set (int, FILE *);
#endif
  void xtrace_reset ();
  const char *indirection_level_string ();
  void xtrace_print_assignment (const char *, const char *, bool, int);
  void xtrace_print_word_list (WORD_LIST *, int);
  void xtrace_print_for_command_head (FOR_SELECT_COM *);

  /* Functions from shell.c. */
  void exit_shell (int) __attribute__ ((__noreturn__));
  void subshell_exit (int) __attribute__ ((__noreturn__));
  void disable_priv_mode ();

  void
  unbind_args ()
  {
    remember_args (nullptr, true);
    pop_args (); /* Reset BASH_ARGV and BASH_ARGC */
  }

#if defined(RESTRICTED_SHELL)
  bool shell_is_restricted (const char *);
  bool maybe_make_restricted (const char *);
#endif

  void unset_bash_input (int);
  void get_current_user_info ();

  /* Functions from eval.cc. */
  int reader_loop ();
  int pretty_print_loop ();
  int parse_command ();
  int read_command ();
  void send_pwd_to_eterm ();

#if defined(ARRAY_VARS)
  void execute_array_command (ARRAY *, const char *);
#endif

  void execute_prompt_command ();

  /* Functions from braces.c. */
#if defined(BRACE_EXPANSION)
  char **brace_expand (char *);
#endif

  int parser_in_command_position ();

  void
  free_pushed_string_input ()
  {
#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
    free_string_list ();
#endif
  }

  bool
  parser_expanding_alias ()
  {
#if defined(ALIAS)
    return pushed_string_list && pushed_string_list->expander;
#else
    return false;
#endif
  }

  void
  parser_save_alias ()
  {
#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
    push_string (nullptr, 0, nullptr);
    pushed_string_list->flags = PSH_SOURCE; /* XXX - for now */
#else
    ;
#endif
  }

  void
  parser_restore_alias ()
  {
#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
    if (pushed_string_list)
      pop_string ();
#else
    ;
#endif
  }

  void
  clear_shell_input_line ()
  {
    shell_input_line.clear ();
  }

  char *decode_prompt_string (const char *);

#if defined(HISTORY)
  void bash_initialize_history ();
  void load_history ();
  const char *history_delimiting_chars (const char *);
  int prompt_history_number (const char *);
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

  /* Functions from errors.cc */

  /* Get the name of the shell or shell script for an error message. */
  const char *get_name_for_error ();

  /* Report an error having to do with FILENAME. */
  void file_error (const char *);

  /* Report a programmer's error, and abort.  Pass REASON, and ARG1 ... ARG5.
   */
  void programming_error (const char *, ...) __attribute__ ((__noreturn__))
  __attribute__ ((__format__ (printf, 2, 3)));

  /* General error reporting.  Pass FORMAT and ARG1 ... ARG5. */
  void report_error (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  /* Error messages for parts of the parser that don't call report_syntax_error
   */
  void parser_error (int, const char *, ...)
      __attribute__ ((__format__ (printf, 3, 4)));

  /* Report an unrecoverable error and exit.  Pass FORMAT and ARG1 ... ARG5. */
  void fatal_error (const char *, ...) __attribute__ ((__noreturn__))
  __attribute__ ((__format__ (printf, 2, 3)));

  /* Report a system error, like BSD warn(3). */
  void sys_error (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  /* Report an internal error. */
  void internal_error (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  /* Report an internal warning. */
  void internal_warning (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  /* Report an internal informational notice. */
  void internal_inform (const char *, ...)
      __attribute__ ((__format__ (printf, 2, 3)));

  /* Debugging functions, not enabled in released version. */
  char *strescape (const char *);
  void itrace (const char *, ...) __attribute__ ((__format__ (printf, 2, 3)));
  void trace (const char *, ...) __attribute__ ((__format__ (printf, 2, 3)));

  /* Report an error having to do with command parsing or execution. */
  void command_error (const char *, cmd_err_type, bash_exception_t)
      __attribute__ ((__noreturn__));

  const char *command_errstr (int);

  /* Specific error message functions that eventually call report_error or
     internal_error. */

  void err_badarraysub (const char *);
  void err_unboundvar (const char *);
  void err_readonly (const char *);

  void error_prolog (int);

  /* Functions from input.cc */

  int getc_with_restart (FILE *);

  int
  ungetc_with_restart (int c, FILE *)
  {
    if (local_index == 0 || c == EOF)
      return EOF;
    localbuf[--local_index] = static_cast<char> (c);
    return c;
  }

  /* Make sure `buffers' has at least N elements. */
  void
  allocate_buffers (size_t n)
  {
    buffers.resize (n + 20);
  }

  BUFFERED_STREAM *make_buffered_stream (int, char *, size_t);

  int
  set_bash_input_fd (int fd)
  {
    if (bash_input.type == st_bstream)
      bash_input.location.buffered_fd = fd;
    else if (!interactive_shell)
      default_buffered_input = fd;
    return 0;
  }

  int
  fd_is_bash_input (int fd)
  {
    if (bash_input.type == st_bstream && bash_input.location.buffered_fd == fd)
      return 1;
    else if (!interactive_shell && default_buffered_input == fd)
      return 1;
    return 0;
  }

  int save_bash_input (int, int);
  int check_bash_input (int);
  int duplicate_buffered_stream (int, int);
  BUFFERED_STREAM *fd_to_buffered_stream (int);

  /* Return a buffered stream corresponding to FILE, a file name. */
  BUFFERED_STREAM *
  open_buffered_stream (const char *file)
  {
    int fd;

    // Infer warns that this is a resource leak. Hopefully it isn't one.
    fd = ::open (file, O_RDONLY);
    return (fd >= 0) ? fd_to_buffered_stream (fd) : nullptr;
  }

  /* Deallocate a buffered stream and free up its resources.  Make sure we
     zero out the slot in BUFFERS that points to BP. */
  void
  free_buffered_stream (BUFFERED_STREAM *bp)
  {
    if (!bp)
      return;

    size_t n = static_cast<size_t> (bp->b_fd);
    if (bp->b_buffer)
      delete[] bp->b_buffer;
    delete bp;
    buffers[n] = nullptr;
  }

  /* Close the file descriptor associated with BP, a buffered stream, and free
     up the stream.  Return the status of closing BP's file descriptor. */
  int
  close_buffered_stream (BUFFERED_STREAM *bp)
  {
    if (!bp)
      return 0;

    int fd = bp->b_fd;
    if (bp->b_flag & B_SHAREDBUF)
      bp->b_buffer = nullptr;
    free_buffered_stream (bp);
    return ::close (fd);
  }

  /* Deallocate the buffered stream associated with file descriptor FD, and
     close FD.  Return the status of the close on FD. */
  int
  close_buffered_fd (int fd)
  {
    if (fd < 0)
      {
        errno = EBADF;
        return -1;
      }
    size_t sfd = static_cast<size_t> (fd);
    if (sfd >= buffers.size () || !buffers[sfd])
      return ::close (fd);
    return close_buffered_stream (buffers[sfd]);
  }

  /* Make the BUFFERED_STREAM associated with buffers[FD] be BP, and return
     the old BUFFERED_STREAM. */
  BUFFERED_STREAM *
  set_buffered_stream (int fd, BUFFERED_STREAM *bp)
  {
    size_t sfd = static_cast<size_t> (fd);
    BUFFERED_STREAM *ret = buffers[sfd];
    buffers[sfd] = bp;
    return ret;
  }

  int b_fill_buffer (BUFFERED_STREAM *);
  int sync_buffered_stream (int);
  int buffered_getchar ();
  int buffered_ungetchar (int);
  void with_input_from_buffered_stream (int, const char *);

  /* Push C back onto buffered stream BP. */
  static int
  bufstream_ungetc (int c, BUFFERED_STREAM *bp)
  {
    if (c == EOF || bp == nullptr || bp->b_inputp == 0)
      return EOF;

    bp->b_buffer[--bp->b_inputp] = static_cast<char> (c);
    return c;
  }

  /* Functions from jobs.h */

#if defined(JOB_CONTROL)

  void
  making_children ()
  {
    if (already_making_children)
      return;

    already_making_children = true;
    start_pipeline ();
  }

  void
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

#endif // JOB_CONTROL

  // Set already_making_children to false. This method is the same,
  // with or without JOB_CONTROL.
  void
  stop_making_children ()
  {
    already_making_children = false;
  }

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
  bool
  has_job_control ()
  {
    return job_control;
  }

#if defined(JOB_CONTROL)
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
#if defined(PGRP_PIPE)
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
  void bgp_resize (); /* XXX */

  /* from lib/sh/casemod.cc */

  std::string sh_modcase (const std::string &, const char *, sh_modcase_flags);

  /* from lib/sh/eaccess.cc */

  int sh_stataccess (const char *, int);

  /* from lib/sh/makepath.cc */

  char *sh_makepath (const char *, const char *, mp_flags);

  /* from lib/sh/mbschr.cc */

  const char *mbschr (const char *, int);

  /* from lib/sh/netopen.cc */

#ifndef HAVE_GETADDRINFO
  int _netopen4 (const char *, const char *, int);
#else /* HAVE_GETADDRINFO */
  int _netopen6 (const char *, const char *, int);
#endif

  int netopen (const char *);

  /*
   * Open a TCP or UDP connection to HOST on port SERV.  Uses getaddrinfo(3)
   * if available, falling back to the traditional BSD mechanisms otherwise.
   * Returns the connected socket or -1 on error.
   */
  int
  _netopen (const char *host, const char *serv, int typ)
  {
#ifdef HAVE_GETADDRINFO
    return _netopen6 (host, serv, typ);
#else
    return _netopen4 (host, serv, typ);
#endif
  }

  /* from lib/sh/pathphys.cc */
  char *sh_realpath (const char *, char *);

  /* from lib/sh/random.c */
  u_bits32_t genseed ();
  int brand ();
  u_bits32_t brand32 ();
  u_bits32_t get_urandom32 ();

  /* Set the random number generator seed to the least-significant 32 bits of
   * SEED. */
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

  /* from lib/sh/shquote.c */

  std::string sh_double_quote (const std::string &);
  std::string sh_mkdoublequoted (const std::string &, int);
  std::string sh_un_double_quote (const std::string &);
  std::string sh_backslash_quote (const std::string &, const char *, int);
  std::string sh_backslash_quote_for_double_quotes (const std::string &);

  /* include all functions from lib/sh/shtty.c here: they're very small. */
  /* shtty.cc -- abstract interface to the terminal, focusing on capabilities.
   */

  static int
  ttgetattr (int fd, TTYSTRUCT *ttp)
  {
#ifdef TERMIOS_TTY_DRIVER
    return ::tcgetattr (fd, ttp);
#else
#ifdef TERMIO_TTY_DRIVER
    return ::ioctl (fd, TCGETA, ttp);
#else
    return ::ioctl (fd, TIOCGETP, ttp);
#endif
#endif
  }

  static int
  ttsetattr (int fd, TTYSTRUCT *ttp)
  {
#ifdef TERMIOS_TTY_DRIVER
    return ::tcsetattr (fd, TCSADRAIN, ttp);
#else
#ifdef TERMIO_TTY_DRIVER
    return ::ioctl (fd, TCSETAW, ttp);
#else
    return ::ioctl (fd, TIOCSETN, ttp);
#endif
#endif
  }

  void
  ttsave ()
  {
    if (ttsaved)
      return;
    (void)ttgetattr (0, &ttin);
    (void)ttgetattr (1, &ttout);
    ttsaved = true;
  }

  void
  ttrestore ()
  {
    if (!ttsaved)
      return;
    (void)ttsetattr (0, &ttin);
    (void)ttsetattr (1, &ttout);
    ttsaved = false;
  }

  /* Retrieve the internally-saved attributes associated with tty fd FD. */
  TTYSTRUCT *
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
  static void
  tt_setonechar (TTYSTRUCT *ttp)
  {
#if defined(TERMIOS_TTY_DRIVER) || defined(TERMIO_TTY_DRIVER)

    /* XXX - might not want this -- it disables erase and kill processing. */
    ttp->c_lflag &= ~(static_cast<tcflag_t> (ICANON));

    ttp->c_lflag |= ISIG;
#ifdef IEXTEN
    ttp->c_lflag |= IEXTEN;
#endif

    ttp->c_iflag |= ICRNL; /* make sure we get CR->NL on input */
    ttp->c_iflag &= ~(static_cast<tcflag_t> (INLCR)); /* but no NL->CR */

#ifdef OPOST
    ttp->c_oflag |= OPOST;
#endif
#ifdef ONLCR
    ttp->c_oflag |= ONLCR;
#endif
#ifdef OCRNL
    ttp->c_oflag &= ~(static_cast<tcflag_t> (OCRNL));
#endif
#ifdef ONOCR
    ttp->c_oflag &= ~(static_cast<tcflag_t> (ONOCR));
#endif
#ifdef ONLRET
    ttp->c_oflag &= ~(static_cast<tcflag_t> (ONLRET));
#endif

    ttp->c_cc[VMIN] = 1;
    ttp->c_cc[VTIME] = 0;

#else

    ttp->sg_flags |= CBREAK;

#endif
  }

  /* Set the tty associated with FD and TTP into one-character-at-a-time mode
   */
  static int
  ttfd_onechar (int fd, TTYSTRUCT *ttp)
  {
    tt_setonechar (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into one-character-at-a-time mode */
  int
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
  static void
  tt_setnoecho (TTYSTRUCT *ttp)
  {
#if defined(TERMIOS_TTY_DRIVER) || defined(TERMIO_TTY_DRIVER)
    ttp->c_lflag &= ~(static_cast<tcflag_t> (ECHO | ECHOK | ECHONL));
#else
    ttp->sg_flags &= ~ECHO;
#endif
  }

  /* Set the tty associated with FD and TTP into no-echo mode */
  static int
  ttfd_noecho (int fd, TTYSTRUCT *ttp)
  {
    tt_setnoecho (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into no-echo mode */
  int
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
  static void
  tt_seteightbit (TTYSTRUCT *ttp)
  {
#if defined(TERMIOS_TTY_DRIVER) || defined(TERMIO_TTY_DRIVER)
    ttp->c_iflag &= ~(static_cast<tcflag_t> (ISTRIP));
    ttp->c_cflag |= CS8;
    ttp->c_cflag &= ~(static_cast<tcflag_t> (PARENB));
#else
    ttp->sg_flags |= ANYP;
#endif
  }

  /* Set the tty associated with FD and TTP into eight-bit mode */
  static int
  ttfd_eightbit (int fd, TTYSTRUCT *ttp)
  {
    tt_seteightbit (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into eight-bit mode */
  int
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
  static void
  tt_setnocanon (TTYSTRUCT *ttp)
  {
#if defined(TERMIOS_TTY_DRIVER) || defined(TERMIO_TTY_DRIVER)
    ttp->c_lflag &= ~(static_cast<tcflag_t> (ICANON));
#endif
  }

  /* Set the tty associated with FD and TTP into non-canonical mode */
  static int
  ttfd_nocanon (int fd, TTYSTRUCT *ttp)
  {
    tt_setnocanon (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into non-canonical mode */
  int
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
  static void
  tt_setcbreak (TTYSTRUCT *ttp)
  {
    tt_setonechar (ttp);
    tt_setnoecho (ttp);
  }

  /* Set the tty associated with FD and TTP into cbreak (no-echo,
     one-character-at-a-time) mode */
  static int
  ttfd_cbreak (int fd, TTYSTRUCT *ttp)
  {
    tt_setcbreak (ttp);
    return ttsetattr (fd, ttp);
  }

  /* Set the terminal into cbreak (no-echo, one-character-at-a-time) mode */
  int
  ttcbreak ()
  {
    TTYSTRUCT tt;

    if (!ttsaved)
      return -1;
    tt = ttin;
    return ttfd_cbreak (0, &tt);
  }

  /* from lib/sh/strtrans.c */

  std::string ansicstr (const std::string &, int, bool *);

  std::string ansiexpand (std::string::const_iterator,
                          std::string::const_iterator);

  /* from lib/sh/tmpfile.c */

  const char *get_tmpdir (int);

  /* from lib/sh/unicode.c */

  char *stub_charset ();
  void u32reset ();

  void u32cconv (uint32_t, std::string &);

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

    check_signals (); /* check for signals before a blocking read */
    while ((r = ::read (fd, buf, len)) < 0 && errno == EINTR)
      {
        int t;
        t = errno;
        /* XXX - bash-5.0 */
        /* We check executing_builtin and run traps here for backwards
         * compatibility */
        if (executing_builtin)
          check_signals_and_traps (); /* XXX - should it be check_signals()? */
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
#undef NUM_INTR
#endif
#define NUM_INTR 3

  static ssize_t
  zreadretry (int fd, char *buf, size_t len)
  {
    for (int nintr = 0;;)
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
  zreset ()
  {
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
  static ssize_t
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

  /* from bashhist.cc */
#if defined(BANG_HISTORY)
  bool bash_history_inhibit_expansion (const char *, int);
#endif

  char *last_history_line ();

  void maybe_save_shell_history ();

  void bash_history_reinit (bool);

#if defined(HISTORY)
  void maybe_add_history (const char *);
#endif

  char *pre_process_line (const char *, bool, bool);

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

#if defined(PROCESS_SUBSTITUTION)
  void close_all_files ();
#endif

#if defined(ARRAY_VARS)
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
  enum redir_flags
  {
    RX_ACTIVE = 0x01,   /* do it; don't just go through the motions */
    RX_UNDOABLE = 0x02, /* make a list to undo these redirections */
    RX_CLEXEC = 0x04,   /* set close-on-exec for opened fds > 2 */
    RX_INTERNAL = 0x08,
    RX_USER = 0x10,
    RX_SAVCLEXEC = 0x20, /* set close-on-exec off in restored fd even though
                            saved on has it on */
    RX_SAVEFD = 0x40     /* fd used to save another even if < SHELL_FD_BASE */
  };

  void redirection_error (REDIRECT *, int, const char *);
  int do_redirections (REDIRECT *, redir_flags);
  char *redirection_expand (WORD_DESC *);
  int stdin_redirects (REDIRECT *);

  /* Functions from shell.cc */

  int parse_long_options (char **, int, int);
  int parse_shell_options (char **, int, int);
  int bind_args (char **, int, int, int);

  void start_debugger ();

  void
  add_shopt_to_alist (char *opt, int on_or_off)
  {
    shopt_alist.push_back (STRING_INT_ALIST (opt, on_or_off));
  }

  void run_shopt_alist ();

  void execute_env_file (char *);
  void run_startup_files ();
  int open_shell_script (const char *);
  void set_bash_input ();
  int run_one_command (const char *);

#if defined(WORDEXP_OPTION)
  int run_wordexp (const char *);
#endif

  bool uidget ();

  /* Some options are initialized to -1 so we have a way to determine whether
     they were set on the command line. This is an issue when listing the
     option values at invocation (`bash -o'), so we set the defaults here and
     reset them after the call to list_minus_o_options (). */
  /* XXX - could also do this for histexp_flag, jobs_m_flag */
  void
  set_option_defaults ()
  {
#if defined(HISTORY)
    enable_history_list = 0;
#endif
  }

  void
  reset_option_defaults ()
  {
#if defined(HISTORY)
    enable_history_list = -1;
#endif
  }

  void init_interactive ();
  void init_noninteractive ();
  void init_interactive_script ();

  void show_shell_usage (FILE *, bool);

  void init_long_args ();
  void set_shell_name (const char *);
  void shell_initialize ();
  void shell_reinitialize ();

  /* A wrapper for exit that (optionally) can do other things. */
  static void
  sh_exit (int s) __attribute__ ((__noreturn__))
  {
    std::exit (s);
  }

  void
  set_exit_status (int s)
  {
    set_pipestatus_from_exit (last_command_exit_value = s);
  }

  /* Functions from sig.cc */

  void
#if !defined(HAVE_SYS_SIGLIST) && !defined(HAVE_UNDER_SYS_SIGLIST)            \
    && !defined(HAVE_STRSIGNAL)
  initialize_signals (bool reinit)
#else
  initialize_signals (bool)
#endif
  {
    initialize_shell_signals ();
//   initialize_job_signals ();
#if !defined(HAVE_SYS_SIGLIST) && !defined(HAVE_UNDER_SYS_SIGLIST)            \
    && !defined(HAVE_STRSIGNAL)
    if (!reinit)
      initialize_siglist ();
#endif /* !HAVE_SYS_SIGLIST && !HAVE_UNDER_SYS_SIGLIST && !HAVE_STRSIGNAL */
  }

  void
  set_sigwinch_handler ()
  {
#if defined(SIGWINCH)
    old_winch = set_signal_handler (SIGWINCH, &sigwinch_sighandler_global);
#endif
  }

  void
  unset_sigwinch_handler ()
  {
#if defined(SIGWINCH)
    set_signal_handler (SIGWINCH, old_winch);
#endif
  }

  void initialize_terminating_signals ();
  void initialize_shell_signals ();
  void reset_terminating_signals ();
  void top_level_cleanup ();
  void throw_to_top_level ();
  void termsig_handler (int);
  void sigint_sighandler (int);
  void restore_default_signal (int);
  void free_trap_strings ();
  void reset_or_restore_signal_handlers (sh_resetsig_func_t);
  SigHandler set_sigint_handler ();
  int run_debug_trap ();

  void
  restore_sigmask ()
  {
#if defined(JOB_CONTROL) || defined(HAVE_POSIX_SIGNALS)
    sigprocmask (SIG_SETMASK, &top_level_mask, nullptr);
#endif
  }

  /* Functions from trap.cc */
  void run_pending_traps ();
  void initialize_traps ();
#ifdef DEBUG
  const char *trap_handler_string (int);
#endif
  int run_exit_trap ();
  void set_trap_state (int);
  int _run_trap_internal (int, const char *);
  void set_signal (int, const char *);
  void ignore_signal (int);

  bool
  signal_is_trapped (int sig)
  {
    return sigmodes[sig] & SIG_TRAPPED;
  }

  bool
  signal_is_pending (int sig)
  {
    return pending_traps[sig];
  }

  bool
  signal_is_special (int sig)
  {
    return sigmodes[sig] & SIG_SPECIAL;
  }

  bool
  signal_is_ignored (int sig)
  {
    return sigmodes[sig] & SIG_IGNORED;
  }

  bool
  signal_is_hard_ignored (int sig)
  {
    return sigmodes[sig] & SIG_HARD_IGNORE;
  }

  void
  set_signal_hard_ignored (int sig)
  {
    sigmodes[sig] |= SIG_HARD_IGNORE;
    original_signals[sig] = SIG_IGN;
  }

  void
  set_signal_ignored (int sig)
  {
    original_signals[sig] = SIG_IGN;
  }

  bool
  signal_in_progress (int sig)
  {
    return sigmodes[sig] & SIG_INPROGRESS;
  }

  void
  run_error_trap ()
  {
    if ((sigmodes[ERROR_TRAP] & SIG_TRAPPED)
        && ((sigmodes[ERROR_TRAP] & SIG_IGNORED) == 0)
        && (sigmodes[ERROR_TRAP] & SIG_INPROGRESS) == 0)
      _run_trap_internal (ERROR_TRAP, "error trap");
  }

  void
  run_return_trap ()
  {
    if ((sigmodes[RETURN_TRAP] & SIG_TRAPPED)
        && ((sigmodes[RETURN_TRAP] & SIG_IGNORED) == 0)
        && (sigmodes[RETURN_TRAP] & SIG_INPROGRESS) == 0)
      {
        int old_exit_value = last_command_exit_value;
        _run_trap_internal (RETURN_TRAP, "return trap");
        last_command_exit_value = old_exit_value;
      }
  }

  /* Run a trap set on SIGINT.  This is called from throw_to_top_level (), and
     declared here to localize the trap functions. */
  void
  run_interrupt_trap (int will_throw)
  {
    if (will_throw && running_trap > 0)
      run_trap_cleanup (running_trap - 1);
    pending_traps[SIGINT] = 0; /* run_pending_traps does this */
    catch_flag = 0;
    _run_trap_internal (SIGINT, "interrupt trap");
  }

  /* Free a trap command string associated with SIG without changing signal
     disposition.  Intended to be called from free_trap_strings()  */
  void
  free_trap_string (int sig)
  {
    change_signal (sig, reinterpret_cast<const char *> (DEFAULT_SIG));
    sigmodes[sig] &= ~SIG_TRAPPED; /* XXX - SIG_INPROGRESS? */
  }

  /* Reset the handler for SIG to the original value but leave the trap string
     in place. */
  void
  reset_signal (int sig)
  {
    set_signal_handler (sig, original_signals[sig]);
    sigmodes[sig] &= ~SIG_TRAPPED; /* XXX - SIG_INPROGRESS? */
  }

  /* Set the handler signal SIG to the original and free any trap
     command associated with it. */
  void
  restore_signal (int sig)
  {
    set_signal_handler (sig, original_signals[sig]);
    change_signal (sig, reinterpret_cast<const char *> (DEFAULT_SIG));
    sigmodes[sig] &= ~SIG_TRAPPED;
  }

  int
  next_pending_trap (int start)
  {
    for (int i = start; i < NSIG; i++)
      if (pending_traps[i])
        return i;
    return -1;
  }

  int
  first_pending_trap ()
  {
    return next_pending_trap (1);
  }

  /* Return > 0 if any of the "real" signals (not fake signals like EXIT) are
     trapped. */
  int
  any_signals_trapped ()
  {
    for (int i = 1; i < NSIG; i++)
      if (sigmodes[i] & SIG_TRAPPED)
        return i;
    return -1;
  }

  void
  clear_pending_traps ()
  {
    for (size_t i = 1; i < NSIG; i++)
      pending_traps[i] = 0;
  }

  void
  check_signals ()
  {
    CHECK_ALRM; /* set by the read builtin */
    QUIT;
  }

  void
  free_trap_command (int sig)
  {
    if ((sigmodes[sig] & SIG_TRAPPED) && trap_list[sig]
        && (trap_list[sig] != reinterpret_cast<const char *> (IGNORE_SIG))
        && (trap_list[sig] != reinterpret_cast<const char *> (DEFAULT_SIG))
        && (trap_list[sig] != IMPOSSIBLE_TRAP_NAME))
      delete[] const_cast<char *> (trap_list[sig]);
  }

  /* If SIG has a string assigned to it, get rid of it.  Then give it
     VALUE. */
  void
  change_signal (int sig, const char *value)
  {
    if ((sigmodes[sig] & SIG_INPROGRESS) == 0)
      free_trap_command (sig);
    trap_list[sig] = value;

    sigmodes[sig] |= SIG_TRAPPED;
    if (value == reinterpret_cast<const char *> (IGNORE_SIG))
      sigmodes[sig] |= SIG_IGNORED;
    else
      sigmodes[sig] &= ~SIG_IGNORED;
    if (sigmodes[sig] & SIG_INPROGRESS)
      sigmodes[sig] |= SIG_CHANGED;
  }

  void
  get_original_signal (int sig)
  {
    /* If we aren't sure the of the original value, then get it. */
    if (sig > 0 && sig < NSIG
        && original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER)
      GETORIGSIG (sig);
  }

  void
  get_all_original_signals ()
  {
    int i;

    for (i = 1; i < NSIG; i++)
      GET_ORIGINAL_SIGNAL (i);
  }

  void
  set_original_signal (int sig, SigHandler handler)
  {
    if (sig > 0 && sig < NSIG
        && original_signals[sig] == IMPOSSIBLE_TRAP_HANDLER)
      SETORIGSIG (sig, handler);
  }

  /* Convenience functions the rest of the shell can use */
  void
  check_signals_and_traps ()
  {
    check_signals ();
    run_pending_traps ();
  }

#if defined(JOB_CONTROL) && defined(SIGCHLD)

  /* Make COMMAND_STRING be executed when SIGCHLD is caught iff SIGCHLD
     is not already trapped.  IMPOSSIBLE_TRAP_HANDLER is used as a sentinel
     to make sure that a SIGCHLD trap handler run via run_sigchld_trap can
     reset the disposition to the default and not have the original signal
     accidentally restored, undoing the user's command. */
  void
  maybe_set_sigchld_trap (const char *command_string)
  {
    if ((sigmodes[SIGCHLD] & SIG_TRAPPED) == 0
        && trap_list[SIGCHLD] == IMPOSSIBLE_TRAP_NAME)
      set_signal (SIGCHLD, command_string);
  }

  /* Temporarily set the SIGCHLD trap string to IMPOSSIBLE_TRAP_HANDLER.  Used
     as a sentinel in run_sigchld_trap and maybe_set_sigchld_trap to see
     whether or not a SIGCHLD trap handler reset SIGCHLD disposition to the
     default. */
  void
  set_impossible_sigchld_trap ()
  {
    restore_default_signal (SIGCHLD);
    change_signal (SIGCHLD, IMPOSSIBLE_TRAP_NAME);
    sigmodes[SIGCHLD] &= ~SIG_TRAPPED; /* maybe_set_sigchld_trap checks this */
  }

  /* Act as if we received SIGCHLD NCHILD times and increment
     pending_traps[SIGCHLD] by that amount.  This allows us to still run the
     SIGCHLD trap once for each exited child. */
  void
  queue_sigchld_trap (int nchild)
  {
    if (nchild > 0)
      {
        catch_flag = 1;
        pending_traps[SIGCHLD] += nchild;
        trapped_signal_received = SIGCHLD;
      }
  }

#endif /* JOB_CONTROL && SIGCHLD */

  /* Set a trap for SIG only if SIG is not already trapped. */
  void
  trap_if_untrapped (int sig, const char *command)
  {
    if ((sigmodes[sig] & SIG_TRAPPED) == 0)
      set_signal (sig, command);
  }

  void
  set_debug_trap (const char *command)
  {
    set_signal (DEBUG_TRAP, command);
  }

  /* Separate function to call when functions and sourced files want to restore
     the original version of the DEBUG trap before returning.  Unless the -T
     option is set, source and shell function execution save the old debug trap
     and unset the trap.  If the function or sourced file changes the DEBUG
     trap, SIG_TRAPPED will be set and we don't bother restoring the original
     trap string. This is used by both functions and the source builtin. */
  void
  maybe_set_debug_trap (const char *command)
  {
    trap_if_untrapped (DEBUG_TRAP, command);
  }

  void
  set_error_trap (const char *command)
  {
    set_signal (ERROR_TRAP, command);
  }

  void
  maybe_set_error_trap (const char *command)
  {
    trap_if_untrapped (ERROR_TRAP, command);
  }

  void
  set_return_trap (char *command)
  {
    set_signal (RETURN_TRAP, command);
  }

  void
  maybe_set_return_trap (const char *command)
  {
    trap_if_untrapped (RETURN_TRAP, command);
  }

  void
  run_trap_cleanup (int sig)
  {
    /* XXX - should we clean up trap_list[sig] == IMPOSSIBLE_TRAP_HANDLER? */
    sigmodes[sig] &= ~(SIG_INPROGRESS | SIG_CHANGED);
  }

  /* Reset trapped signals to their original values, but don't free the
     trap strings.  Called by the command substitution code and other places
     that create a "subshell environment". */
  void
  reset_signal_handlers ()
  {
    reset_or_restore_signal_handlers (&Shell::reset_signal);
  }

  /* Reset all trapped signals to their original values.  Signals set to be
     ignored with trap '' SIGNAL should be ignored, so we make sure that they
     are.  Called by child processes after they are forked. */
  void
  restore_original_signals ()
  {
    reset_or_restore_signal_handlers (&Shell::restore_signal);
  }

  /* Return the correct handler for signal SIG according to the values in
     sigmodes[SIG]. */
  SigHandler
  trap_to_sighandler (int sig)
  {
    if (sigmodes[sig] & (SIG_IGNORED | SIG_HARD_IGNORE))
      return SIG_IGN;
    else if (sigmodes[sig] & SIG_TRAPPED)
      return &trap_handler_global;
    else
      return SIG_DFL;
  }

  /* If a trap handler exists for signal SIG, then call it; otherwise just
     return failure.  Returns true if it called the trap handler. */
  bool
  maybe_call_trap_handler (int sig)
  {
    /* Call the trap handler for SIG if the signal is trapped and not ignored.
     */
    if ((sigmodes[sig] & SIG_TRAPPED) && ((sigmodes[sig] & SIG_IGNORED) == 0))
      {
        switch (sig)
          {
          case SIGINT:
            run_interrupt_trap (0);
            break;
          case EXIT_TRAP:
            run_exit_trap ();
            break;
          case DEBUG_TRAP:
            run_debug_trap ();
            break;
          case ERROR_TRAP:
            run_error_trap ();
            break;
          default:
            trap_handler (sig);
            break;
          }
        return true;
      }
    else
      return false;
  }

  /* Print COMMAND (a command tree) on standard output. */
  void
  print_command (COMMAND *command)
  {
    std::printf ("%s", make_command_string (command));
  }

  /* Make a string which is the printed representation of the command
     tree in COMMAND.  We return this string.  However, the string
     buffer is reused, so you have to make a copy if you want it to
     remain around. */
  const char *
  make_command_string (COMMAND *command)
  {
    the_printed_command.clear ();
    was_heredoc = false;
    deferred_heredocs = nullptr;
    make_command_string_internal (command);
    return the_printed_command.c_str ();
  }

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
  char *
  extract_arithmetic_subst (const char *string, size_t *sindex)
  {
    return extract_delimited_string (string, sindex, "$[", "[", "]",
                                     SX_NOFLAGS); /*]*/
  }

#if defined(PROCESS_SUBSTITUTION)
  /* Extract the <( or >( construct in STRING, and return a new string.
     Start extracting at (SINDEX) as if we had just seen "<(".
     Make (SINDEX) get the position just after the matching ")". */
  std::string
  extract_process_subst (const char *string, size_t *sindex, sx_flags xflags)
  {
    xflags |= (no_throw_on_fatal_error ? SX_NOTHROW : SX_NOFLAGS);
    return xparse_dolparen (string, sindex, xflags);
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
  char *get_dollar_var_value (int64_t);

  /* Quote a string to protect it from word splitting. */
  char *quote_string (const char *);

  /* Quote escape characters (characters special to internals of expansion)
     in a string. */
  char *quote_escapes (const char *);

  /* And remove such quoted special characters. */
  char *remove_quoted_escapes (char *);

  /* Remove CTLNUL characters from STRING unless they are quoted with CTLESC.
   */
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

#if defined(PROCESS_SUBSTITUTION)
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

#if defined(ARRAY_VARS)
  char *extract_array_assignment_list (const char *, int *);
#endif

#if defined(COND_COMMAND)
  char *remove_backslashes (const char *);
  char *cond_expand_word (WORD_DESC *, int);
  void cond_error ();
#endif

  size_t skip_to_delim (const char *, size_t, const char *, sd_flags);

#if defined(BANG_HISTORY)
  size_t skip_to_histexp (const char *, size_t, const char *, sd_flags);
#endif

#if defined(READLINE)
  unsigned int char_is_quoted (const char *, int); /* rl_linebuf_func_t */
  bool unclosed_pair (const char *, int, const char *);
  WORD_LIST *split_at_delims (const char *, int, const char *, int, sd_flags,
                              int *, int *);
#endif

  void invalidate_cached_quoted_dollar_at ();

  // private methods from subst.c

  char *string_extract (const char *, size_t *, const char *, sx_flags);

  char *string_extract_double_quoted (const char *, size_t *, sx_flags);

  size_t skip_double_quoted (const char *, size_t, size_t, sx_flags);

  char *string_extract_single_quoted (const char *, size_t *);

  size_t skip_single_quoted (const char *, size_t, size_t, sx_flags);

  char *string_extract_verbatim (const char *, size_t, size_t *, const char *,
                                 sx_flags);

  char *extract_delimited_string (const char *, size_t *, const char *,
                                  const char *, const char *, sx_flags);

  char *extract_dollar_brace_string (const char *, size_t *, quoted_flags,
                                     sx_flags);

  void exp_throw_to_top_level (const std::exception &);

#if defined(ARRAY_VARS)
  size_t skip_matched_pair (const char *, size_t, char, char, int);

  /* Flags has 1 as a reserved value, since skip_matched_pair uses it for
     skipping over quoted strings and taking the first instance of the
     closing character. */
  size_t
  skipsubscript (const char *string, size_t start, int flags)
  {
    return skip_matched_pair (string, start, '[', ']', flags);
  }
#endif

  /* Evaluates to true if C is a character in $IFS. */
  bool
  isifs (char c)
  {
    return ifs_cmap[static_cast<unsigned char> (c)];
  }

  SHELL_VAR *do_compound_assignment (const char *, const char *, assign_flags);

  // from variables.cc

  void validate_inherited_value (SHELL_VAR *, int);

  SHELL_VAR *set_if_not (const char *, const char *);

  virtual void sh_set_lines_and_columns (unsigned int,
                                         unsigned int) RL_OVERRIDE;
  void set_pwd ();
  void set_ppid ();
  void make_funcname_visible (bool);

  void initialize_shell_variables (char **, bool);

  void delete_all_variables (HASH_TABLE<SHELL_VAR *> *);

  void reinit_special_variables ();

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
  SHELL_VAR *bind_variable (const char *, const char *, int);
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
#if defined(ARRAY_VARS)
  SHELL_VAR **all_array ();
#endif
  char **all_matching_prefix (const char *);

  char **add_or_supercede_exported_var (const char *, int);

  char *get_variable_value (SHELL_VAR *);
  char *get_string_value (const char *);
  char *make_variable_value (SHELL_VAR *, const char *, int);

  /* These four are virtual callbacks when Readline is used. */

  const char *sh_get_env_value (const char *) override;
  std::string sh_single_quote (const std::string &) override;
  std::string sh_get_home_dir () override;
  int sh_unset_nodelay_mode (int) override;

  SHELL_VAR *bind_variable_value (SHELL_VAR *, const char *, int);
  SHELL_VAR *bind_int_variable (const char *, const char *, assign_flags);
  SHELL_VAR *bind_var_to_int (const char *, int64_t);

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
  // void delete_all_vars (HASH_TABLE *);
  void delete_all_contexts (VAR_CONTEXT *);

  VAR_CONTEXT *new_var_context (const char *, int);
  void dispose_var_context (VAR_CONTEXT *);
  // VAR_CONTEXT *push_var_context (const char *, int, HASH_TABLE *);
  void pop_var_context ();
  // VAR_CONTEXT *push_scope (int, HASH_TABLE *);
  int pop_scope (int);

  void clear_dollar_vars ();

  // void push_context (const char *, bool, HASH_TABLE *);
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
  void put_gnu_argv_flags_into_env (int64_t, const char *);

  void print_var_list (SHELL_VAR **);
  void print_func_list (SHELL_VAR **);
  void print_assignment (SHELL_VAR *);
  void print_var_value (SHELL_VAR *, int);
  void print_var_function (SHELL_VAR *);

#if defined(ARRAY_VARS)
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

#if defined(READLINE)
  void sv_comp_wordbreaks (const char *);
  void sv_terminal (const char *);
  void sv_hostfile (const char *);
  void sv_winsize (const char *);
#endif

#if defined(__CYGWIN__)
  void sv_home (const char *);
#endif

#if defined(HISTORY)
  void sv_histsize (const char *);
  void sv_histignore (const char *);
  void sv_history_control (const char *);
#if defined(BANG_HISTORY)
  void sv_histchars (const char *);
#endif
  void sv_histtimefmt (const char *);
#endif /* HISTORY */

#if defined(HAVE_TZSET)
  void sv_tz (const char *);
#endif

#if defined(JOB_CONTROL)
  void sv_childmax (const char *);
#endif

  // Methods implemented in variables.cc.

  void create_variable_tables ();

  /* The variable in NAME has just had its state changed.  Check to see if it
     is one of the special ones where something special happens. */
  void stupidly_hack_special_variables (const char *);

  // Methods implemented in parse.yy.

  int yyparse ();

  // Methods implemented in parser.cc.

  std::string xparse_dolparen (const std::string &, size_t *, sx_flags);
  int return_EOF ();
  void push_token (parser::token::token_kind_type);
  void reset_parser ();
  WORD_LIST *parse_string_to_word_list (char *, int, const char *);
  void init_yy_io (sh_cget_func_t, sh_cunget_func_t, stream_type, const char *,
                   INPUT_STREAM);
  void with_input_from_stdin ();
  void with_input_from_stream (FILE *, const char *);
  void initialize_bash_input ();
  void execute_variable_command (const char *, const char *);
  std::string parse_comsub (int, int, int, pgroup_flags);
  int parse_arith_cmd (char **, int);

#if defined(ARRAY_VARS)
  int token_is_assignment (char *, int);
  int token_is_ident (char *, int);
#endif

  /* Return true if a stream of type TYPE is saved on the stack. */
  bool
  stream_on_stack (stream_type type)
  {
    STREAM_SAVER *s;

    for (s = stream_list; s; s = s->next ())
      if (s->bash_input.type == type)
        return true;

    return false;
  }

  const char *
  yy_input_name ()
  {
    return bash_input.name ? bash_input.name : "stdin";
  }

  /* Get the next character of input. */
  int
  yy_getc ()
  {
    return ((*this).*bash_input.getter) ();
  }

  /* Unget C. This makes C the next character to be read. */
  int
  yy_ungetc (int c)
  {
    return ((*this).*bash_input.ungetter) (c);
  }

  void set_line_mbstate ();
  void pop_string ();
  void free_string_list ();

  // Methods in lib/tilde/tilde.cc.

  size_t tilde_find_prefix (const char *, size_t *);
  char *tilde_expand_word (const char *);
  size_t tilde_find_suffix (const char *);
  char *tilde_expand (const char *);

  // Methods in array.cc.

  alias_t *find_alias (const char *);

  // Methods in arrayfunc.cc.

  bool valid_array_reference (const char *, valid_array_flags);

  // Methods in general.cc.

  // Enable/disable POSIX mode, restoring any saved state.
  void posix_initialize (bool);

  char *
  get_posix_options (char *bitmap)
  {
    if (bitmap == nullptr)
      bitmap = new char[NUM_POSIX_VARS];

    for (int i = 0; i < NUM_POSIX_VARS; i++)
      bitmap[i] = *(posix_vars[i]);
    return bitmap;
  }

  void
  save_posix_options ()
  {
    saved_posix_vars = get_posix_options (saved_posix_vars);
  }

  void
  set_posix_options (const char *bitmap)
  {
    for (int i = 0; i < NUM_POSIX_VARS; i++)
      *(posix_vars[i]) = bitmap[i];
  }

  bool valid_nameref_value (const char *, valid_array_flags);
  bool check_selfref (const char *, const char *);
  bool check_identifier (WORD_DESC *, bool);

  // inlines moved from general.cc.

  bool
  exportable_function_name (const char *string)
  {
    if (absolute_program (string))
      return false;
    if (mbschr (string, '=') != nullptr)
      return false;
    return true;
  }

  bool legal_alias_name (const char *);
  size_t assignment (const char *, int);

  char *make_absolute (const char *, const char *);
  char *full_pathname (char *);

  /* Return 1 if STRING is an absolute program name; it is absolute if it
     contains any slashes.  This is used to decide whether or not to look
     up through $PATH. */
  bool
  absolute_program (const char *string)
  {
    return mbschr (string, '/') != nullptr;
  }

  const char *polite_directory_format (const char *);
  char *trim_pathname (char *);

  char *bash_special_tilde_expansions (char *);

  void tilde_initialize ();

  char *bash_tilde_expand (const char *, int);

  void initialize_group_array ();

  char **get_group_list (int *);
  int *get_group_array (int *);

  int default_columns ();

#ifdef DEBUG
  void debug_parser (parser::debug_level_type);
#endif

  void push_string (char *, int, alias_t *);

#if defined(ALIAS)
  void clear_string_list_expander (alias_t *);
#endif

  const char *read_a_line (bool);
  const char *read_secondary_line (bool);
  void prompt_again ();

  int shell_getc (bool);
  void shell_ungetc (int);

  const char *parser_remaining_input ();
  void discard_until (int);

  parser::symbol_type read_token (int);
  parser::symbol_type read_token_word (int);

#if defined(ALIAS)

  enum alias_expand_token_result{ RE_READ_TOKEN = -99, NO_EXPANSION = -100 };
  alias_expand_token_result alias_expand_token (const char *);

#endif

  bool time_command_acceptable ();

  int special_case_tokens (const char *);

  void
  reset_readahead_token ()
  {
    if (token_to_read == '\n')
      token_to_read = 0;
  }

  std::string parse_matched_pair (int, int, int, pgroup_flags);

  /* This is kind of bogus -- we slip a mini recursive-descent parser in
     here to handle the conditional statement syntax. */
  COMMAND *
  parse_cond_command ()
  {
    COND_COM *cexp;

    global_extglob = extended_glob;
    cexp = cond_expr ();
    return cexp;
  }

  COND_COM *
  cond_expr ()
  {
    return cond_or ();
  }

  COND_COM *
  cond_or ()
  {
    COND_COM *l, *r;

    l = cond_and ();
    if (cond_token == parser::token::OR_OR)
      {
        r = cond_or ();
        l = new COND_COM (line_number, COND_OR, nullptr, l, r);
      }
    return l;
  }

  COND_COM *
  cond_and ()
  {
    COND_COM *l, *r;

    l = cond_term ();
    if (cond_token == parser::token::AND_AND)
      {
        r = cond_and ();
        l = new COND_COM (line_number, COND_AND, nullptr, l, r);
      }
    return l;
  }

  int cond_skip_newlines ();
  COND_COM *cond_term ();

#if defined(DPAREN_ARITHMETIC) || defined(ARITH_FOR_COMMAND)
  int parse_dparen (int);
#endif

  /* Return true if TOKSYM is a token that after being read would allow
     a reserved word to be seen, else false. */
  bool
  reserved_word_acceptable (int toksym)
  {
    switch (toksym)
      {
      case '\n':
      case ';':
      case '(':
      case ')':
      case '|':
      case '&':
      case '{':
      case '}': /* XXX */
      case parser::token::AND_AND:
      case parser::token::BANG:
      case parser::token::BAR_AND:
      case parser::token::DO:
      case parser::token::DONE:
      case parser::token::ELIF:
      case parser::token::ELSE:
      case parser::token::ESAC:
      case parser::token::FI:
      case parser::token::IF:
      case parser::token::OR_OR:
      case parser::token::SEMI_SEMI:
      case parser::token::SEMI_AND:
      case parser::token::SEMI_SEMI_AND:
      case parser::token::THEN:
      case parser::token::TIME:
      case parser::token::TIMEOPT:
      case parser::token::TIMEIGN:
      case parser::token::COPROC:
      case parser::token::UNTIL:
      case parser::token::WHILE:
      case 0:
        return true;
      default:
#if defined(COPROCESS_SUPPORT)
        if (last_read_token == parser::token::WORD
            && token_before_that == parser::token::COPROC)
          return true;
#endif
        if (last_read_token == parser::token::WORD
            && token_before_that == parser::token::FUNCTION)
          return true;

        return false;
      }
  }

  int
  get_current_prompt_level ()
  {
    return (current_prompt_string && current_prompt_string == ps2_prompt) ? 2
                                                                          : 1;
  }

  void
  set_current_prompt_level (int x)
  {
    prompt_string_pointer = (x == 2) ? &ps2_prompt : &ps1_prompt;
    current_prompt_string = *prompt_string_pointer;
  }

  void
  print_prompt ()
  {
    std::fprintf (stderr, "%s", current_decoded_prompt);
    std::fflush (stderr);
  }

  char *parse_compound_assignment (size_t *);

  // methods from alias.cc

  void initialize_aliases ();

  /* ************************************************************** */
  /*		Private Shell Variables (ptr types)		    */
  /* ************************************************************** */

#if defined(NO_MAIN_ENV_ARG)
  char **environ; /* used if no third argument to main() */
#endif

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

public:
  // The following are public for access from the Bison parser.

  COMMAND *global_command;

  // Accessed by print () methods.
  REDIRECT *deferred_heredocs;

protected:
  // Back to protected access.

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

  const char *command_execution_string; /* argument to -c option */
  const char *shell_script_filename;    /* shell script */

  FILE *default_input;

  std::vector<STRING_INT_ALIST> shopt_alist;

  /* The thing that we build the array of builtins out of. */
  struct Builtin
  {
    const char *name;            /* The name that the user types. */
    sh_builtin_func_t function;  /* The address of the invoked function. */
    const char *const *long_doc; /* NULL terminated array of strings. */
    const char *short_doc;       /* Short version of documentation. */
    void *handle;                /* dlsym() handle */
    builtin_flags flags;         /* One or more of the #defines above. */
  };

  // TODO: initialize this vector at startup from generated builtins.cc.
  std::vector<Builtin> shell_builtins;

  jobstats js;

  PROCESS *last_procsub_child;

  //  ps_index_t *pidstat_table;	// FIXME: what type is this

  bgpids bgpids;

  procchain procsubs;

  /* The array of known jobs. */
  std::vector<JOB *> jobs;

  /* The pipeline currently being built. */
  PROCESS *the_pipeline;

#if defined(ARRAY_VARS)
  int *pstatuses; /* list of pipeline statuses */
  size_t statsize;
#endif

  /* The list of shell variables that the user has created at the global
     scope, or that came from the environment. */
  VAR_CONTEXT *global_variables;

  /* The current list of shell variables, including function scopes */
  VAR_CONTEXT *shell_variables;

  /* The list of shell functions that the user has created, or that came from
     the environment. */
  HASH_TABLE<SHELL_VAR *> *shell_functions;

#if defined(DEBUGGER)
  /* The table of shell function definitions that the user defined or that
     came from the environment. */
  HASH_TABLE<FUNCTION_DEF *> *shell_function_defs;
#endif

  /* The set of shell assignments which are made only in the environment
     for a single command. */
  HASH_TABLE<SHELL_VAR *> *temporary_env;

  /* Some funky variables which are known about specially.  Here is where
     "$*", "$1", and all the cruft is kept. */
  char *dollar_vars[10];

  WORD_LIST *rest_of_args;

  /* An array which is passed to commands as their environment.  It is
     manufactured from the union of the initial environment and the
     shell variables that are marked for export. */
  char **export_env;

  HASH_TABLE<SHELL_VAR *> *last_table_searched; /* hash_lookup sets this */
  VAR_CONTEXT *last_context_searched;

  /* variables from common.c */
  sh_builtin_func_t this_shell_builtin;
  sh_builtin_func_t last_shell_builtin;

  std::string the_current_working_directory;

  // The buffer used by print_cmd.cc. */
  std::string the_printed_command;

  // The FILE pointer used for xtrace.
  FILE *xtrace_fp;

  // Buffer to indicate the indirection level (PS4) when set -x is enabled.
  std::string indirection_string;

  // Buffer for the indentation string.
  std::string indentation_string;

  /* The printed representation of the currently-executing command (same as
     the_printed_command), except when a trap is being executed.  Useful for
     a debugger to know where exactly the program is currently executing. */
  std::string the_printed_command_except_trap;

  /* The name of the command that is currently being executed.
     `test' needs this, for example. */
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

  /* The list of things to do originally, before we started trapping. */
  SigHandler original_signals[NSIG];

  /* For each signal, a slot for a string, which is a command to be
     executed when that signal is received.  The slot can also contain
     DEFAULT_SIG, which means do whatever you were going to do before
     you were so rudely interrupted, or IGNORE_SIG, which says ignore
     this signal. */
  const char *trap_list[BASH_NSIG];

  /* private variables from subst.cc */

  /* Variables used to keep track of the characters in IFS. */
  SHELL_VAR *ifs_var;
  const char *ifs_value;

  /* Used to hold a list of variable assignments preceding a command.
     TODO: make sure SIGCHLD trap handlers can save and restore this. */
  WORD_LIST *subst_assign_varlist;

  WORD_LIST *cached_quoted_dollar_at;

  /* A WORD_LIST of words to be expanded by expand_word_list_internal,
     without any leading variable assignments. */
  WORD_LIST *garglist;

  /* variables from lib/sh/getenv.cc */

  char *last_tempenv_value;

  /* variables from lib/sh/shtty.cc */

  TTYSTRUCT ttin, ttout;

  /* local buffer for lib/sh/zread.c functions */
  char *zread_lbuf;

  /* moved from lib/tilde/tilde.cc */

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

#if defined(HAVE_ICONV)
  iconv_t localconv;
#endif

  // Previously-global variables moved from parse.yy.

  /* Default prompt strings */
  const char *primary_prompt;
  const char *secondary_prompt;

  /* PROMPT_STRING_POINTER points to one of these, never to an actual string.
   */
  const char *ps1_prompt, *ps2_prompt;

  /* Displayed after reading a command but before executing it in an
   * interactive shell */
  const char *ps0_prompt;

  /* Handle on the current prompt string.  Indirectly points through
     ps1_ or ps2_prompt. */
  const char **prompt_string_pointer;
  const char *current_prompt_string;

  /* Variables to manage the task of reading here documents, because we need to
     defer the reading until after a complete command has been collected. */
  REDIRECT *redir_stack[HEREDOC_MAX];

  /* Where shell input comes from.  History expansion is performed on each
     line when the shell is interactive. */
  std::string shell_input_line;
  size_t shell_input_line_index;

  /* The decoded prompt string.  Used if READLINE is not defined or if
     editing is turned off.  Analogous to current_readline_prompt. */
  char *current_decoded_prompt;

#if defined(READLINE)
  char *current_readline_prompt;
  std::string current_readline_line;
  size_t current_readline_line_index;
#endif

  std::string read_a_line_buffer;

  WORD_DESC *word_desc_to_read;

  REDIRECTEE source;
  REDIRECTEE redir;

  FILE *yyoutstream;
  FILE *yyerrstream;

  struct BASH_INPUT
  {
    stream_type type;
    char *name; /* freed by the parser */
    INPUT_STREAM location;
    sh_cget_func_t getter;
    sh_cunget_func_t ungetter;
  };

  BASH_INPUT bash_input;

  class STREAM_SAVER : public GENERIC_LIST
  {
  public:
    STREAM_SAVER () : GENERIC_LIST () {}

    STREAM_SAVER *
    next ()
    {
      return static_cast<STREAM_SAVER *> (next_);
    }

    BASH_INPUT bash_input;
    int line;

#if defined(BUFFERED_INPUT)
    BUFFERED_STREAM *bstream;
#endif /* BUFFERED_INPUT */
  };

  parser parser;

  STREAM_SAVER *stream_list;

  STRING_SAVER *pushed_string_list;

  /* The set of groups that this user is a member of. */
  GETGROUPS_T *group_array;

#if defined(SIGWINCH)
  void (*old_winch) (int);
#endif

#if defined(JOB_CONTROL) || defined(HAVE_POSIX_SIGNALS)
  /* The signal masks that this shell runs with. */
  sigset_t top_level_mask;
#endif /* JOB_CONTROL */

  /* This provides a way to map from a file descriptor to the buffer
     associated with that file descriptor, rather than just the other
     way around.  This is needed so that buffers are managed properly
     in constructs like 3<&4.  buffers[x]->b_fd == x -- that is how the
     correspondence is maintained. */
  std::vector<BUFFERED_STREAM *> buffers;

  /* Some long-winded argument names. */
  typedef enum
  {
    Bool,
    Flag,
    Charp
  } arg_type;

  struct LongArg
  {
    const char *name;
    const arg_type type;
    union
    {
      bool *bool_ptr;
      char *flag_ptr;
      const char **charp_ptr;
    } value;

    LongArg () : name (nullptr), type (Bool) { value.bool_ptr = nullptr; }

    LongArg (const char *n, bool *bp) : name (n), type (Bool)
    {
      value.bool_ptr = bp;
    }

    LongArg (const char *n, char *fp) : name (n), type (Flag)
    {
      value.flag_ptr = fp;
    }

    LongArg (const char *n, const char **cpp) : name (n), type (Charp)
    {
      value.charp_ptr = cpp;
    }
  };

  // List of long command-line options and pointers to variables to set
  std::vector<LongArg> long_args;

  char *posix_vars[NUM_POSIX_VARS];

  // Saved POSIX settings.
  char *saved_posix_vars;

  char **bash_tilde_prefixes;
  char **bash_tilde_prefixes2;
  char **bash_tilde_suffixes;
  char **bash_tilde_suffixes2;

  char **group_vector;
  int *group_iarray;

  /* These are used by read_token_word, in parser.cc. */

  // The primary delimiter stack.
  std::vector<char> dstack;

  /* A temporary delimiter stack to be used when decoding prompt strings.
     This is needed because command substitutions in prompt strings (e.g. PS2)
     can screw up the parser's quoting state. */
  std::vector<char> temp_dstack;

  std::string shell_input_line_property;

  // The list of aliases that we have.
  HASH_TABLE<alias_t> aliases;

  // Buffer for buffered input.
  char localbuf[1024];
  int local_index;
  int local_bufused;

  // Temporary directory name buffer.
  char tdir_buf[PATH_MAX];
};

struct sh_input_line_state_t
{
  char *input_line;
  size_t input_line_index;
  size_t input_line_size;
  size_t input_line_len;
#if defined(HANDLE_MULTIBYTE)
  char *input_property;
  size_t input_propsize;
#endif
};

// Structure in which to save partial parsing state when doing things like
// PROMPT_COMMAND and bash_execute_unix_command execution.
struct sh_parser_state_t
{
  // pointer types first

  int *token_state;     // array of 4 ints; free with delete[]
  char *token;

  const char **prompt_string_pointer;

  /* parsing state */
  pstate_flags parser_state;

#if defined(ARRAY_VARS)
  ARRAY *pipestatus;
#endif

  Shell::sh_builtin_func_t last_shell_builtin;
  Shell::sh_builtin_func_t this_shell_builtin;

  /* structures affecting the parser */
  REDIRECT *redir_stack[HEREDOC_MAX];

  // 32-bit int types next

  int token_buffer_size;  // ???

  /* input line state -- line number saved elsewhere */
  int input_line_terminator;
  int eof_encountered;

  /* history state affecting or modified by the parser */
  int current_command_line_count;
#if defined(HISTORY)
  int remember_on_history;
  int history_expansion_inhibited;
#endif

};

} // namespace bash

#endif /* _SHELL_H_ */