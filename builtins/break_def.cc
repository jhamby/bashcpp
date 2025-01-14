// This file is break_def.cc.
// It implements the builtins "break" and "continue" in Bash.

// Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

// $BUILTIN break
// $FUNCTION break_builtin
// $SHORT_DOC break [n]
// Exit for, while, or until loops.

// Exit a FOR, WHILE or UNTIL loop.  If N is specified, break N enclosing
// loops.

// Exit Status:
// The exit status is 0 unless N is not greater than or equal to 1.
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

#if 0
static int check_loop_level (void);

/* The depth of while's and until's. */
int loop_level = 0;

/* Non-zero when a "break" instruction is encountered. */
int breaking = 0;

/* Non-zero when we have encountered a continue instruction. */
int continuing = 0;
#endif

/* Set up to break x levels, where x defaults to 1, but can be specified
   as the first argument. */
int
Shell::break_builtin (WORD_LIST *list)
{
  int64_t newbreak;

  CHECK_HELPOPT (list);

  if (check_loop_level () == 0)
    return EXECUTION_SUCCESS;

  (void)get_numeric_arg (list, 1, &newbreak);

  if (newbreak <= 0)
    {
      sh_erange (list->word->word.c_str (), _ ("loop count"));
      breaking = loop_level;
      return EXECUTION_FAILURE;
    }

  if (newbreak > loop_level)
    newbreak = loop_level;

  breaking = newbreak;

  return EXECUTION_SUCCESS;
}

// $BUILTIN continue
// $FUNCTION continue_builtin
// $SHORT_DOC continue [n]
// Resume for, while, or until loops.

// Resumes the next iteration of the enclosing FOR, WHILE or UNTIL loop.
// If N is specified, resumes the Nth enclosing loop.

// Exit Status:
// The exit status is 0 unless N is not greater than or equal to 1.
// $END

/* Set up to continue x levels, where x defaults to 1, but can be specified
   as the first argument. */
int
Shell::continue_builtin (WORD_LIST *list)
{
  int64_t newcont;

  CHECK_HELPOPT (list);

  if (check_loop_level () == 0)
    return EXECUTION_SUCCESS;

  (void)get_numeric_arg (list, 1, &newcont);

  if (newcont <= 0)
    {
      sh_erange (list->word->word.c_str (), _ ("loop count"));
      breaking = loop_level;
      return EXECUTION_FAILURE;
    }

  if (newcont > loop_level)
    newcont = loop_level;

  continuing = newcont;

  return EXECUTION_SUCCESS;
}

/* Return non-zero if a break or continue command would be okay.
   Print an error message if break or continue is meaningless here. */
static int
check_loop_level ()
{
#if defined(BREAK_COMPLAINS)
  if (loop_level == 0 && posixly_correct == 0)
    builtin_error (_ ("only meaningful in a `for', `while', or `until' loop"));
#endif /* BREAK_COMPLAINS */

  return loop_level;
}

} // namespace bash
