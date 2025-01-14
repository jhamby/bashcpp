// This file is printf_def.cc (previously builtins/printf.def).
// It implements the builtin "printf" in Bash.

// Copyright (C) 1997-2021 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

// $BUILTIN printf
// $FUNCTION printf_builtin
// $SHORT_DOC printf [-v var] format [arguments]
// Formats and prints ARGUMENTS under control of the FORMAT.

// Options:
//   -v var	assign the output to shell variable VAR rather than
// 		display it on the standard output

// FORMAT is a character string which contains three types of objects: plain
// characters, which are simply copied to standard output; character escape
// sequences, which are converted and copied to the standard output; and
// format specifications, each of which causes printing of the next successive
// argument.

// In addition to the standard format specifications described in printf(1),
// printf interprets:

//   %b	expand backslash escape sequences in the corresponding argument
//   %q	quote the argument in a way that can be reused as shell input
//   %Q	like %q, but apply any precision to the unquoted argument before
// 		quoting
//   %(fmt)T	output the date-time string resulting from using FMT as a format
// 	        string for strftime(3)

// The format is re-used as necessary to consume all of the arguments.  If
// there are fewer arguments than the format requires,  extra format
// specifications behave as if a zero value or null string, as appropriate,
// had been supplied.

// Exit Status:
// Returns success unless an invalid option is given or a write or assignment
// error occurs.
// $END

#include "config.h"

#include "shell.hh"

#if defined(PRI_MACROS_BROKEN)
#undef PRIdMAX
#endif

#if !defined(PRIdMAX)
#define PRIdMAX "lld"
#endif

namespace bash
{

#define PC(c)                                                                 \
  do                                                                          \
    {                                                                         \
      char b[2];                                                              \
      tw++;                                                                   \
      b[0] = c;                                                               \
      b[1] = '\0';                                                            \
      if (vflag)                                                              \
        vbadd (b, 1);                                                         \
      else                                                                    \
        putchar (c);                                                          \
      QUIT;                                                                   \
    }                                                                         \
  while (0)

#define PF(f, func)                                                           \
  do                                                                          \
    {                                                                         \
      int nw;                                                                 \
      clearerr (stdout);                                                      \
      if (have_fieldwidth && have_precision)                                  \
        nw = vflag ? vbprintf (f, fieldwidth, precision, func)                \
                   : printf (f, fieldwidth, precision, func);                 \
      else if (have_fieldwidth)                                               \
        nw = vflag ? vbprintf (f, fieldwidth, func)                           \
                   : printf (f, fieldwidth, func);                            \
      else if (have_precision)                                                \
        nw = vflag ? vbprintf (f, precision, func)                            \
                   : printf (f, precision, func);                             \
      else                                                                    \
        nw = vflag ? vbprintf (f, func) : printf (f, func);                   \
      tw += nw;                                                               \
      QUIT;                                                                   \
      if (ferror (stdout))                                                    \
        {                                                                     \
          sh_wrerror ();                                                      \
          clearerr (stdout);                                                  \
          return EXECUTION_FAILURE;                                           \
        }                                                                     \
    }                                                                         \
  while (0)

/* We free the buffer used by mklong() if it's `too big'. */
#define PRETURN(value)                                                        \
  do                                                                          \
    {                                                                         \
      QUIT;                                                                   \
      if (vflag)                                                              \
        {                                                                     \
          SHELL_VAR *v;                                                       \
          v = builtin_bind_variable (vname, vbuf, 0);                         \
          stupidly_hack_special_variables (vname);                            \
          if (v == 0 || readonly_p (v) || noassign_p (v))                     \
            return EXECUTION_FAILURE;                                         \
        }                                                                     \
      if (conv_bufsize > 4096)                                                \
        {                                                                     \
          free (conv_buf);                                                    \
          conv_bufsize = 0;                                                   \
          conv_buf = 0;                                                       \
        }                                                                     \
      if (vbsize > 4096)                                                      \
        {                                                                     \
          free (vbuf);                                                        \
          vbsize = 0;                                                         \
          vbuf = 0;                                                           \
        }                                                                     \
      else if (vbuf)                                                          \
        vbuf[0] = 0;                                                          \
      if (ferror (stdout) == 0)                                               \
        fflush (stdout);                                                      \
      QUIT;                                                                   \
      if (ferror (stdout))                                                    \
        {                                                                     \
          sh_wrerror ();                                                      \
          clearerr (stdout);                                                  \
          return EXECUTION_FAILURE;                                           \
        }                                                                     \
      return value;                                                           \
    }                                                                         \
  while (0)

#define SKIP1 "#'-+ 0"
#define LENMODS "hjlLtz"

// extern time_t shell_start_time;

#if !HAVE_ASPRINTF
extern int asprintf (char **, const char *, ...)
    __attribute__ ((__format__ (printf, 2, 3)));
#endif

#if !HAVE_VSNPRINTF
extern int vsnprintf (char *, size_t, const char *, va_list)
    __attribute__ ((__format__ (printf, 3, 0)));
#endif

#if 0
static void printf_erange (const char *);
static int printstr (const char *, const char *, int, int, int);
static int tescape (char *, char *, int *, int *);
static char *bexpand (char *, int, int *, int *);
static char *vbadd (char *, int);
static int vbprintf (const char *, ...) __attribute__((__format__ (printf, 1, 2)));
static char *mklong (const char *, const char *, size_t);
static int getchr ();
static char *getstr ();
static int  getint ();
static int64_t getintmax ();
static uint64_t getuintmax ();
#endif

#if defined(HAVE_LONG_DOUBLE) && HAVE_DECL_STRTOLD && !defined(STRTOLD_BROKEN)
typedef long double floatmax_t;
#define FLOATMAX_CONV "L"
#define strtofltmax strtold
#else
typedef double floatmax_t;
#define FLOATMAX_CONV ""
#define strtofltmax strtod
#endif

#if 0
static floatmax_t getfloatmax ();

static int64_t asciicode ();

static WORD_LIST *garglist, *orig_arglist;
static int retval;
static bool conversion_error;

/* printf -v var support */
static bool vflag = false;
static char *vbuf, *vname;
static size_t vbsize;
static int vblen;

static int64_t tw;

static char *conv_buf;
static size_t conv_bufsize;
#endif

int
Shell::printf_builtin (WORD_LIST *list)
{
  int ch, fieldwidth, precision;
  int have_fieldwidth, have_precision;
  char convch, thisch, nextch, *format, *modstart, *fmt, *start;
#if defined(HANDLE_MULTIBYTE)
  char mbch[25]; /* 25 > MB_LEN_MAX, plus can handle 4-byte UTF-8 and large
                    Unicode characters*/
  int mbind, mblen;
#endif
#if defined(ARRAY_VARS)
  int arrayflags;
#endif

  conversion_error = false;
  retval = EXECUTION_SUCCESS;

  vflag = false;

  reset_internal_getopt ();
  while ((ch = internal_getopt (list, "v:")) != -1)
    {
      switch (ch)
        {
        case 'v':
          vname = list_optarg;
#if defined(ARRAY_VARS)
          arrayflags = assoc_expand_once ? (VA_NOEXPAND | VA_ONEWORD) : 0;
          if (legal_identifier (vname)
              || valid_array_reference (vname, arrayflags))
#else
          if (legal_identifier (vname))
#endif
            {
              vflag = true;
              if (vbsize == 0)
                vbuf = (char *)xmalloc (vbsize = 16);
              vblen = 0;
              if (vbuf)
                vbuf[0] = 0;
            }
          else
            {
              sh_invalidid (vname);
              return EX_USAGE;
            }
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend; /* skip over possible `--' */

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  /* Allow printf -v var "" to act like var="" */
  if (vflag && list->word->word && list->word->word[0] == '\0')
    {
      SHELL_VAR *v;
      v = builtin_bind_variable (vname, "", bindflags);
      stupidly_hack_special_variables (vname);
      return (v == 0 || readonly_p (v) || noassign_p (v)) ? EXECUTION_FAILURE
                                                          : EXECUTION_SUCCESS;
    }

  if (list->word->word == 0 || list->word->word[0] == '\0')
    return EXECUTION_SUCCESS;

  format = list->word->word;
  tw = 0;

  garglist = orig_arglist = list->next ();

  /* If the format string is empty after preprocessing, return immediately. */
  if (format == 0 || *format == 0)
    return EXECUTION_SUCCESS;

  /* Basic algorithm is to scan the format string for conversion
     specifications -- once one is found, find out if the field
     width or precision is a '*'; if it is, gather up value.  Note,
     format strings are reused as necessary to use up the provided
     arguments, arguments of zero/null string are provided to use
     up the format string. */
  do
    {
      tw = 0;
      /* find next format specification */
      for (fmt = format; *fmt; fmt++)
        {
          precision = fieldwidth = 0;
          have_fieldwidth = have_precision = 0;

          if (*fmt == '\\')
            {
              fmt++;
              /* A NULL third argument to tescape means to bypass the
                 special processing for arguments to %b. */
#if defined(HANDLE_MULTIBYTE)
              /* Accommodate possible use of \u or \U, which can result in
                 multibyte characters */
              memset (mbch, '\0', sizeof (mbch));
              fmt += tescape (fmt, mbch, &mblen, (int *)NULL);
              for (mbind = 0; mbind < mblen; mbind++)
                PC (mbch[mbind]);
#else
              fmt += tescape (fmt, &nextch, (int *)NULL, (int *)NULL);
              PC (nextch);
#endif
              fmt--; /* for loop will increment it for us again */
              continue;
            }

          if (*fmt != '%')
            {
              PC (*fmt);
              continue;
            }

          /* ASSERT(*fmt == '%') */
          start = fmt++;

          if (*fmt == '%') /* %% prints a % */
            {
              PC ('%');
              continue;
            }

          /* found format specification, skip to field width */
          for (; *fmt && strchr (SKIP1, *fmt); ++fmt)
            ;

          /* Skip optional field width. */
          if (*fmt == '*')
            {
              fmt++;
              have_fieldwidth = 1;
              fieldwidth = getint ();
            }
          else
            while (DIGIT (*fmt))
              fmt++;

          /* Skip optional '.' and precision */
          if (*fmt == '.')
            {
              ++fmt;
              if (*fmt == '*')
                {
                  fmt++;
                  have_precision = 1;
                  precision = getint ();
                }
              else
                {
                  /* Negative precisions are allowed but treated as if the
                     precision were missing; I would like to allow a leading
                     `+' in the precision number as an extension, but lots
                     of asprintf/fprintf implementations get this wrong. */
#if 0
		  if (*fmt == '-' || *fmt == '+')
#else
                  if (*fmt == '-')
#endif
                  fmt++;
                  while (DIGIT (*fmt))
                    fmt++;
                }
            }

          /* skip possible format modifiers */
          modstart = fmt;
          while (*fmt && strchr (LENMODS, *fmt))
            fmt++;

          if (*fmt == 0)
            {
              builtin_error (_ ("`%s': missing format character"), start);
              PRETURN (EXECUTION_FAILURE);
            }

          convch = *fmt;
          thisch = modstart[0];
          nextch = modstart[1];
          modstart[0] = convch;
          modstart[1] = '\0';

          QUIT;
          switch (convch)
            {
            case 'c':
              {
                char p;

                p = getchr ();
                PF (start, p);
                break;
              }

            case 's':
              {
                char *p;

                p = getstr ();
                PF (start, p);
                break;
              }

            case '(':
              {
                char *timefmt, timebuf[128], *t;
                int n;
                int64_t arg;
                time_t secs;
                struct tm *tm;

                modstart[1] = nextch; /* restore char after left paren */
                timefmt = new char[strlen (fmt) + 3];
                fmt++; /* skip over left paren */
                for (t = timefmt, n = 1; *fmt;)
                  {
                    if (*fmt == '(')
                      n++;
                    else if (*fmt == ')')
                      n--;
                    if (n == 0)
                      break;
                    *t++ = *fmt++;
                  }
                *t = '\0';
                if (*++fmt != 'T')
                  {
                    builtin_warning (
                        _ ("`%c': invalid time format specification"), *fmt);
                    fmt = start;
                    delete[] timefmt;
                    PC (*fmt);
                    continue;
                  }
                if (timefmt[0] == '\0')
                  {
                    timefmt[0] = '%';
                    timefmt[1] = 'X'; /* locale-specific current time - should
                                         we use `+'? */
                    timefmt[2] = '\0';
                  }
                /* argument is seconds since the epoch with special -1 and -2
                 */
                /* default argument is equivalent to -1; special case */
                arg = garglist ? getintmax () : -1;
                if (arg == -1)
                  secs = NOW; /* roughly date +%s */
                else if (arg == -2)
                  secs = shell_start_time; /* roughly $SECONDS */
                else
                  secs = arg;
#if defined(HAVE_TZSET)
                sv_tz ("TZ"); /* XXX -- just make sure */
#endif
                tm = localtime (&secs);
                if (tm == 0)
                  {
                    secs = 0;
                    tm = localtime (&secs);
                  }
                n = tm ? strftime (timebuf, sizeof (timebuf), timefmt, tm) : 0;
                delete[] timefmt;
                if (n == 0)
                  timebuf[0] = '\0';
                else
                  timebuf[sizeof (timebuf) - 1] = '\0';
                /* convert to %s format that preserves fieldwidth and precision
                 */
                modstart[0] = 's';
                modstart[1] = '\0';
                n = printstr (start, timebuf, strlen (timebuf), fieldwidth,
                              precision); /* XXX - %s for now */
                if (n < 0)
                  {
                    if (ferror (stdout) == 0)
                      {
                        sh_wrerror ();
                        clearerr (stdout);
                      }
                    PRETURN (EXECUTION_FAILURE);
                  }
                break;
              }

            case 'n':
              {
                char *var;

                var = getstr ();
                if (var && *var)
                  {
                    if (legal_identifier (var))
                      bind_var_to_int (var, tw);
                    else
                      {
                        sh_invalidid (var);
                        PRETURN (EXECUTION_FAILURE);
                      }
                  }
                break;
              }

            case 'b': /* expand escapes in argument */
              {
                char *p, *xp;
                int rlen, r;

                p = getstr ();
                ch = rlen = r = 0;
                xp = bexpand (p, strlen (p), &ch, &rlen);

                if (xp)
                  {
                    /* Have to use printstr because of possible NUL bytes
                       in XP -- printf does not handle that well. */
                    r = printstr (start, xp, rlen, fieldwidth, precision);
                    if (r < 0)
                      {
                        if (ferror (stdout) == 0)
                          {
                            sh_wrerror ();
                            clearerr (stdout);
                          }
                        retval = EXECUTION_FAILURE;
                      }
                    free (xp);
                  }

                if (ch || r < 0)
                  PRETURN (retval);
                break;
              }

            case 'q': /* print with shell quoting */
              {
                char *p, *xp;
                int r;

                r = 0;
                p = getstr ();
                if (p && *p == 0) /* XXX - getstr never returns null */
                  xp = savestring ("''");
                else if (ansic_shouldquote (p))
                  xp = ansic_quote (p, 0, (int *)0);
                else
                  xp = sh_backslash_quote (p, 0, 3);
                if (xp)
                  {
                    /* Use printstr to get fieldwidth and precision right. */
                    r = printstr (start, xp, strlen (xp), fieldwidth,
                                  precision);
                    if (r < 0)
                      {
                        sh_wrerror ();
                        clearerr (stdout);
                      }
                    free (xp);
                  }

                if (r < 0)
                  PRETURN (EXECUTION_FAILURE);
                break;
              }

            case 'd':
            case 'i':
              {
                char *f;
                long p;
                int64_t pp;

                p = pp = getintmax ();
                if (p != pp)
                  {
                    f = mklong (start, PRIdMAX, sizeof (PRIdMAX) - 2);
                    PF (f, pp);
                  }
                else
                  {
                    /* Optimize the common case where the integer fits
                       in "long".  This also works around some long
                       long and/or int64_t library bugs in the common
                       case, e.g. glibc 2.2 x86.  */
                    f = mklong (start, "l", 1);
                    PF (f, p);
                  }
                break;
              }

            case 'o':
            case 'u':
            case 'x':
            case 'X':
              {
                char *f;
                unsigned long p;
                uint64_t pp;

                p = pp = getuintmax ();
                if (p != pp)
                  {
                    f = mklong (start, PRIdMAX, sizeof (PRIdMAX) - 2);
                    PF (f, pp);
                  }
                else
                  {
                    f = mklong (start, "l", 1);
                    PF (f, p);
                  }
                break;
              }

            case 'e':
            case 'E':
            case 'f':
            case 'F':
            case 'g':
            case 'G':
#if defined(HAVE_PRINTF_A_FORMAT)
            case 'a':
            case 'A':
#endif
              {
                char *f;
                floatmax_t p;

                p = getfloatmax ();
                f = mklong (start, FLOATMAX_CONV, sizeof (FLOATMAX_CONV) - 1);
                PF (f, p);
                break;
              }

            /* We don't output unrecognized format characters; we print an
               error message and return a failure exit status. */
            default:
              builtin_error (_ ("`%c': invalid format character"), convch);
              PRETURN (EXECUTION_FAILURE);
            }

          modstart[0] = thisch;
          modstart[1] = nextch;
        }

      if (ferror (stdout))
        {
          /* PRETURN will print error message. */
          PRETURN (EXECUTION_FAILURE);
        }
    }
  while (garglist && garglist != list->next);

  if (conversion_error)
    retval = EXECUTION_FAILURE;

  PRETURN (retval);
}

static void
printf_erange (const char *s)
{
  builtin_error (_ ("warning: %s: %s"), s, strerror (ERANGE));
}

/* We duplicate a lot of what printf(3) does here. */
static int
printstr (const char *fmt,    /* format */
          const char *string, /* expanded string argument */
          int len,            /* length of expanded string */
          int fieldwidth,     /* argument for width of `*' */
          int precision)      /* argument for precision of `*' */
{
#if 0
  char *s;
#endif
  int padlen, nc, ljust, i;
  int fw, pr; /* fieldwidth and precision */
  int64_t mfw, mpr;

  if (string == 0)
    string = "";

#if 0
  s = fmt;
#endif
  if (*fmt == '%')
    fmt++;

  ljust = fw = 0;
  pr = -1;
  mfw = 0;
  mpr = -1;

  /* skip flags */
  while (strchr (SKIP1, *fmt))
    {
      if (*fmt == '-')
        ljust = 1;
      fmt++;
    }

  /* get fieldwidth, if present.  rely on caller to clamp fieldwidth at INT_MAX
   */
  if (*fmt == '*')
    {
      fmt++;
      fw = fieldwidth;
      if (fw < 0)
        {
          fw = -fw;
          ljust = 1;
        }
    }
  else if (DIGIT (*fmt))
    {
      mfw = *fmt++ - '0';
      while (DIGIT (*fmt))
        mfw = (mfw * 10) + (*fmt++ - '0');
      /* Error if fieldwidth > INT_MAX here? */
      fw = (mfw < 0 || mfw > INT_MAX) ? INT_MAX : mfw;
    }

  /* get precision, if present */
  if (*fmt == '.')
    {
      fmt++;
      if (*fmt == '*')
        {
          fmt++;
          pr = precision;
        }
      else if (DIGIT (*fmt))
        {
          mpr = *fmt++ - '0';
          while (DIGIT (*fmt))
            mpr = (mpr * 10) + (*fmt++ - '0');
          /* Error if precision > INT_MAX here? */
          pr = (mpr < 0 || mpr > INT_MAX) ? INT_MAX : mpr;
        }
      else
        pr = 0; /* "a null digit string is treated as zero" */
    }

#if 0
  /* If we remove this, get rid of `s'. */
  if (*fmt != 'b' && *fmt != 'q')
    {
      internal_error (_("format parsing problem: %s"), s);
      fw = pr = 0;
    }
#endif

  /* chars from string to print */
  nc = (pr >= 0 && pr <= len) ? pr : len;

  padlen = fw - nc;
  if (padlen < 0)
    padlen = 0;
  if (ljust)
    padlen = -padlen;

  /* leading pad characters */
  for (; padlen > 0; padlen--)
    PC (' ');

  /* output NC characters from STRING */
  for (i = 0; i < nc; i++)
    PC (string[i]);

  /* output any necessary trailing padding */
  for (; padlen < 0; padlen++)
    PC (' ');

  return ferror (stdout) ? -1 : 0;
}

/* Convert STRING by expanding the escape sequences specified by the
   POSIX standard for printf's `%b' format string.  If SAWC is non-null,
   perform the processing appropriate for %b arguments.  In particular,
   recognize `\c' and use that as a string terminator.  If we see \c, set
   *SAWC to 1 before returning.  LEN is the length of STRING. */

/* Translate a single backslash-escape sequence starting at ESTART (the
   character after the backslash) and return the number of characters
   consumed by the sequence.  CP is the place to return the translated
   value.  *SAWC is set to 1 if the escape sequence was \c, since that means
   to short-circuit the rest of the processing.  If SAWC is null, we don't
   do the \c short-circuiting, and \c is treated as an unrecognized escape
   sequence; we also bypass the other processing specific to %b arguments.  */
static int
tescape (char *estart, char *cp, int *lenp, int *sawc)
{
  char *p;
  int temp, c, evalue;
  unsigned long uvalue;

  p = estart;
  if (lenp)
    *lenp = 1;

  switch (c = *p++)
    {
    case 'a':
      *cp = '\a';
      break;

    case 'b':
      *cp = '\b';
      break;

    case 'e':
    case 'E':
      *cp = '\033';
      break; /* ESC -- non-ANSI */

    case 'f':
      *cp = '\f';
      break;

    case 'n':
      *cp = '\n';
      break;

    case 'r':
      *cp = '\r';
      break;

    case 't':
      *cp = '\t';
      break;

    case 'v':
      *cp = '\v';
      break;

    /* The octal escape sequences are `\0' followed by up to three octal
       digits (if SAWC), or `\' followed by up to three octal digits (if
       !SAWC).  As an extension, we allow the latter form even if SAWC. */
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
      evalue = OCTVALUE (c);
      for (temp = 2 + (!evalue && !!sawc); ISOCTAL (*p) && temp--; p++)
        evalue = (evalue * 8) + OCTVALUE (*p);
      *cp = evalue & 0xFF;
      break;

    /* And, as another extension, we allow \xNN, where each N is a
       hex digit. */
    case 'x':
      for (temp = 2, evalue = 0; ISXDIGIT ((unsigned char)*p) && temp--; p++)
        evalue = (evalue * 16) + HEXVALUE (*p);
      if (p == estart + 1)
        {
          builtin_error (_ ("missing hex digit for \\x"));
          *cp = '\\';
          return 0;
        }
      *cp = evalue & 0xFF;
      break;

#if defined(HANDLE_MULTIBYTE)
    case 'u':
    case 'U':
      temp = (c == 'u') ? 4 : 8; /* \uNNNN \UNNNNNNNN */
      for (uvalue = 0; ISXDIGIT ((unsigned char)*p) && temp--; p++)
        uvalue = (uvalue * 16) + HEXVALUE (*p);
      if (p == estart + 1)
        {
          builtin_error (_ ("missing unicode digit for \\%c"), c);
          *cp = '\\';
          return 0;
        }
      if (uvalue <= 0x7f) /* <= 0x7f translates directly */
        *cp = uvalue;
      else
        {
          temp = u32cconv (uvalue, cp);
          cp[temp] = '\0';
          if (lenp)
            *lenp = temp;
        }
      break;
#endif

    case '\\': /* \\ -> \ */
      *cp = c;
      break;

    /* SAWC == 0 means that \', \", and \? are recognized as escape
       sequences, though the only processing performed is backslash
       removal. */
    case '\'':
    case '"':
    case '?':
      if (!sawc)
        *cp = c;
      else
        {
          *cp = '\\';
          return 0;
        }
      break;

    case 'c':
      if (sawc)
        {
          *sawc = 1;
          break;
        }
    /* other backslash escapes are passed through unaltered */
    default:
      *cp = '\\';
      return 0;
    }
  return p - estart;
}

static char *
bexpand (char *string, int len, int *sawc, int *lenp)
{
  int temp;
  char *ret, *r, *s, c;
#if defined(HANDLE_MULTIBYTE)
  char mbch[25];
  int mbind, mblen;
#endif

  if (string == 0 || len == 0)
    {
      if (sawc)
        *sawc = 0;
      if (lenp)
        *lenp = 0;
      ret = (char *)xmalloc (1);
      ret[0] = '\0';
      return ret;
    }

  ret = (char *)xmalloc (len + 1);
  for (r = ret, s = string; s && *s;)
    {
      c = *s++;
      if (c != '\\' || *s == '\0')
        {
          *r++ = c;
          continue;
        }
      temp = 0;
#if defined(HANDLE_MULTIBYTE)
      memset (mbch, '\0', sizeof (mbch));
      s += tescape (s, mbch, &mblen, &temp);
#else
      s += tescape (s, &c, (int *)NULL, &temp);
#endif
      if (temp)
        {
          if (sawc)
            *sawc = 1;
          break;
        }

#if defined(HANDLE_MULTIBYTE)
      for (mbind = 0; mbind < mblen; mbind++)
        *r++ = mbch[mbind];
#else
      *r++ = c;
#endif
    }

  *r = '\0';
  if (lenp)
    *lenp = r - ret;
  return ret;
}

static char *
vbadd (char *buf, int blen)
{
  size_t nlen;

  nlen = vblen + blen + 1;
  if (nlen >= vbsize)
    {
      vbsize = ((nlen + 63) >> 6) << 6;
      vbuf = (char *)xrealloc (vbuf, vbsize);
    }

  if (blen == 1)
    vbuf[vblen++] = buf[0];
  else if (blen > 1)
    {
      FASTCOPY (buf, vbuf + vblen, blen);
      vblen += blen;
    }
  vbuf[vblen] = '\0';

#ifdef DEBUG
  if (strlen (vbuf) != vblen)
    internal_error ("printf:vbadd: vblen (%d) != strlen (vbuf) (%d)", vblen,
                    (int)strlen (vbuf));
#endif

  return vbuf;
}

static int
vbprintf (const char *format, ...)
{
  va_list args;
  size_t nlen;
  int blen;

  SH_VA_START (args, format);
  blen = vsnprintf (vbuf + vblen, vbsize - vblen, format, args);
  va_end (args);

  nlen = vblen + blen + 1;
  if (nlen >= vbsize)
    {
      vbsize = ((nlen + 63) >> 6) << 6;
      vbuf = (char *)xrealloc (vbuf, vbsize);
      SH_VA_START (args, format);
      blen = vsnprintf (vbuf + vblen, vbsize - vblen, format, args);
      va_end (args);
    }

  vblen += blen;
  vbuf[vblen] = '\0';

#ifdef DEBUG
  if (strlen (vbuf) != vblen)
    internal_error ("printf:vbprintf: vblen (%d) != strlen (vbuf) (%d)", vblen,
                    (int)strlen (vbuf));
#endif

  return blen;
}

static char *
mklong (const char *str, const char *modifiers, size_t mlen)
{
  size_t len, slen;

  slen = strlen (str);
  len = slen + mlen + 1;

  if (len > conv_bufsize)
    {
      conv_bufsize = (((len + 1023) >> 10) << 10);
      conv_buf = (char *)xrealloc (conv_buf, conv_bufsize);
    }

  FASTCOPY (str, conv_buf, slen - 1);
  FASTCOPY (modifiers, conv_buf + slen - 1, mlen);

  conv_buf[len - 2] = str[slen - 1];
  conv_buf[len - 1] = '\0';
  return conv_buf;
}

static int
getchr ()
{
  int ret;

  if (garglist == 0)
    return '\0';

  ret = (int)garglist->word->word[0];
  garglist = (WORD_LIST *)garglist->next;
  return ret;
}

static char *
getstr ()
{
  char *ret;

  if (garglist == 0)
    return (char *)("");

  ret = garglist->word->word;
  garglist = (WORD_LIST *)garglist->next;
  return ret;
}

static int
getint ()
{
  int64_t ret;

  ret = getintmax ();

  if (garglist == 0)
    return ret;

  if (ret > INT_MAX)
    {
      printf_erange (garglist->word->word);
      ret = INT_MAX;
    }
  else if (ret < INT_MIN)
    {
      printf_erange (garglist->word->word);
      ret = INT_MIN;
    }

  return (int)ret;
}

static int64_t
getintmax ()
{
  int64_t ret;
  char *ep;

  if (garglist == 0)
    return 0;

  if (garglist->word->word[0] == '\'' || garglist->word->word[0] == '"')
    return asciicode ();

  errno = 0;
  ret = strtoimax (garglist->word->word, &ep, 0);

  if (*ep)
    {
      sh_invalidnum (garglist->word->word);
      /* POSIX.2 says ``...a diagnostic message shall be written to standard
         error, and the utility shall not exit with a zero exit status, but
         shall continue processing any remaining operands and shall write the
         value accumulated at the time the error was detected to standard
         output.''  Yecch. */
#if 0
      ret = 0;		/* return partially-converted value from strtoimax */
#endif
      conversion_error = 1;
    }
  else if (errno == ERANGE)
    printf_erange (garglist->word->word);

  garglist = (WORD_LIST *)garglist->next;
  return ret;
}

static uint64_t
getuintmax ()
{
  uint64_t ret;
  char *ep;

  if (garglist == 0)
    return 0;

  if (garglist->word->word[0] == '\'' || garglist->word->word[0] == '"')
    return asciicode ();

  errno = 0;
  ret = strtoumax (garglist->word->word, &ep, 0);

  if (*ep)
    {
      sh_invalidnum (garglist->word->word);
#if 0
      /* Same POSIX.2 conversion error requirements as getintmax(). */
      ret = 0;
#endif
      conversion_error = 1;
    }
  else if (errno == ERANGE)
    printf_erange (garglist->word->word);

  garglist = (WORD_LIST *)garglist->next;
  return ret;
}

static floatmax_t
getfloatmax ()
{
  floatmax_t ret;
  char *ep;

  if (garglist == 0)
    return 0;

  if (garglist->word->word[0] == '\'' || garglist->word->word[0] == '"')
    return asciicode ();

  errno = 0;
  ret = strtofltmax (garglist->word->word, &ep);

  if (*ep)
    {
      sh_invalidnum (garglist->word->word);
#if 0
      /* Same thing about POSIX.2 conversion error requirements. */
      ret = 0;
#endif
      conversion_error = 1;
    }
  else if (errno == ERANGE)
    printf_erange (garglist->word->word);

  garglist = (WORD_LIST *)garglist->next;
  return ret;
}

/* NO check is needed for garglist here. */
static int64_t
asciicode ()
{
  int64_t ch;
#if defined(HANDLE_MULTIBYTE)
  wchar_t wc;
  size_t slen;
  int mblength;
#endif
  DECLARE_MBSTATE;

#if defined(HANDLE_MULTIBYTE)
  slen = strlen (garglist->word->word + 1);
  wc = 0;
  mblength = mbtowc (&wc, garglist->word->word + 1, slen);
  if (mblength > 0)
    ch = wc; /* XXX */
  else
#endif
    ch = (unsigned char)garglist->word->word[1];

  garglist = (WORD_LIST *)garglist->next;
  return ch;
}

} // namespace bash
