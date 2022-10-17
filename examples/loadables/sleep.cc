/*
 * sleep -- sleep for fractions of a second
 *
 * usage: sleep seconds[.fraction]
 */

/*
   Copyright (C) 1999-2020 Free Software Foundation, Inc.

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

#if defined(TIME_WITH_SYS_TIME)
#include <sys/time.h>
#include <time.h>
#else
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include "builtins.hh"
#include "common.hh"
#include "shell.hh"

namespace bash
{

// Loadable class for "sleep".
class ShellLoadable : public Shell
{
public:
  int sleep_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::sleep_builtin (WORD_LIST *list)
{
  long sec, usec;
  char *ep;
  int r, mul;
  time_t t;

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  /* Skip over `--' */
  if (list->word && ISOPTION (list->word->word, '-'))
    list = list->next ();

  if (*list->word->word == '-' || list->next ())
    {
      builtin_usage ();
      return EX_USAGE;
    }

  r = uconvert (list->word->word, &sec, &usec, &ep);
  /* Maybe postprocess conversion failures here based on EP */

  if (r)
    {
      fsleep (sec, usec);
      QUIT;
      return EXECUTION_SUCCESS;
    }

  builtin_error ("%s: bad sleep interval", list->word->word);
  return EXECUTION_FAILURE;
}

static const char *const sleep_doc[] = {
  "Suspend execution for specified period.",
  ""
  "sleep suspends execution for a minimum of SECONDS[.FRACTION] seconds.",
  nullptr
};

Shell::builtin sleep_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::sleep_builtin),
    sleep_doc, "sleep seconds[.fraction]", nullptr, BUILTIN_ENABLED);

} // namespace bash
