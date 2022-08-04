/* subst.h -- Names of externally visible functions in subst.c. */

/* Copyright (C) 1993-2017 Free Software Foundation, Inc.

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

#if !defined (_SUBST_H_)
#define _SUBST_H_

namespace bash
{

/* Constants which specify how to handle backslashes and quoting in
   expand_word_internal ().  Q_DOUBLE_QUOTES means to use the function
   slashify_in_quotes () to decide whether the backslash should be
   retained.  Q_HERE_DOCUMENT means slashify_in_here_document () to
   decide whether to retain the backslash.  Q_KEEP_BACKSLASH means
   to unconditionally retain the backslash.  Q_PATQUOTE means that we're
   expanding a pattern ${var%#[#%]pattern} in an expansion surrounded
   by double quotes. Q_DOLBRACE means we are expanding a ${...} word, so
   backslashes should also escape { and } and be removed. */
enum quoted_flags {
  Q_NOFLAGS =		     0,
  Q_DOUBLE_QUOTES =	 0x001,
  Q_HERE_DOCUMENT =	 0x002,
  Q_KEEP_BACKSLASH =	 0x004,
  Q_PATQUOTE =		 0x008,
  Q_QUOTED =		 0x010,
  Q_ADDEDQUOTES =	 0x020,
  Q_QUOTEDNULL =	 0x040,
  Q_DOLBRACE =		 0x080,
  Q_ARITH =		 0x100,	/* expanding string for arithmetic evaluation */
  Q_ARRAYSUB =		 0x200	/* expanding indexed array subscript */
};

/* Flag values controlling how assignment statements are treated. */
enum assign_flags {
  ASS_NOFLAGS =		     0,
  ASS_APPEND =		0x0001,
  ASS_MKLOCAL =		0x0002,
  ASS_MKASSOC =		0x0004,
  ASS_MKGLOBAL =	0x0008,	/* force global assignment */
  ASS_NAMEREF =		0x0010,	/* assigning to nameref variable */
  ASS_FORCE =		0x0020,	/* force assignment even to readonly variable */
  ASS_CHKLOCAL =	0x0040,	/* check local variable before assignment */
  ASS_NOEXPAND =	0x0080,	/* don't expand associative array subscripts */
  ASS_NOEVAL =		0x0100,	/* don't evaluate value as expression */
  ASS_NOLONGJMP =	0x0200,	/* don't longjmp on fatal assignment error */
  ASS_NOINVIS =		0x0400	/* don't resolve local invisible variables */
};

/* Flags for the string extraction functions. */
enum sx_flags {
  SX_NOFLAGS =		     0,
  SX_NOALLOC =		0x0001,	/* just skip; don't return substring */
  SX_VARNAME =		0x0002,	/* variable name; for string_extract () */
  SX_REQMATCH =		0x0004,	/* closing/matching delimiter required */
  SX_COMMAND =		0x0008,	/* extracting a shell script/command */
  SX_NOCTLESC =		0x0010,	/* don't honor CTLESC quoting */
  SX_NOESCCTLNUL =	0x0020,	/* don't let CTLESC quote CTLNUL */
  SX_NOTHROW =	0x0040,	/* don't longjmp on fatal error */
  SX_ARITHSUB =		0x0080,	/* extracting $(( ... )) (currently unused) */
  SX_POSIXEXP =		0x0100,	/* extracting new Posix pattern removal expansions in extract_dollar_brace_string */
  SX_WORD =		0x0200,	/* extracting word in ${param op word} */
  SX_COMPLETE =		0x0400,	/* extracting word for completion */
  SX_STRIPDQ =		0x0800	/* strip double quotes when extracting double-quoted string */
};

static inline sx_flags&
operator |= (sx_flags &a, const sx_flags &b) {
  a = static_cast<sx_flags> (static_cast<uint32_t> (a) | static_cast<uint32_t> (b));
  return a;
}

static inline sx_flags
operator | (const sx_flags &a, const sx_flags &b) {
  return static_cast<sx_flags> (static_cast<uint32_t> (a) | static_cast<uint32_t> (b));
}

static inline sx_flags
operator & (const sx_flags &a, const sx_flags &b) {
  return static_cast<sx_flags> (static_cast<uint32_t> (a) & static_cast<uint32_t> (b));
}

/* Evaluates to 1 if C is a character in $IFS. */
// FIXME: needs to be a member of Shell or something with a reference to ifs_cmap.
// static inline bool isifs(char c) { return (ifs_cmap[(unsigned char)(c)]); }

/* How to determine the quoted state of the character C. */
static inline bool quoted_char(char c) { return (c == CTLESC); }

/* Is the first character of STRING a quoted NULL character? */
static inline bool quoted_null(char *str) { return (str[0] == CTLNUL && str[1] == '\0'); }

}  // namespace bash

#endif /* !_SUBST_H_ */
