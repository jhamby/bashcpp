// This file is read_def.cc.
// It implements the builtin "read" in Bash.

// Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

// $BUILTIN read
// $FUNCTION read_builtin
// $SHORT_DOC read [-ers] [-a array] [-d delim] [-i text] [-n nchars] [-N nchars] [-p prompt] [-t timeout] [-u fd] [name ...]
// Read a line from the standard input and split it into fields.

// Reads a single line from the standard input, or from file descriptor FD
// if the -u option is supplied.  The line is split into fields as with word
// splitting, and the first word is assigned to the first NAME, the second
// word to the second NAME, and so on, with any leftover words assigned to
// the last NAME.  Only the characters found in $IFS are recognized as word
// delimiters.

// If no NAMEs are supplied, the line read is stored in the REPLY variable.

// Options:
//   -a array	assign the words read to sequential indices of the array
// 		variable ARRAY, starting at zero
//   -d delim	continue until the first character of DELIM is read, rather
// 		than newline
//   -e	use Readline to obtain the line
//   -i text	use TEXT as the initial text for Readline
//   -n nchars	return after reading NCHARS characters rather than waiting
// 		for a newline, but honor a delimiter if fewer than
// 		NCHARS characters are read before the delimiter
//   -N nchars	return only after reading exactly NCHARS characters, unless
// 		EOF is encountered or read times out, ignoring any
// 		delimiter
//   -p prompt	output the string PROMPT without a trailing newline before
// 		attempting to read
//   -r	do not allow backslashes to escape any characters
//   -s	do not echo input coming from a terminal
//   -t timeout	time out and return failure if a complete line of
// 		input is not read within TIMEOUT seconds.  The value of the
// 		TMOUT variable is the default timeout.  TIMEOUT may be a
// 		fractional number.  If TIMEOUT is 0, read returns
// 		immediately, without trying to read any data, returning
// 		success only if input is available on the specified
// 		file descriptor.  The exit status is greater than 128
// 		if the timeout is exceeded
//   -u fd	read from file descriptor FD instead of the standard input

// Exit Status:
// The return code is zero, unless end-of-file is encountered, read times out
// (in which case it's greater than 128), a variable assignment error occurs,
// or an invalid file descriptor is supplied as the argument to -u.
// $END

#include "config.h"

#include "bashtypes.hh"
#include "posixstat.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <csignal>

#ifdef __CYGWIN__
#include <fcntl.h>
#include <io.h>
#endif

#include "bashintl.hh"

#include "common.hh"
#include "shell.hh"
#include "trap.hh"

#include "shtty.hh"

#if defined(READLINE)
#include "readline.hh"
#endif

#if defined(BUFFERED_INPUT)
#include "input.hh"
#endif

#include "shmbutil.hh"

namespace bash
{

struct ttsave
{
  int fd;
  TTYSTRUCT attrs;
};

#if 0
#if defined(READLINE)
static void reset_attempted_completion_function ();
static int set_itext (void);
static char *edit_line (const char *, char *);
static void set_eol_delim (int);
static void reset_eol_delim ();
#endif
static SHELL_VAR *bind_read_variable (const char *, const char *);
#if defined(HANDLE_MULTIBYTE)
static int read_mbchar (int, char *, int, int, int);
#endif
static void ttyrestore (void *ttp);

static SigHandler sigalrm (int);
static void reset_alarm (void);

/* Try this to see what the rest of the shell can do with the information. */
// procenv_t alrmbuf;
bool sigalrm_seen;

static int reading, tty_modified;
static SigHandler *old_alrm;
static unsigned char delim;

static struct ttsave termsave;
#endif

/* In all cases, SIGALRM just sets a flag that we check periodically.  This
   avoids problems with the semi-tricky stuff we do with the xfree of
   input_string at the top of the unwind-protect list (see below). */

/* Set a flag that CHECK_ALRM can check.  This relies on zread or read_builtin
   calling trap.c:check_signals(), which knows about sigalrm_seen and alrmbuf.
 */
static void
sigalrm (int s)
{
  sigalrm_seen = true;
}

static void
reset_alarm ()
{
  /* Cancel alarm before restoring signal handler. */
  falarm (0, 0);
  set_signal_handler (SIGALRM, old_alrm);
}

/* Read the value of the shell variables whose names follow.
   The reading is done from the current input stream, whatever
   that may be.  Successive words of the input line are assigned
   to the variables mentioned in LIST.  The last variable in LIST
   gets the remainder of the words on the line.  If no variables
   are mentioned in LIST, then the default variable is $REPLY. */
int
Shell::read_builtin (WORD_LIST *list)
{
  int size, nr, pass_next, saw_escape, eof, opt, retval, code, print_ps2,
      nflag;
  bool input_is_tty, input_is_pipe, skip_ctlesc, skip_ctlnul;
  int unbuffered_read;
  int raw, edit, nchars, silent, have_timeout, ignore_delim, fd;
  int lastsig, t_errno;
  int mb_cur_max;
  unsigned int tmsec, tmusec;
  long ival, uval;
  int64_t intval;
  char c;
  char *input_string, *orig_input_string, *prompt, *arrayname;
  char *t, *t1, *ps2, *tofree;
  struct stat tsb;
  SHELL_VAR *var;
  TTYSTRUCT ttattrs, ttset;
#if defined(ARRAY_VARS)
  WORD_LIST *alist;
  int vflags;
#endif
#if defined(READLINE)
  char *rlbuf, *itext;
  int rlind;
  FILE *save_instream;
#endif

  sigalrm_seen = reading = tty_modified = 0;

  int i = 0;      /* Index into the string that we are reading. */
  raw = edit = 0; /* Not reading raw input by default. */
  silent = 0;
  arrayname = prompt = (char *)NULL;
  fd = 0; /* file descriptor to read from */

#if defined(READLINE)
  rlbuf = itext = (char *)0;
  rlind = 0;
#endif

  mb_cur_max = MB_CUR_MAX;
  tmsec = tmusec = 0; /* no timeout */
  nr = nchars = input_is_tty = input_is_pipe = unbuffered_read = have_timeout
      = 0;
  delim = '\n'; /* read until newline */
  ignore_delim = nflag = 0;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "ersa:d:i:n:p:t:u:N:")) != -1)
    {
      switch (opt)
        {
        case 'r':
          raw = 1;
          break;
        case 'p':
          prompt = list_optarg;
          break;
        case 's':
          silent = 1;
          break;
        case 'e':
#if defined(READLINE)
          edit = 1;
#endif
          break;
        case 'i':
#if defined(READLINE)
          itext = list_optarg;
#endif
          break;
#if defined(ARRAY_VARS)
        case 'a':
          arrayname = list_optarg;
          break;
#endif
        case 't':
          code = uconvert (list_optarg, &ival, &uval, (char **)NULL);
          if (code == 0 || ival < 0 || uval < 0)
            {
              builtin_error (_ ("%s: invalid timeout specification"),
                             list_optarg);
              return EXECUTION_FAILURE;
            }
          else
            {
              have_timeout = 1;
              tmsec = ival;
              tmusec = uval;
            }
          break;
        case 'N':
          ignore_delim = 1;
          delim = (unsigned char)-1;
        case 'n':
          nflag = 1;
          code = legal_number (list_optarg, &intval);
          if (code == 0 || intval < 0 || intval != (int)intval)
            {
              sh_invalidnum (list_optarg);
              return EXECUTION_FAILURE;
            }
          else
            nchars = intval;
          break;
        case 'u':
          code = legal_number (list_optarg, &intval);
          if (code == 0 || intval < 0 || intval != (int)intval)
            {
              builtin_error (_ ("%s: invalid file descriptor specification"),
                             list_optarg);
              return EXECUTION_FAILURE;
            }
          else
            fd = intval;
          if (sh_validfd (fd) == 0)
            {
              builtin_error (_ ("%d: invalid file descriptor: %s"), fd,
                             strerror (errno));
              return EXECUTION_FAILURE;
            }
          break;
        case 'd':
          delim = *list_optarg;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  /* `read -t 0 var' tests whether input is available with select/FIONREAD,
     and fails if those are unavailable */
  if (have_timeout && tmsec == 0 && tmusec == 0)
#if 0
    return EXECUTION_FAILURE;
#else
    return input_avail (fd) ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
#endif

  /* Convenience: check early whether or not the first of possibly several
     variable names is a valid identifier, and bail early if so. */
#if defined(ARRAY_VARS)
    vflags = assoc_expand_once ? (VA_NOEXPAND | VA_ONEWORD) : 0;
  if (list && legal_identifier (list->word->word) == 0
      && valid_array_reference (list->word->word, vflags) == 0)
#else
  if (list && legal_identifier (list->word->word) == 0)
#endif
    {
      sh_invalidid (list->word->word);
      return EXECUTION_FAILURE;
    }

  /* If we're asked to ignore the delimiter, make sure we do. */
  if (ignore_delim)
    delim = (unsigned char)-1;

  /* IF IFS is unset, we use the default of " \t\n". */
  const char *ifs_chars = getifs ();
  if (ifs_chars == 0) /* XXX - shouldn't happen */
    ifs_chars = "";
  /* If we want to read exactly NCHARS chars, don't split on IFS */
  if (ignore_delim)
    ifs_chars = "";

  const char *ifs_char;
  for (skip_ctlesc = skip_ctlnul = 0, ifs_char = ifs_chars; *ifs_char;
       ifs_char++)
    skip_ctlesc |= *ifs_char == CTLESC, skip_ctlnul |= *ifs_char == CTLNUL;

  input_string = (char *)xmalloc (size = 112); /* XXX was 128 */
  input_string[0] = '\0';

  /* More input and options validation */
  if (nflag == 1 && nchars == 0)
    {
      retval = read (fd, &c, 0);
      retval = (retval >= 0) ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
      goto assign_vars; /* bail early if asked to read 0 chars */
    }

  /* $TMOUT, if set, is the default timeout for read. */
  char *e;
  if (have_timeout == 0 && (e = get_string_value ("TMOUT")))
    {
      code = uconvert (e, &ival, &uval, (char **)NULL);
      if (code == 0 || ival < 0 || uval < 0)
        tmsec = tmusec = 0;
      else
        {
          tmsec = ival;
          tmusec = uval;
        }
    }

  begin_unwind_frame ("read_builtin");

#if defined(BUFFERED_INPUT)
  if (interactive == 0 && default_buffered_input >= 0 && fd_is_bash_input (fd))
    sync_buffered_stream (default_buffered_input);
#endif

#if 1
  input_is_tty = isatty (fd);
#else
  input_is_tty = 1;
#endif
  if (input_is_tty == 0)
#ifndef __CYGWIN__
    input_is_pipe = (lseek (fd, 0L, SEEK_CUR) < 0) && (errno == ESPIPE);
#else
    input_is_pipe = 1;
#endif

  /* If the -p, -e or -s flags were given, but input is not coming from the
     terminal, turn them off. */
  if ((prompt || edit || silent) && input_is_tty == 0)
    {
      prompt = (char *)NULL;
#if defined(READLINE)
      itext = (char *)NULL;
#endif
      edit = silent = 0;
    }

#if defined(READLINE)
  if (edit)
    add_unwind_protect_ptr (xfree, rlbuf);
#endif

  pass_next = 0;  /* Non-zero signifies last char was backslash. */
  saw_escape = 0; /* Non-zero signifies that we saw an escape char */

  if (tmsec > 0 || tmusec > 0)
    {
      /* Turn off the timeout if stdin is a regular file (e.g. from
         input redirection). */
      if ((fstat (fd, &tsb) < 0) || S_ISREG (tsb.st_mode))
        tmsec = tmusec = 0;
    }

  if (tmsec > 0 || tmusec > 0)
    {
      code = setjmp_nosigs (alrmbuf);
      if (code)
        {
          sigalrm_seen = false;
          /* Tricky.  The top of the unwind-protect stack is the free of
             input_string.  We want to run all the rest and use input_string,
             so we have to save input_string temporarily, run the unwind-
             protects, then restore input_string so we can use it later */
          orig_input_string = 0;
          input_string[i] = '\0'; /* make sure it's terminated */
          if (i == 0)
            {
              t = (char *)xmalloc (1);
              t[0] = 0;
            }
          else
            t = savestring (input_string);

          run_unwind_frame ("read_builtin");
          input_string = t;
          retval = 128 + SIGALRM;
          goto assign_vars;
        }
      if (interactive_shell == 0)
        initialize_terminating_signals ();
      old_alrm = set_signal_handler (SIGALRM, sigalrm);
      add_unwind_protect (reset_alarm);
#if defined(READLINE)
      if (edit)
        {
          add_unwind_protect (reset_attempted_completion_function);
          add_unwind_protect (bashline_reset_event_hook);
        }
#endif
      falarm (tmsec, tmusec);
    }

  /* If we've been asked to read only NCHARS chars, or we're using some
     character other than newline to terminate the line, do the right
     thing to readline or the tty. */
  if (nchars > 0 || delim != '\n')
    {
#if defined(READLINE)
      if (edit)
        {
          if (nchars > 0)
            {
              unwind_protect_var (rl_num_chars_to_read);
              rl_num_chars_to_read = nchars;
            }
          if (delim != '\n')
            {
              set_eol_delim (delim);
              add_unwind_protect (reset_eol_delim);
            }
        }
      else
#endif
          if (input_is_tty)
        {
          /* ttsave() */
          termsave.fd = fd;
          ttgetattr (fd, &ttattrs);
          termsave.attrs = ttattrs;

          ttset = ttattrs;
          i = silent ? ttfd_cbreak (fd, &ttset) : ttfd_onechar (fd, &ttset);
          if (i < 0)
            sh_ttyerror (1);
          tty_modified = 1;
          add_unwind_protect_ptr (ttyrestore, &termsave);
          if (interactive_shell == 0)
            initialize_terminating_signals ();
        }
    }
  else if (silent) /* turn off echo but leave term in canonical mode */
    {
      /* ttsave (); */
      termsave.fd = fd;
      ttgetattr (fd, &ttattrs);
      termsave.attrs = ttattrs;

      ttset = ttattrs;
      i = ttfd_noecho (fd, &ttset); /* ttnoecho (); */
      if (i < 0)
        sh_ttyerror (1);

      tty_modified = 1;
      add_unwind_protect_ptr (ttyrestore, &termsave);
      if (interactive_shell == 0)
        initialize_terminating_signals ();
    }

#if defined(READLINE)
  save_instream = 0;
  if (edit && fd != 0)
    {
      if (bash_readline_initialized == 0)
        initialize_readline ();

      unwind_protect_var (rl_instream);
      save_instream = rl_instream;
      rl_instream = fdopen (fd, "r");
    }
#endif

  /* This *must* be the top unwind-protect on the stack, so the manipulation
     of the unwind-protect stack after the realloc() works right. */
  add_unwind_protect_ptr (xfree, input_string);

  CHECK_ALRM;
  if ((nchars > 0) && (input_is_tty == 0) && ignore_delim) /* read -N */
    unbuffered_read = 2;
  else if ((nchars > 0) || (delim != '\n') || input_is_pipe)
    unbuffered_read = 1;

  if (prompt && edit == 0)
    {
      fprintf (stderr, "%s", prompt);
      fflush (stderr);
    }

#if defined(__CYGWIN__) && defined(O_TEXT)
  setmode (0, O_TEXT);
#endif

  ps2 = 0;
  for (print_ps2 = eof = retval = 0;;)
    {
      CHECK_ALRM;

#if defined(READLINE)
      if (edit)
        {
          /* If we have a null delimiter, don't treat NULL as ending the line
           */
          if (rlbuf && rlbuf[rlind] == '\0' && delim != '\0')
            {
              free (rlbuf);
              rlbuf = (char *)0;
            }
          if (rlbuf == 0)
            {
              reading = 1;
              rlbuf = edit_line (prompt ? prompt : "", itext);
              reading = 0;
              rlind = 0;
            }
          if (rlbuf == 0)
            {
              eof = 1;
              break;
            }
          c = rlbuf[rlind++];
        }
      else
        {
#endif

          if (print_ps2)
            {
              if (ps2 == 0)
                ps2 = get_string_value ("PS2");
              fprintf (stderr, "%s", ps2 ? ps2 : "");
              fflush (stderr);
              print_ps2 = 0;
            }

          reading = 1;
          CHECK_ALRM;
          errno = 0;
          if (unbuffered_read == 2)
            retval = posixly_correct ? zreadintr (fd, &c, 1)
                                     : zreadn (fd, &c, nchars - nr);
          else if (unbuffered_read)
            retval
                = posixly_correct ? zreadintr (fd, &c, 1) : zread (fd, &c, 1);
          else
            retval = posixly_correct ? zreadcintr (fd, &c) : zreadc (fd, &c);
          reading = 0;

          if (retval <= 0)
            {
              int t;

              t = errno;
              if (retval < 0 && errno == EINTR)
                {
                  check_signals (); /* in case we didn't call zread via zreadc
                                     */
                  lastsig = LASTSIG ();
                  if (lastsig == 0)
                    lastsig = trapped_signal_received;
#if 0
	      run_pending_traps ();	/* because interrupt_immediately is not set */
#endif
                }
              else
                lastsig = 0;
              if (terminating_signal && tty_modified)
                ttyrestore (&termsave); /* fix terminal before exiting */
              CHECK_TERMSIG;
              eof = 1;
              errno = t; /* preserve it for the error message below */
              break;
            }

          QUIT; /* in case we didn't call check_signals() */
#if defined(READLINE)
        }
#endif

      if (retval <= 0) /* XXX shouldn't happen */
        CHECK_ALRM;

      /* XXX -- use i + mb_cur_max (at least 4) for multibyte/read_mbchar */
      if (i + (mb_cur_max > 4 ? mb_cur_max : 4) >= size)
        {
          char *t;
          t = (char *)xrealloc (input_string, size += 128);

          /* Only need to change unwind-protect if input_string changes */
          if (t != input_string)
            {
              input_string = t;
              remove_unwind_protect ();
              add_unwind_protect_ptr (xfree, input_string);
            }
        }

      /* If the next character is to be accepted verbatim, a backslash
         newline pair still disappears from the input. */
      if (pass_next)
        {
          pass_next = 0;
          if (c == '\n')
            {
              if (skip_ctlesc == 0 && i > 0)
                i--; /* back up over the CTLESC */
              if (interactive && input_is_tty && raw == 0)
                print_ps2 = 1;
            }
          else
            goto add_char;
          continue;
        }

      /* This may cause problems if IFS contains CTLESC */
      if (c == '\\' && raw == 0)
        {
          pass_next++;
          if (skip_ctlesc == 0)
            {
              saw_escape++;
              input_string[i++] = CTLESC;
            }
          continue;
        }

      if (ignore_delim == 0 && (unsigned char)c == delim)
        break;

      if (c == '\0' && delim != '\0')
        continue; /* skip NUL bytes in input */

      if ((skip_ctlesc == 0 && c == CTLESC)
          || (skip_ctlnul == 0 && c == CTLNUL))
        {
          saw_escape++;
          input_string[i++] = CTLESC;
        }

    add_char:
      input_string[i++] = c;
      CHECK_ALRM;

#if defined(HANDLE_MULTIBYTE)
      /* XXX - what if C == 127? Can DEL introduce a multibyte sequence? */
      if (mb_cur_max > 1 && is_basic (c) == 0)
        {
          input_string[i] = '\0'; /* for simplicity and debugging */
          /* If we got input from readline, grab the next multibyte char from
             rlbuf. */
#if defined(READLINE)
          if (edit)
            {
              size_t clen;
              clen = mbrlen (rlbuf + rlind - 1, mb_cur_max, (mbstate_t *)NULL);
              /* We only deal with valid multibyte sequences longer than one
                 byte. If we get anything else, we leave the one character
                 copied and move on to the next. */
              if ((int)clen > 1)
                {
                  memcpy (input_string + i, rlbuf + rlind, clen - 1);
                  i += clen - 1;
                  rlind += clen - 1;
                }
            }
          else
#endif
              if (locale_utf8locale == 0 || ((c & 0x80) != 0))
            i += read_mbchar (fd, input_string, i, c, unbuffered_read);
        }
#endif

      nr++;

      if (nchars > 0 && nr >= nchars)
        break;
    }
  input_string[i] = '\0';
  CHECK_ALRM;

#if defined(READLINE)
  if (edit)
    free (rlbuf);
#endif

  if (retval < 0)
    {
      t_errno = errno;
      if (errno != EINTR)
        builtin_error (_ ("read error: %d: %s"), fd, strerror (errno));
      run_unwind_frame ("read_builtin");
      return (t_errno != EINTR) ? EXECUTION_FAILURE : 128 + lastsig;
    }

  if (tmsec > 0 || tmusec > 0)
    reset_alarm ();

  if (nchars > 0 || delim != '\n')
    {
#if defined(READLINE)
      if (edit)
        {
          if (nchars > 0)
            rl_num_chars_to_read = 0;
          if (delim != '\n')
            reset_eol_delim ();
        }
      else
#endif
          if (input_is_tty)
        ttyrestore (&termsave);
    }
  else if (silent)
    ttyrestore (&termsave);

  if (unbuffered_read == 0)
    zsyncfd (fd);

#if defined(READLINE)
  if (save_instream)
    rl_instream = save_instream; /* can't portably free it */
#endif

  discard_unwind_frame ("read_builtin");

  retval = eof ? EXECUTION_FAILURE : EXECUTION_SUCCESS;

assign_vars:

#if defined(ARRAY_VARS)
  /* If -a was given, take the string read, break it into a list of words,
     an assign them to `arrayname' in turn. */
  if (arrayname)
    {
      if (legal_identifier (arrayname) == 0)
        {
          sh_invalidid (arrayname);
          free (input_string);
          return EXECUTION_FAILURE;
        }

      var = find_or_make_array_variable (arrayname, 1);
      if (var == 0)
        {
          free (input_string);
          return EXECUTION_FAILURE; /* readonly or noassign */
        }
      if (assoc_p (var))
        {
          builtin_error (_ ("%s: cannot convert associative to indexed array"),
                         arrayname);
          free (input_string);
          return EXECUTION_FAILURE; /* existing associative array */
        }
      else if (invisible_p (var))
        VUNSETATTR (var, att_invisible);
      array_flush (array_cell (var));

      alist = list_string (input_string, ifs_chars, 0);
      if (alist)
        {
          if (saw_escape)
            dequote_list (alist);
          else
            word_list_remove_quoted_nulls (alist);
          assign_array_var_from_word_list (var, alist, 0);
          dispose_words (alist);
        }
      free (input_string);
      return retval;
    }
#endif /* ARRAY_VARS */

  /* If there are no variables, save the text of the line read to the
     variable $REPLY.  ksh93 strips leading and trailing IFS whitespace,
     so that `read x ; echo "$x"' and `read ; echo "$REPLY"' behave the
     same way, but I believe that the difference in behaviors is useful
     enough to not do it.  Without the bash behavior, there is no way
     to read a line completely without interpretation or modification
     unless you mess with $IFS (e.g., setting it to the empty string).
     If you disagree, change the occurrences of `#if 0' to `#if 1' below. */
  if (list == 0)
    {
#if 0
      orig_input_string = input_string;
      for (t = input_string; ifs_chars && *ifs_chars && spctabnl(*t) && isifs(*t); t++)
	;
      input_string = t;
      input_string = strip_trailing_ifs_whitespace (input_string, ifs_chars, saw_escape);
#endif

      if (saw_escape)
        {
          t = dequote_string (input_string);
          var = bind_variable ("REPLY", t, 0);
          free (t);
        }
      else
        var = bind_variable ("REPLY", input_string, 0);
      if (var == 0 || readonly_p (var) || noassign_p (var))
        retval = EXECUTION_FAILURE;
      else
        VUNSETATTR (var, att_invisible);

      free (input_string);
      return retval;
    }

  /* This code implements the Posix.2 spec for splitting the words
     read and assigning them to variables. */
  orig_input_string = input_string;

  /* Remove IFS white space at the beginning of the input string.  If
     $IFS is null, no field splitting is performed. */
  for (t = input_string;
       ifs_chars && *ifs_chars && spctabnl (*t) && isifs (*t); t++)
    ;
  input_string = t;
  for (; list->next; list = (WORD_LIST *)list->next)
    {
      char *varname = list->word->word;
#if defined(ARRAY_VARS)
      if (legal_identifier (varname) == 0
          && valid_array_reference (varname, vflags) == 0)
#else
      if (legal_identifier (varname) == 0)
#endif
        {
          sh_invalidid (varname);
          free (orig_input_string);
          return EXECUTION_FAILURE;
        }

      /* If there are more variables than words read from the input,
         the remaining variables are set to the empty string. */
      if (*input_string)
        {
          /* This call updates INPUT_STRING. */
          t = get_word_from_string (&input_string, ifs_chars, &e);
          if (t)
            *e = '\0';
          /* Don't bother to remove the CTLESC unless we added one
             somewhere while reading the string. */
          if (t && saw_escape)
            {
              t1 = dequote_string (t);
              var = bind_read_variable (varname, t1);
              free (t1);
            }
          else
            var = bind_read_variable (varname, t ? t : "");
        }
      else
        {
          t = (char *)0;
          var = bind_read_variable (varname, "");
        }

      FREE (t);
      if (var == 0)
        {
          free (orig_input_string);
          return EXECUTION_FAILURE;
        }

      stupidly_hack_special_variables (varname);
      VUNSETATTR (var, att_invisible);
    }

    /* Now assign the rest of the line to the last variable argument. */
#if defined(ARRAY_VARS)
  if (legal_identifier (list->word->word) == 0
      && valid_array_reference (list->word->word, vflags) == 0)
#else
  if (legal_identifier (list->word->word) == 0)
#endif
    {
      sh_invalidid (list->word->word);
      free (orig_input_string);
      return EXECUTION_FAILURE;
    }

#if 0
  /* This has to be done this way rather than using string_list
     and list_string because Posix.2 says that the last variable gets the
     remaining words and their intervening separators. */
  input_string = strip_trailing_ifs_whitespace (input_string, ifs_chars, saw_escape);
#else
  /* Check whether or not the number of fields is exactly the same as the
     number of variables. */
  tofree = NULL;
  if (*input_string)
    {
      t1 = input_string;
      t = get_word_from_string (&input_string, ifs_chars, &e);
      if (*input_string == 0)
        tofree = input_string = t;
      else
        {
          input_string
              = strip_trailing_ifs_whitespace (t1, ifs_chars, saw_escape);
          tofree = t;
        }
    }
#endif

  if (saw_escape && input_string && *input_string)
    {
      t = dequote_string (input_string);
      var = bind_read_variable (list->word->word, t);
      free (t);
    }
  else
    var = bind_read_variable (list->word->word,
                              input_string ? input_string : (char *)"");

  if (var)
    {
      stupidly_hack_special_variables (list->word->word);
      VUNSETATTR (var, att_invisible);
    }
  else
    retval = EXECUTION_FAILURE;

  FREE (tofree);
  free (orig_input_string);

  return retval;
}

static SHELL_VAR *
bind_read_variable (const char *name, const char *value)
{
  SHELL_VAR *v;

  v = builtin_bind_variable (name, value, 0);
  return (v == 0
              ? v
              : ((readonly_p (v) || noassign_p (v)) ? (SHELL_VAR *)NULL : v));
}

#if defined(HANDLE_MULTIBYTE)
static int
read_mbchar (int fd, char *string, int ind, int ch, int unbuffered)
{
  char mbchar[MB_LEN_MAX + 1];
  int i, n, r;
  char c;
  size_t ret;
  mbstate_t ps, ps_back;
  wchar_t wc;

  memset (&ps, '\0', sizeof (mbstate_t));
  memset (&ps_back, '\0', sizeof (mbstate_t));

  mbchar[0] = ch;
  i = 1;
  for (n = 0; n <= MB_LEN_MAX; n++)
    {
      ps_back = ps;
      ret = mbrtowc (&wc, mbchar, i, &ps);
      if (ret == (size_t)-2)
        {
          ps = ps_back;

          /* We don't want to be interrupted during a multibyte char read */
          if (unbuffered == 2)
            r = zreadn (fd, &c, 1);
          else if (unbuffered)
            r = zread (fd, &c, 1);
          else
            r = zreadc (fd, &c);
          if (r <= 0)
            goto mbchar_return;
          mbchar[i++] = c;
          continue;
        }
      else if (ret == (size_t)-1 || ret == (size_t)0 || ret > (size_t)0)
        break;
    }

mbchar_return:
  if (i > 1) /* read a multibyte char */
    /* mbchar[0] is already string[ind-1] */
    for (r = 1; r < i; r++)
      string[ind + r - 1] = mbchar[r];
  return i - 1;
}
#endif

static void
ttyrestore (void *arg)
{
  struct ttsave *ttp = (struct ttsave *)arg;
  ttsetattr (ttp->fd, &(ttp->attrs));
  tty_modified = 0;
}

void
read_tty_cleanup ()
{
  if (tty_modified)
    ttyrestore (&termsave);
}

int
read_tty_modified ()
{
  return tty_modified;
}

#if defined(READLINE)
static rl_completion_func_t *old_attempted_completion_function = 0;
static rl_hook_func_t *old_startup_hook;
static char *deftext;

static void
reset_attempted_completion_function ()
{
  if (rl_attempted_completion_function == 0
      && old_attempted_completion_function)
    rl_attempted_completion_function = old_attempted_completion_function;
}

static int
set_itext ()
{
  int r1, r2;

  r1 = r2 = 0;
  if (old_startup_hook)
    r1 = (*old_startup_hook) ();
  if (deftext)
    {
      r2 = rl_insert_text (deftext);
      deftext = (char *)NULL;
      rl_startup_hook = old_startup_hook;
      old_startup_hook = (rl_hook_func_t *)NULL;
    }
  return r1 || r2;
}

static char *
edit_line (const char *p, char *itext)
{
  char *ret;
  int len;

  if (bash_readline_initialized == 0)
    initialize_readline ();

  old_attempted_completion_function = rl_attempted_completion_function;
  rl_attempted_completion_function = (rl_completion_func_t *)NULL;
  bashline_set_event_hook ();
  if (itext)
    {
      old_startup_hook = rl_startup_hook;
      rl_startup_hook = set_itext;
      deftext = itext;
    }

  ret = readline (p);

  rl_attempted_completion_function = old_attempted_completion_function;
  old_attempted_completion_function = (rl_completion_func_t *)NULL;
  bashline_reset_event_hook ();

  if (ret == 0)
    return ret;
  len = strlen (ret);
  ret = (char *)xrealloc (ret, len + 2);
  ret[len++] = delim;
  ret[len] = '\0';
  return ret;
}

static int old_delim_ctype;
static rl_command_func_t *old_delim_func;
static int old_newline_ctype;
static rl_command_func_t *old_newline_func;

static unsigned char delim_char;

static void
set_eol_delim (int c)
{
  Keymap cmap;

  if (bash_readline_initialized == 0)
    initialize_readline ();
  cmap = rl_get_keymap ();

  /* Save the old delimiter char binding */
  old_newline_ctype = cmap[RETURN].type;
  old_newline_func = cmap[RETURN].function;
  old_delim_ctype = cmap[c].type;
  old_delim_func = cmap[c].function;

  /* Change newline to self-insert */
  cmap[RETURN].type = ISFUNC;
  cmap[RETURN].function = rl_insert;

  /* Bind the delimiter character to accept-line. */
  cmap[c].type = ISFUNC;
  cmap[c].function = rl_newline;

  delim_char = c;
}

static void
reset_eol_delim ()
{
  Keymap cmap;

  cmap = rl_get_keymap ();

  cmap[RETURN].type = old_newline_ctype;
  cmap[RETURN].function = old_newline_func;

  cmap[delim_char].type = old_delim_ctype;
  cmap[delim_char].function = old_delim_func;
}
#endif

} // namespace bash
