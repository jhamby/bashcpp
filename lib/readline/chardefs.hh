/* chardefs.h -- Character definitions for readline. */

/* Copyright (C) 1994-2015 Free Software Foundation, Inc.

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

#ifndef _CHARDEFS_H_
#define _CHARDEFS_H_

namespace readline
{

static inline bool
whitespace (char c)
{
  return c == ' ' || c == '\t';
}

static inline bool
member (char c, const char *s)
{
  return c ? (strchr (s, c) != nullptr) : false;
}

/* Some character stuff. */
#define control_character_threshold 0x020 /* Smaller than this is control. */
#define control_character_mask 0x1f       /* 0x20 - 1 */
#define meta_character_threshold 0x07f    /* Larger than this is Meta. */
#define control_character_bit 0x40        /* 0x000000, must be off. */
#define meta_character_bit 0x080          /* x0000000, must be on. */
#define largest_char 255                  /* Largest character value. */

#define CTRL_CHAR(c) ((c) < control_character_threshold && (((c)&0x80) == 0))
#define META_CHAR(c) ((c) > meta_character_threshold && (c) <= largest_char)

// Linux <sys/ttydefaults.h> defines this.
#ifdef CTRL
#undef CTRL
#endif

#define CTRL(c) ((c)&control_character_mask)
#define META(c) ((c) | meta_character_bit)

#define UNMETA(c) (static_cast<char> ((c) & (~meta_character_bit)))

#define UNCTRL(c)                                                             \
  (static_cast<char> (readline::_rl_to_upper (((c) | control_character_bit))))

#define IN_CTYPE_DOMAIN(c) 1

#if defined(CTYPE_NON_ASCII)
#define NON_NEGATIVE(c) 1
#else
#define NON_NEGATIVE(c) (static_cast<unsigned char> (c) == (c))
#endif

/* Some systems define these; we want our definitions. */
#undef ISPRINT

/* Beware:  these only work with single-byte ASCII characters (or EBCDIC). */

static inline bool
_rl_lowercase_p (int c)
{
  return NON_NEGATIVE (c) && c_islower (c);
}

static inline bool
_rl_uppercase_p (int c)
{
  return NON_NEGATIVE (c) && c_isupper (c);
}

static inline bool
_rl_digit_p (int c)
{
  return c >= '0' && c <= '9';
}

static inline bool
_rl_pure_alphabetic (int c)
{
  return NON_NEGATIVE (c) && c_isalpha (c);
}

static inline bool
ALPHABETIC (int c)
{
  return NON_NEGATIVE (c) && c_isalnum (c);
}

static inline int
_rl_to_upper (int c)
{
  return _rl_lowercase_p (c) ? c_toupper (c) : c;
}

static inline int
_rl_to_lower (int c)
{
  return _rl_uppercase_p (c) ? c_tolower (c) : c;
}

static inline int
_rl_digit_value (int c)
{
  return c - '0';
}

static inline bool
_rl_isident (int c)
{
  return c_isalnum (c) || (c) == '_';
}

static inline bool
ISOCTAL (int c)
{
  return c >= '0' && c <= '7';
}

static inline int
OCTVALUE (int c)
{
  return c - '0';
}

static inline bool
ISXDIGIT (int c)
{
  return c_isxdigit (c);
}

static inline int
HEXVALUE (int c)
{
  return (c >= 'a' && c <= 'f')   ? (c - 'a' + 10)
         : (c >= 'A' && c <= 'F') ? (c - 'A' + 10)
                                  : (c - '0');
}

/* Return 0 if C is not a member of the class of characters that belong
   in words, or 1 if it is.  Moved from util.c. */

const bool _rl_allow_pathname_alphabetic_chars = false;
const char *const pathname_alphabetic_chars = "/-_=~.#$";

static inline bool
rl_alphabetic (char c)
{
  if (ALPHABETIC (c))
    return true;

  return (_rl_allow_pathname_alphabetic_chars
          && strchr (pathname_alphabetic_chars, c) != nullptr);
}

#if defined(HANDLE_MULTIBYTE)
static inline bool
_rl_walphabetic (wint_t wc)
{
  if (iswalnum (wc))
    return true;

  return (_rl_allow_pathname_alphabetic_chars
          && strchr (pathname_alphabetic_chars, (wc & 0177)) != nullptr);
}
#endif

// Note: making these chars unsigned eliminates warnings when using them as
// array indices.

// Newline character.
const unsigned char NEWLINE = '\n';

// Return character (^M).
const unsigned char RETURN = CTRL ('M');

// Delete character.
const unsigned char RUBOUT = 0x7f;

// Tab character.
const unsigned char TAB = '\t';

// Abort character (^G).
const unsigned char ABORT_CHAR = CTRL ('G');

// Form-feed character (^L).
const unsigned char PAGE = CTRL ('L');

// Space character.
const unsigned char SPACE = ' '; /* XXX - was 0x20 */

// Escape character.
const unsigned char ESC = CTRL ('[');

} // namespace readline

#endif /* _CHARDEFS_H_ */
