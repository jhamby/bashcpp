/* parse.yy - Bison C++ grammar for bash. */

/* Copyright (C) 1989-2022 Free Software Foundation, Inc.
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

%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor
%define api.namespace {bash}

%{
#include "config.hh"
#include "shell.hh"

static inline bash::parser::symbol_type
yylex ()
{
  return bash::the_shell->yylex ();
}

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wunreachable-code-break"
#pragma clang diagnostic ignored "-Wunused-macros"
#endif
%}

/* Reserved words.  Members of the first group are only recognized
   in the case that they are preceded by a list_terminator.  Members
   of the second group are for [[...]] commands.  Members of the
   third group are recognized only under special circumstances. */

%token IF THEN ELSE ELIF FI CASE ESAC FOR SELECT WHILE UNTIL DO DONE FUNCTION
%token COPROC COND_START COND_END COND_ERROR
%token IN BANG TIME TIMEOPT TIMEIGN

/* More general tokens. yylex () knows how to make these. */

%token <WORD_DESC_PTR> WORD ASSIGNMENT_WORD REDIR_WORD
%token <int64_t> NUMBER
%token <WORD_LIST_PTR> ARITH_CMD ARITH_FOR_EXPRS
%token <COMMAND_PTR> COND_CMD
%token AND_AND OR_OR GREATER_GREATER LESS_LESS LESS_AND LESS_LESS_LESS
%token GREATER_AND SEMI_SEMI SEMI_AND SEMI_SEMI_AND
%token LESS_LESS_MINUS AND_GREATER AND_GREATER_GREATER LESS_GREATER
%token GREATER_BAR BAR_AND

/* Special; never created by yylex; only set by parse_comsub and xparse_dolparen */
%token DOLPAREN

/* The types that the various syntactical units return. */

%type <COMMAND_PTR> inputunit command pipeline pipeline_command
%type <COMMAND_PTR> list0 list1 compound_list simple_list simple_list1
%type <COMMAND_PTR> simple_command shell_command
%type <COMMAND_PTR> for_command select_command case_command group_command
%type <COMMAND_PTR> arith_command
%type <COMMAND_PTR> cond_command
%type <COMMAND_PTR> arith_for_command
%type <COMMAND_PTR> coproc
%type <COMMAND_PTR> comsub
%type <COMMAND_PTR> function_def function_body if_command elif_clause subshell
%type <REDIRECT_PTR> redirection redirection_list
%type <ELEMENT> simple_command_element
%type <WORD_LIST_PTR> word_list pattern
%type <PATTERN_LIST_PTR> pattern_list case_clause_sequence case_clause
%type <int64_t> timespec
%type <int64_t> list_terminator

%start inputunit

%left '&' ';' '\n' yacc_EOF
%left AND_AND OR_OR
%right '|' BAR_AND
%%

inputunit: simple_list simple_list_terminator
                {
                  /* Case of regular command.  Discard the error
                     safety net, and return the command just parsed. */
                  Shell &sh = *the_shell;
                  sh.global_command = $1.value;
                  sh.eof_encountered = 0;
                  /* discard_parser_constructs (0); */
                  if (sh.parser_state & PST_CMDSUBST)
                    sh.parser_state |= PST_EOFTOKEN;
                  YYACCEPT;
                }
       | comsub
                {
                  /* This is special; look at the production and how
                     parse_comsub sets token_to_read */
                  Shell &sh = *the_shell;
                  sh.global_command = $1.value;
                  sh.eof_encountered = 0;
                  YYACCEPT;
                }
       | '\n'
                {
                  /* Case of regular command, but not a very
                     interesting one.  Return a nullptr command. */
                  Shell &sh = *the_shell;
                  sh.global_command = nullptr;
                  if (sh.parser_state & PST_CMDSUBST)
                    sh.parser_state |= PST_EOFTOKEN;
                  YYACCEPT;
                }
        | error '\n'
                {
                  /* Error during parsing.  Return NULL command. */
                  Shell &sh = *the_shell;
                  sh.global_command = nullptr;
                  sh.eof_encountered = 0;
                  /* discard_parser_constructs (1); */
                  if (sh.interactive && sh.parse_and_execute_level == 0)
                    {
                      YYACCEPT;
                    }
                  else
                    {
                      YYABORT;
                    }
                }
        | error yacc_EOF
                {
                  /* EOF after an error.  Do ignoreeof or not.  Really
                     only interesting in non-interactive shells */
                  Shell &sh = *the_shell;
                  sh.global_command = nullptr;
                  if (sh.last_command_exit_value == 0)
                    sh.last_command_exit_value
                        = EX_BADUSAGE; /* force error return */
                  if (sh.interactive && sh.parse_and_execute_level == 0)
                    {
                      sh.handle_eof_input_unit ();
                      YYACCEPT;
                    }
                  else
                    {
                      YYABORT;
                    }
                }
        | yacc_EOF
                {
                  // Case of EOF seen by itself.  Do ignoreeof or not.
                  Shell &sh = *the_shell;
                  sh.global_command = nullptr;
                  sh.handle_eof_input_unit ();
                  YYACCEPT;
                }
        ;

word_list: WORD
                {
                  $$ = WORD_LIST_PTR ($1);
                }
        | word_list WORD
                {
                  $$ = WORD_LIST_PTR ($2, $1);
                }
        ;

redirection: '>' WORD
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_output_direction, redir, REDIR_NOFLAGS);
                }
        | '<' WORD
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_input_direction, redir, REDIR_NOFLAGS);
                }
        | NUMBER '>' WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_output_direction, redir, REDIR_NOFLAGS);
                }
        | NUMBER '<' WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_input_direction, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD '>' WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_output_direction, redir, REDIR_VARASSIGN);
                }
        | REDIR_WORD '<' WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_input_direction, redir, REDIR_VARASSIGN);
                }
        | GREATER_GREATER WORD
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_appending_to, redir, REDIR_NOFLAGS);
                }
        | NUMBER GREATER_GREATER WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_appending_to, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD GREATER_GREATER WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_appending_to, redir, REDIR_VARASSIGN);
                }
        | GREATER_BAR WORD
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_output_force, redir, REDIR_NOFLAGS);
                }
        | NUMBER GREATER_BAR WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_output_force, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD GREATER_BAR WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_output_force, redir, REDIR_VARASSIGN);
                }
        | LESS_GREATER WORD
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_input_output, redir, REDIR_NOFLAGS);
                }
        | NUMBER LESS_GREATER WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_input_output, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD LESS_GREATER WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_input_output, redir, REDIR_VARASSIGN);
                }
        | LESS_LESS WORD
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc ($$.value);
                }
        | NUMBER LESS_LESS WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc ($$.value);
                }
        | REDIR_WORD LESS_LESS WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_reading_until, redir, REDIR_VARASSIGN);
                  the_shell->push_heredoc ($$.value);
                }
        | LESS_LESS_MINUS WORD
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_deblank_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc ($$.value);
                }
        | NUMBER LESS_LESS_MINUS WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_deblank_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc ($$.value);
                }
        | REDIR_WORD LESS_LESS_MINUS WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_deblank_reading_until, redir, REDIR_VARASSIGN);
                  the_shell->push_heredoc ($$.value);
                }
        | LESS_LESS_LESS WORD
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_reading_string, redir, REDIR_NOFLAGS);
                }
        | NUMBER LESS_LESS_LESS WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_reading_string, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD LESS_LESS_LESS WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_reading_string, redir, REDIR_VARASSIGN);
                }
        | LESS_AND NUMBER
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_input, redir, REDIR_NOFLAGS);
                }
        | NUMBER LESS_AND NUMBER
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_input, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD LESS_AND NUMBER
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_input, redir, REDIR_VARASSIGN);
                }
        | GREATER_AND NUMBER
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_output, redir, REDIR_NOFLAGS);
                }
        | NUMBER GREATER_AND NUMBER
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_output, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD GREATER_AND NUMBER
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_output, redir, REDIR_VARASSIGN);
                }
        | LESS_AND WORD
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_input_word, redir, REDIR_NOFLAGS);
                }
        | NUMBER LESS_AND WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_input_word, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD LESS_AND WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$
                      = REDIRECT_PTR (source, r_duplicating_input_word, redir,
                                      REDIR_VARASSIGN);
                }
        | GREATER_AND WORD
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_output_word, redir, REDIR_NOFLAGS);
                }
        | NUMBER GREATER_AND WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$ = REDIRECT_PTR (
                      source, r_duplicating_output_word, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD GREATER_AND WORD
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir ($3);
                  $$
                      = REDIRECT_PTR (source, r_duplicating_output_word, redir,
                                      REDIR_VARASSIGN);
                }
        | GREATER_AND '-'
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (0);
                  $$ = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
        | NUMBER GREATER_AND '-'
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir (0);
                  $$ = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD GREATER_AND '-'
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir (0);
                  $$ = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_VARASSIGN);
                }
        | LESS_AND '-'
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (0);
                  $$ = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
        | NUMBER LESS_AND '-'
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir (0);
                  $$ = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
        | REDIR_WORD LESS_AND '-'
                {
                  REDIRECTEE source ($1);
                  REDIRECTEE redir (0);
                  $$ = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_VARASSIGN);
                }
        | AND_GREATER WORD
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_err_and_out, redir, REDIR_NOFLAGS);
                }
        | AND_GREATER_GREATER WORD
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir ($2);
                  $$ = REDIRECT_PTR (
                      source, r_append_err_and_out, redir, REDIR_NOFLAGS);
                }
        ;

simple_command_element: WORD
                {
                  $$.word = $1.value;
                  $$.redirect = nullptr;
                }
        | ASSIGNMENT_WORD
                {
                  $$.word = $1.value;
                  $$.redirect = nullptr;
                }
        | redirection
                {
                  $$.redirect = $1.value;
                  $$.word = nullptr;
                }
        ;

redirection_list: redirection
                {
                  $$ = $1;
                }
        | redirection_list redirection
                {
                  $$ = REDIRECT_PTR ($1.value->append ($2.value));
                }
        ;

simple_command:	simple_command_element
                {
                  $$ = the_shell->make_simple_command ($1);
                }
        | simple_command simple_command_element
                {
                  $$ = the_shell->make_simple_command ($2, $1);
                }
        ;

command: simple_command
                {
                  $$ = the_shell->clean_simple_command ($1);
                }
        | shell_command
                {
                  $$ = $1;
                }
        | shell_command redirection_list
                {
                  COMMAND *tc = $1.value;
                  if (tc->redirects)
                    {
                      tc->redirects->append ($2.value);
                    }
                  else
                    {
                      tc->redirects = $2.value;
                    }
                  $$ = COMMAND_PTR (tc);
                }
        | function_def
                {
                  $$ = $1;
                }
        | coproc
                {
                  $$ = $1;
                }
        ;

shell_command: for_command
                {
                  $$ = $1;
                }
        | case_command
                {
                  $$ = $1;
                }
        | WHILE compound_list DO compound_list DONE
                {
                  $$ = COMMAND_PTR (new UNTIL_WHILE_COM (
                      cm_while, $2.value, $4.value));
                }
        | UNTIL compound_list DO compound_list DONE
                {
                  $$ = COMMAND_PTR (new UNTIL_WHILE_COM (
                      cm_until, $2.value, $4.value));
                }
        | select_command
                {
                  $$ = $1;
                }
        | if_command
                {
                  $$ = $1;
                }
        | subshell
                {
                  $$ = $1;
                }
        | group_command
                {
                  $$ = $1;
                }
        | arith_command
                {
                  $$ = $1;
                }
        | cond_command
                {
                  $$ = $1;
                }
        | arith_for_command
                {
                  $$ = $1;
                }
        ;

for_command: FOR WORD newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $5.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR WORD newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $5.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR WORD ';' newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $6.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR WORD ';' newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $6.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR WORD newline_list IN word_list list_terminator newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value,
                      $5.value->reverse (),
                      $9.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR WORD newline_list IN word_list list_terminator newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value,
                      $5.value->reverse (),
                      $9.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR WORD newline_list IN list_terminator newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value, nullptr,
                      $8.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR WORD newline_list IN list_terminator newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, $2.value, nullptr,
                      $8.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        ;

arith_for_command: FOR ARITH_FOR_EXPRS list_terminator newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_arith_for_command (
                      $2,
                      $6, sh.arith_for_lineno);
                  if ($$.value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR ARITH_FOR_EXPRS list_terminator newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_arith_for_command (
                      $2,
                      $6, sh.arith_for_lineno);
                  if ($$.value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR ARITH_FOR_EXPRS DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_arith_for_command (
                      $2,
                      $4, sh.arith_for_lineno);
                  if ($$.value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | FOR ARITH_FOR_EXPRS '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_arith_for_command (
                      $2,
                      $4, sh.arith_for_lineno);
                  if ($$.value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        ;

select_command:	SELECT WORD newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $5.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | SELECT WORD newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $5.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | SELECT WORD ';' newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $6.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | SELECT WORD ';' newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      $6.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | SELECT WORD newline_list IN word_list list_terminator newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      $5.value->reverse (),
                      $9.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | SELECT WORD newline_list IN word_list list_terminator newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      $5.value->reverse (),
                      $9.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | SELECT WORD newline_list IN list_terminator newline_list DO compound_list DONE
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      nullptr, $8.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | SELECT WORD newline_list IN list_terminator newline_list '{' compound_list '}'
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, $2.value,
                      nullptr, $8.value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        ;

case_command: CASE WORD newline_list IN newline_list ESAC
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new CASE_COM ($2.value, nullptr,
                                    sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | CASE WORD newline_list IN case_clause_sequence newline_list ESAC
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new CASE_COM ($2.value, $5.value,
                                    sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        | CASE WORD newline_list IN case_clause ESAC
                {
                  Shell &sh = *the_shell;
                  $$ = COMMAND_PTR (new CASE_COM ($2.value, $5.value,
                                    sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
        ;

function_def: WORD '(' ')' newline_list function_body
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_function_def (
                      $1, $5, sh.function_dstart,
                      sh.function_bstart);
                }
        | FUNCTION WORD '(' ')' newline_list function_body
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_function_def (
                      $2, $6, sh.function_dstart,
                      sh.function_bstart);
                }
        | FUNCTION WORD function_body
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_function_def (
                      $2, $3, sh.function_dstart,
                      sh.function_bstart);
                }
        | FUNCTION WORD '\n' newline_list function_body
                {
                  Shell &sh = *the_shell;
                  $$ = sh.make_function_def (
                      $2, $5, sh.function_dstart,
                      sh.function_bstart);
                }
        ;

function_body: shell_command
                {
                  $$ = $1;
                }
        | shell_command redirection_list
                {
                  COMMAND *tc = $1.value;

                  /* According to Posix.2 3.9.5, redirections
                     specified after the body of a function should
                     be attached to the function and performed when
                     the function is executed, not as part of the
                     function definition command. */
                  /* XXX - I don't think it matters, but we might
                     want to change this in the future to avoid
                     problems differentiating between a function
                     definition with a redirection and a function
                     definition containing a single command with a
                     redirection.  The two are semantically equivalent,
                     though -- the only difference is in how the
                     command printing code displays the redirections. */
                  if (tc->redirects)
                    tc->redirects->append ($2.value);
                  else
                    tc->redirects = $2.value;

                  $$ = COMMAND_PTR ($1);
                }
        ;

subshell: '(' compound_list ')'
                {
                  $$ = COMMAND_PTR (new SUBSHELL_COM ($2.value));
                }
        ;

comsub: DOLPAREN compound_list ')'
                {
                  $$ = $2;
                }
       | DOLPAREN newline_list ')'
                {
                  $$ = nullptr;
                }
       ;

coproc: COPROC shell_command
                {
                  $$ = COMMAND_PTR (new COPROC_COM ("COPROC", $2.value));
                }
        | COPROC shell_command redirection_list
                {
                  COMMAND *tc = $2.value;

                  if (tc->redirects)
                    tc->redirects->append ($3.value);
                  else
                    tc->redirects = $3.value;

                  $$ = COMMAND_PTR (new COPROC_COM ("COPROC", tc));
                }
        | COPROC WORD shell_command
                {
                  $$ = COMMAND_PTR (new COPROC_COM ($2.value->word, $3.value));
                }
        | COPROC WORD shell_command redirection_list
                {
                  COMMAND *tc = $3.value;

                  if (tc->redirects)
                    tc->redirects->append ($4.value);
                  else
                    tc->redirects = $4.value;

                  $$ = COMMAND_PTR (new COPROC_COM ($2.value->word, tc));
                }
        | COPROC simple_command
                {
                  $$ = COMMAND_PTR (new COPROC_COM (
                      "COPROC", the_shell->clean_simple_command ($2).value));
                }
        ;

if_command: IF compound_list THEN compound_list FI
                {
                  $$ = COMMAND_PTR (new IF_COM ($2.value, $4.value, nullptr));
                }
        | IF compound_list THEN compound_list ELSE compound_list FI
                {
                  $$ = COMMAND_PTR (new IF_COM ($2.value, $4.value, $6.value));
                }
        | IF compound_list THEN compound_list elif_clause FI
                {
                  $$ = COMMAND_PTR (new IF_COM ($2.value, $4.value, $5.value));
                }
        ;


group_command: '{' compound_list '}'
                {
                  $$ = COMMAND_PTR (new GROUP_COM ($2.value));
                }
        ;

arith_command: ARITH_CMD
                {
                  $$ = COMMAND_PTR (new ARITH_COM ($1.value));
                }
        ;

cond_command: COND_START COND_CMD COND_END
                {
                  $$ = $2;
                }
        ;

elif_clause: ELIF compound_list THEN compound_list
                {
                  $$ = COMMAND_PTR (new IF_COM ($2.value, $4.value, nullptr));
                }
        | ELIF compound_list THEN compound_list ELSE compound_list
                {
                  $$ = COMMAND_PTR (new IF_COM ($2.value, $4.value, $6.value));
                }
        | ELIF compound_list THEN compound_list elif_clause
                {
                  $$ = COMMAND_PTR (new IF_COM ($2.value, $4.value, $5.value));
                }
        ;

case_clause: pattern_list
        | case_clause_sequence pattern_list
                {
                  $2.value->set_next ($1.value);
                  $$ = $2;
                }
        ;

pattern_list: newline_list pattern ')' compound_list
                {
                  $$ = PATTERN_LIST_PTR (
                        new PATTERN_LIST ($2.value, $4.value));
                }
        | newline_list pattern ')' newline_list
                {
                  $$ = PATTERN_LIST_PTR (
                        new PATTERN_LIST ($2.value, nullptr));
                }
        | newline_list '(' pattern ')' compound_list
                {
                  $$ = PATTERN_LIST_PTR (
                        new PATTERN_LIST ($3.value, $5.value));
                }
        | newline_list '(' pattern ')' newline_list
                {
                  $$ = PATTERN_LIST_PTR (
                        new PATTERN_LIST ($3.value, nullptr));
                }
        ;

case_clause_sequence: pattern_list SEMI_SEMI
                {
                  $$ = $1;
                }
        | case_clause_sequence pattern_list SEMI_SEMI
                {
                  $2.value->set_next ($1.value);
                  $$ = $2;
                }
        | pattern_list SEMI_AND
                {
                  $1.value->flags |= CASEPAT_FALLTHROUGH;
                  $$ = $1;
                }
        | case_clause_sequence pattern_list SEMI_AND
                {
                  $2.value->flags |= CASEPAT_FALLTHROUGH;
                  $2.value->set_next ($1.value);
                  $$ = $2;
                }
        | pattern_list SEMI_SEMI_AND
                {
                  $1.value->flags |= CASEPAT_TESTNEXT;
                  $$ = $1;
                }
        | case_clause_sequence pattern_list SEMI_SEMI_AND
                {
                  $2.value->flags |= CASEPAT_TESTNEXT;
                  $2.value->set_next ($1.value);
                  $$ = $2;
                }
        ;

pattern: WORD
                {
                  $$ = WORD_LIST_PTR ($1);
                }
        | pattern '|' WORD
                {
                  $$ = WORD_LIST_PTR ($3, $1);
                }
        ;

/* A list allows leading or trailing newlines and
   newlines as operators (equivalent to semicolons).
   It must end with a newline or semicolon.
   Lists are used within commands such as if, for, while.  */

compound_list: newline_list list0
                {
                  $$ = $2;
                  if (the_shell->need_here_doc && the_shell->last_read_token == '\n')
                    the_shell->gather_here_documents ();
                }
        | newline_list list1
                {
                  $$ = $2;
                }
        ;

list0: list1 '\n' newline_list
        | list1 '&' newline_list
                {
                  COMMAND *cmd = $1.value;
                  if (cmd->type == cm_connection)
                    $$ = connect_async_list (cmd, nullptr, '&');
                  else
                    $$ = COMMAND_PTR (new CONNECTION (cmd, nullptr, '&'));
                }
        | list1 ';' newline_list
        ;

list1: list1 AND_AND newline_list list1
                {
                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value,
                                        static_cast<int> (token::AND_AND)));
                }
        | list1 OR_OR newline_list list1
                {
                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value,
                                        static_cast<int> (token::OR_OR)));
                }
        | list1 '&' newline_list list1
                {
                  COMMAND *cmd = $1.value;
                  if (cmd->type == cm_connection)
                    $$ = connect_async_list (cmd, $4.value, '&');
                  else
                    $$ = COMMAND_PTR (new CONNECTION (cmd, $4.value, '&'));
                }
        | list1 ';' newline_list list1
                {
                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value, ';'));
                }
        | list1 '\n' newline_list list1
                {
                  if (the_shell->parser_state & PST_CMDSUBST)
                    $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value, '\n'));
                  else
                    $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value, ';'));
                }
        | pipeline_command
                {
                  $$ = $1;
                }
        ;

simple_list_terminator:	'\n'
        | yacc_EOF
        ;

list_terminator:'\n'
                { $$ = '\n'; }
        | ';'
                { $$ = ';'; }
        | yacc_EOF
                { $$ = token::yacc_EOF; }
        ;

newline_list:
        | newline_list '\n'
        ;

/* A simple_list is a list that contains no significant newlines
   and no leading or trailing newlines.  Newlines are allowed
   only following operators, where they are not significant.

   This is what an inputunit consists of.  */

simple_list: simple_list1
                {
                  $$ = $1;
                  Shell &sh = *the_shell;
                  if (sh.need_here_doc)
                    sh.gather_here_documents (); /* XXX */
                  if ((sh.parser_state & PST_CMDSUBST)
                      && sh.current_token == sh.shell_eof_token)
                    {
        sh.internal_debug ("LEGACY: parser: command substitution simple_list1 -> simple_list");
                      sh.global_command = $1.value;
                      sh.eof_encountered = 0;
                      if (sh.bash_input.type == st_string)
                        sh.rewind_input_string ();
                      YYACCEPT;
                    }
                }
        | simple_list1 '&'
                {
                  COMMAND *cmd = $1.value;
                  if (cmd->type == cm_connection)
                    $$ = connect_async_list (cmd, nullptr, '&');
                  else
                    $$ = COMMAND_PTR (new CONNECTION (cmd, nullptr, '&'));
                  Shell &sh = *the_shell;
                  if (sh.need_here_doc)
                    sh.gather_here_documents (); /* XXX */
                  if ((sh.parser_state & PST_CMDSUBST)
                      && sh.current_token == sh.shell_eof_token)
                    {
        sh.internal_debug ("LEGACY: parser: command substitution simple_list1 '&' -> simple_list");
                      sh.global_command = cmd;
                      sh.eof_encountered = 0;
                      if (sh.bash_input.type == st_string)
                        sh.rewind_input_string ();
                      YYACCEPT;
                    }
                }
        | simple_list1 ';'
                {
                  $$ = $1;
                  Shell &sh = *the_shell;
                  if (sh.need_here_doc)
                    sh.gather_here_documents (); /* XXX */
                  if ((sh.parser_state & PST_CMDSUBST)
                      && sh.current_token == sh.shell_eof_token)
                    {
        sh.internal_debug ("LEGACY: parser: command substitution simple_list1 ';' -> simple_list");
                      sh.global_command = $1.value;
                      sh.eof_encountered = 0;
                      if (sh.bash_input.type == st_string)
                        sh.rewind_input_string ();
                      YYACCEPT;
                    }
                }
        ;

simple_list1: simple_list1 AND_AND newline_list simple_list1
                {
                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value,
                                        static_cast<int> (token::AND_AND)));
                }
        | simple_list1 OR_OR newline_list simple_list1
                {
                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value,
                                        static_cast<int> (token::OR_OR)));
                }
        | simple_list1 '&' simple_list1
                {
                  COMMAND *cmd = $1.value;
                  if (cmd->type == cm_connection)
                    $$ = connect_async_list (cmd, $3.value, '&');
                  else
                    $$ = COMMAND_PTR (new CONNECTION (cmd, $3.value, '&'));
                }
        | simple_list1 ';' simple_list1
                {
                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $3.value, ';'));
                }
        | pipeline_command
                {
                  $$ = $1;
                }
        ;

pipeline_command: pipeline
                {
                  $$ = $1;
                }
        | BANG pipeline_command
                {
                  if ($2.value)
                    $2.value->flags ^= CMD_INVERT_RETURN; /* toggle */
                  $$ = $2;
                }
        | timespec pipeline_command
                {
                  if ($2.value)
                    $2.value->flags |= static_cast<cmd_flags> ($1);

                  $$ = $2;
                }
        | timespec list_terminator
                {
                  ELEMENT x;

                  /* Boy, this is unclean.  `time' by itself can
                     time a null command.  We cheat and push a
                     newline back if the list_terminator was a newline
                     to avoid the double-newline problem (one to
                     terminate this, one to terminate the command) */
                  x.word = nullptr;
                  x.redirect = nullptr;
                  $$ = COMMAND_PTR (new SIMPLE_COM (x, nullptr));
                  $$.value->flags |= static_cast<cmd_flags> ($1);

                  /* XXX - let's cheat and push a newline back */
                  Shell &sh = *the_shell;
                  if ($2 == '\n')
                    sh.token_to_read = static_cast<parser::token_kind_type> ('\n');
                  else if ($2 == ';')
                    sh.token_to_read = static_cast<parser::token_kind_type> (';');
                  sh.parser_state
                      &= ~PST_REDIRLIST; /* SIMPLE_COM constructor sets this */
                }
        | BANG list_terminator
                {
                  ELEMENT x;

                  /* This is just as unclean.  Posix says that `!'
                     by itself should be equivalent to `false'.
                     We cheat and push a
                     newline back if the list_terminator was a newline
                     to avoid the double-newline problem (one to
                     terminate this, one to terminate the command) */
                  x.word = nullptr;
                  x.redirect = nullptr;
                  $$ = COMMAND_PTR (new SIMPLE_COM (x, nullptr));
                  $$.value->flags |= CMD_INVERT_RETURN;
                  /* XXX - let's cheat and push a newline back */
                  Shell &sh = *the_shell;
                  if ($2 == '\n')
                    sh.token_to_read = static_cast<parser::token_kind_type> ('\n');
                  if ($2 == ';')
                    sh.token_to_read = static_cast<parser::token_kind_type> (';');
                  sh.parser_state
                      &= ~PST_REDIRLIST; /* SIMPLE_COM constructor sets this */
                }
        ;

pipeline: pipeline '|' newline_list pipeline
                {
                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value, '|'));
                }
        | pipeline BAR_AND newline_list pipeline
                {
                  /* Make cmd1 |& cmd2 equivalent to cmd1 2>&1 | cmd2 */
                  COMMAND *tc;
                  REDIRECT *r;
                  REDIRECT **rp; // pointer to "redirects" to update

                  tc = $1.value;
                  SIMPLE_COM *scp = dynamic_cast<SIMPLE_COM *> (tc);
                  if (scp)
                    rp = &(scp->simple_redirects);
                  else
                    rp = &(tc->redirects);

                  REDIRECTEE sd (2);
                  REDIRECTEE rd (1);
                  r = new REDIRECT (sd, r_duplicating_output, rd,
                                    REDIR_NOFLAGS);
                  if (*rp)
                    (*rp)->append (r);
                  else
                    *rp = r;

                  $$ = COMMAND_PTR (new CONNECTION ($1.value, $4.value, '|'));
                }
        | command
                {
                  $$ = $1;
                }
        ;

timespec: TIME
                {
                  $$ = CMD_TIME_PIPELINE;
                }
        | TIME TIMEOPT
                {
                  $$ = (CMD_TIME_PIPELINE | CMD_TIME_POSIX);
                }
        | TIME TIMEIGN
                {
                  $$ = (CMD_TIME_PIPELINE | CMD_TIME_POSIX);
                }
        | TIME TIMEOPT TIMEIGN
                {
                  $$ = (CMD_TIME_PIPELINE | CMD_TIME_POSIX);
                }
        ;
%%

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
