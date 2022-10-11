/* print_cmd.cc -- A way to make readable commands from a command tree. */

/* Copyright (C) 1989-2022 Free Software Foundation, Inc.

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

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <cstdarg>

#include "bashintl.hh"
#include "command.hh"

#include "flags.hh"
#include "input.hh"
#include "shell.hh"

#include "shmbutil.hh"

namespace bash
{

#define CHECK_XTRACE_FP xtrace_fp = (xtrace_fp ? xtrace_fp : stderr)

#if 0
/* shell expansion characters: used in print_redirection_list */
#define EXPCHAR(c) ((c) == '{' || (c) == '~' || (c) == '$' || (c) == '`')
#endif

/* The internal function.  This is the real workhorse. */
void
Shell::make_command_string_internal (COMMAND *command)
{
  if (command == nullptr)
    cprintf ("");
  else
    {
      if (skip_this_indent)
        skip_this_indent--;
      else
        indent (indentation);

      if (command->flags & CMD_TIME_PIPELINE)
        {
          cprintf ("time ");
          if (command->flags & CMD_TIME_POSIX)
            cprintf ("-p ");
        }

      if (command->flags & CMD_INVERT_RETURN)
        cprintf ("! ");

      // dispatch to the virtual print method
      command->print (this);

      if (command->redirects)
        {
          cprintf (" ");
          print_redirection_list (command->redirects);
        }
    }
}

void
Shell::xtrace_set (int fd, FILE *fp)
{
  if (fd >= 0 && sh_validfd (fd) == 0)
    {
      internal_error (_ ("xtrace_set: %d: invalid file descriptor"), fd);
      return;
    }
  if (fp == nullptr)
    {
      internal_error (_ ("xtrace_set: NULL file pointer"));
      return;
    }
  if (fd >= 0 && fileno (fp) != fd)
    internal_warning (_ ("xtrace fd (%d) != fileno xtrace fp (%d)"), fd,
                      fileno (fp));

  xtrace_fd = fd;
  xtrace_fp = fp;
}

void
Shell::xtrace_reset ()
{
  if (xtrace_fd >= 0 && xtrace_fp)
    {
      std::fflush (xtrace_fp);
      std::fclose (xtrace_fp);
    }
  else if (xtrace_fd >= 0)
    close (xtrace_fd);

  xtrace_fd = -1;
  xtrace_fp = stderr;
}

/* Return a string denoting what our indirection level is. */
std::string
Shell::indirection_level_string ()
{
  const char *ps4;
  char ps4_firstc[MB_LEN_MAX + 1];
  int ps4_firstc_len;

  ps4 = get_string_value ("PS4");

  if (ps4 == nullptr || *ps4 == '\0')
    return indirection_string;

  int old = change_flag ('x', FLAG_OFF);

  std::string ps4_str (decode_prompt_string (ps4));

  if (old)
    change_flag ('x', FLAG_ON);

  if (ps4_str.empty ())
    return indirection_string;

#if defined(HANDLE_MULTIBYTE)
  size_t ps4_len = ps4_str.size ();
  ps4_firstc_len = std::mblen (ps4_str.c_str (), ps4_len);
  if (ps4_firstc_len == 1 || ps4_firstc_len == 0 || ps4_firstc_len < 0)
    {
      ps4_firstc[0] = ps4_str[0];
      ps4_firstc[ps4_firstc_len = 1] = '\0';
    }
  else
    std::memcpy (ps4_firstc, ps4_str.data (),
                 static_cast<size_t> (ps4_firstc_len + 1)); // include '\0'
#else
  ps4_firstc[0] = ps4_str[0];
  ps4_firstc[ps4_firstc_len = 1] = '\0';
#endif

  for (int i = 0; i < indirection_level; i++)
    {
      if (ps4_firstc_len == 1)
        indirection_string.push_back (ps4_firstc[0]);
      else
        indirection_string.append (ps4_firstc);
    }

  indirection_string.append (ps4 + ps4_firstc_len);
  delete[] ps4;
  return indirection_string;
}

void
Shell::xtrace_print_assignment (const char *name, const char *value,
                                bool assign_list, int xflags)
{
  std::string nval;

  CHECK_XTRACE_FP;

  if (xflags)
    std::fprintf (xtrace_fp, "%s", indirection_level_string ());

  /* VALUE should not be NULL when this is called. */
  if (*value == '\0' || assign_list)
    nval = value;
  else if (sh_contains_shell_metas (value))
    nval = sh_single_quote (value);
  else if (ansic_shouldquote (value))
    nval = ansic_quote (value);
  else
    nval = value;

  if (assign_list)
    std::fprintf (xtrace_fp, "%s=(%s)\n", name, nval.c_str ());
  else
    std::fprintf (xtrace_fp, "%s=%s\n", name, nval.c_str ());

  std::fflush (xtrace_fp);
}

/* A function to print the words of a simple command when set -x is on.  Also
   used to print the word list in a for or select command header; in that case,
   we suppress quoting the words because they haven't been expanded yet.
   XTFLAGS&1 means to print $PS4; XTFLAGS&2 means to suppress quoting the words
   in LIST. */
void
Shell::xtrace_print_word_list (WORD_LIST *list, int xtflags)
{
  WORD_LIST *w;

  CHECK_XTRACE_FP;

  if (xtflags & 1)
    fprintf (xtrace_fp, "%s", indirection_level_string ());

  for (w = list; w; w = w->next ())
    {
      std::string &t = w->word->word;
      if (t.empty ())
        std::fprintf (xtrace_fp, "''%s", w->next () ? " " : "");
      else if (xtflags & 2)
        std::fprintf (xtrace_fp, "%s%s", t.c_str (), w->next () ? " " : "");
      else if (sh_contains_shell_metas (t))
        {
          std::string x = sh_single_quote (t);
          std::fprintf (xtrace_fp, "%s%s", x.c_str (), w->next () ? " " : "");
        }
      else if (ansic_shouldquote (t))
        {
          std::string x = ansic_quote (t);
          std::fprintf (xtrace_fp, "%s%s", x.c_str (), w->next () ? " " : "");
        }
      else
        std::fprintf (xtrace_fp, "%s%s", t.c_str (), w->next () ? " " : "");
    }
  std::fprintf (xtrace_fp, "\n");
  std::fflush (xtrace_fp);
}

void
Shell::xtrace_print_for_command_head (FOR_SELECT_COM *for_command)
{
  CHECK_XTRACE_FP;
  std::fprintf (xtrace_fp, "%s", indirection_level_string ().c_str ());
  std::fprintf (xtrace_fp, "for %s in ", for_command->name->word.c_str ());
  xtrace_print_word_list (for_command->map_list, 2);
}

// This function is called from execute_for_command ().
void
FOR_SELECT_COM::print_head (Shell *shell)
{
#if defined(SELECT_COMMAND)
  if (type == cm_select)
    shell->cprintf ("select %s in ", name->word.c_str ());
  else
#endif
    shell->cprintf ("for %s in ", name->word.c_str ());

  shell->command_print_word_list (map_list, " ");
}

void
FOR_SELECT_COM::print (Shell *shell)
{
  print_head (shell);
  shell->cprintf (";");
  shell->newline ("do\n");
  shell->add_indentation ();
  shell->make_command_string_internal (action);
  shell->PRINT_DEFERRED_HEREDOCS ("");
  shell->semicolon ();
  shell->sub_indentation ();
  shell->newline ("done");
}

#if defined(ARITH_FOR_COMMAND)
void
ARITH_FOR_COM::print (Shell *shell)
{
  shell->cprintf ("for ((");
  shell->command_print_word_list (init, " ");
  shell->cprintf ("; ");
  shell->command_print_word_list (test, " ");
  shell->cprintf ("; ");
  shell->command_print_word_list (step, " ");
  shell->cprintf ("))");
  shell->newline ("do\n");
  shell->add_indentation ();
  shell->make_command_string_internal (action);
  shell->PRINT_DEFERRED_HEREDOCS ("");
  shell->semicolon ();
  shell->sub_indentation ();
  shell->newline ("done");
}
#endif /* ARITH_FOR_COMMAND */

#if defined(SELECT_COMMAND)
void
Shell::xtrace_print_select_command_head (FOR_SELECT_COM *select_command)
{
  CHECK_XTRACE_FP;
  std::fprintf (xtrace_fp, "%s", indirection_level_string ());
  std::fprintf (xtrace_fp, "select %s in ",
                select_command->name->word.c_str ());
  xtrace_print_word_list (select_command->map_list, 2);
}
#endif /* SELECT_COMMAND */

void
GROUP_COM::print (Shell *shell)
{
  shell->group_command_nesting++;
  shell->cprintf ("{ ");

  if (!shell->inside_function_def)
    shell->skip_this_indent++;
  else
    {
      /* This is a group command { ... } inside of a function
         definition, and should be printed as a multiline group
         command, using the current indentation. */
      shell->cprintf ("\n");
      shell->add_indentation ();
    }

  shell->make_command_string_internal (command);
  shell->PRINT_DEFERRED_HEREDOCS ("");

  if (shell->inside_function_def)
    {
      shell->cprintf ("\n");
      shell->sub_indentation ();
      shell->indent (shell->indentation);
    }
  else
    {
      shell->semicolon ();
      shell->cprintf (" ");
    }

  shell->cprintf ("}");

  shell->group_command_nesting--;
}

void
Shell::xtrace_print_case_command_head (CASE_COM *case_command)
{
  CHECK_XTRACE_FP;
  std::fprintf (xtrace_fp, "%s", indirection_level_string ());
  std::fprintf (xtrace_fp, "case %s in\n", case_command->word->word.c_str ());
}

// This function is called from execute_case_command ().
void
CASE_COM::print_head (Shell *shell)
{
  shell->cprintf ("case %s in ", word->word.c_str ());
}

void
CASE_COM::print (Shell *shell)
{
  print_head (shell);

  if (clauses)
    print_clauses (shell);
  shell->newline ("esac");
}

void
CASE_COM::print_clauses (Shell *shell)
{
  shell->add_indentation ();
  while (clauses)
    {
      shell->newline ("");
      shell->command_print_word_list (clauses->patterns, " | ");
      shell->cprintf (")\n");
      shell->add_indentation ();
      shell->make_command_string_internal (clauses->action);
      shell->sub_indentation ();
      shell->PRINT_DEFERRED_HEREDOCS ("");
      if (clauses->flags & CASEPAT_FALLTHROUGH)
        shell->newline (";&");
      else if (clauses->flags & CASEPAT_TESTNEXT)
        shell->newline (";;&");
      else
        shell->newline (";;");
      clauses = clauses->next ();
    }
  shell->sub_indentation ();
}

void
UNTIL_WHILE_COM::print (Shell *shell)
{
  if (type == cm_until)
    shell->cprintf ("%s ", "until");
  else
    shell->cprintf ("%s ", "while");

  shell->skip_this_indent++;
  shell->make_command_string_internal (test);
  shell->PRINT_DEFERRED_HEREDOCS ("");
  shell->semicolon ();
  shell->cprintf (" do\n"); /* was newline ("do\n"); */
  shell->add_indentation ();
  shell->make_command_string_internal (action);
  shell->PRINT_DEFERRED_HEREDOCS ("");
  shell->sub_indentation ();
  shell->semicolon ();
  shell->newline ("done");
}

void
IF_COM::print (Shell *shell)
{
  shell->cprintf ("if ");
  shell->skip_this_indent++;
  shell->make_command_string_internal (test);
  shell->semicolon ();
  shell->cprintf (" then\n");
  shell->add_indentation ();
  shell->make_command_string_internal (true_case);
  shell->PRINT_DEFERRED_HEREDOCS ("");
  shell->sub_indentation ();

  if (false_case)
    {
      shell->semicolon ();
      shell->newline ("else\n");
      shell->add_indentation ();
      shell->make_command_string_internal (false_case);
      shell->PRINT_DEFERRED_HEREDOCS ("");
      shell->sub_indentation ();
    }
  shell->semicolon ();
  shell->newline ("fi");
}

#if defined(DPAREN_ARITHMETIC) || defined(ARITH_FOR_COMMAND)
void
Shell::print_arith_command (WORD_LIST *arith_cmd_list)
{
  cprintf ("((");
  command_print_word_list (arith_cmd_list, " ");
  cprintf ("))");
}

void
ARITH_COM::print (Shell *shell)
{
  shell->print_arith_command (exp);
}
#endif

#if defined(COND_COMMAND)
void
Shell::print_cond_node (COND_COM *cond)
{
  if (cond->flags & CMD_INVERT_RETURN)
    cprintf ("! ");

  if (cond->type == COND_EXPR)
    {
      cprintf ("( ");
      print_cond_node (cond->left);
      cprintf (" )");
    }
  else if (cond->type == COND_AND)
    {
      print_cond_node (cond->left);
      cprintf (" && ");
      print_cond_node (cond->right);
    }
  else if (cond->type == COND_OR)
    {
      print_cond_node (cond->left);
      cprintf (" || ");
      print_cond_node (cond->right);
    }
  else if (cond->type == COND_UNARY)
    {
      cprintf ("%s", cond->op->word.c_str ());
      cprintf (" ");
      print_cond_node (cond->left);
    }
  else if (cond->type == COND_BINARY)
    {
      print_cond_node (cond->left);
      cprintf (" ");
      cprintf ("%s", cond->op->word.c_str ());
      cprintf (" ");
      print_cond_node (cond->right);
    }
  else if (cond->type == COND_TERM)
    {
      cprintf ("%s", cond->op->word.c_str ()); // need to add quoting here
    }
}

void
COND_COM::print (Shell *shell)
{
  shell->cprintf ("[[ ");
  shell->print_cond_node (this);
  shell->cprintf (" ]]");
}

#ifdef DEBUG
void
Shell::debug_print_cond_command (COND_COM *cond)
{
  std::fprintf (stderr, "DEBUG: ");
  the_printed_command.clear ();
  cond->print (this);
  std::fprintf (stderr, "%s\n", the_printed_command.c_str ());
}
#endif

void
Shell::xtrace_print_cond_term (cond_com_type type, bool invert, WORD_DESC *op,
                               const char *arg1, const char *arg2)
{
  CHECK_XTRACE_FP;
  the_printed_command.clear ();
  std::fprintf (xtrace_fp, "%s", indirection_level_string ());
  std::fprintf (xtrace_fp, "[[ ");

  if (invert)
    std::fprintf (xtrace_fp, "! ");

  if (type == COND_UNARY)
    {
      std::fprintf (xtrace_fp, "%s ", op->word.c_str ());
      std::fprintf (xtrace_fp, "%s", (arg1 && *arg1) ? arg1 : "''");
    }
  else if (type == COND_BINARY)
    {
      std::fprintf (xtrace_fp, "%s", (arg1 && *arg1) ? arg1 : "''");
      std::fprintf (xtrace_fp, " %s ", op->word.c_str ());
      std::fprintf (xtrace_fp, "%s", (arg2 && *arg2) ? arg2 : "''");
    }

  std::fprintf (xtrace_fp, " ]]\n");

  std::fflush (xtrace_fp);
}
#endif /* COND_COMMAND */

#if defined(DPAREN_ARITHMETIC) || defined(ARITH_FOR_COMMAND)
/* A function to print the words of an arithmetic command when set -x is on. */
void
Shell::xtrace_print_arith_cmd (WORD_LIST *list)
{
  WORD_LIST *w;

  CHECK_XTRACE_FP;
  std::fprintf (xtrace_fp, "%s", indirection_level_string ());
  std::fprintf (xtrace_fp, "(( ");

  for (w = list; w; w = w->next ())
    std::fprintf (xtrace_fp, "%s%s", w->word->word.c_str (),
                  w->next () ? " " : "");

  std::fprintf (xtrace_fp, " ))\n");

  std::fflush (xtrace_fp);
}
#endif

void
SIMPLE_COM::print (Shell *shell)
{
  if (words)
    shell->command_print_word_list (words, " ");

  if (redirects)
    {
      if (words)
        shell->cprintf (" ");
      shell->print_redirection_list (redirects);
    }
}

/* Print heredocs that are attached to the command before the connector
   represented by CSTRING.  The parsing semantics require us to print the
   here-doc delimiters, then the connector (CSTRING), then the here-doc
   bodies.  We print the here-doc delimiters in print_redirection_list
   and print the connector and the bodies here. We don't print the connector
   if it's a `;', but we use it to note not to print an extra space after the
   last heredoc body and newline. */
void
Shell::print_deferred_heredocs (const char *cstring)
{
  /* We now print the heredoc headers in print_redirection_list */
  if (cstring && cstring[0] && (cstring[0] != ';' || cstring[1]))
    cprintf ("%s", cstring);
  if (deferred_heredocs)
    {
      print_heredoc_bodies (deferred_heredocs);
      if (cstring && cstring[0] && (cstring[0] != ';' || cstring[1]))
        cprintf (" "); /* make sure there's at least one space */
      delete deferred_heredocs;
      was_heredoc = true;
    }
  deferred_heredocs = nullptr;
}

void
Shell::print_redirection_list (REDIRECT *redirects)
{
  REDIRECT *heredocs, *hdtail, *newredir;

  heredocs = nullptr;
  hdtail = heredocs;

  was_heredoc = false;
  while (redirects)
    {
      /* Defer printing the here document bodiess until we've printed the rest
         of the redirections, but print the headers in the order they're given.
       */
      if (redirects->instruction == r_reading_until
          || redirects->instruction == r_deblank_reading_until)
        {
          newredir = new REDIRECT (*redirects);

          print_heredoc_header (newredir);

          if (heredocs)
            {
              hdtail->set_next (newredir);
              hdtail = newredir;
            }
          else
            hdtail = heredocs = newredir;
        }
#if 0
      /* Remove this heuristic now that the command printing code doesn't
         unconditionally put in the redirector file descriptor. */
      else if (redirects->instruction == r_duplicating_output_word
               && (redirects->flags & REDIR_VARASSIGN) == 0
               && redirects->redirector.r.dest == 1)
        {
          /* Temporarily translate it as the execution code does. */
          const char *rw = redirects->redirectee.r.filename->word.c_str ();
          if (*rw && *rw != '-' && std::isdigit (*rw) == 0
              && EXPCHAR (*rw) == 0)
            redirects->instruction = r_err_and_out;
          print_redirection (redirects);
          redirects->instruction = r_duplicating_output_word;
        }
#endif
      else
        print_redirection (redirects);

      redirects = redirects->next ();
      if (redirects)
        cprintf (" ");
    }

  /* Now that we've printed all the other redirections (on one line),
     print the here documents.  If we're printing a connection, we wait until
     we print the connector symbol, then we print the here document bodies */
  if (heredocs && printing_connection)
    deferred_heredocs = heredocs;
  else if (heredocs)
    {
      print_heredoc_bodies (heredocs);
      delete heredocs;
    }
}

void
Shell::print_heredoc_header (REDIRECT *redirect)
{
  bool kill_leading = (redirect->instruction == r_deblank_reading_until);

  /* Here doc header */
  if (redirect->rflags & REDIR_VARASSIGN)
    cprintf ("{%s}", redirect->redirector.r.filename->word.c_str ());
  else if (redirect->redirector.r.dest != 0)
    cprintf ("%ld", static_cast<long> (redirect->redirector.r.dest));

  /* If the here document delimiter is quoted, single-quote it. */
  if (redirect->redirectee.r.filename->flags & W_QUOTED)
    {
      std::string x = sh_single_quote (redirect->here_doc_eof);
      cprintf ("<<%s%s", kill_leading ? "-" : "", x.c_str ());
    }
  else
    cprintf ("<<%s%s", kill_leading ? "-" : "",
             redirect->here_doc_eof.c_str ());
}

void
Shell::print_redirection (REDIRECT *redirect)
{
  int redirector, redir_fd;
  WORD_DESC *redirectee, *redir_word;

  redirectee = redirect->redirectee.r.filename;
  redir_fd = static_cast<int> (redirect->redirectee.r.dest);

  redir_word = redirect->redirector.r.filename;
  redirector = static_cast<int> (redirect->redirector.r.dest);

  switch (redirect->instruction)
    {
    case r_input_direction:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}", redir_word->word.c_str ());
      else if (redirector != 0)
        cprintf ("%d", redirector);
      cprintf ("< %s", redirectee->word.c_str ());
      break;

    case r_output_direction:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}", redir_word->word.c_str ());
      else if (redirector != 1)
        cprintf ("%d", redirector);
      cprintf ("> %s", redirectee->word.c_str ());
      break;

    case r_inputa_direction: /* Redirection created by the shell. */
      cprintf ("&");
      break;

    case r_output_force:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}", redir_word->word.c_str ());
      else if (redirector != 1)
        cprintf ("%d", redirector);
      cprintf (">| %s", redirectee->word.c_str ());
      break;

    case r_appending_to:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}", redir_word->word.c_str ());
      else if (redirector != 1)
        cprintf ("%d", redirector);
      cprintf (">> %s", redirectee->word.c_str ());
      break;

    case r_input_output:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}", redir_word->word.c_str ());
      else if (redirector != 1)
        cprintf ("%d", redirector);
      cprintf ("<> %s", redirectee->word.c_str ());
      break;

    case r_deblank_reading_until:
    case r_reading_until:
      print_heredoc_header (redirect);
      cprintf ("\n");
      print_heredoc_body (redirect);
      break;

    case r_reading_string:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}", redir_word->word.c_str ());
      else if (redirector != 0)
        cprintf ("%d", redirector);
#if 0
      /* Don't need to check whether or not to requote, since original quotes
         are still intact.  The only thing that has happened is that $'...'
         has been replaced with 'expanded ...'. */
      if (ansic_shouldquote (redirect->redirectee.r.filename->word.c_str ()))
	{
	  char *x;
	  x = ansic_quote (redirect->redirectee.r.filename->word.c_str ());
	  cprintf ("<<< %s", x);
	  delete[] x;
	}
      else
#endif
      cprintf ("<<< %s", redirect->redirectee.r.filename->word.c_str ());
      break;

    case r_duplicating_input:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}<&%d", redir_word->word.c_str (), redir_fd);
      else
        cprintf ("%d<&%d", redirector, redir_fd);
      break;

    case r_duplicating_output:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}>&%d", redir_word->word.c_str (), redir_fd);
      else
        cprintf ("%d>&%d", redirector, redir_fd);
      break;

    case r_duplicating_input_word:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}<&%s", redir_word->word.c_str (),
                 redirectee->word.c_str ());
      else if (redirector == 0)
        cprintf ("<&%s", redirectee->word.c_str ());
      else
        cprintf ("%d<&%s", redirector, redirectee->word.c_str ());
      break;

    case r_duplicating_output_word:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}>&%s", redir_word->word.c_str (),
                 redirectee->word.c_str ());
      else if (redirector == 1)
        cprintf (">&%s", redirectee->word.c_str ());
      else
        cprintf ("%d>&%s", redirector, redirectee->word.c_str ());
      break;

    case r_move_input:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}<&%d-", redir_word->word.c_str (), redir_fd);
      else
        cprintf ("%d<&%d-", redirector, redir_fd);
      break;

    case r_move_output:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}>&%d-", redir_word->word.c_str (), redir_fd);
      else
        cprintf ("%d>&%d-", redirector, redir_fd);
      break;

    case r_move_input_word:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}<&%s-", redir_word->word.c_str (),
                 redirectee->word.c_str ());
      else
        cprintf ("%d<&%s-", redirector, redirectee->word.c_str ());
      break;

    case r_move_output_word:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}>&%s-", redir_word->word.c_str (),
                 redirectee->word.c_str ());
      else
        cprintf ("%d>&%s-", redirector, redirectee->word.c_str ());
      break;

    case r_close_this:
      if (redirect->rflags & REDIR_VARASSIGN)
        cprintf ("{%s}>&-", redir_word->word.c_str ());
      else
        cprintf ("%d>&-", redirector);
      break;

    case r_err_and_out:
      cprintf ("&> %s", redirectee->word.c_str ());
      break;

    case r_append_err_and_out:
      cprintf ("&>> %s", redirectee->word.c_str ());
      break;
    }
}

void
FUNCTION_DEF::print (Shell *shell)
{
  shell->print_function_def (this);
}

void
Shell::print_function_def (FUNCTION_DEF *func)
{
  COMMAND *cmdcopy;
  REDIRECT *func_redirects = nullptr;

  /* When in posix mode, print functions as posix specifies them. */
  if (posixly_correct == 0)
    cprintf ("function %s () \n", func->name->word.c_str ());
  else
    cprintf ("%s () \n", func->name->word.c_str ());

  try
    {
      indent (indentation);
      cprintf ("{ \n");

      inside_function_def++;
      indentation += indentation_amount;

      cmdcopy = func->command->clone ();
      GROUP_COM *gcomcopy = dynamic_cast<GROUP_COM *> (cmdcopy);
      if (gcomcopy)
        {
          func_redirects = cmdcopy->redirects;
          cmdcopy->redirects = nullptr;
        }
      make_command_string_internal (gcomcopy ? gcomcopy->command : cmdcopy);
      PRINT_DEFERRED_HEREDOCS ("");
    }
  catch (const std::exception &)
    {
      inside_function_def = 0;
      indentation = 0;
      printing_connection = 0;
      deferred_heredocs = nullptr;
      throw;
    }

  indentation -= indentation_amount;
  inside_function_def--;

  if (func_redirects)
    {
      newline ("} ");
      print_redirection_list (func_redirects);
      cmdcopy->redirects = func_redirects;
    }
  else
    newline ("}");

  delete cmdcopy;
}

/* Return the string representation of the named function.
   NAME is the name of the function.
   COMMAND is the function body.  It should be a GROUP_COM.
   flags & FUNC_MULTILINE is non-zero to pretty-print, or zero for all on one
   line. flags & FUNC_EXTERNAL means convert from internal to external form.
  */
std::string
Shell::named_function_string (string_view name, COMMAND *command,
                              print_flags flags)
{
  char *result;
  int old_indent, old_amount;
  COMMAND *cmdcopy;
  REDIRECT *func_redirects;

  old_indent = indentation;
  old_amount = indentation_amount;
  the_printed_command.clear ();
  was_heredoc = false;
  deferred_heredocs = nullptr;
  printing_comsub = 0;

  if (name && *name)
    {
      if (find_reserved_word (name) >= 0)
        cprintf ("function ");
      cprintf ("%s ", name);
    }

  cprintf ("() ");

  if ((flags & FUNC_MULTILINE) == 0)
    {
      indentation = 1;
      indentation_amount = 0;
    }
  else
    {
      cprintf ("\n");
      indentation += indentation_amount;
    }

  inside_function_def++;

  cprintf ((flags & FUNC_MULTILINE) ? "{ \n" : "{ ");

  cmdcopy = command->clone ();
  /* Take any redirections specified in the function definition (which should
     apply to the function as a whole) and save them for printing later. */
  func_redirects = nullptr;
  GROUP_COM *gcomcopy = dynamic_cast<GROUP_COM *> (cmdcopy);
  if (gcomcopy)
    {
      func_redirects = cmdcopy->redirects;
      cmdcopy->redirects = nullptr;
    }
  make_command_string_internal (gcomcopy ? gcomcopy->command : cmdcopy);
  PRINT_DEFERRED_HEREDOCS ("");

  indentation = old_indent;
  indentation_amount = old_amount;
  inside_function_def--;

  if (func_redirects)
    {
      newline ("} ");
      print_redirection_list (func_redirects);
      cmdcopy->redirects = func_redirects;
    }
  else
    newline ("}");

  result = the_printed_command.data ();

  if ((flags & FUNC_MULTILINE) == 0)
    {
#if 0
      int i;
      for (i = 0; result[i]; i++)
	if (result[i] == '\n')
	  {
	    strcpy (result + i, result + i + 1);
	    --i;
	  }
#else
      if (result[2] == '\n') /* XXX -- experimental */
        memmove (result + 2, result + 3, strlen (result) - 2);
#endif
    }

  delete cmdcopy;

  if (flags & FUNC_EXTERNAL)
    result = remove_quoted_escapes (result);

  return result;
}

void
CONNECTION::print (Shell *shell)
{
  shell->skip_this_indent++;
  shell->printing_connection++;
  shell->make_command_string_internal (first);

  switch (connector)
    {
    case '&':
    case '|':
      {
        char c = static_cast<char> (connector);
        char s[3] = { ' ', c, '\0' };

        shell->print_deferred_heredocs (s);

        if (c != '&' || second)
          {
            shell->cprintf (" ");
            shell->skip_this_indent++;
          }
      }
      break;

    case parser::token::AND_AND:
      shell->print_deferred_heredocs (" && ");
      if (second)
        shell->skip_this_indent++;
      break;

    case parser::token::OR_OR:
      shell->print_deferred_heredocs (" || ");
      if (second)
        shell->skip_this_indent++;
      break;

    case ';':
    case '\n':
      {
        char c = static_cast<char> (connector);

        char s[2] = { (shell->printing_comsub ? c : ';'), '\0' };

        if (shell->deferred_heredocs == nullptr)
          {
            if (!shell->was_heredoc)
              shell->cprintf ("%s", s); /* inside_function_def? */
            else
              shell->was_heredoc = false;
          }
        else
          /* print_deferred_heredocs special-cases `;' */
          shell->print_deferred_heredocs (shell->inside_function_def ? ""
                                                                     : ";");

        if (shell->inside_function_def)
          shell->cprintf ("\n");
        else
          {
            if (c == ';')
              shell->cprintf (" ");
            if (second)
              shell->skip_this_indent++;
          }
        break;
      }

    default:
      shell->cprintf (_ ("print_command: bad connector `%d'"), connector);
      break;
    }

  shell->make_command_string_internal (second);
  shell->PRINT_DEFERRED_HEREDOCS ("");
  shell->printing_connection--;
}

void
SUBSHELL_COM::print (Shell *shell)
{
  shell->cprintf ("( ");
  shell->skip_this_indent++;
  shell->make_command_string_internal (command);
  shell->PRINT_DEFERRED_HEREDOCS ("");
  shell->cprintf (" )");
}

void
COPROC_COM::print (Shell *shell)
{
  shell->cprintf ("coproc %s ", name.c_str ());
  shell->skip_this_indent++;
  shell->make_command_string_internal (command);
}

/* How to make the string. */
void
Shell::cprintf (const char *control, ...)
{
  const char *s;
  char intbuf[INT_STRLEN_BOUND (unsigned int) + 1];
  int digit_arg;
  va_list args;

  va_start (args, control);

  s = control;
  std::string argp;
  while (s && *s)
    {
      char c = *s++;
      argp.clear ();

      if (c != '%' || !*s)
        {
          argp = c;
        }
      else
        {
          c = *s++;
          switch (c)
            {
            case '%':
              argp = c;
              break;

            case 's':
              argp = va_arg (args, char *);
              break;

            case 'd':
              /* Represent an out-of-range file descriptor with an out-of-range
                 integer value.  We can do this because the only use of `%d' in
                 the calls to cprintf is to output a file descriptor number for
                 a redirection. */
              digit_arg = va_arg (args, int);
              if (digit_arg < 0)
                {
                  std::sprintf (intbuf, "%u", static_cast<unsigned int> (-1));
                  argp = intbuf;
                }
              else
                argp = inttostr (digit_arg);
              break;

            case 'c':
              argp = static_cast<char> (va_arg (args, int));
              break;

            default:
              programming_error (_ ("cprintf: `%c': invalid format character"),
                                 c);
              /*NOTREACHED*/
            }
        }

      if (!argp.empty ())
        the_printed_command.append (argp);
    }

  va_end (args);
}

} // namespace bash
