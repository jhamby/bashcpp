/* rlmbutil.h -- utility functions for multibyte characters. */

/* Copyright (C) 2001-2015 Free Software Foundation, Inc.

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

#if !defined (_RL_MBUTIL_H_)
#define _RL_MBUTIL_H_

/************************************************/
/* check multibyte capability for I18N code     */
/************************************************/

/* For platforms which support the ISO C amendment 1 functionality we
   support user defined character classes.  */
#define HANDLE_MULTIBYTE      1

/* Make sure MB_LEN_MAX is at least 16 on systems that claim to be able to
   handle multibyte chars (some systems define MB_LEN_MAX as 1) */
#ifdef HANDLE_MULTIBYTE
#  include <climits>
#  if defined(MB_LEN_MAX) && (MB_LEN_MAX < 16)
#    undef MB_LEN_MAX
#  endif
#  if !defined (MB_LEN_MAX)
#    define MB_LEN_MAX 16
#  endif
#endif

/************************************************/
/* end of multibyte capability checks for I18N  */
/************************************************/

namespace readline
{

enum find_mbchar_flags {
  MB_FIND_ANY =		0,		// find any multibyte character
  MB_FIND_NONZERO =	0x01		// find a non-zero-width multibyte character
};

#ifdef HANDLE_MULTIBYTE

int _rl_get_char_len (const char *, mbstate_t *);
int _rl_adjust_point (const char *, int, mbstate_t *);

int _rl_read_mbchar (char *, int);
int _rl_read_mbstring (int, char *, int);

wchar_t _rl_char_value (char *, int);
int _rl_walphabetic (wchar_t);

static inline wint_t
_rl_to_wupper (wint_t wc) {
  return std::iswlower (wc) ? std::towupper (wc) : wc;
}

static inline wint_t
_rl_to_wlower (wint_t wc) {
  return std::iswupper (wc) ? std::towlower (wc) : wc;
}

static inline bool
_rl_is_mbchar_matched (const char *string, size_t seed, size_t end,
		       const char *mbchar, size_t length)
{
  if ((end - seed) < length)
    return false;

  for (size_t i = 0; i < length; i++)
    if (string[seed + i] != mbchar[i])
      return false;
  return true;
}

#define MB_NEXTCHAR(b,s,c,f) \
	((MB_CUR_MAX > 1 && !rl_byte_oriented) \
		? _rl_find_next_mbchar ((b), (s), (c), (f)) \
		: ((s) + (c)))
#define MB_PREVCHAR(b,s,f) \
	((MB_CUR_MAX > 1 && !rl_byte_oriented) \
		? _rl_find_prev_mbchar ((b), (s), (f)) \
		: ((s) - 1))

static inline bool
MB_INVALIDCH (int size) {
  return size == -1 || size == -2;
}

static inline bool
MB_INVALIDCH (size_t size) {
  return size == static_cast<size_t> (-1) || size == static_cast<size_t> (-2);
}

#if !defined (MB_NULLWCH)
#define MB_NULLWCH(x)		((x) == 0)
#endif

/* Try and shortcut the printable ascii characters to cut down the number of
   calls to a libc wcwidth() */
static inline int
_rl_wcwidth (wchar_t wc)
{
  switch (wc)
    {
    case ' ': case '!': case '"': case '#': case '%':
    case '&': case '\'': case '(': case ')': case '*':
    case '+': case ',': case '-': case '.': case '/':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
    case ':': case ';': case '<': case '=': case '>':
    case '?':
    case 'A': case 'B': case 'C': case 'D': case 'E':
    case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O':
    case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y':
    case 'Z':
    case '[': case '\\': case ']': case '^': case '_':
    case 'a': case 'b': case 'c': case 'd': case 'e':
    case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o':
    case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y':
    case 'z': case '{': case '|': case '}': case '~':
      return 1;
    default:
      return ::wcwidth (wc);
    }
}

/* Unicode combining characters range from U+0300 to U+036F */
#define UNICODE_COMBINING_CHAR(x) ((x) >= 768 && (x) <= 879)

#if defined (WCWIDTH_BROKEN)
#  define WCWIDTH(wc)	((_rl_utf8locale && UNICODE_COMBINING_CHAR(wc)) ? 0 : _rl_wcwidth(wc))
#else
#  define WCWIDTH(wc)	_rl_wcwidth(wc)
#endif

#if defined (WCWIDTH_BROKEN)
#  define IS_COMBINING_CHAR(x)	(WCWIDTH(x) == 0 && iswcntrl(x) == 0)
#else
#  define IS_COMBINING_CHAR(x)	(WCWIDTH(x) == 0)
#endif

#define UTF8_SINGLEBYTE(c)	(((c) & 0x80) == 0)
#define UTF8_MBFIRSTCHAR(c)	(((c) & 0xc0) == 0xc0)
#define UTF8_MBCHAR(c)		(((c) & 0xc0) == 0x80)

#else /* !HANDLE_MULTIBYTE */

#undef MB_LEN_MAX
#undef MB_CUR_MAX

#define MB_LEN_MAX	1
#define MB_CUR_MAX	1

#define _rl_find_prev_mbchar(b, i, f)		(((i) == 0) ? (i) : ((i) - 1))
#define _rl_find_next_mbchar(b, i1, i2, f)	((i1) + (i2))

#define _rl_char_value(buf,ind)	((buf)[(ind)])

#define _rl_walphabetic(c)	(rl_alphabetic (c))

#define _rl_to_wupper(c)	(_rl_to_upper (c))
#define _rl_to_wlower(c)	(_rl_to_lower (c))

#define MB_NEXTCHAR(b,s,c,f)	((s) + (c))
#define MB_PREVCHAR(b,s,f)	((s) - 1)

#define MB_INVALIDCH(x)		(0)
#define MB_NULLWCH(x)		(0)

#define UTF8_SINGLEBYTE(c)	(1)

#if !defined (HAVE_WCHAR_T) && !defined (wchar_t)
#  define wchar_t int
#endif

#endif /* !HANDLE_MULTIBYTE */

}  // namespace readline

#endif /* _RL_MBUTIL_H_ */
