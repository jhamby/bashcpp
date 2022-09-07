/* xmbsrtowcs.c -- replacement function for mbsrtowcs */

/* Copyright (C) 2002-2020 Free Software Foundation, Inc.

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

/* Ask for GNU extensions to get extern declaration for mbsnrtowcs if
   available via glibc. */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif

#include <config.h>

/* <wchar.h>, <wctype.h> and <stdlib.h> are included in "shmbutil.h".
   If <wchar.h>, <wctype.h>, mbsrtowcs(), exist, HANDLE_MULTIBYTE
   is defined as 1. */
#include <shmbutil.h>

#if HANDLE_MULTIBYTE

#include <cerrno>
#include <cstdlib>	// for MB_CUR_MAX

#include <vector>

const size_t WSBUF_INC = 32;

#if ! HAVE_STRCHRNUL
extern "C" char *strchrnul (const char *, int);
#endif

/* On some locales (ex. ja_JP.sjis), mbsrtowc doesn't convert 0x5c to U<0x5c>.
   So, this function is made for converting 0x5c to U<0x5c>. */

static std::mbstate_t local_state;
static bool local_state_use = false;

size_t
xmbsrtowcs (wchar_t *dest, const char **src, size_t len, mbstate_t *pstate)
{
  std::mbstate_t *ps;
  size_t mblength, wclength, n;

  ps = pstate;
  if (pstate == NULL)
    {
      if (!local_state_use)
	{
	  std::memset (&local_state, 0, sizeof(std::mbstate_t));
	  local_state_use = true;
	}
      ps = &local_state;
    }

  n = std::strlen (*src);

  if (dest == NULL)
    {
      wchar_t *wsbuf;
      const char *mbs;
      std::mbstate_t psbuf;

      /* If new fails here, we'll throw. */
      wsbuf = new wchar_t[n + 1];
      mbs = *src;
      psbuf = *ps;

      wclength = std::mbsrtowcs (wsbuf, &mbs, n, &psbuf);

      delete[] wsbuf;
      return wclength;
    }

  for (wclength = 0; wclength < len; wclength++, dest++)
    {
      if (std::mbsinit(ps))
	{
	  if (**src == '\0')
	    {
	      *dest = L'\0';
	      *src = NULL;
	      return wclength;
	    }
	  else if (**src == '\\')
	    {
	      *dest = L'\\';
	      mblength = 1;
	    }
	  else
	    mblength = std::mbrtowc(dest, *src, n, ps);
	}
      else
	mblength = std::mbrtowc(dest, *src, n, ps);

      /* Cannot convert multibyte character to wide character. */
      if (mblength == (size_t)-1 || mblength == (size_t)-2)
	return (size_t)-1;

      *src += mblength;
      n -= mblength;

      /* The multibyte string  has  been  completely  converted,
	 including  the terminating '\0'. */
      if (*dest == L'\0')
	{
	  *src = NULL;
	  break;
	}
    }

    return wclength;
}

#if HAVE_MBSNRTOWCS
/* Convert a multibyte string to a wide character string. Memory for the
   new wide character string is obtained with new.

   Fast multiple-character version of xdupmbstowcs used when the indices are
   not required and mbsnrtowcs is available. */

static size_t
xdupmbstowcs2 (wchar_t **destp, const char *src)
{
  const char *p;		/* Conversion start position of src */
  std::vector<wchar_t> wsbuf;	/* Buffer for wide characters. */
  size_t wcnum = 0;		/* Number of wide characters in WSBUF */
  std::mbstate_t state;		/* Conversion State */
  size_t n, wcslength;		/* Number of wide characters produced by the conversion. */
  const char *end_or_backslash;
  size_t nms;			/* Number of multibyte characters to convert at one time. */
  std::mbstate_t tmp_state;
  const char *tmp_p;

  std::memset (&state, 0, sizeof(std::mbstate_t));

  p = src;
  do
    {
      end_or_backslash = strchrnul(p, '\\');
      nms = end_or_backslash - p;
      if (*end_or_backslash == '\0')
	nms++;

      /* Compute the number of produced wide-characters. */
      tmp_p = p;
      tmp_state = state;

      if (nms == 0 && *p == '\\')	/* special initial case */
	nms = wcslength = 1;
      else
	wcslength = ::mbsnrtowcs (NULL, &tmp_p, nms, 0, &tmp_state);

      if (wcslength == 0)
	{
	  tmp_p = p;		/* will need below */
	  tmp_state = state;
	  wcslength = 1;	/* take a single byte */
	}

      /* Conversion failed. */
      if (wcslength == (size_t)-1)
	{
	  *destp = NULL;
	  return (size_t)-1;
	}

      /* Resize the buffer if it is not large enough. */
      if (wsbuf.size() < (wcnum + wcslength + 1))       /* 1 for the L'\0' or the potential L'\\' */
        {
	  size_t new_size = wsbuf.size();

	  while (new_size < wcnum + wcslength + 1)	/* 1 for the L'\0' or the potential L'\\' */
	    new_size += WSBUF_INC;

	  wsbuf.resize (new_size);
	}

      /* Perform the conversion. This is assumed to return 'wcslength'.
	 It may set 'p' to NULL. */
      n = ::mbsnrtowcs(wsbuf.data() + wcnum, &p, nms, wsbuf.size() - wcnum, &state);

      if (n == 0 && p == 0)
	{
	  wsbuf[wcnum] = L'\0';
	  break;
	}

      /* Compensate for taking single byte on wcs conversion failure above. */
      if (wcslength == 1 && (n == 0 || n == (size_t)-1))
	{
	  state = tmp_state;
	  p = tmp_p;
	  wsbuf[wcnum] = *p;
	  if (*p == 0)
	    break;
	  else
	    {
	      wcnum++; p++;
	    }
	}
      else
        wcnum += wcslength;

      if (std::mbsinit (&state) && (p != NULL) && (*p == '\\'))
	{
	  wsbuf[wcnum++] = L'\\';
	  p++;
	}
    }
  while (p != NULL);

  *destp = new wchar_t[wcnum + 1];
  std::memcpy (*destp, wsbuf.data(), sizeof(wchar_t) * (wcnum + 1));

  /* Return the length of the wide character string, not including L'\0'. */
  return wcnum;
}
#endif /* HAVE_MBSNRTOWCS */

/* Convert a multibyte string to a wide character string. Memory for the
   new wide character string is obtained with new.

   The return value is the length of the wide character string. Returns a
   pointer to the wide character string in DESTP. If INDICESP is not NULL,
   INDICESP stores the pointer to the pointer array. Each pointer is to
   the first byte of each multibyte character. Memory for the pointer array
   is obtained with new, too.
   If conversion is failed, the return value is (size_t)-1 and the values
   of DESTP and INDICESP are NULL. */

size_t
xdupmbstowcs (wchar_t **destp, char ***indicesp, const char *src)
{
  /* In case SRC or DESP is NULL, conversion doesn't take place. */
  if (src == NULL || destp == NULL)
    {
      if (destp)
	*destp = NULL;
      if (indicesp)
	*indicesp = NULL;
      return (size_t)-1;
    }

#if HAVE_MBSNRTOWCS
  if (indicesp == NULL)
    return xdupmbstowcs2 (destp, src);
#endif

  const char *p = src;		/* Conversion start position of src */
  wchar_t wc;			/* Created wide character by conversion */
  std::vector<wchar_t> wsbuf;	/* Buffer for wide characters. */
  std::vector<char*> indices;	/* Buffer for indices. */
  size_t wcnum = 0;		/* Number of wide characters in WSBUF */
  std::mbstate_t state;		/* Conversion State */

  std::memset (&state, 0, sizeof(mbstate_t));

  do
    {
      size_t mblength;	/* Byte length of one multibyte character. */

      if (std::mbsinit (&state))
	{
	  if (*p == '\0')
	    {
	      wc = L'\0';
	      mblength = 1;
	    }
	  else if (*p == '\\')
	    {
	      wc = L'\\';
	      mblength = 1;
	    }
	  else
	    mblength = std::mbrtowc(&wc, p, MB_LEN_MAX, &state);
	}
      else
	mblength = std::mbrtowc(&wc, p, MB_LEN_MAX, &state);

      /* Conversion failed. */
      if (MB_INVALIDCH (mblength))
	{
	  *destp = NULL;
	  if (indicesp)
	    *indicesp = NULL;
	  return (size_t)-1;
	}

      ++wcnum;

      /* Resize buffers when they are not large enough. */
      if (wsbuf.size() < wcnum)
	{
	  wchar_t *wstmp;
	  char **idxtmp;

	  size_t new_size = wsbuf.size() + WSBUF_INC;
	  wsbuf.resize (new_size);

	  if (indicesp)
	    indices.resize (new_size);
	}

      wsbuf[wcnum - 1] = wc;

      if (indicesp)
        indices[wcnum - 1] = (char *)p;

      p += mblength;
    }
  while (MB_NULLWCH (wc) == 0);

  /* Return the length of the wide character string, not including `\0'. */
  *destp = new wchar_t[wcnum];
  std::memcpy (*destp, wsbuf.data(), sizeof(wchar_t) * wcnum);

  if (indicesp != NULL)
    {
      *indicesp = new char*[wcnum];
      std::memcpy (*indicesp, indices.data(), sizeof(char*) * wcnum);
    }

  return wcnum - 1;
}

/* Convert wide character string to multibyte character string. Treat invalid
   wide characters as bytes.  Used only in unusual circumstances.

   Written by Bruno Haible <bruno@clisp.org>, 2008, adapted by Chet Ramey
   for use in Bash. */

/* Convert wide character string *SRCP to a multibyte character string and
   store the result in DEST. Store at most LEN bytes in DEST. */
size_t
xwcsrtombs (char *dest, const wchar_t **srcp, size_t len, std::mbstate_t *ps)
{
  const wchar_t *src;
  size_t cur_max;			/* XXX - locale_cur_max */
  char buf[64], *destptr, *tmp_dest;
  unsigned char uc;
  std::mbstate_t prev_state;

  cur_max = MB_CUR_MAX;
  if (cur_max > sizeof (buf))		/* Holy cow. */
    return (size_t)-1;

  src = *srcp;

  if (dest != NULL)
    {
      destptr = dest;

      for (; len > 0; src++)
	{
	  wchar_t wc;
	  size_t ret;

	  wc = *src;
	  /* If we have room, store directly into DEST. */
	  tmp_dest = destptr;
	  ret = std::wcrtomb (len >= cur_max ? destptr : buf, wc, ps);

	  if (ret == (size_t)(-1))		/* XXX */
	    {
	      /* Since this is used for globbing and other uses of filenames,
		 treat invalid wide character sequences as bytes.  This is
		 intended to be symmetric with xdupmbstowcs2. */
handle_byte:
	      destptr = tmp_dest;	/* in case wcrtomb modified it */
	      uc = wc;
	      ret = 1;
	      if (len >= cur_max)
		*destptr = uc;
	      else
		buf[0] = uc;
	      if (ps)
		std::memset (ps, 0, sizeof (mbstate_t));
	    }

	  if (ret > cur_max)		/* Holy cow */
	    goto bad_input;

	  if (len < ret)
	    break;

	  if (len < cur_max)
	    std::memcpy (destptr, buf, ret);

	  if (wc == 0)
	    {
	      src = NULL;
	      /* Here mbsinit (ps).  */
	      break;
	    }
	  destptr += ret;
	  len -= ret;
	}
      *srcp = src;
      return destptr - dest;
    }
  else
    {
      /* Ignore dest and len, don't store *srcp at the end, and
	 don't clobber *ps.  */
      std::mbstate_t state = *ps;
      size_t totalcount = 0;

      for (;; src++)
	{
	  wchar_t wc;
	  size_t ret;

	  wc = *src;
	  ret = std::wcrtomb (buf, wc, &state);

	  if (ret == (size_t)(-1))
	    goto bad_input2;
	  if (wc == 0)
	    {
	      /* Here mbsinit (&state).  */
	      break;
	    }
	  totalcount += ret;
	}
      return totalcount;
    }

bad_input:
  *srcp = src;
bad_input2:
  errno = EILSEQ;
  return (size_t)(-1);
}

#endif /* HANDLE_MULTIBYTE */
