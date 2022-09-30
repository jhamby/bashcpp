/* command.hh -- The structures used internally to represent commands, and
   the extern declarations of the functions used to create them. */

/* Copyright (C) 1993-2021 Free Software Foundation, Inc.

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

#if !defined(_COMMAND_H_)
#define _COMMAND_H_

#include "general.hh"
#include "shmbutil.hh"

#include <string>
#include <typeinfo>
#include <vector>

namespace bash
{

/* Instructions describing what kind of thing to do for a redirection. */
enum r_instruction
{
  r_output_direction,
  r_input_direction,
  r_inputa_direction,
  r_appending_to,
  r_reading_until,
  r_reading_string,
  r_duplicating_input,
  r_duplicating_output,
  r_deblank_reading_until,
  r_close_this,
  r_err_and_out,
  r_input_output,
  r_output_force,
  r_duplicating_input_word,
  r_duplicating_output_word,
  r_move_input,
  r_move_output,
  r_move_input_word,
  r_move_output_word,
  r_append_err_and_out
};

/* Redirection flags; values for rflags */
enum redir_flags
{
  REDIR_NOFLAGS = 0,
  REDIR_VARASSIGN = 0x01
};

/* Redirection errors. */
enum redir_error_type
{
  AMBIGUOUS_REDIRECT = -1,
  NOCLOBBER_REDIRECT = -2,
  RESTRICTED_REDIRECT = -3, /* can only happen in restricted shells. */
  HEREDOC_REDIRECT = -4,    /* here-doc temp file can't be created */
  BADVAR_REDIRECT = -5      /* something wrong with {varname}redir */
};

static inline bool
CLOBBERING_REDIRECT (r_instruction ri)
{
  return (ri == r_output_direction || ri == r_err_and_out);
}

static inline bool
OUTPUT_REDIRECT (r_instruction ri)
{
  return (ri == r_output_direction || ri == r_input_output
          || ri == r_err_and_out || ri == r_append_err_and_out);
}

static inline bool
INPUT_REDIRECT (r_instruction ri)
{
  return (ri == r_input_direction || ri == r_inputa_direction
          || ri == r_input_output);
}

static inline bool
WRITE_REDIRECT (r_instruction ri)
{
  return (ri == r_output_direction || ri == r_input_output
          || ri == r_err_and_out || ri == r_appending_to
          || ri == r_append_err_and_out || ri == r_output_force);
}

/* redirection needs translation */
static inline bool
TRANSLATE_REDIRECT (r_instruction ri)
{
  return (ri == r_duplicating_input_word || ri == r_duplicating_output_word
          || ri == r_move_input_word || ri == r_move_output_word);
}

/* Possible values for the `flags' field of a WORD_DESC. */
enum word_desc_flags
{
  W_NOFLAGS = 0,
  W_HASDOLLAR = 1 << 0,  // Dollar sign present.
  W_QUOTED = 1 << 1,     // Some form of quote character is present.
  W_ASSIGNMENT = 1 << 2, // This word is a variable assignment.
  W_SPLITSPACE = 1 << 3, // Split this word on " " regardless of IFS
  W_NOSPLIT = 1 << 4,    // Do not perform word splitting on this word because
                         // ifs is empty string.
  W_NOGLOB = 1 << 5,     // Do not perform globbing on this word.
  W_NOSPLIT2 = 1 << 6,   // Don't split word except for $@ expansion (using
                         // spaces) because context does not allow it.
  W_TILDEEXP = 1 << 7,   // Tilde expand this assignment word
  W_DOLLARAT = 1 << 8,   // UNUSED - $@ and its special handling
  W_ARRAYREF = 1 << 9,   // word is a valid array reference
  W_NOCOMSUB = 1 << 10,  // Don't perform command substitution on this word
  W_ASSIGNRHS = 1 << 11, // Word is rhs of an assignment statement
  W_NOTILDE = 1 << 12,   // Don't perform tilde expansion on this word
  W_NOASSNTILDE
  = 1 << 13, // don't do tilde expansion like an assignment statement
  W_EXPANDRHS = 1 << 14,  // Expanding word in ${paramOPword}
  W_COMPASSIGN = 1 << 15, // Compound assignment
  W_ASSNBLTIN = 1 << 16,  // word is a builtin command that takes assignments
  W_ASSIGNARG = 1 << 17,  // word is assignment argument to command
  W_HASQUOTEDNULL = 1 << 18, // word contains a quoted null character
  W_DQUOTE = 1 << 19,    // UNUSED - word should be treated as if double-quoted
  W_NOPROCSUB = 1 << 20, // don't perform process substitution
  W_SAWQUOTEDNULL = 1 << 21, // word contained a quoted null that was removed
  W_ASSIGNASSOC = 1 << 22,   // word looks like associative array assignment
  W_ASSIGNARRAY
  = 1 << 23,            // word looks like a compound indexed array assignment
  W_ARRAYIND = 1 << 24, // word is an array index being expanded
  W_ASSNGLOBAL
  = 1 << 25, // word is a global assignment to declare (declare/typeset -g)
  W_NOBRACE = 1 << 26,   // Don't perform brace expansion
  W_COMPLETE = 1 << 27,  // word is being expanded for completion
  W_CHKLOCAL = 1 << 28,  // check for local vars on assignment
  W_FORCELOCAL = 1 << 29 // force assignments to be to local variables,
                         // non-fatal on assignment errors
};

static inline word_desc_flags &
operator|= (word_desc_flags &a, const word_desc_flags &b)
{
  a = static_cast<word_desc_flags> (static_cast<uint32_t> (a)
                                    | static_cast<uint32_t> (b));
  return a;
}

static inline word_desc_flags
operator| (const word_desc_flags &a, const word_desc_flags &b)
{
  return static_cast<word_desc_flags> (static_cast<uint32_t> (a)
                                       | static_cast<uint32_t> (b));
}

static inline word_desc_flags &
operator&= (word_desc_flags &a, const word_desc_flags &b)
{
  a = static_cast<word_desc_flags> (static_cast<uint32_t> (a)
                                    & static_cast<uint32_t> (b));
  return a;
}

static inline word_desc_flags
operator& (const word_desc_flags &a, const word_desc_flags &b)
{
  return static_cast<word_desc_flags> (static_cast<uint32_t> (a)
                                       & static_cast<uint32_t> (b));
}

static inline word_desc_flags
operator~(const word_desc_flags &a)
{
  return static_cast<word_desc_flags> (~static_cast<uint32_t> (a));
}

/* Flags for the `pflags' argument to param_expand() and various
   parameter_brace_expand_xxx functions; also used for string_list_dollar_at */
enum param_flags
{
  PF_NOFLAGS = 0,
  PF_NOCOMSUB = 0x01,   /* Do not perform command substitution */
  PF_IGNUNBOUND = 0x02, /* ignore unbound vars even if -u set */
  PF_NOSPLIT2 = 0x04,   /* same as W_NOSPLIT2 */
  PF_ASSIGNRHS = 0x08,  /* same as W_ASSIGNRHS */
  PF_COMPLETE = 0x10,   /* same as W_COMPLETE, sets SX_COMPLETE */
  PF_EXPANDRHS = 0x20,  /* same as W_EXPANDRHS */
  PF_ALLINDS = 0x40     /* array, act as if [@] was supplied */
};

/* Possible values for subshell_environment */
enum subshell_flags
{
  SUBSHELL_NOFLAGS = 0,
  SUBSHELL_ASYNC = 0x01,   /* subshell caused by `command &' */
  SUBSHELL_PAREN = 0x02,   /* subshell caused by ( ... ) */
  SUBSHELL_COMSUB = 0x04,  /* subshell caused by `command` or $(command) */
  SUBSHELL_FORK = 0x08,    /* subshell caused by executing a disk command */
  SUBSHELL_PIPE = 0x10,    /* subshell from a pipeline element */
  SUBSHELL_PROCSUB = 0x20, /* subshell caused by <(command) or >(command) */
  SUBSHELL_COPROC = 0x40,  /* subshell from a coproc pipeline */
  SUBSHELL_RESETTRAP
  = 0x80, /* subshell needs to reset trap strings on first call to trap */
  SUBSHELL_IGNTRAP
  = 0x100 /* subshell should reset trapped signals from trap_handler */
};

/* A structure which represents a word. */
class WORD_DESC
{
public:
  WORD_DESC () : flags (W_NOFLAGS) {}

  WORD_DESC (const std::string &w) : word (w), flags (W_NOFLAGS) {}
  WORD_DESC (const char *w) : word (w), flags (W_NOFLAGS) {}
  WORD_DESC (const WORD_DESC &w) : word (w.word), flags (w.flags) {}

#if __cplusplus >= 201103L
  WORD_DESC (WORD_DESC &&w)
  noexcept : word (std::move (w.word)), flags (w.flags) {}
#endif

  std::string word;      /* C++ string. */
  word_desc_flags flags; /* Flags associated with this word. */
};

struct WORD_DESC_PTR
{
  WORD_DESC_PTR () : value (nullptr) {}
  WORD_DESC_PTR (const WORD_DESC_PTR &other) : value (other.value) {}

  WORD_DESC *value;
};

/* A linked list of words. */
class WORD_LIST : public GENERIC_LIST
{
public:
  WORD_LIST (WORD_DESC *word_) : GENERIC_LIST (nullptr), word (word_) {}

  // Constructor that takes another WORD_LIST to link to.
  WORD_LIST (WORD_DESC *word_, WORD_LIST *next)
      : GENERIC_LIST (next), word (word_)
  {
  }

  WORD_LIST (const WORD_LIST &w) : GENERIC_LIST (w.next_), word (w.word) {}

#if __cplusplus >= 201103L
  WORD_LIST (WORD_LIST &&w)
  noexcept : GENERIC_LIST (w.next_), word (w.word) {}
#endif

  // Non-virtual destructor should be safe here with the static_cast.
  ~WORD_LIST () noexcept
  {
    delete next ();
    delete word;
  }

  WORD_LIST *
  append (WORD_LIST *new_item)
  {
    append_ (new_item);
    return this;
  }

  WORD_LIST *
  next ()
  {
    return static_cast<WORD_LIST *> (next_);
  }

  const WORD_LIST *
  next () const
  {
    return static_cast<const WORD_LIST *> (next_);
  }

  WORD_LIST *
  reverse ()
  {
    return static_cast<WORD_LIST *> (reverse_ ());
  }

  WORD_DESC *word;
};

struct WORD_LIST_PTR
{
  WORD_LIST_PTR () : value (nullptr) {}
  WORD_LIST_PTR (WORD_DESC_PTR word) : value (new WORD_LIST (word.value)) {}

  // Constructor that takes another WORD_LIST to link to.
  WORD_LIST_PTR (WORD_DESC_PTR word, WORD_LIST_PTR next)
      : value (new WORD_LIST (word.value, next.value))
  {
  }

  WORD_LIST *value;
};

/* **************************************************************** */
/*								    */
/*			Shell Command Structs			    */
/*								    */
/* **************************************************************** */

/* What a redirection descriptor looks like.  If the redirection instruction
   is ri_duplicating_input or ri_duplicating_output, use DEST, otherwise
   use the file in FILENAME.  Out-of-range descriptors are identified by a
   negative DEST. */

class REDIRECTEE
{
public:
  // C++03-compatible constructors.
  REDIRECTEE (int64_t dest_) { r.dest = dest_; }
  REDIRECTEE (WORD_DESC &filename_) { r.filename = &filename_; }
  REDIRECTEE (WORD_DESC_PTR ptr) { r.filename = ptr.value; }

  union redirectee_t
  {
    int64_t dest;        /* Place to redirect REDIRECTOR to, or ... */
    WORD_DESC *filename; /* filename to redirect to. */
  } r;
};

/* Structure describing a redirection.  If REDIRECTOR is negative, the parser
   (or translator in redir.c) encountered an out-of-range file descriptor. */
class REDIRECT : public GENERIC_LIST
{
public:
  REDIRECT (REDIRECTEE source_, r_instruction instruction_,
            REDIRECTEE dest_and_filename_, redir_flags flags_)
      : GENERIC_LIST (), redirector (source_), redirectee (dest_and_filename_),
        rflags (flags_), instruction (instruction_)
  {
  }

  // Shallow copy constructor.
  REDIRECT (const REDIRECT &other)
      : GENERIC_LIST (), here_doc_eof (other.here_doc_eof),
        redirector (other.redirector), redirectee (other.redirectee),
        rflags (other.rflags), flags (other.flags),
        instruction (other.instruction)
  {
    if (other.rflags & REDIR_VARASSIGN)
      redirector.r.filename = new WORD_DESC (*(other.redirector.r.filename));

    switch (other.instruction)
      {
      case r_reading_until:
      case r_deblank_reading_until:
      case r_reading_string:
      case r_appending_to:
      case r_output_direction:
      case r_input_direction:
      case r_inputa_direction:
      case r_err_and_out:
      case r_append_err_and_out:
      case r_input_output:
      case r_output_force:
      case r_duplicating_input_word:
      case r_duplicating_output_word:
      case r_move_input_word:
      case r_move_output_word:
        redirectee.r.filename = new WORD_DESC (*(other.redirectee.r.filename));
        break;
      case r_duplicating_input:
      case r_duplicating_output:
      case r_move_input:
      case r_move_output:
      case r_close_this:
        break;
      }
  }

  // Non-virtual destructor should be safe here with the static_cast.
  ~REDIRECT () noexcept
  {
    delete next ();

    if (rflags & REDIR_VARASSIGN)
      delete redirector.r.filename;

    switch (instruction)
      {
      case r_reading_until:
      case r_deblank_reading_until:
      case r_reading_string:
      case r_output_direction:
      case r_input_direction:
      case r_inputa_direction:
      case r_appending_to:
      case r_err_and_out:
      case r_append_err_and_out:
      case r_input_output:
      case r_output_force:
      case r_duplicating_input_word:
      case r_duplicating_output_word:
      case r_move_input_word:
      case r_move_output_word:
        delete redirectee.r.filename;
        break;
      case r_duplicating_input:
      case r_duplicating_output:
      case r_move_input:
      case r_close_this:
      case r_move_output:
        break;
      }
  }

  REDIRECT *
  next ()
  {
    return static_cast<REDIRECT *> (next_);
  }

  REDIRECT *
  reverse ()
  {
    return static_cast<REDIRECT *> (reverse_ ());
  }

  REDIRECT *
  append (REDIRECT *new_item)
  {
    append_ (new_item);
    return this;
  }

  std::string here_doc_eof;  /* The word that appeared in <<foo. */
  REDIRECTEE redirector;     /* Descriptor or varname to be redirected. */
  REDIRECTEE redirectee;     /* File descriptor or filename */
  redir_flags rflags;        /* Private flags for this redirection */
  uint32_t flags;            /* Flag value for `open'. */
  r_instruction instruction; /* What to do with the information. */
};

struct REDIRECT_PTR
{
  REDIRECT_PTR () : value (nullptr) {}
  REDIRECT_PTR (REDIRECTEE source_, r_instruction instruction_,
                REDIRECTEE dest_and_filename_, redir_flags flags_)
      : value (
          new REDIRECT (source_, instruction_, dest_and_filename_, flags_))
  {
  }

  REDIRECT_PTR (REDIRECT *value_) : value (value_) {}
  REDIRECT_PTR (const REDIRECT_PTR &other) : value (other.value) {}

  REDIRECT_PTR &
  operator= (const REDIRECT_PTR &other)
  {
    value = other.value;
    return *this;
  }

  REDIRECT *value;
};

/* An element used in parsing.  A single word or a single redirection.
   This is an ephemeral construct. */
struct ELEMENT
{
  WORD_DESC *word;
  REDIRECT *redirect;
};

/* Possible values for command->flags. */
enum cmd_flags
{
  CMD_NOFLAGS = 0,
  CMD_WANT_SUBSHELL = 0x01,     /* User wants a subshell: ( command ) */
  CMD_FORCE_SUBSHELL = 0x02,    /* Shell needs to force a subshell. */
  CMD_INVERT_RETURN = 0x04,     /* Invert the exit value. */
  CMD_IGNORE_RETURN = 0x08,     /* Ignore the exit value.  For set -e. */
  CMD_NO_FUNCTIONS = 0x10,      /* Ignore functions during command lookup. */
  CMD_INHIBIT_EXPANSION = 0x20, /* Do not expand the command words. */
  CMD_NO_FORK = 0x40,           /* Don't fork; just call execve */
  CMD_TIME_PIPELINE = 0x80,     /* Time a pipeline */
  CMD_TIME_POSIX = 0x100,       /* time -p; use POSIX.2 time output spec. */
  CMD_AMPERSAND = 0x200,        /* command & */
  CMD_STDIN_REDIR = 0x400,      /* async command needs implicit </dev/null */
  CMD_COMMAND_BUILTIN = 0x0800, /* command executed by `command' builtin */
  CMD_COPROC_SUBSHELL = 0x1000,
  CMD_LASTPIPE = 0x2000,
  CMD_STDPATH = 0x4000,       /* use standard path for command lookup */
  CMD_TRY_OPTIMIZING = 0x8000 /* try to optimize this simple command */
};

static inline cmd_flags &
operator|= (cmd_flags &a, const cmd_flags &b)
{
  a = static_cast<cmd_flags> (static_cast<uint32_t> (a)
                              | static_cast<uint32_t> (b));
  return a;
}

static inline cmd_flags
operator| (const cmd_flags &a, const cmd_flags &b)
{
  return static_cast<cmd_flags> (static_cast<uint32_t> (a)
                                 | static_cast<uint32_t> (b));
}

static inline cmd_flags &
operator&= (cmd_flags &a, const cmd_flags &b)
{
  a = static_cast<cmd_flags> (static_cast<uint32_t> (a)
                              & static_cast<uint32_t> (b));
  return a;
}

static inline cmd_flags
operator& (const cmd_flags &a, const cmd_flags &b)
{
  return static_cast<cmd_flags> (static_cast<uint32_t> (a)
                                 & static_cast<uint32_t> (b));
}

static inline cmd_flags &
operator^= (cmd_flags &a, const cmd_flags &b)
{
  a = static_cast<cmd_flags> (static_cast<uint32_t> (a)
                              ^ static_cast<uint32_t> (b));
  return a;
}

static inline cmd_flags
operator~(const cmd_flags &a)
{
  return static_cast<cmd_flags> (~static_cast<uint32_t> (a));
}

class Shell;

/* What a command looks like (virtual base class). */
class COMMAND
{
public:
  COMMAND (int line_)
      : line (line_) {} // requires a line number to be specified

  virtual ~COMMAND () noexcept; /* virtual base class destructor */

  virtual COMMAND *clone (); // return a copy of the command

  virtual void print (Shell *) = 0;

  virtual bool control_structure ();

  REDIRECT *redirects; /* List of special redirects for FOR CASE, etc. */

  cmd_flags flags; /* Flags controlling execution environment. */
  int line;        /* line number the command starts on */

private:
  // disallow copy constructor and copy assignment operator
  COMMAND (const COMMAND &);
  COMMAND &operator= (const COMMAND &);
};

// This wrapper is used for all command types.
struct COMMAND_PTR
{
  COMMAND_PTR () : value (nullptr) {}
  COMMAND_PTR (COMMAND *value_) : value (value_) {}
  COMMAND_PTR (const COMMAND_PTR &other) : value (other.value) {}

  COMMAND_PTR &
  operator= (const COMMAND_PTR &other)
  {
    value = other.value;
    return *this;
  }

  COMMAND *value;
};

/* Structure used to represent the CONNECTION type. */
class CONNECTION : public COMMAND
{
public:
  // Constructor that uses the first
  CONNECTION (COMMAND *com1, COMMAND *com2, int con_)
      : COMMAND (com1->line), first (com1), second (com2), connector (con_)
  {
  }

  virtual ~CONNECTION () noexcept override; /* virtual base class destructor */

  virtual COMMAND *clone () override; // return a new deep copy of the command

  virtual void print (Shell *) override;

  COMMAND *first;  /* Pointer to the first command. */
  COMMAND *second; /* Pointer to the second command. */
  int connector;   /* What separates this command from others. */
};

/* Structures used to represent the CASE command. */

/* Values for FLAGS word in a PATTERN */
enum pattern_flags
{
  CASEPAT_NOFLAGS = 0,
  CASEPAT_FALLTHROUGH = 0x01,
  CASEPAT_TESTNEXT = 0x02
};

static inline pattern_flags &
operator|= (pattern_flags &a, const pattern_flags &b)
{
  a = static_cast<pattern_flags> (static_cast<uint32_t> (a)
                                  | static_cast<uint32_t> (b));
  return a;
}

static inline pattern_flags
operator| (const pattern_flags &a, const pattern_flags &b)
{
  return static_cast<pattern_flags> (static_cast<uint32_t> (a)
                                     | static_cast<uint32_t> (b));
}

/* Pattern/action structure for CASE_COM. */
class PATTERN_LIST : public GENERIC_LIST
{
public:
  // Constructor that takes ownership of the passed WORD_LIST and COMMAND.
  PATTERN_LIST (WORD_LIST *patterns_, COMMAND *action_)
      : patterns (patterns_), action (action_)
  {
  }

  // Non-virtual destructor should be safe here with the static_cast.
  ~PATTERN_LIST () noexcept
  {
    delete next ();
    delete patterns;
    delete action;
  }

  PATTERN_LIST *
  append (PATTERN_LIST *new_item)
  {
    append_ (new_item);
    return this;
  }

  PATTERN_LIST *
  next ()
  {
    return static_cast<PATTERN_LIST *> (next_);
  }

  void
  set_next (PATTERN_LIST *new_next)
  {
    next_ = new_next;
  }

  PATTERN_LIST *
  reverse ()
  {
    return static_cast<PATTERN_LIST *> (reverse_ ());
  }

  WORD_LIST *patterns; /* Linked list of patterns to test. */
  COMMAND *action;     /* Thing to execute if a pattern matches. */
  pattern_flags flags;
};

struct PATTERN_LIST_PTR
{
  PATTERN_LIST_PTR () : value (nullptr) {}
  PATTERN_LIST_PTR (PATTERN_LIST *value_) : value (value_) {}
  PATTERN_LIST_PTR (const PATTERN_LIST_PTR &other) : value (other.value) {}

  PATTERN_LIST_PTR &
  operator= (const PATTERN_LIST_PTR &other)
  {
    value = other.value;
    return *this;
  }

  PATTERN_LIST *value;
};

/* The CASE command. */
class CASE_COM : public COMMAND
{
public:
  CASE_COM (WORD_DESC *word_, PATTERN_LIST *clauses_, int line_)
      : COMMAND (line_), word (word_), clauses (clauses_)
  {
  }

  virtual ~CASE_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  WORD_DESC *word;       /* The thing to test. */
  PATTERN_LIST *clauses; /* The clauses to test against, or nullptr. */

private:
  void print_clauses (Shell *);
};

enum for_loop_type
{ // is this a for or a select loop?
  FOR_LOOP,
  SELECT_LOOP
};

/* FOR or KSH SELECT command. */
class FOR_SELECT_COM : public COMMAND
{
public:
  FOR_SELECT_COM (for_loop_type type_, WORD_DESC *name_, WORD_LIST *map_list_,
                  COMMAND *action_, int line_)
      : COMMAND (line_), name (name_), map_list (map_list_), action (action_),
        loop_type (type_)
  {
  }

  virtual ~FOR_SELECT_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  WORD_DESC *name;     /* The variable name to get mapped over. */
  WORD_LIST *map_list; /* The things to map over.  This is never nullptr. */
  COMMAND *action;     /* The action to execute.
                          During execution, NAME is bound to successive
                          members of MAP_LIST. */

  for_loop_type loop_type; // Whether this is a FOR or a SELECT loop. */
};

class ARITH_FOR_COM : public COMMAND
{
public:
  ARITH_FOR_COM (WORD_LIST *init_, WORD_LIST *test_, WORD_LIST *step_,
                 COMMAND *action_, int line_)
      : COMMAND (line_), init (init_), test (test_), step (step_),
        action (action_)
  {
  }

  virtual ~ARITH_FOR_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  WORD_LIST *init;
  WORD_LIST *test;
  WORD_LIST *step;
  COMMAND *action;
};

/* IF command. */
class IF_COM : public COMMAND
{
public:
  // Constructor called by parser that uses the line number of the 'test'
  // argument. In the C version of bash, the line number isn't initialized for
  // this struct.
  IF_COM (COMMAND *test_, COMMAND *true_case_, COMMAND *false_case_)
      : COMMAND (test_->line), test (test_), true_case (true_case_),
        false_case (false_case_)
  {
  }

  virtual ~IF_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  COMMAND *test;       /* Thing to test. */
  COMMAND *true_case;  /* What to do if the test returned non-zero. */
  COMMAND *false_case; /* What to do if the test returned zero. */
};

enum loop_t
{ // is this an until or a while loop?
  LOOP_UNTIL,
  LOOP_WHILE
};

/* UNTIL_WHILE command. */
class UNTIL_WHILE_COM : public COMMAND
{
public:
  // Constructor called by parser that uses the line number of the 'test'
  // argument. In the C version of bash, the line number isn't initialized for
  // this struct.
  UNTIL_WHILE_COM (loop_t type_, COMMAND *test_, COMMAND *action_)
      : COMMAND (test_->line), test (test_), action (action_),
        loop_type (type_)
  {
  }

  virtual ~UNTIL_WHILE_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  COMMAND *test;    // Thing to test.
  COMMAND *action;  // Thing to do while test is non-zero.
  loop_t loop_type; // Whether this is an UNTIL or WHILE loop.
};

/* The arithmetic evaluation command, ((...)).  Just a set of flags and
   a WORD_LIST, of which the first element is the only one used, for the
   time being. */
class ARITH_COM : public COMMAND
{
public:
  // Contruct a new ARITH_COM, wrapping the specified WORD_LIST.
  ARITH_COM (WORD_LIST *exp_) : COMMAND (0), exp (exp_) {}

  virtual ~ARITH_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  WORD_LIST *exp;
};

#if defined(COND_COMMAND)
/* The conditional command, [[...]].  This is a binary tree -- we slipped
   a recursive-descent parser into the YACC grammar to parse it. */
enum cond_com_type
{
  COND_AND = 1,
  COND_OR = 2,
  COND_UNARY = 3,
  COND_BINARY = 4,
  COND_TERM = 5,
  COND_EXPR = 6
};

class COND_COM : public COMMAND
{
public:
  COND_COM (int line_, cond_com_type type_, WORD_DESC *op_, COND_COM *left_,
            COND_COM *right_)
      : COMMAND (line_), op (op_), left (left_), right (right_), type (type_)
  {
  }

  virtual ~COND_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  WORD_DESC *op;
  COND_COM *left, *right;
  cond_com_type type;
};
#endif

/* The "simple" command.  Just a collection of words and redirects. */
class SIMPLE_COM : public COMMAND
{
public:
  SIMPLE_COM (ELEMENT, COMMAND *);

  virtual ~SIMPLE_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  virtual bool control_structure () override; // not a control structure

  WORD_LIST *words;           /* The program name, the arguments,
                                 variable assignments, etc. */
  REDIRECT *simple_redirects; /* This class has its own separate redirects. */
};

/* The "function definition" command. */
class FUNCTION_DEF : public COMMAND
{
public:
  virtual ~FUNCTION_DEF () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  WORD_DESC *name;         /* The name of the function. */
  COMMAND *command;        /* The parsed execution tree. */
  std::string source_file; /* file in which function was defined, if any */
};

/* A command that is `grouped' allows pipes and redirections to affect all
   commands in the group. */
class GROUP_COM : public COMMAND
{
public:
  // Contruct a new GROUP_COM, wrapping the specified command.
  GROUP_COM (COMMAND *command_) : COMMAND (command_->line), command (command_)
  {
  }

  virtual ~GROUP_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  COMMAND *command;
};

// A wrapper for a command that runs in a subshell.
class SUBSHELL_COM : public COMMAND
{
public:
  SUBSHELL_COM (COMMAND *command_)
      : COMMAND (command_->line), command (command_)
  {
    flags |= CMD_WANT_SUBSHELL; // set the appropriate flags
  }

  virtual ~SUBSHELL_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  virtual bool control_structure () override; // not a control structure

  COMMAND *command; // The actual command to run.
};

enum coproc_status
{
  COPROC_RUNNING = 0x01,
  COPROC_DEAD = 0x02
};

struct Coproc
{
  std::string c_name;
  pid_t c_pid;
  int c_rfd;
  int c_wfd;
  int c_rsave;
  int c_wsave;
  int c_flags;
  coproc_status c_status;
  int c_lock;
};

// A wrapper for a command that runs as a coprocess.
class COPROC_COM : public COMMAND
{
public:
  COPROC_COM (const char *name_, COMMAND *command_)
      : COMMAND (command_->line), name (name_), command (command_)
  {
    flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
  }

  COPROC_COM (const std::string &name_, COMMAND *command_)
      : COMMAND (command_->line), name (name_), command (command_)
  {
    flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
  }

  virtual ~COPROC_COM () noexcept override;
  virtual COMMAND *clone () override;
  virtual void print (Shell *) override;

  virtual bool control_structure () override; // not a control structure

  std::string name;
  COMMAND *command; // The actual command to run.
};

/* Possible command errors */
enum cmd_err_type
{
  CMDERR_DEFAULT = 0,
  CMDERR_BADTYPE = 1,
  CMDERR_BADCONN = 2,
  CMDERR_BADJUMP = 3,

  CMDERR_LAST = 3
};

/* Declarations for functions needed by the parser. */
COMMAND *connect_async_list (COMMAND *, COMMAND *, int);

} // namespace bash

#endif /* _COMMAND_H_ */
