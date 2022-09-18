/* make_cmd.c -- Functions for making instances of the various
   parser constructs. */

/* Copyright (C) 1989-2020 Free Software Foundation, Inc.

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

#include "bashtypes.hh"

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include "filecntl.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "flags.hh"
#include "input.hh"
#include "parser.hh"
#include "shell.hh"

#if defined(JOB_CONTROL)
#include "jobs.hh"
#endif

#include "shmbutil.hh"

namespace bash
{

// TODO: try to move all of the constructors inline

/* Note that this function calls dispose_words on EXPRS, since it doesn't
   use the word list directly.  We free it here rather than at the caller
   because no other function in this file requires that the caller free
   any arguments. */
ARITH_FOR_COM *
Shell::make_arith_for_command (WORD_LIST *exprs, COMMAND *action, int lineno)
{
#if defined(ARITH_FOR_COMMAND)
  arith_for_com *temp;
  WORD_LIST *init, *test, *step;
  char *s, *t, *start;
  int nsemi, i;

  init = test = step = (WORD_LIST *)NULL;
  /* Parse the string into the three component sub-expressions. */
  start = t = s = exprs->word->word;
  for (nsemi = 0;;)
    {
      /* skip whitespace at the start of each sub-expression. */
      while (whitespace (*s))
        s++;
      start = s;
      /* skip to the semicolon or EOS */
      i = skip_to_delim (start, 0, ";", SD_NOJMP | SD_NOPROCSUB);
      s = start + i;

      t = (i > 0) ? substring (start, 0, i) : (char *)NULL;

      nsemi++;
      switch (nsemi)
        {
        case 1:
          init = make_arith_for_expr (t);
          break;
        case 2:
          test = make_arith_for_expr (t);
          break;
        case 3:
          step = make_arith_for_expr (t);
          break;
        }

      FREE (t);
      if (*s == '\0')
        break;
      s++; /* skip over semicolon */
    }

  if (nsemi != 3)
    {
      if (nsemi < 3)
        parser_error (lineno,
                      _ ("syntax error: arithmetic expression required"));
      else
        parser_error (lineno, _ ("syntax error: `;' unexpected"));
      parser_error (lineno, _ ("syntax error: `((%s))'"), exprs->word->word);
      free (init);
      free (test);
      free (step);
      set_exit_status (2);
      return (COMMAND *)NULL;
    }

  temp = (arith_for_com *)xmalloc (sizeof (arith_for_com));
  temp->flags = 0;
  temp->line = lineno;
  temp->init = init ? init : make_arith_for_expr ("1");
  temp->test = test ? test : make_arith_for_expr ("1");
  temp->step = step ? step : make_arith_for_expr ("1");
  temp->action = action;

  dispose_words (exprs);
  return make_command (cm_arith_for, (simple_com *)temp);
#else
  dispose_words (exprs);
  set_exit_status (2);
  return (COMMAND *)NULL;
#endif /* ARITH_FOR_COMMAND */
}

PATTERN_LIST *
make_pattern_list (WORD_LIST *patterns, COMMAND *action)
{
  PATTERN_LIST *temp;

  temp = (PATTERN_LIST *)xmalloc (sizeof (PATTERN_LIST));
  temp->patterns = REVERSE_LIST (patterns, WORD_LIST *);
  temp->action = action;
  temp->next = NULL;
  temp->flags = 0;
  return temp;
}

COMMAND *
make_if_command (COMMAND *test, COMMAND *true_case, COMMAND *false_case)
{
  if_com *temp;

  temp = (if_com *)xmalloc (sizeof (if_com));
  temp->flags = 0;
  temp->test = test;
  temp->true_case = true_case;
  temp->false_case = false_case;
  return make_command (cm_if, (simple_com *)temp);
}

static COMMAND *
make_until_or_while (enum command_type which, COMMAND *test, COMMAND *action)
{
  while_com *temp;

  temp = (while_com *)xmalloc (sizeof (while_com));
  temp->flags = 0;
  temp->test = test;
  temp->action = action;
  return make_command (which, (simple_com *)temp);
}

COMMAND *
make_while_command (COMMAND *test, COMMAND *action)
{
  return make_until_or_while (cm_while, test, action);
}

COMMAND *
make_until_command (COMMAND *test, COMMAND *action)
{
  return make_until_or_while (cm_until, test, action);
}

COMMAND *
make_arith_command (WORD_LIST *exp)
{
#if defined(DPAREN_ARITHMETIC)
  COMMAND *command;
  arith_com *temp;

  command = (COMMAND *)xmalloc (sizeof (COMMAND));
  command->value.Arith = temp = (arith_com *)xmalloc (sizeof (arith_com));

  temp->flags = 0;
  temp->line = line_number;
  temp->exp = exp;

  command->type = cm_arith;
  command->redirects = (REDIRECT *)NULL;
  command->flags = 0;

  return command;
#else
  set_exit_status (2);
  return (COMMAND *)NULL;
#endif
}

#if defined(COND_COMMAND)
struct cond_com *
make_cond_node (int type, WORD_DESC *op, struct cond_com *left,
                struct cond_com *right)
{
  cond_com *temp;

  temp = (cond_com *)xmalloc (sizeof (cond_com));
  temp->flags = 0;
  temp->line = line_number;
  temp->type = type;
  temp->op = op;
  temp->left = left;
  temp->right = right;

  return temp;
}
#endif

COMMAND *
make_cond_command (cond_com *cond_node)
{
#if defined(COND_COMMAND)
  COMMAND *command;

  command = (COMMAND *)xmalloc (sizeof (COMMAND));
  command->value.Cond = cond_node;

  command->type = cm_cond;
  command->redirects = (REDIRECT *)NULL;
  command->flags = 0;
  command->line = cond_node ? cond_node->line : 0;

  return command;
#else
  set_exit_status (2);
  return (COMMAND *)NULL;
#endif
}

COMMAND *
make_bare_simple_command ()
{
  COMMAND *command;
  simple_com *temp;

  command = (COMMAND *)xmalloc (sizeof (COMMAND));
  command->value.Simple = temp = (simple_com *)xmalloc (sizeof (simple_com));

  temp->flags = 0;
  temp->line = line_number;
  temp->words = (WORD_LIST *)NULL;
  temp->redirects = (REDIRECT *)NULL;

  command->type = cm_simple;
  command->redirects = (REDIRECT *)NULL;
  command->flags = 0;

  return command;
}

/* Return a command which is the connection of the word or redirection
   in ELEMENT, and the command * or NULL in COMMAND. */
SIMPLE_COM *
Shell::make_simple_command (ELEMENT element, COMMAND *command)
{
  /* If we are starting from scratch, then make the initial command
     structure.  Also note that we have to fill in all the slots, since
     malloc doesn't return zeroed space. */
  if (command == 0)
    {
      command = make_bare_simple_command ();
      parser_state |= PST_REDIRLIST;
    }

  if (element.word)
    {
      command->value.Simple->words
          = make_word_list (element.word, command->value.Simple->words);
      parser_state &= ~PST_REDIRLIST;
    }
  else if (element.redirect)
    {
      REDIRECT *r = element.redirect;
      /* Due to the way <> is implemented, there may be more than a single
         redirection in element.redirect.  We just follow the chain as far
         as it goes, and hook onto the end. */
      while (r->next)
        r = r->next;
      r->next = redirects;
      redirects = element.redirect;
    }
}

/* Because we are Bourne compatible, we read the input for this
   << or <<- redirection now, from wherever input is coming from.
   We store the input read into a WORD_DESC.  Replace the text of
   the redirectee.word with the new input text.  If <<- is on,
   then remove leading TABS from each line. */
void
Shell::make_here_document (REDIRECT *temp, int lineno)
{
  int kill_leading, redir_len;
  char *redir_word, *document, *full_line;
  int document_index, document_size, delim_unquoted;

  if (temp->instruction != r_deblank_reading_until
      && temp->instruction != r_reading_until)
    {
      internal_error (_ ("make_here_document: bad instruction type %d"),
                      temp->instruction);
      return;
    }

  kill_leading = temp->instruction == r_deblank_reading_until;

  document = (char *)NULL;
  document_index = document_size = 0;

  /* Quote removal is the only expansion performed on the delimiter
     for here documents, making it an extremely special case. */
  redir_word = string_quote_removal (temp->redirectee.filename->word, 0);

  /* redirection_expand will return NULL if the expansion results in
     multiple words or no words.  Check for that here, and just abort
     this here document if it does. */
  if (redir_word)
    redir_len = strlen (redir_word);
  else
    {
      temp->here_doc_eof = (char *)xmalloc (1);
      temp->here_doc_eof[0] = '\0';
      goto document_done;
    }

  free (temp->redirectee.filename->word);
  temp->here_doc_eof = redir_word;

  /* Read lines from wherever lines are coming from.
     For each line read, if kill_leading, then kill the
     leading tab characters.
     If the line matches redir_word exactly, then we have
     manufactured the document.  Otherwise, add the line to the
     list of lines in the document. */

  /* If the here-document delimiter was quoted, the lines should
     be read verbatim from the input.  If it was not quoted, we
     need to perform backslash-quoted newline removal. */
  delim_unquoted = (temp->redirectee.filename->flags & W_QUOTED) == 0;
  while ((full_line = read_secondary_line (delim_unquoted)))
    {
      char *line;
      int len;

      here_doc_first_line = 0;
      line = full_line;
      line_number++;

      /* If set -v is in effect, echo the line read.  read_secondary_line/
         read_a_line leaves the newline at the end, so don't print another. */
      if (echo_input_at_read)
        fprintf (stderr, "%s", line);

      if (kill_leading && *line)
        {
          /* Hack:  To be compatible with some Bourne shells, we
             check the word before stripping the whitespace.  This
             is a hack, though. */
          if (STREQN (line, redir_word, redir_len) && line[redir_len] == '\n')
            goto document_done;

          while (*line == '\t')
            line++;
        }

      if (*line == 0)
        continue;

      if (STREQN (line, redir_word, redir_len) && line[redir_len] == '\n')
        goto document_done;

      len = strlen (line);
      if (len + document_index >= document_size)
        {
          document_size = document_size ? 2 * (document_size + len) : len + 2;
          document = (char *)xrealloc (document, document_size);
        }

      /* len is guaranteed to be > 0 because of the check for line
         being an empty string before the call to strlen. */
      FASTCOPY (line, document + document_index, len);
      document_index += len;
    }

  if (full_line == 0)
    internal_warning (
        _ ("here-document at line %d delimited by end-of-file (wanted `%s')"),
        lineno, redir_word);

document_done:
  if (document)
    document[document_index] = '\0';
  else
    {
      document = (char *)xmalloc (1);
      document[0] = '\0';
    }
  temp->redirectee.filename->word = document;
  here_doc_first_line = 0;
}

/* Generate a REDIRECT from SOURCE, DEST, and INSTRUCTION.
   INSTRUCTION is the instruction type, SOURCE is a file descriptor,
   and DEST is a file descriptor or a WORD_DESC *. */
REDIRECT::REDIRECT (REDIRECTEE source, r_instruction instruction,
                    REDIRECTEE dest_and_filename, redir_flags flags)
{
  WORD_DESC *w;
  int wlen;
  int64_t lfd;

  /* First do the common cases. */
  redirector = source;
  redirectee = dest_and_filename;
  instruction = instruction;
  rflags = flags;

  switch (instruction)
    {

    case r_output_direction: /* >foo */
    case r_output_force:     /* >| foo */
    case r_err_and_out:      /* &>filename */
      flags = O_TRUNC | O_WRONLY | O_CREAT;
      break;

    case r_appending_to:       /* >>foo */
    case r_append_err_and_out: /* &>> filename */
      flags = O_APPEND | O_WRONLY | O_CREAT;
      break;

    case r_input_direction:  /* <foo */
    case r_inputa_direction: /* foo & makes this. */
      flags = O_RDONLY;
      break;

    case r_input_output: /* <>foo */
      flags = O_RDWR | O_CREAT;
      break;

    case r_deblank_reading_until: /* <<-foo */
    case r_reading_until:         /* << foo */
    case r_reading_string:        /* <<< foo */
    case r_close_this:            /* <&- */
    case r_duplicating_input:     /* 1<&2 */
    case r_duplicating_output:    /* 1>&2 */
      break;

    /* the parser doesn't pass these. */
    case r_move_input:       /* 1<&2- */
    case r_move_output:      /* 1>&2- */
    case r_move_input_word:  /* 1<&$foo- */
    case r_move_output_word: /* 1>&$foo- */
      break;

    /* The way the lexer works we have to do this here. */
    case r_duplicating_input_word:  /* 1<&$foo */
    case r_duplicating_output_word: /* 1>&$foo */
      w = dest_and_filename.filename;
      wlen = w->word.size () - 1;
      if (w->word[wlen] == '-') /* Yuck */
        {
          w->word[wlen] = '\0';
          if (all_digits (w->word.c_str ())
              && legal_number (w->word.c_str (), &lfd)
              && lfd == static_cast<int> (lfd))
            {
              delete w;
              instruction = (instruction == r_duplicating_input_word)
                                ? r_move_input
                                : r_move_output;
              redirectee.dest = static_cast<int> (lfd);
            }
          else
            instruction = (instruction == r_duplicating_input_word)
                              ? r_move_input_word
                              : r_move_output_word;
        }

      break;

    default:
      programming_error (
          _ ("make_redirection: redirection instruction `%d' out of range"),
          instruction);
      abort ();
      break;
    }
}

COMMAND *
make_function_def (WORD_DESC *name, COMMAND *command, int lineno, int lstart)
{
  function_def *temp;
#if defined(ARRAY_VARS)
  SHELL_VAR *bash_source_v;
  ARRAY *bash_source_a;
#endif

  temp = (function_def *)xmalloc (sizeof (function_def));
  temp->command = command;
  temp->name = name;
  temp->line = lineno;
  temp->flags = 0;
  command->line = lstart;

  /* Information used primarily for debugging. */
  temp->source_file = 0;
#if defined(ARRAY_VARS)
  GET_ARRAY_FROM_VAR ("BASH_SOURCE", bash_source_v, bash_source_a);
  if (bash_source_a && array_num_elements (bash_source_a) > 0)
    temp->source_file = array_reference (bash_source_a, 0);
#endif
  /* Assume that shell functions without a source file before the shell is
     initialized come from the environment.  Otherwise default to "main"
     (usually functions being defined interactively) */
  if (temp->source_file == 0)
    temp->source_file = (char *)(shell_initialized ? "main" : "environment");

#if defined(DEBUGGER)
  bind_function_def (name->word, temp, 0);
#endif

  temp->source_file = temp->source_file ? savestring (temp->source_file) : 0;

  return make_command (cm_function_def, (simple_com *)temp);
}

COMMAND *
make_subshell_command (COMMAND *command)
{
  subshell_com *temp;

  temp = (subshell_com *)xmalloc (sizeof (subshell_com));
  temp->command = command;
  temp->flags = CMD_WANT_SUBSHELL;
  temp->line = line_number;
  return make_command (cm_subshell, (simple_com *)temp);
}

COMMAND *
make_coproc_command (const char *name, COMMAND *command)
{
  coproc_com *temp;

  temp = (coproc_com *)xmalloc (sizeof (coproc_com));
  temp->name = savestring (name);
  temp->command = command;
  temp->flags = CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL;
  return make_command (cm_coproc, (simple_com *)temp);
}

/* Reverse the word list and redirection list in the simple command
   has just been parsed.  It seems simpler to do this here the one
   time then by any other method that I can think of. */
COMMAND *
clean_simple_command (COMMAND *command)
{
  if (command->type != cm_simple)
    command_error ("clean_simple_command", CMDERR_BADTYPE, command->type, 0);
  else
    {
      command->value.Simple->words
          = REVERSE_LIST (command->value.Simple->words, WORD_LIST *);
      command->value.Simple->redirects
          = REVERSE_LIST (command->value.Simple->redirects, REDIRECT *);
    }

  parser_state &= ~PST_REDIRLIST;
  return command;
}

/* The Yacc grammar productions have a problem, in that they take a
   list followed by an ampersand (`&') and do a simple command connection,
   making the entire list effectively asynchronous, instead of just
   the last command.  This means that when the list is executed, all
   the commands have stdin set to /dev/null when job control is not
   active, instead of just the last.  This is wrong, and needs fixing
   up.  This function takes the `&' and applies it to the last command
   in the list.  This is done only for lists connected by `;'; it makes
   `;' bind `tighter' than `&'. */
COMMAND *
connect_async_list (COMMAND *command, COMMAND *command2, int connector)
{
  COMMAND *t, *t1, *t2;

  t1 = command;
  t = command->value.Connection->second;

  if (!t || (command->flags & CMD_WANT_SUBSHELL)
      || command->value.Connection->connector != ';')
    {
      t = command_connect (command, command2, connector);
      return t;
    }

  /* This is just defensive programming.  The Yacc precedence rules
     will generally hand this function a command where t points directly
     to the command we want (e.g. given a ; b ; c ; d &, t1 will point
     to the `a ; b ; c' list and t will be the `d').  We only want to do
     this if the list is not being executed as a unit in the background
     with `( ... )', so we have to check for CMD_WANT_SUBSHELL.  That's
     the only way to tell. */
  while (((t->flags & CMD_WANT_SUBSHELL) == 0) && t->type == cm_connection
         && t->value.Connection->connector == ';')
    {
      t1 = t;
      t = t->value.Connection->second;
    }
  /* Now we have t pointing to the last command in the list, and
     t1->value.Connection->second == t. */
  t2 = command_connect (t, command2, connector);
  t1->value.Connection->second = t2;
  return command;
}

} // namespace bash
