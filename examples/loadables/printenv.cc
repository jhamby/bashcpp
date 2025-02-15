/*
 * printenv -- minimal builtin clone of BSD printenv(1).
 *
 * usage: printenv [varname]
 *
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

// Loadable class for "printenv".
class ShellLoadable : public Shell
{
public:
  int printenv_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::printenv_builtin (WORD_LIST *list)
{
  char **envp;
  int opt;
  SHELL_VAR *var;

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

  /* printenv */
  if (list == 0)
    {
      maybe_make_export_env (); /* this allows minimal code */
      for (envp = export_env; *envp; envp++)
        printf ("%s\n", *envp);
      return EXECUTION_SUCCESS;
    }

  /* printenv varname */
  var = find_variable (list->word->word);
  if (var == 0 || (exported_p (var) == 0))
    return EXECUTION_FAILURE;

  if (function_p (var))
    print_var_function (var);
  else
    print_var_value (var, 0);

  printf ("\n");
  return EXECUTION_SUCCESS;
}

static const char *const printenv_doc[]
    = { "Display environment.", "",
        "Print names and values of environment variables", nullptr };

Shell::builtin printenv_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::printenv_builtin),
    printenv_doc, "printenv [varname]", nullptr, BUILTIN_ENABLED);

} // namespace bash
