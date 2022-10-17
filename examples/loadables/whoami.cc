/*
 * whoami - print out username of current user
 */

/*
   Copyright (C) 1999-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash.
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

#include "builtins.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

// Loadable class for "whoami".
class ShellLoadable : public Shell
{
public:
  int whoami_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::whoami_builtin (WORD_LIST *list)
{
  int opt;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "")) != -1)
    {
      switch (opt)
        {
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;
  if (list)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  if (current_user.user_name == 0)
    get_current_user_info ();
  printf ("%s\n", current_user.user_name);
  return EXECUTION_SUCCESS;
}

static const char *const whoami_doc[]
    = { "Print user name", "", "Display name of current user.", nullptr };

Shell::builtin whoami_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::whoami_builtin),
    whoami_doc, "whoami", nullptr, BUILTIN_ENABLED);

} // namespace bash
