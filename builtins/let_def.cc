// This file is let_def.cc.
// It implements the builtin "let" in Bash.

// Copyright (C) 1987-2009 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

// $BUILTIN let
// $FUNCTION let_builtin
// $SHORT_DOC let arg [arg ...]
// Evaluate arithmetic expressions.

// Evaluate each ARG as an arithmetic expression.  Evaluation is done in
// fixed-width integers with no check for overflow, though division by 0
// is trapped and flagged as an error.  The following list of operators is
// grouped into levels of equal-precedence operators.  The levels are listed
// in order of decreasing precedence.

// 	id++, id--	variable post-increment, post-decrement
// 	++id, --id	variable pre-increment, pre-decrement
// 	-, +		unary minus, plus
// 	!, ~		logical and bitwise negation
// 	**		exponentiation
// 	*, /, %		multiplication, division, remainder
// 	+, -		addition, subtraction
// 	<<, >>		left and right bitwise shifts
// 	<=, >=, <, >	comparison
// 	==, !=		equality, inequality
// 	&		bitwise AND
// 	^		bitwise XOR
// 	|		bitwise OR
// 	&&		logical AND
// 	||		logical OR
// 	expr ? expr : expr
// 			conditional operator
// 	=, *=, /=, %=,
// 	+=, -=, <<=, >>=,
// 	&=, ^=, |=	assignment

// Shell variables are allowed as operands.  The name of the variable
// is replaced by its value (coerced to a fixed-width integer) within
// an expression.  The variable need not have its integer attribute
// turned on to be used in an expression.

// Operators are evaluated in order of precedence.  Sub-expressions in
// parentheses are evaluated first and may override the precedence
// rules above.

// Exit Status:
// If the last ARG evaluates to 0, let returns 1; let returns 0 otherwise.
// $END

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "common.hh"
#include "shell.hh"

namespace bash
{

/* Arithmetic LET function. */
int
Shell::let_builtin (WORD_LIST *list)
{
  int64_t ret;
  bool expok;

  CHECK_HELPOPT (list);

  /* Skip over leading `--' argument. */
  if (list && list->word && ISOPTION (list->word->word, '-'))
    list = list->next ();

  if (list == 0)
    {
      builtin_error (_ ("expression expected"));
      return EXECUTION_FAILURE;
    }

  for (; list; list = list->next ())
    {
      ret = evalexp (list->word->word, EXP_EXPANDED, &expok);
      if (expok == 0)
        return EXECUTION_FAILURE;
    }

  return (ret == 0) ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
}

} // namespace bash
