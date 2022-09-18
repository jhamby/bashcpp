/* general.h -- defines that everybody likes to use. */

/* Copyright (C) 1993-2020 Free Software Foundation, Inc.

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

#if !defined(_GENERAL_H_)
#define _GENERAL_H_

#include "bashtypes.hh"
#include "chartypes.hh"
#include "filecntl.hh"

#if defined(HAVE_SYS_RESOURCE_H) && defined(RLIMTYPE)
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#include <sys/resource.h>
#endif

#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <exception>
#include <string>

namespace bash
{

/* Exceptions to replace setjmp and longjmp calls. */

enum bash_exception_t
{
  NOEXCEPTION = 0, // Report an error other than an exception.
  FORCE_EOF = 1,   // We want to stop parsing.
  DISCARD = 2,     // Discard current command.
  EXITPROG = 3,    // Unconditionally exit the program now.
  ERREXIT = 4      // Exit due to error condition.
};

/* General shell exception, including an exception type enum. */
class bash_exception : public std::exception
{
public:
  bash_exception (bash_exception_t t) : type (t) {}
  virtual const char *what () const noexcept override;

  bash_exception_t type;
};

// Exception to jump to the top level to execute a command in a subshell.
class subshell_child_start : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

// Exception used by builtins/read_def.cc

class sigalarm_interrupt : public std::exception
{
  virtual const char *what () const noexcept override;
};

// Exception used by builtins/wait_def.cc

class wait_interrupt : public std::exception
{
  virtual const char *what () const noexcept override;
};

// Exception used for returning from evaluation loops

class return_exception : public std::exception
{
  virtual const char *what () const noexcept override;
};

/* Local exceptions used by expansion functions in subst.c. */

class subst_expand_error : public std::exception
{
  virtual const char *what () const noexcept override;
};

class subst_expand_fatal : public subst_expand_error
{
  virtual const char *what () const noexcept override;
};

/* Global inline functions, previously C preprocessor macros. */

// Create a new copy of null-terminated string s. Free with delete[].
static inline char *
savestring (const char *s)
{
  return std::strcpy (new char[1 + std::strlen (s)], s);
}

// Create a new copy of C++ string s. Free with delete[].
static inline char *
savestring (const std::string &s)
{
  return std::strcpy (new char[1 + s.size ()], s.c_str ());
}

/* Nonzero if the integer type T is signed.  */
#define TYPE_SIGNED(t) (!(static_cast<t> (0) < static_cast<t> (-1)))

/* The width in bits of the integer type or expression T.
   Padding bits are not supported; this is checked at compile-time below.  */
#define TYPE_WIDTH(t) (sizeof (t) * CHAR_BIT)

/* Bound on length of the string representing an unsigned integer
   value representable in B bits.  log10 (2.0) < 146/485.  The
      smallest value of B where this bound is not tight is 2621.  */
#define INT_BITS_STRLEN_BOUND(b) (((b)*146 + 484) / 485)

/* Bound on length of the string representing an integer value of type T.
   Subtract one for the sign bit if T is signed;
   302 / 1000 is log10 (2) rounded up;
   add one for integer division truncation;
   add one more for a minus sign if t is signed.  */
#define INT_STRLEN_BOUND(t)                                                   \
  ((TYPE_WIDTH (t) - TYPE_SIGNED (t)) * 302 / 1000 + 1 + TYPE_SIGNED (t))

/* Bound on buffer size needed to represent an integer type or expression T,
   including the terminating null.  */
#define INT_BUFSIZE_BOUND(t) (INT_STRLEN_BOUND (t) + 1)

/* Define exactly what a legal shell identifier consists of. */
#define legal_variable_starter(c) (std::isalpha (c) || (c == '_'))
#define legal_variable_char(c) (std::isalnum (c) || c == '_')

/* Definitions used in subst.c and by the `read' builtin for field
   splitting. */
#define spctabnl(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

/* All structs which contain a `next' field should inherit from this
   class to get access to its reverse () method. */
class GENERIC_LIST
{
protected:
  // No-argument constructor.
  GENERIC_LIST () {}

  // Constructor that initializes the next element.
  GENERIC_LIST (GENERIC_LIST *next) : next_ (next) {}

  // Children call this, then cast the result to their derived type.
  GENERIC_LIST *
  reverse_ ()
  {
    GENERIC_LIST *next, *prev;
    GENERIC_LIST *list = this;

    for (prev = nullptr; list;)
      {
        next = list->next_;
        list->next_ = prev;
        prev = list;
        list = next;
      }
    return prev;
  }

  void
  append_ (GENERIC_LIST *new_item)
  {
    GENERIC_LIST *last = this;
    while (last->next_)
      last = last->next_;

    last->next_ = new_item;
  }

  // Pointer to next entry, or nullptr.
  GENERIC_LIST *next_;

public:
  void
  set_next (GENERIC_LIST *next)
  {
    next_ = next;
  }
};

// Compare two strings for equality.
static inline bool
STREQ (const char *a, const char *b)
{
  return (std::strcmp (a, b) == 0);
}

// Compare two strings for equality, up to n characters.
static inline bool
STREQN (const char *a, const char *b, size_t n)
{
  return (std::strncmp (a, b, n) == 0);
}

static inline bool
whitespace (char c)
{
  return (c == ' ') || (c == '\t');
}

static inline bool
member (char c, const char *s)
{
  return c ? (std::strchr (s, c) != nullptr) : false;
}

/* UTF-8 functions, inlined from lib/sh/utf8.c. */

#if defined(HANDLE_MULTIBYTE)

static inline const char *
utf8_mbschr (const char *s, int c)
{
  return std::strchr (s, c); /* for now */
}

static inline char *
utf8_mbschr (char *s, int c)
{
  return std::strchr (s, c); /* for now */
}

static inline const char *
utf8_mbsmbchar (const char *str)
{
  for (const char *s = str; *s; s++)
    if ((*s & 0xc0) == 0x80)
      return s;
  return nullptr;
}

static inline char *
utf8_mbsmbchar (char *str)
{
  return const_cast<char *> (utf8_mbsmbchar (const_cast<const char *> (str)));
}

/* Adapted from GNU gnulib. Handles UTF-8 characters up to 4 bytes long */
static inline size_t
utf8_mblen (const char *s, size_t n)
{
  unsigned char c, c1, c2, c3;

  if (s == nullptr)
    return 0; /* no shift states */
  if (n <= 0)
    return static_cast<size_t> (-1);

  c = static_cast<unsigned char> (*s);
  if (c < 0x80)
    return c != 0;
  if (c >= 0xc2)
    {
      c1 = static_cast<unsigned char> (s[1]);
      if (c < 0xe0)
        {
          if (n == 1)
            return static_cast<size_t> (-2);

          /*
           *				c	c1
           *
           *    U+0080..U+07FF       C2..DF   80..BF
           */

          if (n >= 2 && (c1 ^ 0x80) < 0x40) /* 0x80..0xbf */
            return 2;
        }
      else if (c < 0xf0)
        {
          if (n == 1)
            return static_cast<size_t> (-2);

          /*
           *				c	c1	c2
           *
           *    U+0800..U+0FFF       E0       A0..BF   80..BF
           *    U+1000..U+CFFF       E1..EC   80..BF   80..BF
           *    U+D000..U+D7FF       ED       80..9F   80..BF
           *    U+E000..U+FFFF       EE..EF   80..BF   80..BF
           */

          if ((c1 ^ 0x80) < 0x40 && (c >= 0xe1 || c1 >= 0xa0)
              && (c != 0xed || c1 < 0xa0))
            {
              if (n == 2)
                return static_cast<size_t> (-2); /* incomplete */

              c2 = static_cast<unsigned char> (s[2]);
              if ((c2 ^ 0x80) < 0x40)
                return 3;
            }
        }
      else if (c <= 0xf4)
        {
          if (n == 1)
            return static_cast<size_t> (-2);

          /*
           *				c	c1	c2	c3
           *
           *    U+10000..U+3FFFF     F0       90..BF   80..BF   80..BF
           *    U+40000..U+FFFFF     F1..F3   80..BF   80..BF   80..BF
           *    U+100000..U+10FFFF   F4       80..8F   80..BF   80..BF
           */
          if (((c1 ^ 0x80) < 0x40) && (c >= 0xf1 || c1 >= 0x90)
              && (c < 0xf4 || (c == 0xf4 && c1 < 0x90)))
            {
              if (n == 2)
                return static_cast<size_t> (-2); /* incomplete */

              c2 = static_cast<unsigned char> (s[2]);
              if ((c2 ^ 0x80) < 0x40)
                {
                  if (n == 3)
                    return static_cast<size_t> (-2);

                  c3 = static_cast<unsigned char> (s[3]);
                  if ((c3 ^ 0x80) < 0x40)
                    return 4;
                }
            }
        }
    }
  /* invalid or incomplete multibyte character */
  return static_cast<size_t> (-1);
}

#endif // HANDLE_MULTIBYTE

#define NOW (std::time (nullptr))
#define GETTIME(tv) (::gettimeofday (&(tv), nullptr))

/* Some defines for calling file status functions. */
enum file_stat_flags
{
  FS_EXISTS = 0x01,
  FS_EXECABLE = 0x02,
  FS_EXEC_PREFERRED = 0x04,
  FS_EXEC_ONLY = 0x08,
  FS_DIRECTORY = 0x10,
  FS_NODIRS = 0x20,
  FS_READABLE = 0x40
};

/* Default maximum for move_to_high_fd */
constexpr int HIGH_FD_MAX = 256;

/* Some useful definitions for Unix pathnames.  Argument convention:
   x == string, c == character */

#if !defined(__CYGWIN__)
#define ABSPATH(x) ((x)[0] == '/')
#define RELPATH(x) ((x)[0] != '/')
#else /* __CYGWIN__ */
#define ABSPATH(x)                                                            \
  (((x)[0] && ISALPHA ((unsigned char)(x)[0]) && (x)[1] == ':')               \
   || ISDIRSEP ((x)[0]))
#define RELPATH(x) (ABSPATH (x) == 0)
#endif /* __CYGWIN__ */

#define ROOTEDPATH(x) (ABSPATH (x))

#define DIRSEP '/'
#if !defined(__CYGWIN__)
#define ISDIRSEP(c) ((c) == '/')
#else
#define ISDIRSEP(c) ((c) == '/' || (c) == '\\')
#endif /* __CYGWIN__ */
#define PATHSEP(c) (ISDIRSEP (c) || (c) == 0)

#define DOT_OR_DOTDOT(s)                                                      \
  (s[0] == '.' && (s[1] == 0 || (s[1] == '.' && s[2] == 0)))
#if defined(HANDLE_MULTIBYTE)
#define WDOT_OR_DOTDOT(w)                                                     \
  (w[0] == L'.' && (w[1] == L'\0' || (w[1] == L'.' && w[2] == L'\0')))
#endif

/* Declarations for functions defined in general.c */
void posix_initialize (bool);

int num_posix_options ();
char *get_posix_options (char *);
void set_posix_options (const char *);

void save_posix_options ();

#if defined(RLIMTYPE)
// for use from builtins.c
RLIMTYPE string_to_rlimtype (char *);
void print_rlimtype (RLIMTYPE, int);
#endif

// forward declaration for prototype below
class WORD_DESC;

bool all_digits (const char *);
bool legal_number (const char *, int64_t *);
bool legal_identifier (const char *);
bool check_identifier (WORD_DESC *, int);
bool valid_nameref_value (const char *, int);
bool check_selfref (const char *, const char *, int);
bool legal_alias_name (const char *, int);
bool line_isblank (const char *);
int assignment (const char *, int);

int sh_unset_nodelay_mode (int);
int move_to_high_fd (int, int, int);

} // namespace bash

#endif /* _GENERAL_H_ */
