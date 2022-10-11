/* wcsnwidth.cc - compute display width of wide character string, up to max
                  specified width, return length. */

/* Copyright (C) 2012 Free Software Foundation, Inc.

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

#include "config.h"

#if defined(HANDLE_MULTIBYTE)

#include <cwchar>

#include "externs.hh"

namespace bash
{

/* Return the number of wide characters that will be displayed from wide string
   PWCS.  If the display width exceeds MAX, return the number of wide chars
   from PWCS required to display MAX characters on the screen. */
ssize_t
wcsnwidth (const wchar_t *pwcs, size_t n, size_t max)
{
  wchar_t wc;

  size_t len = 0;
  const wchar_t *ws = pwcs;
  while (n-- > 0 && (wc = *ws++) != L'\0')
    {
      int l = ::wcwidth (wc);
      if (l < 0)
        return -1;
      else if (l == static_cast<int> (max - len))
        return ws - pwcs;
      else if (l > static_cast<int> (max - len))
        return --ws - pwcs;
      len += static_cast<unsigned int> (l);
    }
  return ws - pwcs;
}

} // namespace bash

#endif
