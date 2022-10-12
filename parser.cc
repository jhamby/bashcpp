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

#include "config.h"

#include "shell.hh"

namespace bash
{

#if defined(HANDLE_MULTIBYTE)
#define last_shell_getc_is_singlebyte                                         \
  ((shell_input_line.size () > 1)                                             \
       ? shell_input_line_property[shell_input_line.size () - 1]              \
       : true)
#define MBTEST(x) ((x) && last_shell_getc_is_singlebyte)
#else
#define last_shell_getc_is_singlebyte true
#define MBTEST(x) ((x))
#endif

/* Initial size to reserve for tokens. */
#define TOKEN_DEFAULT_INITIAL_SIZE 496

/* Should we call prompt_again? */
#define SHOULD_PROMPT()                                                       \
  (interactive                                                                \
   && (bash_input.type == st_stdin || bash_input.type == st_stream))

#if defined(ALIAS)
#define expanding_alias() (pushed_string_list && pushed_string_list->expander)
#else
#define expanding_alias() (false)
#endif

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
  bash_input.name.clear ();
  bash_input.location.file = nullptr;
  bash_input.location.string = nullptr;
  bash_input.getter = nullptr;
  bash_input.ungetter = nullptr;
}

/* Set the contents of the current bash input stream from
   GET, UNGET, TYPE, NAME, and LOCATION. */
void
Shell::init_yy_io (sh_cget_func_t get, sh_cunget_func_t unget,
                   stream_type type, string_view name, INPUT_STREAM location)
{
  bash_input.type = type;
  bash_input.name = to_string (name);

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
      current_readline_line = readline (current_readline_prompt);

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

  if (bash_input.type != st_stdin && !stream_on_stack (st_stdin))
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
Shell::with_input_from_string (char *string, string_view name)
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
     parse_and_execute or xparse_dolparen? xparse_dolparen needs to know how
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
Shell::with_input_from_stream (FILE *stream, string_view name)
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
  bash_input.name.clear ();
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

      delete saver;
    }
}

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

#if defined(ALIAS)
  if (pushed_string_list->expand_alias)
    parser_state |= PST_ALEXPNEXT;
  else
    parser_state &= ~PST_ALEXPNEXT;
#endif

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
const std::string *
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
        continue;

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
        return &read_a_line_buffer;
    }
}

/* Return a line as in read_a_line (), but insure that the prompt is
   the secondary prompt.  This is used to read the lines of a here
   document.  REMOVE_QUOTED_NEWLINE is non-zero if we should remove
   newlines quoted with backslashes while reading the line.  It is
   non-zero unless the delimiter of the here document was quoted. */
const std::string *
Shell::read_secondary_line (bool remove_quoted_newline)
{
  prompt_string_pointer = &ps2_prompt;
  if (SHOULD_PROMPT ())
    prompt_again ();

  const std::string *ret = read_a_line (remove_quoted_newline);

#if defined(HISTORY)
  if (ret && !ret->empty () && remember_on_history
      && (parser_state & PST_HEREDOC))
    {
      /* To make adding the here-document body right, we need to rely on
         history_delimiting_chars() returning \n for the first line of the
         here-document body and the null string for the second and subsequent
         lines, so we avoid double newlines.
         current_command_line_count == 2 for the first line of the body. */

      current_command_line_count++;
      maybe_add_history (*ret);
    }
#endif /* HISTORY */

  return ret;
}

/* **************************************************************** */
/*								    */
/*				YYLEX ()			    */
/*								    */
/* **************************************************************** */

#if __cplusplus < 201103L
#define emplace(x) insert (x)
#endif

void
Shell::init_token_lists ()
{
  // Reserved words. Only recognized as the first word of a command.

  word_token_map.emplace (std::make_pair ("if", parser::token::IF));
  word_token_map.emplace (std::make_pair ("then", parser::token::THEN));
  word_token_map.emplace (std::make_pair ("else", parser::token::ELSE));
  word_token_map.emplace (std::make_pair ("elif", parser::token::ELIF));
  word_token_map.emplace (std::make_pair ("fi", parser::token::FI));
  word_token_map.emplace (std::make_pair ("case", parser::token::CASE));
  word_token_map.emplace (std::make_pair ("esac", parser::token::ESAC));
  word_token_map.emplace (std::make_pair ("for", parser::token::FOR));
#if defined(SELECT_COMMAND)
  word_token_map.emplace (std::make_pair ("select", parser::token::SELECT));
#endif
  word_token_map.emplace (std::make_pair ("while", parser::token::WHILE));
  word_token_map.emplace (std::make_pair ("until", parser::token::UNTIL));
  word_token_map.emplace (std::make_pair ("do", parser::token::DO));
  word_token_map.emplace (std::make_pair ("done", parser::token::DONE));
  word_token_map.emplace (std::make_pair ("in", parser::token::IN));
  word_token_map.emplace (
      std::make_pair ("function", parser::token::FUNCTION));
#if defined(COMMAND_TIMING)
  word_token_map.emplace (std::make_pair ("time", parser::token::TIME));
#endif
  word_token_map.emplace (std::make_pair ("{", parser::token_kind_type ('{')));
  word_token_map.emplace (std::make_pair ("}", parser::token_kind_type ('}')));
  word_token_map.emplace (std::make_pair ("!", parser::token::BANG));
#if defined(COND_COMMAND)
  word_token_map.emplace (std::make_pair ("[[", parser::token::COND_START));
  word_token_map.emplace (std::make_pair ("]]", parser::token::COND_END));
#endif
#if defined(COPROCESS_SUPPORT)
  word_token_map.emplace (std::make_pair ("coproc", parser::token::COPROC));
#endif

  // other tokens that can be returned by read_token ()

  /* Multiple-character tokens with special values */

  other_token_map.emplace (std::make_pair ("--", parser::token::TIMEIGN));
  other_token_map.emplace (std::make_pair ("-p", parser::token::TIMEOPT));
  other_token_map.emplace (std::make_pair ("&&", parser::token::AND_AND));
  other_token_map.emplace (std::make_pair ("||", parser::token::OR_OR));
  other_token_map.emplace (
      std::make_pair (">>", parser::token::GREATER_GREATER));
  other_token_map.emplace (std::make_pair ("<<", parser::token::LESS_LESS));
  other_token_map.emplace (std::make_pair ("<&", parser::token::LESS_AND));
  other_token_map.emplace (std::make_pair (">&", parser::token::GREATER_AND));
  other_token_map.emplace (std::make_pair (";;", parser::token::SEMI_SEMI));
  other_token_map.emplace (std::make_pair (";&", parser::token::SEMI_AND));
  other_token_map.emplace (
      std::make_pair (";;&", parser::token::SEMI_SEMI_AND));
  other_token_map.emplace (
      std::make_pair ("<<-", parser::token::LESS_LESS_MINUS));
  other_token_map.emplace (
      std::make_pair ("<<<", parser::token::LESS_LESS_LESS));
  other_token_map.emplace (std::make_pair ("&>", parser::token::AND_GREATER));
  other_token_map.emplace (
      std::make_pair ("&>>", parser::token::AND_GREATER_GREATER));
  other_token_map.emplace (std::make_pair ("<>", parser::token::LESS_GREATER));
  other_token_map.emplace (std::make_pair (">|", parser::token::GREATER_BAR));
  other_token_map.emplace (std::make_pair ("|&", parser::token::BAR_AND));
  other_token_map.emplace (std::make_pair ("EOF", parser::token::yacc_EOF));

  /* Tokens whose value is the character itself */

  other_token_map.emplace (
      std::make_pair (">", parser::token_kind_type ('>')));
  other_token_map.emplace (
      std::make_pair ("<", parser::token_kind_type ('<')));
  other_token_map.emplace (
      std::make_pair ("-", parser::token_kind_type ('-')));
  other_token_map.emplace (
      std::make_pair ("{", parser::token_kind_type ('{')));
  other_token_map.emplace (
      std::make_pair ("}", parser::token_kind_type ('}')));
  other_token_map.emplace (
      std::make_pair (";", parser::token_kind_type (';')));
  other_token_map.emplace (
      std::make_pair ("(", parser::token_kind_type ('(')));
  other_token_map.emplace (
      std::make_pair (")", parser::token_kind_type (')')));
  other_token_map.emplace (
      std::make_pair ("|", parser::token_kind_type ('|')));
  other_token_map.emplace (
      std::make_pair ("&", parser::token_kind_type ('&')));
  other_token_map.emplace (
      std::make_pair ("newline", parser::token_kind_type ('\n')));

#if defined(HISTORY)

  no_semi_successors.emplace (static_cast<parser::token_kind_type> ('\n'));
  no_semi_successors.emplace (static_cast<parser::token_kind_type> ('{'));
  no_semi_successors.emplace (static_cast<parser::token_kind_type> ('('));
  no_semi_successors.emplace (static_cast<parser::token_kind_type> (')'));
  no_semi_successors.emplace (static_cast<parser::token_kind_type> (';'));
  no_semi_successors.emplace (static_cast<parser::token_kind_type> ('&'));
  no_semi_successors.emplace (static_cast<parser::token_kind_type> ('|'));
  no_semi_successors.emplace (parser::token::CASE);
  no_semi_successors.emplace (parser::token::DO);
  no_semi_successors.emplace (parser::token::ELSE);
  no_semi_successors.emplace (parser::token::IF);
  no_semi_successors.emplace (parser::token::SEMI_SEMI);
  no_semi_successors.emplace (parser::token::SEMI_AND);
  no_semi_successors.emplace (parser::token::SEMI_SEMI_AND);
  no_semi_successors.emplace (parser::token::THEN);
  no_semi_successors.emplace (parser::token::UNTIL);
  no_semi_successors.emplace (parser::token::WHILE);
  no_semi_successors.emplace (parser::token::AND_AND);
  no_semi_successors.emplace (parser::token::OR_OR);
  no_semi_successors.emplace (parser::token::IN);

#endif
}

#if __cplusplus < 201103L
#undef emplace
#endif

/* others not listed here (values contained in parser::symbol_type):
        WORD
        ASSIGNMENT_WORD
        NUMBER
        ARITH_CMD
        ARITH_FOR_EXPRS
        COND_CMD
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
          expansions = pre_process_line (shell_input_line, true, true);
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
            maybe_add_history (shell_input_line);
          else
            {
              std::string hdcs;
              hdcs = history_delimiting_chars (shell_input_line);
              if (!hdcs.empty () && hdcs[0] == ';')
                maybe_add_history (shell_input_line);
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

#if defined(HANDLE_MULTIBYTE)
          /* This is kind of an abstraction violation, but there's no need to
             go through the entire shell_input_line again with a call to
             set_line_mbstate(). */
          shell_input_line_property.reserve (shell_input_line.size () + 1);
          shell_input_line_property[shell_input_line.size ()] = 1;
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
#endif /* ALIAS || DPAREN_ARITHMETIC */
  /* This case works for PSH_DPAREN as well as the shell_ungets() case that
     uses push_string */
  if (uc == 0 && pushed_string_list && pushed_string_list->flags != PSH_SOURCE)
    {
      parser_state &= ~PST_ENDALIAS;
      pop_string ();
      uc = static_cast<unsigned char> (
          shell_input_line[shell_input_line_index]);
      if (uc)
        shell_input_line_index++;
    }

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

/* Push S back into shell_input_line; updating shell_input_line_index */
void
Shell::shell_ungets (string_view s)
{
  size_t slen = s.size ();

  if (slen == shell_input_line_index)
    {
      /* Easy, just overwrite shell_input_line. This is preferred because it
        saves on set_line_mbstate () and other overhead like push_string */
      shell_input_line = to_string (s);
      shell_input_line_index = 0;
      shell_input_line_terminator = 0;
    }
  else if (shell_input_line_index >= slen)
    {
      /* Just as easy, just back up shell_input_line_index, but it means we
        will re-process some characters in set_line_mbstate(). Need to
        watch pushing back newlines here. */
      while (slen > 0)
        shell_input_line[--shell_input_line_index] = s[--slen];
    }
  else if (s[slen - 1] == '\n')
    {
      push_string (savestring (s), 0, nullptr);
      /* push_string does set_line_mbstate () */
      return;
    }
  else
    {
      /* Harder case: pushing back input string that's longer than what we've
        consumed from shell_input_line so far. */
      internal_debug ("shell_ungets: not at end of shell_input_line");
      shell_input_line.replace (0, shell_input_line_index, to_string (s));
      shell_input_line_index = 0;
    }

#if defined(HANDLE_MULTIBYTE)
  set_line_mbstate (); /* XXX */
#endif
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
Shell::execute_variable_command (string_view command, string_view vname)
{
  sh_parser_state_t ps;

  save_parser_state (&ps);
  const char *last_lastarg = get_string_value ("_");

  parse_and_execute (savestring (command), vname,
                     (SEVAL_NONINT | SEVAL_NOHIST));

  restore_parser_state (&ps);
  bind_variable ("_", last_lastarg, 0);

  if (token_to_read == '\n') /* reset_parser was called */
    token_to_read = parser::token::YYEOF;
}

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

  // read_token () will set 'current_token' to the new value.
  return read_token (READ);
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

/* Are we in a position where we can read an assignment statement? */
#define assignment_acceptable(token)                                          \
  (command_token_position (token) && ((parser_state & PST_CASEPAT) == 0))

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
Shell::alias_expand_token (string_view tokstr)
{
  char *expanded;
  alias_t *ap;

#if 0
  if (((parser_state & PST_ALEXPNEXT)
       || command_token_position (last_read_token))
      && (parser_state & PST_CASEPAT) == 0)
#else
  if ((parser_state & PST_ALEXPNEXT)
      || assignment_acceptable (last_read_token))
#endif
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
  if (posixly_correct && shell_compatibility_level > 41)
    {
      /* Quick check of the rest of the line to find the next token.  If it
         begins with a `-', Posix says to not return `time' as the token.
         This was interp 267. */
      size_t i = shell_input_line_index;
      while (i < shell_input_line.size ()
             && (shell_input_line[i] == ' ' || shell_input_line[i] == '\t'))
        i++;
      if (shell_input_line[i] == '-')
        return false;
    }

  // cast to int to suppress warnings about case values not in enum type
  switch (static_cast<int> (last_read_token))
    {
    case parser::token::YYEOF:
    case static_cast<parser::token_kind_type> (';'):
    case static_cast<parser::token_kind_type> ('\n'):
      if (token_before_that == '|')
        return false;
      __attribute__ ((fallthrough));
      /* FALLTHROUGH */

    case parser::token::AND_AND:
    case parser::token::OR_OR:
    case static_cast<parser::token_kind_type> ('&'):
    case parser::token::WHILE:
    case parser::token::DO:
    case parser::token::UNTIL:
    case parser::token::IF:
    case parser::token::THEN:
    case parser::token::ELIF:
    case parser::token::ELSE:
    case static_cast<parser::token_kind_type> ('{'): // }
    case static_cast<parser::token_kind_type> ('('): // )(
    case static_cast<parser::token_kind_type> (
        ')'):                    // only valid in case statement
    case parser::token::BANG:    // ! time pipeline
    case parser::token::TIME:    // time time pipeline
    case parser::token::TIMEOPT: // time -p time pipeline
    case parser::token::TIMEIGN: // time -p -- ...
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
parser::symbol_type
Shell::special_case_tokens (string_view tokstr)
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
      (tokstr == "in"))
    {
      if (token_before_that == parser::token::CASE)
        {
          parser_state |= PST_CASEPAT;
          esacs_needed_count++;
        }
      if (expecting_in_token)
        expecting_in_token--;
      return parser::make_IN ();
    }

  /* XXX - leaving above code intact for now, but it should eventually be
     removed in favor of this clause. */
  /* Posix grammar rule 6 */
  if (expecting_in_token
      && (last_read_token == parser::token::WORD || last_read_token == '\n')
      && (tokstr == "in"))
    {
      if (parser_state & PST_CASESTMT)
        {
          parser_state |= PST_CASEPAT;
          esacs_needed_count++;
        }
      expecting_in_token--;
      return parser::make_IN ();
    }
  /* Posix grammar rule 6, third word in FOR: for i; do command-list; done */
  else if (expecting_in_token
           && (last_read_token == '\n' || last_read_token == ';')
           && (tokstr == "do"))
    {
      expecting_in_token--;
      return parser::make_DO ();
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
      (tokstr == "do"))
    {
      if (expecting_in_token)
        expecting_in_token--;
      return parser::make_DO ();
    }

  /* Ditto for ESAC in the CASE case.
     Specifically, this handles "case word in esac", which is a legal
     construct, certainly because someone will pass an empty arg to the
     case construct, and we don't want it to barf.  Of course, we should
     insist that the case construct has at least one pattern in it, but
     the designers disagree. */
  if (esacs_needed_count)
    {
      if (last_read_token == parser::token::IN && tokstr == "esac")
        {
          esacs_needed_count--;
          parser_state &= ~PST_CASEPAT;
          return parser::make_ESAC ();
        }
    }

  /* The start of a shell function definition. */
  if (parser_state & PST_ALLOWOPNBRC)
    {
      parser_state &= ~PST_ALLOWOPNBRC;
      if (tokstr == "{") // }
        {
          open_brace_count++;
          function_bstart = line_number;
          return parser::symbol_type ('{'); // }
        }
    }

  /* We allow a `do' after a for ((...)) without an intervening
     list_terminator */
  if (last_read_token == parser::token::ARITH_FOR_EXPRS && tokstr == "do")
    return parser::make_DO ();

  if (last_read_token == parser::token::ARITH_FOR_EXPRS && tokstr == "{")
    {
      open_brace_count++;
      return parser::symbol_type ('{'); // }
    }

  if (open_brace_count && reserved_word_acceptable (last_read_token)
      && tokstr == "}")
    {
      open_brace_count--; // {
      return parser::symbol_type ('}');
    }

#if defined(COMMAND_TIMING)
  /* Handle -p after `time'. */
  if (last_read_token == parser::token::TIME && tokstr == "-p")
    return parser::make_TIMEOPT ();

  /* Handle -- after `time'. */
  if (last_read_token == parser::token::TIME && tokstr == "--")
    return parser::make_TIMEIGN ();

  /* Handle -- after `time -p'. */
  if (last_read_token == parser::token::TIMEOPT && tokstr == "--")
    return parser::make_TIMEIGN ();
#endif

#if defined(COND_COMMAND)
  if ((parser_state & PST_CONDEXPR) && tokstr == "]]")
    return parser::make_COND_END ();
#endif

  return parser::make_YYerror ();
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
  if (parser_state & (PST_EXTPAT | PST_CMDSUBST))
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

  delete word_desc_to_read.value;
  word_desc_to_read.value = nullptr;

  eol_ungetc_lookahead = 0;

  /* added post-bash-5.1 */
  need_here_doc = 0;
  redir_stack[0] = nullptr;
  esacs_needed_count = expecting_in_token = 0;

  current_token = static_cast<parser::token_kind_type> ('\n');
  last_read_token = static_cast<parser::token_kind_type> ('\n');
  token_to_read = static_cast<parser::token_kind_type> ('\n');
}

/* Read the next token.  Command can be READ (normal operation) or
   RESET (to normalize state). CURRENT_TOKEN is set to the token
   that was read, before returning a new symbol to the parser. */
parser::symbol_type
Shell::read_token (read_token_cmd command)
{
  int character; /* Current character. */
  int peek_char; /* Temporary look-ahead character. */

  if (command == RESET)
    {
      reset_parser (); // this sets current_token
      return parser::symbol_type ('\n');
    }

  if (token_to_read)
    {
      current_token = token_to_read;
      token_to_read = parser::token::YYEOF;

      if (current_token == parser::token::WORD)
        {
          WORD_DESC_PTR wdp = word_desc_to_read;
          word_desc_to_read.value = nullptr;
          return parser::make_WORD (wdp);
        }
      else if (current_token == parser::token::ASSIGNMENT_WORD)
        {
          WORD_DESC_PTR wdp = word_desc_to_read;
          word_desc_to_read.value = nullptr;
          return parser::make_ASSIGNMENT_WORD (wdp);
        }

      return parser::symbol_type (current_token);
    }

#if defined(COND_COMMAND)
  if ((parser_state & (PST_CONDCMD | PST_CONDEXPR)) == PST_CONDCMD)
    {
      cond_lineno = line_number;
      parser_state |= PST_CONDEXPR;
      parser::symbol_type cond_symbol (parse_cond_command ());
      if (cond_symbol.kind () != parser::symbol_kind::S_COND_END)
        {
          cond_error (current_token, cond_symbol);
          return parser::make_YYerror ();
        }
      token_to_read = parser::token::COND_END;
      parser_state &= ~(PST_CONDEXPR | PST_CONDCMD);
      return cond_symbol;
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
      current_token = parser::token::yacc_EOF;
      return parser::make_yacc_EOF ();
    }

  /* If we hit the end of the string and we're not expanding an alias (e.g.,
     we are eval'ing a string that is an incomplete command), return EOF */
  if (character == '\0' && bash_input.type == st_string
      && !parser_expanding_alias ())
    {
      internal_debug ("shell_getc: bash_input.location.string = `%s'",
                      bash_input.location.string);
      EOF_Reached = true;
      current_token = parser::token::yacc_EOF;
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

  if MBTEST (character == '\n')
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
  if MBTEST (shellmeta (character))
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
        peek_char = shell_getc (false);
      else
        peek_char = shell_getc (true);

      if MBTEST (character == peek_char)
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
            case '(':
              try
                {
                  return parse_dparen (character);
                }
              catch (const parse_error &)
                {
                  break;
                }
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
      if MBTEST (character == '(' && (parser_state & PST_CASEPAT) == 0)
        parser_state |= PST_SUBSHELL;
      else if MBTEST ((parser_state & PST_CASEPAT) && character == ')')
        parser_state &= ~PST_CASEPAT;
      else if MBTEST ((parser_state & PST_SUBSHELL) && character == ')')
        parser_state &= ~PST_SUBSHELL;

#if defined(PROCESS_SUBSTITUTION)
      /* Check for the constructs which introduce process substitution.
         Shells running in `posix mode' don't do process substitution. */
      if MBTEST ((character != '>' && character != '<') || peek_char != '(')
#endif /* PROCESS_SUBSTITUTION */
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
      return read_token_word (character);

#if defined(ALIAS)
    }
  catch (const read_again_exception &)
    {
      goto re_read_token;
    }
#endif
}

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
  int count;
  int start_lineno;
  dolbrace_state_t dolbrace_state;

  dolbrace_state = (flags & P_DOLBRACE) ? DOLBRACE_PARAM : DOLBRACE_NOFLAGS;

  // itrace ("parse_matched_pair[%d]: open = %c close = %c flags = %d",
  //         line_number, open, close, flags);
  count = 1;
  lexical_state_flags tflags = LEX_NOFLAGS;

  if ((flags & P_COMMAND) && qc != '`' && qc != '\'' && qc != '"'
      && (flags & P_DQUOTE) == 0)
    tflags |= LEX_CKCOMMENT;

  /* RFLAGS is the set of flags we want to pass to recursive calls. */
  pgroup_flags rflags = (qc == '"') ? P_DQUOTE : (flags & P_DQUOTE);

  std::string ret;

  start_lineno = line_number;
  while (count)
    {
      int ch = shell_getc (qc != '\'' && (tflags & (LEX_PASSNEXT)) == 0);

      if (ch == EOF)
        {
          parser_error (start_lineno,
                        _ ("unexpected EOF while looking for matching `%c'"),
                        close);
          EOF_Reached = true; /* XXX */
          throw matched_pair_error ();
        }

      /* Possible reprompting. */
      if MBTEST (ch == '\n' && SHOULD_PROMPT ())
        prompt_again ();

      /* Don't bother counting parens or doing anything else if in a comment
         or part of a case statement */
      if (tflags & LEX_INCOMMENT)
        {
          /* Add this character. */
          ret.push_back (static_cast<char> (ch));

          if MBTEST (ch == '\n')
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
          /* XXX - PST_NOEXPAND? */
          if MBTEST (qc != '\''
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
                      && ch == open)
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
              catch (const std::exception &)
                {
                  dstack.pop_back ();
                  throw;
                }

              dstack.pop_back ();

              if MBTEST ((tflags & LEX_WASDOL) && ch == '\''
                         && (extended_quote || (rflags & P_DQUOTE) == 0
                             || dolbrace_state == DOLBRACE_QUOTE
                             || dolbrace_state == DOLBRACE_QUOTE2))
                {
                  /* Translate $'...' here. */
                  /* PST_NOEXPAND */
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
                      && (dolbrace_state == DOLBRACE_QUOTE2
                          || dolbrace_state == DOLBRACE_QUOTE)
                      && (flags & P_DOLBRACE))
                    {
                      nestret = sh_single_quote (ttrans);
                      ttrans.clear ();
                    }
#if 0 /* TAG:bash-5.3 */
                  /* This single-quotes PARAM in ${PARAM OP WORD} when PARAM
                     contains a $'...' even when extended_quote is set. */
                  else if ((rflags & P_DQUOTE)
                           && (dolbrace_state == DOLBRACE_PARAM)
                           && (flags & P_DOLBRACE))
                    {
                      nestret = sh_single_quote (ttrans);
                      ttrans.clear ();
                    }
#endif
                  else if ((rflags & P_DQUOTE) == 0)
                    {
                      nestret = sh_single_quote (ttrans);
                      ttrans.clear ();
                    }
                  else
                    {
                      nestret = ttrans;
                    }
                  ret.erase (ret.size () - 2, 2); /* back up before the $' */
                }
#if defined(TRANSLATABLE_STRINGS)
              else if MBTEST ((tflags & LEX_WASDOL) && ch == '"'
                              && (extended_quote || (rflags & P_DQUOTE) == 0))
                {
                  /* Locale expand $"..." here. */
                  /* PST_NOEXPAND */
                  std::string ttrans = locale_expand (nestret, start_lineno);

                  /* If we're supposed to single-quote translated strings,
                    check whether the translated result is different from
                    the original and single-quote the string if it is. */
                  if (singlequote_translations
                      && ((nestret.size () - 1) != ttrans.size ()
                          || nestret == ttrans))
                    {
                      if ((rflags & P_DQUOTE) == 0)
                        nestret = sh_single_quote (ttrans);
                      else if ((rflags & P_DQUOTE)
                               && (dolbrace_state == DOLBRACE_QUOTE2)
                               && (flags & P_DOLBRACE))
                        nestret = sh_single_quote (ttrans);
                      else
                        // single quotes aren't special, use backslash instead
                        nestret
                            = sh_backslash_quote_for_double_quotes (ttrans);
                    }
                  else
                    nestret = sh_mkdoublequoted (ttrans, 0);

                  ret.erase (ret.size () - 2, 2); /* back up before the $" */
                }
#endif /* TRANSLATABLE_STRINGS */

              ret += nestret;
            }
          else if ((flags & (P_ARRAYSUB | P_DOLBRACE)) && (tflags & LEX_WASDOL)
                   && (ch == '(' || ch == '{' || ch == '[')) /* ) } ] */
            goto parse_dollar_word;
#if defined(PROCESS_SUBSTITUTION)
          /* XXX - technically this should only be recognized at the start of
             a word */
          else if ((flags & (P_ARRAYSUB | P_DOLBRACE)) && (tflags & LEX_GTLT)
                   && (ch == '('))
            goto parse_dollar_word;
#endif
        }

      /* Parse an old-style command substitution within double quotes as a
         single word. */
      /* XXX - sh and ksh93 don't do this - XXX */
      else if MBTEST (open == '"' && ch == '`')
        {
          // this call may throw matched_pair_error
          std::string nestret = parse_matched_pair (0, '`', '`', rflags);
          ret += nestret;
        }
      else if MBTEST (open != '`' && (tflags & LEX_WASDOL)
                      && (ch == '(' || ch == '{' || ch == '[')) /* ) } ] */
        /* check for $(), $[], or ${} inside quoted string. */
        {
        parse_dollar_word:
          std::string nestret;
          if (open == ch) /* undo previous increment */
            count--;
          if (ch == '(')
            nestret = parse_comsub (0, '(', ')');
          else if (ch == '{')
            // this call may throw matched_pair_error
            nestret = parse_matched_pair (
                0, '{', '}', (P_FIRSTCLOSE | P_DOLBRACE | rflags));
          else if (ch == '[')
            // this call may throw matched_pair_error
            nestret = parse_matched_pair (0, '[', ']', rflags);

          ret += nestret;
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

  // itrace ("parse_matched_pair[%d]: returning %s", line_number, ret.c_str
  //         ());
  return ret;
}

/* Parse a $(...) command substitution.  This reads input from the current
   input stream. */
std::string
Shell::parse_comsub (int qc, int open, int close)
{
  sh_parser_state_t ps;
  STRING_SAVER *saved_strings;
  COMMAND *saved_global, *parsed_command;

  /* Posix interp 217 says arithmetic expressions have precedence, so
     assume $(( introduces arithmetic expansion and parse accordingly. */
  if (open == '(')
    {
      int peekc = shell_getc (1);
      shell_ungetc (peekc);
      if (peekc == '(')
        return parse_matched_pair (qc, open, close, P_NOFLAGS);
    }

  /*itrace("parse_comsub: qc = `%c' open = %c close = %c", qc, open, close);*/

  /*debug_parser(1);*/

  save_parser_state (&ps);

  bool was_extpat = (parser_state & PST_EXTPAT);

  /* State flags we don't want to persist into command substitutions. */
  parser_state &= ~(PST_REGEXP | PST_EXTPAT | PST_CONDCMD | PST_CONDEXPR
                    | PST_COMPASSIGN);
  /* Could do PST_CASESTMT too, but that also affects history. Setting
     expecting_in_token below should take care of the parsing requirements.
     Unsetting PST_REDIRLIST isn't strictly necessary because of how we set
     token_to_read below, but we do it anyway. */
  parser_state
      &= ~(PST_CASEPAT | PST_ALEXPNEXT | PST_SUBSHELL | PST_REDIRLIST);
  /* State flags we want to set for this run through the parser. */
  parser_state |= PST_CMDSUBST | PST_EOFTOKEN | PST_NOEXPAND;

  /* leave pushed_string_list alone, since we might need to consume characters
     from it to satisfy this command substitution (in some perverse case). */
  shell_eof_token = close;

  saved_global = global_command; /* might not be necessary */
  global_command = nullptr;

  /* These are reset by reset_parser() */
  need_here_doc = 0;
  esacs_needed_count = expecting_in_token = 0;

  /* We want to expand aliases on this pass if we're in posix mode, since the
     standard says you have to take aliases into account when looking for the
     terminating right paren. Otherwise, we defer until execution time for
     backwards compatibility. */
  if (expand_aliases)
    expand_aliases = (posixly_correct != 0);

#if defined(EXTENDED_GLOB)
  char local_extglob = 0;

  /* If (parser_state & PST_EXTPAT), we're parsing an extended pattern for a
     conditional command and have already set global_extglob appropriately. */
  if (shell_compatibility_level <= 51 && !was_extpat)
    {
      local_extglob = global_extglob = extended_glob;
      extended_glob = 1;
    }
#endif

  current_token = static_cast<parser::token_kind_type> ('\n'); /* XXX */
  token_to_read = parser::token::DOLPAREN; /* let's trick the parser */

  int r = yyparse ();

  if (need_here_doc > 0)
    {
      internal_warning (
          "command substitution: %d unterminated here-document%s",
          need_here_doc, (need_here_doc == 1) ? "" : "s");
      gather_here_documents (); /* XXX check compatibility level? */
    }

#if defined(EXTENDED_GLOB)
  if (shell_compatibility_level <= 51 && !was_extpat)
    extended_glob = local_extglob;
#endif

  parsed_command = global_command;

  if (EOF_Reached)
    {
      shell_eof_token = ps.eof_token;
      expand_aliases = ps.expand_aliases;

      /* yyparse() has already called yyerror() and reset_parser() */
      throw matched_pair_error ();
    }
  else if (r != 0)
    {
      /* parser_error (start_lineno, _("could not parse command
       * substitution")); */
      // Non-interactive shells exit on parse error in a command substitution.
      if (last_command_exit_value == 0)
        last_command_exit_value = EXECUTION_FAILURE;

      set_exit_status (last_command_exit_value);

      if (!interactive_shell)
        throw bash_exception (FORCE_EOF); /* This is like reader_loop() */
      else
        {
          shell_eof_token = ps.eof_token;
          expand_aliases = ps.expand_aliases;

          throw bash_exception (
              DISCARD); /* XXX - throw matched_pair_error ()? */
        }
    }

  if (current_token != shell_eof_token)
    {
      internal_debug ("current_token (%d) != shell_eof_token (%c)",
                      current_token, shell_eof_token);

      token_to_read = current_token;

      /* If we get here we can check eof_encountered and if it's 1 but the
         previous EOF_Reached test didn't succeed, we can assume that the shell
         is interactive and ignoreeof is set. We might want to restore the
         parser state in this case. */
      shell_eof_token = ps.eof_token;
      expand_aliases = ps.expand_aliases;

      throw matched_pair_error ();
    }

  /* We don't want to restore the old pushed string list, since we might have
     used it to consume additional input from an alias while parsing this
     command substitution. */
  saved_strings = pushed_string_list;
  restore_parser_state (&ps);
  pushed_string_list = saved_strings;

  std::string tcmd (print_comsub (parsed_command));

  size_t retlen = tcmd.size ();
  if (tcmd[0] == '(') /* ) need a space to prevent arithmetic expansion */
    retlen++;

  std::string ret;
  ret.reserve (retlen + 1);

  if (tcmd[0] == '(')
    {
      ret.push_back (' ');
      ret += tcmd;
    }
  else
    ret = tcmd;

  ret.push_back (')');

  delete parsed_command;
  global_command = saved_global;

  // itrace("parse_comsub:%d: returning `%s'", line_number, ret.c_str ());
  return ret;
}

/* Recursively call the parser to parse a $(...) command substitution. This is
   called by the word expansion code and so does not have to reset as much
   parser state before calling yyparse(). */
char *
Shell::xparse_dolparen (char *base, char *string, size_t *indp, sx_flags flags)
{
  sh_parser_state_t ps;
  sh_input_line_state_t ls;

  // debug_parser (1);

  char *ostring = string;
  int start_lineno = line_number;

  // itrace ("xparse_dolparen: size = %d shell_input_line = `%s' string=`%s'",
  //         shell_input_line.size (), shell_input_line.c_str (),
  //         to_string (string).c_str ());

  if (*string == '\0')
    {
      if (flags & SX_NOALLOC)
        return nullptr;

      char *ret = new char[1];
      ret[0] = '\0';
      return ret;
    }

  parse_flags sflags = SEVAL_NONINT | SEVAL_NOHIST | SEVAL_NOFREE;
  if (flags & SX_NOTHROW)
    sflags |= SEVAL_NOTHROW;

  save_parser_state (&ps);
  save_input_line_state (&ls);

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  pushed_string_list = nullptr;
#endif

  parser_state |= (PST_CMDSUBST | PST_EOFTOKEN); /* allow instant ')' */
  shell_eof_token = ')';
  if (flags & SX_COMPLETE)
    parser_state |= PST_NOERROR;

  /* Don't expand aliases on this pass at all. Either parse_comsub() does it
     at parse time, in which case this string already has aliases expanded,
     or command_substitute() does it in the child process executing the
     command substitution and we want to defer it completely until then. The
     old value will be restored by restore_parser_state(). */
  expand_aliases = 0;

#if defined(EXTENDED_GLOB)
  global_extglob = extended_glob; /* for reset_parser() */
#endif

  token_to_read = parser::token::DOLPAREN; /* let's trick the parser */

  bash_exception_t exception = NOEXCEPTION;
  char *ep = nullptr;

  try
    {
      // const_cast<> is safe because we set SEVAL_NOFREE above.
      (void)parse_string (const_cast<char *> (string), "command substitution",
                          sflags, nullptr, &ep);
    }
  catch (const bash_exception &e)
    {
      exception = e.type;
    }

  reset_parser ();

  /* reset_parser() clears shell_input_line and associated variables, including
     parser_state, so we want to reset things, then restore what we need. */
  restore_input_line_state (&ls);
  restore_parser_state (&ps);

  token_to_read = parser::token::YYEOF;

  /* If parse_string returns < 0, we need to jump to top level with the
     negative of the return value. We abandon the rest of this input line
     first */
  if (exception)
    {
      clear_shell_input_line ();        /* XXX */
      if (bash_input.type != st_string) /* paranoia */
        parser_state &= ~(PST_CMDSUBST | PST_EOFTOKEN);
      if ((flags & SX_NOTHROW) == 0)
        throw bash_exception (exception); /* XXX */
    }

  /* Need to find how many characters parse_string() consumed, update
     *indp, if flags != 0, copy the portion of the string parsed into RET
     and return it.  If flags & 1 (SX_NOALLOC) we can return nullptr. */

  if (ep[-1] != ')')
    {
#if 0
      if (ep[-1] != '\n')
        itrace ("xparse_dolparen:%d: ep[-1] != RPAREN (%d), ep = `%s'",
                line_number, ep[-1], ep);
#endif

      while (ep > ostring && ep[-1] == '\n')
        ep--;
    }

  size_t nc = static_cast<size_t> (ep - ostring);
  *indp = static_cast<size_t> (ep - base - 1);

#if 0
  if (base[*indp] != ')')
    itrace ("xparse_dolparen:%d: base[%d] != RPAREN (%d), base = `%s'",
            line_number, *indp, base[*indp], base);
  if (*indp < orig_ind)
    itrace (
        "xparse_dolparen:%d: *indp (%d) < orig_ind (%d), orig_string = `%s'",
        line_number, *indp, orig_ind, ostring);
#endif

  if (base[*indp] != ')' && (flags & SX_NOTHROW) == 0)
    {
      if ((flags & SX_NOERROR) == 0)
        parser_error (start_lineno,
                      _ ("unexpected EOF while looking for matching `%c'"),
                      ')');
      throw bash_exception (DISCARD);
    }

  if (flags & SX_NOALLOC)
    return nullptr;

  if (nc == 0)
    {
      char *ret = new char[1];
      ret[0] = '\0';
      return ret;
    }
  else
    return substring (ostring, 0, nc - 1);
}

/* Recursively call the parser to parse the string from a $(...) command
   substitution to a COMMAND *. This is called from command_substitute () and
   has the same parser state constraints as xparse_dolparen(). */
COMMAND *
Shell::parse_string_to_command (const char *string, sx_flags flags)
{
  sh_parser_state_t ps;
  sh_input_line_state_t ls;

  size_t slen = strlen (string);

  if (!string || *string == '\0')
    return nullptr;

  /*itrace("parse_string_to_command: size = %d shell_input_line = `%s'
   * string=`%s'", shell_input_line_size, shell_input_line, string);*/

  parse_flags sflags = SEVAL_NONINT | SEVAL_NOHIST | SEVAL_NOFREE;

  if (flags & SX_NOTHROW)
    sflags |= SEVAL_NOTHROW;

  save_parser_state (&ps);
  save_input_line_state (&ls);

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  pushed_string_list = nullptr;
#endif

  if (flags & SX_COMPLETE)
    parser_state |= PST_NOERROR;

  expand_aliases = 0;

  bash_exception_t exception = NOEXCEPTION;
  size_t nc = 0;
  COMMAND *cmd = nullptr;
  try
    {
      // const_cast<> is safe because we set SEVAL_NOFREE above.
      nc = parse_string (const_cast<char *> (string), "command substitution",
                         sflags, &cmd, nullptr);
    }
  catch (const bash_exception &e)
    {
      exception = e.type;
    }

  reset_parser ();
  /* reset_parser() clears shell_input_line and associated variables, including
     parser_state, so we want to reset things, then restore what we need. */
  restore_input_line_state (&ls);
  restore_parser_state (&ps);

  /* If parse_string returns < 0, we need to jump to top level with the
     negative of the return value. We abandon the rest of this input line
     first */
  if (exception)
    {
      clear_shell_input_line (); /* XXX */
      if ((flags & SX_NOTHROW) == 0)
        throw bash_exception (exception); /* XXX */
    }

  /* Need to check how many characters parse_string() consumed, make sure it's
     the entire string. */
  if (nc < slen)
    {
      delete cmd;
      return nullptr;
    }

  return cmd;
}

#if defined(DPAREN_ARITHMETIC) || defined(ARITH_FOR_COMMAND)
/* Parse a double-paren construct.  It can be either an arithmetic
   command, an arithmetic `for' command, or a nested subshell.  Returns
   the parsed token, -1 on error, or -2 if we didn't do anything and
   should just go on. */
parser::symbol_type
Shell::parse_dparen (int c)
{
#if defined(ARITH_FOR_COMMAND)
  if (last_read_token == parser::token::FOR)
    {
      if (word_top < MAX_CASE_NEST)
        word_top++;

      arith_for_lineno = word_lineno[word_top] = line_number;

      char *wval;
      int cmdtyp = parse_arith_cmd (&wval, 0);
      if (cmdtyp == 1)
        {
          WORD_DESC *wd = new WORD_DESC (wval);
          return parser::make_ARITH_FOR_EXPRS (WORD_LIST_PTR (wd));
        }
      else
        throw parse_error (); /* ERROR */
    }
#endif

#if defined(DPAREN_ARITHMETIC)
  if (reserved_word_acceptable (last_read_token))
    {
      char *wval;
      int cmdtyp = parse_arith_cmd (&wval, 0);
      if (cmdtyp == 1) /* arithmetic command */
        {
          WORD_DESC *wd = new WORD_DESC (wval, W_QUOTED | W_NOSPLIT | W_NOGLOB
                                                   | W_NOTILDE | W_NOPROCSUB);
          delete[] wval;

          return parser::make_ARITH_CMD (WORD_LIST_PTR (wd));
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
        throw parse_error ();
    }
#endif

  throw parse_error (); /* XXX */
}

/* We've seen a `(('.  Look for the matching `))'.  If we get it, return 1.
   If not, assume it's a nested subshell for backwards compatibility and
   return 0.  In any case, put the characters we've consumed into a locally-
   allocated buffer and make *ep point to that buffer.  Return -1 on an
   error, for example EOF. */
int
Shell::parse_arith_cmd (char **ep, bool adddq)
{
  std::string ttok;

  try
    {
      ttok = parse_matched_pair (0, '(', ')', P_NOFLAGS);
    }
  catch (const matched_pair_error &)
    {
      return -1;
    }

  int rval = 1;

  /* Check that the next character is the closing right paren.  If
     not, this is a syntax error. ( */
  int c = shell_getc (0);
  if MBTEST (c != ')')
    rval = 0;

  std::string tokstr;

  /* if ADDDQ != 0 then (( ... )) -> "..." */
  if (rval == 1 && adddq) /* arith cmd, add double quotes */
    {
      tokstr.push_back ('"');
      tokstr += ttok;
      tokstr.push_back ('"');
    }
  else if (rval == 1) /* arith cmd, don't add double quotes */
    {
      tokstr = ttok;
    }
  else /* nested subshell */
    {
      tokstr.push_back ('(');
      tokstr += ttok;
      tokstr.push_back (')');
      tokstr.push_back (static_cast<char> (c));
    }

  *ep = savestring (tokstr);
  return rval;
}
#endif /* DPAREN_ARITHMETIC || ARITH_FOR_COMMAND */

#if defined(COND_COMMAND)
void
Shell::cond_error (parser::token_kind_type token,
                   const parser::symbol_type &cond_symbol)
{
  if (EOF_Reached && cond_symbol.kind () != parser::symbol_kind::S_COND_ERROR)
    parser_error (cond_lineno, _ ("unexpected EOF while looking for `]]'"));
  else if (cond_symbol.kind () != parser::symbol_kind::S_COND_ERROR)
    {
      std::string etext (error_string_from_token (token, &cond_symbol));
      if (!etext.empty ())
        {
          parser_error (cond_lineno,
                        _ ("syntax error in conditional expression: "
                           "unexpected token `%s'"),
                        etext.c_str ());
        }
      else
        parser_error (cond_lineno,
                      _ ("syntax error in conditional expression"));
    }
}

parser::symbol_type
Shell::cond_skip_newlines ()
{
  parser::token_kind_type saved_current_token = current_token;
  while (true)
    {
      parser::symbol_type new_token = read_token (READ);
      if (current_token != '\n')
        {
          cond_token = current_token;
          current_token = saved_current_token;
          return new_token;
        }

      if (SHOULD_PROMPT ())
        prompt_again ();
    }
}

#define COND_RETURN_ERROR()                                                   \
  do                                                                          \
    {                                                                         \
      cond_token = parser::token::COND_ERROR;                                 \
      return nullptr;                                                         \
    }                                                                         \
  while (0)

COND_COM *
Shell::cond_term ()
{
  WORD_DESC *op;
  COND_COM *term, *tleft, *tright;
  int lineno;

  /* Read a token.  It can be a left paren, a `!', a unary operator, or a
     word that should be the first argument of a binary operator.  Start by
     skipping newlines, since this is a compound command. */
  parser::symbol_type symbol (cond_skip_newlines ());
  lineno = line_number;
  if (cond_token == parser::token::COND_END)
    {
      COND_RETURN_ERROR ();
    }
  else if (cond_token == static_cast<parser::token_kind_type> ('('))
    {
      term = cond_expr ();
      if (cond_token != static_cast<parser::token_kind_type> (')'))
        {
          if (term)
            delete term;

          std::string etext (error_string_from_token (cond_token, &symbol));
          if (!etext.empty ())
            {
              parser_error (lineno, _ ("unexpected token `%s', expected `)'"),
                            etext.c_str ());
            }
          else
            parser_error (lineno, _ ("expected `)'"));
          COND_RETURN_ERROR ();
        }
      term = new COND_COM (line_number, COND_EXPR, nullptr, term, nullptr);
      (void)cond_skip_newlines ();
    }
  else if (cond_token == parser::token::BANG
           || (cond_token == parser::token::WORD
               && (symbol.value.as<WORD_DESC_PTR> ().value->word == "!")))
    {
      term = cond_term ();
      if (term)
        term->flags ^= CMD_INVERT_RETURN;
    }
  else if (cond_token == parser::token::WORD)
    {
      WORD_DESC *word = symbol.value.as<WORD_DESC_PTR> ().value;
      if (test_unop (word->word))
        {
          parser::symbol_type sym2 (read_token (READ));
          if (sym2.kind () == parser::symbol_kind::S_WORD)
            {
              WORD_DESC *word2 = sym2.value.as<WORD_DESC_PTR> ().value;
              tleft = new COND_COM (line_number, COND_TERM, word2, nullptr,
                                    nullptr);
              term = new COND_COM (line_number, COND_UNARY, word, tleft,
                                   nullptr);
            }
          else
            {
              delete word;
              std::string etext (
                  error_string_from_token (cond_token, &symbol));
              if (!etext.empty ())
                {
                  parser_error (line_number,
                                _ ("unexpected argument `%s' to conditional "
                                   "unary operator"),
                                etext.c_str ());
                }
              else
                parser_error (
                    line_number,
                    _ ("unexpected argument to conditional unary operator"));
              COND_RETURN_ERROR ();
            }

          (void)cond_skip_newlines ();
        }
      else /* left argument to binary operator */
        {
          /* lhs */
          tleft
              = new COND_COM (line_number, COND_TERM, word, nullptr, nullptr);

          /* binop */
          /* tok = cond_skip_newlines (); ? */
          parser::symbol_type sym2 (read_token (READ));
          if (sym2.kind () == parser::symbol_kind::S_WORD
              && test_binop (sym2.value.as<WORD_DESC_PTR> ().value->word))
            {
              op = sym2.value.as<WORD_DESC_PTR> ().value;
              if (op->word == "=" || op->word == "==")
                parser_state |= PST_EXTPAT;
              else if (op->word == "!=")
                parser_state |= PST_EXTPAT;
            }
#if defined(COND_REGEXP)
          else if (sym2.kind () == parser::symbol_kind::S_WORD
                   && sym2.value.as<WORD_DESC_PTR> ().value->word == "=~")
            {
              op = sym2.value.as<WORD_DESC_PTR> ().value;
              parser_state |= PST_REGEXP;
            }
#endif
          else if (current_token == '<' || current_token == '>')
            op = new WORD_DESC (static_cast<char> (current_token));
          /* There should be a check before blindly accepting the `)' that we
             have seen the opening `('. */
          else if (cond_token == parser::token::COND_END
                   || cond_token == parser::token::AND_AND
                   || cond_token == parser::token::OR_OR
                   || cond_token == static_cast<parser::token_kind_type> (')'))
            {
              /* Special case.  [[ x ]] is equivalent to [[ -n x ]], just like
                 the test command.  Similarly for [[ x && expr ]] or
                 [[ x || expr ]] or [[ (x) ]]. */
              op = make_word ("-n");
              term
                  = new COND_COM (line_number, COND_UNARY, op, tleft, nullptr);
              // XXX cond_token = tok2;
              return term;
            }
          else
            {
              std::string etext (
                  error_string_from_token (current_token, &sym2));
              if (!etext.empty ())
                {
                  parser_error (line_number,
                                _ ("unexpected token `%s', conditional binary "
                                   "operator expected"),
                                etext.c_str ());
                }
              else
                parser_error (line_number,
                              _ ("conditional binary operator expected"));
              delete tleft;
              COND_RETURN_ERROR ();
            }

          /* rhs */
          char local_extglob = extended_glob;
          if (parser_state & PST_EXTPAT)
            extended_glob = 1;

          parser::symbol_type sym3 (read_token (READ));
          if (parser_state & PST_EXTPAT)
            extended_glob = local_extglob;
          parser_state &= ~(PST_REGEXP | PST_EXTPAT);

          if (sym3.kind () == parser::symbol_kind::S_WORD)
            {
              WORD_DESC *op2 = sym3.value.as<WORD_DESC_PTR> ().value;
              tright = new COND_COM (line_number, COND_TERM, op2, nullptr,
                                     nullptr);
              term
                  = new COND_COM (line_number, COND_BINARY, op, tleft, tright);
            }
          else
            {
              std::string etext (
                  error_string_from_token (current_token, &sym3));
              if (!etext.empty ())
                {
                  parser_error (line_number,
                                _ ("unexpected argument `%s' to conditional "
                                   "binary operator"),
                                etext.c_str ());
                }
              else
                parser_error (
                    line_number,
                    _ ("unexpected argument to conditional binary operator"));
              delete tleft;
              delete op;
              COND_RETURN_ERROR ();
            }

          (void)cond_skip_newlines ();
        }
    }
  else
    {
      if (cond_token < 256)
        parser_error (line_number,
                      _ ("unexpected token `%c' in conditional command"),
                      static_cast<char> (cond_token));
      else
        {
          std::string etext (error_string_from_token (cond_token, &symbol));
          if (!etext.empty ())
            {
              parser_error (line_number,
                            _ ("unexpected token `%s' in conditional command"),
                            etext.c_str ());
            }
          else
            parser_error (line_number,
                          _ ("unexpected token %d in conditional command"),
                          cond_token);
        }
      COND_RETURN_ERROR ();
    }
  return term;
}
#endif

#if defined(ARRAY_VARS)
// This is now called with a modifiable buffer.
bool
Shell::token_is_assignment (std::string &t)
{
  t.push_back ('=');

  size_t r = assignment (t.c_str (), (parser_state & PST_COMPASSIGN) != 0);

  t.erase (t.size () - 1, 1);

  /* XXX - check that r == t.size () to avoid returning false positive for
     t containing `=' before t[t.size ()]. */
  return r > 0 && r == t.size ();
}
#endif

parser::symbol_type
Shell::read_token_word (int character)
{
  /* The value for YYLVAL when a WORD is read. */
  WORD_DESC *the_word;

  /* ALL_DIGITS becomes zero when we see a non-digit. */
  bool all_digit_token;

  /* DOLLAR_PRESENT becomes non-zero if we see a `$'. */
  bool dollar_present;

  /* COMPOUND_ASSIGNMENT becomes non-zero if we are parsing a compound
     assignment. */
  bool compound_assignment;

  /* QUOTED becomes non-zero if we see one of ("), ('), (`), or (\). */
  bool quoted;

  /* Non-zero means to ignore the value of the next character, and just
     to add it no matter what. */
  bool pass_next_character;

  /* The current delimiting character. */
  char cd;

  if (token_buffer.capacity () < TOKEN_DEFAULT_INITIAL_SIZE)
    token_buffer.reserve (TOKEN_DEFAULT_INITIAL_SIZE);

  all_digit_token = c_isdigit (character);
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

      cd = dstack.back ();

      /* Handle backslashes.  Quote lots of things when not inside of
         double-quotes, quote some things inside of double-quotes. */
      if MBTEST (character == '\\')
        {
          if (parser_state & PST_NOEXPAND)
            {
              pass_next_character = true;
              quoted = true;
              goto got_character;
            }

          int peek_char = shell_getc (false);

          /* Backslash-newline is ignored in all cases except
             when quoted with single quotes. */
          if MBTEST (peek_char == '\n')
            {
              character = '\n';
              goto next_character;
            }
          else
            {
              shell_ungetc (peek_char);

              /* If the next character is to be quoted, note it now. */
              if MBTEST (cd == 0 || cd == '`'
                         || (cd == '"' && peek_char >= 0
                             && (sh_syntaxtab[peek_char] & CBSDQUOTE)))
                pass_next_character = true;

              quoted = true;
              goto got_character;
            }
        }

      /* Parse a matched pair of quote characters. */
      if MBTEST (shellquote (character))
        {
          dstack.push_back (static_cast<char> (character));
          std::string ttok;
          try
            {
              ttok = parse_matched_pair (character, character, character,
                                         (character == '`') ? P_COMMAND
                                                            : P_NOFLAGS);
            }
          catch (const matched_pair_error &)
            {
              dstack.pop_back ();
              return parser::make_YYerror (); // Bail immediately.
            }
          dstack.pop_back ();

          token_buffer.push_back (static_cast<char> (character));
          token_buffer += ttok;

          all_digit_token = false;
          if (character != '`')
            quoted = true;

          dollar_present
              |= (character == '"' && ttok.find ('$') != std::string::npos);
          goto next_character;
        }

#ifdef COND_REGEXP
      /* When parsing a regexp as a single word inside a conditional command,
         we need to special-case characters special to both the shell and
         regular expressions.  Right now, that is only '(' and '|'. */
      if MBTEST ((parser_state & PST_REGEXP)
                 && (character == '(' || character == '|'))
        {
          if (character == '|')
            goto got_character;

          dstack.push_back (static_cast<char> (character));
          std::string ttok;
          try
            {
              ttok = parse_matched_pair (cd, '(', ')', P_NOFLAGS);
            }
          catch (const matched_pair_error &)
            {
              dstack.pop_back ();
              return parser::make_YYerror (); // Bail immediately.
            }
          dstack.pop_back ();

          token_buffer.push_back (static_cast<char> (character));
          token_buffer += ttok;
          dollar_present = all_digit_token = false;
          goto next_character;
        }
#endif /* COND_REGEXP */

#ifdef EXTENDED_GLOB
      /* Parse a ksh-style extended pattern matching specification. */
      if MBTEST (extended_glob && PATTERN_CHAR (character))
        {
          char peek_char = static_cast<char> (shell_getc (true));

          if MBTEST (peek_char == '(')
            {
              dstack.push_back (peek_char);
              std::string ttok;
              try
                {
                  ttok = parse_matched_pair (cd, '(', ')', P_NOFLAGS);
                }
              catch (const matched_pair_error &)
                {
                  dstack.pop_back ();
                  return parser::make_YYerror (); // Bail immediately.
                }
              dstack.pop_back ();

              token_buffer.push_back (static_cast<char> (character));
              token_buffer.push_back (peek_char);
              token_buffer += ttok;
              dollar_present = all_digit_token = false;
              goto next_character;
            }
          else
            shell_ungetc (peek_char);
        }
#endif /* EXTENDED_GLOB */

      /* If the delimiter character is not single quote, parse some of
         the shell expansions that must be read as a single word. */
      if MBTEST (shellexp (character))
        {
          char peek_char = static_cast<char> (shell_getc (true));
          // $(...), <(...), >(...), $((...)), ${...}, and $[...] constructs
          if MBTEST (peek_char == '('
                     || ((peek_char == '{' || peek_char == '[')
                         && character == '$')) /* ) ] } */
            {
              std::string ttok;
              try
                {
                  if (peek_char == '{')
                    ttok = parse_matched_pair (cd, '{', '}',
                                               P_FIRSTCLOSE | P_DOLBRACE);
                  else if (peek_char == '(')
                    {
                      /* XXX - push and pop the `(' as a delimiter for use by
                         the command-oriented-history code.  This way newlines
                         appearing in the $(...) string get added to the
                         history literally rather than causing a possibly-
                         incorrect `;' to be added. ) */
                      dstack.push_back (peek_char);
                      ttok = parse_comsub (cd, '(', ')');
                      dstack.pop_back ();
                    }
                  else
                    ttok = parse_matched_pair (cd, '[', ']', P_NOFLAGS);
                }
              catch (const matched_pair_error &)
                {
                  if (peek_char == '(')
                    dstack.pop_back ();

                  return parser::make_YYerror (); // Bail immediately.
                }

              token_buffer.push_back (static_cast<char> (character));
              token_buffer.push_back (peek_char);
              token_buffer += ttok;
              dollar_present = true;
              all_digit_token = false;
              goto next_character;
            }
#if defined(TRANSLATABLE_STRINGS)
          /* This handles $'...' and $"..." new-style quoted strings. */
          else if MBTEST (character == '$'
                          && (peek_char == '\'' || peek_char == '"'))
#else
          else if MBTEST (character == '$' && peek_char == '\'')
#endif
            {
              int first_line = line_number;
              dstack.push_back (peek_char);

              std::string ttok;
              try
                {
                  ttok = parse_matched_pair (peek_char, peek_char, peek_char,
                                             (peek_char == '\'') ? P_ALLOWESC
                                                                 : P_NOFLAGS);
                }
              catch (const matched_pair_error &)
                {
                  dstack.pop_back ();
                  return parser::make_YYerror (); // Bail immediately.
                }
              dstack.pop_back ();

              std::string ttrans;
              if (peek_char == '\'')
                {
                  /* PST_NOEXPAND */
                  ttrans = ansiexpand (ttok.begin (), ttok.end ());
                  ttok.clear ();

                  /* Insert the single quotes and correctly quote any
                     embedded single quotes (allowed because P_ALLOWESC was
                     passed to parse_matched_pair). */
                  ttrans = sh_single_quote (ttrans);
                }
#if defined(TRANSLATABLE_STRINGS)
              else
                {
                  /* PST_NOEXPAND */
                  /* Try to locale-expand the converted string. */
                  ttrans = locale_expand (ttok, first_line);
                  ttok.clear ();

                  /* Add the double quotes back */
                  ttrans = sh_mkdoublequoted (ttrans, 0);
                }
#endif

              token_buffer += ttrans;
              quoted = true;
              all_digit_token = false;
              goto next_character;
            }
          /* This could eventually be extended to recognize all of the
             shell's single-character parameter expansions, and set flags.*/
          else if MBTEST (character == '$' && peek_char == '$')
            {
              token_buffer.push_back ('$');
              token_buffer.push_back (peek_char);
              dollar_present = true;
              all_digit_token = false;
              goto next_character;
            }
          else
            shell_ungetc (peek_char);
        }

#if defined(ARRAY_VARS)
      /* Identify possible array subscript assignment; match [...].  If
         parser_state & PST_COMPASSIGN, we need to parse [sub]=words treating
         `sub' as if it were enclosed in double quotes. */
      else if MBTEST (character == '['
                      && ((!token_buffer.empty ()
                           && assignment_acceptable (last_read_token)
                           && token_is_ident (token_buffer))
                          || (token_buffer.empty ()
                              && (parser_state & PST_COMPASSIGN))))
        {
          std::string ttok;
          try
            {
              ttok = parse_matched_pair (cd, '[', ']', P_ARRAYSUB);
            }
          catch (const matched_pair_error &)
            {
              return parser::make_YYerror (); // Bail immediately.
            }

          token_buffer.push_back (static_cast<char> (character));
          token_buffer += ttok;
          all_digit_token = false;
          goto next_character;
        }
      /* Identify possible compound array variable assignment. */
      else if MBTEST (character == '=' && !token_buffer.empty ()
                      && (assignment_acceptable (last_read_token)
                          || (parser_state & PST_ASSIGNOK))
                      && token_is_assignment (token_buffer))
        {
          char peek_char = static_cast<char> (shell_getc (true));
          if MBTEST (peek_char == '(')
            {
              std::string ttok = parse_compound_assignment ();
              token_buffer.push_back ('=');
              token_buffer.push_back ('(');
              token_buffer += ttok;
              token_buffer.push_back (')');
              all_digit_token = false;
              compound_assignment = true;
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
      if MBTEST (character == CTLESC || character == CTLNUL)
        {
          token_buffer.push_back (CTLESC);
        }

    got_escaped_character:
      token_buffer.push_back (static_cast<char> (character));

      all_digit_token &= c_isdigit (character);
      dollar_present |= character == '$';

    next_character:
      if (character == '\n' && SHOULD_PROMPT ())
        prompt_again ();

      /* We want to remove quoted newlines (that is, a \<newline> pair)
         unless we are within single quotes or pass_next_character is
         set (the shell equivalent of literal-next). */
      cd = dstack.back ();
      character = shell_getc (cd != '\'' && !pass_next_character);
    } /* end for (;;) */

got_token:

  /* Check to see what thing we should return.  If the last_read_token
     is a `<', or a `&', or the character which ended this token is
     a '>' or '<', then, and ONLY then, is this input token a NUMBER.
     Otherwise, it is just a word, and should be returned as such. */
  if MBTEST (all_digit_token
             && (character == '<' || character == '>'
                 || last_read_token == parser::token::LESS_AND
                 || last_read_token == parser::token::GREATER_AND))
    {
      int64_t lvalue;
      if (legal_number (token_buffer.c_str (), &lvalue))
        return parser::make_NUMBER (lvalue);
    }

  /* Check for special case tokens. */
  if (last_shell_getc_is_singlebyte)
    {
      parser::symbol_type sctok (special_case_tokens (token_buffer));
      if (sctok.kind () != parser::symbol_kind::S_YYerror)
        return sctok;
    }

#if defined(ALIAS)
  /* Posix.2 does not allow reserved words to be aliased, so check for all
     of them, including special cases, before expanding the current token
     as an alias. */
  if MBTEST (posixly_correct)
    if (!dollar_present && !quoted
        && reserved_word_acceptable (last_read_token))
      {
        parser::symbol_type restok (check_for_reserved_word (token_buffer));
        if (restok.kind () != parser::symbol_kind::S_YYerror)
          return restok;
      }

  /* Aliases are expanded iff EXPAND_ALIASES is non-zero, and quoting
     inhibits alias expansion. */
  if (expand_aliases && !quoted)
    {
      alias_expand_token_result result = alias_expand_token (token_buffer);
      if (result == RE_READ_TOKEN)
        return RE_READ_TOKEN;
      else if (result == NO_EXPANSION)
        parser_state &= ~PST_ALEXPNEXT;
      // XXX - will there be any other return values?
    }

  /* If not in Posix.2 mode, check for reserved words after alias
     expansion. */
  if MBTEST (!posixly_correct)
#endif
    if (!dollar_present && !quoted
        && reserved_word_acceptable (last_read_token))
      {
        parser::symbol_type restok (check_for_reserved_word (token_buffer));
        if (restok.kind () != parser::symbol_kind::S_YYerror)
          return restok;
      }

  the_word = new WORD_DESC (token_buffer);
  if (dollar_present)
    the_word->flags |= W_HASDOLLAR;
  if (quoted)
    the_word->flags |= W_QUOTED;
  if (compound_assignment && token_buffer[token_buffer.size () - 1] == ')')
    the_word->flags |= W_COMPASSIGN;

  /* A word is an assignment if it appears at the beginning of a
     simple command, or after another assignment word.  This is
     context-dependent, so it cannot be handled in the grammar. */
  if (assignment (token_buffer, (parser_state & PST_COMPASSIGN) != 0))
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
      std::map<string_view, builtin>::iterator it
          = shell_builtins.find (token_buffer);

      if (it != shell_builtins.end ()
          && ((*it).second.flags & ASSIGNMENT_BUILTIN))
        parser_state |= PST_ASSIGNOK;
      else if (token_buffer == "eval" || token_buffer == "let")
        parser_state |= PST_ASSIGNOK;
    }

  /* should we check that quoted == 0 as well? */
  if MBTEST (token_buffer[0] == '{'
             && token_buffer[token_buffer.size () - 1] == '}'
             && (character == '<' || character == '>'))
    {
      /* can use token; already copied to the_word */
      std::string token_plus_1 (token_buffer.substr (1));

#if defined(ARRAY_VARS)
      if (legal_identifier (token_plus_1)
          || valid_array_reference (token_plus_1, VA_NOFLAGS))
#else
      if (legal_identifier (token_plus_1))
#endif
        {
          the_word->word = token_plus_1;
          // itrace ("read_token_word: returning REDIR_WORD for %s",
          //         the_word->word.c_str ());
          return parser::make_REDIR_WORD (WORD_DESC_PTR (the_word));
        }
    }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

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
    default:
      break;
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

  if ((the_word->flags & (W_ASSIGNMENT | W_NOSPLIT))
      == (W_ASSIGNMENT | W_NOSPLIT))
    return parser::make_ASSIGNMENT_WORD (WORD_DESC_PTR (the_word));
  else
    return parser::make_WORD (WORD_DESC_PTR (the_word));
}

#if defined(HISTORY)

/* If we are not within a delimited expression, try to be smart
   about which separators can be semi-colons and which must be
   newlines.  Returns the string that should be added into the
   history entry.  LINE is the line we're about to add; it helps
   make some more intelligent decisions in certain cases. */
const char *
Shell::history_delimiting_chars (string_view line)
{
  if ((parser_state & PST_HEREDOC) == 0)
    last_was_heredoc = false;

  if (!dstack.empty ())
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
          last_was_heredoc = false;
          return "\n";
        }
      return here_doc_first_line ? "\n" : "";
    }

  if (parser_state & PST_COMPASSIGN)
    return " ";

  /* First, handle some special cases. */
  /* If we just read `()', assume it's a function definition, and don't
     add a semicolon.  If the token before the `)' was not `(', and we're
     not in the midst of parsing a case statement, assume it's a
     parenthesized command and add the semicolon. */
  if (token_before_that == ')')
    {
      if (two_tokens_ago == '(') /* function def */
        return " ";
      /* This does not work for subshells inside case statement
         command lists.  It's a suboptimal solution. */
      else if (parser_state & PST_CASESTMT) /* case statement pattern */
        return " ";
      else
        return "; "; /* (...) subshell */
    }
  else if (token_before_that == parser::token::WORD
           && two_tokens_ago == parser::token::FUNCTION)
    return " "; /* function def using `function name' without `()' */

  /* If we're not in a here document, but we think we're about to parse one,
     and we would otherwise return a `;', return a newline to delimit the
     line with the here-doc delimiter */
  else if ((parser_state & PST_HEREDOC) == 0 && current_command_line_count > 1
           && last_read_token == parser::token_kind_type ('\n')
           && line.find ("<<") != line.npos)
    {
      last_was_heredoc = true;
      return "\n";
    }
  else if ((parser_state & PST_HEREDOC) == 0 && current_command_line_count > 1
           && need_here_doc > 0)
    return "\n";
  else if (token_before_that == parser::token::WORD
           && two_tokens_ago == parser::token::FOR)
    {
      size_t i;
      const char *input_line_cstr = shell_input_line.c_str ();
      /* Tricky.  `for i\nin ...' should not have a semicolon, but
         `for i\ndo ...' should.  We do what we can. */
      for (i = shell_input_line_index; whitespace (input_line_cstr[i]); i++)
        ;
      if (input_line_cstr[i] == 'i' && input_line_cstr[i + 1] == 'n')
        return " ";
      return ";";
    }
  else if (two_tokens_ago == parser::token::CASE
           && token_before_that == parser::token::WORD
           && (parser_state & PST_CASESTMT))
    return " ";

  if (no_semi_successors.find (token_before_that) != no_semi_successors.end ())
    return " ";

  /* Assume that by this point we are reading lines in a multi-line command.
     If we have multiple consecutive blank lines we want to return only one
     semicolon. */
  if (line_isblank (line))
    return (current_command_line_count > 1 && last_read_token == '\n'
            && token_before_that != '\n')
               ? "; "
               : "";

  return "; ";
}
#endif /* HISTORY */

/* Issue a prompt, or prepare to issue a prompt when the next character
   is read. */
void
Shell::prompt_again ()
{
  if (!interactive || expanding_alias ()) /* XXX */
    return;

  ps1_prompt = get_string_value ("PS1");
  ps2_prompt = get_string_value ("PS2");

  ps0_prompt = get_string_value ("PS0");

  if (!prompt_string_pointer)
    prompt_string_pointer = &ps1_prompt;

  std::string temp_prompt;

  if (!prompt_string_pointer->empty ())
    temp_prompt = decode_prompt_string (*prompt_string_pointer);

  current_prompt_string = *prompt_string_pointer;
  prompt_string_pointer = &ps2_prompt;

#if defined(READLINE)
  if (!no_line_editing)
    {
      current_readline_prompt = temp_prompt;
    }
  else
#endif /* READLINE */
    {
      current_decoded_prompt = temp_prompt;
    }
}

#if defined(HISTORY)
/* The history library increments the history offset as soon as it stores
   the first line of a potentially multi-line command, so we compensate
   here by returning one fewer when appropriate. */
size_t
Shell::prompt_history_number (string_view pmt)
{
  size_t ret = history_number ();
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
#define PROMPT_GROWTH 48
std::string
Shell::decode_prompt_string (string_view string)
{
#if defined(PROMPT_STRING_DECODE)
  size_t size;
  int n;
  struct tm *now;
  time_t the_time;
  char octal_string[4];
  char timebuf[128];
  const char *temp;

  std::string result;
  result.reserve (PROMPT_GROWTH);

  string_view::const_iterator it;
  for (it = string.begin (); it != string.end (); ++it)
    {
      if (posixly_correct && *it == '!')
        {
          if (*(it + 1) == '!')
            {
              result.push_back ('!');
              ++it;
              continue;
            }
          else
            {
#if !defined(HISTORY)
              result.push_back ("1");
#else  /* HISTORY */
              result += itos (
                  static_cast<int64_t> (prompt_history_number (string)));
#endif /* HISTORY */
              continue;
            }
        }
      if (*it == '\\')
        {
          char c = *(++it);

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
              strncpy (octal_string, &(*it), 3);
              octal_string[3] = '\0';

              n = read_octal (octal_string);

              if (n == CTLESC || n == CTLNUL)
                {
                  result.push_back (CTLESC);
                  result.push_back (static_cast<char> (n));
                }
              else if (n == -1)
                result.push_back ('\\');
              else
                result.push_back (static_cast<char> (n));

              if (n != -1)
                {
                  for (int i = 0;
                       i < 3 && it != string.end () && isoctal (*it); i++)
                    ++it;

                  --it; // back up one
                }
              break;

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
              now = localtime (&the_time);
              size = 0;
              if (c == 'd')
                size = strftime (timebuf, sizeof (timebuf), "%a %b %d", now);
              else if (c == 't')
                size = strftime (timebuf, sizeof (timebuf), "%H:%M:%S", now);
              else if (c == 'T')
                size = strftime (timebuf, sizeof (timebuf), "%I:%M:%S", now);
              else if (c == '@')
                size = strftime (timebuf, sizeof (timebuf), "%I:%M %p", now);
              else if (c == 'A')
                size = strftime (timebuf, sizeof (timebuf), "%H:%M", now);

              if (size != 0)
                {
                  timebuf[sizeof (timebuf) - 1] = '\0';
                  result += timebuf;
                }
              break;

            case 'D': /* strftime format */
              if (*(it + 1) != '{')
                goto not_escape;

              it += 2; /* skip { */
              {
                (void)time (&the_time);
                now = localtime (&the_time);

                // make a time format substring to pass to strftime ()
                size_t start_pos = static_cast<size_t> (it - string.begin ());
                size_t close_bracket = string.find ('}', start_pos);
                it = (string.begin ()
                      + static_cast<ptrdiff_t> (close_bracket));

                std::string timefmt (to_string (
                    string.substr (start_pos, (close_bracket - start_pos))));
                if (timefmt.empty ())
                  timefmt = "%X"; // locale-specific current time

                size = strftime (timebuf, sizeof (timebuf), timefmt.c_str (),
                                 now);

                if (size != 0)
                  {
                    timebuf[sizeof (timebuf) - 1] = '\0';
                    if (promptvars || posixly_correct)
                      /* Make sure that expand_prompt_string is called with a
                         second argument of Q_DOUBLE_QUOTES if we use this
                         function here. */
                      result += sh_backslash_quote_for_double_quotes (
                          std::string (timebuf));
                    else
                      result += timebuf;
                  }
              }
              break;

            case 'n':
              if (no_line_editing)
                result.push_back ('\n');
              else
                result += "\r\n";
              break;

            case 's':
              {
                std::string stemp (sh_strvis (base_pathname (shell_name)));

                // Try to quote anything the user can set in the file system
                if (promptvars || posixly_correct)
                  result += sh_backslash_quote_for_double_quotes (stemp);
                else
                  result += stemp;
              }
              break;

            case 'v':
              result += dist_version;
              break;

            case 'V':
              {
                result += dist_version;
                result.push_back ('.');
                result += itos (patch_level);
              }
              break;

            case 'w':
            case 'W':
              {
                /* Use the value of PWD because it is much more efficient. */
                char t_string[PATH_MAX];
                size_t tlen;

                temp = get_string_value ("PWD");

                if (temp == nullptr)
                  {
                    if (getcwd (t_string, sizeof (t_string)) == nullptr)
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

                const char *t;
                std::string dir_string;

#define ROOT_PATH(x) ((x)[0] == '/' && (x)[1] == 0)
#define DOUBLE_SLASH_ROOT(x) ((x)[0] == '/' && (x)[1] == '/' && (x)[2] == 0)
                /* Abbreviate \W as ~ if $PWD == $HOME */
                if (c == 'W'
                    && (((t = get_string_value ("HOME")) == nullptr)
                        || !STREQ (t, t_string)))
                  {
                    if (!ROOT_PATH (t_string) && !DOUBLE_SLASH_ROOT (t_string))
                      {
                        t = strrchr (t_string, '/');
                        if (t)
                          memmove (t_string, t + 1, strlen (t)); // copy NULL
                      }
                    dir_string = t_string;
                  }
#undef ROOT_PATH
#undef DOUBLE_SLASH_ROOT
                else
                  {
                    dir_string = polite_directory_format (t_string);
                  }

                /* If we're going to be expanding the prompt string later,
                   quote the directory name. */
                if (promptvars || posixly_correct)
                  /* Make sure that expand_prompt_string is called with a
                     second argument of Q_DOUBLE_QUOTES if we use this
                     function here. */
                  result += sh_backslash_quote_for_double_quotes (
                      sh_strvis (dir_string));
                else
                  result += sh_strvis (dir_string);

                break;
              }

            case 'u':
              if (current_user.user_name == nullptr)
                get_current_user_info ();
              result += current_user.user_name;
              break;

            case 'h':
            case 'H':
              {
                std::string t_host (current_host_name);
                if (c == 'h' && ((size = t_host.find ('.')) != t_host.npos))
                  t_host.resize (size);
                if (promptvars || posixly_correct)
                  /* Make sure that expand_prompt_string is called with a
                     second argument of Q_DOUBLE_QUOTES if we use this
                     function here. */
                  result += sh_backslash_quote_for_double_quotes (t_host);
                else
                  result += t_host;
              }
              break;

            case '#':
              n = current_command_number;
              /* If we have already incremented current_command_number (PS4,
                 ${var@P}), compensate */
              if (string != ps0_prompt && string != ps1_prompt
                  && string != ps2_prompt)
                n--;
              result += itos (n);
              break;

            case '!':
#if !defined(HISTORY)
              result.push_back ('1');
#else  /* HISTORY */
              result += itos (
                  static_cast<int64_t> (prompt_history_number (string)));
#endif /* HISTORY */
              break;

            case '$':
              if ((promptvars || posixly_correct) && (current_user.euid != 0))
                result.push_back ('\\');
              result.push_back (current_user.euid == 0 ? '#' : '$');
              break;

            case 'j':
              result += itos (count_all_jobs ());
              break;

            case 'l':
#if defined(HAVE_TTYNAME)
              temp = ttyname (fileno (stdin));
              if (temp)
                result += base_pathname (temp);
              else
                result += "tty";
#else
              result += "tty";
#endif /* !HAVE_TTYNAME */
              break;

#if defined(READLINE)
            case '[':
            case ']':
              if (no_line_editing)
                break;

              c = (c == '[') ? RL_PROMPT_START_IGNORE : RL_PROMPT_END_IGNORE;
              if (c == CTLESC || c == CTLNUL)
                result.push_back (CTLESC);

              result.push_back (c);
              break;
#endif /* READLINE */

            case '\\':
              result.push_back (*it);
              break;

            case 'a':
              result.push_back ('\07');
              break;

            case 'e':
              result.push_back ('\033');
              break;

            case 'r':
              result.push_back ('\r');
              break;

            default:
            not_escape:
              result.push_back ('\\');
              result.push_back (*it);
            }
        }
      else
        {
          /* dequote_string should take care of removing this if we are not
             performing the rest of the word expansions. */
          if (*it == CTLESC || *it == CTLNUL)
            result.push_back (CTLESC);

          result.push_back (*it);
        }
    }
#else  /* !PROMPT_STRING_DECODE */
  std::string result (string);
#endif /* !PROMPT_STRING_DECODE */

#if __cplusplus >= 201103L
  // Save the delimiter stack and clear `dstack' so any command substitutions
  // in the prompt string won't screw up the parser state.
  temp_dstack = std::move (dstack);
#else
  temp_dstack = dstack;
#endif
  dstack.clear ();

  /* Perform variable and parameter expansion and command substitution on
     the prompt string. */
  if (promptvars || posixly_correct)
    {
      int last_exit_value = last_command_exit_value;
      pid_t last_comsub_pid = last_command_subst_pid;
      WORD_LIST *list = expand_prompt_string (result, Q_DOUBLE_QUOTES, 0);
      result = string_list (list);
      delete list;
      last_command_exit_value = last_exit_value;
      last_command_subst_pid = last_comsub_pid;
    }
  else
    result = dequote_string (result);

#if __cplusplus >= 201103L
  // Save the delimiter stack and clear `dstack' so any command substitutions
  // in the prompt string won't screw up the parser state.
  dstack = std::move (temp_dstack);
#else
  dstack = temp_dstack;
#endif
  temp_dstack.clear ();

  return result;
}

/************************************************
 *						*
 *		ERROR HANDLING			*
 *						*
 ************************************************/

// Report a syntax error and restart the parser (called by Bison).
void
BashParser::error (const std::string &msg)
{
  the_shell->yyerror (msg);
}

// Report a syntax error and restart the parser. Call here for fatal errors.
void
Shell::yyerror (const std::string &msg)
{
  if ((the_shell->parser_state & PST_NOERROR) == 0)
    the_shell->report_syntax_error (msg.c_str ());
  the_shell->reset_parser ();
}

std::string
Shell::error_string_from_token (parser::token_kind_type token,
                                const parser::symbol_type *symbol)
{
  std::string t (find_token_in_map (token, word_token_map));
  if (!t.empty ())
    return t;

  t = find_token_in_map (token, other_token_map);
  if (!t.empty ())
    return t;

  if (symbol == nullptr)
    return t;

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

  // This should now be safer than the C version was.
  switch (token)
    {
    case parser::token::WORD:
    case parser::token::ASSIGNMENT_WORD:
      {
        WORD_DESC *word = symbol->value.as<WORD_DESC_PTR> ().value;
        return word->word;
      }
    case parser::token::NUMBER:
      {
        int64_t number = symbol->value.as<int64_t> ();
        return itos (number);
      }
    case parser::token::ARITH_CMD:
      {
        WORD_LIST *list = symbol->value.as<WORD_LIST_PTR> ().value;
        return string_list (list);
      }
    case parser::token::ARITH_FOR_EXPRS:
      {
        WORD_LIST *list = symbol->value.as<WORD_LIST_PTR> ().value;
        return string_list_internal (list, " ; ");
      }
    case parser::token::COND_CMD:
    default:
      /* punt */
      break;
    }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

  return t;
}

std::string
Shell::error_token_from_text ()
{
  const std::string &t = shell_input_line;
  size_t i = shell_input_line_index;
  size_t token_end = 0;

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

  if (token_end)
    return t.substr (i, (token_end - i));
  else if (i == 0 && token_end == 0) // one-character token
    return t.substr (i, 1);

  return std::string ();
}

void
Shell::print_offending_line ()
{
  size_t token_end = shell_input_line.size ();
  while (token_end && shell_input_line[token_end - 1] == '\n')
    --token_end;

  std::string msg (shell_input_line.substr (0, token_end));
  parser_error (line_number, "`%s'", msg.c_str ());
}

/* Report a syntax error with line numbers, etc.
   Call here for recoverable errors, with a message to print. */
void
Shell::report_syntax_error (const char *message)
{
  if (message && message[0])
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

  std::string msg;

  /* If the line of input we're reading is not null, try to find the
     objectionable token. First, try to figure out what token the
     parser's complaining about by looking at current_token. */
  if (current_token != parser::token::YYEOF && !EOF_Reached
      && !((msg = error_string_from_token (current_token, nullptr)).empty ()))
    {
      if (ansic_shouldquote (msg))
        msg = ansic_quote (msg);

      parser_error (line_number, _ ("syntax error near unexpected token `%s'"),
                    msg.c_str ());

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
      if (!msg.empty ())
        {
          parser_error (line_number, _ ("syntax error near `%s'"),
                        msg.c_str ());
        }

      /* If not interactive, print the line containing the error. */
      if (!interactive)
        print_offending_line ();
    }
  else
    {
      if (EOF_Reached && shell_eof_token && current_token != shell_eof_token)
        parser_error (line_number,
                      _ ("unexpected EOF while looking for matching `%c'"),
                      shell_eof_token);
      else
        {
          const char *err_msg
              = EOF_Reached ? _ ("syntax error: unexpected end of file")
                            : _ ("syntax error");
          parser_error (line_number, "%s", err_msg);
        }

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
              last_read_token = current_token
                  = static_cast<parser::token_kind_type> ('\n');
              /* Reset the prompt string to be $PS1. */
              prompt_string_pointer = nullptr;
              prompt_again ();
              return;
            }
        }

      /* In this case EOF should exit the shell.  Do it now. */
      reset_parser ();

      last_shell_builtin = this_shell_builtin;
      this_shell_builtin = &Shell::exit_builtin;
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

/* Take a string and run it through the shell parser, returning the
   resultant word list. Used by compound array assignment. */
WORD_LIST *
Shell::parse_string_to_word_list (string_view s, int flags, string_view whom)
{
  sh_parser_state_t ps;
  pstate_flags orig_parser_state = PST_NOFLAGS;
  bool ea;

  int orig_line_number = line_number;
  save_parser_state (&ps);

#if defined(HISTORY)
  bash_history_disable ();
#endif

  push_stream (1);

  if ((ea = expanding_alias ()))
    parser_save_alias ();

  /* WORD to avoid parsing reserved words as themselves and just parse them
     as WORDs. */
  last_read_token = parser::token::WORD;

  current_command_line_count = 0;
  echo_input_at_read = expand_aliases = 0;

  with_input_from_string (savestring (s), whom);

  if (flags & 1)
    {
      orig_parser_state = parser_state; /* XXX - not needed? */
      /* State flags we don't want to persist into compound assignments. */
      parser_state &= ~PST_NOEXPAND; /* parse_comsub sentinel */
      /* State flags we want to set for this run through the tokenizer. */
      parser_state |= PST_COMPASSIGN | PST_REPARSE;
    }

  WORD_LIST *wl = nullptr;
  bool parse_string_error = false;
  for (;;)
    {
      parser::symbol_type symbol (read_token (READ));
      parser::token_type tok = current_token;
      if (symbol.kind () == parser::symbol_kind::S_yacc_EOF
          || (current_token == static_cast<parser::token_kind_type> ('\n')
              && *bash_input.location.string == '\0'))
        break;

      // Allow newlines in compound assignments.
      if (current_token == static_cast<parser::token_kind_type> ('\n'))
        continue;

      if (tok != parser::token::WORD && tok != parser::token::ASSIGNMENT_WORD)
        {
          line_number = orig_line_number + line_number - 1;
          yyerror (std::string ()); /* does the right thing */
          delete wl;
          parse_string_error = true;
          break;
        }

      wl = new WORD_LIST (symbol.value.as<WORD_DESC_PTR> ().value, wl);
    }

  last_read_token = static_cast<parser::token_kind_type> ('\n');
  pop_stream ();

  if (ea)
    parser_restore_alias ();

  restore_parser_state (&ps);

  if (flags & 1)
    parser_state = orig_parser_state; /* XXX - not needed? */

  if (parse_string_error)
    {
      set_exit_status (EXECUTION_FAILURE);
      if (!interactive_shell && posixly_correct)
        throw bash_exception (FORCE_EOF);
      else
        throw bash_exception (DISCARD);
    }

  return wl->reverse ();
}

std::string
Shell::parse_compound_assignment ()
{
  sh_parser_state_t ps;

  int orig_line_number = line_number;
  save_parser_state (&ps);

  /* WORD to avoid parsing reserved words as themselves and just parse them
     as WORDs. Plus it means we won't be in a command position and so alias
     expansion won't happen. */
  last_read_token = parser::token::WORD;

  token_buffer.clear ();
#if __cplusplus < 201103L
  token_buffer.reserve ();
#else
  token_buffer.shrink_to_fit ();
#endif

  WORD_LIST *wl = nullptr;

  bool assignok = parser_state & PST_ASSIGNOK; /* XXX */

  /* State flags we don't want to persist into compound assignments. */
  parser_state &= ~(PST_NOEXPAND | PST_CONDCMD | PST_CONDEXPR | PST_REGEXP
                    | PST_EXTPAT);

  /* State flags we want to set for this run through the tokenizer. */
  parser_state |= PST_COMPASSIGN;

  esacs_needed_count = expecting_in_token = 0;

  bool parse_string_error = false;
  for (;;)
    {
      parser::symbol_type symbol (read_token (READ));
      parser::token_type tok = current_token;

      if (tok == static_cast<parser::token_kind_type> (')'))
        break;

      if (tok == '\n') /* Allow newlines in compound assignments */
        {
          if (SHOULD_PROMPT ())
            prompt_again ();
          continue;
        }

      if (tok != parser::token::WORD && tok != parser::token::ASSIGNMENT_WORD)
        {
          if (tok == parser::token::yacc_EOF)
            parser_error (orig_line_number,
                          _ ("unexpected EOF while looking for matching `)'"));
          else
            yyerror (std::string ()); /* does the right thing */

          delete wl;
          parse_string_error = true;
          break;
        }

      wl = new WORD_LIST (symbol.value.as<WORD_DESC_PTR> ().value, wl);
    }

  restore_parser_state (&ps);

  if (parse_string_error)
    {
      set_exit_status (EXECUTION_FAILURE);
      last_read_token = static_cast<parser::token_kind_type> ('\n'); // XXX
      if (!interactive_shell && posixly_correct)
        throw bash_exception (FORCE_EOF);
      else
        throw bash_exception (DISCARD);
    }

  std::string ret;

  if (wl)
    {
      wl = wl->reverse ();
      ret = string_list (wl);
      delete wl;
    }

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
  ps->eol_lookahead = eol_ungetc_lookahead;

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

  ps->esacs_needed = esacs_needed_count;
  ps->expecting_in = expecting_in_token;

  if (need_here_doc == 0)
    ps->redir_stack[0] = nullptr;
  else
    memcpy (ps->redir_stack, redir_stack,
            sizeof (redir_stack[0]) * HEREDOC_MAX);

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  ps->pushed_strings = pushed_string_list;
#endif

  ps->eof_token = shell_eof_token;
  ps->token_buffer = token_buffer;

  /* Force reallocation on next call to read_token_word */
  token_buffer.clear ();
#if __cplusplus < 201103L
  token_buffer.reserve ();
#else
  token_buffer.shrink_to_fit ();
#endif

  return ps;
}

void
Shell::restore_parser_state (sh_parser_state_t *ps)
{
  if (ps == nullptr)
    return;

  parser_state = ps->parser_state;
  if (ps->token_state)
    {
      restore_token_state (ps->token_state);
      delete[] ps->token_state;
    }

  shell_input_line_terminator = ps->input_line_terminator;
  eof_encountered = ps->eof_encountered;
  eol_ungetc_lookahead = ps->eol_lookahead;

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

  esacs_needed_count = ps->esacs_needed;
  expecting_in_token = ps->expecting_in;

  if (need_here_doc == 0)
    redir_stack[0] = nullptr;
  else
    memcpy (redir_stack, ps->redir_stack,
            sizeof (redir_stack[0]) * HEREDOC_MAX);

#if defined(ALIAS) || defined(DPAREN_ARITHMETIC)
  pushed_string_list = ps->pushed_strings;
#endif

#if __cplusplus < 201103L
  token_buffer = ps->token_buffer;
  token_buffer.reserve (token_buffer.size ());
  ps->token_buffer.clear ();
  ps->token_buffer.reserve ();
#else
  token_buffer = std::move (ps->token_buffer);
  token_buffer.shrink_to_fit ();
  ps->token_buffer.clear ();
  ps->token_buffer.shrink_to_fit ();
#endif

  shell_eof_token = ps->eof_token;
}

sh_input_line_state_t *
Shell::save_input_line_state (sh_input_line_state_t *ls)
{
  if (ls == nullptr)
    ls = new sh_input_line_state_t ();

  ls->input_line = shell_input_line;
  ls->input_line_index = shell_input_line_index;

#if defined(HANDLE_MULTIBYTE)
  ls->input_property = shell_input_line_property;
#endif

  /* force reallocation */
  shell_input_line.clear ();
#if __cplusplus < 201103L
  shell_input_line.reserve ();
#else
  shell_input_line.shrink_to_fit ();
#endif
  shell_input_line_index = 0;

#if defined(HANDLE_MULTIBYTE)
  shell_input_line_property.clear ();
#if __cplusplus < 201103L
  shell_input_line_property.reserve (0);
#else
  shell_input_line_property.shrink_to_fit ();
#endif
#endif

  return ls;
}

void
Shell::restore_input_line_state (sh_input_line_state_t *ls)
{
  shell_input_line = ls->input_line;
  shell_input_line_index = ls->input_line_index;

#if defined(HANDLE_MULTIBYTE)
  shell_input_line_property = ls->input_property;
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

void
Shell::set_line_mbstate ()
{
  if (shell_input_line.empty ())
    return;

  size_t len = shell_input_line.size ();

  if (locale_mb_cur_max == 1)
    {
      shell_input_line_property.assign (len, true);
      return;
    }

  mbstate_t mbs, prevs;
  size_t mbclen;

  shell_input_line_property.assign (len, false);

  /* XXX - use whether or not we are in a UTF-8 locale to avoid calls to
     mbrlen */
  if (!locale_utf8locale)
    memset (&prevs, '\0', sizeof (mbstate_t));

  size_t i, previ;
  for (i = previ = 0; i < len; i++)
    {
      if (!locale_utf8locale)
        mbs = prevs;

      int c = shell_input_line[i];
      if (c == EOF)
        {
          size_t j;
          for (j = i; j < len; j++)
            shell_input_line_property[j] = true;
          break;
        }

      if (locale_utf8locale)
        {
          /* i != previ */
          if (static_cast<unsigned char> (shell_input_line[previ]) < 128)
            mbclen = 1;
          else
            {
              mbclen = utf8_mblen (&shell_input_line[previ], i - previ + 1);
            }
        }
      else
        mbclen = mbrlen (&shell_input_line[previ], i - previ + 1, &mbs);

      if (mbclen == 1 || mbclen == static_cast<size_t> (-1))
        {
          mbclen = 1;
          previ = i + 1;
        }
      else if (mbclen == static_cast<size_t> (-2))
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
            shell_input_line_property[j] = true;
          break;
        }

      shell_input_line_property[i] = mbclen;
    }
}
#endif /* HANDLE_MULTIBYTE */

} // namespace bash
