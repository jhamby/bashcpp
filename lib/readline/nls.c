/* nls.c -- skeletal internationalization code. */

/* Copyright (C) 1996-2017 Free Software Foundation, Inc.

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

#include "readline.h"
#include "rlprivate.h"

#include <sys/types.h>

#if defined (HAVE_LANGINFO_CODESET)
#  include <langinfo.h>
#endif

namespace readline
{

static bool utf8locale (const char *);

#if !defined (HAVE_SETLOCALE)
/* A list of legal values for the LANG or LC_CTYPE environment variables.
   If a locale name in this list is the value for the LC_ALL, LC_CTYPE,
   or LANG environment variable (using the first of those with a value),
   readline eight-bit mode is enabled. */
static const char *legal_lang_values[] =
{
 "iso88591",
 "iso88592",
 "iso88593",
 "iso88594",
 "iso88595",
 "iso88596",
 "iso88597",
 "iso88598",
 "iso88599",
 "iso885910",
 "koi8r",
 "utf8",
  0
};

static char *normalize_codeset (char *);
#endif /* !HAVE_SETLOCALE */

#if !defined (HAVE_LANGINFO_CODESET) || !defined (HAVE_SETLOCALE)
static char *find_codeset (char *, size_t *);
#endif  // !HAVE_LANGINFO_CODESET || !HAVE_SETLOCALE

bool
#if defined (HAVE_LANGINFO_CODESET)
utf8locale (const char *)
#else
utf8locale (const char *lspec)
#endif
{
  char *cp;

#if defined (HAVE_LANGINFO_CODESET)
  cp = ::nl_langinfo (CODESET);
  return STREQ (cp, "UTF-8") || STREQ (cp, "utf8");
#else
  size_t len;
  cp = find_codeset (const_cast<char *> (lspec), &len);

  if (cp == 0 || len < 4 || len > 5)
    return 0;
  return (len == 5) ? std::strncmp (cp, "UTF-8", len) == 0 : std::strncmp (cp, "utf8", 4) == 0;
#endif
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
  if (lspec == nullptr || *lspec == '\0')
    lspec = std::setlocale (LC_CTYPE, nullptr);
  if (lspec == nullptr)
    lspec = "";

  char *ret = std::setlocale (LC_CTYPE, lspec);	/* ok, since it does not change locale */

  _rl_utf8locale = (ret && *ret) ? utf8locale (ret) : 0;

  return ret;
}

/* Check for LC_ALL, LC_CTYPE, and LANG and use the first with a value
   to decide the defaults for 8-bit character input and output. */
void
Readline::_rl_init_eightbit ()
{
/* If we have setlocale(3), just check the current LC_CTYPE category
   value, and go into eight-bit mode if it's not C or POSIX. */
#if defined (HAVE_SETLOCALE)
  char *t = _rl_init_locale ();	/* returns static pointer */

  if (t && *t && (t[0] != 'C' || t[1]) && (STREQ (t, "POSIX") == 0))
    {
      _rl_meta_flag = true;
      _rl_convert_meta_chars_to_ascii = false;
      _rl_output_meta_chars = true;
    }

#else /* !HAVE_SETLOCALE */
  char *lspec, *t;
  int i;

  /* We don't have setlocale.  Finesse it.  Check the environment for the
     appropriate variables and set eight-bit mode if they have the right
     values. */
  lspec = _rl_get_locale_var ("LC_CTYPE");

  if (lspec == 0 || (t = normalize_codeset (lspec)) == 0)
    return 0;
  for (i = 0; t && legal_lang_values[i]; i++)
    if (STREQ (t, legal_lang_values[i]))
      {
	_rl_meta_flag = true;
	_rl_convert_meta_chars_to_ascii = false;
	_rl_output_meta_chars = true;
	break;
      }

  _rl_utf8locale = *t ? STREQ (t, "utf8") : 0;

  delete[] t;
#endif /* !HAVE_SETLOCALE */
}

#if !defined (HAVE_SETLOCALE)
static char *
normalize_codeset (char *codeset)
{
  size_t namelen, i;
  int len;
  char *wp, *retval;

  codeset = find_codeset (codeset, &namelen);

  if (codeset == 0)
    return codeset;

  bool all_digits = true;
  for (len = 0, i = 0; i < namelen; i++)
    {
      if (std::isalnum (static_cast<unsigned char> (codeset[i])))
	{
	  len++;
	  all_digits &= _rl_digit_p (codeset[i]);
	}
    }

  retval = new char[(all_digits ? 3 : 0) + len + 1];

  wp = retval;
  /* Add `iso' to beginning of an all-digit codeset */
  if (all_digits)
    {
      *wp++ = 'i';
      *wp++ = 's';
      *wp++ = 'o';
    }

  for (i = 0; i < namelen; i++)
    if (std::isalpha (static_cast<unsigned char> (codeset[i])))
      *wp++ = _rl_to_lower (codeset[i]);
    else if (_rl_digit_p (codeset[i]))
      *wp++ = codeset[i];
  *wp = '\0';

  return retval;
}
#endif /* !HAVE_SETLOCALE */

#if !defined (HAVE_LANGINFO_CODESET) || !defined (HAVE_SETLOCALE)
/* Isolate codeset portion of locale specification. */
static char *
find_codeset (char *name, size_t *lenp)
{
  char *cp, *language, *result;

  cp = language = name;
  result = NULL;

  while (*cp && *cp != '_' && *cp != '@' && *cp != '+' && *cp != ',')
    cp++;

  /* This does not make sense: language has to be specified.  As
     an exception we allow the variable to contain only the codeset
     name.  Perhaps there are funny codeset names.  */
  if (language == cp)
    {
      *lenp = std::strlen (language);
      result = language;
    }
  else
    {
      /* Next is the territory. */
      if (*cp == '_')
	do
	  ++cp;
	while (*cp && *cp != '.' && *cp != '@' && *cp != '+' && *cp != ',' && *cp != '_');

      /* Now, finally, is the codeset. */
      result = cp;
      if (*cp == '.')
	do
	  ++cp;
	while (*cp && *cp != '@');

      if (cp - result > 2)
	{
	  result++;
	  *lenp = cp - result;
	}
      else
	{
	  *lenp = std::strlen (language);
	  result = language;
	}
    }

  return result;
}
#endif  // !HAVE_LANGINFO_CODESET || !HAVE_SETLOCALE

}  // namespace readline
