/* fnxform - use iconv(3) to transform strings to and from "filename" format */

/* Copyright (C) 2009-2020 Free Software Foundation, Inc.

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

#include <cstdio>
#include <cstring>

#include "bashintl.hh"
#include "bashtypes.hh"

#include "externs.hh"

#if defined(HAVE_ICONV)
#include <iconv.h>
#endif

#if defined(HAVE_LOCALE_CHARSET)
extern "C" const char *locale_charset (void);
#endif

namespace bash
{

#ifdef MACOSX
static iconv_t conv_fromfs = reinterpret_cast<iconv_t> (-1);
static iconv_t conv_tofs = reinterpret_cast<iconv_t> (-1);

#define OUTLEN_MAX 4096

static char *outbuf = 0;
static size_t outlen = 0;

static const char *curencoding (void);
static void init_tofs (void);
static void init_fromfs (void);

static const char *
curencoding ()
{
  char *loc;
#if defined(HAVE_LOCALE_CHARSET)
  loc = (char *)locale_charset ();
  return loc;
#else

  loc = get_locale_var ("LC_CTYPE");
  if (loc == 0 || *loc == 0)
    return "";
  char *dot = std::strchr (loc, '.');
  if (dot == 0)
    return loc;
  char *mod = std::strchr (dot, '@');
  if (mod)
    *mod = '\0';
  return ++dot;
#endif
}

static void
init_tofs ()
{
  const char *cur;

  cur = curencoding ();
  conv_tofs = iconv_open ("UTF-8-MAC", cur);
}

static void
init_fromfs ()
{
  const char *cur;

  cur = curencoding ();
  conv_fromfs = iconv_open (cur, "UTF-8-MAC");
}

char *
fnx_tofs (char *string, size_t len)
{
  char *inbuf;
  char *tempbuf;
  size_t templen;

  if (conv_tofs == reinterpret_cast<iconv_t> (-1))
    init_tofs ();
  if (conv_tofs == reinterpret_cast<iconv_t> (-1))
    return string;

  /* Free and reallocate outbuf if it's *too* big */
  if (outlen >= OUTLEN_MAX && len < OUTLEN_MAX - 8)
    {
      free (outbuf);
      outbuf = 0;
      outlen = 0;
    }

  inbuf = string;
  if (outbuf == 0 || outlen < len + 8)
    {
      outlen = len + 8;
      outbuf = outbuf ? xrealloc (outbuf, outlen + 1) : xmalloc (outlen + 1);
    }
  tempbuf = outbuf;
  templen = outlen;

  iconv (conv_tofs, NULL, NULL, NULL, NULL);

  if (iconv (conv_tofs, &inbuf, &len, &tempbuf, &templen) == (size_t)-1)
    return string;

  *tempbuf = '\0';
  return outbuf;
}

char *
fnx_fromfs (char *string, size_t len)
{
  char *inbuf;
  char *tempbuf;
  size_t templen;

  if (conv_fromfs == reinterpret_cast<iconv_t> (-1))
    init_fromfs ();
  if (conv_fromfs == reinterpret_cast<iconv_t> (-1))
    return string;

  /* Free and reallocate outbuf if it's *too* big */
  if (outlen >= OUTLEN_MAX && len < OUTLEN_MAX - 8)
    {
      free (outbuf);
      outbuf = 0;
      outlen = 0;
    }

  inbuf = string;
  if (outbuf == 0 || outlen < (len + 8))
    {
      outlen = len + 8;
      outbuf = outbuf ? xrealloc (outbuf, outlen + 1) : xmalloc (outlen + 1);
    }
  tempbuf = outbuf;
  templen = outlen;

  iconv (conv_fromfs, NULL, NULL, NULL, NULL);

  if (iconv (conv_fromfs, &inbuf, &len, &tempbuf, &templen) == (size_t)-1)
    return string;

  *tempbuf = '\0';
  return outbuf;
}
#endif

} // namespace bash
