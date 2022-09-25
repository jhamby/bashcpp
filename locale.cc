/* locale.cc - Miscellaneous internationalization functions. */

/* Copyright (C) 1996-2009,2012,2016,2020 Free Software Foundation, Inc.

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

#include "bashtypes.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif

#include "bashintl.hh"
#include "chartypes.hh"

#include "input.hh" /* For bash_input */
#include "shell.hh"

namespace bash
{

static std::string mk_msgstr (const std::string &, bool *);

#if defined(HAVE_LANGINFO_CODESET) || defined(HAVE_LOCALE_CHARSET)
static bool locale_isutf8 ();
#else
static bool locale_isutf8 (const char *);
#endif

/* Set the value of default_locale and make the current locale the
   system default locale.  This should be called very early in main(). */
void
Shell::set_default_locale ()
{
#if defined(HAVE_SETLOCALE)
  default_locale = std::setlocale (LC_ALL, "");
  if (default_locale)
    default_locale = savestring (default_locale);
#else
  default_locale = savestring ("C");
#endif /* HAVE_SETLOCALE */
  bindtextdomain (PACKAGE, LOCALEDIR);
  textdomain (PACKAGE);

  locale_mb_cur_max = MB_CUR_MAX;
#if defined(HAVE_LANGINFO_CODESET) || defined(HAVE_LOCALE_CHARSET)
  locale_utf8locale = locale_isutf8 ();
#else
  locale_utf8locale = locale_isutf8 (default_locale);
#endif
#if defined(HANDLE_MULTIBYTE)
  // XXX calling mblen with nullptr isn't portable. e.g. on Solaris 10,
  // the man page says that it always returns 0 when passed nullptr.
  locale_shiftstates = mblen (nullptr, 0);
#else
  local_shiftstates = 0;
#endif
}

/* Set default values for LC_CTYPE, LC_COLLATE, LC_MESSAGES, LC_NUMERIC and
   LC_TIME if they are not specified in the environment, but LC_ALL is.  This
   should be called from main() after parsing the environment. */
void
Shell::set_default_locale_vars ()
{
  char *val;

#if defined(HAVE_SETLOCALE)

#if defined(LC_CTYPE)
  val = get_string_value ("LC_CTYPE");
  if (val == nullptr && lc_all && *lc_all)
    {
      std::setlocale (LC_CTYPE, lc_all);
      locale_setblanks ();
      locale_mb_cur_max = MB_CUR_MAX;
#if defined(HAVE_LANGINFO_CODESET) || defined(HAVE_LOCALE_CHARSET)
      locale_utf8locale = locale_isutf8 ();
#else
      locale_utf8locale = locale_isutf8 (lc_all);
#endif

#if defined(HANDLE_MULTIBYTE)
      locale_shiftstates = std::mblen (nullptr, 0);
#else
      local_shiftstates = 0;
#endif

      u32reset ();
    }
#endif

#if defined(LC_COLLATE)
  val = get_string_value ("LC_COLLATE");
  if (val == nullptr && lc_all && *lc_all)
    std::setlocale (LC_COLLATE, lc_all);
#endif /* LC_COLLATE */

#if defined(LC_MESSAGES)
  val = get_string_value ("LC_MESSAGES");
  if (val == nullptr && lc_all && *lc_all)
    std::setlocale (LC_MESSAGES, lc_all);
#endif /* LC_MESSAGES */

#if defined(LC_NUMERIC)
  val = get_string_value ("LC_NUMERIC");
  if (val == nullptr && lc_all && *lc_all)
    std::setlocale (LC_NUMERIC, lc_all);
#endif /* LC_NUMERIC */

#if defined(LC_TIME)
  val = get_string_value ("LC_TIME");
  if (val == nullptr && lc_all && *lc_all)
    std::setlocale (LC_TIME, lc_all);
#endif /* LC_TIME */

#endif /* HAVE_SETLOCALE */

  val = get_string_value ("TEXTDOMAIN");
  if (val && *val)
    {
      delete[] default_domain;
      default_domain = savestring (val);
      if (default_dir && *default_dir)
        bindtextdomain (default_domain, default_dir);
    }

  val = get_string_value ("TEXTDOMAINDIR");
  if (val && *val)
    {
      delete[] default_dir;
      default_dir = savestring (val);
      if (default_domain && *default_domain)
        bindtextdomain (default_domain, default_dir);
    }
}

/* Set one of the locale categories (specified by VAR) to VALUE.  Returns 1
  if successful, 0 otherwise. */
bool
Shell::set_locale_var (const char *var, const char *value)
{
  const char *x = "";
  errno = 0;
  if (var[0] == 'T' && var[10] == 0) /* TEXTDOMAIN */
    {
      delete[] default_domain;
      default_domain = value ? savestring (value) : nullptr;
      if (default_dir && *default_dir)
        bindtextdomain (default_domain, default_dir);
      return true;
    }
  else if (var[0] == 'T') /* TEXTDOMAINDIR */
    {
      delete[] default_dir;
      default_dir = value ? savestring (value) : nullptr;
      if (default_domain && *default_domain)
        bindtextdomain (default_domain, default_dir);
      return true;
    }

  /* var[0] == 'L' && var[1] == 'C' && var[2] == '_' */

  else if (var[3] == 'A') /* LC_ALL */
    {
      delete[] lc_all;
      if (value)
        lc_all = savestring (value);
      else
        {
          lc_all = new char[1];
          lc_all[0] = '\0';
        }
#if defined(HAVE_SETLOCALE)
      bool r = *lc_all ? ((x = std::setlocale (LC_ALL, lc_all)) != nullptr)
                       : reset_locale_vars ();
      if (x == nullptr)
        {
          if (errno == 0)
            internal_warning (
                _ ("setlocale: LC_ALL: cannot change locale (%s)"), lc_all);
          else
            internal_warning (
                _ ("setlocale: LC_ALL: cannot change locale (%s): %s"), lc_all,
                strerror (errno));
        }
      locale_setblanks ();
      locale_mb_cur_max = MB_CUR_MAX;
      /* if LC_ALL == "", reset_locale_vars has already called this */
      if (*lc_all && x)
#if defined(HAVE_LANGINFO_CODESET) || defined(HAVE_LOCALE_CHARSET)
        locale_utf8locale = locale_isutf8 ();
#else
        locale_utf8locale = locale_isutf8 (lc_all);
#endif
#if defined(HANDLE_MULTIBYTE)
      locale_shiftstates = mblen (nullptr, 0);
#else
      local_shiftstates = 0;
#endif
      u32reset ();
      return r;
#else
      return true;
#endif
    }

#if defined(HAVE_SETLOCALE)
  else if (var[3] == 'C' && var[4] == 'T') /* LC_CTYPE */
    {
#if defined(LC_CTYPE)
      if (lc_all == nullptr || *lc_all == '\0')
        {
          x = std::setlocale (LC_CTYPE, get_locale_var ("LC_CTYPE"));
          locale_setblanks ();
          locale_mb_cur_max = MB_CUR_MAX;
          /* if setlocale() returns nullptr, the locale is not changed */
          if (x)
#if defined(HAVE_LANGINFO_CODESET) || defined(HAVE_LOCALE_CHARSET)
            locale_utf8locale = locale_isutf8 ();
#else
            locale_utf8locale = locale_isutf8 (x);
#endif
#if defined(HANDLE_MULTIBYTE)
          locale_shiftstates = std::mblen (nullptr, 0);
#else
          local_shiftstates = 0;
#endif
          u32reset ();
        }
#endif
    }
  else if (var[3] == 'C' && var[4] == 'O') /* LC_COLLATE */
    {
#if defined(LC_COLLATE)
      if (lc_all == nullptr || *lc_all == '\0')
        x = std::setlocale (LC_COLLATE, get_locale_var ("LC_COLLATE"));
#endif /* LC_COLLATE */
    }
  else if (var[3] == 'M' && var[4] == 'E') /* LC_MESSAGES */
    {
#if defined(LC_MESSAGES)
      if (lc_all == nullptr || *lc_all == '\0')
        x = std::setlocale (LC_MESSAGES, get_locale_var ("LC_MESSAGES"));
#endif /* LC_MESSAGES */
    }
  else if (var[3] == 'N' && var[4] == 'U') /* LC_NUMERIC */
    {
#if defined(LC_NUMERIC)
      if (lc_all == nullptr || *lc_all == '\0')
        x = std::setlocale (LC_NUMERIC, get_locale_var ("LC_NUMERIC"));
#endif /* LC_NUMERIC */
    }
  else if (var[3] == 'T' && var[4] == 'I') /* LC_TIME */
    {
#if defined(LC_TIME)
      if (lc_all == nullptr || *lc_all == '\0')
        x = std::setlocale (LC_TIME, get_locale_var ("LC_TIME"));
#endif /* LC_TIME */
    }
#endif /* HAVE_SETLOCALE */

  if (x == nullptr)
    {
      if (errno == 0)
        internal_warning (_ ("setlocale: %s: cannot change locale (%s)"), var,
                          get_locale_var (var));
      else
        internal_warning (_ ("setlocale: %s: cannot change locale (%s): %s"),
                          var, get_locale_var (var), std::strerror (errno));
    }

  return x != nullptr;
}

/* Called when LANG is assigned a value.  Tracks value in `lang'.  Calls
   reset_locale_vars() to reset any default values if LC_ALL is unset or
   null. */
int
Shell::set_lang (const char *, const char *value)
{
  delete[] lang;
  if (value)
    lang = savestring (value);
  else
    {
      lang = new char[1];
      lang[0] = '\0';
    }

  return (lc_all == nullptr || *lc_all == 0) ? reset_locale_vars () : 0;
}

/* Set default values for LANG and LC_ALL.  Default values for all other
   locale-related variables depend on these. */
void
Shell::set_default_lang ()
{
  char *v;

  v = get_string_value ("LC_ALL");
  set_locale_var ("LC_ALL", v);

  v = get_string_value ("LANG");
  set_lang ("LANG", v);
}

/* Get the value of one of the locale variables (LC_MESSAGES, LC_CTYPE).
   The precedence is as POSIX.2 specifies:  LC_ALL has precedence over
   the specific locale variables, and LANG, if set, is used as the default. */
const char *
Shell::get_locale_var (const char *var)
{
  const char *locale;

  locale = lc_all;

  if (locale == nullptr || *locale == 0)
    locale = get_string_value (var); /* XXX - no mem leak */
  if (locale == nullptr || *locale == 0)
    locale = lang;
  if (locale == nullptr || *locale == 0)
#if 0
    locale = default_locale;	/* system-dependent; not really portable.  should it be "C"? */
#else
    locale = "";
#endif
    return locale;
}

/* Called to reset all of the locale variables to their appropriate values
   if (and only if) LC_ALL has not been assigned a value.  DO NOT CALL THIS
   IF LC_ALL HAS BEEN ASSIGNED A VALUE. */
int
Shell::reset_locale_vars ()
{
  char *x;
#if defined(HAVE_SETLOCALE)
  if (lang == nullptr || *lang == '\0')
    maybe_make_export_env (); /* trust that this will change environment for
                                 setlocale */
  if (std::setlocale (LC_ALL, lang ? lang : "") == nullptr)
    return 0;

  x = nullptr;
#if defined(LC_CTYPE)
  x = std::setlocale (LC_CTYPE, get_locale_var ("LC_CTYPE"));
#endif
#if defined(LC_COLLATE)
  (void)std::setlocale (LC_COLLATE, get_locale_var ("LC_COLLATE"));
#endif
#if defined(LC_MESSAGES)
  (void)std::setlocale (LC_MESSAGES, get_locale_var ("LC_MESSAGES"));
#endif
#if defined(LC_NUMERIC)
  (void)std::setlocale (LC_NUMERIC, get_locale_var ("LC_NUMERIC"));
#endif
#if defined(LC_TIME)
  (void)std::setlocale (LC_TIME, get_locale_var ("LC_TIME"));
#endif

  locale_setblanks ();
  locale_mb_cur_max = MB_CUR_MAX;
  if (x)
#if defined(HAVE_LANGINFO_CODESET) || defined(HAVE_LOCALE_CHARSET)
    locale_utf8locale = locale_isutf8 ();
#else
    locale_utf8locale = locale_isutf8 (x);
#endif
#if defined(HANDLE_MULTIBYTE)
  locale_shiftstates = std::mblen (nullptr, 0);
#else
  local_shiftstates = 0;
#endif
  u32reset ();
#endif
  return 1;
}

// Translate the contents of STRING, a $"..." quoted string, according
// to the current locale.  In the `C' or `POSIX' locale, or if gettext()
// is not available, the passed string is returned unchanged.
std::string
Shell::localetrans (const std::string &string)
{
  /* Don't try to translate null strings. */
  if (string.empty ())
    {
      return std::string ();
    }

  const char *locale = get_locale_var ("LC_MESSAGES");

  /* If we don't have setlocale() or the current locale is `C' or `POSIX',
     just return the string.  If we don't have gettext(), there's no use
     doing anything else. */
  if (locale == nullptr || locale[0] == '\0'
      || (locale[0] == 'C' && locale[1] == '\0') || STREQ (locale, "POSIX"))
    {
      return string;
    }

  /* Now try to translate it. */

  const char *translated;
  const char *original = string.c_str ();

  if (default_domain && *default_domain)
    translated = dgettext (default_domain, original);
  else
    translated = original;

  if (translated == original) // gettext returns its argument if untranslatable
    return string;
  else
    return savestring (translated);
}

// Change a bash string into a string suitable for inclusion in a `po' file.
// This backslash-escapes `"' and `\' and changes newlines into \\\n"\n".
std::string
mk_msgstr (const std::string &string, bool *foundnlp)
{
  std::string ret;
  ret.reserve (string.length () + 2);

  ret.push_back ('"');

  std::string::const_iterator it;
  for (it = string.begin (); it != string.end (); ++it)
    {
      if (*it == '\n') /* <NL> -> \n"<NL>" */
        {
          ret.append ("\\n\"\n\"");
          if (foundnlp)
            *foundnlp = true;
          continue;
        }

      if (*it == '"' || *it == '\\')
        ret.push_back ('\\');

      ret.push_back (*it);
    }

  ret.push_back ('"');
  return ret;
}

// $"..." -- Translate STRING according to current locale using gettext (if
// available) and return the result. The caller will take care of leaving the
// quotes intact. The string will be left without the leading `$' by the
// caller. If translation is performed, the translated string will be
// double-quoted by the caller.
std::string
Shell::localeexpand (const std::string &string, int lineno)
{
  /* If we're just dumping translatable strings, don't do anything with the
     string itself, but if we're dumping in `po' file format, convert it into
     a form more palatable to gettext(3) and friends by quoting `"' and `\'
     with backslashes and converting <NL> into `\n"<NL>"'.  If we find a
     newline in TEMP, we first output a `msgid ""' line and then the
     translated string; otherwise we output the `msgid' and translated
     string all on one line. */
  if (dump_translatable_strings)
    {
      if (dump_po_strings)
        {
          bool foundnl = false;
          std::string t = mk_msgstr (string.c_str (), &foundnl);
          const char *t2 = foundnl ? "\"\"\n" : "";

          std::printf ("#: %s:%d\nmsgid %s%s\nmsgstr \"\"\n", yy_input_name (),
                       lineno, t2, t.c_str ());
        }
      else
        std::printf ("\"%s\"\n", string.c_str ());

      return string;
    }
  else if (!string.empty ())
    return localetrans (string);
  else
    return string;
}

/* Set every character in the <blank> character class to be a shell break
   character for the lexical analyzer when the locale changes. */
void
Shell::locale_setblanks ()
{
  int x;

  for (x = 0; x < SYNSIZE; x++)
    {
      if (std::isblank (static_cast<unsigned char> (x)))
        sh_syntaxtab[x] |= (CSHBRK | CBLANK);
      else if (member (static_cast<char> (x), shell_break_chars))
        {
          sh_syntaxtab[x] |= CSHBRK;
          sh_syntaxtab[x] &= ~CBLANK;
        }
      else
        sh_syntaxtab[x] &= ~(CSHBRK | CBLANK);
    }
}

/* Parse a locale specification
     language[_territory][.codeset][@modifier][+special][,[sponsor][_revision]]
   and return TRUE if the codeset is UTF-8 or utf8 */
static bool
#if defined(HAVE_LANGINFO_CODESET) || defined(HAVE_LOCALE_CHARSET)
locale_isutf8 ()
#else
locale_isutf8 (const char *lspec)
#endif
{
  char *cp;

#if defined(HAVE_LANGINFO_CODESET)
  cp = nl_langinfo (CODESET);
  return STREQ (cp, "UTF-8") || STREQ (cp, "utf8");
#elif defined(HAVE_LOCALE_CHARSET)
  cp = locale_charset ();
  return STREQ (cp, "UTF-8") || STREQ (cp, "utf8");
#else
  /* Take a shot */
  for (cp = lspec; *cp && *cp != '@' && *cp != '+' && *cp != ','; cp++)
    {
      if (*cp == '.')
        {
          char *encoding;
          for (encoding = ++cp; *cp && *cp != '@' && *cp != '+' && *cp != ',';
               cp++)
            ;
          /* The encoding (codeset) is the substring between encoding and cp */
          if ((cp - encoding == 5 && STREQN (encoding, "UTF-8", 5))
              || (cp - encoding == 4 && STREQN (encoding, "utf8", 4)))
            return true;
          else
            return false;
        }
    }
  return false;
#endif
}

} // namespace bash
