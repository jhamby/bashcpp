/* dispose_command.cc -- dispose of a COMMAND structure. */

/* Copyright (C) 1987-2009 Free Software Foundation, Inc.

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

#include "config.h"

#include "shell.hh"

namespace bash
{

// Virtual destructor for COMMAND. Subclasses will free any additional data.
COMMAND::~COMMAND () noexcept {}

// Virtual destructor for standard 'for' or ksh 'select' command.
FOR_SELECT_COM::~FOR_SELECT_COM () noexcept
{
  delete name;
  delete map_list;
  delete action;
}

#if defined(ARITH_FOR_COMMAND)
// Virtual destructor for arithmetic 'for' command.
ARITH_FOR_COM::~ARITH_FOR_COM () noexcept
{
  delete init;
  delete test;
  delete step;
  delete action;
}
#endif

// Virtual destructor for group command.
GROUP_COM::~GROUP_COM () noexcept { delete command; }

// Virtual destructor for subshell command.
SUBSHELL_COM::~SUBSHELL_COM () noexcept { delete command; }

// Virtual destructor for coprocess command.
COPROC_COM::~COPROC_COM () noexcept { delete command; }

// Virtual destructor for case command.
CASE_COM::~CASE_COM () noexcept
{
  delete word;
  delete clauses;
}

// Virtual destructor for until and while commands.
UNTIL_WHILE_COM::~UNTIL_WHILE_COM () noexcept
{
  delete test;
  delete action;
}

// Virtual destructor for if command.
IF_COM::~IF_COM () noexcept
{
  delete test;
  delete true_case;
  delete false_case;
}

// Virtual destructor for simple command.
SIMPLE_COM::~SIMPLE_COM () noexcept { delete words; }

// Virtual destructor for connection command.
CONNECTION::~CONNECTION () noexcept
{
  delete first;
  delete second;
}

#if defined(DPAREN_ARITHMETIC)
// Virtual destructor for arithmetic expression.
ARITH_COM::~ARITH_COM () noexcept { delete exp; }
#endif

// Virtual destructor for function definition.
FUNCTION_DEF::~FUNCTION_DEF () noexcept
{
  delete name;
  delete command;
}

#if defined(COND_COMMAND)
// Virtual destructor for conditional command.
COND_COM::~COND_COM () noexcept
{
  delete left;
  delete right;
  delete op;
}
#endif /* COND_COMMAND */

} // namespace bash
