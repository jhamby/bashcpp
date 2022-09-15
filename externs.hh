/* externs.h -- extern function declarations which do not appear in their
   own header file. */

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

/* Make sure that this is included *after* config.h! */

#if !defined(_EXTERNS_H_)
#define _EXTERNS_H_

#include <cstring>
#include <ctime>

#include <string>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#include "general.hh"

namespace bash
{

/* Miscellaneous functions from parse.yy */
#if 0
int yyparse ();
int return_EOF ();
void push_token (int);
char *xparse_dolparen (const char *, char *, int *, int);
void reset_parser ();
void reset_readahead_token ();
WordList *parse_string_to_word_list (char *, int, const char *);
#endif

/* Declarations for functions needed by the parser. */
COMMAND *connect_async_list (COMMAND *, COMMAND *, int);

struct StringIntAlist;

/* Declarations for functions defined in stringlib.c */
int find_string_in_alist (char *, StringIntAlist *, int);
char *find_token_in_alist (int, StringIntAlist *, int);
int find_index_in_alist (char *, StringIntAlist *, int);

/* Cons a new string from STRING starting at START and ending at END,
   not including END. */
static inline char *
substring (const char *string, size_t start, size_t end)
{
  size_t len;
  char *result;

  len = end - start;
  result = new char[len + 1];
  std::memcpy (result, string + start, len);
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

/* Functions from version.c. */
char *shell_version_string ();
void show_shell_version (int);

// functions from builtins/common.h

/* Keeps track of the current working directory. */
char *get_working_directory (const char *);
void set_working_directory (const char *);

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

char *fmtulong (unsigned long, int, fmt_flags);

/* Declarations for functions defined in lib/sh/fmtulong.c */
char *fmtullong (unsigned long long, int, fmt_flags);

/* Declarations for functions defined in lib/sh/fmtumax.c */
char *fmtumax (uint64_t, int, fmt_flags);

/* Declarations for functions defined in lib/sh/fnxform.c */

#if defined(MACOSX)
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

static inline char *
inttostr (int64_t i)
{
  return fmtumax (static_cast<uint64_t> (i), 10, FL_NOFLAGS);
}

/* Integer to string conversion. */
static inline char *
itos (int64_t i)
{
  return fmtumax (static_cast<uint64_t> (i), 10, FL_NOFLAGS);
}

/* Integer to string conversion.  This conses the string using savestring;
   caller should delete it and be prepared to catch alloc exceptions. */
static inline char *
mitos (int64_t i)
{
  return fmtumax (static_cast<uint64_t> (i), 10, FL_NOFLAGS);
}

static inline char *
uinttostr (uint64_t i)
{
  return fmtumax (i, 10, FL_UNSIGNED);
}

/* Integer to string conversion.  This conses the string; the
   caller should delete it. */
static inline char *
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

/* declarations for functions defined in lib/sh/pathcanon.c */
enum path_flags
{
  PATH_NOFLAGS = 0,
  PATH_CHECKDOTDOT = 0x0001,
  PATH_CHECKEXISTS = 0x0002,
  PATH_HARDPATH = 0x0004,
  PATH_NOALLOC = 0x0008
};

char *sh_canonpath (const char *, path_flags);

/* declarations for functions defined in lib/sh/pathphys.c */
char *sh_physpath (const char *, int);
char *sh_realpath (const char *, char *);

/* declarations for functions defined in lib/sh/random.c */
int brand ();
void sbrand (unsigned long); /* set bash random number generator. */
void seedrand ();            /* seed generator randomly */
void seedrand32 ();
u_bits32_t get_urandom32 ();

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
char *sh_single_quote (const char *);
char *sh_double_quote (const char *);
char *sh_mkdoublequoted (const char *, int, int);
char *sh_un_double_quote (const char *);
char *sh_backslash_quote (const char *, const char *, int);
char *sh_backslash_quote_for_double_quotes (const char *);
char *sh_quote_reusable (const char *, int);
bool sh_contains_shell_metas (const char *);
bool sh_contains_quotes (const char *);

/* declarations for functions defined in lib/sh/spell.c */
int spname (const char *, char *);
char *dirspell (const char *);

/* declarations for functions and structures defined in lib/sh/stringlist.c */

#if 0
typedef std::vector<std::string> STRINGLIST;

// typedef int sh_strlist_map_func_t (char *);

static inline STRINGLIST *
strlist_create (int n)
{
  if (n)
    return new STRINGLIST(n);
  else
    return new STRINGLIST;
}

static inline STRINGLIST *
strlist_resize (STRINGLIST *sl, int n)
{
  if (n > sl->size())
    sl->resize(n);

  return sl;
}

void strlist_flush (STRINGLIST *);
void strlist_dispose (STRINGLIST *);
int strlist_remove (STRINGLIST *, char *);
STRINGLIST *strlist_copy (STRINGLIST *);
STRINGLIST *strlist_merge (STRINGLIST *, STRINGLIST *);
STRINGLIST *strlist_append (STRINGLIST *, STRINGLIST *);
STRINGLIST *strlist_prefix_suffix (STRINGLIST *, char *, char *);
void strlist_print (STRINGLIST *, char *);
void strlist_walk (STRINGLIST *, sh_strlist_map_func_t *);
void strlist_sort (STRINGLIST *);
#endif

/* declarations for functions defined in lib/sh/stringvec.c */

#if 0
// TODO: refactor to use vector<char*>
char **strvec_create (int);
char **strvec_resize (char **, int);
char **strvec_mcreate (int);
char **strvec_mresize (char **, int);
void strvec_flush (char **);
void strvec_dispose (char **);
int strvec_remove (char **, char *);
size_t strvec_len (char **);
int strvec_search (char **, char *);
char **strvec_copy (char **);
int strvec_posixcmp (char **, char **);
int strvec_strcmp (char **, char **);
void strvec_sort (char **, int);

char **strvec_from_word_list (WORD_LIST *, int, int, int *);
WORD_LIST *strvec_to_word_list (char **, int, int);
#endif

/* declarations for functions defined in lib/sh/strtrans.c */
char *ansic_quote (const char *, unsigned int *);
bool ansic_shouldquote (const char *);
char *ansiexpand (const char *, unsigned int, unsigned int, unsigned int *);

/* declarations for functions defined in lib/sh/timeval.c */
void timeval_to_secs (struct timeval *tvp, time_t *sp, int *sfp);
void print_timeval (FILE *fp, struct timeval *tvp);

/* declarations for functions defined in lib/sh/tmpfile.c */
enum mktmp_flags
{
  MT_NOFLAGS = 0,
  MT_USETMPDIR = 0x0001,
  MT_READWRITE = 0x0002,
  MT_USERANDOM = 0x0004,
  MT_TEMPLATE = 0x0008
};

char *sh_mktmpname (char *, int);
int sh_mktmpfd (const char *, int, char **);
/* extern FILE *sh_mktmpfp (const char *, int, char **); */
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

/* declarations for functions defined in lib/sh/winsize.c */
void get_new_window_size (int, int *, int *);

/* declarations for functions defined in lib/sh/zgetline.c */
ssize_t zgetline (int, char **, size_t *, int, bool);

/* declarations for functions defined in lib/sh/zmapfd.c */
int zmapfd (int, char **, const char *);

/* declarations for functions defined in lib/glob/gmisc.c */
bool match_pattern_char (const char *, const char *, int);
int umatchlen (const char *, size_t);

#if defined(HANDLE_MULTIBYTE)
bool match_pattern_wchar (const wchar_t *, const wchar_t *, int);
int wmatchlen (const wchar_t *, size_t);
#endif

/* Declarations for functions defined in lib/sh/dprintf.c */
#if !defined(HAVE_DPRINTF)
void dprintf (int, const char *, ...)
    __attribute__ ((__format__ (printf, 2, 3)));
#endif

/* Declarations for functions defined in lib/sh/fpurge.c */

#if defined(NEED_FPURGE_DECL)
#if !defined(HAVE_DECL_FPURGE)

#if defined(HAVE_FPURGE)
#define fpurge _bash_fpurge
#endif
int fpurge (FILE *stream);

#endif /* HAVE_DECL_FPURGE */
#endif /* NEED_FPURGE_DECL */

/* Declarations for functions defined in lib/sh/getcwd.c */
#if !defined(HAVE_GETCWD)
char *getcwd (char *, size_t);
#endif

/* Declarations for  functions defined in lib/sh/oslib.c */

#if !defined(HAVE_DUP2) || defined(DUP2_BROKEN)
int dup2 (int, int);
#endif

#if !defined(HAVE_GETDTABLESIZE)
int getdtablesize ();
#endif /* !HAVE_GETDTABLESIZE */

#if !defined(HAVE_GETHOSTNAME)
int gethostname (char *, int);
#endif /* !HAVE_GETHOSTNAME */

/* declarations for functions defined in lib/sh/strcasecmp.c */
#if !defined(HAVE_STRCASECMP)
int strncasecmp (const char *, const char *, size_t);
int strcasecmp (const char *, const char *);
#endif /* HAVE_STRCASECMP */

/* declarations for functions defined in lib/sh/strcasestr.c */
#if !defined(HAVE_STRCASESTR)
char *strcasestr (const char *, const char *);
#endif

/* declarations for functions defined in lib/sh/strchrnul.c */
#if !defined(HAVE_STRCHRNUL)
char *strchrnul (const char *, int);
#endif

/* declarations for functions defined in lib/sh/strerror.c */
#if !defined(HAVE_STRERROR) && !defined(strerror)
char *strerror (int);
#endif

/* declarations for functions defined in lib/sh/strftime.c */
#if !defined(HAVE_STRFTIME) && defined(NEED_STRFTIME_DECL)
size_t strftime (char *, size_t, const char *, const struct tm *);
#endif

/* global namespace declarations for functions defined in lib/sh/strnlen.c */
#if !defined(HAVE_STRNLEN)
size_t strnlen (const char *, size_t);
#endif

/* declarations for functions defined in lib/sh/strpbrk.c */
#if !defined(HAVE_STRPBRK)
char *strpbrk (const char *, const char *);
#endif

/* declarations for functions defined in lib/sh/strimax.c */
#if !defined(HAVE_DECL_STRTOIMAX)
int64_t strtoimax (const char *, char **, int);
#endif

/* declarations for functions defined in lib/sh/strumax.c */
#if !defined(HAVE_DECL_STRTOUMAX)
uint64_t strtoumax (const char *, char **, int);
#endif

// This value was previously stored in lib/sh/zread.c.
#ifndef ZBUFSIZ
#define ZBUFSIZ 4096
#endif

} // namespace bash

#endif /* _EXTERNS_H_ */
