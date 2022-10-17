/* unlink - remove a directory entry */

/* Should only be used to remove directories by a superuser prepared to let
   fsck clean up the file system. */

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "builtins.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

// Loadable class for "unlink".
class ShellLoadable : public Shell
{
public:
  int unlink_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::unlink_builtin (WORD_LIST *list)
{
  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  if (unlink (list->word->word) != 0)
    {
      builtin_error ("%s: cannot unlink: %s", list->word->word,
                     strerror (errno));
      return EXECUTION_FAILURE;
    }

  return EXECUTION_SUCCESS;
}

static const char *const unlink_doc[]
    = { "Remove a directory entry.", "",
        "Forcibly remove a directory entry, even if it's a directory.",
        nullptr };

Shell::builtin unlink_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::unlink_builtin),
    unlink_doc, "unlink name", nullptr, BUILTIN_ENABLED);

} // namespace bash
