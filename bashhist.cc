/* bashhist.cc -- bash interface to the GNU history library. */

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

#include "config.h"

#if defined(HISTORY)

#include "flags.hh"
#include "input.hh"
#include "parser.hh"
#include "pathexp.hh" /* for the struct ignorevar stuff */
#include "shell.hh"

#include "builtins/common.hh"

#include <glob.h>

#if defined(SYSLOG_HISTORY)
#include <syslog.h>
#endif

#ifndef HISTSIZE_DEFAULT
#define HISTSIZE_DEFAULT "500"
#endif

namespace bash
{

#if 0
// this needs to migrate into Shell class

static int histignore_item_func (struct ign *);
static int check_history_control (const char *);
static void hc_erasedups (const char *);
static void really_add_history (const char *);

static struct ignorevar histignore =
{
  "HISTIGNORE",
  (struct ign *)0,
  0,
  (char *)0,
  (sh_iv_item_func_t *)histignore_item_func,
};

#define HIGN_EXPAND 0x01

/* Declarations of bash history variables. */
/* Non-zero means to remember lines typed to the shell on the history
   list.  This is different than the user-controlled behaviour; this
   becomes zero when we read lines from a file, for example. */
bool remember_on_history = false;
char enable_history_list = -1;	/* value for `set -o history' */

/* The number of lines that Bash has added to this history session.  The
   difference between the number of the top element in the history list
   (offset from history_base) and the number of lines in the history file.
   Appending this session's history to the history file resets this to 0. */
int history_lines_this_session;

/* The number of lines that Bash has read from the history file. */
int history_lines_in_file;

#if defined(BANG_HISTORY)
/* Non-zero means do no history expansion on this line, regardless
   of what history_expansion says. */
bool history_expansion_inhibited;
/* If non-zero, double quotes can quote the history expansion character. */
bool double_quotes_inhibit_history_expansion = false;
#endif

/* With the old default, every line was saved in the history individually.
   I.e., if the user enters:
	bash$ for i in a b c
	> do
	> echo $i
	> done
   Each line will be individually saved in the history.
	bash$ history
	10  for i in a b c
	11  do
	12  echo $i
	13  done
	14  history
   If the variable command_oriented_history is set, multiple lines
   which form one command will be saved as one history entry.
	bash$ for i in a b c
	> do
	> echo $i
	> done
	bash$ history
	10  for i in a b c
    do
    echo $i
    done
	11  history
   The user can then recall the whole command all at once instead
   of just being able to recall one line at a time.

   This is now enabled by default.
   */
char command_oriented_history = 1;

/* Set to 1 if the first line of a possibly-multi-line command was saved
   in the history list.  Managed by maybe_add_history(), but global so
   the history-manipluating builtins can see it. */
bool current_command_first_line_saved = false;

/* Set to the number of the most recent line of a possibly-multi-line command
   that contains a shell comment.  Used by bash_add_history() to determine
   whether to add a newline or a semicolon. */
bool current_command_line_comment = false;

/* Non-zero means to store newlines in the history list when using
   command_oriented_history rather than trying to use semicolons. */
char literal_history;

/* Non-zero means to append the history to the history file at shell
   exit, even if the history has been stifled. */
char force_append_history;

/* A nit for picking at history saving.  Flags have the following values:

   Value == 0 means save all lines parsed by the shell on the history.
   Value & HC_IGNSPACE means save all lines that do not start with a space.
   Value & HC_IGNDUPS means save all lines that do not match the last
   line saved.
   Value & HC_ERASEDUPS means to remove all other matching lines from the
   history list before saving the latest line. */
int history_control;

/* Set to 1 if the last command was added to the history list successfully
   as a separate history entry; set to 0 if the line was ignored or added
   to a previous entry as part of command-oriented-history processing. */
bool hist_last_line_added;

/* Set to 1 if builtins/history.def:push_history added the last history
   entry. */
bool hist_last_line_pushed;

#if defined(READLINE)
/* If non-zero, and readline is being used, the user is offered the
   chance to re-edit a failed history expansion. */
bool history_reediting;

/* If non-zero, and readline is being used, don't directly execute a
   line with history substitution.  Reload it into the editing buffer
   instead and let the user further edit and confirm with a newline. */
char hist_verify;

#endif /* READLINE */

/* Non-zero means to not save function definitions in the history list. */
char dont_save_function_defs;

#if defined(BANG_HISTORY)
static int bash_history_inhibit_expansion (char *, int);	/* rl_linebuf_func_t */
#endif
#if defined(READLINE)
static void re_edit (const char *);
#endif
static bool history_expansion_p (const char *);
static int shell_comment (const char *);	/* returns 0, 1, or 2 */
static bool should_expand (const char *);
static HIST_ENTRY *last_history_entry (void);
static char *expand_histignore_pattern (const char *);
static bool history_should_ignore (const char *);
#endif

#if defined(BANG_HISTORY)
/* Is the history expansion starting at string[i] one that should not
   be expanded? */
bool
Shell::bash_history_inhibit_expansion (const char *string, int i)
{
  int t, si;
  char hx[2];

  hx[0] = history_expansion_char;
  hx[1] = '\0';

  /* The shell uses ! as a pattern negation character in globbing [...]
     expressions, so let those pass without expansion. */
  if (i > 0 && (string[i - 1] == '[') && member (']', string + i + 1))
    return true;
  /* The shell uses ! as the indirect expansion character, so let those
     expansions pass as well. */
  else if (i > 1 && string[i - 1] == '{' && string[i - 2] == '$'
           && member ('}', string + i + 1))
    return true;
  /* The shell uses $! as a defined parameter expansion. */
  else if (i > 1 && string[i - 1] == '$' && string[i] == '!')
    return true;
#if defined(EXTENDED_GLOB)
  else if (extended_glob && i > 1 && string[i + 1] == '('
           && member (')', string + i + 2))
    return true;
#endif

  si = 0;
  /* If we're supposed to be in single-quoted string, skip over the
     single-quoted part and then look at what's left. */
  if (history_quoting_state == '\'')
    {
      si = skip_to_delim (string, 0, "'", SD_NOJMP | SD_HISTEXP);
      if (string[si] == 0 || si >= i)
        return true;
      si++;
    }

  /* Make sure the history expansion should not be skipped by quoting or
     command/process substitution. */
  if ((t = skip_to_histexp (string, si, hx, SD_NOJMP | SD_HISTEXP)) > 0)
    {
      /* Skip instances of history expansion appearing on the line before
         this one. */
      while (t < i)
        {
          t = skip_to_histexp (string, t + 1, hx, SD_NOJMP | SD_HISTEXP);
          if (t <= 0)
            return false;
        }
      return t > i;
    }
  else
    return false;
}
#endif

void
bash_initialize_history ()
{
  history_quotes_inhibit_expansion = 1;
  history_search_delimiter_chars = ";&()|<>";
#if defined(BANG_HISTORY)
  history_inhibit_expansion_function = bash_history_inhibit_expansion;
  sv_histchars ("histchars");
#endif
}

void
bash_history_reinit (bool interact)
{
#if defined(BANG_HISTORY)
  history_expansion = (!interact) ? histexp_flag : HISTEXPAND_DEFAULT;
  history_expansion_inhibited
      = (!interact) ? (!histexp_flag)
                    : false; /* changed in bash_history_enable() */
  history_inhibit_expansion_function = bash_history_inhibit_expansion;
#endif
  remember_on_history = enable_history_list;
}

void
bash_history_disable ()
{
  remember_on_history = false;
#if defined(BANG_HISTORY)
  history_expansion_inhibited = true;
#endif
}

void
bash_history_enable ()
{
  remember_on_history = enable_history_list = true;
#if defined(BANG_HISTORY)
  history_expansion_inhibited = false;
  history_inhibit_expansion_function = bash_history_inhibit_expansion;
#endif
  sv_history_control ("HISTCONTROL");
  sv_histignore ("HISTIGNORE");
}

/* Load the history list from the history file. */
void
load_history ()
{
  char *hf;

  /* Truncate history file for interactive shells which desire it.
     Note that the history file is automatically truncated to the
     size of HISTSIZE if the user does not explicitly set the size
     differently. */
  set_if_not ("HISTSIZE", HISTSIZE_DEFAULT);
  sv_histsize ("HISTSIZE");

  set_if_not ("HISTFILESIZE", get_string_value ("HISTSIZE"));
  sv_histsize ("HISTFILESIZE");

  /* Read the history in HISTFILE into the history list. */
  hf = get_string_value ("HISTFILE");

  if (hf && *hf && file_exists (hf))
    {
      read_history (hf);
      /* We have read all of the lines from the history file, even if we
         read more lines than $HISTSIZE.  Remember the total number of lines
         we read so we don't count the last N lines as new over and over
         again. */
      history_lines_in_file = history_lines_read_from_file;
      using_history ();
      /* history_lines_in_file = where_history () + history_base - 1; */
    }
}

void
bash_clear_history ()
{
  clear_history ();
  history_lines_this_session = 0;
  /* XXX - reset history_lines_read_from_file? */
}

/* Delete and free the history list entry at offset I. */
bool
bash_delete_histent (int i)
{
  HIST_ENTRY *discard;

  discard = remove_history (i);
  if (discard)
    {
      free_history_entry (discard);
      history_lines_this_session--;
    }
  return discard != 0;
}

bool
bash_delete_history_range (int first, int last)
{
  int i;
  HIST_ENTRY **discard_list = remove_history_range (first, last);

  for (i = 0; discard_list && discard_list[i]; i++)
    free_history_entry (discard_list[i]);

  history_lines_this_session -= i;

  free (discard_list);

  return true;
}

bool
bash_delete_last_history ()
{
  HIST_ENTRY **hlist = history_list ();
  if (hlist == NULL)
    return 0;

  int i;
  for (i = 0; hlist[i]; i++)
    ;
  i--;

  /* History_get () takes a parameter that must be offset by history_base. */
  HIST_ENTRY *histent = history_get (history_base + i); /* Don't free this */
  if (histent == NULL)
    return false;

  bool r = bash_delete_histent (i);

  if (where_history () > history_length)
    history_set_pos (history_length);

  return r;
}

int
maybe_append_history (const char *filename)
{
  int fd, result, histlen;
  struct stat buf;

  result = EXECUTION_SUCCESS;
  if (history_lines_this_session > 0)
    {
      /* If the filename was supplied, then create it if necessary. */
      if (stat (filename, &buf) == -1 && errno == ENOENT)
        {
          fd = open (filename, O_WRONLY | O_CREAT, 0600);
          if (fd < 0)
            {
              builtin_error (_ ("%s: cannot create: %s"), filename,
                             strerror (errno));
              return EXECUTION_FAILURE;
            }
          close (fd);
        }
      /* cap the number of lines we write at the length of the history list */
      histlen = where_history ();
      if (histlen > 0 && history_lines_this_session > histlen)
        history_lines_this_session = histlen; /* reset below anyway */
      result = append_history (history_lines_this_session, filename);
      /* Pretend we already read these lines from the file because we just
         added them */
      history_lines_in_file += history_lines_this_session;
      history_lines_this_session = 0;
    }
  else
    history_lines_this_session = 0; /* reset if > where_history() */

  return result;
}

/* If this is an interactive shell, then append the lines executed
   this session to the history file. */
int
maybe_save_shell_history ()
{
  int result;
  char *hf;

  result = 0;
  if (history_lines_this_session > 0)
    {
      hf = get_string_value ("HISTFILE");

      if (hf && *hf)
        {
          /* If the file doesn't exist, then create it. */
          if (file_exists (hf) == 0)
            {
              int file;
              file = open (hf, O_CREAT | O_TRUNC | O_WRONLY, 0600);
              if (file != -1)
                close (file);
            }

          /* Now actually append the lines if the history hasn't been
             stifled.  If the history has been stifled, rewrite the
             history file. */
          using_history ();
          if (history_lines_this_session <= where_history ()
              || force_append_history)
            {
              result = append_history (history_lines_this_session, hf);
              history_lines_in_file += history_lines_this_session;
            }
          else
            {
              result = write_history (hf);
              history_lines_in_file = history_lines_written_to_file;
              /* history_lines_in_file = where_history () + history_base - 1;
               */
            }
          history_lines_this_session = 0;

          sv_histsize ("HISTFILESIZE");
        }
    }
  return result;
}

#if defined(READLINE)
/* Tell readline () that we have some text for it to edit. */
static void
re_edit (const char *text)
{
  if (bash_input.type == st_stdin)
    bash_re_edit (text);
}
#endif /* READLINE */

/* Return 1 if this line needs history expansion. */
static bool
history_expansion_p (const char *line)
{
  for (const char *s = line; *s; s++)
    if (*s == history_expansion_char || *s == history_subst_char)
      return true;
  return false;
}

/* Do pre-processing on LINE.  If PRINT_CHANGES is non-zero, then
   print the results of expanding the line if there were any changes.
   If there is an error, return NULL, otherwise the expanded line is
   returned.  If ADDIT is non-zero the line is added to the history
   list after history expansion.  ADDIT is just a suggestion;
   REMEMBER_ON_HISTORY can veto, and does.
   Right now this does history expansion. */
char *
pre_process_line (char *line, bool print_changes, bool addit)
{
  char *history_value;
  char *return_value = line;
  int expanded = 0;

#if defined(BANG_HISTORY)
  /* History expand the line.  If this results in no errors, then
     add that line to the history if ADDIT is non-zero. */
  if (!history_expansion_inhibited && history_expansion
      && history_expansion_p (line))
    {
      int old_len;

      /* If we are expanding the second or later line of a multi-line
         command, decrease history_length so references to history expansions
         in these lines refer to the previous history entry and not the
         current command. */
      old_len = history_length;
      if (history_length > 0 && command_oriented_history
          && current_command_first_line_saved
          && current_command_line_count > 1)
        history_length--;
      expanded = history_expand (line, &history_value);
      if (history_length >= 0 && command_oriented_history
          && current_command_first_line_saved
          && current_command_line_count > 1)
        history_length = old_len;

      if (expanded)
        {
          if (print_changes)
            {
              if (expanded < 0)
                internal_error ("%s", history_value);
#if defined(READLINE)
              else if (!hist_verify || expanded == 2)
#else
              else
#endif
                fprintf (stderr, "%s\n", history_value);
            }

          /* If there was an error, return NULL. */
          if (expanded < 0 || expanded == 2) /* 2 == print only */
            {
#if defined(READLINE)
              if (expanded == 2 && rl_dispatching == 0 && *history_value)
#else
              if (expanded == 2 && *history_value)
#endif /* !READLINE */
                maybe_add_history (history_value);

              free (history_value);

#if defined(READLINE)
              /* New hack.  We can allow the user to edit the
                 failed history expansion. */
              if (history_reediting && expanded < 0 && rl_done)
                re_edit (line);
#endif /* READLINE */
              return (char *)NULL;
            }

#if defined(READLINE)
          if (hist_verify && expanded == 1)
            {
              re_edit (history_value);
              free (history_value);
              return (char *)NULL;
            }
#endif
        }

      /* Let other expansions know that return_value can be free'ed,
         and that a line has been added to the history list.  Note
         that we only add lines that have something in them. */
      expanded = 1;
      return_value = history_value;
    }
#endif /* BANG_HISTORY */

  if (addit && remember_on_history && *return_value)
    maybe_add_history (return_value);

#if 0
  if (expanded == 0)
    return_value = savestring (line);
#endif

  return return_value;
}

/* Return 1 if the first non-whitespace character in LINE is a `#', indicating
   that the line is a shell comment.  Return 2 if there is a comment after the
   first non-whitespace character. Return 0 if the line does not contain a
   comment. */
static int
shell_comment (const char *line)
{
  const char *p;
  int n;

  if (dstack.delimiter_depth != 0 || (parser_state & PST_HEREDOC))
    return 0;
  if (line == 0)
    return 0;
  for (p = line; p && *p && whitespace (*p); p++)
    ;
  if (p && *p == '#')
    return 1;
  n = skip_to_delim (line, p - line, "#",
                     (SD_NOJMP | SD_GLOB | SD_EXTGLOB | SD_COMPLETE));
  return (line[n] == '#') ? 2 : 0;
}

/* Check LINE against what HISTCONTROL says to do.  Returns 1 if the line
   should be saved; 0 if it should be discarded. */
static int
check_history_control (char *line)
{
  if (history_control == 0)
    return true;

  /* ignorespace or ignoreboth */
  if ((history_control & HC_IGNSPACE) && *line == ' ')
    return false;

  /* ignoredups or ignoreboth */
  if (history_control & HC_IGNDUPS)
    {
      using_history ();
      HIST_ENTRY *temp = previous_history ();

      bool r = (temp == 0 || STREQ (temp->line, line) == 0);

      using_history ();

      return r;
    }

  return true;
}

/* Remove all entries matching LINE from the history list.  Triggered when
   HISTCONTROL includes `erasedups'. */
static void
hc_erasedups (char *line)
{
  HIST_ENTRY *temp;
  int r;

  using_history ();
  while ((temp = previous_history ()))
    {
      if (STREQ (temp->line, line))
        {
          r = where_history ();
          temp = remove_history (r);
          if (temp)
            free_history_entry (temp);
        }
    }
  using_history ();
}

/* Add LINE to the history list, handling possibly multi-line compound
   commands.  We note whether or not we save the first line of each command
   (which is usually the entire command and history entry), and don't add
   the second and subsequent lines of a multi-line compound command if we
   didn't save the first line.  We don't usually save shell comment lines in
   compound commands in the history, because they could have the effect of
   commenting out the rest of the command when the entire command is saved as
   a single history entry (when COMMAND_ORIENTED_HISTORY is enabled).  If
   LITERAL_HISTORY is set, we're saving lines in the history with embedded
   newlines, so it's OK to save comment lines.  If we're collecting the body
   of a here-document, we should act as if literal_history is enabled, because
   we want to save the entire contents of the here-document as it was
   entered.  We also make sure to save multiple-line quoted strings or other
   constructs. */
void
maybe_add_history (char *line)
{
  int is_comment;

  hist_last_line_added = 0;
  is_comment = shell_comment (line);

  /* Don't use the value of history_control to affect the second
     and subsequent lines of a multi-line command (old code did
     this only when command_oriented_history is enabled). */
  if (current_command_line_count > 1)
    {
      if (current_command_first_line_saved
          && ((parser_state & PST_HEREDOC) || literal_history
              || dstack.delimiter_depth != 0 || is_comment != 1))
        bash_add_history (line);
      current_command_line_comment
          = is_comment ? current_command_line_count : -2;
      return;
    }

  /* This is the first line of a (possible multi-line) command.  Note whether
     or not we should save the first line and remember it. */
  current_command_line_comment = is_comment ? current_command_line_count : -2;
  current_command_first_line_saved = check_add_history (line, 0);
}

/* Just check LINE against HISTCONTROL and HISTIGNORE and add it to the
   history if it's OK.  Used by `history -s' as well as maybe_add_history().
   Returns 1 if the line was saved in the history, 0 otherwise. */
bool
check_add_history (char *line, bool force)
{
  if (check_history_control (line) && !(history_should_ignore (line)))
    {
      /* We're committed to saving the line.  If the user has requested it,
         remove other matching lines from the history. */
      if (history_control & HC_ERASEDUPS)
        hc_erasedups (line);

      if (force)
        {
          really_add_history (line);
          using_history ();
        }
      else
        bash_add_history (line);
      return true;
    }
  return false;
}

#if defined(SYSLOG_HISTORY)
#define SYSLOG_MAXMSG 1024
#define SYSLOG_MAXLEN SYSLOG_MAXMSG
#define SYSLOG_MAXHDR 256

#ifndef OPENLOG_OPTS
#define OPENLOG_OPTS 0
#endif

#if defined(SYSLOG_SHOPT)
int syslog_history = SYSLOG_SHOPT;
#else
int syslog_history = 1;
#endif

void
bash_syslog_history (const char *line)
{
  char trunc[SYSLOG_MAXLEN], *msg;
  char loghdr[SYSLOG_MAXHDR];
  char seqbuf[32], *seqnum;
  int hdrlen, msglen, seqlen, chunks, i;
  static int first = 1;

  if (first)
    {
      openlog (shell_name, OPENLOG_OPTS, SYSLOG_FACILITY);
      first = 0;
    }

  hdrlen = snprintf (loghdr, sizeof (loghdr), "HISTORY: PID=%d UID=%d",
                     getpid (), current_user.uid);
  msglen = strlen (line);

  if ((msglen + hdrlen + 1) < SYSLOG_MAXLEN)
    syslog (SYSLOG_FACILITY | SYSLOG_LEVEL, "%s %s", loghdr, line);
  else
    {
      chunks = ((msglen + hdrlen) / SYSLOG_MAXLEN) + 1;
      for (msg = line, i = 0; i < chunks; i++)
        {
          seqnum = inttostr (i + 1, seqbuf, sizeof (seqbuf));
          seqlen = STRLEN (seqnum);

          /* 7 == "(seq=) " */
          strncpy (trunc, msg, SYSLOG_MAXLEN - hdrlen - seqlen - 7 - 1);
          trunc[SYSLOG_MAXLEN - 1] = '\0';
          syslog (SYSLOG_FACILITY | SYSLOG_LEVEL, "%s (seq=%s) %s", loghdr,
                  seqnum, trunc);
          msg += SYSLOG_MAXLEN - hdrlen - seqlen - 8;
        }
    }
}
#endif

/* Add a line to the history list.
   The variable COMMAND_ORIENTED_HISTORY controls the style of history
   remembering;  when non-zero, and LINE is not the first line of a
   complete parser construct, append LINE to the last history line instead
   of adding it as a new line. */
void
bash_add_history (const char *line)
{
  int offset, curlen;
  bool add_it, is_comment;
  HIST_ENTRY *current, *old;
  const char *chars_to_add;

  add_it = true;
  if (command_oriented_history && current_command_line_count > 1)
    {
      is_comment = shell_comment (line);

      /* The second and subsequent lines of a here document have the trailing
         newline preserved.  We don't want to add extra newlines here, but we
         do want to add one after the first line (which is the command that
         contains the here-doc specifier).  parse.y:history_delimiting_chars()
         does the right thing to take care of this for us.  We don't want to
         add extra newlines if the user chooses to enable literal_history,
         so we have to duplicate some of what that function does here. */
      /* If we're in a here document and past the first line,
                (current_command_line_count > 2)
         don't add a newline here. This will also take care of the
         literal_history case if the other conditions are met. */
      if ((parser_state & PST_HEREDOC) && current_command_line_count > 2
          && line[strlen (line) - 1] == '\n')
        chars_to_add = "";
      else if (current_command_line_count == current_command_line_comment + 1)
        chars_to_add = "\n";
      else if (literal_history)
        chars_to_add = "\n";
      else
        chars_to_add = history_delimiting_chars (line);

      using_history ();
      current = previous_history ();

      current_command_line_comment
          = is_comment ? current_command_line_count : -2;

      if (current)
        {
          /* If the previous line ended with an escaped newline (escaped
             with backslash, but otherwise unquoted), then remove the quoted
             newline, since that is what happens when the line is parsed. */
          curlen = strlen (current->line);

          if (dstack.delimiter_depth == 0 && current->line[curlen - 1] == '\\'
              && current->line[curlen - 2] != '\\')
            {
              current->line[curlen - 1] = '\0';
              curlen--;
              chars_to_add = "";
            }

          /* If we're not in some kind of quoted construct, the current history
             entry ends with a newline, and we're going to add a semicolon,
             don't.  In some cases, it results in a syntax error (e.g., before
             a close brace), and it should not be needed. */
          if (dstack.delimiter_depth == 0 && current->line[curlen - 1] == '\n'
              && *chars_to_add == ';')
            chars_to_add++;

          char *new_line = (char *)xmalloc (1 + curlen + strlen (line)
                                            + strlen (chars_to_add));
          sprintf (new_line, "%s%s%s", current->line, chars_to_add, line);
          offset = where_history ();
          old = replace_history_entry (offset, new_line, current->data);
          free (new_line);

          if (old)
            free_history_entry (old);

          add_it = false;
        }
    }

  if (add_it && history_is_stifled () && history_length == 0
      && history_length == history_max_entries)
    add_it = false;

  if (add_it)
    really_add_history (line);

#if defined(SYSLOG_HISTORY)
  if (syslog_history)
    bash_syslog_history (line);
#endif

  using_history ();
}

static void
really_add_history (const char *line)
{
  hist_last_line_added = true;
  hist_last_line_pushed = false;
  add_history (line);
  history_lines_this_session++;
}

static bool
should_expand (const char *s)
{
  const char *p;

  for (p = s; p && *p; p++)
    {
      if (*p == '\\')
        p++;
      else if (*p == '&')
        return true;
    }
  return false;
}

static int
histignore_item_func (struct ign *ign)
{
  if (should_expand (ign->val))
    ign->flags |= HIGN_EXPAND;
  return 0;
}

void
setup_history_ignore ()
{
  setup_ignore_patterns (&histignore);
}

static HIST_ENTRY *
last_history_entry ()
{
  HIST_ENTRY *he;

  using_history ();
  he = previous_history ();
  using_history ();
  return he;
}

char *
last_history_line ()
{
  HIST_ENTRY *he;

  he = last_history_entry ();
  if (he == 0)
    return (char *)NULL;
  return he->line;
}

static char *
expand_histignore_pattern (const char *pat)
{
  HIST_ENTRY *phe;
  char *ret;

  phe = last_history_entry ();

  if (phe == (HIST_ENTRY *)0)
    return savestring (pat);

  ret = strcreplace (pat, '&', phe->line, 1);

  return ret;
}

/* Return 1 if we should not put LINE into the history according to the
   patterns in HISTIGNORE. */
static bool
history_should_ignore (const char *line)
{
  int i;
  bool match;

  if (histignore.num_ignores == 0)
    return 0;

  for (i = match = 0; i < histignore.num_ignores; i++)
    {
      char *npat;
      if (histignore.ignores[i].flags & HIGN_EXPAND)
        npat = expand_histignore_pattern (histignore.ignores[i].val);
      else
        npat = histignore.ignores[i].val;

      match = (strmatch (npat, line, FNMATCH_EXTFLAG) != FNM_NOMATCH);

      if (histignore.ignores[i].flags & HIGN_EXPAND)
        free (npat);

      if (match)
        break;
    }

  return match;
}

} // namespace bash

#endif /* HISTORY */
