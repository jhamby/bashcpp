// This file is test_def.cc.
// It implements the builtin "test" in Bash.

// Copyright (C) 1987-2015 Free Software Foundation, Inc.

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

// $BUILTIN test
// $FUNCTION test_builtin
// $SHORT_DOC test [expr]
// Evaluate conditional expression.

// Exits with a status of 0 (true) or 1 (false) depending on
// the evaluation of EXPR.  Expressions may be unary or binary.  Unary
// expressions are often used to examine the status of a file.  There
// are string operators and numeric comparison operators as well.

// The behavior of test depends on the number of arguments.  Read the
// bash manual page for the complete specification.

// File operators:

//   -a FILE        True if file exists.
//   -b FILE        True if file is block special.
//   -c FILE        True if file is character special.
//   -d FILE        True if file is a directory.
//   -e FILE        True if file exists.
//   -f FILE        True if file exists and is a regular file.
//   -g FILE        True if file is set-group-id.
//   -h FILE        True if file is a symbolic link.
//   -L FILE        True if file is a symbolic link.
//   -k FILE        True if file has its `sticky' bit set.
//   -p FILE        True if file is a named pipe.
//   -r FILE        True if file is readable by you.
//   -s FILE        True if file exists and is not empty.
//   -S FILE        True if file is a socket.
//   -t FD          True if FD is opened on a terminal.
//   -u FILE        True if the file is set-user-id.
//   -w FILE        True if the file is writable by you.
//   -x FILE        True if the file is executable by you.
//   -O FILE        True if the file is effectively owned by you.
//   -G FILE        True if the file is effectively owned by your group.
//   -N FILE        True if the file has been modified since it was last read.

//   FILE1 -nt FILE2  True if file1 is newer than file2 (according to
//                    modification date).

//   FILE1 -ot FILE2  True if file1 is older than file2.

//   FILE1 -ef FILE2  True if file1 is a hard link to file2.

// String operators:

//   -z STRING      True if string is empty.

//   -n STRING
//      STRING      True if string is not empty.

//   STRING1 = STRING2
//                  True if the strings are equal.
//   STRING1 != STRING2
//                  True if the strings are not equal.
//   STRING1 < STRING2
//                  True if STRING1 sorts before STRING2 lexicographically.
//   STRING1 > STRING2
//                  True if STRING1 sorts after STRING2 lexicographically.

// Other operators:

//   -o OPTION      True if the shell option OPTION is enabled.
//   -v VAR         True if the shell variable VAR is set.
//   -R VAR         True if the shell variable VAR is set and is a name
//                  reference.
//   ! EXPR         True if expr is false.
//   EXPR1 -a EXPR2 True if both expr1 AND expr2 are true.
//   EXPR1 -o EXPR2 True if either expr1 OR expr2 is true.

//   arg1 OP arg2   Arithmetic tests.  OP is one of -eq, -ne,
//                  -lt, -le, -gt, or -ge.

// Arithmetic binary operators return true if ARG1 is equal, not-equal,
// less-than, less-than-or-equal, greater-than, or greater-than-or-equal
// than ARG2.

// Exit Status:
// Returns success if EXPR evaluates to true; fails if EXPR evaluates to
// false or an invalid argument is given.
// $END

// $BUILTIN [
// $DOCNAME test_bracket
// $FUNCTION test_builtin
// $SHORT_DOC [ arg... ]
// Evaluate conditional expression.

// This is a synonym for the "test" builtin, but the last argument must
// be a literal `]', to match the opening `['.
// $END

#include "config.h"

#include <unistd.h>

#include "shell.hh"

namespace bash
{

/* TEST/[ builtin. */
int
Shell::test_builtin (WORD_LIST *list)
{
  /* We let Matthew Bradburn and Kevin Braunsdorf's code do the
     actual test command.  So turn the list of args into a vector
     of char *, since that is what their code wants. */
  if (list == nullptr)
    {
      if (this_command_name[0] == '[' && !this_command_name[1])
        {
          builtin_error (_ ("missing `]'"));
          return EX_BADUSAGE;
        }

      return EXECUTION_FAILURE;
    }

  STRINGLIST *args = make_builtin_argv (list);
  int result = test_command (*args);
  strlist_dispose (args);

  return result;
}

} // namespace bash
