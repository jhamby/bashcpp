/* strmatch.c -- ksh-like extended pattern matching for the shell and filename
		globbing. */

/* Copyright (C) 1991-2020 Free Software Foundation, Inc.

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

#include <config.h>

#include <cstdio>	/* for debugging */
#include <cstdlib>

#include "strmatch.h"
#include <chartypes.h>

#include "shmbutil.h"

#include <cerrno>

#if FNMATCH_EQUIV_FALLBACK
#include <fnmatch.h>
#endif

/* First, compile `sm_loop.c' for single-byte characters. */
#define CHAR	unsigned char
#define U_CHAR	unsigned char
#define XCHAR	char
#define INT	int
#define L(CS)	CS
#define INVALID	-1

#ifndef GLOBASCII_DEFAULT
#  define GLOBASCII_DEFAULT 0
#endif

// FIXME: map this to ShellOptions.
bool glob_asciirange = GLOBASCII_DEFAULT;

#if FNMATCH_EQUIV_FALLBACK
/* Construct a string w1 = "c1" and a pattern w2 = "[[=c2=]]" and pass them
   to fnmatch to see if wide characters c1 and c2 collate as members of the
   same equivalence class. We can't really do this portably any other way */
static int
_fnmatch_fallback (int s, int p)
{
  char s1[2];			/* string */
  char s2[8];			/* constructed pattern */

  s1[0] = (unsigned char)s;
  s1[1] = '\0';

  /* reconstruct the pattern */
  s2[0] = s2[1] = '[';
  s2[2] = '=';
  s2[3] = (unsigned char)p;
  s2[4] = '=';
  s2[5] = s2[6] = ']';
  s2[7] = '\0';

  return fnmatch ((const char *)s2, (const char *)s1, 0);
}
#endif

/* We use strcoll(3) for range comparisons in bracket expressions,
   even though it can have unwanted side effects in locales
   other than POSIX or US.  For instance, in the de locale, [A-Z] matches
   all characters.  If GLOB_ASCIIRANGE is non-zero, and we're not forcing
   the use of strcoll (e.g., for explicit collating symbols), we use
   straight ordering as if in the C locale. */

#if defined (HAVE_STRCOLL)
/* Helper functions for collating symbol equivalence. */

/* Return 0 if C1 == C2 or collates equally if FORCECOLL is non-zero. */
static int
charcmp (int c1, int c2, bool forcecoll)
{
  static char s1[2] = { ' ', '\0' };
  static char s2[2] = { ' ', '\0' };
  int ret;

  /* Eight bits only.  Period. */
  c1 &= 0xFF;
  c2 &= 0xFF;

  if (c1 == c2)
    return 0;

  if (forcecoll == 0 && glob_asciirange)
    return c1 - c2;

  s1[0] = c1;
  s2[0] = c2;

  return std::strcoll (s1, s2);
}

static int
rangecmp (int c1, int c2, bool forcecoll)
{
  int r;

  r = charcmp (c1, c2, forcecoll);

  /* We impose a total ordering here by returning c1-c2 if charcmp returns 0 */
  if (r != 0)
    return r;
  return c1 - c2;		/* impose total ordering */
}
#else /* !HAVE_STRCOLL */
#  define rangecmp(c1, c2, f)	((int)(c1) - (int)(c2))
#endif /* !HAVE_STRCOLL */

#if defined (HAVE_STRCOLL)
/* Returns 1 if chars C and EQUIV collate equally in the current locale. */
static int
collequiv (int c, int equiv)
{
  if (charcmp (c, equiv, 1) == 0)
    return 1;

#if FNMATCH_EQUIV_FALLBACK
  return _fnmatch_fallback (c, equiv) == 0;
#else
  return 0;
#endif

}
#else
#  define collequiv(c, equiv)	((c) == (equiv))
#endif

#define __COLLSYM	__collsym
#define POSIXCOLL	posix_collsyms
#include "collsyms.h"

static int
collsym (const CHAR *s, size_t len)
{
  __collsym *csp;
  char *x;

  x = (char *)s;
  for (csp = posix_collsyms; csp->name; csp++)
    {
      if (STREQN (csp->name, x, len) && csp->name[len] == '\0')
	return csp->code;
    }
  if (len == 1)
    return s[0];
  return INVALID;
}

enum char_class
  {
    CC_NO_CLASS = 0,
    CC_ASCII, CC_ALNUM, CC_ALPHA, CC_BLANK, CC_CNTRL, CC_DIGIT, CC_GRAPH,
    CC_LOWER, CC_PRINT, CC_PUNCT, CC_SPACE, CC_UPPER, CC_WORD, CC_XDIGIT
  };

static char const *const cclass_name[] =
  {
    "",
    "ascii", "alnum", "alpha", "blank", "cntrl", "digit", "graph",
    "lower", "print", "punct", "space", "upper", "word", "xdigit"
  };

#define N_CHAR_CLASS (sizeof(cclass_name) / sizeof (cclass_name[0]))

static char_class
is_valid_cclass (const char *name)
{
  char_class ret = CC_NO_CLASS;

  for (int i = 1; i < N_CHAR_CLASS; i++)
    {
      if (STREQ (name, cclass_name[i]))
	{
	  ret = (char_class)i;
	  break;
	}
    }

  return ret;
}

static int
cclass_test (int c, char_class char_class)
{
  int result;

  switch (char_class)
    {
      case CC_ASCII:
	result = ::isascii (c);		// not in C++03
	break;
      case CC_ALNUM:
	result = std::isalnum (c);
	break;
      case CC_ALPHA:
	result = std::isalpha (c);
	break;
      case CC_BLANK:
	result = bash::isblank (c);	// in C99 and C++11
	break;
      case CC_CNTRL:
	result = std::iscntrl (c);
	break;
      case CC_DIGIT:
	result = std::isdigit (c);
	break;
      case CC_GRAPH:
	result = std::isgraph (c);
	break;
      case CC_LOWER:
	result = std::islower (c);
	break;
      case CC_PRINT:
	result = std::isprint (c);
	break;
      case CC_PUNCT:
	result = std::ispunct (c);
	break;
      case CC_SPACE:
	result = std::isspace (c);
	break;
      case CC_UPPER:
	result = std::isupper (c);
	break;
      case CC_WORD:
        result = (std::isalnum (c) || c == '_');
	break;
      case CC_XDIGIT:
	result = std::isxdigit (c);
	break;
      default:
	result = -1;
	break;
    }

  return result;
}

static int
is_cclass (int c, const char *name)
{
  char_class char_class;
  int result;

  char_class = is_valid_cclass (name);
  if (char_class == CC_NO_CLASS)
    return -1;

  result = cclass_test (c, char_class);
  return result;
}

/* Now include `sm_loop.c' for single-byte characters. */
/* The result of FOLD is an `unsigned char' */
# define FOLD(c) ((flags & FNM_CASEFOLD) \
	? std::tolower ((unsigned char)c) \
	: ((unsigned char)c))

#define FCT			internal_strmatch
#define GMATCH			gmatch
#define COLLSYM			collsym
#define PARSE_COLLSYM		parse_collsym
#define BRACKMATCH		brackmatch
#define PATSCAN			glob_patscan
#define STRCOMPARE		strcompare
#define EXTMATCH		extmatch
#define DEQUOTE_PATHNAME	udequote_pathname
#define STRUCT			smat_struct
#define STRCHR(S, C)		std::strchr((S), (C))
#define MEMCHR(S, C, N)		std::memchr((S), (C), (N))
#define STRCOLL(S1, S2)		std::strcoll((S1), (S2))
#define STRLEN(S)		std::strlen(S)
#define STRCMP(S1, S2)		std::strcmp((S1), (S2))
#define RANGECMP(C1, C2, F)	rangecmp((C1), (C2), (F))
#define COLLEQUIV(C1, C2)	collequiv((C1), (C2))
#define CTYPE_T			char_class
#define IS_CCLASS(C, S)		is_cclass((C), (S))
#include "sm_loop.c"

#if HANDLE_MULTIBYTE

#  define CHAR		wchar_t
#  define U_CHAR	wint_t
#  define XCHAR		wchar_t
#  define INT		wint_t
#  define L(CS)		L##CS
#  define INVALID	WEOF

#if FNMATCH_EQUIV_FALLBACK
/* Construct a string w1 = "c1" and a pattern w2 = "[[=c2=]]" and pass them
   to fnmatch to see if wide characters c1 and c2 collate as members of the
   same equivalence class. We can't really do this portably any other way */
static int
_fnmatch_fallback_wc (wchar_t c1, wchar_t c2)
{
  char w1[MB_LEN_MAX+1];		/* string */
  char w2[MB_LEN_MAX+8];		/* constructed pattern */
  int l1, l2;

  l1 = std::wctomb (w1, c1);
  if (l1 == -1)
    return 2;
  w1[l1] = '\0';

  /* reconstruct the pattern */
  w2[0] = w2[1] = '[';
  w2[2] = '=';
  l2 = std::wctomb (w2+3, c2);
  if (l2 == -1)
    return 2;
  w2[l2+3] = '=';
  w2[l2+4] = w2[l2+5] = ']';
  w2[l2+6] = '\0';

  return ::fnmatch ((const char *)w2, (const char *)w1, 0);
}
#endif

static int
charcmp_wc (wint_t c1, wint_t c2, bool forcecoll)
{
  static wchar_t s1[2] = { L' ', L'\0' };
  static wchar_t s2[2] = { L' ', L'\0' };
  int r;

  if (c1 == c2)
    return 0;

  if (!forcecoll && glob_asciirange && c1 <= UCHAR_MAX && c2 <= UCHAR_MAX)
    return (int)(c1 - c2);

  s1[0] = c1;
  s2[0] = c2;

  return std::wcscoll (s1, s2);
}

static int
rangecmp_wc (wint_t c1, wint_t c2, bool forcecoll)
{
  int r;

  r = charcmp_wc (c1, c2, forcecoll);

  /* We impose a total ordering here by returning c1-c2 if charcmp returns 0,
     as we do above in the single-byte case. */
  if (r != 0 || forcecoll)
    return r;
  return (int)(c1 - c2);		/* impose total ordering */
}

/* Returns true if wide chars C and EQUIV collate equally in the current locale. */
static bool
collequiv_wc (wint_t c, wint_t equiv)
{
  wchar_t s, p;

  if (charcmp_wc (c, equiv, 1) == 0)
    return true;

#if FNMATCH_EQUIV_FALLBACK
/* We check explicitly for success (fnmatch returns 0) to avoid problems if
   our local definition of FNM_NOMATCH (strmatch.h) doesn't match the
   system's (fnmatch.h). We don't care about error return values here. */

  s = c;
  p = equiv;
  return _fnmatch_fallback_wc (s, p) == 0;
#else
  return false;
#endif
}

/* Helper function for collating symbol. */
#  define __COLLSYM	__collwcsym
#  define POSIXCOLL	posix_collwcsyms
#  include "collsyms.h"

static wint_t
collwcsym (const wchar_t *s, int len)
{
  __collwcsym *csp;

  for (csp = posix_collwcsyms; csp->name; csp++)
    {
      if (STREQN (csp->name, s, len) && csp->name[len] == L'\0')
	return csp->code;
    }
  if (len == 1)
    return s[0];
  return INVALID;
}

static int
is_wcclass (wint_t wc, const wchar_t *name)
{
  std::mbstate_t state;
  size_t mbslength;
  wctype_t desc;

  if ((std::wctype ("ascii") == (wctype_t)0) && (std::wcscmp (name, L"ascii") == 0))
    {
      int c = std::wctob (wc);

      if (c == EOF)
	return 0;
      else
        return c <= 0x7F;
    }

  bool want_word = (std::wcscmp (name, L"word") == 0);
  if (want_word)
    name = L"alnum";

  std::memset (&state, 0, sizeof (std::mbstate_t));

  // alloc temp buf on stack
  char mbs[std::wcslen(name) * MB_CUR_MAX + 1];

  mbslength = std::wcsrtombs (mbs, (const wchar_t **)&name, (std::wcslen(name) * MB_CUR_MAX + 1), &state);

  if (mbslength == (size_t)-1 || mbslength == (size_t)-2)
    {
      return -1;
    }
  desc = std::wctype (mbs);

  if (desc == (wctype_t)0)
    return -1;

  if (want_word)
    return std::iswctype (wc, desc) || wc == L'_';
  else
    return std::iswctype (wc, desc);
}

/* Return true if there are no char class [:class:] expressions (degenerate case)
   or only posix-specified (C locale supported) char class expressions in
   PATTERN.  These are the ones where it's safe to punt to the single-byte
   code, since wide character support allows locale-defined char classes.
   This only uses single-byte code, but is only needed to support multibyte
   locales. */
static bool
posix_cclass_only (const char *pattern)
{
  char cc[16];		/* sufficient for all valid posix char class names */
  char_class valid;

  const char *p = pattern;
  while ((p = std::strchr (p, '[')))
    {
      if (p[1] != ':')
	{
	  p++;
	  continue;
        }
      p += 2;		/* skip past "[:" */

      /* Find end of char class expression */
      const char *p1;
      for (p1 = p; *p1;  p1++)
	if (*p1 == ':' && p1[1] == ']')
	  break;
      if (*p1 == 0)	/* no char class expression found */
	break;

      /* Find char class name and validate it against posix char classes */
      if ((p1 - p) >= sizeof (cc))
	return false;

      std::memcpy (cc, p, p1 - p);
      cc[p1 - p] = '\0';

      valid = is_valid_cclass (cc);
      if (valid == CC_NO_CLASS)
	return false;		/* found unrecognized char class name */

      p = p1 + 2;		/* found posix char class name */
    }

  return true;			/* no char class names or only posix */
}

/* Now include `sm_loop.c' for multibyte characters. */
#define FOLD(c) ((flags & FNM_CASEFOLD) && std::iswupper (c) ? std::towlower (c) : (c))
#define FCT			internal_wstrmatch
#define GMATCH			gmatch_wc
#define COLLSYM			collwcsym
#define PARSE_COLLSYM		parse_collwcsym
#define BRACKMATCH		brackmatch_wc
#define PATSCAN			glob_patscan_wc
#define STRCOMPARE		wscompare
#define EXTMATCH		extmatch_wc
#define DEQUOTE_PATHNAME	wcdequote_pathname
#define STRUCT			wcsmat_struct
#define STRCHR(S, C)		std::wcschr((S), (C))
#define MEMCHR(S, C, N)		std::wmemchr((S), (C), (N))
#define STRCOLL(S1, S2)		std::wcscoll((S1), (S2))
#define STRLEN(S)		std::wcslen(S)
#define STRCMP(S1, S2)		std::wcscmp((S1), (S2))
#define RANGECMP(C1, C2, F)	rangecmp_wc((C1), (C2), (F))
#define COLLEQUIV(C1, C2)	collequiv_wc((C1), (C2))
#define CTYPE_T			char_class
#define IS_CCLASS(C, S)		is_wcclass((C), (S))
#include "sm_loop.c"

#endif /* HAVE_MULTIBYTE */

int
xstrmatch (const char *pattern, const char *string, int flags)
{
#if HANDLE_MULTIBYTE
  int ret;
  size_t n;
  wchar_t *wpattern, *wstring;
  size_t plen, slen, mplen, mslen;

  if (MB_CUR_MAX == 1)
    return internal_strmatch ((unsigned char *)pattern, (unsigned char *)string, flags);

  if (mbsmbchar (string) == 0 && mbsmbchar (pattern) == 0 && posix_cclass_only (pattern))
    return internal_strmatch ((unsigned char *)pattern, (unsigned char *)string, flags);

  n = xdupmbstowcs (&wpattern, NULL, pattern);
  if (n == (size_t)-1 || n == (size_t)-2)
    return internal_strmatch ((unsigned char *)pattern, (unsigned char *)string, flags);

  n = xdupmbstowcs (&wstring, NULL, string);
  if (n == (size_t)-1 || n == (size_t)-2)
    {
      delete[] wpattern;
      return internal_strmatch ((unsigned char *)pattern, (unsigned char *)string, flags);
    }

  ret = internal_wstrmatch (wpattern, wstring, flags);

  delete[] wpattern;
  delete[] wstring;

  return ret;
#else
  return internal_strmatch ((unsigned char *)pattern, (unsigned char *)string, flags);
#endif /* !HANDLE_MULTIBYTE */
}
