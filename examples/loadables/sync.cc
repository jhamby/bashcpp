/* sync - sync the disks by forcing pending filesystem writes to complete */

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
#include "shell.hh"

namespace bash
{

// Loadable class for "sync".
class ShellLoadable : public Shell
{
public:
  int sync_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::sync_builtin (WORD_LIST *list)
{
  sync ();
  return EXECUTION_SUCCESS;
}

static const char *const sync_doc[]
    = { "Sync disks.",
        ""
        "Force completion of pending disk writes",
        nullptr };

Shell::builtin sync_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::sync_builtin),
    sync_doc, "sync", nullptr, BUILTIN_ENABLED);

} // namespace bash
