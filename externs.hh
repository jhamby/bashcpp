/* externs.hh -- extern function declarations which do not appear in their
   own header file. */

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

/* Make sure that this is included *after* config.h! */

#if !defined(_EXTERNS_H_)
#define _EXTERNS_H_

#include "bashtypes.hh"
#include "general.hh"
#include "posixstat.hh"

#include <unistd.h>

namespace bash
{

struct STRING_INT_ALIST;

/* Declarations for functions defined in stringlib.c */

int find_string_in_alist (char *, STRING_INT_ALIST *, int);
int find_index_in_alist (char *, STRING_INT_ALIST *, int);

// Cons a new string from STRING starting at START and ending at END,
// not including END.
//
// This should be replaced with std::string's substr (pos, count).
static inline char *
substring (const char *string, size_t start, size_t end)
{
  size_t len;
  char *result;

  len = end - start;
  result = new char[len + 1];
  memcpy (result, string + start, len);
  result[len] = '\0';
  return result;
}

char *strsub (const char *, const char *, const char *, bool);
char *strcreplace (const char *, int, const char *, bool);
void strip_leading (char *);
void strip_trailing (char *, int, bool);

/* Functions from error.cc */

void itrace (const char *format, ...)
    __attribute__ ((__format__ (printf, 1, 2)));

/* Functions from version.cc. */
std::string shell_version_string ();
void show_shell_version (int);

// functions from builtins/common.hh

/* Functions from the bash library, lib/sh/libsh.a.  These should really
   go into a separate include file. */

/* Enum type for the flags argument to sh_modcase. */
enum sh_modcase_flags
{
  CASE_NOOP = 0,
  CASE_LOWER = 0x0001,
  CASE_UPPER = 0x0002,
  CASE_CAPITALIZE = 0x0004,
  CASE_UNCAP = 0x0008,
  CASE_TOGGLE = 0x0010,
  CASE_TOGGLEALL = 0x0020,
  CASE_UPFIRST = 0x0040,
  CASE_LOWFIRST = 0x0080,

  CASE_USEWORDS = 0x1000
};

static inline sh_modcase_flags &
operator&= (sh_modcase_flags &a, const sh_modcase_flags &b)
{
  a = static_cast<sh_modcase_flags> (static_cast<uint32_t> (a)
                                     & static_cast<uint32_t> (b));
  return a;
}

static inline sh_modcase_flags
operator~(const sh_modcase_flags &a)
{
  return static_cast<sh_modcase_flags> (~static_cast<uint32_t> (a));
}

/* declarations for functions defined in lib/sh/casemod.c */

char *sh_modcase (const char *, const char *, sh_modcase_flags);

/* declarations for functions defined in lib/sh/clktck.c */

long get_clk_tck ();

/* declarations for functions defined in lib/sh/clock.c */

void clock_t_to_secs (clock_t, time_t *, int *);
void print_clock_t (FILE *, clock_t);

/* from lib/sh/eaccess.c */

int sh_stat (const char *, struct stat *);

/* Enum for functions defined in lib/sh/fmtulong.c */
enum fmt_flags
{
  FL_NOFLAGS = 0,
  FL_PREFIX = 0x01,   /* add 0x, 0X, or 0 prefix as appropriate */
  FL_ADDBASE = 0x02,  /* add base# prefix to converted value */
  FL_HEXUPPER = 0x04, /* use uppercase when converting to hex */
  FL_UNSIGNED = 0x08  /* don't add any sign */
};

std::string fmtulong (unsigned long, int, fmt_flags);

/* Declarations for functions defined in lib/sh/fmtulong.c */

std::string fmtullong (unsigned long long, int, fmt_flags);

/* Declarations for functions defined in lib/sh/fmtumax.c */

std::string fmtumax (uint64_t, int, fmt_flags);

/* Declarations for functions defined in lib/sh/fnxform.c */

#if defined(__APPLE__)
char *fnx_fromfs (char *, size_t);
char *fnx_tofs (char *, size_t);
#else
static inline char *
fnx_fromfs (char *fname, size_t)
{
  return fname;
}

static inline char *
fnx_tofs (char *fname, size_t)
{
  return fname;
}
#endif

/* Declarations for functions defined in lib/sh/input_avail.c */

int input_avail (int);

/* Inline declarations of functions previously defined in lib/sh/itos.c */

static inline std::string
inttostr (int64_t i)
{
  return fmtumax (static_cast<uint64_t> (i), 10, FL_NOFLAGS);
}

// Integer to C++ string conversion.
static inline std::string
itos (int64_t i)
{
  return fmtumax (static_cast<uint64_t> (i), 10, FL_NOFLAGS);
}

// Integer to C++ string conversion.
static inline std::string
mitos (int64_t i)
{
  return fmtumax (static_cast<uint64_t> (i), 10, FL_NOFLAGS);
}

// Unsigned integer to C++ string conversion.
static inline std::string
uinttostr (uint64_t i)
{
  return fmtumax (i, 10, FL_UNSIGNED);
}

// Integer to C++ string conversion.
static inline std::string
uitos (uint64_t i)
{
  return fmtumax (i, 10, FL_UNSIGNED);
}

/* declarations for functions defined in lib/sh/mbscasecmp.c */

char *mbscasecmp (const char *, const char *);

/* declarations for functions defined in lib/sh/mbschr.c */

char *mbschr (const char *, int);

/* declarations for functions defined in lib/sh/netconn.c */

bool isnetconn (int);

/* declarations for functions defined in lib/sh/netopen.c */

int netopen (const char *);

/* Declarations for  functions defined in lib/sh/oslib.c */

int getmaxgroups ();
long getmaxchild ();

/* declarations for functions defined in lib/sh/random.c */

int brand ();
void sbrand (unsigned long); /* set bash random number generator. */
void seedrand ();            /* seed generator randomly */
void seedrand32 ();
uint32_t get_urandom32 ();

/* declarations for functions defined in lib/sh/setlinebuf.c */

int sh_setlinebuf (FILE *);

/* declarations for functions defined in lib/sh/shaccess.c */

int sh_eaccess (const char *, int);

/* declarations for functions defined in lib/sh/shmatch.c */

int sh_regmatch (const char *, const char *, int);

/* defines for flags argument to sh_regmatch. */
enum sh_match_flags
{
  SHMAT_NOFLAGS = 0,
  SHMAT_SUBEXP = 0x001, /* save subexpressions in SH_REMATCH */
  SHMAT_PWARN = 0x002   /* print a warning message on invalid regexp */
};

/* declarations for functions defined in lib/sh/shmbchar.c */

size_t mbstrlen (const char *);

/* declarations for functions defined in lib/sh/shquote.c */

std::string sh_single_quote (string_view);
std::string sh_double_quote (string_view);
std::string sh_mkdoublequoted (string_view, int, int);
std::string sh_un_double_quote (string_view);
std::string sh_backslash_quote (string_view, string_view, int);
std::string sh_backslash_quote_for_double_quotes (string_view);
std::string sh_quote_reusable (string_view, int);
bool sh_contains_shell_metas (string_view);

static inline bool
sh_contains_quotes (string_view string)
{
  string_view::iterator it;
  for (it = string.begin (); it != string.end (); ++it)
    {
      if (*it == '\'' || *it == '"' || *it == '\\')
        return true;
    }
  return false;
}

/* declarations for functions defined in lib/sh/spell.c */

int spname (const char *, char *);
char *dirspell (const char *);

/* declarations for functions and structures defined in lib/sh/stringlist.c */

// String lists and vectors are now the same thing.
typedef std::vector<char *> STRINGLIST;

#if 0
static inline STRINGLIST *
strlist_create (size_t n)
{
  return new STRINGLIST (n);
}

// Note: call resize () directly, with or without size check.
static inline STRINGLIST *
strlist_resize (STRINGLIST *sl, size_t n)
{
  if (n > sl->size ())
    sl->resize (n);

  return sl;
}
#endif

int strlist_remove (STRINGLIST *, char *);
STRINGLIST *strlist_copy (const STRINGLIST *);
STRINGLIST *strlist_merge (const STRINGLIST *, const STRINGLIST *);
void strlist_append (STRINGLIST *, const STRINGLIST *);
void strlist_prefix_suffix (STRINGLIST *, const char *, const char *);

static inline void
strlist_print (const STRINGLIST *sl, const char *prefix)
{
  if (sl == nullptr)
    return;

  STRINGLIST::const_iterator it;
  for (it = sl->begin (); it != sl->end (); ++it)
    printf ("%s%s\n", prefix ? prefix : "", *it);
}

// Custom comparison function for sorting string pointer vectors.
struct string_ptr_comp
{
  bool
  operator() (const std::string *s1, const std::string *s2)
  {
#if defined(HAVE_STRCOLL)
    return strcoll (s1->c_str (), s2->c_str ()) < 0;
#else
    return strcmp (s1->c_str (), s2->c_str ()) < 0;
#endif
  }
};

/* Comparison routine for use by qsort that conforms to the new Posix
   requirements (http://austingroupbugs.net/view.php?id=1070).

   Perform a bytewise comparison if *S1 and *S2 collate equally. */
struct strlist_posixcmp
{
  bool
  operator() (const char *s1, const char *s2)
  {
    int result;

#if defined(HAVE_STRCOLL)
    result = strcoll (s1, s2);
    if (result != 0)
      return (result < 0);
#endif

    return strcmp (s1, s2) < 0;
  }
};

/* Comparison routine for use with qsort() on arrays of strings.  Uses
   strcoll(3) if available, otherwise it uses strcmp(3). */
struct strlist_strcmp
{
  bool
  operator() (const char *s1, const char *s2)
  {
#if defined(HAVE_STRCOLL)
    return strcoll (s1, s2) < 0;
#else  /* !HAVE_STRCOLL */
    return strcmp (s1, s2) < 0;
#endif /* !HAVE_STRCOLL */
  }
};

/* Sort ARRAY, a null terminated array of pointers to strings. */
static inline void
strlist_sort (STRINGLIST *array, bool posix)
{
  if (posix)
    {
      strlist_posixcmp posixcmp;
      std::sort (array->begin (), array->end (), posixcmp);
    }
  else
    {
      strlist_strcmp strcollcmp;
      std::sort (array->begin (), array->end (), strcollcmp);
    }
}

/* declarations for functions defined in lib/sh/stringvec.c */

// There is now a single STRINGLIST typedef for both sets of functions.

/* Free the contents of sl, a C++ vector of char *. */
static inline void
strlist_flush (STRINGLIST *sl)
{
  if (sl == nullptr)
    return;

  STRINGLIST::iterator it;
  for (it = sl->begin (); it != sl->end (); ++it)
    delete[](*it);
}

// Free the contents of sl, a C++ vector of char *, then the array itself.
static inline void
strlist_dispose (STRINGLIST *sl)
{
  if (sl == nullptr)
    return;

  strlist_flush (sl);
  delete sl;
}

// Search for and remove the first occurrence of the specified name. Returns
// true if an item was found; false otherwise.
static inline bool
strlist_remove (STRINGLIST *sl, string_view name)
{
  if (sl == nullptr)
    return false;

  STRINGLIST::iterator it;
  for (it = sl->begin (); it != sl->end (); ++it)
    if (name == *it)
      {
        delete[](*it);
        return true;
      }
  return false;
}

// Find NAME in STRINGLIST.  Return the index of NAME, or -1 if not present.
static inline int
strlist_search (const STRINGLIST *array, string_view name)
{
  STRINGLIST::const_iterator it;

  for (it = array->begin (); it != array->end (); ++it)
    if (name == *it)
      return static_cast<int> (it - array->begin ());

  return -1;
}

class WORD_LIST;

STRINGLIST *strlist_from_word_list (const WORD_LIST *, size_t);
WORD_LIST *strlist_to_word_list (const STRINGLIST *, size_t);

/* declarations for functions defined in lib/sh/strtrans.cc */

std::string ansic_quote (string_view);
bool ansic_shouldquote (string_view);
std::string ansiexpand (string_view, size_t, size_t);

/* declarations for functions defined in lib/sh/strvec.cc */

static inline void
strvec_dispose (char **array)
{
  if (array == nullptr)
    return;

  for (int i = 0; array[i]; i++)
    delete[] array[i];

  delete[] array;
}

/* declarations for functions defined in lib/sh/timeval.cc */

void timeval_to_secs (struct timeval *tvp, time_t *sp, int *sfp);
void print_timeval (FILE *fp, struct timeval *tvp);

/* declarations for functions defined in lib/sh/tmpfile.cc */

enum mktmp_flags
{
  MT_NOFLAGS = 0,
  MT_USETMPDIR = 0x0001,
  MT_READWRITE = 0x0002,
  MT_USERANDOM = 0x0004,
  MT_TEMPLATE = 0x0008
};

char *sh_mktmpname (char *, int);
int sh_mktmpfd (string_view, int, char **);
/* extern FILE *sh_mktmpfp (string_view, int, char **); */
char *sh_mktmpdir (char *, int);

/* declarations for functions defined in lib/sh/uconvert.c */

int uconvert (char *, long *, long *, char **);

/* declarations for functions defined in lib/sh/ufuncs.c */

unsigned int falarm (unsigned int, unsigned int);
unsigned int fsleep (unsigned int, unsigned int);

/* declarations for functions defined in lib/sh/unicode.c */

int u32cconv (unsigned long, char *);
void u32reset ();

/* declarations for functions defined in lib/sh/wcsnwidth.c */

#if defined(HANDLE_MULTIBYTE)
ssize_t wcsnwidth (const wchar_t *, size_t, size_t);
#endif

/* declarations for functions defined in lib/sh/zgetline.c */

ssize_t zgetline (int, char **, size_t *, int, bool);

/* declarations for functions defined in lib/sh/zmapfd.c */

int zmapfd (int, char **, string_view);

/* declarations for functions defined in lib/glob/gmisc.c */

bool match_pattern_char (string_view, string_view, int);
int umatchlen (string_view, size_t);

#if defined(HANDLE_MULTIBYTE)
bool match_pattern_wchar (const wchar_t *, const wchar_t *, int);
int wmatchlen (const wchar_t *, size_t);
#endif

// This value was previously stored in lib/sh/zread.c.
#ifndef ZBUFSIZ
#define ZBUFSIZ 4096
#endif

// Inline functions moved from general.cc.

/* Return non-zero if the characters from SAMPLE are not all valid
   characters to be found in the first line of a shell script.  We
   check up to the first newline, or SAMPLE_LEN, whichever comes first.
   All of the characters must be printable or whitespace. */
static inline bool
check_binary_file (string_view sample, size_t sample_len)
{
  unsigned char c;

  for (size_t i = 0; i < sample_len; i++)
    {
      c = static_cast<unsigned char> (sample[i]);
      if (c == '\n')
        return false;
      if (c == '\0')
        return true;
    }

  return false;
}

/* **************************************************************** */
/*								    */
/*		    Functions to manipulate pipes		    */
/*								    */
/* **************************************************************** */

static inline int
sh_openpipe (int *pv)
{
  int r;

  if ((r = pipe (pv)) < 0)
    return r;

  pv[0] = move_to_high_fd (pv[0], 1, 64);
  pv[1] = move_to_high_fd (pv[1], 1, 64);

  return 0;
}

static inline int
sh_closepipe (int *pv)
{
  if (pv[0] >= 0)
    close (pv[0]);

  if (pv[1] >= 0)
    close (pv[1]);

  pv[0] = pv[1] = -1;
  return 0;
}

/* Just a wrapper for the define in include/filecntl.h */
static inline int
sh_setclexec (int fd)
{
  return SET_CLOSE_ON_EXEC (fd);
}

/* Return true if file descriptor FD is valid; false otherwise. */
static inline bool
sh_validfd (int fd)
{
  return fcntl (fd, F_GETFD, 0) >= 0;
}

static inline bool
fd_ispipe (int fd)
{
  errno = 0;
  return (lseek (fd, 0L, SEEK_CUR) < 0) && (errno == ESPIPE);
}

#ifdef _POSIXSTAT_H_
bool same_file (const char *, const char *, struct stat *, struct stat *);
#endif

/* **************************************************************** */
/*								    */
/*		    Functions to inspect pathnames		    */
/*								    */
/* **************************************************************** */

static inline bool
file_exists (const char *fn)
{
  struct stat sb;

  return stat (fn, &sb) == 0;
}

static inline bool
file_isdir (const char *fn)
{
  struct stat sb;

  return (stat (fn, &sb) == 0) && S_ISDIR (sb.st_mode);
}

static inline bool
file_iswdir (const char *fn)
{
  return file_isdir (fn) && sh_eaccess (fn, W_OK) == 0;
}

/* Return 1 if STRING is "." or "..", optionally followed by a directory
   separator */
static inline bool
path_dot_or_dotdot (string_view string)
{
  if (string.empty () || string[0] != '.')
    return false;

  /* string[0] == '.' */
  if (PATHSEP (string[1]) || (string[1] == '.' && PATHSEP (string[2])))
    return true;

  return false;
}

/* Return true if STRING contains an absolute pathname.  Used by `cd'
   to decide whether or not to look up a directory name in $CDPATH. */
static inline bool
absolute_pathname (string_view string)
{
  if (string.empty ())
    return false;

  if (ABSPATH (string))
    return true;

  if (string[0] == '.' && PATHSEP (string[1])) /* . and ./ */
    return true;

  if (string[0] == '.' && string[1] == '.'
      && PATHSEP (string[2])) /* .. and ../ */
    return true;

  return false;
}

/* Return the `basename' of the pathname in STRING (the stuff after the
   last '/').  If STRING is `/', just return it. */
static inline const char *
base_pathname (const char *string) noexcept
{
  if (string[0] == '/' && string[1] == '\0')
    return string;

  const char *p = strrchr (string, '/');

  return p ? ++p : string;
}

char *make_absolute (string_view, string_view);
char *full_pathname (char *);

/* Return a printable representation of FN without special characters.  The
   caller is responsible for freeing memory if this returns something other
   than its argument.  If FLAGS is non-zero, we are printing for portable
   re-input and should single-quote filenames appropriately. */
static inline std::string
printable_filename (string_view fn, int flags)
{
  if (ansic_shouldquote (fn))
    return ansic_quote (fn);
  else if (flags && sh_contains_shell_metas (fn))
    return sh_single_quote (fn);
  else
    return to_string (fn);
}

char *extract_colon_unit (const char *, size_t *);

void tilde_initialize ();
char *bash_tilde_find_word (const char *s, int, size_t *);
char *bash_tilde_expand (const char *s, int);

#if !defined(HAVE_GROUP_MEMBER)
int group_member (gid_t);
#endif

std::string conf_standard_path ();

} // namespace bash

#endif /* _EXTERNS_H_ */
