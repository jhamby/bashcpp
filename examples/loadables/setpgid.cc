/* setpgid.c: bash loadable wrapper for setpgid system call

   An example of how to wrap a system call with a loadable builtin.

   Originally contributed by Jason Vas Dias <jason.vas.dias@gmail.com>

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

#if !defined(_POSIX_VERSION)
#define setpgid(pid, pgrp) setpgrp (pid, pgrp)
#endif

namespace bash
{

// Loadable class for "setpgid".
class ShellLoadable : public Shell
{
public:
  int setpgid_builtin (WORD_LIST *);

private:
};

int
ShellLoadable::setpgid_builtin (WORD_LIST *list)
{
  WORD_LIST *wl;
  int64_t pid_arg, pgid_arg;
  pid_t pid, pgid;
  char *pidstr, *pgidstr;

  wl = list;
  pid = pgid = 0;

  if (wl == 0 || wl->next == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  pidstr = wl->word ? wl->word->word : 0;
  pgidstr = wl->next->word ? wl->next->word->word : 0;

  if (pidstr == 0 || pgidstr == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  if (legal_number (pidstr, &pid_arg) == 0)
    {
      builtin_error ("%s: pid argument must be numeric", pidstr);
      return EXECUTION_FAILURE;
    }
  if (pid_arg < 0)
    {
      builtin_error ("%s: negative pid  values not allowed", pidstr);
      return EXECUTION_FAILURE;
    }
  pid = pid_arg;

  if (legal_number (pgidstr, &pgid_arg) == 0)
    {
      builtin_error ("%s: pgrp argument must be numeric", pgidstr);
      return EXECUTION_FAILURE;
    }
  if (pgid_arg < 0)
    {
      builtin_error ("%s: negative pgrp values not allowed", pgidstr);
      return EXECUTION_FAILURE;
    }
  pgid = pgid_arg;

  errno = 0;
  if (setpgid (pid, pgid) < 0)
    {
      builtin_error ("setpgid failed: %s", strerror (errno));
      return EXECUTION_FAILURE;
    }
  return EXECUTION_SUCCESS;
}

static const char *const setpgid_doc[]
    = { "invoke the setpgid(2) system call",
        "",
        "Arguments:",
        "   pid : numeric process identifier, >= 0",
        "   pgrpid: numeric process group identifier, >=0",
        "See the setpgid(2) manual page.",
        nullptr };

Shell::builtin setpgid_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::setpgid_builtin),
    setpgid_doc, "setpgid pid pgrpid", nullptr, BUILTIN_ENABLED);

} // namespace bash
