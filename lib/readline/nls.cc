/* nls.cc -- skeletal internationalization code. */

/* Copyright (C) 1996-2022 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library (Readline), a library
   for reading lines of text with interactive input and history editing.

   Readline is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Readline is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "readline.hh"
#include "rlprivate.hh"

#include <sys/types.h>

#include <clocale>

#include <langinfo.h>

namespace readline
{

#define RL_DEFAULT_LOCALE "C"

static inline bool
utf8locale ()
{
  const char *cp = nl_langinfo (CODESET);
  return STREQ (cp, "UTF-8") || STREQ (cp, "utf8");
}

/* Query the right environment variables and call setlocale() to initialize
   the C library locale settings. */
char *
Readline::_rl_init_locale ()
{
  /* Set the LC_CTYPE locale category from environment variables. */
  const char *lspec = _rl_get_locale_var ("LC_CTYPE");

  /* Since _rl_get_locale_var queries the right environment variables,
     we query the current locale settings with setlocale(), and, if
     that doesn't return anything, we set lspec to the empty string to
     force the subsequent call to setlocale() to define the `native'
     environment. */
  if (lspec == nullptr || *lspec == 0)
    lspec = setlocale (LC_CTYPE, nullptr);
  if (lspec == nullptr)
    lspec = "";

  const char *ret
      = setlocale (LC_CTYPE, lspec); /* ok, since it does not change locale */

  if (ret == nullptr || *ret == 0)
    ret = setlocale (LC_CTYPE, nullptr);

  if (ret == nullptr || *ret == 0)
    ret = RL_DEFAULT_LOCALE;

  _rl_utf8locale = (ret && *ret) ? utf8locale () : false;

  _rl_current_locale = savestring (ret);
  return _rl_current_locale;
}

/* If we have setlocale(3), just check the current LC_CTYPE category
   value (passed as LOCALESTR), and go into eight-bit mode if it's not "C"
   or "POSIX". If FORCE is non-zero, we reset the locale variables to values
   appropriate for the C locale if the locale is "C" or "POSIX". FORCE is 0
   when this is called from _rl_init_eightbit, since we're modifying the
   default initial values and don't need to change anything else. */
bool
Readline::_rl_set_localevars (const char *localestr, bool force)
{
  if (localestr && *localestr && (localestr[0] != 'C' || localestr[1])
      && (STREQ (localestr, "POSIX") == 0))
    {
      _rl_meta_flag = true;
      _rl_convert_meta_chars_to_ascii = false;
      _rl_output_meta_chars = true;
      return true;
    }
  else if (force)
    {
      /* Default "C" locale settings. */
      _rl_meta_flag = false;
      _rl_convert_meta_chars_to_ascii = true;
      _rl_output_meta_chars = false;
      return false;
    }
  else
    return false;
}

} // namespace readline
