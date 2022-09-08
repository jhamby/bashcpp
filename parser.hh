/* parser.h -- Everything you wanted to know about the parser, but were
   afraid to ask. */

/* Copyright (C) 1995-2019 Free Software Foundation, Inc.

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

#if !defined (_PARSER_H_)
#  define _PARSER_H_

#  include "command.hh"
#  include "input.hh"

/* Possible states for the parser that require it to do special things. */
enum parser_state_flags {
  PST_NOFLAGS =		       0,
  PST_CASEPAT =		0x000001,	/* in a case pattern list */
  PST_ALEXPNEXT =	0x000002,	/* expand next word for aliases */
  PST_ALLOWOPNBRC =	0x000004,	/* allow open brace for function def */
  PST_NEEDCLOSBRC =	0x000008,	/* need close brace */
  PST_DBLPAREN =	0x000010,	/* double-paren parsing */
  PST_SUBSHELL =	0x000020,	/* ( ... ) subshell */
  PST_CMDSUBST =	0x000040,	/* $( ... ) command substitution */
  PST_CASESTMT =	0x000080,	/* parsing a case statement */
  PST_CONDCMD =		0x000100,	/* parsing a [[...]] command */
  PST_CONDEXPR =	0x000200,	/* parsing the guts of [[...]] */
  PST_ARITHFOR =	0x000400,	/* parsing an arithmetic for command - unused */
  PST_ALEXPAND =	0x000800,	/* OK to expand aliases - unused */
  PST_EXTPAT =		0x001000,	/* parsing an extended shell pattern */
  PST_COMPASSIGN =	0x002000,	/* parsing x=(...) compound assignment */
  PST_ASSIGNOK =	0x004000,	/* assignment statement ok in this context */
  PST_EOFTOKEN =	0x008000,	/* yylex checks against shell_eof_token */
  PST_REGEXP =		0x010000,	/* parsing an ERE/BRE as a single word */
  PST_HEREDOC =		0x020000,	/* reading body of here-document */
  PST_REPARSE =		0x040000,	/* re-parsing in parse_string_to_word_list */
  PST_REDIRLIST =	0x080000,	/* parsing a list of redirections preceding a simple command name */
  PST_COMMENT =		0x100000,	/* parsing a shell comment; used by aliases */
  PST_ENDALIAS =	0x200000	/* just finished expanding and consuming an alias */
};

/* Definition of the delimiter stack.  Needed by parse.y and bashhist.c. */
struct dstack {
/* DELIMITERS is a stack of the nested delimiters that we have
   encountered so far. */
  char *delimiters;

/* Offset into the stack of delimiters. */
  int delimiter_depth;

/* How many slots are allocated to DELIMITERS. */
  int delimiter_space;
};

/* States we can be in while scanning a ${...} expansion.  Shared between
   parse.y and subst.c */
enum dolbrace_state_t {
  DOLBRACE_NOFLAGS =	   0,
  DOLBRACE_PARAM =	0x01,
  DOLBRACE_OP =		0x02,
  DOLBRACE_WORD =	0x04,

  DOLBRACE_QUOTE =	0x40,	/* single quote is special in double quotes */
  DOLBRACE_QUOTE2 =	0x80	/* single quote is semi-special in double quotes */
};

/* variable declarations from parse.y */
extern struct dstack dstack;

extern const char *primary_prompt;
extern const char *secondary_prompt;

extern const char *current_prompt_string;

extern const char *ps1_prompt;
extern const char *ps2_prompt;
extern const char *ps0_prompt;

extern char expand_aliases;
extern int current_command_line_count;
extern int saved_command_line_count;
extern int shell_eof_token;
extern int current_token;
extern int parser_state;
extern int need_here_doc;

extern char ignoreeof;
extern int eof_encountered;
extern int eof_encountered_limit;

extern int line_number, line_number_base;

#endif /* _PARSER_H_ */
