/* bashintl.h -- Internationalization functions and defines. */

/* Copyright (C) 1996-2009 Free Software Foundation, Inc.

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

#if !defined (_BASHINTL_H_)
#define _BASHINTL_H_

#if defined (BUILDTOOL)
#  undef ENABLE_NLS
#  define ENABLE_NLS 0
#endif

/* Include this *after* config.h */
#include "gettext.hh"

#include <clocale>

namespace bash
{

#define _(msgid)	gettext(msgid)
#define N_(msgid)	const_cast<char*>(msgid)
#define D_(d, msgid)	dgettext(d, msgid)

#define P_(m1, m2, n)	ngettext(m1, m2, n)

#if defined (HAVE_SETLOCALE) && !defined (LC_ALL)
#  undef HAVE_SETLOCALE
#endif

// moved from locale.c for performance

#if defined (HAVE_LOCALECONV)
static inline char
locale_decpoint ()
{
  struct lconv *lv;

  lv = std::localeconv ();
  return (lv && lv->decimal_point && lv->decimal_point[0]) ? lv->decimal_point[0] : '.';
}
#else
static inline char
locale_decpoint ()
{
  return '.';
}
#endif

}  // namespace bash

#endif /* !_BASHINTL_H_ */
