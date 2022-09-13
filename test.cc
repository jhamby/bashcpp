/* test.c - GNU test program (ksb and mjb) */

/* Modified to run with the GNU shell Apr 25, 1988 by bfox. */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

/* Define PATTERN_MATCHING to get the csh-like =~ and !~ pattern-matching
   binary operators. */
/* #define PATTERN_MATCHING */

#include "config.hh"

#include "bashtypes.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "filecntl.hh"
#include "posixstat.hh"
#include "stat-time.hh"

#include "bashintl.hh"

#include "builtins/common.hh"
#include "pathexp.hh"
#include "shell.hh"
#include "test.hh"

#include "strmatch.hh"

namespace bash
{

#define EQ 0
#define NE 1
#define LT 2
#define GT 3
#define LE 4
#define GE 5

#define NT 0
#define OT 1
#define EF 2

/* The following few defines control the truth and false output of each stage.
   TRUE and FALSE are what we use to compute the final output value.
   SHELL_BOOLEAN is the form which returns truth or falseness in shell terms.
   Default is TRUE = 1, FALSE = 0, SHELL_BOOLEAN = (!value). */
#define SHELL_BOOLEAN(value) (!(value))

#define TEST_ERREXIT_STATUS 2

// static procenv_t test_exit_buf;
static int test_error_return;
#define test_exit(val)                                                        \
  do                                                                          \
    {                                                                         \
      test_error_return = val;                                                \
      sh_longjmp (test_exit_buf, 1);                                          \
    }                                                                         \
  while (0)

extern int sh_stat (const char *, struct stat *);

static int pos;     /* The offset of the current argument in ARGV. */
static int argc;    /* The number of arguments present in ARGV. */
static char **argv; /* The argument list. */
static int noeval;

static void test_syntax_error (const char *, const char *)
    __attribute__ ((__noreturn__));
static void beyond (void) __attribute__ ((__noreturn__));
static void integer_expected_error (const char *)
    __attribute__ ((__noreturn__));

static bool unary_operator (void);
static bool binary_operator (void);
static bool two_arguments (void);
static bool three_arguments (void);
static bool posixtest (void);

static bool expr (void);
static bool term (void);
static bool and_ (void);
static bool or_ (void);

static bool filecomp (const char *, const char *, int);
static bool arithcomp (const char *, const char *, int, int);
static bool patcomp (const char *, const char *, int);

static void
test_syntax_error (const char *format, const char *arg)
{
  builtin_error (format, arg);
  test_exit (TEST_ERREXIT_STATUS);
}

/*
 * beyond - call when we're beyond the end of the argument list (an
 *	error condition)
 */
static void
beyond ()
{
  test_syntax_error (_ ("argument expected"), nullptr);
}

/* Syntax error for when an integer argument was expected, but
   something else was found. */
static void
integer_expected_error (const char *pch)
{
  test_syntax_error (_ ("%s: integer expression expected"), pch);
}

/* Increment our position in the argument list.  Check that we're not
   past the end of the argument list.  This check is suppressed if the
   argument is FALSE.  Made a macro for efficiency. */
#define advance(f)                                                            \
  do                                                                          \
    {                                                                         \
      ++pos;                                                                  \
      if (f && pos >= argc)                                                   \
        beyond ();                                                            \
    }                                                                         \
  while (0)
#define unary_advance()                                                       \
  do                                                                          \
    {                                                                         \
      advance (1);                                                            \
      ++pos;                                                                  \
    }                                                                         \
  while (0)

/*
 * expr:
 *	or
 */
static bool
expr ()
{
  if (pos >= argc)
    beyond ();

  return or_ (); /* Same with this. */
}

/*
 * or:
 *	and
 *	and '-o' or
 */
static bool
or_ ()
{
  int value, v2;

  value = and_ ();
  if (pos < argc && argv[pos][0] == '-' && argv[pos][1] == 'o'
      && !argv[pos][2])
    {
      advance (0);
      v2 = or_ ();
      return value || v2;
    }

  return value;
}

/*
 * and:
 *	term
 *	term '-a' and
 */
static bool
and_ ()
{
  int value, v2;

  value = term ();
  if (pos < argc && argv[pos][0] == '-' && argv[pos][1] == 'a'
      && !argv[pos][2])
    {
      advance (0);
      v2 = and_ ();
      return value && v2;
    }
  return value;
}

/*
 * term - parse a term and return 1 or 0 depending on whether the term
 *	evaluates to true or false, respectively.
 *
 * term ::=
 *	'-'('a'|'b'|'c'|'d'|'e'|'f'|'g'|'h'|'k'|'p'|'r'|'s'|'u'|'w'|'x')
 *filename
 *	'-'('G'|'L'|'O'|'S'|'N') filename
 * 	'-t' [int]
 *	'-'('z'|'n') string
 *	'-'('v'|'R') varname
 *	'-o' option
 *	string
 *	string ('!='|'='|'==') string
 *	<int> '-'(eq|ne|le|lt|ge|gt) <int>
 *	file '-'(nt|ot|ef) file
 *	'(' <expr> ')'
 * int ::=
 *	positive and negative integers
 */
static bool
term ()
{
  bool value;

  if (pos >= argc)
    beyond ();

  /* Deal with leading `not's. */
  if (argv[pos][0] == '!' && argv[pos][1] == '\0')
    {
      value = false;
      while (pos < argc && argv[pos][0] == '!' && argv[pos][1] == '\0')
        {
          advance (1);
          value = !value;
        }

      return value ? !term () : term ();
    }

  /* A paren-bracketed argument. */
  if (argv[pos][0] == '(' && argv[pos][1] == '\0') /* ) */
    {
      advance (1);
      value = expr ();
      if (argv[pos] == 0) /* ( */
        test_syntax_error (_ ("`)' expected"), nullptr);
      else if (argv[pos][0] != ')' || argv[pos][1]) /* ( */
        test_syntax_error (_ ("`)' expected, found %s"), argv[pos]);
      advance (0);
      return value;
    }

  /* are there enough arguments left that this could be dyadic? */
  if ((pos + 3 <= argc) && test_binop (argv[pos + 1]))
    value = binary_operator ();

  /* Might be a switch type argument -- make sure we have enough arguments for
     the unary operator and argument */
  else if ((pos + 2) <= argc && test_unop (argv[pos]))
    value = unary_operator ();

  else
    {
      value = argv[pos][0] != '\0';
      advance (0);
    }

  return value;
}

static int
stat_mtime (const char *fn, struct stat *st, struct timespec *ts)
{
  int r;

  r = sh_stat (fn, st);
  if (r < 0)
    return r;
  *ts = get_stat_mtime (st);
  return 0;
}

static bool
filecomp (const char *s, const char *t, int op)
{
  struct stat st1, st2;
  struct timespec ts1, ts2;
  int r1, r2;

  if ((r1 = stat_mtime (s, &st1, &ts1)) < 0)
    {
      if (op == EF)
        return false;
    }
  if ((r2 = stat_mtime (t, &st2, &ts2)) < 0)
    {
      if (op == EF)
        return false;
    }

  switch (op)
    {
    case OT:
      return r1 < r2 || (r2 == 0 && timespec_cmp (ts1, ts2) < 0);
    case NT:
      return r1 > r2 || (r1 == 0 && timespec_cmp (ts1, ts2) > 0);
    case EF:
      return same_file (s, t, &st1, &st2);
    }
  return false;
}

bool
Shell::arithcomp (const char *s, const char *t, int op, int flags)
{
  int64_t l, r;

  if (flags & TEST_ARITHEXP)
    {
      bool expok;
      l = evalexp (s, EXP_EXPANDED, &expok);
      if (!expok)
        test_exit (SHELL_BOOLEAN (false)); /* should probably longjmp here */
      r = evalexp (t, EXP_EXPANDED, &expok);
      if (!expok)
        test_exit (SHELL_BOOLEAN (false)); /* ditto */
    }
  else
    {
      if (legal_number (s, &l) == 0)
        integer_expected_error (s);
      if (legal_number (t, &r) == 0)
        integer_expected_error (t);
    }

  switch (op)
    {
    case EQ:
      return l == r;
    case NE:
      return l != r;
    case LT:
      return l < r;
    case GT:
      return l > r;
    case LE:
      return l <= r;
    case GE:
      return l >= r;
    }

  return false;
}

bool
Shell::patcomp (const char *string, const char *pat, int op)
{
  int m;

  m = strmatch (pat, string, (FNMATCH_EXTFLAG | FNMATCH_IGNCASE));
  return (op == EQ) ? (m == 0) : (m != 0);
}

bool
Shell::binary_test (const char *op, const char *arg1, const char *arg2,
                    int flags)
{
  int patmatch;

  patmatch = (flags & TEST_PATMATCH);

  if (op[0] == '=' && (op[1] == '\0' || (op[1] == '=' && op[2] == '\0')))
    return patmatch ? patcomp (arg1, arg2, EQ) : STREQ (arg1, arg2);
  else if ((op[0] == '>' || op[0] == '<') && op[1] == '\0')
    {
#if defined(HAVE_STRCOLL)
      if (shell_compatibility_level > 40 && flags & TEST_LOCALE)
        return (op[0] == '>') ? (strcoll (arg1, arg2) > 0)
                              : (strcoll (arg1, arg2) < 0);
      else
#endif
        return (op[0] == '>') ? (strcmp (arg1, arg2) > 0)
                              : (strcmp (arg1, arg2) < 0);
    }
  else if (op[0] == '!' && op[1] == '=' && op[2] == '\0')
    return patmatch ? patcomp (arg1, arg2, NE) : (STREQ (arg1, arg2) == 0);

  else if (op[2] == 't')
    {
      switch (op[1])
        {
        case 'n':
          return filecomp (arg1, arg2, NT); /* -nt */
        case 'o':
          return filecomp (arg1, arg2, OT); /* -ot */
        case 'l':
          return arithcomp (arg1, arg2, LT, flags); /* -lt */
        case 'g':
          return arithcomp (arg1, arg2, GT, flags); /* -gt */
        }
    }
  else if (op[1] == 'e')
    {
      switch (op[2])
        {
        case 'f':
          return filecomp (arg1, arg2, EF); /* -ef */
        case 'q':
          return arithcomp (arg1, arg2, EQ, flags); /* -eq */
        }
    }
  else if (op[2] == 'e')
    {
      switch (op[1])
        {
        case 'n':
          return arithcomp (arg1, arg2, NE, flags); /* -ne */
        case 'g':
          return arithcomp (arg1, arg2, GE, flags); /* -ge */
        case 'l':
          return arithcomp (arg1, arg2, LE, flags); /* -le */
        }
    }

  return false; /* should never get here */
}

static bool
binary_operator ()
{
  bool value;
  char *w;

  w = argv[pos + 1];
  if ((w[0] == '=' && (w[1] == '\0' || (w[1] == '=' && w[2] == '\0')))
      ||                                                /* =, == */
      ((w[0] == '>' || w[0] == '<') && w[1] == '\0') || /* <, > */
      (w[0] == '!' && w[1] == '=' && w[2] == '\0'))     /* != */
    {
      value = binary_test (w, argv[pos], argv[pos + 2], 0);
      pos += 3;
      return value;
    }

#if defined(PATTERN_MATCHING)
  if ((w[0] == '=' || w[0] == '!') && w[1] == '~' && w[2] == '\0')
    {
      value = patcomp (argv[pos], argv[pos + 2], w[0] == '=' ? EQ : NE);
      pos += 3;
      return value;
    }
#endif

  if ((w[0] != '-' || w[3] != '\0') || test_binop (w) == 0)
    {
      test_syntax_error (_ ("%s: binary operator expected"), w);
      /* NOTREACHED */
      return false;
    }

  value = binary_test (w, argv[pos], argv[pos + 2], 0);
  pos += 3;
  return value;
}

static bool
unary_operator ()
{
  char *op;
  int64_t r;

  op = argv[pos];
  if (test_unop (op) == 0)
    return false;

  /* the only tricky case is `-t', which may or may not take an argument. */
  if (op[1] == 't')
    {
      advance (0);
      if (pos < argc)
        {
          if (legal_number (argv[pos], &r))
            {
              advance (0);
              return unary_test (op, argv[pos - 1]);
            }
          else
            return false;
        }
      else
        return unary_test (op, "1");
    }

  /* All of the unary operators take an argument, so we first call
     unary_advance (), which checks to make sure that there is an
     argument, and then advances pos right past it.  This means that
     pos - 1 is the location of the argument. */
  unary_advance ();
  return unary_test (op, argv[pos - 1]);
}

bool
Shell::unary_test (const char *op, const char *arg)
{
  int64_t r;
  struct stat stat_buf;
  struct timespec mtime, atime;
  SHELL_VAR *v;

  switch (op[1])
    {
    case 'a': /* file exists in the file system? */
    case 'e':
      return sh_stat (arg, &stat_buf) == 0;

    case 'r': /* file is readable? */
      return sh_eaccess (arg, R_OK) == 0;

    case 'w': /* File is writeable? */
      return sh_eaccess (arg, W_OK) == 0;

    case 'x': /* File is executable? */
      return sh_eaccess (arg, X_OK) == 0;

    case 'O': /* File is owned by you? */
      return (sh_stat (arg, &stat_buf) == 0
              && (uid_t)current_user.euid == (uid_t)stat_buf.st_uid);

    case 'G': /* File is owned by your group? */
      return (sh_stat (arg, &stat_buf) == 0
              && (gid_t)current_user.egid == (gid_t)stat_buf.st_gid);

    case 'N':
      if (sh_stat (arg, &stat_buf) < 0)
        return false;
      atime = get_stat_atime (&stat_buf);
      mtime = get_stat_mtime (&stat_buf);
      return timespec_cmp (mtime, atime) > 0;

    case 'f': /* File is a file? */
      if (sh_stat (arg, &stat_buf) < 0)
        return false;

        /* -f is true if the given file exists and is a regular file. */
#if defined(S_IFMT)
      return S_ISREG (stat_buf.st_mode) || (stat_buf.st_mode & S_IFMT) == 0;
#else
      return S_ISREG (stat_buf.st_mode);
#endif /* !S_IFMT */

    case 'd': /* File is a directory? */
      return sh_stat (arg, &stat_buf) == 0 && (S_ISDIR (stat_buf.st_mode));

    case 's': /* File has something in it? */
      return sh_stat (arg, &stat_buf) == 0 && stat_buf.st_size > (off_t)0;

    case 'S': /* File is a socket? */
#if !defined(S_ISSOCK)
      return false;
#else
      return sh_stat (arg, &stat_buf) == 0 && S_ISSOCK (stat_buf.st_mode);
#endif /* S_ISSOCK */

    case 'c': /* File is character special? */
      return sh_stat (arg, &stat_buf) == 0 && S_ISCHR (stat_buf.st_mode);

    case 'b': /* File is block special? */
      return sh_stat (arg, &stat_buf) == 0 && S_ISBLK (stat_buf.st_mode);

    case 'p': /* File is a named pipe? */
#ifndef S_ISFIFO
      return false;
#else
      return sh_stat (arg, &stat_buf) == 0 && S_ISFIFO (stat_buf.st_mode);
#endif /* S_ISFIFO */

    case 'L': /* Same as -h  */
    case 'h': /* File is a symbolic link? */
#if !defined(S_ISLNK) || !defined(HAVE_LSTAT)
      return false;
#else
      return ((arg[0] != '\0') && (lstat (arg, &stat_buf) == 0)
              && S_ISLNK (stat_buf.st_mode));
#endif /* S_IFLNK && HAVE_LSTAT */

    case 'u': /* File is setuid? */
      return sh_stat (arg, &stat_buf) == 0
             && (stat_buf.st_mode & S_ISUID) != 0;

    case 'g': /* File is setgid? */
      return sh_stat (arg, &stat_buf) == 0
             && (stat_buf.st_mode & S_ISGID) != 0;

    case 'k': /* File has sticky bit set? */
#if !defined(S_ISVTX)
      /* This is not Posix, and is not defined on some Posix systems. */
      return false;
#else
      return sh_stat (arg, &stat_buf) == 0
             && (stat_buf.st_mode & S_ISVTX) != 0;
#endif

    case 't': /* File fd is a terminal? */
      if (!legal_number (arg, &r))
        return false;
      return (r == (int)r) && isatty ((int)r);

    case 'n': /* True if arg has some length. */
      return arg[0] != '\0';

    case 'z': /* True if arg has no length. */
      return arg[0] == '\0';

    case 'o': /* True if option `arg' is set. */
      return minus_o_option_value (arg) == 1;

    case 'v':
#if defined(ARRAY_VARS)
      if (valid_array_reference (arg, 0))
        {
          char *t;
          int rtype, flags;
          bool ret;

          /* Let's assume that this has already been expanded once. */
          flags = assoc_expand_once ? AV_NOEXPAND : 0;
          t = array_value (arg, 0, flags, &rtype, (arrayind_t *)0);
          ret = t;
          if (rtype > 0) /* subscript is * or @ */
            free (t);
          return ret;
        }
      else if (legal_number (arg, &r)) /* -v n == is $n set? */
        return r >= 0 && r <= number_of_args ();
      v = find_variable (arg);
      if (v && invisible_p (v) == 0 && array_p (v))
        {
          char *t;
          /* [[ -v foo ]] == [[ -v foo[0] ]] */
          t = array_reference (array_cell (v), 0);
          return t;
        }
      else if (v && invisible_p (v) == 0 && assoc_p (v))
        {
          char *t;
          t = assoc_reference (assoc_cell (v), "0");
          return t;
        }
#else
      v = find_variable (arg);
#endif
      return v && invisible_p (v) == 0 && var_isset (v);

    case 'R':
      v = find_variable_noref (arg);
      return v && invisible_p (v) == 0 && var_isset (v) && nameref_p (v);
    }

  /* We can't actually get here, but this shuts up gcc. */
  return false;
}

/* Return TRUE if OP is one of the test command's binary operators. */
bool
test_binop (const char *op)
{
  if (op[0] == '=' && op[1] == '\0')
    return 1;                                               /* '=' */
  else if ((op[0] == '<' || op[0] == '>') && op[1] == '\0') /* string <, > */
    return 1;
  else if ((op[0] == '=' || op[0] == '!') && op[1] == '=' && op[2] == '\0')
    return 1; /* `==' and `!=' */
#if defined(PATTERN_MATCHING)
  else if (op[2] == '\0' && op[1] == '~' && (op[0] == '=' || op[0] == '!'))
    return 1;
#endif
  else if (op[0] != '-' || op[1] == '\0' || op[2] == '\0' || op[3] != '\0')
    return 0;
  else
    {
      if (op[2] == 't')
        switch (op[1])
          {
          case 'n': /* -nt */
          case 'o': /* -ot */
          case 'l': /* -lt */
          case 'g': /* -gt */
            return 1;
          default:
            return 0;
          }
      else if (op[1] == 'e')
        switch (op[2])
          {
          case 'q': /* -eq */
          case 'f': /* -ef */
            return 1;
          default:
            return 0;
          }
      else if (op[2] == 'e')
        switch (op[1])
          {
          case 'n': /* -ne */
          case 'g': /* -ge */
          case 'l': /* -le */
            return 1;
          default:
            return 0;
          }
      else
        return 0;
    }
}

/* Return non-zero if OP is one of the test command's unary operators. */
bool
test_unop (const char *op)
{
  if (op[0] != '-' || (op[1] && op[2] != 0))
    return false;

  switch (op[1])
    {
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'k':
    case 'n':
    case 'o':
    case 'p':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'z':
    case 'G':
    case 'L':
    case 'O':
    case 'S':
    case 'N':
    case 'R':
      return true;
    }

  return false;
}

static bool
two_arguments ()
{
  if (argv[pos][0] == '!' && argv[pos][1] == '\0')
    return argv[pos + 1][0] == '\0';
  else if (argv[pos][0] == '-' && argv[pos][1] && argv[pos][2] == '\0')
    {
      if (test_unop (argv[pos]))
        return unary_operator ();
      else
        test_syntax_error (_ ("%s: unary operator expected"), argv[pos]);
    }
  else
    test_syntax_error (_ ("%s: unary operator expected"), argv[pos]);

  return false;
}

#define ANDOR(s) (s[0] == '-' && (s[1] == 'a' || s[1] == 'o') && s[2] == 0)

/* This could be augmented to handle `-t' as equivalent to `-t 1', but
   POSIX requires that `-t' be given an argument. */
#define ONE_ARG_TEST(s) ((s)[0] != '\0')

static bool
three_arguments ()
{
  bool value;

  if (test_binop (argv[pos + 1]))
    {
      value = binary_operator ();
      pos = argc;
    }
  else if (ANDOR (argv[pos + 1]))
    {
      if (argv[pos + 1][1] == 'a')
        value = ONE_ARG_TEST (argv[pos]) && ONE_ARG_TEST (argv[pos + 2]);
      else
        value = ONE_ARG_TEST (argv[pos]) || ONE_ARG_TEST (argv[pos + 2]);
      pos = argc;
    }
  else if (argv[pos][0] == '!' && argv[pos][1] == '\0')
    {
      advance (1);
      value = !two_arguments ();
    }
  else if (argv[pos][0] == '(' && argv[pos + 2][0] == ')')
    {
      value = ONE_ARG_TEST (argv[pos + 1]);
      pos = argc;
    }
  else
    test_syntax_error (_ ("%s: binary operator expected"), argv[pos + 1]);

  return value;
}

/* This is an implementation of a Posix.2 proposal by David Korn. */
static bool
posixtest ()
{
  bool value;

  switch (argc - 1) /* one extra passed in */
    {
    case 0:
      value = false;
      pos = argc;
      break;

    case 1:
      value = ONE_ARG_TEST (argv[1]);
      pos = argc;
      break;

    case 2:
      value = two_arguments ();
      pos = argc;
      break;

    case 3:
      value = three_arguments ();
      break;

    case 4:
      if (argv[pos][0] == '!' && argv[pos][1] == '\0')
        {
          advance (1);
          value = !three_arguments ();
          break;
        }
      else if (argv[pos][0] == '(' && argv[pos][1] == '\0'
               && argv[argc - 1][0] == ')' && argv[argc - 1][1] == '\0')
        {
          advance (1);
          value = two_arguments ();
          pos = argc;
          break;
        }
      /* FALLTHROUGH */
    default:
      value = expr ();
    }

  return value;
}

/*
 * [:
 *	'[' expr ']'
 * test:
 *	test expr
 */
int
test_command (int margc, char **margv)
{
  int value;
  int code;

  code = setjmp_nosigs (test_exit_buf);

  if (code)
    return test_error_return;

  argv = margv;

  if (margv[0] && margv[0][0] == '[' && margv[0][1] == '\0')
    {
      --margc;

      if (margv[margc] && (margv[margc][0] != ']' || margv[margc][1]))
        test_syntax_error (_ ("missing `]'"), nullptr);

      if (margc < 2)
        test_exit (SHELL_BOOLEAN (false));
    }

  argc = margc;
  pos = 1;

  if (pos >= argc)
    test_exit (SHELL_BOOLEAN (false));

  noeval = 0;
  value = posixtest ();

  if (pos != argc)
    {
      if (pos < argc && argv[pos][0] == '-')
        test_syntax_error (_ ("syntax error: `%s' unexpected"), argv[pos]);
      else
        test_syntax_error (_ ("too many arguments"), nullptr);
    }

  test_exit (SHELL_BOOLEAN (value));
}

} // namespace bash
