/* copy_command.cc -- copy command objects.  */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

#include "bashtypes.hh"

#include "shell.hh"

namespace bash
{

/* Copy the chain of words in LIST into the new object. */
WORD_LIST::WORD_LIST (const WORD_LIST &w)
    : GENERIC_LIST (w.next_), word (w.word)
{
  if (word)
    word = new WORD_DESC (*word);

  // Recursively clone the remaining list items.
  WORD_LIST *next_word = next ();
  if (next_word)
    set_next (new WORD_LIST (*next_word));
}

/* Copy the chain of case clauses into the new object. */
PATTERN_LIST::PATTERN_LIST (const PATTERN_LIST &p)
    : GENERIC_LIST (p.next_), patterns (p.patterns), action (p.action),
      flags (p.flags)
{
  if (patterns)
    patterns = new WORD_LIST (*patterns);

  if (action)
    action = action->clone ();

  // Recursively clone the remaining clauses.
  PATTERN_LIST *next_clause = next ();
  if (next_clause)
    set_next (new PATTERN_LIST (*next_clause));
}

REDIRECT::REDIRECT (const REDIRECT &other)
    : GENERIC_LIST (other.next_), here_doc_eof (other.here_doc_eof),
      redirector (other.redirector), redirectee (other.redirectee),
      rflags (other.rflags), flags (other.flags),
      instruction (other.instruction)
{
  if (rflags & REDIR_VARASSIGN)
    redirector.r.filename = new WORD_DESC (*(redirector.r.filename));

  switch (instruction)
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
      redirectee.r.filename = new WORD_DESC (*(redirectee.r.filename));
      break;
    case r_duplicating_input:
    case r_duplicating_output:
    case r_move_input:
    case r_move_output:
    case r_close_this:
      break;
    }

  // Recursively clone the remaining redirections.
  REDIRECT *next_redirection = next ();
  if (next_redirection)
    set_next (new REDIRECT (*next_redirection));
}

/* Copy the command structure in COMMAND.  Return a pointer to the
   copy.  Don't forget to call delete on the returned copy. */

COMMAND *
CONNECTION::clone ()
{
  return new CONNECTION (*this);
}

COMMAND *
CASE_COM::clone ()
{
  return new CASE_COM (*this);
}

COMMAND *
FOR_SELECT_COM::clone ()
{
  return new FOR_SELECT_COM (*this);
}

COMMAND *
ARITH_FOR_COM::clone ()
{
  return new ARITH_FOR_COM (*this);
}

COMMAND *
IF_COM::clone ()
{
  return new IF_COM (*this);
}

COMMAND *
UNTIL_WHILE_COM::clone ()
{
  return new UNTIL_WHILE_COM (*this);
}

COMMAND *
ARITH_COM::clone ()
{
  return new ARITH_COM (*this);
}

COMMAND *
COND_COM::clone ()
{
  return new COND_COM (*this);
}

COMMAND *
SIMPLE_COM::clone ()
{
  return new SIMPLE_COM (*this);
}

COMMAND *
FUNCTION_DEF::clone ()
{
  return new FUNCTION_DEF (*this);
}

COMMAND *
GROUP_COM::clone ()
{
  return new GROUP_COM (*this);
}

COMMAND *
SUBSHELL_COM::clone ()
{
  return new SUBSHELL_COM (*this);
}

COMMAND *
COPROC_COM::clone ()
{
  return new COPROC_COM (*this);
}

} // namespace bash
