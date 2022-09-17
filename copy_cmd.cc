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

#include "config.hh"

#include "bashtypes.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "shell.hh"

namespace bash
{

/* Copy the chain of words in LIST.  Return a pointer to
   the new chain. */
WORD_LIST *
copy_word_list (WORD_LIST *list)
{
  WORD_LIST *new_list, *tl;

  for (new_list = tl = nullptr; list; list = list->next ())
    {
      if (new_list == 0)
        new_list = tl = make_word_list (copy_word (list->word), new_list);
      else
        {
          tl->next = make_word_list (copy_word (list->word), nullptr);
          tl = tl->next ());
        }
    }

  return new_list;
}

static PATTERN_LIST *
copy_case_clause (PATTERN_LIST *clause)
{
  PATTERN_LIST *new_clause;

  new_clause = (PATTERN_LIST *)xmalloc (sizeof (PATTERN_LIST));
  new_clause->patterns = copy_word_list (clause->patterns);
  new_clause->action = copy_command (clause->action);
  new_clause->flags = clause->flags;
  return new_clause;
}

static PATTERN_LIST *
copy_case_clauses (PATTERN_LIST *clauses)
{
  PATTERN_LIST *new_list, *new_clause;

  for (new_list = (PATTERN_LIST *)NULL; clauses; clauses = clauses->next)
    {
      new_clause = copy_case_clause (clauses);
      new_clause->next = new_list;
      new_list = new_clause;
    }
  return REVERSE_LIST (new_list, PATTERN_LIST *);
}

REDIRECT *
copy_redirects (REDIRECT *list)
{
  REDIRECT *new_list, *temp;

  for (new_list = nullptr; list; list = list->next ())
    {
      temp = new REDIRECT (*list);
      temp->set_next (new_list);
      new_list = temp;
    }
  return new_list->reverse ();
}

static FOR_COM *
copy_for_command (FOR_COM *com)
{
  FOR_COM *new_for;

  new_for = (FOR_COM *)xmalloc (sizeof (FOR_COM));
  new_for->flags = com->flags;
  new_for->line = com->line;
  new_for->name = copy_word (com->name);
  new_for->map_list = copy_word_list (com->map_list);
  new_for->action = copy_command (com->action);
  return new_for;
}

#if defined(ARITH_FOR_COMMAND)
static ARITH_FOR_COM *
copy_arith_for_command (ARITH_FOR_COM *com)
{
  ARITH_FOR_COM *new_arith_for;

  new_arith_for = (ARITH_FOR_COM *)xmalloc (sizeof (ARITH_FOR_COM));
  new_arith_for->flags = com->flags;
  new_arith_for->line = com->line;
  new_arith_for->init = copy_word_list (com->init);
  new_arith_for->test = copy_word_list (com->test);
  new_arith_for->step = copy_word_list (com->step);
  new_arith_for->action = copy_command (com->action);
  return new_arith_for;
}
#endif /* ARITH_FOR_COMMAND */

static GROUP_COM *
copy_group_command (GROUP_COM *com)
{
  GROUP_COM *new_group;

  new_group = (GROUP_COM *)xmalloc (sizeof (GROUP_COM));
  new_group->command = copy_command (com->command);
  return new_group;
}

static SUBSHELL_COM *
copy_subshell_command (SUBSHELL_COM *com)
{
  SUBSHELL_COM *new_subshell;

  new_subshell = (SUBSHELL_COM *)xmalloc (sizeof (SUBSHELL_COM));
  new_subshell->command = copy_command (com->command);
  new_subshell->flags = com->flags;
  new_subshell->line = com->line;
  return new_subshell;
}

static COPROC_COM *
copy_coproc_command (COPROC_COM *com)
{
  COPROC_COM *new_coproc;

  new_coproc = (COPROC_COM *)xmalloc (sizeof (COPROC_COM));
  new_coproc->name = savestring (com->name);
  new_coproc->command = copy_command (com->command);
  new_coproc->flags = com->flags;
  return new_coproc;
}

static CASE_COM *
copy_case_command (CASE_COM *com)
{
  CASE_COM *new_case;

  new_case = (CASE_COM *)xmalloc (sizeof (CASE_COM));
  new_case->flags = com->flags;
  new_case->line = com->line;
  new_case->word = copy_word (com->word);
  new_case->clauses = copy_case_clauses (com->clauses);
  return new_case;
}

static WHILE_COM *
copy_while_command (WHILE_COM *com)
{
  WHILE_COM *new_while;

  new_while = (WHILE_COM *)xmalloc (sizeof (WHILE_COM));
  new_while->flags = com->flags;
  new_while->test = copy_command (com->test);
  new_while->action = copy_command (com->action);
  return new_while;
}

static IF_COM *
copy_if_command (IF_COM *com)
{
  IF_COM *new_if;

  new_if = (IF_COM *)xmalloc (sizeof (IF_COM));
  new_if->flags = com->flags;
  new_if->test = copy_command (com->test);
  new_if->true_case = copy_command (com->true_case);
  new_if->false_case
      = com->false_case ? copy_command (com->false_case) : com->false_case;
  return new_if;
}

#if defined(DPAREN_ARITHMETIC)
static ARITH_COM *
copy_arith_command (ARITH_COM *com)
{
  ARITH_COM *new_arith;

  new_arith = (ARITH_COM *)xmalloc (sizeof (ARITH_COM));
  new_arith->flags = com->flags;
  new_arith->exp = copy_word_list (com->exp);
  new_arith->line = com->line;

  return new_arith;
}
#endif

#if defined(COND_COMMAND)
static COND_COM *
copy_cond_command (COND_COM *com)
{
  COND_COM *new_cond;

  new_cond = (COND_COM *)xmalloc (sizeof (COND_COM));
  new_cond->flags = com->flags;
  new_cond->line = com->line;
  new_cond->type = com->type;
  new_cond->op = com->op ? copy_word (com->op) : com->op;
  new_cond->left
      = com->left ? copy_cond_command (com->left) : (COND_COM *)NULL;
  new_cond->right
      = com->right ? copy_cond_command (com->right) : (COND_COM *)NULL;

  return new_cond;
}
#endif

static SIMPLE_COM *
copy_simple_command (SIMPLE_COM *com)
{
  SIMPLE_COM *new_simple;

  new_simple = (SIMPLE_COM *)xmalloc (sizeof (SIMPLE_COM));
  new_simple->flags = com->flags;
  new_simple->words = copy_word_list (com->words);
  new_simple->redirects
      = com->redirects ? copy_redirects (com->redirects) : (REDIRECT *)NULL;
  new_simple->line = com->line;
  return new_simple;
}

FUNCTION_DEF *
copy_function_def_contents (FUNCTION_DEF *old, FUNCTION_DEF *new_def)
{
  new_def->name = copy_word (old->name);
  new_def->command = old->command ? copy_command (old->command) : old->command;
  new_def->flags = old->flags;
  new_def->line = old->line;
  new_def->source_file
      = old->source_file ? savestring (old->source_file) : old->source_file;
  return new_def;
}

FUNCTION_DEF *
copy_function_def (FUNCTION_DEF *com)
{
  FUNCTION_DEF *new_def;

  new_def = (FUNCTION_DEF *)xmalloc (sizeof (FUNCTION_DEF));
  new_def = copy_function_def_contents (com, new_def);
  return new_def;
}

/* Copy the command structure in COMMAND.  Return a pointer to the
   copy.  Don't forget to call delete on the returned copy. */
COMMAND *
COMMAND::clone ()
{
  return new COMMAND (*this);
}

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
