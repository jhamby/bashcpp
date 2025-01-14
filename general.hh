/* general.hh -- defines that everybody likes to use. */

/* Copyright (C) 1993-2021 Free Software Foundation, Inc.

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

#include "bashintl.hh"
#include "chartypes.hh"

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
  ERREXIT = 4,     // Exit due to error condition.
  EXITBLTIN = 5    // Exit due to the exit builtin.
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
public:
  virtual const char *what () const noexcept override;
};

// Exception used by builtins/wait_def.cc

class wait_interrupt : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

// Exception used for returning from evaluation loops

class return_exception : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

/* Local exceptions used by expansion functions in subst.c. */

class subst_expand_error : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

class subst_expand_fatal : public subst_expand_error
{
public:
  virtual const char *what () const noexcept override;
};

class extract_string_error : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

class extract_string_fatal : public extract_string_error
{
public:
  virtual const char *what () const noexcept override;
};

class matched_pair_error : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

class parse_error : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

#if defined(ALIAS)
class read_again_exception : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};
#endif

class invalid_nameref_value : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

class nameref_maxloop_value : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

class eval_exception : public std::exception
{
public:
  virtual const char *what () const noexcept override;
};

class return_catch_exception : public std::exception
{
public:
  return_catch_exception (int value) : return_catch_value (value) {}
  virtual const char *what () const noexcept override;

  int return_catch_value;
};

class test_exit_exception : public std::exception
{
public:
  test_exit_exception (int value) : test_error_return (value) {}
  virtual const char *what () const noexcept override;

  int test_error_return;
};

/* Global inline functions, previously C preprocessor macros. */

// Create a new copy of null-terminated string s. Free with delete[].
static inline char *
savestring (const char *s)
{
  return strcpy (new char[1 + strlen (s)], s);
}

// Create a new copy of C++ string s. Free with delete[].
static inline char *
savestring (const std::string &s)
{
  return strcpy (new char[1 + s.size ()], s.c_str ());
}

// Create a new copy of C++ string_view s. Free with delete[].
static inline char *
savestring (string_view s)
{
  char *result = strcpy (new char[1 + s.size ()], s.data ());
  result[s.size ()] = '\0';
  return result;
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
#define legal_variable_starter(c) (c_isalpha (c) || (c == '_'))
#define legal_variable_char(c) (c_isalnum (c) || c == '_')

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
  explicit GENERIC_LIST (GENERIC_LIST *next) : next_ (next) {}

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

  size_t
  size () const
  {
    size_t size = 1;
    GENERIC_LIST *list = next_;

    while (list)
      {
        size++;
        list = list->next_;
      }

    return size;
  }
};

// Compare two strings for equality.
static inline bool
STREQ (const char *a, const char *b)
{
  return (strcmp (a, b) == 0);
}

// Compare two strings for equality, up to n characters.
static inline bool
STREQN (const char *a, const char *b, size_t n)
{
  return (strncmp (a, b, n) == 0);
}

static inline bool
whitespace (char c)
{
  return (c == ' ') || (c == '\t');
}

static inline bool
member (char c, const char *s)
{
  return c ? (strchr (s, c) != nullptr) : false;
}

/* UTF-8 functions, inlined from lib/sh/utf8.c. */

#if defined(HANDLE_MULTIBYTE)

static inline const char *
utf8_mbschr (const char *s, int c)
{
  return strchr (s, c); /* for now */
}

static inline char *
utf8_mbschr (char *s, int c)
{
  return strchr (s, c); /* for now */
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
utf8_mblen (string_view s, size_t n)
{
  unsigned char c, c1, c2, c3;

  if (s.empty ())
    return 0; /* no shift states */
  if (n <= 0)
    return static_cast<size_t> (-1);

  c = static_cast<unsigned char> (s[0]);
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

#define NOW (time (nullptr))
#define GETTIME(tv) (gettimeofday (&(tv), nullptr))

/* Some defines for calling file status functions. */
enum file_stat_flags
{
  FS_NONE = 0,
  FS_EXISTS = 0x01,
  FS_EXECABLE = 0x02,
  FS_EXEC_PREFERRED = 0x04,
  FS_EXEC_ONLY = 0x08,
  FS_DIRECTORY = 0x10,
  FS_NODIRS = 0x20,
  FS_READABLE = 0x40
};

static inline file_stat_flags
operator| (const file_stat_flags &a, const file_stat_flags &b)
{
  return static_cast<file_stat_flags> (static_cast<uint32_t> (a)
                                       | static_cast<uint32_t> (b));
}

/* Default maximum for move_to_high_fd */
constexpr int HIGH_FD_MAX = 256;

/* Some useful definitions for Unix pathnames.  Argument convention:
   x == string, c == character */

#if !defined(__CYGWIN__)
#define ABSPATH(x) ((x)[0] == '/')
#define RELPATH(x) ((x)[0] != '/')
#else /* __CYGWIN__ */
#define ABSPATH(x)                                                            \
  (((x)[0] && c_isalpha (((x)[0])) && (x)[1] == ':') || ISDIRSEP ((x)[0]))
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

// Return true if this token is a legal shell `identifier', i.e. it consists
// solely of letters, digits, and underscores, and does not begin with a digit.
static inline bool
legal_identifier (string_view name)
{
  if (name.empty () || (legal_variable_starter (name[0]) == 0))
    return false;

  string_view::const_iterator it;
  for (it = name.begin () + 1; it != name.end (); ++it)
    {
      if (!legal_variable_char (*it))
        return false;
    }
  return true;
}

static inline bool
line_isblank (string_view line)
{
  string_view::const_iterator it;
  for (it = line.begin (); it != line.end (); ++it)
    {
      if (!c_isblank (*it))
        return false;
    }

  return true;
}

/* Return non-zero if all of the characters in STRING are digits. */
static inline bool
all_digits (string_view string)
{
  string_view::const_iterator it;
  for (it = string.begin (); it != string.end (); ++it)
    if (!c_isdigit (*it))
      return false;

  return true;
}

bool legal_number (const char *, int64_t *);
bool check_identifier (WORD_DESC *, int);
bool valid_nameref_value (string_view, int);
bool check_selfref (string_view, string_view, int);
bool legal_alias_name (string_view, int);
int assignment (string_view, int);

int sh_unset_nodelay_mode (int);
int move_to_high_fd (int, int, int);

} // namespace bash

#endif /* _GENERAL_H_ */
