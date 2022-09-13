/* makepath.c - glue PATH and DIR together into a full pathname. */

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

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "externs.hh"
#include "shell.hh"

namespace bash
{

static const char *nullpath = "";

/* Take PATH, an element from, e.g., $CDPATH, and DIR, a directory name,
   and paste them together into PATH/DIR.  Tilde expansion is performed on
   PATH if (flags & MP_DOTILDE) is non-zero.  If PATH is the empty
   string, it is converted to the current directory.  A full pathname is
   used if (flags & MP_DOCWD) is non-zero, otherwise `./' is used.  If
   (flags & MP_RMDOT) is non-zero, any `./' is removed from the beginning
   of DIR.  If (flags & MP_IGNDOT) is non-zero, a PATH that is "." or "./"
   is ignored. */

#define MAKEDOT()                                                             \
  do                                                                          \
    {                                                                         \
      xpath = new char[2];                                                    \
      xpath[0] = '.';                                                         \
      xpath[1] = '\0';                                                        \
      pathlen = 1;                                                            \
    }                                                                         \
  while (0)

char *
Shell::sh_makepath (const std::string &path, const std::string &dir,
                    mp_flags flags)
{
  size_t dirlen, pathlen;
  const char *xdir, *s;
  char *ret, *xpath, *r;

  if (path.empty ())
    {
      if (flags & MP_DOCWD)
        {
          xpath = get_working_directory ("sh_makepath");
          if (xpath == nullptr)
            {
              ret = get_string_value ("PWD");
              if (ret)
                xpath = savestring (ret);
            }
          if (xpath == nullptr)
            MAKEDOT ();
          else
            pathlen = std::strlen (xpath);
        }
      else
        MAKEDOT ();
    }
  else if ((flags & MP_IGNDOT) && path[0] == '.'
           && (path[1] == '\0' || (path[1] == '/' && path[2] == '\0')))
    {
      xpath = const_cast<char *> (
          nullpath); // hopefully 'nullpath' is always the same pointer
      pathlen = 0;
    }
  else
    {
      xpath = ((flags & MP_DOTILDE) && path[0] == '~')
                  ? bash_tilde_expand (path.c_str (), 0)
                  : const_cast<char *> (path.c_str ());
      pathlen = std::strlen (xpath);
    }

  xdir = dir.c_str ();
  dirlen = dir.size ();
  if ((flags & MP_RMDOT) && dir[0] == '.' && dir[1] == '/')
    {
      xdir += 2;
      dirlen -= 2;
    }

  r = ret = new char[2 + dirlen + pathlen];
  s = xpath;

  while (*s)
    *r++ = *s++;

  if (s > xpath && s[-1] != '/')
    *r++ = '/';

  s = xdir;

  while ((*r++ = *s++))
    ;

  // hopefully we don't get a different pointer value for path.c_str () than
  // we did earlier in the function.
  if (xpath != path.c_str () && xpath != nullpath)
    delete[] xpath;

  return ret;
}

} // namespace bash
