/* test.cc - GNU test program (ksb and mjb) */

/* Modified to run with the GNU shell Apr 25, 1988 by bfox. */

/* Copyright (C) 1987-2021 Free Software Foundation, Inc.

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

#include "config.h"

#include "bashtypes.hh"

#if !defined(HAVE_LIMITS_H) && defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#include <fnmatch.h>
#include <unistd.h>

#include "pathexp.hh"
#include "shell.hh"
#include "stat-time.hh"

#include "builtins/common.hh"

namespace bash
{

#if !defined(R_OK)
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0
#endif /* R_OK */

#define EQ 0
#define NE 1
#define LT 2
#define GT 3
#define LE 4
#define GE 5

#define NT 0
#define OT 1
#define EF 2

/* SHELL_BOOLEAN is the form which returns truth or falseness in shell terms.
   Default is SHELL_BOOLEAN = (!value). */
#define SHELL_BOOLEAN(value) (!(value))

#define TEST_ERREXIT_STATUS 2

#if 0
static procenv_t test_exit_buf;
static int test_error_return;
#define test_exit(val)                                                        \
  do                                                                          \
    {                                                                         \
      test_error_return = val;                                                \
      sh_longjmp (test_exit_buf, 1);                                          \
    }                                                                         \
  while (0)

extern int sh_stat PARAMS((string_view , struct stat *));

static int pos;		/* The offset of the current argument in ARGV. */
static int argc;	/* The number of arguments present in ARGV. */
static char **argv;	/* The argument list. */
static int noeval;
#endif

void
Shell::test_syntax_error (const char *format, const char *arg)
{
  builtin_error (format, arg);
  throw test_exit_exception (TEST_ERREXIT_STATUS);
}

/*
 * beyond - call when we're beyond the end of the argument list (an
 *	error condition)
 */
void
Shell::beyond ()
{
  test_syntax_error (_ ("argument expected"), nullptr);
}

/* Syntax error for when an integer argument was expected, but
   something else was found. */
void
Shell::integer_expected_error (const char *pch)
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
bool
Shell::expr (STRINGLIST &args, size_t &pos)
{
  if (pos >= args.size ())
    beyond ();

  return test_or (args, pos); /* Same with this. */
}

/*
 * or:
 *	and
 *	and '-o' or
 */
bool
Shell::test_or (STRINGLIST &args, size_t &pos)
{
  size_t argc = args.size ();

  bool value = test_and (args, pos);
  if (pos < argc && args[pos][0] == '-' && args[pos][1] == 'o'
      && !args[pos][2])
    {
      advance (0);
      bool v2 = test_or (args, pos);
      return value || v2;
    }

  return value;
}

/*
 * and:
 *	term
 *	term '-a' and
 */
bool
Shell::test_and (STRINGLIST &args, size_t &pos)
{
  size_t argc = args.size ();

  bool value = term (args, pos);
  if (pos < argc && args[pos][0] == '-' && args[pos][1] == 'a'
      && !args[pos][2])
    {
      advance (0);
      bool v2 = test_and (args, pos);
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
bool
Shell::term (STRINGLIST &args, size_t &pos)
{
  size_t argc = args.size ();

  if (pos >= argc)
    beyond ();

  /* Deal with leading `not's. */
  if (args[pos][0] == '!' && args[pos][1] == '\0')
    {
      int value = 0;
      while (pos < argc && args[pos][0] == '!' && args[pos][1] == '\0')
        {
          advance (1);
          value = 1 - value;
        }

      return value ? !term (args, pos) : term (args, pos);
    }

  /* A paren-bracketed argument. */
  if (args[pos][0] == '(' && args[pos][1] == '\0')
    {
      advance (1);
      int value = expr (args, pos);
      if (args[pos] == 0)
        test_syntax_error (_ ("`)' expected"), nullptr);
      else if (args[pos][0] != ')' || args[pos][1])
        test_syntax_error (_ ("`)' expected, found %s"), args[pos]);
      advance (0);
      return value;
    }

  /* are there enough arguments left that this could be dyadic? */
  if ((pos + 3 <= argc) && test_binop (args[pos + 1]))
    return binary_operator (args, pos);

  /* Might be a switch type argument -- make sure we have enough arguments for
     the unary operator and argument */
  else if ((pos + 2) <= argc && test_unop (args[pos]))
    return unary_operator (args, pos);

  else
    {
      bool value = args[pos][0] != '\0';
      advance (0);
      return value;
    }
}

bool
Shell::stat_mtime (const char *fn, struct stat *st, struct timespec *ts)
{
  int r = sh_stat (fn, st);
  if (r < 0)
    return r;
  *ts = get_stat_mtime (st);
  return 0;
}

bool
Shell::filecomp (const char *s, const char *t, int op)
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
Shell::arithcomp (const char *s, const char *t, int op,
                  binary_test_flags flags)
{
  int64_t l, r;
  bool expok;

  if (flags & TEST_ARITHEXP) /* conditional command */
    {
      eval_flags eflag;

      eflag = (shell_compatibility_level > 51) ? EXP_NOFLAGS : EXP_EXPANDED;
      l = evalexp (s, eflag, &expok);
      if (!expok)
        return false; /* should probably longjmp here */
      r = evalexp (t, eflag, &expok);
      if (!expok)
        return false; /* ditto */
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
  int m = fnmatch (pat, string, FNMATCH_EXTFLAG | FNMATCH_IGNCASE);
  return (op == EQ) ? (m == 0) : (m != 0);
}

bool
Shell::binary_test (const char *op, const char *arg1, const char *arg2,
                    binary_test_flags flags)
{
  bool patmatch = (flags & TEST_PATMATCH);

  if (op[0] == '=' && (op[1] == '\0' || (op[1] == '=' && op[2] == '\0')))
    return patmatch ? patcomp (arg1, arg2, EQ) : STREQ (arg1, arg2);
  else if ((op[0] == '>' || op[0] == '<') && op[1] == '\0')
    {
#if defined(HAVE_STRCOLL)
      if (shell_compatibility_level > 40 && flags & TEST_LOCALE)
        return ((op[0] == '>') ? (strcoll (arg1, arg2) > 0)
                               : (strcoll (arg1, arg2) < 0));
      else
#endif
        return ((op[0] == '>') ? (strcmp (arg1, arg2) > 0)
                               : (strcmp (arg1, arg2) < 0));
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

bool
Shell::binary_operator (STRINGLIST &args, size_t &pos)
{
  int value;
  char *w;

  w = args[pos + 1];
  if ((w[0] == '=' && (w[1] == '\0' || (w[1] == '=' && w[2] == '\0')))
      ||                                                /* =, == */
      ((w[0] == '>' || w[0] == '<') && w[1] == '\0') || /* <, > */
      (w[0] == '!' && w[1] == '=' && w[2] == '\0'))     /* != */
    {
      value = binary_test (w, args[pos], args[pos + 2], TEST_NOFLAGS);
      pos += 3;
      return value;
    }

#if defined(PATTERN_MATCHING)
  if ((w[0] == '=' || w[0] == '!') && w[1] == '~' && w[2] == '\0')
    {
      value = patcomp (args[pos], args[pos + 2], w[0] == '=' ? EQ : NE);
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

  value = binary_test (w, args[pos], args[pos + 2], TEST_NOFLAGS);
  pos += 3;
  return value;
}

bool
Shell::unary_operator (STRINGLIST &args, size_t &pos)
{
  char *op;
  int64_t r;

  op = args[pos];
  if (test_unop (op) == 0)
    return false;

  /* the only tricky case is `-t', which may or may not take an argument. */
  if (op[1] == 't')
    {
      advance (0);
      if (pos < argc)
        {
          if (legal_number (args[pos], &r))
            {
              advance (0);
              return unary_test (op, args[pos - 1], 0);
            }
          else
            return false;
        }
      else
        return unary_test (op, "1", 0);
    }

  /* All of the unary operators take an argument, so we first call
     unary_advance (), which checks to make sure that there is an
     argument, and then advances pos right past it.  This means that
     pos - 1 is the location of the argument. */
  unary_advance ();
  return unary_test (op, args[pos - 1], 0);
}

bool
Shell::unary_test (const char *op, const char *arg)
{
  int64_t r;
  struct stat stat_buf;
  struct timespec mtime, atime;
  SHELL_VAR *v;
  av_flags aflags;

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
              && current_user.euid == stat_buf.st_uid);

    case 'G': /* File is owned by your group? */
      return (sh_stat (arg, &stat_buf) == 0
              && current_user.egid == stat_buf.st_gid);

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
      return (sh_stat (arg, &stat_buf) == 0
              && (stat_buf.st_mode & S_ISUID) != 0);

    case 'g': /* File is setgid? */
      return (sh_stat (arg, &stat_buf) == 0
              && (stat_buf.st_mode & S_ISGID) != 0);

    case 'k': /* File has sticky bit set? */
#if !defined(S_ISVTX)
      /* This is not Posix, and is not defined on some Posix systems. */
      return false;
#else
      return (sh_stat (arg, &stat_buf) == 0
              && (stat_buf.st_mode & S_ISVTX) != 0);
#endif

    case 't': /* File fd is a terminal? */
      if (legal_number (arg, &r) == 0)
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
      aflags = assoc_expand_once ? AV_NOEXPAND : AV_NOFLAGS;
      if (valid_array_reference (arg, aflags))
        {
          char *t;
          int ret;
          array_eltstate_t es;

          /* Let's assume that this has already been expanded once. */
          /* XXX - TAG:bash-5.2 fix with corresponding fix to execute_cmd.c:
             execute_cond_node() that passes TEST_ARRAYEXP in FLAGS */

          if (shell_compatibility_level > 51)
            /* Allow associative arrays to use `test -v array[@]' to look for
               a key named `@'. */
            aflags |= AV_ATSTARKEYS; /* XXX */

          init_eltstate (&es);

          t = get_array_value (arg, aflags | AV_ALLOWALL, &es);
          ret = (t != nullptr);
          if (es.subtype > 0) /* subscript is * or @ */
            free (t);
          flush_eltstate (&es);
          return ret;
        }
      else if (legal_number (arg, &r)) /* -v n == is $n set? */
        return r >= 0 && r <= number_of_args ();
      v = find_variable (arg);
      if (v && !v->invisible () && v->array ())
        {
          char *t;
          /* [[ -v foo ]] == [[ -v foo[0] ]] */
          t = array_reference (array_cell (v), 0);
          return t != nullptr;
        }
      else if (v && !v->invisible () && v->assoc ())
        {
          char *t;
          t = assoc_reference (assoc_cell (v), "0");
          return t != nullptr;
        }
#else
      v = find_variable (arg);
#endif
      return v && !v->invisible () && v->is_set ();

    case 'R':
      v = find_variable_noref (arg);
      return v && !v->invisible () && v->is_set () && v->nameref ();
    }

  /* We can't actually get here, but this shuts up gcc. */
  return false;
}

/* Return TRUE if OP is one of the test command's binary operators. */
bool
Shell::test_binop (const char *op)
{
  if (op == "=")
    return true;                   /* '=' */
  else if (op == "<" || op == ">") /* string <, > */
    return true;
  else if (op == "==" || op == "!=")
    return true; /* `==' and `!=' */
#if defined(PATTERN_MATCHING)
  else if (op == "=~" || op == "!~")
    return true;
#endif
  else if (op[0] != '-' || op[1] == '\0' || op[2] == '\0' || op[3] != '\0')
    return false;
  else
    {
      if (op[2] == 't')
        switch (op[1])
          {
          case 'n': /* -nt */
          case 'o': /* -ot */
          case 'l': /* -lt */
          case 'g': /* -gt */
            return true;
          default:
            return false;
          }
      else if (op[1] == 'e')
        switch (op[2])
          {
          case 'q': /* -eq */
          case 'f': /* -ef */
            return true;
          default:
            return false;
          }
      else if (op[2] == 'e')
        switch (op[1])
          {
          case 'n': /* -ne */
          case 'g': /* -ge */
          case 'l': /* -le */
            return true;
          default:
            return false;
          }
      else
        return false;
    }
}

bool
Shell::two_arguments (STRINGLIST &args, size_t &pos)
{
  if (args[pos][0] == '!' && args[pos][1] == '\0')
    return args[pos + 1][0] == '\0';
  else if (args[pos][0] == '-' && args[pos][1] && args[pos][2] == '\0')
    {
      if (test_unop (args[pos]))
        return unary_operator (args, pos);
      else
        test_syntax_error (_ ("%s: unary operator expected"), args[pos]);
    }
  else
    test_syntax_error (_ ("%s: unary operator expected"), args[pos]);

  return false;
}

#define ANDOR(s) (s[0] == '-' && (s[1] == 'a' || s[1] == 'o') && s[2] == 0)

/* This could be augmented to handle `-t' as equivalent to `-t 1', but
   POSIX requires that `-t' be given an argument. */
#define ONE_ARG_TEST(s) ((s)[0] != '\0')

int
Shell::three_arguments (STRINGLIST &args, size_t &pos)
{
  size_t argc = args.size ();
  int value;

  if (test_binop (args[pos + 1]))
    {
      value = binary_operator (args, pos);
      pos = argc;
    }
  else if (ANDOR (args[pos + 1]))
    {
      if (args[pos + 1][1] == 'a')
        value = ONE_ARG_TEST (args[pos]) && ONE_ARG_TEST (args[pos + 2]);
      else
        value = ONE_ARG_TEST (args[pos]) || ONE_ARG_TEST (args[pos + 2]);
      pos = argc;
    }
  else if (args[pos][0] == '!' && args[pos][1] == '\0')
    {
      advance (1);
      value = !two_arguments (args, pos);
      pos = argc;
    }
  else if (args[pos][0] == '(' && args[pos + 2][0] == ')')
    {
      value = ONE_ARG_TEST (args[pos + 1]);
      pos = argc;
    }
  else
    test_syntax_error (_ ("%s: binary operator expected"), args[pos + 1]);

  return value;
}

/* This is an implementation of a Posix.2 proposal by David Korn. */
int
Shell::posixtest (STRINGLIST &args, size_t &pos)
{
  size_t argc = args.size ();
  int value;

  switch (args.size () - 1) /* one extra passed in */
    {
    case 0:
      value = false;
      pos = argc;
      break;

    case 1:
      value = ONE_ARG_TEST (args[1]);
      pos = argc;
      break;

    case 2:
      value = two_arguments (args, pos);
      pos = argc;
      break;

    case 3:
      value = three_arguments (args, pos);
      break;

    case 4:
      if (args[pos][0] == '!' && args[pos][1] == '\0')
        {
          advance (1);
          value = !three_arguments (args, pos);
          break;
        }
      else if (args[pos][0] == '(' && args[pos][1] == '\0'
               && args[argc - 1][0] == ')' && args[argc - 1][1] == '\0')
        {
          advance (1);
          value = two_arguments (args, pos);
          pos = argc;
          break;
        }
      /* FALLTHROUGH */
    default:
      value = expr (args, pos);
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
Shell::test_command (STRINGLIST &args)
{
  try
    {
      size_t argc = args.size ();
      if (argc && args[0][0] == '[' && args[0][1] == '\0')
        {
          --argc;

          if (args[argc] && (args[argc][0] != ']' || args[argc][1]))
            test_syntax_error (_ ("missing `]'"), nullptr);

          if (argc < 2)
            return SHELL_BOOLEAN (false);

          args.pop_back ();
        }

      size_t pos = 1;

      if (pos >= argc)
        return SHELL_BOOLEAN (false);

      int value = posixtest (args, pos);

      if (pos != argc)
        {
          if (pos < argc && args[pos][0] == '-')
            test_syntax_error (_ ("syntax error: `%s' unexpected"), args[pos]);
          else
            test_syntax_error (_ ("too many arguments"), nullptr);
        }

      return SHELL_BOOLEAN (value);
    }
  catch (const test_exit_exception &e)
    {
      return e.test_error_return;
    }
}

} // namespace bash
