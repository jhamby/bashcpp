// This file is shift_def.cc.
// It implements the builtin "shift" in Bash.

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

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "common.hh"
#include "shell.hh"

// $BUILTIN shift
// $FUNCTION shift_builtin
// $SHORT_DOC shift [n]
// Shift positional parameters.

// Rename the positional parameters $N+1,$N+2 ... to $1,$2 ...  If N is
// not given, it is assumed to be 1.

// Exit Status:
// Returns success unless N is negative or greater than $#.
// $END

namespace bash
{

/* Shift the arguments ``left''.  Shift DOLLAR_VARS down then take one
   off of REST_OF_ARGS and place it into DOLLAR_VARS[9].  If LIST has
   anything in it, it is a number which says where to start the
   shifting.  Return > 0 if `times' > $#, otherwise 0. */
int
Shell::shift_builtin (WORD_LIST *list)
{
  int64_t times;
  int nargs;

  CHECK_HELPOPT (list);

  if (get_numeric_arg (list, 0, &times) == 0)
    return EXECUTION_FAILURE;

  if (times == 0)
    return EXECUTION_SUCCESS;
  else if (times < 0)
    {
      sh_erange (list ? list->word->word.c_str () : nullptr,
                 _ ("shift count"));
      return EXECUTION_FAILURE;
    }
  nargs = number_of_args ();
  if (times > nargs)
    {
      if (print_shift_error)
        sh_erange (list ? list->word->word.c_str () : nullptr,
                   _ ("shift count"));
      return EXECUTION_FAILURE;
    }
  else if (times == nargs)
    clear_dollar_vars ();
  else
    shift_args (static_cast<int> (times));

  invalidate_cached_quoted_dollar_at ();

  return EXECUTION_SUCCESS;
}

} // namespace bash
