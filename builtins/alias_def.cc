// This file is alias_def.cc.
// It implements the builtins "alias" and "unalias" in Bash.

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

// $BUILTIN alias
// $FUNCTION alias_builtin
// $DEPENDS_ON ALIAS
// $SHORT_DOC alias [-p] [name[=value] ... ]
// Define or display aliases.

// Without arguments, `alias' prints the list of aliases in the reusable
// form `alias NAME=VALUE' on standard output.

// Otherwise, an alias is defined for each NAME whose VALUE is given.
// A trailing space in VALUE causes the next word to be checked for
// alias substitution when the alias is expanded.

// Options:
//   -p	print all defined aliases in a reusable format

// Exit Status:
// alias returns true unless a NAME is supplied for which no alias has been
// defined.
// $END

#include "config.h"

#if defined(ALIAS)

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "alias.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

/* Hack the alias command in a Korn shell way. */
int
Shell::alias_builtin (WORD_LIST *list)
{
  int offset;
  alias_t **alias_list, *t;
  char *name, *value;

  print_alias_flags dflags = posixly_correct ? AL_NOFLAGS : AL_REUSABLE;
  bool pflag = false;
  reset_internal_getopt ();
  while ((offset = internal_getopt (list, "p")) != -1)
    {
      switch (offset)
        {
        case 'p':
          pflag = true;
          dflags |= AL_REUSABLE;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }

  list = loptend;

  if (list == 0 || pflag)
    {
      if (aliases == 0)
        return EXECUTION_SUCCESS;

      alias_list = all_aliases ();

      if (alias_list == 0)
        return EXECUTION_SUCCESS;

      for (offset = 0; alias_list[offset]; offset++)
        print_alias (alias_list[offset], dflags);

      free (alias_list); /* XXX - Do not free the strings. */

      if (list == 0)
        return sh_chkwrite (EXECUTION_SUCCESS);
    }

  bool any_failed = false;
  while (list)
    {
      name = list->word->word;

      for (offset = 0; name[offset] && name[offset] != '='; offset++)
        ;

      if (offset && name[offset] == '=')
        {
          name[offset] = '\0';
          value = name + offset + 1;

          if (legal_alias_name (name, 0) == 0)
            {
              builtin_error (_ ("`%s': invalid alias name"), name);
              any_failed = true;
            }
          else
            add_alias (name, value);
        }
      else
        {
          t = find_alias (name);
          if (t)
            print_alias (t, dflags);
          else
            {
              sh_notfound (name);
              any_failed = true;
            }
        }
      list = list->next ();
    }

  return any_failed ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
}
} // namespace bash
#endif /* ALIAS */

// $BUILTIN unalias
// $FUNCTION unalias_builtin
// $DEPENDS_ON ALIAS
// $SHORT_DOC unalias [-a] name [name ...]
// Remove each NAME from the list of defined aliases.

// Options:
//   -a	remove all alias definitions

// Return success unless a NAME is not an existing alias.
// $END

#if defined(ALIAS)

namespace bash
{

/* Remove aliases named in LIST from the aliases database. */
int
Shell::unalias_builtin (WORD_LIST *list)
{
  int opt;

  bool aflag = false;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "a")) != -1)
    {
      switch (opt)
        {
        case 'a':
          aflag = true;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }

  list = loptend;

  if (aflag)
    {
      delete_all_aliases ();
      return EXECUTION_SUCCESS;
    }

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  aflag = false;
  while (list)
    {
      alias_t *alias = find_alias (list->word->word);

      if (alias)
        remove_alias (alias->name);
      else
        {
          sh_notfound (list->word->word.c_str ());
          aflag = true;
        }

      list = list->next ();
    }

  return aflag ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
}

/* Output ALIAS in such a way as to allow it to be read back in. */
void
Shell::print_alias (alias_t *alias, print_alias_flags flags)
{
  std::string value = sh_single_quote (alias->value);
  if (flags & AL_REUSABLE)
    printf ("alias %s", (!(alias->name.empty ()) && alias->name[0] == '-')
                                 ? "-- "
                                 : "");
  printf ("%s=%s\n", alias->name, value.c_str ());

  fflush (stdout);
}

} // namespace bash

#endif /* ALIAS */
