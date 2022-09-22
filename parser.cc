/* parser.cc - Parser code previously in parse.y. */

/* Copyright (C) 1989-2020 Free Software Foundation, Inc.
   Copyright 2022, Jake Hamby.

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

#include "config.hh"

#include "shell.hh"

namespace bash
{

#if defined(HANDLE_MULTIBYTE)
#define last_shell_getc_is_singlebyte                                         \
  ((shell_input_line.size () > 1)                                             \
       ? shell_input_line_property[shell_input_line.size () - 1]              \
       : 1)
#define MBTEST(x) ((x) && last_shell_getc_is_singlebyte)
#else
#define last_shell_getc_is_singlebyte 1
#define MBTEST(x) ((x))
#endif

static void print_prompt ();
static bool reserved_word_acceptable (int);

/* Should we call prompt_again? */
#define SHOULD_PROMPT()                                                       \
  (interactive                                                                \
   && (bash_input.type == st_stdin || bash_input.type == st_stream))

#ifdef DEBUG
void
Shell::debug_parser (parser::debug_level_type i)
{
#if YYDEBUG != 0
  parser_.set_debug_level (i);
  parser_.set_debug_stream (std::cerr);
#endif
}
#endif

/* yy_getc () returns the next available character from input or EOF.
   yy_ungetc (c) makes `c' the next character to read.
   init_yy_io (get, unget, type, location) makes the function GET the
   installed function for getting the next character, makes UNGET the
   installed function for un-getting a character, sets the type of stream
   (either string or file) from TYPE, and makes LOCATION point to where
   the input is coming from. */

/* Unconditionally returns end-of-file. */
int
Shell::return_EOF ()
{
  return EOF;
}

/* Set all of the fields in BASH_INPUT to nullptr.  Free bash_input.name if it
   is non-null, avoiding a memory leak. */
void
Shell::initialize_bash_input ()
{
  bash_input.type = st_none;
  delete[] bash_input.name;
  bash_input.name = nullptr;
  bash_input.location.file = nullptr;
  bash_input.location.string = nullptr;
  bash_input.getter = nullptr;
  bash_input.ungetter = nullptr;
}

/* Set the contents of the current bash input stream from
   GET, UNGET, TYPE, NAME, and LOCATION. */
void
Shell::init_yy_io (sh_cget_func_t get, sh_cunget_func_t unget,
                   stream_type type, const char *name, INPUT_STREAM location)
{
  bash_input.type = type;
  delete[] bash_input.name;
  bash_input.name = name ? savestring (name) : nullptr;

  /* XXX */
  bash_input.location = location;
  bash_input.getter = get;
  bash_input.ungetter = unget;
}

/* **************************************************************** */
/*								    */
/*		  Let input be read from readline ().		    */
/*								    */
/* **************************************************************** */

#if defined(READLINE)

int
Shell::yy_readline_get ()
{
  SigHandler old_sigint;

  if (current_readline_line.empty ())
    {
      if (!bash_readline_initialized)
        initialize_readline ();

#if defined(JOB_CONTROL)
      if (job_control)
        give_terminal_to (shell_pgrp, 0);
#endif /* JOB_CONTROL */

      old_sigint = IMPOSSIBLE_TRAP_HANDLER;
      if (signal_is_ignored (SIGINT) == 0)
        {
          old_sigint = set_signal_handler (SIGINT, &sigint_sighandler_global);
        }

      sh_unset_nodelay_mode (::fileno (rl_instream)); /* just in case */
      current_readline_line
          = readline (current_readline_prompt ? current_readline_prompt : "");

      CHECK_TERMSIG;
      if (signal_is_ignored (SIGINT) == 0)
        {
          if (old_sigint != IMPOSSIBLE_TRAP_HANDLER)
            set_signal_handler (SIGINT, old_sigint);
        }

#if 0
      /* Reset the prompt to the decoded value of prompt_string_pointer. */
      reset_readline_prompt ();
#endif

      if (current_readline_line.empty ())
        return EOF;

      current_readline_line_index = 0;
      current_readline_line.push_back ('\n');
    }

  if (current_readline_line.size () == current_readline_line_index)
    {
      current_readline_line.clear ();
      return yy_readline_get ();
    }
  else
    {
      return static_cast<unsigned char> (
          current_readline_line[current_readline_line_index++]);
    }
}

void
Shell::with_input_from_stdin ()
{
  INPUT_STREAM location;

  if (bash_input.type != st_stdin && stream_on_stack (st_stdin) == 0)
    {
      location.string = savestring (current_readline_line);
      init_yy_io (&Shell::yy_readline_get, &Shell::yy_readline_unget, st_stdin,
                  "readline stdin", location);
    }
}

#else /* !READLINE */

void
Shell::with_input_from_stdin ()
{
  with_input_from_stream (stdin, "stdin");
}

#endif /* !READLINE */

/* **************************************************************** */
/*								    */
/*   Let input come from STRING.  STRING is zero terminated.	    */
/*								    */
/* **************************************************************** */

int
Shell::yy_string_get ()
{
  char *string = bash_input.location.string;

  /* If the string doesn't exist, or is empty, EOF found. */
  if (string && *string)
    {
      unsigned char c = static_cast<unsigned char> (*string++);
      bash_input.location.string = string;
      return c;
    }
  else
    return EOF;
}

int
Shell::yy_string_unget (int c)
{
  *(--bash_input.location.string) = static_cast<char> (c);
  return c;
}

void
Shell::with_input_from_string (char *string, const char *name)
{
  INPUT_STREAM location;

  location.string = string;
  init_yy_io (&Shell::yy_string_get, &Shell::yy_string_unget, st_string, name,
              location);
}

/* Count the number of characters we've consumed from
   bash_input.location.string and read into shell_input_line, but have not
   returned from shell_getc. That is the true input location.  Rewind
   bash_input.location.string by that number of characters, so it points to the
   last character actually consumed by the parser. */
void
Shell::rewind_input_string ()
{
  size_t xchars;

  /* number of unconsumed characters in the input -- XXX need to take newlines
     into account, e.g., $(...\n) */
  xchars = shell_input_line.size () - shell_input_line_index;
  if (bash_input.location.string[-1] == '\n')
    xchars++;

  /* XXX - how to reflect bash_input.location.string back to string passed to
     parse_and_execute or xparse_dolparen?  xparse_dolparen needs to know how
     far into the string we parsed.  parse_and_execute knows where bash_input.
     location.string is, and how far from orig_string that is -- that's the
     number of characters the command consumed. */

  /* bash_input.location.string - xchars should be where we parsed to */
  /* need to do more validation on xchars value for sanity -- test cases. */
  bash_input.location.string -= xchars;
}

/* **************************************************************** */
/*								    */
/*		     Let input come from STREAM.		    */
/*								    */
/* **************************************************************** */

/* These two functions used to test the value of the HAVE_RESTARTABLE_SYSCALLS
   define, and just use getc/ungetc if it was defined, but since bash
   installs most of its signal handlers without the SA_RESTART flag, some
   signals received during a read(2) will not cause the read to be restarted.
   We will need to restart it ourselves. */

int
Shell::yy_stream_get ()
{
  int result;

  result = EOF;
  if (bash_input.location.file)
    {
      /* XXX - don't need terminate_immediately; getc_with_restart checks
         for terminating signals itself if read returns < 0 */
      result = getc_with_restart (bash_input.location.file);
    }
  return result;
}

int
Shell::yy_stream_unget (int c)
{
  return ungetc_with_restart (c, bash_input.location.file);
}

void
Shell::with_input_from_stream (FILE *stream, const char *name)
{
  INPUT_STREAM location;

  location.file = stream;
  init_yy_io (&Shell::yy_stream_get, &Shell::yy_stream_unget, st_stream, name,
              location);
}

void
Shell::push_stream (int reset_lineno)
{
  STREAM_SAVER *saver = new STREAM_SAVER ();

  saver->bash_input = bash_input;

#if defined(BUFFERED_INPUT)
  saver->bstream = nullptr;
  /* If we have a buffered stream, clear out buffers[fd]. */
  if (bash_input.type == st_bstream && bash_input.location.buffered_fd >= 0)
    saver->bstream
        = set_buffered_stream (bash_input.location.buffered_fd, nullptr);
#endif /* BUFFERED_INPUT */

  saver->line = line_number;
  bash_input.name = nullptr;
  saver->set_next (stream_list);
  stream_list = saver;
  EOF_Reached = false;

  if (reset_lineno)
    line_number = 0;
}

void
Shell::pop_stream ()
{
  if (!stream_list)
    EOF_Reached = true;
  else
    {
      STREAM_SAVER *saver = stream_list;

      EOF_Reached = false;
      stream_list = stream_list->next ();

      init_yy_io (saver->bash_input.getter, saver->bash_input.ungetter,
                  saver->bash_input.type, saver->bash_input.name,
                  saver->bash_input.location);

#if defined(BUFFERED_INPUT)
      /* If we have a buffered stream, restore buffers[fd]. */
      /* If the input file descriptor was changed while this was on the
         save stack, update the buffered fd to the new file descriptor and
         re-establish the buffer <-> bash_input fd correspondence. */
      if (bash_input.type == st_bstream
          && bash_input.location.buffered_fd >= 0)
        {
          if (bash_input_fd_changed)
            {
              bash_input_fd_changed = 0;
              if (default_buffered_input >= 0)
                {
                  bash_input.location.buffered_fd = default_buffered_input;
                  saver->bstream->b_fd = default_buffered_input;
                  SET_CLOSE_ON_EXEC (default_buffered_input);
                }
            }
          /* XXX could free buffered stream returned as result here. */
          set_buffered_stream (bash_input.location.buffered_fd,
                               saver->bstream);
        }
#endif /* BUFFERED_INPUT */

      line_number = saver->line;

      delete[] saver->bash_input.name;
      delete saver;
    }
}

/*
 * This is used to inhibit alias expansion and reserved word recognition
 * inside case statement pattern lists.  A `case statement pattern list' is:
 *
 *	everything between the `in' in a `case word in' and the next ')'
 *	or `esac'
 *	everything between a `;;' and the next `)' or `esac'
 */

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)

#define END_OF_ALIAS 0

/*
 * Pseudo-global variables used in implementing token-wise alias expansion.
 */

/*
 * Push the current shell_input_line onto a stack of such lines and make S
 * the current input.  Used when expanding aliases.  EXPAND is used to set
 * the value of expand_next_token when the string is popped, so that the
 * word after the alias in the original line is handled correctly when the
 * alias expands to multiple words.  TOKEN is the token that was expanded
 * into S; it is saved and used to prevent infinite recursive expansion.
 */
void
Shell::push_string (char *s, int expand, alias_t *ap)
{
  STRING_SAVER *temp = new STRING_SAVER ();

  temp->expand_alias = expand;
  temp->saved_line = shell_input_line;
  temp->saved_line_index = shell_input_line_index;
  temp->saved_line_terminator = shell_input_line_terminator;

#if defined(ALIAS)
  temp->expander = ap;
  if (ap)
    temp->flags = PSH_ALIAS;
#endif

  temp->set_next (pushed_string_list);
  pushed_string_list = temp;

#if defined(ALIAS)
  if (ap)
    ap->flags |= AL_BEINGEXPANDED;
#endif

  shell_input_line = s;
  shell_input_line_index = 0;
  shell_input_line_terminator = '\0';
#if 0
  parser_state &= ~PST_ALEXPNEXT;	/* XXX */
#endif

  set_line_mbstate ();
}

/*
 * Make the top of the pushed_string stack be the current shell input.
 * Only called when there is something on the stack.  Called from shell_getc
 * when it thinks it has consumed the string generated by an alias expansion
 * and needs to return to the original input line.
 */
void
Shell::pop_string ()
{
  STRING_SAVER *t;

  shell_input_line = pushed_string_list->saved_line;
  shell_input_line_index = pushed_string_list->saved_line_index;
  shell_input_line_terminator = pushed_string_list->saved_line_terminator;

  if (pushed_string_list->expand_alias)
    parser_state |= PST_ALEXPNEXT;
  else
    parser_state &= ~PST_ALEXPNEXT;

  t = pushed_string_list;
  pushed_string_list = pushed_string_list->next ();

#if defined(ALIAS)
  if (t->expander)
    t->expander->flags &= ~AL_BEINGEXPANDED;
#endif

  delete t;

  set_line_mbstate ();
}

void
Shell::free_string_list ()
{
  STRING_SAVER *t, *t1;

  for (t = pushed_string_list; t;)
    {
      t1 = t->next ();
#if defined(ALIAS)
      if (t->expander)
        t->expander->flags &= ~AL_BEINGEXPANDED;
#endif
      delete t;
      t = t1;
    }
  pushed_string_list = nullptr;
}

#endif /* ALIAS || DPAREN_ARITHMETIC */

#if defined(ALIAS)
/* Before freeing AP, make sure that there aren't any cases of pointer
   aliasing that could cause us to reference freed memory later on. */
void
Shell::clear_string_list_expander (alias_t *ap)
{
  STRING_SAVER *t;

  for (t = pushed_string_list; t; t = t->next ())
    {
      if (t->expander && t->expander == ap)
        t->expander = nullptr;
    }
}
#endif

/* Return a line of text, taken from wherever yylex () reads input.
   If there is no more input, then we return nullptr.  If REMOVE_QUOTED_NEWLINE
   is non-zero, we remove unquoted \<newline> pairs.  This is used by
   read_secondary_line to read here documents. */
const char *
Shell::read_a_line (bool remove_quoted_newline)
{
  int c, peekc;

#if defined(READLINE)
  if (no_line_editing && SHOULD_PROMPT ())
#else
  if (SHOULD_PROMPT ())
#endif
    print_prompt ();

  read_a_line_buffer.clear ();

  bool pass_next = false;
  while (1)
    {
      /* Allow immediate exit if interrupted during input. */
      QUIT;

      c = yy_getc ();

      /* Ignore null bytes in input. */
      if (c == 0)
        {
#if 0
	  internal_warning ("read_a_line: ignored null byte in input");
#endif
          continue;
        }

      /* If there is no more input, then we return nullptr. */
      if (c == EOF)
        {
          if (interactive && bash_input.type == st_stream)
            ::clearerr (stdin);
          if (read_a_line_buffer.empty ())
            return nullptr;
          c = '\n';
        }

      /* IF REMOVE_QUOTED_NEWLINES is non-zero, we are reading a
         here document with an unquoted delimiter.  In this case,
         the line will be expanded as if it were in double quotes.
         We allow a backslash to escape the next character, but we
         need to treat the backslash specially only if a backslash
         quoting a backslash-newline pair appears in the line. */
      if (pass_next)
        {
          read_a_line_buffer.push_back (static_cast<char> (c));
          pass_next = false;
        }
      else if (c == '\\' && remove_quoted_newline)
        {
          QUIT;
          peekc = yy_getc ();
          if (peekc == '\n')
            {
              line_number++;
              continue; /* Make the unquoted \<newline> pair disappear. */
            }
          else
            {
              yy_ungetc (peekc);
              pass_next = true;
              /* Preserve the backslash. */
              read_a_line_buffer.push_back (static_cast<char> (c));
            }
        }
      else
        {
          /* remove_quoted_newline is non-zero if the here-document delimiter
             is unquoted. In this case, we will be expanding the lines and
             need to make sure CTLESC and CTLNUL in the input are quoted. */
          if (remove_quoted_newline && (c == CTLESC || c == CTLNUL))
            read_a_line_buffer.push_back (CTLESC);

          read_a_line_buffer.push_back (static_cast<char> (c));
        }

      if (c == '\n')
        return read_a_line_buffer.c_str ();
    }
}

/* Return a line as in read_a_line (), but insure that the prompt is
   the secondary prompt.  This is used to read the lines of a here
   document.  REMOVE_QUOTED_NEWLINE is non-zero if we should remove
   newlines quoted with backslashes while reading the line.  It is
   non-zero unless the delimiter of the here document was quoted. */
const char *
Shell::read_secondary_line (bool remove_quoted_newline)
{
  const char *ret;

  prompt_string_pointer = &ps2_prompt;
  if (SHOULD_PROMPT ())
    prompt_again ();

  ret = read_a_line (remove_quoted_newline);

#if defined(HISTORY)
  if (ret && remember_on_history && (parser_state & PST_HEREDOC))
    {
      /* To make adding the here-document body right, we need to rely on
         history_delimiting_chars() returning \n for the first line of the
         here-document body and the null string for the second and subsequent
         lines, so we avoid double newlines.
         current_command_line_count == 2 for the first line of the body. */

      current_command_line_count++;
      maybe_add_history (ret);
    }
#endif /* HISTORY */

  return ret;
}

/* **************************************************************** */
/*								    */
/*				YYLEX ()			    */
/*								    */
/* **************************************************************** */

/* Reserved words.  These are only recognized as the first word of a
   command. */
static constexpr STRING_INT_ALIST word_token_alist[]
    = { STRING_INT_ALIST ("if", parser::token::IF),
        STRING_INT_ALIST ("then", parser::token::THEN),
        STRING_INT_ALIST ("else", parser::token::ELSE),
        STRING_INT_ALIST ("elif", parser::token::ELIF),
        STRING_INT_ALIST ("fi", parser::token::FI),
        STRING_INT_ALIST ("case", parser::token::CASE),
        STRING_INT_ALIST ("esac", parser::token::ESAC),
        STRING_INT_ALIST ("for", parser::token::FOR),
#if defined(SELECT_COMMAND)
        STRING_INT_ALIST ("select", parser::token::SELECT),
#endif
        STRING_INT_ALIST ("while", parser::token::WHILE),
        STRING_INT_ALIST ("until", parser::token::UNTIL),
        STRING_INT_ALIST ("do", parser::token::DO),
        STRING_INT_ALIST ("done", parser::token::DONE),
        STRING_INT_ALIST ("in", parser::token::IN),
        STRING_INT_ALIST ("function", parser::token::FUNCTION),
#if defined(COMMAND_TIMING)
        STRING_INT_ALIST ("time", parser::token::TIME),
#endif
        STRING_INT_ALIST ("{", '{'),
        STRING_INT_ALIST ("}", '}'),
        STRING_INT_ALIST ("!", parser::token::BANG),
#if defined(COND_COMMAND)
        STRING_INT_ALIST ("[[", parser::token::COND_START),
        STRING_INT_ALIST ("]]", parser::token::COND_END),
#endif
#if defined(COPROCESS_SUPPORT)
        STRING_INT_ALIST ("coproc", parser::token::COPROC),
#endif
        STRING_INT_ALIST (nullptr, 0) };

/* other tokens that can be returned by read_token() */
static constexpr STRING_INT_ALIST other_token_alist[] = {
  /* Multiple-character tokens with special values */
  STRING_INT_ALIST ("--", parser::token::TIMEIGN),
  STRING_INT_ALIST ("-p", parser::token::TIMEOPT),
  STRING_INT_ALIST ("&&", parser::token::AND_AND),
  STRING_INT_ALIST ("||", parser::token::OR_OR),
  STRING_INT_ALIST (">>", parser::token::GREATER_GREATER),
  STRING_INT_ALIST ("<<", parser::token::LESS_LESS),
  STRING_INT_ALIST ("<&", parser::token::LESS_AND),
  STRING_INT_ALIST (">&", parser::token::GREATER_AND),
  STRING_INT_ALIST (";;", parser::token::SEMI_SEMI),
  STRING_INT_ALIST (";&", parser::token::SEMI_AND),
  STRING_INT_ALIST (";;&", parser::token::SEMI_SEMI_AND),
  STRING_INT_ALIST ("<<-", parser::token::LESS_LESS_MINUS),
  STRING_INT_ALIST ("<<<", parser::token::LESS_LESS_LESS),
  STRING_INT_ALIST ("&>", parser::token::AND_GREATER),
  STRING_INT_ALIST ("&>>", parser::token::AND_GREATER_GREATER),
  STRING_INT_ALIST ("<>", parser::token::LESS_GREATER),
  STRING_INT_ALIST (">|", parser::token::GREATER_BAR),
  STRING_INT_ALIST ("|&", parser::token::BAR_AND),
  STRING_INT_ALIST ("EOF", parser::token::yacc_EOF),
  /* Tokens whose value is the character itself */
  STRING_INT_ALIST (">", '>'), STRING_INT_ALIST ("<", '<'),
  STRING_INT_ALIST ("-", '-'), STRING_INT_ALIST ("{", '{'),
  STRING_INT_ALIST ("}", '}'), STRING_INT_ALIST (";", ';'),
  STRING_INT_ALIST ("(", '('), STRING_INT_ALIST (")", ')'),
  STRING_INT_ALIST ("|", '|'), STRING_INT_ALIST ("&", '&'),
  STRING_INT_ALIST ("newline", '\n'), STRING_INT_ALIST (nullptr, 0)
};

/* others not listed here:
        WORD			look at yylval.word
        ASSIGNMENT_WORD		look at yylval.word
        NUMBER			look at yylval.number
        ARITH_CMD		look at yylval.word_list
        ARITH_FOR_EXPRS		look at yylval.word_list
        COND_CMD		look at yylval.command
*/

/* Return the next shell input character.  This always reads characters
   from shell_input_line; when that line is exhausted, it is time to
   read the next line.  This is called by read_token when the shell is
   processing normal command input. */
int
Shell::shell_getc (bool remove_quoted_newline)
{
  int c;
  unsigned char uc;

  QUIT;

  bool last_was_backslash = false;
  if (sigwinch_received)
    {
      sigwinch_received = 0;
      get_new_window_size (0, nullptr, nullptr);
    }

  if (eol_ungetc_lookahead)
    {
      c = eol_ungetc_lookahead;
      eol_ungetc_lookahead = 0;
      return c;
    }

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  /* If shell_input_line[shell_input_line_index] == 0, but there is
     something on the pushed list of strings, then we don't want to go
     off and get another line.  We let the code down below handle it. */

  if (shell_input_line.empty ()
      || ((!shell_input_line[shell_input_line_index])
          && (pushed_string_list == nullptr)))
#else  /* !ALIAS && !DPAREN_ARITHMETIC */
  if (shell_input_line.empty () || !shell_input_line[shell_input_line_index])
#endif /* !ALIAS && !DPAREN_ARITHMETIC */
    {
      line_number++;

      /* Let's not let one really really long line blow up memory allocation */
      if (shell_input_line.capacity () >= 32768)
        shell_input_line.reserve (0);

    restart_read:

      /* Allow immediate exit if interrupted during input. */
      QUIT;

      int i = 0, truncating = 0;
      shell_input_line_terminator = 0;

      /* If the shell is interactive, but not currently printing a prompt
         (interactive_shell && interactive == 0), we don't want to print
         notifies or cleanup the jobs -- we want to defer it until we do
         print the next prompt. */
      if (!interactive_shell || SHOULD_PROMPT ())
        {
#if defined(JOB_CONTROL)
          /* This can cause a problem when reading a command as the result
             of a trap, when the trap is called from flush_child.  This call
             had better not cause jobs to disappear from the job table in
             that case, or we will have big trouble. */
          notify_and_cleanup ();
#else  /* !JOB_CONTROL */
          cleanup_dead_jobs ();
#endif /* !JOB_CONTROL */
        }

#if defined(READLINE)
      if (no_line_editing && SHOULD_PROMPT ())
#else
      if (SHOULD_PROMPT ())
#endif
        print_prompt ();

      if (bash_input.type == st_stream)
        clearerr (stdin);

      while (1)
        {
          c = yy_getc ();

          /* Allow immediate exit if interrupted during input. */
          QUIT;

          if (c == '\0')
            {
#if 0
	      internal_warning ("shell_getc: ignored null byte in input");
#endif
              /* If we get EOS while parsing a string, treat it as EOF so we
                 don't just keep looping. Happens very rarely */
              if (bash_input.type == st_string)
                {
                  if (i == 0)
                    shell_input_line_terminator = EOF;
                  c = EOF;
                  break;
                }
              continue;
            }

          if (c == EOF)
            {
              if (bash_input.type == st_stream)
                clearerr (stdin);

              if (i == 0)
                shell_input_line_terminator = EOF;

              break;
            }

          if (truncating == 0 || c == '\n')
            shell_input_line.push_back (static_cast<char> (c));

          if (c == '\n')
            {
              current_command_line_count++;
              break;
            }

          last_was_backslash = (!last_was_backslash && c == '\\');
        }

      shell_input_line_index = 0;

      set_line_mbstate ();

#if defined(HISTORY)
      if (remember_on_history && !shell_input_line.empty ())
        {
          char *expansions;
#if defined(BANG_HISTORY)
          /* If the current delimiter is a single quote, we should not be
             performing history expansion, even if we're on a different
             line from the original single quote. */
          if (dstack.back () == '\'')
            history_quoting_state = '\'';
          else if (dstack.back () == '"')
            history_quoting_state = '"';
          else
            history_quoting_state = 0;
#endif
          /* Calling with a third argument of 1 allows remember_on_history to
             determine whether or not the line is saved to the history list */
          expansions
              = pre_process_line (shell_input_line.c_str (), true, true);
#if defined(BANG_HISTORY)
          history_quoting_state = 0;
#endif
          if (expansions != shell_input_line.c_str ())
            {
              shell_input_line = expansions;
              delete[] expansions;

              if (shell_input_line.empty ())
                current_command_line_count--;

              set_line_mbstate ();
            }
        }
      /* Try to do something intelligent with blank lines encountered while
         entering multi-line commands.  XXX - this is grotesque */
      else if (remember_on_history && !shell_input_line.empty ()
               && current_command_line_count > 1)
        {
          if (!dstack.empty ())
            /* We know shell_input_line[0] == 0 and we're reading some sort of
               quoted string.  This means we've got a line consisting of only
               a newline in a quoted string.  We want to make sure this line
               gets added to the history. */
            maybe_add_history (shell_input_line.c_str ());
          else
            {
              const char *hdcs;
              hdcs = history_delimiting_chars (shell_input_line.c_str ());
              if (hdcs && hdcs[0] == ';')
                maybe_add_history (shell_input_line.c_str ());
            }
        }

#endif /* HISTORY */

      if (!shell_input_line.empty ())
        {
          /* Lines that signify the end of the shell's input should not be
             echoed.  We should not echo lines while parsing command
             substitutions with recursive calls into the parsing engine; those
             should only be echoed once when we read the word.  That is the
             reason for the test against shell_eof_token, which is set to a
             right paren when parsing the contents of command substitutions. */
          if (echo_input_at_read
              && (shell_input_line[0] || shell_input_line_terminator != EOF)
              && shell_eof_token == 0)
            std::fprintf (stderr, "%s\n", shell_input_line.c_str ());
        }
      else
        {
          prompt_string_pointer = &current_prompt_string;
          if (SHOULD_PROMPT ())
            prompt_again ();
          goto restart_read;
        }

      /* Add the newline to the end of this string, iff the string does
         not already end in an EOF character.  */
      if (shell_input_line_terminator != EOF)
        {
          /* Don't add a newline to a string that ends with a backslash if
             we're going to be removing quoted newlines, since that will eat
             the backslash.  Add another backslash instead (will be removed by
             word expansion). */
          if (bash_input.type == st_string && !parser_expanding_alias ()
              && last_was_backslash && c == EOF && remove_quoted_newline)
            shell_input_line.push_back ('\\');
          else
            shell_input_line.push_back ('\n');

#if 0
	  set_line_mbstate ();		/* XXX - this is wasteful */
#else
#if defined(HANDLE_MULTIBYTE)
          /* This is kind of an abstraction violation, but there's no need to
             go through the entire shell_input_line again with a call to
             set_line_mbstate(). */
          shell_input_line_property.reserve (shell_input_line.size () + 1);
          shell_input_line_property[shell_input_line.size ()] = 1;
#endif
#endif
        }
    }

next_alias_char:
  if (shell_input_line_index == 0)
    unquoted_backslash = false;

  uc = static_cast<unsigned char> (shell_input_line[shell_input_line_index]);

  if (uc)
    {
      unquoted_backslash = (!unquoted_backslash && uc == '\\');
      shell_input_line_index++;
    }

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  /* If UC is nullptr, we have reached the end of the current input string.  If
     pushed_string_list is non-empty, it's time to pop to the previous string
     because we have fully consumed the result of the last alias expansion.
     Do it transparently; just return the next character of the string popped
     to. */
  /* If pushed_string_list != 0 but pushed_string_list->expander == 0 (not
     currently tested) and the flags value is not PSH_SOURCE, we are not
     parsing an alias, we have just saved one (push_string, when called by
     the parse_dparen code) In this case, just go on as well.  The PSH_SOURCE
     case is handled below. */

  /* If we're at the end of an alias expansion add a space to make sure that
     the alias remains marked as being in use while we expand its last word.
     This makes sure that pop_string doesn't mark the alias as not in use
     before the string resulting from the alias expansion is tokenized and
     checked for alias expansion, preventing recursion.  At this point, the
     last character in shell_input_line is the last character of the alias
     expansion.  We test that last character to determine whether or not to
     return the space that will delimit the token and postpone the
     pop_string. This set of conditions duplicates what used to be in
     mk_alexpansion () below, with the addition that we don't add a space if
     we're currently reading a quoted string or in a shell comment. */
  if (uc == 0 && pushed_string_list && pushed_string_list->flags != PSH_SOURCE
      && pushed_string_list->flags != PSH_DPAREN
      && (parser_state & PST_COMMENT) == 0
      && (parser_state & PST_ENDALIAS) == 0 && /* only once */
      shell_input_line_index > 0
      && shellblank (shell_input_line[shell_input_line_index - 1]) == 0
      && shell_input_line[shell_input_line_index - 1] != '\n'
      && !unquoted_backslash
      && shellmeta (shell_input_line[shell_input_line_index - 1]) == 0
      && (dstack.back () != '\'' && dstack.back () != '"'))
    {
      parser_state |= PST_ENDALIAS;
      return ' '; /* END_ALIAS */
    }

pop_alias:
  /* This case works for PSH_DPAREN as well */
  if (uc == 0 && pushed_string_list && pushed_string_list->flags != PSH_SOURCE)
    {
      parser_state &= ~PST_ENDALIAS;
      pop_string ();
      uc = static_cast<unsigned char> (
          shell_input_line[shell_input_line_index]);
      if (uc)
        shell_input_line_index++;
    }
#endif /* ALIAS || DPAREN_ARITHMETIC */

  if MBTEST (uc == '\\' && remove_quoted_newline
             && shell_input_line[shell_input_line_index] == '\n')
    {
      if (SHOULD_PROMPT ())
        prompt_again ();
      line_number++;

      /* What do we do here if we're expanding an alias whose definition
         includes an escaped newline?  If that's the last character in the
         alias expansion, we just pop the pushed string list (recall that
         we inhibit the appending of a space if newline is the last
         character).  If it's not the last character, we need to consume the
         quoted newline and move to the next character in the expansion. */
#if defined(ALIAS)
      if (parser_expanding_alias ()
          && shell_input_line[shell_input_line_index + 1] == '\0')
        {
          uc = 0;
          goto pop_alias;
        }
      else if (parser_expanding_alias ()
               && shell_input_line[shell_input_line_index + 1] != '\0')
        {
          shell_input_line_index++; /* skip newline */
          goto next_alias_char;     /* and get next character */
        }
      else
#endif
        goto restart_read;
    }

  if (uc == 0 && shell_input_line_terminator == EOF)
    return (shell_input_line_index != 0) ? '\n' : EOF;

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  /* We already know that we are not parsing an alias expansion because of the
     check for expanding_alias() above.  This knows how parse_and_execute
     handles switching to st_string input while an alias is being expanded,
     hence the check for pushed_string_list without
     pushed_string_list->expander and the check for PSH_SOURCE as
     pushed_string_list->flags. parse_and_execute and parse_string both change
     the input type to st_string and place the string to be parsed and executed
     into location.string, so we should not stop reading that until the pointer
     is '\0'. The check for shell_input_line_terminator may be superfluous.

     This solves the problem of `.' inside a multi-line alias with embedded
     newlines executing things out of order. */
  if (uc == 0 && bash_input.type == st_string && *bash_input.location.string
      && pushed_string_list && pushed_string_list->flags == PSH_SOURCE
      && shell_input_line_terminator == 0)
    {
      shell_input_line_index = 0;
      goto restart_read;
    }
#endif

  return uc;
}

/* Put C back into the input for the shell.  This might need changes for
   HANDLE_MULTIBYTE around EOLs.  Since we (currently) never push back a
   character different than we read, shell_input_line_property doesn't need
   to change when manipulating shell_input_line.  The define for
   last_shell_getc_is_singlebyte should take care of it, though. */
void
Shell::shell_ungetc (int c)
{
  if (!shell_input_line.empty () && shell_input_line_index)
    shell_input_line[--shell_input_line_index] = static_cast<char> (c);
  else
    eol_ungetc_lookahead = c;
}

const char *
Shell::parser_remaining_input ()
{
  if (shell_input_line.empty ())
    return nullptr;

  if (static_cast<int> (shell_input_line_index) < 0
      || shell_input_line_index >= shell_input_line.size ())
    return ""; /* XXX */

  return &shell_input_line[shell_input_line_index];
}

/* Discard input until CHARACTER is seen, then push that character back
   onto the input stream. */
void
Shell::discard_until (int character)
{
  int c;

  while ((c = shell_getc (0)) != EOF && c != character)
    ;

  if (c != EOF)
    shell_ungetc (c);
}

void
Shell::execute_variable_command (const char *command, const char *vname)
{
  char *last_lastarg;
  sh_parser_state_t ps;

  save_parser_state (&ps);
  last_lastarg = get_string_value ("_");
  if (last_lastarg)
    last_lastarg = savestring (last_lastarg);

  parse_and_execute (savestring (command), vname,
                     (SEVAL_NONINT | SEVAL_NOHIST));

  restore_parser_state (&ps);
  bind_variable ("_", last_lastarg, 0);
  delete[] last_lastarg;

  if (token_to_read == '\n') /* reset_parser was called */
    token_to_read = 0;
}

void
Shell::push_token (parser::token::token_kind_type x)
{
  two_tokens_ago = token_before_that;
  token_before_that = last_read_token;
  last_read_token = current_token;

  current_token = x;
}

#if 0
/* Place to remember the token.  We try to keep the buffer
   at a reasonable size, but it can grow. */
static char *token = nullptr;

/* Current size of the token buffer. */
static int token_buffer_size;
#endif

/* Command to read_token () explaining what we want it to do. */
#define READ 0
#define RESET 1
#define prompt_is_ps1                                                         \
  (!prompt_string_pointer || prompt_string_pointer == &ps1_prompt)

/* Function for yyparse to call.  yylex keeps track of
   the last two tokens read, and calls read_token.  */
parser::symbol_type
Shell::yylex ()
{
  if (interactive && (current_token == 0 || current_token == '\n'))
    {
      /* Before we print a prompt, we might have to check mailboxes.
         We do this only if it is time to do so. Notice that only here
         is the mail alarm reset; nothing takes place in check_mail ()
         except the checking of mail.  Please don't change this. */
      if (prompt_is_ps1 && parse_and_execute_level == 0
          && time_to_check_mail ())
        {
          check_mail ();
          reset_mail_timer ();
        }

      /* Avoid printing a prompt if we're not going to read anything, e.g.
         after resetting the parser with read_token (RESET). */
      if (token_to_read == 0 && SHOULD_PROMPT ())
        prompt_again ();
    }

  two_tokens_ago = token_before_that;
  token_before_that = last_read_token;
  last_read_token = current_token;

  parser::symbol_type symbol = read_token (READ);
  current_token = symbol.kind ();

  if ((parser_state & PST_EOFTOKEN) && current_token == shell_eof_token)
    {
      current_token = parser::token::yacc_EOF;
      if (bash_input.type == st_string)
        rewind_input_string ();
    }
  parser_state &= ~PST_EOFTOKEN; /* ??? */

  return symbol;
}

void
Shell::gather_here_documents ()
{
  int r = 0;
  here_doc_first_line = true;
  while (need_here_doc > 0)
    {
      parser_state |= PST_HEREDOC;
      make_here_document (redir_stack[r++], line_number);
      parser_state &= ~PST_HEREDOC;
      need_here_doc--;
      redir_stack[r - 1] = nullptr; /* XXX */
    }
  here_doc_first_line = false; /* just in case */
}

/* In the following three macros, `token' is always last_read_token */

/* Are we in the middle of parsing a redirection where we are about to read
   a word?  This is used to make sure alias expansion doesn't happen in the
   middle of a redirection, even though we're parsing a simple command. */
static inline bool
parsing_redirection (int token)
{
  return token == '<' || token == '>'
         || token == parser::token::GREATER_GREATER
         || token == parser::token::GREATER_BAR
         || token == parser::token::LESS_GREATER
         || token == parser::token::LESS_LESS_MINUS
         || token == parser::token::LESS_LESS
         || token == parser::token::LESS_LESS_LESS
         || token == parser::token::LESS_AND
         || token == parser::token::GREATER_AND
         || token == parser::token::AND_GREATER;
}

/* Is `token' one that will allow a WORD to be read in a command position?
   We can read a simple command name on which we should attempt alias expansion
   or we can read an assignment statement. */
static inline bool
command_token_position (int token, pstate_flags parser_state)
{
  return (token == parser::token::ASSIGNMENT_WORD)
         || ((parser_state & PST_REDIRLIST) && !parsing_redirection (token))
         || (token != parser::token::SEMI_SEMI
             && token != parser::token::SEMI_AND
             && token != parser::token::SEMI_SEMI_AND
             && reserved_word_acceptable (token));
}

/* Are we in a position where we can read an assignment statement? */
#define assignment_acceptable(token)                                          \
  (command_token_position (token) && ((parser_state & PST_CASEPAT) == 0))

/* Check to see if TOKEN is a reserved word and return the token
   value if it is. */
#define CHECK_FOR_RESERVED_WORD(tok)                                          \
  do                                                                          \
    {                                                                         \
      if (!dollar_present && !quoted                                          \
          && reserved_word_acceptable (last_read_token))                      \
        {                                                                     \
          int i;                                                              \
          for (i = 0; word_token_alist[i].word != nullptr; i++)               \
            if (STREQ (tok, word_token_alist[i].word))                        \
              {                                                               \
                if ((parser_state & PST_CASEPAT)                              \
                    && (word_token_alist[i].token != parser::token::ESAC))    \
                  break;                                                      \
                if (word_token_alist[i].token == parser::token::TIME          \
                    && time_command_acceptable () == 0)                       \
                  break;                                                      \
                if ((parser_state & PST_CASEPAT) && last_read_token == '|'    \
                    && word_token_alist[i].token == parser::token::ESAC)      \
                  break; /* Posix grammar rule 4 */                           \
                if (word_token_alist[i].token == parser::token::ESAC)         \
                  parser_state &= ~(PST_CASEPAT | PST_CASESTMT);              \
                else if (word_token_alist[i].token == parser::token::CASE)    \
                  parser_state |= PST_CASESTMT;                               \
                else if (word_token_alist[i].token                            \
                         == parser::token::COND_END)                          \
                  parser_state &= ~(PST_CONDCMD | PST_CONDEXPR);              \
                else if (word_token_alist[i].token                            \
                         == parser::token::COND_START)                        \
                  parser_state |= PST_CONDCMD;                                \
                else if (word_token_alist[i].token == '{')                    \
                  open_brace_count++;                                         \
                else if (word_token_alist[i].token == '}'                     \
                         && open_brace_count)                                 \
                  open_brace_count--;                                         \
                return word_token_alist[i].token;                             \
              }                                                               \
        }                                                                     \
    }                                                                         \
  while (0)

#if defined(ALIAS)

/* OK, we have a token.  Let's try to alias expand it, if (and only if)
   it's eligible.

   It is eligible for expansion if EXPAND_ALIASES is set, and
   the token is unquoted and the last token read was a command
   separator (or expand_next_token is set), and we are currently
   processing an alias (pushed_string_list is non-empty) and this
   token is not the same as the current or any previously
   processed alias.

   Special cases that disqualify:
     In a pattern list in a case statement (parser_state & PST_CASEPAT). */
Shell::alias_expand_token_result
Shell::alias_expand_token (const char *tokstr)
{
  char *expanded;
  alias_t *ap;

  if (((parser_state & PST_ALEXPNEXT)
       || command_token_position (last_read_token, parser_state))
      && (parser_state & PST_CASEPAT) == 0)
    {
      ap = find_alias (tokstr);

      /* Currently expanding this token. */
      if (ap && (ap->flags & AL_BEINGEXPANDED))
        return NO_EXPANSION;

      expanded = ap ? savestring (ap->value) : nullptr;

      if (expanded)
        {
          push_string (expanded, ap->flags & AL_EXPANDNEXT, ap);
          return RE_READ_TOKEN;
        }
      else
        /* This is an eligible token that does not have an expansion. */
        return NO_EXPANSION;
    }
  return NO_EXPANSION;
}

#endif /* ALIAS */

bool
Shell::time_command_acceptable ()
{
#if defined(COMMAND_TIMING)
  size_t i;

  if (posixly_correct && shell_compatibility_level > 41)
    {
      /* Quick check of the rest of the line to find the next token.  If it
         begins with a `-', Posix says to not return `time' as the token.
         This was interp 267. */
      i = shell_input_line_index;
      while (i < shell_input_line.size ()
             && (shell_input_line[i] == ' ' || shell_input_line[i] == '\t'))
        i++;
      if (shell_input_line[i] == '-')
        return false;
    }

  switch (last_read_token)
    {
    case 0:
    case ';':
    case '\n':
      if (token_before_that == '|')
        return false;
      __attribute__ ((fallthrough));
      /* FALLTHROUGH */

    case parser::token::AND_AND:
    case parser::token::OR_OR:
    case '&':
    case parser::token::WHILE:
    case parser::token::DO:
    case parser::token::UNTIL:
    case parser::token::IF:
    case parser::token::THEN:
    case parser::token::ELIF:
    case parser::token::ELSE:
    case '{':                    /* } */
    case '(':                    /* )( */
    case ')':                    /* only valid in case statement */
    case parser::token::BANG:    /* ! time pipeline */
    case parser::token::TIME:    /* time time pipeline */
    case parser::token::TIMEOPT: /* time -p time pipeline */
    case parser::token::TIMEIGN: /* time -p -- ... */
      return true;
    default:
      return false;
    }
#else
  return false;
#endif /* COMMAND_TIMING */
}

/* Handle special cases of token recognition:
        IN is recognized if the last token was WORD and the token
        before that was FOR or CASE or SELECT.

        DO is recognized if the last token was WORD and the token
        before that was FOR or SELECT.

        ESAC is recognized if the last token caused `esacs_needed_count'
        to be set

        `{' is recognized if the last token as WORD and the token
        before that was FUNCTION, or if we just parsed an arithmetic
        `for' command.

        `}' is recognized if there is an unclosed `{' present.

        `-p' is returned as TIMEOPT if the last read token was TIME.
        `--' is returned as TIMEIGN if the last read token was TIME or TIMEOPT.

        ']]' is returned as COND_END if the parser is currently parsing
        a conditional expression ((parser_state & PST_CONDEXPR) != 0)

        `time' is returned as TIME if and only if it is immediately
        preceded by one of `;', `\n', `||', `&&', or `&'.
*/
int
Shell::special_case_tokens (const char *tokstr)
{
  /* Posix grammar rule 6 */
  if ((last_read_token == parser::token::WORD) &&
#if defined(SELECT_COMMAND)
      ((token_before_that == parser::token::FOR)
       || (token_before_that == parser::token::CASE)
       || (token_before_that == parser::token::SELECT))
      &&
#else
      ((token_before_that == parser::token::FOR)
       || (token_before_that == parser::token::CASE))
      &&
#endif
      (tokstr[0] == 'i' && tokstr[1] == 'n' && tokstr[2] == '\0'))
    {
      if (token_before_that == parser::token::CASE)
        {
          parser_state |= PST_CASEPAT;
          esacs_needed_count++;
        }
      if (expecting_in_token)
        expecting_in_token--;
      return parser::token::IN;
    }

  /* XXX - leaving above code intact for now, but it should eventually be
     removed in favor of this clause. */
  /* Posix grammar rule 6 */
  if (expecting_in_token
      && (last_read_token == parser::token::WORD || last_read_token == '\n')
      && (tokstr[0] == 'i' && tokstr[1] == 'n' && tokstr[2] == 0))
    {
      if (parser_state & PST_CASESTMT)
        {
          parser_state |= PST_CASEPAT;
          esacs_needed_count++;
        }
      expecting_in_token--;
      return parser::token::IN;
    }
  /* Posix grammar rule 6, third word in FOR: for i; do command-list; done */
  else if (expecting_in_token
           && (last_read_token == '\n' || last_read_token == ';')
           && (tokstr[0] == 'd' && tokstr[1] == 'o' && tokstr[2] == '\0'))
    {
      expecting_in_token--;
      return parser::token::DO;
    }

  /* for i do; command-list; done */
  if (last_read_token == parser::token::WORD &&
#if defined(SELECT_COMMAND)
      (token_before_that == parser::token::FOR
       || token_before_that == parser::token::SELECT)
      &&
#else
      (token_before_that == parser::token::FOR) &&
#endif
      (tokstr[0] == 'd' && tokstr[1] == 'o' && tokstr[2] == '\0'))
    {
      if (expecting_in_token)
        expecting_in_token--;
      return parser::token::DO;
    }

  /* Ditto for ESAC in the CASE case.
     Specifically, this handles "case word in esac", which is a legal
     construct, certainly because someone will pass an empty arg to the
     case construct, and we don't want it to barf.  Of course, we should
     insist that the case construct has at least one pattern in it, but
     the designers disagree. */
  if (esacs_needed_count)
    {
      if (last_read_token == parser::token::IN && STREQ (tokstr, "esac"))
        {
          esacs_needed_count--;
          parser_state &= ~PST_CASEPAT;
          return parser::token::ESAC;
        }
    }

  /* The start of a shell function definition. */
  if (parser_state & PST_ALLOWOPNBRC)
    {
      parser_state &= ~PST_ALLOWOPNBRC;
      if (tokstr[0] == '{' && tokstr[1] == '\0') /* } */
        {
          open_brace_count++;
          function_bstart = line_number;
          return '{'; /* } */
        }
    }

  /* We allow a `do' after a for ((...)) without an intervening
     list_terminator */
  if (last_read_token == parser::token::ARITH_FOR_EXPRS && tokstr[0] == 'd'
      && tokstr[1] == 'o' && !tokstr[2])
    return parser::token::DO;
  if (last_read_token == parser::token::ARITH_FOR_EXPRS && tokstr[0] == '{'
      && tokstr[1] == '\0') /* } */
    {
      open_brace_count++;
      return '{'; /* } */
    }

  if (open_brace_count && reserved_word_acceptable (last_read_token)
      && tokstr[0] == '}' && !tokstr[1])
    {
      open_brace_count--; /* { */
      return '}';
    }

#if defined(COMMAND_TIMING)
  /* Handle -p after `time'. */
  if (last_read_token == parser::token::TIME && tokstr[0] == '-'
      && tokstr[1] == 'p' && !tokstr[2])
    return parser::token::TIMEOPT;
  /* Handle -- after `time'. */
  if (last_read_token == parser::token::TIME && tokstr[0] == '-'
      && tokstr[1] == '-' && !tokstr[2])
    return parser::token::TIMEIGN;
  /* Handle -- after `time -p'. */
  if (last_read_token == parser::token::TIMEOPT && tokstr[0] == '-'
      && tokstr[1] == '-' && !tokstr[2])
    return parser::token::TIMEIGN;
#endif

#if defined(COND_COMMAND) /* [[ */
  if ((parser_state & PST_CONDEXPR) && tokstr[0] == ']' && tokstr[1] == ']'
      && tokstr[2] == '\0')
    return parser::token::COND_END;
#endif

  return -1;
}

/* Called from shell.cc when Control-C is typed at top level, or by the error
 * rule at top level. */
void
Shell::reset_parser ()
{
  dstack.clear (); /* No delimiters found so far. */
  open_brace_count = 0;

#if defined(EXTENDED_GLOB)
  /* Reset to global value of extended glob */
  if (parser_state & PST_EXTPAT)
    extended_glob = global_extglob;
#endif

  parser_state = PST_NOFLAGS;
  here_doc_first_line = 0;

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  if (pushed_string_list)
    free_string_list ();
#endif /* ALIAS || DPAREN_ARITHMETIC */

  /* This is where we resynchronize to the next newline on error/reset */
  shell_input_line.clear ();
  shell_input_line_index = 0;

  delete word_desc_to_read;
  word_desc_to_read = nullptr;

  eol_ungetc_lookahead = 0;

  current_token = '\n'; /* XXX */
  last_read_token = '\n';
  token_to_read = '\n';
}

/* Read the next token.  Command can be READ (normal operation) or
   RESET (to normalize state). */
parser::symbol_type
Shell::read_token (int command)
{
  int character; /* Current character. */
  int peek_char; /* Temporary look-ahead character. */
  int result;    /* The thing to return. */

  if (command == RESET)
    {
      reset_parser ();
      return parser::symbol_type ('\n');
    }

  if (token_to_read)
    {
      result = token_to_read;
      token_to_read = 0;
      if (token_to_read == parser::token::WORD)
        {
          parser::symbol_type word = parser::make_WORD (word_desc_to_read);
          word_desc_to_read = nullptr;
          return word;
        }
      else if (token_to_read == parser::token::ASSIGNMENT_WORD)
        {
          parser::symbol_type word
              = parser::make_ASSIGNMENT_WORD (word_desc_to_read);
          word_desc_to_read = nullptr;
          return word;
        }

      return parser::symbol_type (result);
    }

#if defined(COND_COMMAND)
  if ((parser_state & (PST_CONDCMD | PST_CONDEXPR)) == PST_CONDCMD)
    {
      cond_lineno = line_number;
      parser_state |= PST_CONDEXPR;
      COMMAND *cond_command = parse_cond_command ();
      if (cond_token != parser::token::COND_END)
        {
          cond_error ();
          return -1;
        }
      token_to_read = parser::token::COND_END;
      parser_state &= ~(PST_CONDEXPR | PST_CONDCMD);
      return parser::make_COND_CMD (cond_command);
    }
#endif

#if defined(ALIAS)
  /* This is a place to jump back to once we have successfully expanded a
     token with an alias and pushed the string with push_string () */
re_read_token:
#endif /* ALIAS */

  /* Read a single word from input.  Start by skipping blanks. */
  while ((character = shell_getc (1)) != EOF && shellblank (character))
    ;

  if (character == EOF)
    {
      EOF_Reached = true;
      return parser::make_yacc_EOF ();
    }

  /* If we hit the end of the string and we're not expanding an alias (e.g.,
     we are eval'ing a string that is an incomplete command), return EOF */
  if (character == '\0' && bash_input.type == st_string
      && !parser_expanding_alias ())
    {
#if defined(DEBUG)
      itrace ("shell_getc: bash_input.location.string = `%s'",
              bash_input.location.string);
#endif
      EOF_Reached = true;
      return parser::make_yacc_EOF ();
    }

  if MBTEST (character == '#' && (!interactive || interactive_comments))
    {
      /* A comment.  Discard until EOL or EOF, and then return a newline. */
      parser_state |= PST_COMMENT;
      discard_until ('\n');
      shell_getc (0);
      parser_state &= ~PST_COMMENT;
      character = '\n'; /* this will take the next if statement and return. */
    }

  if (character == '\n')
    {
      /* If we're about to return an unquoted newline, we can go and collect
         the text of any pending here document. */
      if (need_here_doc)
        gather_here_documents ();

#if defined(ALIAS)
      parser_state &= ~PST_ALEXPNEXT;
#endif /* ALIAS */

      parser_state &= ~PST_ASSIGNOK;

      return parser::symbol_type (character);
    }

  if (parser_state & PST_REGEXP)
    goto tokword;

  /* Shell meta-characters. */
  if MBTEST (shellmeta (character) && ((parser_state & PST_DBLPAREN) == 0))
    {
#if defined(ALIAS)
      /* Turn off alias tokenization iff this character sequence would
         not leave us ready to read a command. */
      if (character == '<' || character == '>')
        parser_state &= ~PST_ALEXPNEXT;
#endif /* ALIAS */

      parser_state &= ~PST_ASSIGNOK;

      /* If we are parsing a command substitution and we have read a character
         that marks the end of it, don't bother to skip over quoted newlines
         when we read the next token. We're just interested in a character
         that will turn this into a two-character token, so we let the higher
         layers deal with quoted newlines following the command substitution.
       */
      if ((parser_state & PST_CMDSUBST) && character == shell_eof_token)
        peek_char = shell_getc (0);
      else
        peek_char = shell_getc (1);

      if (character == peek_char)
        {
          switch (character)
            {
            case '<':
              /* If '<' then we could be at "<<" or at "<<-".  We have to
                 look ahead one more character. */
              peek_char = shell_getc (1);
              if MBTEST (peek_char == '-')
                return parser::make_LESS_LESS_MINUS ();
              else if MBTEST (peek_char == '<')
                return parser::make_LESS_LESS_LESS ();
              else
                {
                  shell_ungetc (peek_char);
                  return parser::make_LESS_LESS ();
                }

            case '>':
              return parser::make_GREATER_GREATER ();

            case ';':
              parser_state |= PST_CASEPAT;
#if defined(ALIAS)
              parser_state &= ~PST_ALEXPNEXT;
#endif /* ALIAS */

              peek_char = shell_getc (1);
              if MBTEST (peek_char == '&')
                return parser::make_SEMI_SEMI_AND ();
              else
                {
                  shell_ungetc (peek_char);
                  return parser::make_SEMI_SEMI ();
                }

            case '&':
              return parser::make_AND_AND ();

            case '|':
              return parser::make_OR_OR ();

#if defined(DPAREN_ARITHMETIC) || defined(ARITH_FOR_COMMAND)
            case '(': /* ) */
              result = parse_dparen (character);
              if (result == -2)
                break;
              else
                return result;
#endif
            }
        }
      else if MBTEST (character == '<' && peek_char == '&')
        return parser::make_LESS_AND ();
      else if MBTEST (character == '>' && peek_char == '&')
        return parser::make_GREATER_AND ();
      else if MBTEST (character == '<' && peek_char == '>')
        return parser::make_LESS_GREATER ();
      else if MBTEST (character == '>' && peek_char == '|')
        return parser::make_GREATER_BAR ();
      else if MBTEST (character == '&' && peek_char == '>')
        {
          peek_char = shell_getc (1);
          if MBTEST (peek_char == '>')
            return parser::make_AND_GREATER_GREATER ();
          else
            {
              shell_ungetc (peek_char);
              return parser::make_AND_GREATER ();
            }
        }
      else if MBTEST (character == '|' && peek_char == '&')
        return parser::make_BAR_AND ();
      else if MBTEST (character == ';' && peek_char == '&')
        {
          parser_state |= PST_CASEPAT;
#if defined(ALIAS)
          parser_state &= ~PST_ALEXPNEXT;
#endif /* ALIAS */
          return parser::make_SEMI_AND ();
        }

      shell_ungetc (peek_char);

      /* If we look like we are reading the start of a function
         definition, then let the reader know about it so that
         we will do the right thing with `{'. */
      if MBTEST (character == ')' && last_read_token == '('
                 && token_before_that == parser::token::WORD)
        {
          parser_state |= PST_ALLOWOPNBRC;
#if defined(ALIAS)
          parser_state &= ~PST_ALEXPNEXT;
#endif /* ALIAS */
          function_dstart = line_number;
        }

      /* case pattern lists may be preceded by an optional left paren.  If
         we're not trying to parse a case pattern list, the left paren
         indicates a subshell. */
      if MBTEST (character == '(' && (parser_state & PST_CASEPAT) == 0) /* ) */
        parser_state |= PST_SUBSHELL;
      /*(*/
      else if MBTEST ((parser_state & PST_CASEPAT) && character == ')')
        parser_state &= ~PST_CASEPAT;
      /*(*/
      else if MBTEST ((parser_state & PST_SUBSHELL) && character == ')')
        parser_state &= ~PST_SUBSHELL;

#if defined(PROCESS_SUBSTITUTION)
      /* Check for the constructs which introduce process substitution.
         Shells running in `posix mode' don't do process substitution. */
      if MBTEST ((character != '>' && character != '<')
                 || peek_char != '(') /*)*/
#endif                                /* PROCESS_SUBSTITUTION */
        return parser::symbol_type (character);
    }

  /* Hack <&- (close stdin) case.  Also <&N- (dup and close). */
  if MBTEST (character == '-'
             && (last_read_token == parser::token::LESS_AND
                 || last_read_token == parser::token::GREATER_AND))
    return parser::symbol_type (character);

tokword:

#if defined(ALIAS)
  try
    {
#endif
      /* Okay, if we got this far, we have to read a word.  Read one,
         and then check it against the known ones. */
      parser::symbol_type symbol = read_token_word (character);
      return symbol;

#if defined(ALIAS)
    }
  catch (const read_again_exception &)
    {
      goto re_read_token;
    }
#endif
}

#define COMSUB_META(ch) ((ch) == ';' || (ch) == '&' || (ch) == '|')

#if 0
#define CHECK_NESTRET_ERROR()                                                 \
  do                                                                          \
    {                                                                         \
      if (nestret == &matched_pair_error)                                     \
        {                                                                     \
          free (ret);                                                         \
          return &matched_pair_error;                                         \
        }                                                                     \
    }                                                                         \
  while (0)

#define APPEND_NESTRET()                                                      \
  do                                                                          \
    {                                                                         \
      if (nestlen)                                                            \
        {                                                                     \
          RESIZE_MALLOCED_BUFFER (ret, retind, nestlen, retsize, 64);         \
          strcpy (ret + retind, nestret);                                     \
          retind += nestlen;                                                  \
        }                                                                     \
    }                                                                         \
  while (0)

// static char matched_pair_error;
#endif

/*
 * Match a $(...) or other grouping construct.  This has to handle embedded
 * quoted strings ('', ``, "") and nested constructs.  It also must handle
 * reprompting the user, if necessary, after reading a newline, and returning
 * correct error values if it reads EOF.
 */
std::string
Shell::parse_matched_pair (
    int qc, /* `"' if this construct is within double quotes */
    int open, int close, pgroup_flags flags)
{
  int count, ch, prevch;
  int start_lineno;
  //   char *ret, *nestret, *ttrans;
  //   int retind, retsize;
  pgroup_flags rflags;
  dolbrace_state_t dolbrace_state;

  dolbrace_state = (flags & P_DOLBRACE) ? DOLBRACE_PARAM : DOLBRACE_NOFLAGS;

  /*itrace("parse_matched_pair[%d]: open = %c close = %c flags = %d",
   * line_number, open, close, flags);*/
  count = 1;
  lexical_state_flags tflags = LEX_NOFLAGS;

  if ((flags & P_COMMAND) && qc != '`' && qc != '\'' && qc != '"'
      && (flags & P_DQUOTE) == 0)
    tflags |= LEX_CKCOMMENT;

  /* RFLAGS is the set of flags we want to pass to recursive calls. */
  rflags = (qc == '"') ? P_DQUOTE : (flags & P_DQUOTE);

  std::string ret;

  start_lineno = line_number;
  ch = EOF; /* just in case */
  while (count)
    {
      prevch = ch;
      ch = shell_getc (qc != '\'' && (tflags & (LEX_PASSNEXT)) == 0);

      if (ch == EOF)
        {
          parser_error (start_lineno,
                        _ ("unexpected EOF while looking for matching `%c'"),
                        close);
          EOF_Reached = true; /* XXX */
          throw matched_pair_error ();
        }

      /* Possible reprompting. */
      if (ch == '\n' && SHOULD_PROMPT ())
        prompt_again ();

      /* Don't bother counting parens or doing anything else if in a comment
         or part of a case statement */
      if (tflags & LEX_INCOMMENT)
        {
          /* Add this character. */
          ret.push_back (static_cast<char> (ch));

          if (ch == '\n')
            tflags &= ~LEX_INCOMMENT;

          continue;
        }

      /* Not exactly right yet, should handle shell metacharacters, too.  If
         any changes are made to this test, make analogous changes to subst.c:
         extract_delimited_string(). */
      else if MBTEST ((tflags & LEX_CKCOMMENT) && (tflags & LEX_INCOMMENT) == 0
                      && ch == '#'
                      && (ret.empty () || ret[ret.size () - 1] == '\n'
                          || shellblank (ret[ret.size () - 1])))
        tflags |= LEX_INCOMMENT;

      if (tflags & LEX_PASSNEXT) /* last char was backslash */
        {
          tflags &= ~LEX_PASSNEXT;
          if (qc != '\''
              && ch == '\n') /* double-quoted \<newline> disappears. */
            {
              // swallow previously-added backslash
              if (!ret.empty ())
                ret.erase (ret.size () - 1, 1);
              continue;
            }

          if MBTEST (ch == CTLESC)
            ret.push_back (CTLESC);

          ret.push_back (static_cast<char> (ch));
          continue;
        }
      /* If we're reparsing the input (e.g., from parse_string_to_word_list),
         we've already prepended CTLESC to single-quoted results of $'...'.
         We may want to do this for other CTLESC-quoted characters in
         reparse, too. */
      else if MBTEST ((parser_state & PST_REPARSE) && open == '\''
                      && (ch == CTLESC || ch == CTLNUL))
        {
          ret.push_back (static_cast<char> (ch));
          continue;
        }
      else if MBTEST (ch == CTLESC || ch == CTLNUL) /* special shell escapes */
        {
          ret.push_back (CTLESC);
          ret.push_back (static_cast<char> (ch));
          continue;
        }
      else if MBTEST (ch == close) /* ending delimiter */
        count--;
      /* handle nested ${...} specially. */
      else if MBTEST (open != close && (tflags & LEX_WASDOL) && open == '{'
                      && ch == open) /* } */
        count++;
      else if MBTEST (((flags & P_FIRSTCLOSE) == 0)
                      && ch == open) /* nested begin */
        count++;

      /* Add this character. */
      ret.push_back (static_cast<char> (ch));

      /* If we just read the ending character, don't bother continuing. */
      if (count == 0)
        break;

      if (open == '\'') /* '' inside grouping construct */
        {
          if MBTEST ((flags & P_ALLOWESC) && ch == '\\')
            tflags |= LEX_PASSNEXT;
          continue;
        }

      if MBTEST (ch == '\\') /* backslashes */
        tflags |= LEX_PASSNEXT;

      /* Based on which dolstate is currently in (param, op, or word),
         decide what the op is.  We're really only concerned if it's % or
         #, so we can turn on a flag that says whether or not we should
         treat single quotes as special when inside a double-quoted
         ${...}. This logic must agree with subst.c:extract_dollar_brace_string
         since they share the same defines. */
      /* FLAG POSIX INTERP 221 */
      if (flags & P_DOLBRACE)
        {
          /* ${param%[%]word} */
          if MBTEST (dolbrace_state == DOLBRACE_PARAM && ch == '%'
                     && ret.size () > 1)
            dolbrace_state = DOLBRACE_QUOTE;
          /* ${param#[#]word} */
          else if MBTEST (dolbrace_state == DOLBRACE_PARAM && ch == '#'
                          && ret.size () > 1)
            dolbrace_state = DOLBRACE_QUOTE;
          /* ${param/[/]pat/rep} */
          else if MBTEST (dolbrace_state == DOLBRACE_PARAM && ch == '/'
                          && ret.size () > 1)
            dolbrace_state = DOLBRACE_QUOTE2; /* XXX */
                                              /* ${param^[^]pat} */
          else if MBTEST (dolbrace_state == DOLBRACE_PARAM && ch == '^'
                          && ret.size () > 1)
            dolbrace_state = DOLBRACE_QUOTE;
          /* ${param,[,]pat} */
          else if MBTEST (dolbrace_state == DOLBRACE_PARAM && ch == ','
                          && ret.size () > 1)
            dolbrace_state = DOLBRACE_QUOTE;
          else if MBTEST (dolbrace_state == DOLBRACE_PARAM
                          && std::strchr ("#%^,~:-=?+/", ch) != nullptr)
            dolbrace_state = DOLBRACE_OP;
          else if MBTEST (dolbrace_state == DOLBRACE_OP
                          && std::strchr ("#%^,~:-=?+/", ch) == nullptr)
            dolbrace_state = DOLBRACE_WORD;
        }

      /* The big hammer.  Single quotes aren't special in double quotes.  The
         problem is that Posix used to say the single quotes are semi-special:
         within a double-quoted ${...} construct "an even number of
         unescaped double-quotes or single-quotes, if any, shall occur." */
      /* This was changed in Austin Group Interp 221 */
      if MBTEST (posixly_correct && shell_compatibility_level > 41
                 && dolbrace_state != DOLBRACE_QUOTE
                 && dolbrace_state != DOLBRACE_QUOTE2 && (flags & P_DQUOTE)
                 && (flags & P_DOLBRACE) && ch == '\'')
        continue;

      /* Could also check open == '`' if we want to parse grouping constructs
         inside old-style command substitution. */
      if (open != close) /* a grouping construct */
        {
          if MBTEST (shellquote (ch))
            {
              std::string nestret;
              dstack.push_back (static_cast<char> (ch));

              /* '', ``, or "" inside $(...) or other grouping construct. */
              try
                {
                  if MBTEST ((tflags & LEX_WASDOL)
                             && ch == '\'') /* $'...' inside group */
                    nestret = parse_matched_pair (ch, ch, ch,
                                                  (P_ALLOWESC | rflags));
                  else
                    nestret = parse_matched_pair (ch, ch, ch, rflags);
                }
              catch (const matched_pair_error &)
                {
                  dstack.pop_back ();
                  throw;
                }

              dstack.pop_back ();

              if MBTEST ((tflags & LEX_WASDOL) && ch == '\''
                         && (extended_quote || (rflags & P_DQUOTE) == 0))
                {
                  /* Translate $'...' here. */
                  std::string ttrans
                      = ansiexpand (nestret.begin (), nestret.end ());
                  nestret.clear ();

                  /* If we're parsing a double-quoted brace expansion and we
                     are not in a place where single quotes are treated
                     specially, make sure we single-quote the results of the
                     ansi expansion because quote removal should remove them
                     later */
                  /* FLAG POSIX INTERP 221 */
                  if ((shell_compatibility_level > 42) && (rflags & P_DQUOTE)
                      && (dolbrace_state == DOLBRACE_QUOTE2)
                      && (flags & P_DOLBRACE))
                    {
                      nestret = sh_single_quote (ttrans);
                      free (ttrans);
                      nestlen = strlen (nestret);
                    }
                  else if ((rflags & P_DQUOTE) == 0)
                    {
                      nestret = sh_single_quote (ttrans);
                      free (ttrans);
                      nestlen = strlen (nestret);
                    }
                  else
                    {
                      nestret = ttrans;
                      nestlen = ttranslen;
                    }
                  retind -= 2; /* back up before the $' */
                }
              else if MBTEST ((tflags & LEX_WASDOL) && ch == '"'
                              && (extended_quote || (rflags & P_DQUOTE) == 0))
                {
                  /* Locale expand $"..." here. */
                  ttrans = localeexpand (nestret, 0, nestlen - 1, start_lineno,
                                         &ttranslen);
                  free (nestret);

                  nestret = sh_mkdoublequoted (ttrans, ttranslen, 0);
                  free (ttrans);
                  nestlen = ttranslen + 2;
                  retind -= 2; /* back up before the $" */
                }

              APPEND_NESTRET ();
              FREE (nestret);
            }
          else if ((flags & (P_ARRAYSUB | P_DOLBRACE)) && (tflags & LEX_WASDOL)
                   && (ch == '(' || ch == '{' || ch == '[')) /* ) } ] */
            goto parse_dollar_word;
#if defined(PROCESS_SUBSTITUTION)
          /* XXX - technically this should only be recognized at the start of
             a word */
          else if ((flags & (P_ARRAYSUB | P_DOLBRACE)) && (tflags & LEX_GTLT)
                   && (ch == '(')) /* ) */
            goto parse_dollar_word;
#endif
        }
      /* Parse an old-style command substitution within double quotes as a
         single word. */
      /* XXX - sh and ksh93 don't do this - XXX */
      else if MBTEST (open == '"' && ch == '`')
        {
          nestret = parse_matched_pair (0, '`', '`', &nestlen, rflags);

          CHECK_NESTRET_ERROR ();
          APPEND_NESTRET ();

          FREE (nestret);
        }
      else if MBTEST (open != '`' && (tflags & LEX_WASDOL)
                      && (ch == '(' || ch == '{' || ch == '[')) /* ) } ] */
        /* check for $(), $[], or ${} inside quoted string. */
        {
        parse_dollar_word:
          if (open == ch) /* undo previous increment */
            count--;
          if (ch == '(') /* ) */
            nestret = parse_comsub (0, '(', ')', &nestlen,
                                    (rflags | P_COMMAND) & ~P_DQUOTE);
          else if (ch == '{') /* } */
            nestret = parse_matched_pair (
                0, '{', '}', (P_FIRSTCLOSE | P_DOLBRACE | rflags));
          else if (ch == '[') /* ] */
            nestret = parse_matched_pair (0, '[', ']', &nestlen, rflags);

          CHECK_NESTRET_ERROR ();
          APPEND_NESTRET ();

          FREE (nestret);
        }
#if defined(PROCESS_SUBSTITUTION)
      if MBTEST ((ch == '<' || ch == '>') && (tflags & LEX_GTLT) == 0)
        tflags |= LEX_GTLT;
      else
        tflags &= ~LEX_GTLT;
#endif
      if MBTEST (ch == '$' && (tflags & LEX_WASDOL) == 0)
        tflags |= LEX_WASDOL;
      else
        tflags &= ~LEX_WASDOL;
    }

  /*itrace("parse_matched_pair[%d]: returning %s", line_number, ret.c_str
   * ());*/
  return ret;
}

#if defined(DEBUG)
static void
dump_tflags (lexical_state_flags flags)
{
  int f;

  f = flags;
  fprintf (stderr, "%d -> ", f);
  if (f & LEX_WASDOL)
    {
      f &= ~LEX_WASDOL;
      fprintf (stderr, "LEX_WASDOL%s", f ? "|" : "");
    }
  if (f & LEX_CKCOMMENT)
    {
      f &= ~LEX_CKCOMMENT;
      fprintf (stderr, "LEX_CKCOMMENT%s", f ? "|" : "");
    }
  if (f & LEX_INCOMMENT)
    {
      f &= ~LEX_INCOMMENT;
      fprintf (stderr, "LEX_INCOMMENT%s", f ? "|" : "");
    }
  if (f & LEX_PASSNEXT)
    {
      f &= ~LEX_PASSNEXT;
      fprintf (stderr, "LEX_PASSNEXT%s", f ? "|" : "");
    }
  if (f & LEX_RESWDOK)
    {
      f &= ~LEX_RESWDOK;
      fprintf (stderr, "LEX_RESWDOK%s", f ? "|" : "");
    }
  if (f & LEX_CKCASE)
    {
      f &= ~LEX_CKCASE;
      fprintf (stderr, "LEX_CKCASE%s", f ? "|" : "");
    }
  if (f & LEX_INCASE)
    {
      f &= ~LEX_INCASE;
      fprintf (stderr, "LEX_INCASE%s", f ? "|" : "");
    }
  if (f & LEX_INHEREDOC)
    {
      f &= ~LEX_INHEREDOC;
      fprintf (stderr, "LEX_INHEREDOC%s", f ? "|" : "");
    }
  if (f & LEX_HEREDELIM)
    {
      f &= ~LEX_HEREDELIM;
      fprintf (stderr, "LEX_HEREDELIM%s", f ? "|" : "");
    }
  if (f & LEX_STRIPDOC)
    {
      f &= ~LEX_STRIPDOC;
      fprintf (stderr, "LEX_WASDOL%s", f ? "|" : "");
    }
  if (f & LEX_QUOTEDDOC)
    {
      f &= ~LEX_QUOTEDDOC;
      fprintf (stderr, "LEX_QUOTEDDOC%s", f ? "|" : "");
    }
  if (f & LEX_INWORD)
    {
      f &= ~LEX_INWORD;
      fprintf (stderr, "LEX_INWORD%s", f ? "|" : "");
    }

  fprintf (stderr, "\n");
  fflush (stderr);
}
#endif

/* Parse a $(...) command substitution.  This is messier than I'd like, and
   reproduces a lot more of the token-reading code than I'd like. */
std::string
Shell::parse_comsub (int qc, int open, int close, pgroup_flags flags)
{
  int count, ch, peekc, lex_rwlen, lex_wlen, lex_firstind;
  int nestlen, ttranslen, start_lineno, orig_histexp;
  char *ret, *nestret, *ttrans, *heredelim;
  int retind, retsize, rflags, hdlen;
  lexical_state_flags tflags;

  /* Posix interp 217 says arithmetic expressions have precedence, so
     assume $(( introduces arithmetic expansion and parse accordingly. */
  peekc = shell_getc (0);
  shell_ungetc (peekc);
  if (peekc == '(')
    return parse_matched_pair (qc, open, close, lenp, 0);

  /*itrace("parse_comsub: qc = `%c' open = %c close = %c", qc, open, close);*/
  count = 1;
  tflags = LEX_RESWDOK;
#if defined(BANG_HISTORY)
  orig_histexp = history_expansion_inhibited;
#endif

  if ((flags & P_COMMAND) && qc != '\'' && qc != '"'
      && (flags & P_DQUOTE) == 0)
    tflags |= LEX_CKCASE;
  if ((tflags & LEX_CKCASE) && (interactive == 0 || interactive_comments))
    tflags |= LEX_CKCOMMENT;

  /* RFLAGS is the set of flags we want to pass to recursive calls. */
  rflags = (flags & P_DQUOTE);

  ret = (char *)xmalloc (retsize = 64);
  retind = 0;

  start_lineno = line_number;
  lex_rwlen = lex_wlen = 0;

  heredelim = 0;
  lex_firstind = -1;

  while (count)
    {
    comsub_readchar:
      ch = shell_getc (
          qc != '\''
          && (tflags & (LEX_INCOMMENT | LEX_PASSNEXT | LEX_QUOTEDDOC)) == 0);

      if (ch == EOF)
        {
        eof_error:
#if defined(BANG_HISTORY)
          history_expansion_inhibited = orig_histexp;
#endif
          free (ret);
          FREE (heredelim);
          parser_error (start_lineno,
                        _ ("unexpected EOF while looking for matching `%c'"),
                        close);
          EOF_Reached = true; /* XXX */
          return &matched_pair_error;
        }

      /* If we hit the end of a line and are reading the contents of a here
         document, and it's not the same line that the document starts on,
         check for this line being the here doc delimiter.  Otherwise, if
         we're in a here document, mark the next character as the beginning
         of a line. */
      if (ch == '\n')
        {
          if ((tflags & LEX_HEREDELIM) && heredelim)
            {
              tflags &= ~LEX_HEREDELIM;
              tflags |= LEX_INHEREDOC;
#if defined(BANG_HISTORY)
              history_expansion_inhibited = 1;
#endif
              lex_firstind = retind + 1;
            }
          else if (tflags & LEX_INHEREDOC)
            {
              int tind;
              tind = lex_firstind;
              while ((tflags & LEX_STRIPDOC) && ret[tind] == '\t')
                tind++;
              if (retind - tind == hdlen
                  && STREQN (ret + tind, heredelim, hdlen))
                {
                  tflags &= ~(LEX_STRIPDOC | LEX_INHEREDOC | LEX_QUOTEDDOC);
                  /*itrace("parse_comsub:%d: found here doc end `%s'",
                   * line_number, ret + tind);*/
                  free (heredelim);
                  heredelim = 0;
                  lex_firstind = -1;
#if defined(BANG_HISTORY)
                  history_expansion_inhibited = orig_histexp;
#endif
                }
              else
                lex_firstind = retind + 1;
            }
        }

      /* Possible reprompting. */
      if (ch == '\n' && SHOULD_PROMPT ())
        prompt_again ();

      /* XXX -- we currently allow here doc to be delimited by ending right
         paren in default mode and posix mode. To change posix mode, change
         the #if 1 to #if 0 below */
      if ((tflags & LEX_INHEREDOC) && ch == close && count == 1)
        {
          int tind;
          /*itrace("parse_comsub:%d: in here doc, ch == close, retind -
           * firstind = %d hdlen = %d retind = %d", line_number,
           * retind-lex_firstind, hdlen, retind);*/
          tind = lex_firstind;
          while ((tflags & LEX_STRIPDOC) && ret[tind] == '\t')
            tind++;
#if 1
          if (retind - tind == hdlen && STREQN (ret + tind, heredelim, hdlen))
#else
          /* Posix-mode shells require the newline after the here-document
             delimiter. */
          if (retind - tind == hdlen && STREQN (ret + tind, heredelim, hdlen)
              && posixly_correct == 0)
#endif
            {
              tflags &= ~(LEX_STRIPDOC | LEX_INHEREDOC | LEX_QUOTEDDOC);
              /*itrace("parse_comsub:%d: found here doc end `%*s'",
               * line_number, hdlen, ret + tind);*/
              free (heredelim);
              heredelim = 0;
              lex_firstind = -1;
#if defined(BANG_HISTORY)
              history_expansion_inhibited = orig_histexp;
#endif
            }
        }

      /* Don't bother counting parens or doing anything else if in a comment or
         here document (not exactly right for here-docs -- if we want to allow
         recursive calls to parse_comsub to have their own here documents,
         change the LEX_INHEREDOC to LEX_QUOTEDDOC here and uncomment the next
         clause below.  Note that to make this work completely, we need to make
         additional changes to allow xparse_dolparen to work right when the
         command substitution is parsed, because read_secondary_line doesn't
         know to recursively parse through command substitutions embedded in
         here- documents */
      if (tflags & (LEX_INCOMMENT | LEX_INHEREDOC))
        {
          /* Add this character. */
          ret[retind++] = ch;

          if ((tflags & LEX_INCOMMENT) && ch == '\n')
            {
              /*itrace("parse_comsub:%d: lex_incomment -> 0 ch = `%c'",
               * line_number, ch);*/
              tflags &= ~LEX_INCOMMENT;
            }

          continue;
        }

      if (tflags & LEX_PASSNEXT) /* last char was backslash */
        {
          /*itrace("parse_comsub:%d: lex_passnext -> 0 ch = `%c' (%d)",
           * line_number, ch, __LINE__);*/
          tflags &= ~LEX_PASSNEXT;
          if (qc != '\''
              && ch == '\n') /* double-quoted \<newline> disappears. */
            {
              if (retind > 0)
                retind--; /* swallow previously-added backslash */
              continue;
            }

          if MBTEST (ch == CTLESC)
            ret[retind++] = CTLESC;
          ret[retind++] = ch;
          continue;
        }

      /* If this is a shell break character, we are not in a word.  If not,
         we either start or continue a word. */
      if MBTEST (shellbreak (ch))
        {
          tflags &= ~LEX_INWORD;
          /*itrace("parse_comsub:%d: lex_inword -> 0 ch = `%c' (%d)",
           * line_number, ch, __LINE__);*/
        }
      else
        {
          if (tflags & LEX_INWORD)
            {
              lex_wlen++;
              /*itrace("parse_comsub:%d: lex_inword == 1 ch = `%c' lex_wlen =
               * %d (%d)", line_number, ch, lex_wlen, __LINE__);*/
            }
          else
            {
              /*itrace("parse_comsub:%d: lex_inword -> 1 ch = `%c' (%d)",
               * line_number, ch, __LINE__);*/
              tflags |= LEX_INWORD;
              lex_wlen = 0;
              if (tflags & LEX_RESWDOK)
                lex_rwlen = 0;
            }
        }

      /* Skip whitespace */
      if MBTEST (shellblank (ch) && (tflags & LEX_HEREDELIM) == 0
                 && lex_rwlen == 0)
        {
          /* Add this character. */
          ret[retind++] = ch;
          continue;
        }

      /* Either we are looking for the start of the here-doc delimiter
         (lex_firstind == -1) or we are reading one (lex_firstind >= 0).
         If this character is a shell break character and we are reading
         the delimiter, save it and note that we are now reading a here
         document.  If we've found the start of the delimiter, note it by
         setting lex_firstind.  Backslashes can quote shell metacharacters
         in here-doc delimiters. */
      if (tflags & LEX_HEREDELIM)
        {
          if (lex_firstind == -1 && shellbreak (ch) == 0)
            lex_firstind = retind;
          else if (lex_firstind >= 0 && (tflags & LEX_PASSNEXT) == 0
                   && shellbreak (ch))
            {
              if (heredelim == 0)
                {
                  nestret = substring (ret, lex_firstind, retind);
                  heredelim = string_quote_removal (nestret, 0);
                  hdlen = STRLEN (heredelim);
                  /*itrace("parse_comsub:%d: found here doc delimiter `%s'
                   * (%d)", line_number, heredelim, hdlen);*/
                  if (STREQ (heredelim, nestret) == 0)
                    tflags |= LEX_QUOTEDDOC;
                  free (nestret);
                }
              if (ch == '\n')
                {
                  tflags |= LEX_INHEREDOC;
                  tflags &= ~LEX_HEREDELIM;
                  lex_firstind = retind + 1;
#if defined(BANG_HISTORY)
                  history_expansion_inhibited = 1;
#endif
                }
              else
                lex_firstind = -1;
            }
        }

      /* Meta-characters that can introduce a reserved word.  Not perfect yet.
       */
      if MBTEST ((tflags & LEX_RESWDOK) == 0 && (tflags & LEX_CKCASE)
                 && (tflags & LEX_INCOMMENT) == 0
                 && (shellmeta (ch) || ch == '\n'))
        {
          /* Add this character. */
          ret[retind++] = ch;
          peekc = shell_getc (1);
          if (ch == peekc
              && (ch == '&' || ch == '|'
                  || ch == ';')) /* two-character tokens */
            {
              ret[retind++] = peekc;
              /*itrace("parse_comsub:%d: set lex_reswordok = 1, ch = `%c'",
               * line_number, ch);*/
              tflags |= LEX_RESWDOK;
              lex_rwlen = 0;
              continue;
            }
          else if (ch == '\n' || COMSUB_META (ch))
            {
              shell_ungetc (peekc);
              /*itrace("parse_comsub:%d: set lex_reswordok = 1, ch = `%c'",
               * line_number, ch);*/
              tflags |= LEX_RESWDOK;
              lex_rwlen = 0;
              continue;
            }
          else if (ch == EOF)
            goto eof_error;
          else
            {
              /* `unget' the character we just added and fall through */
              retind--;
              shell_ungetc (peekc);
            }
        }

      /* If we can read a reserved word, try to read one. */
      if (tflags & LEX_RESWDOK)
        {
          if MBTEST (islower ((unsigned char)ch))
            {
              /* Add this character. */
              ret[retind++] = ch;
              lex_rwlen++;
              continue;
            }
          else if MBTEST (lex_rwlen == 4 && shellbreak (ch))
            {
              if (STREQN (ret + retind - 4, "case", 4))
                {
                  tflags |= LEX_INCASE;
                  tflags &= ~LEX_RESWDOK;
                  /*itrace("parse_comsub:%d: found `case', lex_incase -> 1
                   * lex_reswdok -> 0", line_number);*/
                }
              else if (STREQN (ret + retind - 4, "esac", 4))
                {
                  tflags &= ~LEX_INCASE;
                  /*itrace("parse_comsub:%d: found `esac', lex_incase -> 0
                   * lex_reswdok -> 1", line_number);*/
                  tflags |= LEX_RESWDOK;
                  lex_rwlen = 0;
                }
              else if (STREQN (ret + retind - 4, "done", 4)
                       || STREQN (ret + retind - 4, "then", 4)
                       || STREQN (ret + retind - 4, "else", 4)
                       || STREQN (ret + retind - 4, "elif", 4)
                       || STREQN (ret + retind - 4, "time", 4))
                {
                  /* these are four-character reserved words that can be
                     followed by a reserved word; anything else turns off
                     the reserved-word-ok flag */
                  /*itrace("parse_comsub:%d: found `%.4s', lex_reswdok -> 1",
                   * line_number, ret+retind-4);*/
                  tflags |= LEX_RESWDOK;
                  lex_rwlen = 0;
                }
              else if (shellmeta (ch) == 0)
                {
                  tflags &= ~LEX_RESWDOK;
                  /*itrace("parse_comsub:%d: found `%.4s', lex_reswdok -> 0",
                   * line_number, ret+retind-4);*/
                }
              else /* can't be in a reserved word any more */
                lex_rwlen = 0;
            }
          else if MBTEST ((tflags & LEX_CKCOMMENT) && ch == '#'
                          && (lex_rwlen == 0
                              || ((tflags & LEX_INWORD) && lex_wlen == 0)))
            ; /* don't modify LEX_RESWDOK if we're starting a comment */
          /* Allow `do' followed by space, tab, or newline to preserve the
             RESWDOK flag, but reset the reserved word length counter so we
             can read another one. */
          else if MBTEST (((tflags & LEX_INCASE) == 0)
                          && (isblank ((unsigned char)ch) || ch == '\n')
                          && lex_rwlen == 2
                          && STREQN (ret + retind - 2, "do", 2))
            {
              /*itrace("parse_comsub:%d: lex_incase == 0 found `%c', found
               * \"do\"", line_number, ch);*/
              lex_rwlen = 0;
            }
          else if MBTEST ((tflags & LEX_INCASE) && ch != '\n')
            /* If we can read a reserved word and we're in case, we're at the
               point where we can read a new pattern list or an esac.  We
               handle the esac case above.  If we read a newline, we want to
               leave LEX_RESWDOK alone.  If we read anything else, we want to
               turn off LEX_RESWDOK, since we're going to read a pattern list.
             */
            {
              tflags &= ~LEX_RESWDOK;
              /*itrace("parse_comsub:%d: lex_incase == 1 found `%c',
               * lex_reswordok -> 0", line_number, ch);*/
            }
          else if MBTEST (shellbreak (ch) == 0)
            {
              tflags &= ~LEX_RESWDOK;
              /*itrace("parse_comsub:%d: found `%c', lex_reswordok -> 0",
               * line_number, ch);*/
            }
#if 0
	  /* If we find a space or tab but have read something and it's not
	     `do', turn off the reserved-word-ok flag */
	  else if MBTEST(isblank ((unsigned char)ch) && lex_rwlen > 0)
	    {
	      tflags &= ~LEX_RESWDOK;
/*itrace("parse_comsub:%d: found `%c', lex_reswordok -> 0", line_number, ch);*/
	    }
#endif
        }

      /* Might be the start of a here-doc delimiter */
      if MBTEST ((tflags & LEX_INCOMMENT) == 0 && (tflags & LEX_CKCASE)
                 && ch == '<')
        {
          /* Add this character. */
          ret[retind++] = ch;
          peekc = shell_getc (1);
          if (peekc == EOF)
            goto eof_error;
          if (peekc == ch)
            {
              ret[retind++] = peekc;
              peekc = shell_getc (1);
              if (peekc == EOF)
                goto eof_error;
              if (peekc == '-')
                {
                  ret[retind++] = peekc;
                  tflags |= LEX_STRIPDOC;
                }
              else
                shell_ungetc (peekc);
              if (peekc != '<')
                {
                  tflags |= LEX_HEREDELIM;
                  lex_firstind = -1;
                }
              continue;
            }
          else
            {
              shell_ungetc (peekc); /* not a here-doc, start over */
              continue;
            }
        }
      else if MBTEST ((tflags & LEX_CKCOMMENT) && (tflags & LEX_INCOMMENT) == 0
                      && ch == '#'
                      && (((tflags & LEX_RESWDOK) && lex_rwlen == 0)
                          || ((tflags & LEX_INWORD) && lex_wlen == 0)))
        {
          /*itrace("parse_comsub:%d: lex_incomment -> 1 (%d)", line_number,
           * __LINE__);*/
          tflags |= LEX_INCOMMENT;
        }

      if MBTEST (ch == CTLESC || ch == CTLNUL) /* special shell escapes */
        {
          ret[retind++] = CTLESC;
          ret[retind++] = ch;
          continue;
        }
      else if MBTEST (ch == close
                      && (tflags & LEX_INCASE) == 0) /* ending delimiter */
        {
          count--;
          /*itrace("parse_comsub:%d: found close: count = %d", line_number,
           * count);*/
        }
      else if MBTEST (((flags & P_FIRSTCLOSE) == 0)
                      && (tflags & LEX_INCASE) == 0
                      && ch == open) /* nested begin */
        {
          count++;
          /*itrace("parse_comsub:%d: found open: count = %d", line_number,
           * count);*/
        }

      /* Add this character. */
      ret[retind++] = ch;

      /* If we just read the ending character, don't bother continuing. */
      if (count == 0)
        break;

      if MBTEST (ch == '\\') /* backslashes */
        tflags |= LEX_PASSNEXT;

      if MBTEST (shellquote (ch))
        {
          /* '', ``, or "" inside $(...). */
          push_delimiter (dstack, ch);
          if MBTEST ((tflags & LEX_WASDOL)
                     && ch == '\'') /* $'...' inside group */
            nestret = parse_matched_pair (ch, ch, ch, &nestlen,
                                          (P_ALLOWESC | rflags));
          else
            nestret = parse_matched_pair (ch, ch, ch, &nestlen, rflags);
          pop_delimiter (dstack);
          CHECK_NESTRET_ERROR ();

          if MBTEST ((tflags & LEX_WASDOL) && ch == '\''
                     && (extended_quote || (rflags & P_DQUOTE) == 0))
            {
              /* Translate $'...' here. */
              ttrans = ansiexpand (nestret, 0, nestlen - 1, &ttranslen);
              free (nestret);

              if ((rflags & P_DQUOTE) == 0)
                {
                  nestret = sh_single_quote (ttrans);
                  free (ttrans);
                  nestlen = strlen (nestret);
                }
              else
                {
                  nestret = ttrans;
                  nestlen = ttranslen;
                }
              retind -= 2; /* back up before the $' */
            }
          else if MBTEST ((tflags & LEX_WASDOL) && ch == '"'
                          && (extended_quote || (rflags & P_DQUOTE) == 0))
            {
              /* Locale expand $"..." here. */
              ttrans = localeexpand (nestret, 0, nestlen - 1, start_lineno,
                                     &ttranslen);
              free (nestret);

              nestret = sh_mkdoublequoted (ttrans, ttranslen, 0);
              free (ttrans);
              nestlen = ttranslen + 2;
              retind -= 2; /* back up before the $" */
            }

          APPEND_NESTRET ();
          FREE (nestret);
        }
      else if MBTEST ((tflags & LEX_WASDOL)
                      && (ch == '(' || ch == '{' || ch == '[')) /* ) } ] */
        /* check for $(), $[], or ${} inside command substitution. */
        {
          if ((tflags & LEX_INCASE) == 0
              && open == ch) /* undo previous increment */
            count--;
          if (ch == '(') /* ) */
            nestret = parse_comsub (0, '(', ')', &nestlen,
                                    (rflags | P_COMMAND) & ~P_DQUOTE);
          else if (ch == '{') /* } */
            nestret = parse_matched_pair (
                0, '{', '}', &nestlen, (P_FIRSTCLOSE | P_DOLBRACE | rflags));
          else if (ch == '[') /* ] */
            nestret = parse_matched_pair (0, '[', ']', &nestlen, rflags);

          CHECK_NESTRET_ERROR ();
          APPEND_NESTRET ();

          FREE (nestret);
        }
      if MBTEST (ch == '$' && (tflags & LEX_WASDOL) == 0)
        tflags |= LEX_WASDOL;
      else
        tflags &= ~LEX_WASDOL;
    }

#if defined(BANG_HISTORY)
  history_expansion_inhibited = orig_histexp;
#endif
  FREE (heredelim);
  ret[retind] = '\0';
  if (lenp)
    *lenp = retind;
  /*itrace("parse_comsub:%d: returning `%s'", line_number, ret);*/
  return ret;
}

/* Recursively call the parser to parse a $(...) command substitution. */
std::string
Shell::xparse_dolparen (const std::string &base, size_t indp, sx_flags flags)
{
  sh_parser_state_t ps;
  sh_input_line_state_t ls;
  int orig_ind, nc, orig_eof_token, start_lineno;
//   char *ret, *ep, *ostring;
#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  STRING_SAVER *saved_pushed_strings;
#endif

  /*debug_parser(1);*/
  orig_ind = *indp;
  ostring = string;
  start_lineno = line_number;

  if (*string == 0)
    {
      if (flags & SX_NOALLOC)
        return nullptr;

      ret = (char *)xmalloc (1);
      ret[0] = '\0';
      return ret;
    }

  /*itrace("xparse_dolparen: size = %d shell_input_line = `%s' string=`%s'",
   * shell_input_line_size, shell_input_line, string);*/
  parse_flags sflags = SEVAL_NONINT | SEVAL_NOHIST | SEVAL_NOFREE;
  if (flags & SX_NOTHROW)
    sflags |= SEVAL_NOTHROW;
  save_parser_state (&ps);
  save_input_line_state (&ls);
  orig_eof_token = shell_eof_token;
#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  saved_pushed_strings = pushed_string_list; /* separate parsing context */
  pushed_string_list = nullptr;
#endif

  /*(*/
  parser_state |= (PST_CMDSUBST | PST_EOFTOKEN); /* allow instant ')' */ /*(*/
  shell_eof_token = ')';

  /* Should we save and restore the bison/yacc lookahead token (yychar) here?
     Or only if it's not YYEMPTY? */

  nc = parse_string (string, "command substitution", sflags, &ep);

  if (current_token == shell_eof_token)
    yyclearin; /* might want to clear lookahead token unconditionally */

  reset_parser ();
  /* reset_parser() clears shell_input_line and associated variables, including
     parser_state, so we want to reset things, then restore what we need. */
  restore_input_line_state (&ls);

  shell_eof_token = orig_eof_token;
  restore_parser_state (&ps);

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  pushed_string_list = saved_pushed_strings;
#endif

  token_to_read = 0;

  /* If parse_string returns < 0, we need to jump to top level with the
     negative of the return value. We abandon the rest of this input line
     first */
  if (nc < 0)
    {
      clear_shell_input_line ();        /* XXX */
      if (bash_input.type != st_string) /* paranoia */
        parser_state &= ~(PST_CMDSUBST | PST_EOFTOKEN);
      jump_to_top_level (-nc); /* XXX */
    }

  /* Need to find how many characters parse_and_execute consumed, update
     *indp, if flags != 0, copy the portion of the string parsed into RET
     and return it.  If flags & 1 (SX_NOALLOC) we can return nullptr. */

  /*(*/
  if (ep[-1] != ')')
    {
#if DEBUG
      if (ep[-1] != '\n')
        itrace ("xparse_dolparen:%d: ep[-1] != RPAREN (%d), ep = `%s'",
                line_number, ep[-1], ep);
#endif

      while (ep > ostring && ep[-1] == '\n')
        ep--;
    }

  nc = ep - ostring;
  *indp = ep - base - 1;

  /*((*/
#if DEBUG
  if (base[*indp] != ')')
    itrace ("xparse_dolparen:%d: base[%d] != RPAREN (%d), base = `%s'",
            line_number, *indp, base[*indp], base);
  if (*indp < orig_ind)
    itrace (
        "xparse_dolparen:%d: *indp (%d) < orig_ind (%d), orig_string = `%s'",
        line_number, *indp, orig_ind, ostring);
#endif

  if (base[*indp] != ')')
    {
      /*(*/
      parser_error (start_lineno,
                    _ ("unexpected EOF while looking for matching `%c'"), ')');
      jump_to_top_level (DISCARD);
    }

  if (flags & SX_NOALLOC)
    return nullptr;

  if (nc == 0)
    {
      ret = (char *)xmalloc (1);
      ret[0] = '\0';
    }
  else
    ret = substring (ostring, 0, nc - 1);

  return ret;
}

#if defined(DPAREN_ARITHMETIC) || defined(ARITH_FOR_COMMAND)
/* Parse a double-paren construct.  It can be either an arithmetic
   command, an arithmetic `for' command, or a nested subshell.  Returns
   the parsed token, -1 on error, or -2 if we didn't do anything and
   should just go on. */
int
Shell::parse_dparen (int c)
{
  int cmdtyp, sline;
  char *wval;
  WORD_DESC *wd;

#if defined(ARITH_FOR_COMMAND)
  if (last_read_token == parser::token::FOR)
    {
      arith_for_lineno = line_number;
      cmdtyp = parse_arith_cmd (&wval, 0);
      if (cmdtyp == 1)
        {
          wd = alloc_word_desc ();
          wd->word = wval;
          yylval.word_list = make_word_list (wd, nullptr);
          return parser::token::ARITH_FOR_EXPRS;
        }
      else
        return -1; /* ERROR */
    }
#endif

#if defined(DPAREN_ARITHMETIC)
  if (reserved_word_acceptable (last_read_token))
    {
      sline = line_number;

      cmdtyp = parse_arith_cmd (&wval, 0);
      if (cmdtyp == 1) /* arithmetic command */
        {
          wd = alloc_word_desc ();
          wd->word = wval;
          wd->flags = W_QUOTED | W_NOSPLIT | W_NOGLOB | W_DQUOTE;
          yylval.word_list = make_word_list (wd, nullptr);
          return parser::token::ARITH_CMD;
        }
      else if (cmdtyp == 0) /* nested subshell */
        {
          push_string (wval, 0, nullptr);
          pushed_string_list->flags = PSH_DPAREN;
          if ((parser_state & PST_CASEPAT) == 0)
            parser_state |= PST_SUBSHELL;
          return c;
        }
      else /* ERROR */
        return -1;
    }
#endif

  return -2; /* XXX */
}

/* We've seen a `(('.  Look for the matching `))'.  If we get it, return 1.
   If not, assume it's a nested subshell for backwards compatibility and
   return 0.  In any case, put the characters we've consumed into a locally-
   allocated buffer and make *ep point to that buffer.  Return -1 on an
   error, for example EOF. */
int
Shell::parse_arith_cmd (char **ep, int adddq)
{
  int exp_lineno, rval, c;
  char *ttok, *tokstr;
  int ttoklen;

  exp_lineno = line_number;
  ttok = parse_matched_pair (0, '(', ')', &ttoklen, 0);
  rval = 1;
  if (ttok == &matched_pair_error)
    return -1;
  /* Check that the next character is the closing right paren.  If
     not, this is a syntax error. ( */
  c = shell_getc (0);
  if MBTEST (c != ')')
    rval = 0;

  tokstr = (char *)xmalloc (ttoklen + 4);

  /* if ADDDQ != 0 then (( ... )) -> "..." */
  if (rval == 1 && adddq) /* arith cmd, add double quotes */
    {
      tokstr[0] = '"';
      strncpy (tokstr + 1, ttok, ttoklen - 1);
      tokstr[ttoklen] = '"';
      tokstr[ttoklen + 1] = '\0';
    }
  else if (rval == 1) /* arith cmd, don't add double quotes */
    {
      strncpy (tokstr, ttok, ttoklen - 1);
      tokstr[ttoklen - 1] = '\0';
    }
  else /* nested subshell */
    {
      tokstr[0] = '(';
      strncpy (tokstr + 1, ttok, ttoklen - 1);
      tokstr[ttoklen] = ')';
      tokstr[ttoklen + 1] = c;
      tokstr[ttoklen + 2] = '\0';
    }

  *ep = tokstr;
  FREE (ttok);
  return rval;
}
#endif /* DPAREN_ARITHMETIC || ARITH_FOR_COMMAND */

#if defined(COND_COMMAND)
void
Shell::cond_error ()
{
  char *etext;

  if (EOF_Reached && cond_token != parser::token::COND_ERROR) /* [[ */
    parser_error (cond_lineno, _ ("unexpected EOF while looking for `]]'"));
  else if (cond_token != parser::token::COND_ERROR)
    {
      if ((etext = error_token_from_token (cond_token)))
        {
          parser_error (cond_lineno,
                        _ ("syntax error in conditional expression: "
                           "unexpected token `%s'"),
                        etext);
          free (etext);
        }
      else
        parser_error (cond_lineno,
                      _ ("syntax error in conditional expression"));
    }
}

int
Shell::cond_skip_newlines ()
{
  while ((cond_token = read_token (READ)) == '\n')
    {
      if (SHOULD_PROMPT ())
        prompt_again ();
    }
  return cond_token;
}

#define COND_RETURN_ERROR()                                                   \
  do                                                                          \
    {                                                                         \
      cond_token = COND_ERROR;                                                \
      return nullptr;                                                         \
    }                                                                         \
  while (0)

COND_COM *
Shell::cond_term ()
{
  WORD_DESC *op;
  COND_COM *term, *tleft, *tright;
  int tok, lineno;
  char *etext;

  /* Read a token.  It can be a left paren, a `!', a unary operator, or a
     word that should be the first argument of a binary operator.  Start by
     skipping newlines, since this is a compound command. */
  tok = cond_skip_newlines ();
  lineno = line_number;
  if (tok == parser::token::COND_END)
    {
      COND_RETURN_ERROR ();
    }
  else if (tok == '(')
    {
      term = cond_expr ();
      if (cond_token != ')')
        {
          if (term)
            dispose_cond_node (term); /* ( */
          if ((etext = error_token_from_token (cond_token)))
            {
              parser_error (lineno, _ ("unexpected token `%s', expected `)'"),
                            etext);
              free (etext);
            }
          else
            parser_error (lineno, _ ("expected `)'"));
          COND_RETURN_ERROR ();
        }
      term = make_cond_node (COND_EXPR, nullptr, term, nullptr);
      (void)cond_skip_newlines ();
    }
  else if (tok == parser::token::BANG
           || (tok == parser::token::WORD
               && (yylval.word->word[0] == '!'
                   && yylval.word->word[1] == '\0')))
    {
      if (tok == parser::token::WORD)
        dispose_word (yylval.word); /* not needed */
      term = cond_term ();
      if (term)
        term->flags ^= CMD_INVERT_RETURN;
    }
  else if (tok == parser::token::WORD && yylval.word->word[0] == '-'
           && yylval.word->word[1] && yylval.word->word[2] == 0
           && test_unop (yylval.word->word))
    {
      op = yylval.word;
      tok = read_token (READ);
      if (tok == parser::token::WORD)
        {
          tleft = make_cond_node (COND_TERM, yylval.word, nullptr, nullptr);
          term = make_cond_node (COND_UNARY, op, tleft, nullptr);
        }
      else
        {
          dispose_word (op);
          if ((etext = error_token_from_token (tok)))
            {
              parser_error (
                  line_number,
                  _ ("unexpected argument `%s' to conditional unary operator"),
                  etext);
              free (etext);
            }
          else
            parser_error (
                line_number,
                _ ("unexpected argument to conditional unary operator"));
          COND_RETURN_ERROR ();
        }

      (void)cond_skip_newlines ();
    }
  else if (tok == WORD) /* left argument to binary operator */
    {
      /* lhs */
      tleft = make_cond_node (COND_TERM, yylval.word, nullptr, nullptr);

      /* binop */
      tok = read_token (READ);
      if (tok == parser::token::WORD && test_binop (yylval.word->word))
        {
          op = yylval.word;
          if (op->word[0] == '='
              && (op->word[1] == '\0'
                  || (op->word[1] == '=' && op->word[2] == '\0')))
            parser_state |= PST_EXTPAT;
          else if (op->word[0] == '!' && op->word[1] == '='
                   && op->word[2] == '\0')
            parser_state |= PST_EXTPAT;
        }
#if defined(COND_REGEXP)
      else if (tok == WORD && STREQ (yylval.word->word, "=~"))
        {
          op = yylval.word;
          parser_state |= PST_REGEXP;
        }
#endif
      else if (tok == '<' || tok == '>')
        op = make_word_from_token (tok); /* ( */
      /* There should be a check before blindly accepting the `)' that we have
         seen the opening `('. */
      else if (tok == parser::token::COND_END || tok == parser::token::AND_AND
               || tok == parser::token::OR_OR || tok == ')')
        {
          /* Special case.  [[ x ]] is equivalent to [[ -n x ]], just like
             the test command.  Similarly for [[ x && expr ]] or
             [[ x || expr ]] or [[ (x) ]]. */
          op = make_word ("-n");
          term = make_cond_node (COND_UNARY, op, tleft, nullptr);
          cond_token = tok;
          return term;
        }
      else
        {
          if ((etext = error_token_from_token (tok)))
            {
              parser_error (line_number,
                            _ ("unexpected token `%s', conditional binary "
                               "operator expected"),
                            etext);
              free (etext);
            }
          else
            parser_error (line_number,
                          _ ("conditional binary operator expected"));
          dispose_cond_node (tleft);
          COND_RETURN_ERROR ();
        }

      /* rhs */
      if (parser_state & PST_EXTPAT)
        extended_glob = 1;
      tok = read_token (READ);
      if (parser_state & PST_EXTPAT)
        extended_glob = global_extglob;
      parser_state &= ~(PST_REGEXP | PST_EXTPAT);

      if (tok == WORD)
        {
          tright = make_cond_node (COND_TERM, yylval.word, nullptr, nullptr);
          term = make_cond_node (COND_BINARY, op, tleft, tright);
        }
      else
        {
          if ((etext = error_token_from_token (tok)))
            {
              parser_error (line_number,
                            _ ("unexpected argument `%s' to conditional "
                               "binary operator"),
                            etext);
              free (etext);
            }
          else
            parser_error (
                line_number,
                _ ("unexpected argument to conditional binary operator"));
          dispose_cond_node (tleft);
          dispose_word (op);
          COND_RETURN_ERROR ();
        }

      (void)cond_skip_newlines ();
    }
  else
    {
      if (tok < 256)
        parser_error (line_number,
                      _ ("unexpected token `%c' in conditional command"), tok);
      else if ((etext = error_token_from_token (tok)))
        {
          parser_error (line_number,
                        _ ("unexpected token `%s' in conditional command"),
                        etext);
          free (etext);
        }
      else
        parser_error (line_number,
                      _ ("unexpected token %d in conditional command"), tok);
      COND_RETURN_ERROR ();
    }
  return term;
}
#endif

#if defined(ARRAY_VARS)
/* When this is called, it's guaranteed that we don't care about anything
   in t beyond i.  We use a buffer with room for the characters we add just
   in case assignment() ends up doing something like parsing a command
   substitution that will reallocate atoken.  We don't want to write beyond
   the end of an allocated buffer. */
int
Shell::token_is_assignment (char *t, int i)
{
  int r;
  char *atoken;

  atoken = (char *)xmalloc (i + 3);
  memcpy (atoken, t, i);
  atoken[i] = '=';
  atoken[i + 1] = '\0';

  r = assignment (atoken, (parser_state & PST_COMPASSIGN) != 0);

  free (atoken);

  /* XXX - check that r == i to avoid returning false positive for
     t containing `=' before t[i]. */
  return r > 0 && r == i;
}

/* XXX - possible changes here for `+=' */
int
Shell::token_is_ident (char *t, int i)
{
  unsigned char c;
  int r;

  c = t[i];
  t[i] = '\0';
  r = legal_identifier (t);
  t[i] = c;
  return r;
}
#endif

parser::symbol_type
Shell::read_token_word (int character)
{
  /* The value for YYLVAL when a WORD is read. */
  WORD_DESC *the_word;

  /* Index into the token that we are building. */
  int token_index;

  /* ALL_DIGITS becomes zero when we see a non-digit. */
  int all_digit_token;

  /* DOLLAR_PRESENT becomes non-zero if we see a `$'. */
  int dollar_present;

  /* COMPOUND_ASSIGNMENT becomes non-zero if we are parsing a compound
     assignment. */
  int compound_assignment;

  /* QUOTED becomes non-zero if we see one of ("), ('), (`), or (\). */
  int quoted;

  /* Non-zero means to ignore the value of the next character, and just
     to add it no matter what. */
  int pass_next_character;

  /* The current delimiting character. */
  int cd;
  int result, peek_char;
  char *ttok, *ttrans;
  int ttoklen, ttranslen;
  int64_t lvalue;

  if (token_buffer_size < TOKEN_DEFAULT_INITIAL_SIZE)
    token = (char *)xrealloc (token,
                              token_buffer_size = TOKEN_DEFAULT_INITIAL_SIZE);

  token_index = 0;
  all_digit_token = DIGIT (character);
  dollar_present = quoted = pass_next_character = compound_assignment = 0;

  for (;;)
    {
      if (character == EOF)
        goto got_token;

      if (pass_next_character)
        {
          pass_next_character = 0;
          goto got_escaped_character;
        }

      cd = current_delimiter (dstack);

      /* Handle backslashes.  Quote lots of things when not inside of
         double-quotes, quote some things inside of double-quotes. */
      if MBTEST (character == '\\')
        {
          peek_char = shell_getc (0);

          /* Backslash-newline is ignored in all cases except
             when quoted with single quotes. */
          if (peek_char == '\n')
            {
              character = '\n';
              goto next_character;
            }
          else
            {
              shell_ungetc (peek_char);

              /* If the next character is to be quoted, note it now. */
              if (cd == 0 || cd == '`'
                  || (cd == '"' && peek_char >= 0
                      && (sh_syntaxtab[peek_char] & CBSDQUOTE)))
                pass_next_character++;

              quoted = 1;
              goto got_character;
            }
        }

      /* Parse a matched pair of quote characters. */
      if MBTEST (shellquote (character))
        {
          push_delimiter (dstack, character);
          ttok = parse_matched_pair (character, character, character, &ttoklen,
                                     (character == '`') ? P_COMMAND : 0);
          pop_delimiter (dstack);
          if (ttok == &matched_pair_error)
            return -1; /* Bail immediately. */
          token[token_index++] = character;
          strcpy (token + token_index, ttok);
          token_index += ttoklen;
          all_digit_token = 0;
          if (character != '`')
            quoted = 1;
          dollar_present |= (character == '"' && strchr (ttok, '$') != 0);
          FREE (ttok);
          goto next_character;
        }

#ifdef COND_REGEXP
      /* When parsing a regexp as a single word inside a conditional command,
         we need to special-case characters special to both the shell and
         regular expressions.  Right now, that is only '(' and '|'. */ /*)*/
      if MBTEST ((parser_state & PST_REGEXP)
                 && (character == '(' || character == '|')) /*)*/
        {
          if (character == '|')
            goto got_character;

          push_delimiter (dstack, character);
          ttok = parse_matched_pair (cd, '(', ')', &ttoklen, 0);
          pop_delimiter (dstack);
          if (ttok == &matched_pair_error)
            return -1; /* Bail immediately. */
          token[token_index++] = character;
          strcpy (token + token_index, ttok);
          token_index += ttoklen;
          FREE (ttok);
          dollar_present = all_digit_token = 0;
          goto next_character;
        }
#endif /* COND_REGEXP */

#ifdef EXTENDED_GLOB
      /* Parse a ksh-style extended pattern matching specification. */
      if MBTEST (extended_glob && PATTERN_CHAR (character))
        {
          peek_char = shell_getc (1);
          if MBTEST (peek_char == '(') /* ) */
            {
              push_delimiter (dstack, peek_char);
              ttok = parse_matched_pair (cd, '(', ')', &ttoklen, 0);
              pop_delimiter (dstack);
              if (ttok == &matched_pair_error)
                return -1; /* Bail immediately. */
              token[token_index++] = character;
              token[token_index++] = peek_char;
              strcpy (token + token_index, ttok);
              token_index += ttoklen;
              FREE (ttok);
              dollar_present = all_digit_token = 0;
              goto next_character;
            }
          else
            shell_ungetc (peek_char);
        }
#endif /* EXTENDED_GLOB */

      /* If the delimiter character is not single quote, parse some of
         the shell expansions that must be read as a single word. */
      if (shellexp (character))
        {
          peek_char = shell_getc (1);
          /* $(...), <(...), >(...), $((...)), ${...}, and $[...] constructs */
          if MBTEST (peek_char == '('
                     || ((peek_char == '{' || peek_char == '[')
                         && character == '$')) /* ) ] } */
            {
              if (peek_char == '{') /* } */
                ttok = parse_matched_pair (cd, '{', '}', &ttoklen,
                                           (P_FIRSTCLOSE | P_DOLBRACE));
              else if (peek_char == '(') /* ) */
                {
                  /* XXX - push and pop the `(' as a delimiter for use by
                     the command-oriented-history code.  This way newlines
                     appearing in the $(...) string get added to the
                     history literally rather than causing a possibly-
                     incorrect `;' to be added. ) */
                  push_delimiter (dstack, peek_char);
                  ttok = parse_comsub (cd, '(', ')', &ttoklen, P_COMMAND);
                  pop_delimiter (dstack);
                }
              else
                ttok = parse_matched_pair (cd, '[', ']', &ttoklen, 0);
              if (ttok == &matched_pair_error)
                return -1; /* Bail immediately. */
              token[token_index++] = character;
              token[token_index++] = peek_char;
              strcpy (token + token_index, ttok);
              token_index += ttoklen;
              FREE (ttok);
              dollar_present = 1;
              all_digit_token = 0;
              goto next_character;
            }
          /* This handles $'...' and $"..." new-style quoted strings. */
          else if MBTEST (character == '$'
                          && (peek_char == '\'' || peek_char == '"'))
            {
              int first_line;

              first_line = line_number;
              push_delimiter (dstack, peek_char);
              ttok = parse_matched_pair (peek_char, peek_char, peek_char,
                                         &ttoklen,
                                         (peek_char == '\'') ? P_ALLOWESC : 0);
              pop_delimiter (dstack);
              if (ttok == &matched_pair_error)
                return -1;
              if (peek_char == '\'')
                {
                  ttrans = ansiexpand (ttok, 0, ttoklen - 1, &ttranslen);
                  free (ttok);

                  /* Insert the single quotes and correctly quote any
                     embedded single quotes (allowed because P_ALLOWESC was
                     passed to parse_matched_pair). */
                  ttok = sh_single_quote (ttrans);
                  free (ttrans);
                  ttranslen = strlen (ttok);
                  ttrans = ttok;
                }
              else
                {
                  /* Try to locale-expand the converted string. */
                  ttrans = localeexpand (ttok, 0, ttoklen - 1, first_line,
                                         &ttranslen);
                  free (ttok);

                  /* Add the double quotes back */
                  ttok = sh_mkdoublequoted (ttrans, ttranslen, 0);
                  free (ttrans);
                  ttranslen += 2;
                  ttrans = ttok;
                }

              strcpy (token + token_index, ttrans);
              token_index += ttranslen;
              FREE (ttrans);
              quoted = 1;
              all_digit_token = 0;
              goto next_character;
            }
          /* This could eventually be extended to recognize all of the
             shell's single-character parameter expansions, and set flags.*/
          else if MBTEST (character == '$' && peek_char == '$')
            {
              token[token_index++] = '$';
              token[token_index++] = peek_char;
              dollar_present = 1;
              all_digit_token = 0;
              goto next_character;
            }
          else
            shell_ungetc (peek_char);
        }

#if defined(ARRAY_VARS)
      /* Identify possible array subscript assignment; match [...].  If
         parser_state&PST_COMPASSIGN, we need to parse [sub]=words treating
         `sub' as if it were enclosed in double quotes. */
      else if MBTEST (character == '[' && /* ] */
                      ((token_index > 0
                        && assignment_acceptable (last_read_token)
                        && token_is_ident (token, token_index))
                       || (token_index == 0
                           && (parser_state & PST_COMPASSIGN))))
        {
          ttok = parse_matched_pair (cd, '[', ']', &ttoklen, P_ARRAYSUB);
          if (ttok == &matched_pair_error)
            return -1; /* Bail immediately. */
          token[token_index++] = character;
          strcpy (token + token_index, ttok);
          token_index += ttoklen;
          FREE (ttok);
          all_digit_token = 0;
          goto next_character;
        }
      /* Identify possible compound array variable assignment. */
      else if MBTEST (character == '=' && token_index > 0
                      && (assignment_acceptable (last_read_token)
                          || (parser_state & PST_ASSIGNOK))
                      && token_is_assignment (token, token_index))
        {
          peek_char = shell_getc (1);
          if MBTEST (peek_char == '(') /* ) */
            {
              ttok = parse_compound_assignment (&ttoklen);

              token[token_index++] = '=';
              token[token_index++] = '(';
              if (ttok)
                {
                  strcpy (token + token_index, ttok);
                  token_index += ttoklen;
                }
              token[token_index++] = ')';
              FREE (ttok);
              all_digit_token = 0;
              compound_assignment = 1;
#if 1
              goto next_character;
#else
              goto got_token; /* ksh93 seems to do this */
#endif
            }
          else
            shell_ungetc (peek_char);
        }
#endif

      /* When not parsing a multi-character word construct, shell meta-
         characters break words. */
      if MBTEST (shellbreak (character))
        {
          shell_ungetc (character);
          goto got_token;
        }

    got_character:
      if (character == CTLESC || character == CTLNUL)
        {
          token[token_index++] = CTLESC;
        }

    got_escaped_character:
      token[token_index++] = character;

      all_digit_token &= DIGIT (character);
      dollar_present |= character == '$';

    next_character:
      if (character == '\n' && SHOULD_PROMPT ())
        prompt_again ();

      /* We want to remove quoted newlines (that is, a \<newline> pair)
         unless we are within single quotes or pass_next_character is
         set (the shell equivalent of literal-next). */
      cd = current_delimiter (dstack);
      character = shell_getc (cd != '\'' && pass_next_character == 0);
    } /* end for (;;) */

got_token:

  /* Calls to RESIZE_MALLOCED_BUFFER ensure there is sufficient room. */
  token[token_index] = '\0';

  /* Check to see what thing we should return.  If the last_read_token
     is a `<', or a `&', or the character which ended this token is
     a '>' or '<', then, and ONLY then, is this input token a NUMBER.
     Otherwise, it is just a word, and should be returned as such. */
  if MBTEST (all_digit_token
             && (character == '<' || character == '>'
                 || last_read_token == parser::token::LESS_AND
                 || last_read_token == parser::token::GREATER_AND))
    {
      if (legal_number (token, &lvalue) && (int)lvalue == lvalue)
        {
          yylval.number = lvalue;
          return NUMBER;
        }
    }

  /* Check for special case tokens. */
  result = (last_shell_getc_is_singlebyte) ? special_case_tokens (token) : -1;
  if (result >= 0)
    return result;

#if defined(ALIAS)
  /* Posix.2 does not allow reserved words to be aliased, so check for all
     of them, including special cases, before expanding the current token
     as an alias. */
  if MBTEST (posixly_correct)
    CHECK_FOR_RESERVED_WORD (token);

  /* Aliases are expanded iff EXPAND_ALIASES is non-zero, and quoting
     inhibits alias expansion. */
  if (expand_aliases && quoted == 0)
    {
      result = alias_expand_token (token);
      if (result == RE_READ_TOKEN)
        return RE_READ_TOKEN;
      else if (result == NO_EXPANSION)
        parser_state &= ~PST_ALEXPNEXT;
      // XXX - will there be any other return values?
    }

  /* If not in Posix.2 mode, check for reserved words after alias
     expansion. */
  if MBTEST (posixly_correct == 0)
#endif
    CHECK_FOR_RESERVED_WORD (token);

  the_word = alloc_word_desc ();
  the_word->word = (char *)xmalloc (1 + token_index);
  the_word->flags = 0;
  strcpy (the_word->word, token);
  if (dollar_present)
    the_word->flags |= W_HASDOLLAR;
  if (quoted)
    the_word->flags |= W_QUOTED; /*(*/
  if (compound_assignment && token[token_index - 1] == ')')
    the_word->flags |= W_COMPASSIGN;
  /* A word is an assignment if it appears at the beginning of a
     simple command, or after another assignment word.  This is
     context-dependent, so it cannot be handled in the grammar. */
  if (assignment (token, (parser_state & PST_COMPASSIGN) != 0))
    {
      the_word->flags |= W_ASSIGNMENT;
      /* Don't perform word splitting on assignment statements. */
      if (assignment_acceptable (last_read_token)
          || (parser_state & PST_COMPASSIGN) != 0)
        {
          the_word->flags |= W_NOSPLIT;
          if (parser_state & PST_COMPASSIGN)
            the_word->flags |= W_NOGLOB; /* XXX - W_NOBRACE? */
        }
    }

  if (command_token_position (last_read_token))
    {
      struct builtin *b;
      b = builtin_address_internal (token, 0);
      if (b && (b->flags & ASSIGNMENT_BUILTIN))
        parser_state |= PST_ASSIGNOK;
      else if (STREQ (token, "eval") || STREQ (token, "let"))
        parser_state |= PST_ASSIGNOK;
    }

  yylval.word = the_word;

  /* should we check that quoted == 0 as well? */
  if (token[0] == '{' && token[token_index - 1] == '}'
      && (character == '<' || character == '>'))
    {
      /* can use token; already copied to the_word */
      token[token_index - 1] = '\0';
#if defined(ARRAY_VARS)
      if (legal_identifier (token + 1) || valid_array_reference (token + 1, 0))
#else
      if (legal_identifier (token + 1))
#endif
        {
          strcpy (the_word->word, token + 1);
          /* itrace("read_token_word: returning REDIR_WORD for %s",
           * the_word->word); */
          yylval.word = the_word; /* accommodate recursive call */
          return REDIR_WORD;
        }
      else
        /* valid_array_reference can call the parser recursively; need to
           make sure that yylval.word doesn't change if we are going to
           return WORD or ASSIGNMENT_WORD */
        yylval.word = the_word;
    }

  result = ((the_word->flags & (W_ASSIGNMENT | W_NOSPLIT))
            == (W_ASSIGNMENT | W_NOSPLIT))
               ? parser::token::ASSIGNMENT_WORD
               : parser::token::WORD;

  switch (last_read_token)
    {
    case parser::token::FUNCTION:
      parser_state |= PST_ALLOWOPNBRC;
      function_dstart = line_number;
      break;
    case parser::token::CASE:
    case parser::token::SELECT:
    case parser::token::FOR:
      if (word_top < MAX_CASE_NEST)
        word_top++;
      word_lineno[word_top] = line_number;
      expecting_in_token++;
      break;
    }

  return result;
}

/* Return the index of TOKEN in the alist of reserved words, or -1 if
   TOKEN is not a shell reserved word. */
int
find_reserved_word (const char *tokstr)
{
  int i;
  for (i = 0; word_token_alist[i].word; i++)
    if (STREQ (tokstr, word_token_alist[i].word))
      return i;
  return -1;
}

/* An interface to let the rest of the shell (primarily the completion
   system) know what the parser is expecting. */
int
parser_in_command_position ()
{
  return command_token_position (last_read_token);
}

#if defined(HISTORY)

/* A list of tokens which can be followed by newlines, but not by
   semi-colons.  When concatenating multiple lines of history, the
   newline separator for such tokens is replaced with a space. */
static const int no_semi_successors[] = { '\n',
                                          '{',
                                          '(',
                                          ')',
                                          ';',
                                          '&',
                                          '|',
                                          parser::token::CASE,
                                          parser::token::DO,
                                          parser::token::ELSE,
                                          parser::token::IF,
                                          parser::token::SEMI_SEMI,
                                          parser::token::SEMI_AND,
                                          parser::token::SEMI_SEMI_AND,
                                          parser::token::THEN,
                                          parser::token::UNTIL,
                                          parser::token::WHILE,
                                          parser::token::AND_AND,
                                          parser::token::OR_OR,
                                          parser::token::IN,
                                          0 };

/* If we are not within a delimited expression, try to be smart
   about which separators can be semi-colons and which must be
   newlines.  Returns the string that should be added into the
   history entry.  LINE is the line we're about to add; it helps
   make some more intelligent decisions in certain cases. */
const char *
Shell::history_delimiting_chars (const char *line)
{
  static int last_was_heredoc
      = 0; /* was the last entry the start of a here document? */
  int i;

  if ((parser_state & PST_HEREDOC) == 0)
    last_was_heredoc = 0;

  if (dstack.delimiter_depth != 0)
    return "\n";

  /* We look for current_command_line_count == 2 because we are looking to
     add the first line of the body of the here document (the second line
     of the command).  We also keep LAST_WAS_HEREDOC as a private sentinel
     variable to note when we think we added the first line of a here doc
     (the one with a "<<" somewhere in it) */
  if (parser_state & PST_HEREDOC)
    {
      if (last_was_heredoc)
        {
          last_was_heredoc = 0;
          return "\n";
        }
      return here_doc_first_line ? "\n" : "";
    }

  if (parser_state & PST_COMPASSIGN)
    return " ";

  /* First, handle some special cases. */
  /*(*/
  /* If we just read `()', assume it's a function definition, and don't
     add a semicolon.  If the token before the `)' was not `(', and we're
     not in the midst of parsing a case statement, assume it's a
     parenthesized command and add the semicolon. */
  /*)(*/
  if (token_before_that == ')')
    {
      if (two_tokens_ago == '(') /*)*/ /* function def */
        return " ";
      /* This does not work for subshells inside case statement
         command lists.  It's a suboptimal solution. */
      else if (parser_state & PST_CASESTMT) /* case statement pattern */
        return " ";
      else
        return "; "; /* (...) subshell */
    }
  else if (token_before_that == WORD && two_tokens_ago == FUNCTION)
    return " "; /* function def using `function name' without `()' */

  /* If we're not in a here document, but we think we're about to parse one,
     and we would otherwise return a `;', return a newline to delimit the
     line with the here-doc delimiter */
  else if ((parser_state & PST_HEREDOC) == 0 && current_command_line_count > 1
           && last_read_token == '\n' && strstr (line, "<<"))
    {
      last_was_heredoc = 1;
      return "\n";
    }
  else if ((parser_state & PST_HEREDOC) == 0 && current_command_line_count > 1
           && need_here_doc > 0)
    return "\n";
  else if (token_before_that == WORD && two_tokens_ago == FOR)
    {
      /* Tricky.  `for i\nin ...' should not have a semicolon, but
         `for i\ndo ...' should.  We do what we can. */
      for (i = shell_input_line_index; whitespace (shell_input_line[i]); i++)
        ;
      if (shell_input_line[i] && shell_input_line[i] == 'i'
          && shell_input_line[i + 1] == 'n')
        return " ";
      return ";";
    }
  else if (two_tokens_ago == CASE && token_before_that == WORD
           && (parser_state & PST_CASESTMT))
    return " ";

  for (i = 0; no_semi_successors[i]; i++)
    {
      if (token_before_that == no_semi_successors[i])
        return " ";
    }

  if (line_isblank (line))
    return "";

  return "; ";
}
#endif /* HISTORY */

/* Issue a prompt, or prepare to issue a prompt when the next character
   is read. */
void
Shell::prompt_again ()
{
  char *temp_prompt;

  if (interactive == 0 || expanding_alias ()) /* XXX */
    return;

  ps1_prompt = get_string_value ("PS1");
  ps2_prompt = get_string_value ("PS2");

  ps0_prompt = get_string_value ("PS0");

  if (!prompt_string_pointer)
    prompt_string_pointer = &ps1_prompt;

  temp_prompt = *prompt_string_pointer
                    ? decode_prompt_string (*prompt_string_pointer)
                    : nullptr;

  if (temp_prompt == 0)
    {
      temp_prompt = (char *)xmalloc (1);
      temp_prompt[0] = '\0';
    }

  current_prompt_string = *prompt_string_pointer;
  prompt_string_pointer = &ps2_prompt;

#if defined(READLINE)
  if (!no_line_editing)
    {
      FREE (current_readline_prompt);
      current_readline_prompt = temp_prompt;
    }
  else
#endif /* READLINE */
    {
      FREE (current_decoded_prompt);
      current_decoded_prompt = temp_prompt;
    }
}

#if defined(HISTORY)
/* The history library increments the history offset as soon as it stores
   the first line of a potentially multi-line command, so we compensate
   here by returning one fewer when appropriate. */
int
Shell::prompt_history_number (const char *pmt)
{
  int ret;

  ret = history_number ();
  if (ret == 1)
    return ret;

  if (pmt == ps1_prompt) /* are we expanding $PS1? */
    return ret;
  else if (pmt == ps2_prompt && command_oriented_history == 0)
    return ret; /* not command oriented history */
  else if (pmt == ps2_prompt && command_oriented_history
           && current_command_first_line_saved)
    return ret - 1;
  else
    return ret - 1; /* PS0, PS4, ${var@P}, PS2 other cases */
}
#endif

/* Return a string which will be printed as a prompt.  The string
   may contain special characters which are decoded as follows:

        \a	bell (ascii 07)
        \d	the date in Day Mon Date format
        \e	escape (ascii 033)
        \h	the hostname up to the first `.'
        \H	the hostname
        \j	the number of active jobs
        \l	the basename of the shell's tty device name
        \n	CRLF
        \r	CR
        \s	the name of the shell
        \t	the time in 24-hour hh:mm:ss format
        \T	the time in 12-hour hh:mm:ss format
        \@	the time in 12-hour hh:mm am/pm format
        \A	the time in 24-hour hh:mm format
        \D{fmt}	the result of passing FMT to strftime(3)
        \u	your username
        \v	the version of bash (e.g., 2.00)
        \V	the release of bash, version + patchlevel (e.g., 2.00.0)
        \w	the current working directory
        \W	the last element of $PWD
        \!	the history number of this command
        \#	the command number of this command
        \$	a $ or a # if you are root
        \nnn	character code nnn in octal
        \\	a backslash
        \[	begin a sequence of non-printing chars
        \]	end a sequence of non-printing chars
*/
char *
Shell::decode_prompt_string (const char *string)
{
  WORD_LIST *list;
  char *result, *t;
  struct dstack save_dstack;
  int last_exit_value, last_comsub_pid;
#if defined(PROMPT_STRING_DECODE)
  size_t result_size;
  int result_index;
  int c, n, i;
  char *temp, *t_host, octal_string[4];
  struct tm *tm;
  time_t the_time;
  char timebuf[128];
  char *timefmt;

  result = (char *)xmalloc (result_size = PROMPT_GROWTH);
  result[result_index = 0] = 0;
  temp = nullptr;
  const char *orig_string = string;

  while ((c = *string++))
    {
      if (posixly_correct && c == '!')
        {
          if (*string == '!')
            {
              temp = savestring ("!");
              goto add_string;
            }
          else
            {
#if !defined(HISTORY)
              temp = savestring ("1");
#else                   /* HISTORY */
              temp = itos (prompt_history_number (orig_string));
#endif                  /* HISTORY */
              string--; /* add_string increments string again. */
              goto add_string;
            }
        }
      if (c == '\\')
        {
          c = *string;

          switch (c)
            {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
              strncpy (octal_string, string, 3);
              octal_string[3] = '\0';

              n = read_octal (octal_string);
              temp = (char *)xmalloc (3);

              if (n == CTLESC || n == CTLNUL)
                {
                  temp[0] = CTLESC;
                  temp[1] = n;
                  temp[2] = '\0';
                }
              else if (n == -1)
                {
                  temp[0] = '\\';
                  temp[1] = '\0';
                }
              else
                {
                  temp[0] = n;
                  temp[1] = '\0';
                }

              for (c = 0; n != -1 && c < 3 && ISOCTAL (*string); c++)
                string++;

              c = 0; /* tested at add_string: */
              goto add_string;

            case 'd':
            case 't':
            case 'T':
            case '@':
            case 'A':
              /* Make the current time/date into a string. */
              (void)time (&the_time);
#if defined(HAVE_TZSET)
              sv_tz ("TZ"); /* XXX -- just make sure */
#endif
              tm = localtime (&the_time);

              if (c == 'd')
                n = strftime (timebuf, sizeof (timebuf), "%a %b %d", tm);
              else if (c == 't')
                n = strftime (timebuf, sizeof (timebuf), "%H:%M:%S", tm);
              else if (c == 'T')
                n = strftime (timebuf, sizeof (timebuf), "%I:%M:%S", tm);
              else if (c == '@')
                n = strftime (timebuf, sizeof (timebuf), "%I:%M %p", tm);
              else if (c == 'A')
                n = strftime (timebuf, sizeof (timebuf), "%H:%M", tm);

              if (n == 0)
                timebuf[0] = '\0';
              else
                timebuf[sizeof (timebuf) - 1] = '\0';

              temp = savestring (timebuf);
              goto add_string;

            case 'D':               /* strftime format */
              if (string[1] != '{') /* } */
                goto not_escape;

              (void)time (&the_time);
              tm = localtime (&the_time);
              string += 2; /* skip { */
              timefmt = (char *)xmalloc (strlen (string) + 3);
              for (t = timefmt; *string && *string != '}';)
                *t++ = *string++;
              *t = '\0';
              c = *string; /* tested at add_string */
              if (timefmt[0] == '\0')
                {
                  timefmt[0] = '%';
                  timefmt[1] = 'X'; /* locale-specific current time */
                  timefmt[2] = '\0';
                }
              n = strftime (timebuf, sizeof (timebuf), timefmt, tm);
              free (timefmt);

              if (n == 0)
                timebuf[0] = '\0';
              else
                timebuf[sizeof (timebuf) - 1] = '\0';

              if (promptvars || posixly_correct)
                /* Make sure that expand_prompt_string is called with a
                   second argument of Q_DOUBLE_QUOTES if we use this
                   function here. */
                temp = sh_backslash_quote_for_double_quotes (timebuf);
              else
                temp = savestring (timebuf);
              goto add_string;

            case 'n':
              temp = (char *)xmalloc (3);
              temp[0] = no_line_editing ? '\n' : '\r';
              temp[1] = no_line_editing ? '\0' : '\n';
              temp[2] = '\0';
              goto add_string;

            case 's':
              temp = (char *)base_pathname (shell_name);
              /* Try to quote anything the user can set in the file system */
              if (promptvars || posixly_correct)
                temp = sh_backslash_quote_for_double_quotes (temp);
              else
                temp = savestring (temp);
              goto add_string;

            case 'v':
            case 'V':
              temp = (char *)xmalloc (16);
              if (c == 'v')
                strcpy (temp, dist_version);
              else
                sprintf (temp, "%s.%d", dist_version, patch_level);
              goto add_string;

            case 'w':
            case 'W':
              {
                /* Use the value of PWD because it is much more efficient. */
                char t_string[PATH_MAX];
                int tlen;

                temp = get_string_value ("PWD");

                if (temp == 0)
                  {
                    if (getcwd (t_string, sizeof (t_string)) == 0)
                      {
                        t_string[0] = '.';
                        tlen = 1;
                      }
                    else
                      tlen = strlen (t_string);
                  }
                else
                  {
                    tlen = sizeof (t_string) - 1;
                    strncpy (t_string, temp, tlen);
                  }
                t_string[tlen] = '\0';

#if defined(MACOSX)
                /* Convert from "fs" format to "input" format */
                temp = fnx_fromfs (t_string, strlen (t_string));
                if (temp != t_string)
                  strcpy (t_string, temp);
#endif

#define ROOT_PATH(x) ((x)[0] == '/' && (x)[1] == 0)
#define DOUBLE_SLASH_ROOT(x) ((x)[0] == '/' && (x)[1] == '/' && (x)[2] == 0)
                /* Abbreviate \W as ~ if $PWD == $HOME */
                if (c == 'W'
                    && (((t = get_string_value ("HOME")) == 0)
                        || STREQ (t, t_string) == 0))
                  {
                    if (ROOT_PATH (t_string) == 0
                        && DOUBLE_SLASH_ROOT (t_string) == 0)
                      {
                        t = strrchr (t_string, '/');
                        if (t)
                          memmove (t_string, t + 1,
                                   strlen (t)); /* strlen(t) to copy nullptr */
                      }
                  }
#undef ROOT_PATH
#undef DOUBLE_SLASH_ROOT
                else
                  {
                    /* polite_directory_format is guaranteed to return a string
                       no longer than PATH_MAX - 1 characters. */
                    const char *t2 = polite_directory_format (t_string);
                    if (t2 != t_string)
                      strcpy (t_string, t2);
                  }

                temp = trim_pathname (t_string, PATH_MAX - 1);
                /* If we're going to be expanding the prompt string later,
                   quote the directory name. */
                if (promptvars || posixly_correct)
                  /* Make sure that expand_prompt_string is called with a
                     second argument of Q_DOUBLE_QUOTES if we use this
                     function here. */
                  temp = sh_backslash_quote_for_double_quotes (t_string);
                else
                  temp = savestring (t_string);

                goto add_string;
              }

            case 'u':
              if (current_user.user_name == 0)
                get_current_user_info ();
              temp = savestring (current_user.user_name);
              goto add_string;

            case 'h':
            case 'H':
              t_host = savestring (current_host_name);
              if (c == 'h' && (t = (char *)strchr (t_host, '.')))
                *t = '\0';
              if (promptvars || posixly_correct)
                /* Make sure that expand_prompt_string is called with a
                   second argument of Q_DOUBLE_QUOTES if we use this
                   function here. */
                temp = sh_backslash_quote_for_double_quotes (t_host);
              else
                temp = savestring (t_host);
              free (t_host);
              goto add_string;

            case '#':
              n = current_command_number;
              /* If we have already incremented current_command_number (PS4,
                 ${var@P}), compensate */
              if (orig_string != ps0_prompt && orig_string != ps1_prompt
                  && orig_string != ps2_prompt)
                n--;
              temp = itos (n);
              goto add_string;

            case '!':
#if !defined(HISTORY)
              temp = savestring ("1");
#else  /* HISTORY */
              temp = itos (prompt_history_number (orig_string));
#endif /* HISTORY */
              goto add_string;

            case '$':
              t = temp = (char *)xmalloc (3);
              if ((promptvars || posixly_correct) && (current_user.euid != 0))
                *t++ = '\\';
              *t++ = current_user.euid == 0 ? '#' : '$';
              *t = '\0';
              goto add_string;

            case 'j':
              temp = itos (count_all_jobs ());
              goto add_string;

            case 'l':
#if defined(HAVE_TTYNAME)
              temp = (char *)ttyname (fileno (stdin));
              t = temp ? (char *)base_pathname (temp) : (char *)"tty";
              temp = savestring (t);
#else
              temp = savestring ("tty");
#endif /* !HAVE_TTYNAME */
              goto add_string;

#if defined(READLINE)
            case '[':
            case ']':
              if (no_line_editing)
                {
                  string++;
                  break;
                }
              temp = (char *)xmalloc (3);
              n = (c == '[') ? RL_PROMPT_START_IGNORE : RL_PROMPT_END_IGNORE;
              i = 0;
              if (n == CTLESC || n == CTLNUL)
                temp[i++] = CTLESC;
              temp[i++] = n;
              temp[i] = '\0';
              goto add_string;
#endif /* READLINE */

            case '\\':
            case 'a':
            case 'e':
            case 'r':
              temp = (char *)xmalloc (2);
              if (c == 'a')
                temp[0] = '\07';
              else if (c == 'e')
                temp[0] = '\033';
              else if (c == 'r')
                temp[0] = '\r';
              else /* (c == '\\') */
                temp[0] = c;
              temp[1] = '\0';
              goto add_string;

            default:
            not_escape:
              temp = (char *)xmalloc (3);
              temp[0] = '\\';
              temp[1] = c;
              temp[2] = '\0';

            add_string:
              if (c)
                string++;
              result = sub_append_string (temp, result, &result_index,
                                          &result_size);
              temp = nullptr; /* Freed in sub_append_string (). */
              result[result_index] = '\0';
              break;
            }
        }
      else
        {
          /* dequote_string should take care of removing this if we are not
             performing the rest of the word expansions. */
          if (c == CTLESC || c == CTLNUL)
            result[result_index++] = CTLESC;
          result[result_index++] = c;
          result[result_index] = '\0';
        }
    }
#else  /* !PROMPT_STRING_DECODE */
  result = savestring (string);
#endif /* !PROMPT_STRING_DECODE */

  /* Save the delimiter stack and point `dstack' to temp space so any
     command substitutions in the prompt string won't result in screwing
     up the parser's quoting state. */
  save_dstack = dstack;
  dstack = temp_dstack;
  dstack.delimiter_depth = 0;

  /* Perform variable and parameter expansion and command substitution on
     the prompt string. */
  if (promptvars || posixly_correct)
    {
      last_exit_value = last_command_exit_value;
      last_comsub_pid = last_command_subst_pid;
      list = expand_prompt_string (result, Q_DOUBLE_QUOTES, 0);
      free (result);
      result = string_list (list);
      dispose_words (list);
      last_command_exit_value = last_exit_value;
      last_command_subst_pid = last_comsub_pid;
    }
  else
    {
      t = dequote_string (result);
      free (result);
      result = t;
    }

  dstack = save_dstack;

  return result;
}

/************************************************
 *						*
 *		ERROR HANDLING			*
 *						*
 ************************************************/

/* Report a syntax error, and restart the parser.  Call here for fatal
   errors. */
int
yyerror (const char *msg)
{
  report_syntax_error (nullptr);
  reset_parser ();
  return 0;
}

static char *
error_token_from_token (int tok)
{
  char *t;

  if ((t = find_token_in_alist (tok, word_token_alist, 0)))
    return t;

  if ((t = find_token_in_alist (tok, other_token_alist, 0)))
    return t;

  t = nullptr;
  /* This stuff is dicy and needs closer inspection */
  switch (current_token)
    {
    case parser::token::WORD:
    case parser::token::ASSIGNMENT_WORD:
      if (yylval.word)
        t = savestring (yylval.word->word);
      break;
    case parser::token::NUMBER:
      t = itos (yylval.number);
      break;
    case parser::token::ARITH_CMD:
      if (yylval.word_list)
        t = string_list (yylval.word_list);
      break;
    case parser::token::ARITH_FOR_EXPRS:
      if (yylval.word_list)
        t = string_list_internal (yylval.word_list, " ; ");
      break;
    case parser::token::COND_CMD:
      t = nullptr; /* punt */
      break;
    }

  return t;
}

static char *
error_token_from_text ()
{
  char *msg, *t;
  int token_end, i;

  t = shell_input_line;
  i = shell_input_line_index;
  token_end = 0;
  msg = nullptr;

  if (i && t[i] == '\0')
    i--;

  while (i && (whitespace (t[i]) || t[i] == '\n'))
    i--;

  if (i)
    token_end = i + 1;

  while (i && (member (t[i], " \n\t;|&") == 0))
    i--;

  while (i != token_end && (whitespace (t[i]) || t[i] == '\n'))
    i++;

  /* Return our idea of the offending token. */
  if (token_end || (i == 0 && token_end == 0))
    {
      if (token_end)
        msg = substring (t, i, token_end);
      else /* one-character token */
        {
          msg = (char *)xmalloc (2);
          msg[0] = t[i];
          msg[1] = '\0';
        }
    }

  return msg;
}

static void
print_offending_line ()
{
  char *msg;
  int token_end;

  msg = savestring (shell_input_line);
  token_end = strlen (msg);
  while (token_end && msg[token_end - 1] == '\n')
    msg[--token_end] = '\0';

  parser_error (line_number, "`%s'", msg);
  free (msg);
}

/* Report a syntax error with line numbers, etc.
   Call here for recoverable errors.  If you have a message to print,
   then place it in MESSAGE, otherwise pass nullptr and this will figure
   out an appropriate message for you. */
void
Shell::report_syntax_error (const char *message)
{
  char *msg, *p;

  if (message)
    {
      parser_error (line_number, "%s", message);
      if (interactive && EOF_Reached)
        EOF_Reached = false;
      last_command_exit_value = (executing_builtin && parse_and_execute_level)
                                    ? EX_BADSYNTAX
                                    : EX_BADUSAGE;
      set_pipestatus_from_exit (last_command_exit_value);
      return;
    }

  /* If the line of input we're reading is not null, try to find the
     objectionable token.  First, try to figure out what token the
     parser's complaining about by looking at current_token. */
  if (current_token != 0 && !EOF_Reached
      && (msg = error_token_from_token (current_token)))
    {
      if (ansic_shouldquote (msg))
        {
          p = ansic_quote (msg);
          delete[] msg;
          msg = p;
        }
      parser_error (line_number, _ ("syntax error near unexpected token `%s'"),
                    msg);
      delete[] msg;

      if (!interactive)
        print_offending_line ();

      last_command_exit_value = (executing_builtin && parse_and_execute_level)
                                    ? EX_BADSYNTAX
                                    : EX_BADUSAGE;
      set_pipestatus_from_exit (last_command_exit_value);
      return;
    }

  /* If looking at the current token doesn't prove fruitful, try to find the
     offending token by analyzing the text of the input line near the current
     input line index and report what we find. */
  if (!shell_input_line.empty ())
    {
      msg = error_token_from_text ();
      if (msg)
        {
          parser_error (line_number, _ ("syntax error near `%s'"), msg);
          delete[] msg;
        }

      /* If not interactive, print the line containing the error. */
      if (interactive == 0)
        print_offending_line ();
    }
  else
    {
      const char *cmsg = EOF_Reached
                             ? _ ("syntax error: unexpected end of file")
                             : _ ("syntax error");
      parser_error (line_number, "%s", cmsg);
      /* When the shell is interactive, this file uses EOF_Reached
         only for error reporting.  Other mechanisms are used to
         decide whether or not to exit. */
      if (interactive && EOF_Reached)
        EOF_Reached = false;
    }

  last_command_exit_value = (executing_builtin && parse_and_execute_level)
                                ? EX_BADSYNTAX
                                : EX_BADUSAGE;
  set_pipestatus_from_exit (last_command_exit_value);
}

/* ??? Needed function. ??? We have to be able to discard the constructs
   created during parsing.  In the case of error, we want to return
   allocated objects to the memory pool.  In the case of no error, we want
   to throw away the information about where the allocated objects live.
   (dispose_command () will actually free the command.) */
static void
discard_parser_constructs (int error_p)
{
}

/************************************************
 *						*
 *		EOF HANDLING			*
 *						*
 ************************************************/

/* If we have EOF as the only input unit, this user wants to leave
   the shell.  If the shell is not interactive, then just leave.
   Otherwise, if ignoreeof is set, and we haven't done this the
   required number of times in a row, print a message. */
void
Shell::handle_eof_input_unit ()
{
  if (interactive)
    {
      /* shell.c may use this to decide whether or not to write out the
         history, among other things.  We use it only for error reporting
         in this file. */
      EOF_Reached = false;

      /* If the user wants to "ignore" eof, then let her do so, kind of. */
      if (ignoreeof)
        {
          if (eof_encountered < eof_encountered_limit)
            {
              fprintf (stderr, _ ("Use \"%s\" to leave the shell.\n"),
                       login_shell ? "logout" : "exit");
              eof_encountered++;
              /* Reset the parsing state. */
              last_read_token = current_token = '\n';
              /* Reset the prompt string to be $PS1. */
              prompt_string_pointer = nullptr;
              prompt_again ();
              return;
            }
        }

      /* In this case EOF should exit the shell.  Do it now. */
      reset_parser ();

      last_shell_builtin = this_shell_builtin;
      this_shell_builtin = exit_builtin;
      exit_builtin (nullptr);
    }
  else
    {
      /* We don't write history files, etc., for non-interactive shells. */
      EOF_Reached = true;
    }
}

/************************************************
 *						*
 *	STRING PARSING FUNCTIONS		*
 *						*
 ************************************************/

/* It's very important that these two functions treat the characters
   between ( and ) identically. */

// static WORD_LIST parse_string_error;

/* Take a string and run it through the shell parser, returning the
   resultant word list.  Used by compound array assignment. */
WORD_LIST *
Shell::parse_string_to_word_list (char *s, int flags, const char *whom)
{
  WORD_LIST *wl;
  int tok, orig_current_token, orig_line_number, orig_input_terminator;
  int orig_line_count;
  int old_echo_input, old_expand_aliases, ea;
#if defined(HISTORY)
  int old_remember_on_history, old_history_expansion_inhibited;
#endif

#if defined(HISTORY)
  old_remember_on_history = remember_on_history;
#if defined(BANG_HISTORY)
  old_history_expansion_inhibited = history_expansion_inhibited;
#endif
  bash_history_disable ();
#endif

  orig_line_number = line_number;
  orig_line_count = current_command_line_count;
  orig_input_terminator = shell_input_line_terminator;
  old_echo_input = echo_input_at_read;
  old_expand_aliases = expand_aliases;

  push_stream (1);
  if ((ea = expanding_alias ()))
    parser_save_alias ();
  last_read_token = WORD; /* WORD to allow reserved words here */
  current_command_line_count = 0;
  echo_input_at_read = expand_aliases = 0;

  with_input_from_string (s, whom);
  wl = nullptr;

  if (flags & 1)
    parser_state |= (PST_COMPASSIGN | PST_REPARSE);

  while ((tok = read_token (READ)) != yacc_EOF)
    {
      if (tok == '\n' && *bash_input.location.string == '\0')
        break;
      if (tok == '\n') /* Allow newlines in compound assignments */
        continue;
      if (tok != WORD && tok != ASSIGNMENT_WORD)
        {
          line_number = orig_line_number + line_number - 1;
          orig_current_token = current_token;
          current_token = tok;
          yyerror (nullptr); /* does the right thing */
          current_token = orig_current_token;
          if (wl)
            dispose_words (wl);
          wl = &parse_string_error;
          break;
        }
      wl = make_word_list (yylval.word, wl);
    }

  last_read_token = '\n';
  pop_stream ();

  if (ea)
    parser_restore_alias ();

#if defined(HISTORY)
  remember_on_history = old_remember_on_history;
#if defined(BANG_HISTORY)
  history_expansion_inhibited = old_history_expansion_inhibited;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

  echo_input_at_read = old_echo_input;
  expand_aliases = old_expand_aliases;

  current_command_line_count = orig_line_count;
  shell_input_line_terminator = orig_input_terminator;

  if (flags & 1)
    parser_state &= ~(PST_COMPASSIGN | PST_REPARSE);

  if (wl == &parse_string_error)
    {
      set_exit_status (EXECUTION_FAILURE);
      if (interactive_shell == 0 && posixly_correct)
        jump_to_top_level (FORCE_EOF);
      else
        jump_to_top_level (DISCARD);
    }

  return REVERSE_LIST (wl, WORD_LIST *);
}

char *
Shell::parse_compound_assignment (size_t *retlenp)
{
  WORD_LIST *wl, *rl;
  int tok, orig_line_number, orig_token_size, orig_last_token, assignok;
  char *saved_token, *ret;

  saved_token = token;
  orig_token_size = token_buffer_size;
  orig_line_number = line_number;
  orig_last_token = last_read_token;

  last_read_token = WORD; /* WORD to allow reserved words here */

  token = nullptr;
  token_buffer_size = 0;

  assignok = parser_state & PST_ASSIGNOK; /* XXX */

  wl = nullptr; /* ( */
  parser_state |= PST_COMPASSIGN;

  while ((tok = read_token (READ)) != ')')
    {
      if (tok == '\n') /* Allow newlines in compound assignments */
        {
          if (SHOULD_PROMPT ())
            prompt_again ();
          continue;
        }
      if (tok != WORD && tok != ASSIGNMENT_WORD)
        {
          current_token = tok; /* for error reporting */
          if (tok == yacc_EOF) /* ( */
            parser_error (orig_line_number,
                          _ ("unexpected EOF while looking for matching `)'"));
          else
            yyerror (nullptr); /* does the right thing */
          if (wl)
            dispose_words (wl);
          wl = &parse_string_error;
          break;
        }
      wl = make_word_list (yylval.word, wl);
    }

  FREE (token);
  token = saved_token;
  token_buffer_size = orig_token_size;

  parser_state &= ~PST_COMPASSIGN;

  if (wl == &parse_string_error)
    {
      set_exit_status (EXECUTION_FAILURE);
      last_read_token = '\n'; /* XXX */
      if (interactive_shell == 0 && posixly_correct)
        jump_to_top_level (FORCE_EOF);
      else
        jump_to_top_level (DISCARD);
    }

  last_read_token = orig_last_token; /* XXX - was WORD? */

  if (wl)
    {
      rl = REVERSE_LIST (wl, WORD_LIST *);
      ret = string_list (rl);
      dispose_words (rl);
    }
  else
    ret = nullptr;

  if (retlenp)
    *retlenp = (ret && *ret) ? strlen (ret) : 0;

  if (assignok)
    parser_state |= PST_ASSIGNOK;

  return ret;
}

/************************************************
 *						*
 *   SAVING AND RESTORING PARTIAL PARSE STATE   *
 *						*
 ************************************************/

sh_parser_state_t *
Shell::save_parser_state (sh_parser_state_t *ps)
{
  if (ps == nullptr)
    ps = new sh_parser_state_t ();

  ps->parser_state = parser_state;
  ps->token_state = save_token_state ();

  ps->input_line_terminator = shell_input_line_terminator;
  ps->eof_encountered = eof_encountered;

  ps->prompt_string_pointer = prompt_string_pointer;

  ps->current_command_line_count = current_command_line_count;

#if defined(HISTORY)
  ps->remember_on_history = remember_on_history;
#if defined(BANG_HISTORY)
  ps->history_expansion_inhibited = history_expansion_inhibited;
#endif
#endif

  ps->last_command_exit_value = last_command_exit_value;
#if defined(ARRAY_VARS)
  ps->pipestatus = save_pipestatus_array ();
#endif

  ps->last_shell_builtin = last_shell_builtin;
  ps->this_shell_builtin = this_shell_builtin;

  ps->expand_aliases = expand_aliases;
  ps->echo_input_at_read = echo_input_at_read;
  ps->need_here_doc = need_here_doc;
  ps->here_doc_first_line = here_doc_first_line;

  if (need_here_doc == 0)
    ps->redir_stack[0] = 0;
  else
    memcpy (ps->redir_stack, redir_stack,
            sizeof (redir_stack[0]) * HEREDOC_MAX);

  ps->token = token;
  ps->token_buffer_size = token_buffer_size;
  /* Force reallocation on next call to read_token_word */
  token = 0;
  token_buffer_size = 0;

  return ps;
}

void
restore_parser_state (sh_parser_state_t *ps)
{
  int i;

  if (ps == 0)
    return;

  parser_state = ps->parser_state;
  if (ps->token_state)
    {
      restore_token_state (ps->token_state);
      free (ps->token_state);
    }

  shell_input_line_terminator = ps->input_line_terminator;
  eof_encountered = ps->eof_encountered;

  prompt_string_pointer = ps->prompt_string_pointer;

  current_command_line_count = ps->current_command_line_count;

#if defined(HISTORY)
  remember_on_history = ps->remember_on_history;
#if defined(BANG_HISTORY)
  history_expansion_inhibited = ps->history_expansion_inhibited;
#endif
#endif

  last_command_exit_value = ps->last_command_exit_value;
#if defined(ARRAY_VARS)
  restore_pipestatus_array (ps->pipestatus);
#endif

  last_shell_builtin = ps->last_shell_builtin;
  this_shell_builtin = ps->this_shell_builtin;

  expand_aliases = ps->expand_aliases;
  echo_input_at_read = ps->echo_input_at_read;
  need_here_doc = ps->need_here_doc;
  here_doc_first_line = ps->here_doc_first_line;

#if 0
  for (i = 0; i < HEREDOC_MAX; i++)
    redir_stack[i] = ps->redir_stack[i];
#else
  if (need_here_doc == 0)
    redir_stack[0] = 0;
  else
    memcpy (redir_stack, ps->redir_stack,
            sizeof (redir_stack[0]) * HEREDOC_MAX);
#endif

  FREE (token);
  token = ps->token;
  token_buffer_size = ps->token_buffer_size;
}

sh_input_line_state_t *
save_input_line_state (sh_input_line_state_t *ls)
{
  if (ls == 0)
    ls = (sh_input_line_state_t *)xmalloc (sizeof (sh_input_line_state_t));
  if (ls == 0)
    return nullptr;

  ls->input_line = shell_input_line;
  ls->input_line_size = shell_input_line_size;
  ls->input_line_len = shell_input_line_len;
  ls->input_line_index = shell_input_line_index;

#if defined(HANDLE_MULTIBYTE)
  ls->input_property = shell_input_line_property;
  ls->input_propsize = shell_input_line_propsize;
#endif

  /* force reallocation */
  shell_input_line = 0;
  shell_input_line_size = shell_input_line_len = shell_input_line_index = 0;

#if defined(HANDLE_MULTIBYTE)
  shell_input_line_property = 0;
  shell_input_line_propsize = 0;
#endif

  return ls;
}

void
restore_input_line_state (sh_input_line_state_t *ls)
{
  FREE (shell_input_line);
  shell_input_line = ls->input_line;
  shell_input_line_size = ls->input_line_size;
  shell_input_line_len = ls->input_line_len;
  shell_input_line_index = ls->input_line_index;

#if defined(HANDLE_MULTIBYTE)
  FREE (shell_input_line_property);
  shell_input_line_property = ls->input_property;
  shell_input_line_propsize = ls->input_propsize;
#endif

#if 0
  set_line_mbstate ();
#endif
}

/************************************************
 *						*
 *	MULTIBYTE CHARACTER HANDLING		*
 *						*
 ************************************************/

#if defined(HANDLE_MULTIBYTE)

/* We don't let the property buffer get larger than this unless the line is */
#define MAX_PROPSIZE 32768

void
Shell::set_line_mbstate ()
{
  int c;
  size_t i, previ, len;
  mbstate_t mbs, prevs;
  size_t mbclen;
  int ilen;

  if (shell_input_line == nullptr)
    return;
  len = STRLEN (shell_input_line); /* XXX - shell_input_line_len ? */
  if (len == 0)
    return;
  if (shell_input_line_propsize >= MAX_PROPSIZE && len < MAX_PROPSIZE >> 1)
    {
      free (shell_input_line_property);
      shell_input_line_property = 0;
      shell_input_line_propsize = 0;
    }
  if (len + 1 > shell_input_line_propsize)
    {
      shell_input_line_propsize = len + 1;
      shell_input_line_property = (char *)xrealloc (shell_input_line_property,
                                                    shell_input_line_propsize);
    }

  if (locale_mb_cur_max == 1)
    {
      memset (shell_input_line_property, 1, len);
      return;
    }

  /* XXX - use whether or not we are in a UTF-8 locale to avoid calls to
     mbrlen */
  if (locale_utf8locale == 0)
    memset (&prevs, '\0', sizeof (mbstate_t));

  for (i = previ = 0; i < len; i++)
    {
      if (locale_utf8locale == 0)
        mbs = prevs;

      c = shell_input_line[i];
      if (c == EOF)
        {
          size_t j;
          for (j = i; j < len; j++)
            shell_input_line_property[j] = 1;
          break;
        }

      if (locale_utf8locale)
        {
          if ((unsigned char)shell_input_line[previ] < 128) /* i != previ */
            mbclen = 1;
          else
            {
              ilen = utf8_mblen (shell_input_line + previ, i - previ + 1);
              mbclen = (ilen == -1)
                           ? (size_t)-1
                           : ((ilen == -2) ? (size_t)-2 : (size_t)ilen);
            }
        }
      else
        mbclen = mbrlen (shell_input_line + previ, i - previ + 1, &mbs);

      if (mbclen == 1 || mbclen == (size_t)-1)
        {
          mbclen = 1;
          previ = i + 1;
        }
      else if (mbclen == (size_t)-2)
        mbclen = 0;
      else if (mbclen > 1)
        {
          mbclen = 0;
          previ = i + 1;
          if (locale_utf8locale == 0)
            prevs = mbs;
        }
      else
        {
          size_t j;
          for (j = i; j < len; j++)
            shell_input_line_property[j] = 1;
          break;
        }

      shell_input_line_property[i] = mbclen;
    }
}
#endif /* HANDLE_MULTIBYTE */

} // namespace bash
