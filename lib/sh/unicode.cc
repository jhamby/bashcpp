/* unicode.cc - functions to convert unicode characters */

/* Copyright (C) 2010-2020 Free Software Foundation, Inc.

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>

#if defined(HAVE_LOCALE_CHARSET)
extern "C" const char *locale_charset (void);
#endif

#if defined(HAVE_LANGINFO_CODESET)
#include <langinfo.h>
#endif

#include "shell.hh"

namespace bash
{

#ifndef HAVE_LOCALE_CHARSET

char *
Shell::stub_charset ()
{
  const char *locale, *s;

  locale = get_locale_var ("LC_CTYPE");
  if (locale == nullptr || *locale == 0)
    {
      strcpy (charsetbuf, "ASCII");
      return charsetbuf;
    }
  s = strrchr (locale, '.');
  if (s)
    {
      strncpy (charsetbuf, s + 1, sizeof (charsetbuf) - 1);
      charsetbuf[sizeof (charsetbuf) - 1] = '\0';
      char *t = strchr (charsetbuf, '@');
      if (t)
        *t = 0;
      return charsetbuf;
    }
  strncpy (charsetbuf, locale, sizeof (charsetbuf) - 1);
  charsetbuf[sizeof (charsetbuf) - 1] = '\0';
  return charsetbuf;
}
#endif

void
Shell::u32reset ()
{
#if defined(HAVE_ICONV)
  if (u32init && localconv != reinterpret_cast<iconv_t> (-1))
    {
      iconv_close (localconv);
      localconv = reinterpret_cast<iconv_t> (-1);
    }
#endif
  u32init = false;
  utf8locale = false;
}

static inline int
u32tocesc (uint32_t wc, std::string &s)
{
  int l;

  s.resize (s.size () + 11); // max length + null terminator
  if (wc < 0x10000)
    l = sprintf (&(*(s.end () - 10)), "\\u%04X", wc);
  else
    l = sprintf (&(*(s.end () - 10)), "\\u%08X", wc);

  s.resize (strlen (&(*(s.begin ())))); // shrink to actual size
  return l;
}

// Convert unsigned 32-bit int and append to utf-8 character string.
static inline int
u32toutf8 (uint32_t wc, std::string &s)
{
  int l;

  if (wc < 0x0080)
    {
      s[0] = static_cast<char> (wc);
      l = 1;
    }
  else if (wc < 0x0800)
    {
      s[0] = static_cast<char> ((wc >> 6) | 0xc0);
      s[1] = static_cast<char> ((wc & 0x3f) | 0x80);
      l = 2;
    }
  else if (wc < 0x10000)
    {
      /* Technically, we could return 0 here if 0xd800 <= wc <= 0x0dfff */
      s[0] = static_cast<char> ((wc >> 12) | 0xe0);
      s[1] = static_cast<char> (((wc >> 6) & 0x3f) | 0x80);
      s[2] = static_cast<char> ((wc & 0x3f) | 0x80);
      l = 3;
    }
  else if (wc < 0x200000)
    {
      s[0] = static_cast<char> ((wc >> 18) | 0xf0);
      s[1] = static_cast<char> (((wc >> 12) & 0x3f) | 0x80);
      s[2] = static_cast<char> (((wc >> 6) & 0x3f) | 0x80);
      s[3] = static_cast<char> ((wc & 0x3f) | 0x80);
      l = 4;
    }
  /* Strictly speaking, UTF-8 doesn't have characters longer than 4 bytes */
  else if (wc < 0x04000000)
    {
      s[0] = static_cast<char> ((wc >> 24) | 0xf8);
      s[1] = static_cast<char> (((wc >> 18) & 0x3f) | 0x80);
      s[2] = static_cast<char> (((wc >> 12) & 0x3f) | 0x80);
      s[3] = static_cast<char> (((wc >> 6) & 0x3f) | 0x80);
      s[4] = static_cast<char> ((wc & 0x3f) | 0x80);
      l = 5;
    }
  else if (wc < 0x080000000)
    {
      s[0] = static_cast<char> ((wc >> 30) | 0xfc);
      s[1] = static_cast<char> (((wc >> 24) & 0x3f) | 0x80);
      s[2] = static_cast<char> (((wc >> 18) & 0x3f) | 0x80);
      s[3] = static_cast<char> (((wc >> 12) & 0x3f) | 0x80);
      s[4] = static_cast<char> (((wc >> 6) & 0x3f) | 0x80);
      s[5] = static_cast<char> ((wc & 0x3f) | 0x80);
      l = 6;
    }
  else
    l = 0;

  return l;
}

#if SIZEOF_WCHAR_T == 2
/* Convert a 32-bit unsigned int (unicode) to a UTF-16 string.  Rarely used,
   only if sizeof(wchar_t) == 2. */
static inline int
u32toutf16 (u_bits32_t c, wchar_t *s)
{
  int l;

  l = 0;
  if (c < 0x0d800 || (c >= 0x0e000 && c <= 0x0ffff))
    {
      s[0] = static_cast<wchar_t> (c & 0xFFFF);
      l = 1;
    }
  else if (c >= 0x10000 && c <= 0x010ffff)
    {
      c -= 0x010000;
      s[0] = static_cast<wchar_t> ((c >> 10) + 0xd800);
      s[1] = static_cast<wchar_t> ((c & 0x3ff) + 0xdc00);
      l = 2;
    }
  s[l] = 0;
  return l;
}
#endif

// Convert a single unicode-32 character into a multibyte string and append
// it to the C++ string passed by reference.
void
Shell::u32cconv (uint32_t c, std::string &s)
{
  wchar_t wc;
  wchar_t ws[3];
  int n;
#if defined(HAVE_ICONV)
  const char *charset;
  char obuf[25], *optr;
  size_t obytesleft;
  const char *iptr;
  size_t sn;
#endif

#if defined(__STDC_ISO_10646__)
  wc = static_cast<wchar_t> (c);
  if (sizeof (wchar_t) == 4 && c <= 0x7fffffff)
    n = wctomb (s.c_str (), wc);
#if SIZEOF_WCHAR_T == 2
  else if (sizeof (wchar_t) == 2 && c <= 0x10ffff && u32toutf16 (c, ws))
    n = wcstombs (s, ws, MB_LEN_MAX);
#endif
  else
    n = -1;
  if (n != -1)
    return n;
#endif

#if defined(HAVE_ICONV)
  /* this is mostly from coreutils-8.5/lib/unicodeio.c */
  if (!u32init)
    {
      utf8locale = locale_utf8locale;
      localconv = reinterpret_cast<iconv_t> (-1);
      if (!utf8locale)
        {
#if defined(HAVE_LOCALE_CHARSET)
          charset = locale_charset ();
#elif defined(HAVE_NL_LANGINFO)
          charset = nl_langinfo (CODESET);
#else
          charset = stub_charset ();
#endif
          localconv = iconv_open (charset, "UTF-8");
          if (localconv == reinterpret_cast<iconv_t> (-1))
            /* We assume ASCII when presented with an unknown encoding. */
            localconv = iconv_open ("ASCII", "UTF-8");
        }
      u32init = true;
    }

  /* NL_LANGINFO and locale_charset used when setting locale_utf8locale */

  /* If we have a UTF-8 locale, convert to UTF-8 and return converted value. */
  n = u32toutf8 (c, s);
  if (utf8locale)
    return n;

  /* If the conversion is not supported, even the ASCII requested above, we
     bail now.  Currently we return the UTF-8 conversion.  We could return
     u32tocesc(). */
  if (localconv == reinterpret_cast<iconv_t> (-1))
    return n;

  optr = obuf;
  obytesleft = sizeof (obuf);
  iptr = s;
  sn = static_cast<size_t> (n);

  iconv (localconv, nullptr, nullptr, nullptr, nullptr);

  if (iconv (localconv, const_cast<char **> (&iptr), &sn, &optr, &obytesleft)
      == static_cast<size_t> (-1))
    {
      /* You get ISO C99 escape sequences if iconv fails */
      n = u32tocesc (c, s);
      return n;
    }

  *optr = '\0';

  /* number of chars to be copied is optr - obuf if we want to do bounds
     checking */
  strcpy (s, obuf);
  return static_cast<int> (optr - obuf);

#else  /* !HAVE_ICONV */
  if (locale_utf8locale)
    n = u32toutf8 (c, s);
  else
    n = u32tocesc (c, s); /* fallback is ISO C99 escape sequences */
  return n;
#endif /* HAVE_ICONV */
}
#else
void
u32reset ()
{
}

#endif /* HANDLE_MULTIBYTE */

} // namespace bash
