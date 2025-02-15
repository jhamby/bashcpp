// This file is colon_def.cc.
// It implements the builtin ":" in Bash.

// Copyright (C) 1987-2019 Free Software Foundation, Inc.

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

// $BUILTIN :
// $DOCNAME colon
// $FUNCTION colon_builtin
// $SHORT_DOC :
// Null command.

// No effect; the command does nothing.

// Exit Status:
// Always succeeds.
// $END

// $BUILTIN true
// $FUNCTION colon_builtin
// $SHORT_DOC true
// Return a successful result.

// Exit Status:
// Always succeeds.
// $END

// $BUILTIN false
// $FUNCTION false_builtin
// $SHORT_DOC false
// Return an unsuccessful result.

// Exit Status:
// Always fails.
// $END

#include "config.h"

#include "shell.hh"

namespace bash
{

/* Return a successful result. */
int
Shell::colon_builtin (WORD_LIST *ignore)
{
  return 0;
}

/* Return an unsuccessful result. */
int
Shell::false_builtin (WORD_LIST *ignore)
{
  return 1;
}

} // namespace bash
