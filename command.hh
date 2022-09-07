/* command.h -- The structures used internally to represent commands, and
   the extern declarations of the functions used to create them. */

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

#if !defined (_COMMAND_H_)
#define _COMMAND_H_

#include "general.hh"
#include "shmbutil.hh"

#include <vector>
#include <string>

namespace bash
{

/* Instructions describing what kind of thing to do for a redirection. */
enum r_instruction {
  r_output_direction, r_input_direction, r_inputa_direction,
  r_appending_to, r_reading_until, r_reading_string,
  r_duplicating_input, r_duplicating_output, r_deblank_reading_until,
  r_close_this, r_err_and_out, r_input_output, r_output_force,
  r_duplicating_input_word, r_duplicating_output_word,
  r_move_input, r_move_output, r_move_input_word, r_move_output_word,
  r_append_err_and_out
};

/* Redirection flags; values for rflags */
constexpr int REDIR_VARASSIGN = 0x01;

/* Redirection errors. */
enum redir_error_type {
  AMBIGUOUS_REDIRECT =	-1,
  NOCLOBBER_REDIRECT =	-2,
  RESTRICTED_REDIRECT =	-3,	/* can only happen in restricted shells. */
  HEREDOC_REDIRECT =	-4,	/* here-doc temp file can't be created */
  BADVAR_REDIRECT =	-5	/* something wrong with {varname}redir */
};

static inline bool CLOBBERING_REDIRECT (r_instruction ri) {
  return (ri == r_output_direction || ri == r_err_and_out);
}

static inline bool OUTPUT_REDIRECT (r_instruction ri) {
  return (ri == r_output_direction || ri == r_input_output ||
	  ri == r_err_and_out || ri == r_append_err_and_out);
}

static inline bool INPUT_REDIRECT (r_instruction ri) {
  return (ri == r_input_direction || ri == r_inputa_direction || ri == r_input_output);
}

static inline bool WRITE_REDIRECT (r_instruction ri) {
  return (ri == r_output_direction ||
	  ri == r_input_output ||
	  ri == r_err_and_out ||
	  ri == r_appending_to ||
	  ri == r_append_err_and_out ||
	  ri == r_output_force);
}

/* redirection needs translation */
static inline bool TRANSLATE_REDIRECT (r_instruction ri) {
  return (ri == r_duplicating_input_word || ri == r_duplicating_output_word ||
	  ri == r_move_input_word || ri == r_move_output_word);
}

/* Possible values for the `flags' field of a WORD_DESC. */
enum word_desc_flags {
  W_NOFLAGS =		      0,
  W_HASDOLLAR =		(1 << 0),	/* Dollar sign present. */
  W_QUOTED =		(1 << 1),	/* Some form of quote character is present. */
  W_ASSIGNMENT =	(1 << 2),	/* This word is a variable assignment. */
  W_SPLITSPACE =	(1 << 3),	/* Split this word on " " regardless of IFS */
  W_NOSPLIT =		(1 << 4),	/* Do not perform word splitting on this word because ifs is empty string. */
  W_NOGLOB =		(1 << 5),	/* Do not perform globbing on this word. */
  W_NOSPLIT2 =		(1 << 6),	/* Don't split word except for $@ expansion (using spaces) because context does not allow it. */
  W_TILDEEXP =		(1 << 7),	/* Tilde expand this assignment word */
  W_DOLLARAT =		(1 << 8),	/* $@ and its special handling -- UNUSED */
  W_DOLLARSTAR =	(1 << 9),	/* $* and its special handling -- UNUSED */
  W_NOCOMSUB =		(1 << 10),	/* Don't perform command substitution on this word */
  W_ASSIGNRHS =		(1 << 11),	/* Word is rhs of an assignment statement */
  W_NOTILDE =		(1 << 12),	/* Don't perform tilde expansion on this word */
  W_ITILDE =		(1 << 13),	/* Internal flag for word expansion */
  W_EXPANDRHS =		(1 << 14),	/* Expanding word in ${paramOPword} */
  W_COMPASSIGN =	(1 << 15),	/* Compound assignment */
  W_ASSNBLTIN =		(1 << 16),	/* word is a builtin command that takes assignments */
  W_ASSIGNARG =		(1 << 17),	/* word is assignment argument to command */
  W_HASQUOTEDNULL = 	(1 << 18),	/* word contains a quoted null character */
  W_DQUOTE =		(1 << 19),	/* word should be treated as if double-quoted */
  W_NOPROCSUB =		(1 << 20),	/* don't perform process substitution */
  W_SAWQUOTEDNULL = 	(1 << 21),	/* word contained a quoted null that was removed */
  W_ASSIGNASSOC =	(1 << 22),	/* word looks like associative array assignment */
  W_ASSIGNARRAY =	(1 << 23),	/* word looks like a compound indexed array assignment */
  W_ARRAYIND =		(1 << 24),	/* word is an array index being expanded */
  W_ASSNGLOBAL =	(1 << 25),	/* word is a global assignment to declare (declare/typeset -g) */
  W_NOBRACE =		(1 << 26),	/* Don't perform brace expansion */
  W_COMPLETE =		(1 << 27),	/* word is being expanded for completion */
  W_CHKLOCAL =		(1 << 28),	/* check for local vars on assignment */
  W_NOASSNTILDE =	(1 << 29),	/* don't do tilde expansion like an assignment statement */
  W_FORCELOCAL =	(1 << 30)	/* force assignments to be to local variables, non-fatal on assignment errors */
};

static inline word_desc_flags&
operator |= (word_desc_flags &a, const word_desc_flags &b) {
  a = static_cast<word_desc_flags> (static_cast<uint32_t> (a) | static_cast<uint32_t> (b));
  return a;
}

static inline word_desc_flags
operator | (const word_desc_flags &a, const word_desc_flags &b) {
  return static_cast<word_desc_flags> (static_cast<uint32_t> (a) | static_cast<uint32_t> (b));
}

static inline word_desc_flags&
operator &= (word_desc_flags &a, const word_desc_flags &b) {
  a = static_cast<word_desc_flags> (static_cast<uint32_t> (a) & static_cast<uint32_t> (b));
  return a;
}

static inline word_desc_flags
operator & (const word_desc_flags &a, const word_desc_flags &b) {
  return static_cast<word_desc_flags> (static_cast<uint32_t> (a) & static_cast<uint32_t> (b));
}

static inline word_desc_flags
operator ~ (const word_desc_flags &a) {
  return static_cast<word_desc_flags> (~static_cast<uint32_t> (a));
}


/* Flags for the `pflags' argument to param_expand() and various
   parameter_brace_expand_xxx functions; also used for string_list_dollar_at */
enum param_flags {
  PF_NOFLAGS =		   0,
  PF_NOCOMSUB =		0x01,	/* Do not perform command substitution */
  PF_IGNUNBOUND =	0x02,	/* ignore unbound vars even if -u set */
  PF_NOSPLIT2 =		0x04,	/* same as W_NOSPLIT2 */
  PF_ASSIGNRHS =	0x08,	/* same as W_ASSIGNRHS */
  PF_COMPLETE =		0x10,	/* same as W_COMPLETE, sets SX_COMPLETE */
  PF_EXPANDRHS =	0x20,	/* same as W_EXPANDRHS */
  PF_ALLINDS =		0x40	/* array, act as if [@] was supplied */
};

/* Possible values for subshell_environment */
enum subshell_flags {
  SUBSHELL_NOFLAGS =	    0,
  SUBSHELL_ASYNC =	 0x01,	/* subshell caused by `command &' */
  SUBSHELL_PAREN =	 0x02,	/* subshell caused by ( ... ) */
  SUBSHELL_COMSUB =	 0x04,	/* subshell caused by `command` or $(command) */
  SUBSHELL_FORK =	 0x08,	/* subshell caused by executing a disk command */
  SUBSHELL_PIPE =	 0x10,	/* subshell from a pipeline element */
  SUBSHELL_PROCSUB =	 0x20,	/* subshell caused by <(command) or >(command) */
  SUBSHELL_COPROC =	 0x40,	/* subshell from a coproc pipeline */
  SUBSHELL_RESETTRAP =	 0x80,	/* subshell needs to reset trap strings on first call to trap */
  SUBSHELL_IGNTRAP =	0x100	/* subshell should reset trapped signals from trap_handler */
};

/* A structure which represents a word. */
class WORD_DESC {
public:
  WORD_DESC() : flags(W_NOFLAGS) {}
  WORD_DESC(const std::string &w) : word(w), flags(W_NOFLAGS) {}
  WORD_DESC(const char *w) : word(w), flags(W_NOFLAGS) {}
  WORD_DESC(const WORD_DESC &w) : word(w.word), flags(w.flags) {}
#if __cplusplus >= 201103L
  WORD_DESC(WORD_DESC &&w) noexcept : word(std::move(w.word)), flags(w.flags) {}
#endif

  std::string word;			/* C++ string. */
  word_desc_flags flags;		/* Flags associated with this word. */
  int _pad;				// silence clang -Wpadded warning
};

/* A vector of pointers to words. */
typedef std::vector<WORD_DESC *> WORD_LIST;


/* **************************************************************** */
/*								    */
/*			Shell Command Structs			    */
/*								    */
/* **************************************************************** */

/* What a redirection descriptor looks like.  If the redirection instruction
   is ri_duplicating_input or ri_duplicating_output, use DEST, otherwise
   use the file in FILENAME.  Out-of-range descriptors are identified by a
   negative DEST. */

union REDIRECTEE {
  int dest;			/* Place to redirect REDIRECTOR to, or ... */
  WORD_DESC *filename;		/* filename to redirect to. */
};

/* Structure describing a redirection.  If REDIRECTOR is negative, the parser
   (or translator in redir.c) encountered an out-of-range file descriptor. */
struct REDIRECT {
  char *here_doc_eof;		/* The word that appeared in <<foo. */
  REDIRECTEE redirector;	/* Descriptor or varname to be redirected. */
  REDIRECTEE redirectee;	/* File descriptor or filename */
  uint32_t rflags;		/* Private flags for this redirection */
  uint32_t flags;		/* Flag value for `open'. */
  r_instruction instruction;	/* What to do with the information. */
  int _pad;			// silence clang -Wpadded warning
};

/* An element used in parsing.  A single word or a single redirection.
   This is an ephemeral construct. */
struct ELEMENT {
  WORD_DESC *word;
  REDIRECT *redirect;
};

/* Possible values for command->flags. */
enum cmd_flags {
  CMD_NOFLAGS =		     0,
  CMD_WANT_SUBSHELL =	  0x01,	/* User wants a subshell: ( command ) */
  CMD_FORCE_SUBSHELL =	  0x02,	/* Shell needs to force a subshell. */
  CMD_INVERT_RETURN =	  0x04,	/* Invert the exit value. */
  CMD_IGNORE_RETURN =	  0x08,	/* Ignore the exit value.  For set -e. */
  CMD_NO_FUNCTIONS =	  0x10,	/* Ignore functions during command lookup. */
  CMD_INHIBIT_EXPANSION = 0x20,	/* Do not expand the command words. */
  CMD_NO_FORK =		  0x40,	/* Don't fork; just call execve */
  CMD_TIME_PIPELINE =	  0x80,	/* Time a pipeline */
  CMD_TIME_POSIX =	 0x100,	/* time -p; use POSIX.2 time output spec. */
  CMD_AMPERSAND =	 0x200,	/* command & */
  CMD_STDIN_REDIR =	 0x400,	/* async command needs implicit </dev/null */
  CMD_COMMAND_BUILTIN =	0x0800,	/* command executed by `command' builtin */
  CMD_COPROC_SUBSHELL =	0x1000,
  CMD_LASTPIPE =	0x2000,
  CMD_STDPATH =		0x4000,	/* use standard path for command lookup */
  CMD_TRY_OPTIMIZING =	0x8000	/* try to optimize this simple command */
};

/* What a command looks like (virtual base class). */
class COMMAND {
public:
  COMMAND();
  virtual ~COMMAND () noexcept;	/* virtual base class destructor */

  cmd_flags flags;		/* Flags controlling execution environment. */
  unsigned int line;		/* line number the command starts on */

protected:
  REDIRECT *redirects;		/* Special redirects for FOR CASE, etc. */
};

/* Structure used to represent the CONNECTION type. */
struct CONNECTION {
  COMMAND *first;		/* Pointer to the first command. */
  COMMAND *second;		/* Pointer to the second command. */
  int connector;		/* What separates this command from others. */
};

/* Structures used to represent the CASE command. */

/* Values for FLAGS word in a PATTERN_LIST */
enum pattern_list_flags {
  CASEPAT_FALLTHROUGH =	0x01,
  CASEPAT_TESTNEXT =	0x02
};

/* Pattern/action structure for CASE_COM. */
struct PATTERN_LIST {
  WORD_LIST *patterns;		/* Linked list of patterns to test. */
  COMMAND *action;		/* Thing to execute if a pattern matches. */
  pattern_list_flags flags;
  int _pad;
};

/* The CASE command. */
class CASE_COM : public COMMAND {
public:
  virtual ~CASE_COM () noexcept override;
protected:
  WORD_DESC *word;		/* The thing to test. */
  PATTERN_LIST *clauses;	/* The clauses to test against, or nullptr. */
};

/* FOR command. */
class FOR_COM : public COMMAND {
public:
  virtual ~FOR_COM () noexcept override;
protected:
  WORD_DESC *name;	/* The variable name to get mapped over. */
  WORD_LIST *map_list;	/* The things to map over.  This is never nullptr. */
  COMMAND *action;	/* The action to execute.
			   During execution, NAME is bound to successive
			   members of MAP_LIST. */
};

#if defined (ARITH_FOR_COMMAND)
class ARITH_FOR_COM : public COMMAND {
public:
  virtual ~ARITH_FOR_COM () noexcept override;
protected:
  WORD_LIST *init;
  WORD_LIST *test;
  WORD_LIST *step;
  COMMAND *action;
};
#endif

#if defined (SELECT_COMMAND)
/* KSH SELECT command. */
class SELECT_COM : public COMMAND {
public:
  virtual ~SELECT_COM () noexcept override;
protected:
  WORD_DESC *name;	/* The variable name to get mapped over. */
  WORD_LIST *map_list;	/* The things to map over.  This is never nullptr. */
  COMMAND *action;	/* The action to execute.
			   During execution, NAME is bound to the member of
			   MAP_LIST chosen by the user. */
};
#endif /* SELECT_COMMAND */

/* IF command. */
class IF_COM : public COMMAND {
public:
  virtual ~IF_COM () noexcept override;
protected:
  COMMAND *test;		/* Thing to test. */
  COMMAND *true_case;		/* What to do if the test returned non-zero. */
  COMMAND *false_case;		/* What to do if the test returned zero. */
};

/* WHILE command. */
class WHILE_COM : public COMMAND {
public:
  virtual ~WHILE_COM () noexcept override;
protected:
  COMMAND *test;		/* Thing to test. */
  COMMAND *action;		/* Thing to do while test is non-zero. */
};

#if defined (DPAREN_ARITHMETIC)
/* The arithmetic evaluation command, ((...)).  Just a set of flags and
   a WORD_LIST, of which the first element is the only one used, for the
   time being. */
class ARITH_COM : public COMMAND {
public:
  virtual ~ARITH_COM () noexcept override;
protected:
  WORD_LIST *exp;
};
#endif /* DPAREN_ARITHMETIC */

/* The conditional command, [[...]].  This is a binary tree -- we slipped
   a recursive-descent parser into the YACC grammar to parse it. */
enum cond_com_type {
  COND_AND =	1,
  COND_OR =	2,
  COND_UNARY =	3,
  COND_BINARY =	4,
  COND_TERM =	5,
  COND_EXPR =	6
};

class COND_COM : public COMMAND {
public:
  virtual ~COND_COM () noexcept override;
protected:
  int type;
  WORD_DESC *op;
  COND_COM *left, *right;
};

/* The "simple" command.  Just a collection of words and redirects. */
class SIMPLE_COM : public COMMAND {
public:
  virtual ~SIMPLE_COM () noexcept override;
protected:
  WORD_LIST *words;		/* The program name, the arguments,
				   variable assignments, etc. */
};

/* The "function definition" command. */
class FUNCTION_DEF : public COMMAND {
public:
  virtual ~FUNCTION_DEF () noexcept override;
protected:
  WORD_DESC *name;		/* The name of the function. */
  COMMAND *command;		/* The parsed execution tree. */
  char *source_file;		/* file in which function was defined, if any */
};

/* A command that is `grouped' allows pipes and redirections to affect all
   commands in the group. */
class GROUP_COM : public COMMAND {
public:
  virtual ~GROUP_COM () noexcept override;
protected:
};

class SUBSHELL_COM : public COMMAND {
public:
  virtual ~SUBSHELL_COM () noexcept override;
protected:
};

enum coproc_status {
  COPROC_RUNNING =	0x01,
  COPROC_DEAD =		0x02
};

struct Coproc {
  char *c_name;
  pid_t c_pid;
  int c_rfd;
  int c_wfd;
  int c_rsave;
  int c_wsave;
  int c_flags;
  coproc_status c_status;
  int c_lock;
};

class COPROC_COM : public COMMAND {
public:
  virtual ~COPROC_COM () noexcept override;
protected:
  std::string name;
  COMMAND *command;
};

#if 0
extern COMMAND *global_command;
extern Coproc sh_coproc;
#endif

/* Possible command errors */
enum cmd_err_type {
  CMDERR_DEFAULT =	0,
  CMDERR_BADTYPE =	1,
  CMDERR_BADCONN =	2,
  CMDERR_BADJUMP =	3,

  CMDERR_LAST =		3
};

}  // namespace bash

#endif /* _COMMAND_H_ */
