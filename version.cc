/* version.c -- distribution and version numbers. */

/* Copyright (C) 1989-2020 Free Software Foundation, Inc.

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

#include <cstdio>

#include "version.hh"
#include "patchlevel.hh"
#include "conftypes.hh"

#include "bashintl.hh"

namespace bash
{

extern const char *shell_name;

/* Defines from version.h */
extern const char * const dist_version = DISTVERSION;
extern const int patch_level = PATCHLEVEL;
const int build_version = BUILDVERSION;
#ifdef RELSTATUS
const char * const release_status = RELSTATUS;
#else
const char * const release_status = nullptr;
#endif

// A version string for use by the what command. Use a separate
// declaration to satisfy -Wmissing-variable-declarations.
extern const char * const sccs_version;
extern const char * const sccs_version = SCCSVERSION;

const char * const bash_copyright = N_("Copyright (C) 2020 Free Software Foundation, Inc.");
const char * const bash_license = N_("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");

/* Functions for getting, setting, and displaying the shell version. */

/* Forward declarations so we don't have to include externs.h */
extern char *shell_version_string ();
extern void show_shell_version (bool);

/* Give version information about this shell. */
char *
shell_version_string ()
{
  static char tt[32] = { '\0' };

  if (tt[0] == '\0')
    {
      if (release_status)
	std::snprintf (tt, sizeof (tt), "%s.%d(%d)-%s", dist_version, patch_level, build_version, release_status);
      else
	std::snprintf (tt, sizeof (tt), "%s.%d(%d)", dist_version, patch_level, build_version);
    }
  return tt;
}

void
show_shell_version (bool extended)
{
  std::printf (_("GNU bash, version %s (%s)\n"), shell_version_string (), MACHTYPE);
  if (extended)
    {
      std::printf ("%s\n", _(bash_copyright));
      std::printf ("%s\n", _(bash_license));
      std::printf ("%s\n", _("This is free software; you are free to change and redistribute it."));
      std::printf ("%s\n", _("There is NO WARRANTY, to the extent permitted by law."));
    }
}

}  // namespace bash
