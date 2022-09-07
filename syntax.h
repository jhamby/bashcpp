/* syntax.h -- Syntax definitions for the shell */

/* Copyright (C) 2000, 2001, 2005, 2008, 2009-2020 Free Software Foundation, Inc.

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

#ifndef _SYNTAX_H_
#define _SYNTAX_H_

#include "bashtypes.h"

namespace bash
{

/* Defines for use by mksyntax.c */

static const char *slashify_in_quotes = "\\`$\"\n";
static const char *slashify_in_here_document = "\\`$";

static const char *shell_meta_chars = "()<>;&|";
static const char *shell_break_chars = "()<>;&| \t\n";

static const char *shell_quote_chars = "\"`'";

#if defined (PROCESS_SUBSTITUTION)
static const char *shell_exp_chars = "$<>";
#else
static const char *shell_exp_chars = "$";
#endif

#if defined (EXTENDED_GLOB)
static const char *ext_glob_chars = "@*+?!";
#else
static const char *ext_glob_chars = "";
#endif

static const char *shell_glob_chars = "*?[]^";

/* Defines shared by mksyntax.c and the rest of the shell code. */

/* Values for character flags in syntax tables */

enum char_flags {
  CWORD =		0x0000,	/* nothing special; an ordinary character */
  CSHMETA =		0x0001,	/* shell meta character */
  CSHBRK =		0x0002,	/* shell break character */
  CBACKQ =		0x0004,	/* back quote */
  CQUOTE =		0x0008,	/* shell quote character */
  CSPECL =		0x0010,	/* special character that needs quoting */
  CEXP =		0x0020,	/* shell expansion character */
  CBSDQUOTE =		0x0040,	/* characters escaped by backslash in double quotes */
  CBSHDOC =		0x0080,	/* characters escaped by backslash in here doc */
  CGLOB =		0x0100,	/* globbing characters */
  CXGLOB =		0x0200,	/* extended globbing characters */
  CXQUOTE =		0x0400,	/* cquote + backslash */
  CSPECVAR =		0x0800,	/* single-character shell variable name */
  CSUBSTOP =		0x1000,	/* values of OP for ${word[:]OPstuff} */
  CBLANK =		0x2000	/* whitespace (blank) character */
};

/* Defines for use by the rest of the shell. */
// extern int sh_syntaxtab[];
// extern int sh_syntabsiz;

#define shellmeta(c)	(sh_syntaxtab[static_cast<unsigned char> (c)] & CSHMETA)
#define shellbreak(c)	(sh_syntaxtab[static_cast<unsigned char> (c)] & CSHBRK)
#define shellquote(c)	(sh_syntaxtab[static_cast<unsigned char> (c)] & CQUOTE)
#define shellxquote(c)	(sh_syntaxtab[static_cast<unsigned char> (c)] & CXQUOTE)

#define shellblank(c)	(sh_syntaxtab[static_cast<unsigned char> (c)] & CBLANK)

#define parserblank(c)	((c) == ' ' || (c) == '\t')

#define issyntype(c, t)	((sh_syntaxtab[static_cast<unsigned char> (c)] & (t)) != 0)
#define notsyntype(c,t) ((sh_syntaxtab[static_cast<unsigned char> (c)] & (t)) == 0)

#if defined (PROCESS_SUBSTITUTION)
#  define shellexp(c)	((c) == '$' || (c) == '<' || (c) == '>')
#else
#  define shellexp(c)	((c) == '$')
#endif

#if defined (EXTENDED_GLOB)
#  define PATTERN_CHAR(c) \
	((c) == '@' || (c) == '*' || (c) == '+' || (c) == '?' || (c) == '!')
#else
#  define PATTERN_CHAR(c) 0
#endif

#define GLOB_CHAR(c) \
	((c) == '*' || (c) == '?' || (c) == '[' || (c) == ']' || (c) == '^')

#define CTLESC '\001'
#define CTLNUL '\177'

}  // namespace bash

#endif /* _SYNTAX_H_ */
