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

#if defined(HAVE_LANGINFO_CODESET)
#include <langinfo.h>
#endif

namespace readline
{

static bool utf8locale (const char *);

#if !defined(HAVE_SETLOCALE)
/* A list of legal values for the LANG or LC_CTYPE environment variables.
   If a locale name in this list is the value for the LC_ALL, LC_CTYPE,
   or LANG environment variable (using the first of those with a value),
   readline eight-bit mode is enabled. */
static const char *legal_lang_values[]
    = { "iso88591", "iso88592", "iso88593", "iso88594", "iso88595",
        "iso88596", "iso88597", "iso88598", "iso88599", "iso885910",
        "koi8r",    "utf8",     0 };

static char *normalize_codeset (char *);
#endif /* !HAVE_SETLOCALE */

static char *find_codeset (char *, size_t *);

static char *_rl_get_locale_var (const char *);

static char *
_rl_get_locale_var (const char *v)
{
  char *lspec;

  lspec = sh_get_env_value ("LC_ALL");
  if (lspec == 0 || *lspec == 0)
    lspec = sh_get_env_value (v);
  if (lspec == 0 || *lspec == 0)
    lspec = sh_get_env_value ("LANG");

  return lspec;
}

static int
utf8locale (char *lspec)
{
  char *cp;

#if defined(HAVE_LANGINFO_CODESET)
  cp = nl_langinfo (CODESET);
  return STREQ (cp, "UTF-8") || STREQ (cp, "utf8");
#else
  size_t len;
  cp = find_codeset (const_cast<char *> (lspec), &len);

  if (cp == 0 || len < 4 || len > 5)
    return 0;
  return (len == 5) ? strncmp (cp, "UTF-8", len) == 0
                    : strncmp (cp, "utf8", 4) == 0;
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
#if defined (HAVE_SETLOCALE)
  if (lspec == 0 || *lspec == 0)
    lspec = setlocale (LC_CTYPE, (char *)NULL);
  if (lspec == 0)
    lspec = "";
  ret = setlocale (LC_CTYPE, lspec);	/* ok, since it does not change locale */
  if (ret == 0 || *ret == 0)
    ret = setlocale (LC_CTYPE, (char *)NULL);
  if (ret == 0 || *ret == 0)
    ret = RL_DEFAULT_LOCALE;
#else
  ret = (lspec == 0 || *lspec == 0) ? RL_DEFAULT_LOCALE : lspec;
#endif

  _rl_utf8locale = (ret && *ret) ? utf8locale (ret) : 0;

  _rl_current_locale = savestring (ret);
  return ret;
}

/* If we have setlocale(3), just check the current LC_CTYPE category
   value (passed as LOCALESTR), and go into eight-bit mode if it's not "C"
   or "POSIX". If FORCE is non-zero, we reset the locale variables to values
   appropriate for the C locale if the locale is "C" or "POSIX". FORCE is 0
   when this is called from _rl_init_eightbit, since we're modifying the
   default initial values and don't need to change anything else. If we
   don't have setlocale(3), we check the codeset portion of LOCALESTR against
   a set of known values and go into eight-bit mode if it matches one of those.
   Returns 1 if we set eight-bit (multibyte) mode. */
static int
_rl_set_localevars (char *localestr, int force)
{
#if defined (HAVE_SETLOCALE)
  if (localestr && *localestr && (localestr[0] != 'C' || localestr[1]) && (STREQ (localestr, "POSIX") == 0))
    {
      _rl_meta_flag = true;
      _rl_convert_meta_chars_to_ascii = false;
      _rl_output_meta_chars = true;
    }
  else if (force)
    {
      /* Default "C" locale settings. */
      _rl_meta_flag = 0;
      _rl_convert_meta_chars_to_ascii = 1;
      _rl_output_meta_chars = 0;
      return (0);
    }
  else
    return (0);

#else /* !HAVE_SETLOCALE */
  char *t;
  int i;

  /* We don't have setlocale.  Finesse it.  Check the environment for the
     appropriate variables and set eight-bit mode if they have the right
     values. */
  if (localestr == 0 || (t = normalize_codeset (localestr)) == 0)
    return (0);
  for (i = 0; t && legal_lang_values[i]; i++)
    if (STREQ (t, legal_lang_values[i]))
      {
        _rl_meta_flag = true;
        _rl_convert_meta_chars_to_ascii = false;
        _rl_output_meta_chars = true;
        break;
      }

  if (force && legal_lang_values[i] == 0)	/* didn't find it */
    {
      /* Default "C" locale settings. */
      _rl_meta_flag = 0;
      _rl_convert_meta_chars_to_ascii = 1;
      _rl_output_meta_chars = 0;
    }

  _rl_utf8locale = *t ? STREQ (t, "utf8") : 0;

  delete[] t;
#endif /* !HAVE_SETLOCALE */
}

/* Check for LC_ALL, LC_CTYPE, and LANG and use the first with a value
   to decide the defaults for 8-bit character input and output.  Returns
   1 if we set eight-bit mode. */
int
_rl_init_eightbit (void)
{
  char *t, *ol;

  ol = _rl_current_locale;
  t = _rl_init_locale ();	/* resets _rl_current_locale, returns static pointer */
  xfree (ol);

  return (_rl_set_localevars (t, 0));
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
      if (c_isalnum (codeset[i]))
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
    if (c_isalpha (codeset[i]))
      *wp++ = _rl_to_lower (codeset[i]);
    else if (_rl_digit_p (codeset[i]))
      *wp++ = codeset[i];
  *wp = '\0';

  return retval;
}
#endif /* !HAVE_SETLOCALE */

#if !defined(HAVE_LANGINFO_CODESET) || !defined(HAVE_SETLOCALE)
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
      *lenp = strlen (language);
      result = language;
    }
  else
    {
      /* Next is the territory. */
      if (*cp == '_')
        do
          ++cp;
        while (*cp && *cp != '.' && *cp != '@' && *cp != '+' && *cp != ','
               && *cp != '_');

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
          *lenp = strlen (language);
          result = language;
        }
    }

  return result;
}

void
_rl_reset_locale (void)
{
  char *ol, *nl;

  /* This should not be NULL; _rl_init_eightbit sets it on the first call to
     readline() or rl_initialize(). */
  ol = _rl_current_locale;
  nl = _rl_init_locale ();		/* resets _rl_current_locale */

  if ((ol == 0 && nl) || (ol && nl && (STREQ (ol, nl) == 0)))
    (void)_rl_set_localevars (nl, 1);

  xfree (ol);
}

} // namespace readline
