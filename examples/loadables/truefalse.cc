/* true and false builtins */

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

#include "bashtypes.hh"
#include "builtins.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

// Loadable class for "true" and "false".
class ShellLoadable : public Shell
{
public:
  int true_builtin (WORD_LIST *);
  int false_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::true_builtin (WORD_LIST *list)
{
  return EXECUTION_SUCCESS;
}

int
ShellLoadable::false_builtin (WORD_LIST *list)
{
  return EXECUTION_FAILURE;
}

static const char *const true_doc[]
    = { "Exit successfully.", "", "Return a successful result.", nullptr };

static const char *const false_doc[]
    = { "Exit unsuccessfully.", "", "Return an unsuccessful result.",
        nullptr };

Shell::builtin true_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::true_builtin),
    true_doc, "true", nullptr, BUILTIN_ENABLED);

Shell::builtin false_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::false_builtin),
    false_doc, "false", nullptr, BUILTIN_ENABLED);

} // namespace bash
