/* parse.yy - Bison C++ grammar for bash. */

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

%require "3.2"
%language "c++"
%define api.value.type variant
%define api.token.constructor
%define api.namespace {bash}

%{
#include "config.hh"
#include "shell.hh"

static inline bash::parser::token::token_kind_type
yylex () {
  return bash::the_shell->yylex ();
}

#if defined (__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

%}

/* Reserved words.  Members of the first group are only recognized
   in the case that they are preceded by a list_terminator.  Members
   of the second group are for [[...]] commands.  Members of the
   third group are recognized only under special circumstances. */
%token IF THEN ELSE ELIF FI CASE ESAC FOR SELECT WHILE UNTIL DO DONE FUNCTION COPROC
%token COND_START COND_END COND_ERROR
%token IN BANG TIME TIMEOPT TIMEIGN

/* More general tokens. yylex () knows how to make these. */
%token <WORD_DESC*> WORD ASSIGNMENT_WORD REDIR_WORD
%token <int> NUMBER
%token <WORD_LIST*> ARITH_CMD ARITH_FOR_EXPRS
%token <COMMAND*> COND_CMD
%token AND_AND OR_OR GREATER_GREATER LESS_LESS LESS_AND LESS_LESS_LESS
%token GREATER_AND SEMI_SEMI SEMI_AND SEMI_SEMI_AND
%token LESS_LESS_MINUS AND_GREATER AND_GREATER_GREATER LESS_GREATER
%token GREATER_BAR BAR_AND

/* The types that the various syntactical units return. */

%type <COMMAND*> inputunit command pipeline pipeline_command
%type <COMMAND*> list list0 list1 compound_list simple_list simple_list1
%type <COMMAND*> simple_command shell_command
%type <COMMAND*> for_command select_command case_command group_command
%type <COMMAND*> arith_command
%type <COMMAND*> cond_command
%type <COMMAND*> arith_for_command
%type <COMMAND*> coproc
%type <COMMAND*> function_def function_body if_command elif_clause subshell
%type <REDIRECT*> redirection
%type <REDIRECT_LIST*> redirection_list
%type <ELEMENT> simple_command_element
%type <WORD_LIST*> word_list pattern
%type <PATTERN_LIST*> pattern_list case_clause_sequence case_clause
%type <int> timespec
%type <int> list_terminator

%start inputunit

%left '&' ';' '\n' yacc_EOF
%left AND_AND OR_OR
%right '|' BAR_AND
%%

inputunit:	simple_list simple_list_terminator
			{
			  /* Case of regular command.  Discard the error
			     safety net, and return the command just parsed. */
			  Shell &sh = *the_shell;
			  sh.global_command = $1;
			  sh.eof_encountered = 0;
			  /* discard_parser_constructs (0); */
			  if (sh.parser_state & PST_CMDSUBST)
			    sh.parser_state |= PST_EOFTOKEN;
			  YYACCEPT;
			}
	|	'\n'
			{
			  /* Case of regular command, but not a very
			     interesting one.  Return a NULL command. */
			  Shell &sh = *the_shell;
			  sh.global_command = nullptr;
			  if (sh.parser_state & PST_CMDSUBST)
			    sh.parser_state |= PST_EOFTOKEN;
			  YYACCEPT;
			}
	|	error '\n'
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
	|	error yacc_EOF
			{
			  /* EOF after an error.  Do ignoreeof or not.  Really
			     only interesting in non-interactive shells */
			  Shell &sh = *the_shell;
			  sh.global_command = nullptr;
			  if (sh.last_command_exit_value == 0)
			    sh.last_command_exit_value = EX_BADUSAGE;	/* force error return */
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
	|	yacc_EOF
			{
			  /* Case of EOF seen by itself.  Do ignoreeof or
			     not. */
			  Shell &sh = *the_shell;
			  sh.global_command = nullptr;
			  sh.handle_eof_input_unit ();
			  YYACCEPT;
			}
	;

word_list:	WORD
			{ $$ = new WORD_LIST ($1); }
	|	word_list WORD
			{ $$ = $1; $1->push_back ($2); }
	;

redirection:	'>' WORD
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_output_direction, redir, REDIR_NOFLAGS);
			}
	|	'<' WORD
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_input_direction, redir, REDIR_NOFLAGS);
			}
	|	NUMBER '>' WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_output_direction, redir, REDIR_NOFLAGS);
			}
	|	NUMBER '<' WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_input_direction, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD '>' WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_output_direction, redir, REDIR_VARASSIGN);
			}
	|	REDIR_WORD '<' WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_input_direction, redir, REDIR_VARASSIGN);
			}
	|	GREATER_GREATER WORD
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_appending_to, redir, REDIR_NOFLAGS);
			}
	|	NUMBER GREATER_GREATER WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_appending_to, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD GREATER_GREATER WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_appending_to, redir, REDIR_VARASSIGN);
			}
	|	GREATER_BAR WORD
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_output_force, redir, REDIR_NOFLAGS);
			}
	|	NUMBER GREATER_BAR WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_output_force, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD GREATER_BAR WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_output_force, redir, REDIR_VARASSIGN);
			}
	|	LESS_GREATER WORD
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_input_output, redir, REDIR_NOFLAGS);
			}
	|	NUMBER LESS_GREATER WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_input_output, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD LESS_GREATER WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_input_output, redir, REDIR_VARASSIGN);
			}
	|	LESS_LESS WORD
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_reading_until, redir, REDIR_NOFLAGS);
			  the_shell->push_heredoc ($$);
			}
	|	NUMBER LESS_LESS WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_reading_until, redir, REDIR_NOFLAGS);
			  the_shell->push_heredoc ($$);
			}
	|	REDIR_WORD LESS_LESS WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_reading_until, redir, REDIR_VARASSIGN);
			  the_shell->push_heredoc ($$);
			}
	|	LESS_LESS_MINUS WORD
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_deblank_reading_until, redir, REDIR_NOFLAGS);
			  the_shell->push_heredoc ($$);
			}
	|	NUMBER LESS_LESS_MINUS WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_deblank_reading_until, redir, REDIR_NOFLAGS);
			  the_shell->push_heredoc ($$);
			}
	|	REDIR_WORD  LESS_LESS_MINUS WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_deblank_reading_until, redir, REDIR_VARASSIGN);
			  the_shell->push_heredoc ($$);
			}
	|	LESS_LESS_LESS WORD
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_reading_string, redir, REDIR_NOFLAGS);
			}
	|	NUMBER LESS_LESS_LESS WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_reading_string, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD LESS_LESS_LESS WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_reading_string, redir, REDIR_VARASSIGN);
			}
	|	LESS_AND NUMBER
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_duplicating_input, redir, REDIR_NOFLAGS);
			}
	|	NUMBER LESS_AND NUMBER
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_input, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD LESS_AND NUMBER
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_input, redir, REDIR_VARASSIGN);
			}
	|	GREATER_AND NUMBER
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_duplicating_output, redir, REDIR_NOFLAGS);
			}
	|	NUMBER GREATER_AND NUMBER
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_output, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD GREATER_AND NUMBER
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_output, redir, REDIR_VARASSIGN);
			}
	|	LESS_AND WORD
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_duplicating_input_word, redir, REDIR_NOFLAGS);
			}
	|	NUMBER LESS_AND WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_input_word, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD LESS_AND WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_input_word, redir, REDIR_VARASSIGN);
			}
	|	GREATER_AND WORD
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_duplicating_output_word, redir, REDIR_NOFLAGS);
			}
	|	NUMBER GREATER_AND WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_output_word, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD GREATER_AND WORD
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir ($3);
			  $$ = new REDIRECT (source, r_duplicating_output_word, redir, REDIR_VARASSIGN);
			}
	|	GREATER_AND '-'
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir (0);
			  $$ = new REDIRECT (source, r_close_this, redir, REDIR_NOFLAGS);
			}
	|	NUMBER GREATER_AND '-'
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir (0);
			  $$ = new REDIRECT (source, r_close_this, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD GREATER_AND '-'
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir (0);
			  $$ = new REDIRECT (source, r_close_this, redir, REDIR_VARASSIGN);
			}
	|	LESS_AND '-'
			{
			  REDIRECTEE source (0);
			  REDIRECTEE redir (0);
			  $$ = new REDIRECT (source, r_close_this, redir, REDIR_NOFLAGS);
			}
	|	NUMBER LESS_AND '-'
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir (0);
			  $$ = new REDIRECT (source, r_close_this, redir, REDIR_NOFLAGS);
			}
	|	REDIR_WORD LESS_AND '-'
			{
			  REDIRECTEE source ($1);
			  REDIRECTEE redir (0);
			  $$ = new REDIRECT (source, r_close_this, redir, REDIR_VARASSIGN);
			}
	|	AND_GREATER WORD
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_err_and_out, redir, REDIR_NOFLAGS);
			}
	|	AND_GREATER_GREATER WORD
			{
			  REDIRECTEE source (1);
			  REDIRECTEE redir ($2);
			  $$ = new REDIRECT (source, r_append_err_and_out, redir, REDIR_NOFLAGS);
			}
	;

simple_command_element: WORD
			{ $$.word = $1; $$.redirect = nullptr; }
	|	ASSIGNMENT_WORD
			{ $$.word = $1; $$.redirect = nullptr; }
	|	redirection
			{ $$.redirect = $1; $$.word = nullptr; }
	;

redirection_list: redirection
			{
			  $$ = new REDIRECT_LIST ($1);
			}
	|	redirection_list redirection
			{
			  $1->push_back ($2);
			  $$ = $1;
			}
	;

simple_command:	simple_command_element
			{ $$ = the_shell->make_simple_command ($1); }
	|	simple_command simple_command_element
			{ $$ = the_shell->make_simple_command ($2, $1); }
	;

command:	simple_command
			{ $$ = $1; }
	|	shell_command
			{ $$ = $1; }
	|	shell_command redirection_list
			{
			  COMMAND *tc;

			  tc = $1;
			  if (tc && tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = $2;
			    }
			  else if (tc)
			    tc->redirects = $2;
			  $$ = $1;
			}
	|	function_def
			{ $$ = $1; }
	|	coproc
			{ $$ = $1; }
	;

shell_command:	for_command
			{ $$ = $1; }
	|	case_command
			{ $$ = $1; }
 	|	WHILE compound_list DO compound_list DONE
			{ $$ = make_while_command ($2, $4); }
	|	UNTIL compound_list DO compound_list DONE
			{ $$ = make_until_command ($2, $4); }
	|	select_command
			{ $$ = $1; }
	|	if_command
			{ $$ = $1; }
	|	subshell
			{ $$ = $1; }
	|	group_command
			{ $$ = $1; }
	|	arith_command
			{ $$ = $1; }
	|	cond_command
			{ $$ = $1; }
	|	arith_for_command
			{ $$ = $1; }
	;

for_command:	FOR WORD newline_list DO compound_list DONE
			{
			  $$ = make_for_command ($2, add_string_to_list ("\"$@\"", nullptr), $5, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	FOR WORD newline_list '{' compound_list '}'
			{
			  $$ = make_for_command ($2, add_string_to_list ("\"$@\"", nullptr), $5, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	FOR WORD ';' newline_list DO compound_list DONE
			{
			  $$ = make_for_command ($2, add_string_to_list ("\"$@\"", nullptr), $6, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	FOR WORD ';' newline_list '{' compound_list '}'
			{
			  $$ = make_for_command ($2, add_string_to_list ("\"$@\"", nullptr), $6, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	FOR WORD newline_list IN word_list list_terminator newline_list DO compound_list DONE
			{
			  $$ = make_for_command ($2, REVERSE_LIST ($5, WORD_LIST *), $9, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	FOR WORD newline_list IN word_list list_terminator newline_list '{' compound_list '}'
			{
			  $$ = make_for_command ($2, REVERSE_LIST ($5, WORD_LIST *), $9, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	FOR WORD newline_list IN list_terminator newline_list DO compound_list DONE
			{
			  $$ = make_for_command ($2, nullptr, $8, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	FOR WORD newline_list IN list_terminator newline_list '{' compound_list '}'
			{
			  $$ = make_for_command ($2, nullptr, $8, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	;

arith_for_command:	FOR ARITH_FOR_EXPRS list_terminator newline_list DO compound_list DONE
				{
				  $$ = make_arith_for_command ($2, $6, arith_for_lineno);
				  if ($$ == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
	|		FOR ARITH_FOR_EXPRS list_terminator newline_list '{' compound_list '}'
				{
				  $$ = make_arith_for_command ($2, $6, arith_for_lineno);
				  if ($$ == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
	|		FOR ARITH_FOR_EXPRS DO compound_list DONE
				{
				  $$ = make_arith_for_command ($2, $4, arith_for_lineno);
				  if ($$ == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
	|		FOR ARITH_FOR_EXPRS '{' compound_list '}'
				{
				  $$ = make_arith_for_command ($2, $4, arith_for_lineno);
				  if ($$ == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
	;

select_command:	SELECT WORD newline_list DO list DONE
			{
			  $$ = make_select_command ($2, add_string_to_list ("\"$@\"", nullptr), $5, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	SELECT WORD newline_list '{' list '}'
			{
			  $$ = make_select_command ($2, add_string_to_list ("\"$@\"", nullptr), $5, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	SELECT WORD ';' newline_list DO list DONE
			{
			  $$ = make_select_command ($2, add_string_to_list ("\"$@\"", nullptr), $6, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	SELECT WORD ';' newline_list '{' list '}'
			{
			  $$ = make_select_command ($2, add_string_to_list ("\"$@\"", nullptr), $6, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	SELECT WORD newline_list IN word_list list_terminator newline_list DO list DONE
			{
			  $$ = make_select_command ($2, REVERSE_LIST ($5, WORD_LIST *), $9, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	SELECT WORD newline_list IN word_list list_terminator newline_list '{' list '}'
			{
			  $$ = make_select_command ($2, REVERSE_LIST ($5, WORD_LIST *), $9, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	SELECT WORD newline_list IN list_terminator newline_list DO compound_list DONE
			{
			  $$ = make_select_command ($2, nullptr, $8, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	SELECT WORD newline_list IN list_terminator newline_list '{' compound_list '}'
			{
			  $$ = make_select_command ($2, nullptr, $8, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	;

case_command:	CASE WORD newline_list IN newline_list ESAC
			{
			  $$ = make_case_command ($2, nullptr, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	CASE WORD newline_list IN case_clause_sequence newline_list ESAC
			{
			  $$ = make_case_command ($2, $5, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	|	CASE WORD newline_list IN case_clause ESAC
			{
			  $$ = make_case_command ($2, $5, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
	;

function_def:	WORD '(' ')' newline_list function_body
			{ $$ = make_function_def ($1, $5, function_dstart, function_bstart); }
	|	FUNCTION WORD '(' ')' newline_list function_body
			{ $$ = make_function_def ($2, $6, function_dstart, function_bstart); }
	|	FUNCTION WORD function_body
			{ $$ = make_function_def ($2, $3, function_dstart, function_bstart); }
	|	FUNCTION WORD '\n' newline_list function_body
			{ $$ = make_function_def ($2, $5, function_dstart, function_bstart); }
	;

function_body:	shell_command
			{ $$ = $1; }
	|	shell_command redirection_list
			{
			  COMMAND *tc = $1;

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
			  if (tc && tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = $2;
			    }
			  else if (tc)
			    tc->redirects = $2;
			  $$ = $1;
			}
	;

subshell:	'(' compound_list ')'
			{
			  $$ = make_subshell_command ($2);
			  $$->flags |= CMD_WANT_SUBSHELL;
			}
	;

coproc:		COPROC shell_command
			{
			  $$ = make_coproc_command ("COPROC", $2);
			  $$->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
	|	COPROC shell_command redirection_list
			{
			  COMMAND *tc;

			  tc = $2;
			  if (tc && tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = $3;
			    }
			  else if (tc)
			    tc->redirects = $3;
			  $$ = make_coproc_command ("COPROC", $2);
			  $$->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
	|	COPROC WORD shell_command
			{
			  $$ = make_coproc_command ($2->word, $3);
			  $$->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
	|	COPROC WORD shell_command redirection_list
			{
			  COMMAND *tc;

			  tc = $3;
			  if (tc && tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = $4;
			    }
			  else if (tc)
			    tc->redirects = $4;
			  $$ = make_coproc_command ($2->word, $3);
			  $$->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
	|	COPROC simple_command
			{
			  $$ = make_coproc_command ("COPROC", $2);
			  $$->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
	;

if_command:	IF compound_list THEN compound_list FI
			{ $$ = make_if_command ($2, $4, nullptr); }
	|	IF compound_list THEN compound_list ELSE compound_list FI
			{ $$ = make_if_command ($2, $4, $6); }
	|	IF compound_list THEN compound_list elif_clause FI
			{ $$ = make_if_command ($2, $4, $5); }
	;


group_command:	'{' compound_list '}'
			{ $$ = make_group_command ($2); }
	;

arith_command:	ARITH_CMD
			{ $$ = make_arith_command ($1); }
	;

cond_command:	COND_START COND_CMD COND_END
			{ $$ = $2; }
	;

elif_clause:	ELIF compound_list THEN compound_list
			{ $$ = make_if_command ($2, $4, nullptr); }
	|	ELIF compound_list THEN compound_list ELSE compound_list
			{ $$ = make_if_command ($2, $4, $6); }
	|	ELIF compound_list THEN compound_list elif_clause
			{ $$ = make_if_command ($2, $4, $5); }
	;

case_clause:	pattern_list
	|	case_clause_sequence pattern_list
			{ $2->next = $1; $$ = $2; }
	;

pattern_list:	newline_list pattern ')' compound_list
			{ $$ = make_pattern_list ($2, $4); }
	|	newline_list pattern ')' newline_list
			{ $$ = make_pattern_list ($2, nullptr); }
	|	newline_list '(' pattern ')' compound_list
			{ $$ = make_pattern_list ($3, $5); }
	|	newline_list '(' pattern ')' newline_list
			{ $$ = make_pattern_list ($3, nullptr); }
	;

case_clause_sequence:  pattern_list SEMI_SEMI
			{ $$ = $1; }
	|	case_clause_sequence pattern_list SEMI_SEMI
			{ $2->next = $1; $$ = $2; }
	|	pattern_list SEMI_AND
			{ $1->flags |= CASEPAT_FALLTHROUGH; $$ = $1; }
	|	case_clause_sequence pattern_list SEMI_AND
			{ $2->flags |= CASEPAT_FALLTHROUGH; $2->next = $1; $$ = $2; }
	|	pattern_list SEMI_SEMI_AND
			{ $1->flags |= CASEPAT_TESTNEXT; $$ = $1; }
	|	case_clause_sequence pattern_list SEMI_SEMI_AND
			{ $2->flags |= CASEPAT_TESTNEXT; $2->next = $1; $$ = $2; }
	;

pattern:	WORD
			{ $$ = make_word_list ($1, nullptr); }
	|	pattern '|' WORD
			{ $$ = make_word_list ($3, $1); }
	;

/* A list allows leading or trailing newlines and
   newlines as operators (equivalent to semicolons).
   It must end with a newline or semicolon.
   Lists are used within commands such as if, for, while.  */

list:		newline_list list0
			{
			  $$ = $2;
			  if (need_here_doc)
			    gather_here_documents ();
			 }
	;

compound_list:	list
	|	newline_list list1
			{
			  $$ = $2;
			}
	;

list0:  	list1 '\n' newline_list
	|	list1 '&' newline_list
			{
			  if ($1->type == cm_connection)
			    $$ = connect_async_list ($1, nullptr, '&');
			  else
			    $$ = command_connect ($1, nullptr, '&');
			}
	|	list1 ';' newline_list

	;

list1:		list1 AND_AND newline_list list1
			{ $$ = command_connect ($1, $4, AND_AND); }
	|	list1 OR_OR newline_list list1
			{ $$ = command_connect ($1, $4, OR_OR); }
	|	list1 '&' newline_list list1
			{
			  if ($1->type == cm_connection)
			    $$ = connect_async_list ($1, $4, '&');
			  else
			    $$ = command_connect ($1, $4, '&');
			}
	|	list1 ';' newline_list list1
			{ $$ = command_connect ($1, $4, ';'); }
	|	list1 '\n' newline_list list1
			{ $$ = command_connect ($1, $4, ';'); }
	|	pipeline_command
			{ $$ = $1; }
	;

simple_list_terminator:	'\n'
	|	yacc_EOF
	;

list_terminator:'\n'
		{ $$ = '\n'; }
	|	';'
		{ $$ = ';'; }
	|	yacc_EOF
		{ $$ = yacc_EOF; }
	;

newline_list:
	|	newline_list '\n'
	;

/* A simple_list is a list that contains no significant newlines
   and no leading or trailing newlines.  Newlines are allowed
   only following operators, where they are not significant.

   This is what an inputunit consists of.  */

simple_list:	simple_list1
			{
			  $$ = $1;
			  if (need_here_doc)
			    gather_here_documents ();
			  if ((parser_state & PST_CMDSUBST) && current_token == shell_eof_token)
			    {
			      global_command = $1;
			      eof_encountered = 0;
			      rewind_input_string ();
			      YYACCEPT;
			    }
			}
	|	simple_list1 '&'
			{
			  if ($1->type == cm_connection)
			    $$ = connect_async_list ($1, nullptr, '&');
			  else
			    $$ = command_connect ($1, nullptr, '&');
			  if (need_here_doc)
			    gather_here_documents ();
			  if ((parser_state & PST_CMDSUBST) && current_token == shell_eof_token)
			    {
			      global_command = $1;
			      eof_encountered = 0;
			      rewind_input_string ();
			      YYACCEPT;
			    }
			}
	|	simple_list1 ';'
			{
			  $$ = $1;
			  if (need_here_doc)
			    gather_here_documents ();
			  if ((parser_state & PST_CMDSUBST) && current_token == shell_eof_token)
			    {
			      global_command = $1;
			      eof_encountered = 0;
			      rewind_input_string ();
			      YYACCEPT;
			    }
			}
	;

simple_list1:	simple_list1 AND_AND newline_list simple_list1
			{ $$ = command_connect ($1, $4, AND_AND); }
	|	simple_list1 OR_OR newline_list simple_list1
			{ $$ = command_connect ($1, $4, OR_OR); }
	|	simple_list1 '&' simple_list1
			{
			  if ($1->type == cm_connection)
			    $$ = connect_async_list ($1, $3, '&');
			  else
			    $$ = command_connect ($1, $3, '&');
			}
	|	simple_list1 ';' simple_list1
			{ $$ = command_connect ($1, $3, ';'); }

	|	pipeline_command
			{ $$ = $1; }
	;

pipeline_command: pipeline
			{ $$ = $1; }
	|	BANG pipeline_command
			{
			  if ($2)
			    $2->flags ^= CMD_INVERT_RETURN;	/* toggle */
			  $$ = $2;
			}
	|	timespec pipeline_command
			{
			  if ($2)
			    $2->flags |= $1;
			  $$ = $2;
			}
	|	timespec list_terminator
			{
			  ELEMENT x;

			  /* Boy, this is unclean.  `time' by itself can
			     time a null command.  We cheat and push a
			     newline back if the list_terminator was a newline
			     to avoid the double-newline problem (one to
			     terminate this, one to terminate the command) */
			  x.word = 0;
			  x.redirect = 0;
			  $$ = make_simple_command (x, nullptr);
			  $$->flags |= $1;
			  /* XXX - let's cheat and push a newline back */
			  if ($2 == '\n')
			    token_to_read = '\n';
			  else if ($2 == ';')
			    token_to_read = ';';
			  parser_state &= ~PST_REDIRLIST;	/* make_simple_command sets this */
			}
	|	BANG list_terminator
			{
			  ELEMENT x;

			  /* This is just as unclean.  Posix says that `!'
			     by itself should be equivalent to `false'.
			     We cheat and push a
			     newline back if the list_terminator was a newline
			     to avoid the double-newline problem (one to
			     terminate this, one to terminate the command) */
			  x.word = 0;
			  x.redirect = 0;
			  $$ = make_simple_command (x, nullptr);
			  $$->flags |= CMD_INVERT_RETURN;
			  /* XXX - let's cheat and push a newline back */
			  if ($2 == '\n')
			    token_to_read = '\n';
			  if ($2 == ';')
			    token_to_read = ';';
			  parser_state &= ~PST_REDIRLIST;	/* make_simple_command sets this */
			}
	;

pipeline:	pipeline '|' newline_list pipeline
			{ $$ = command_connect ($1, $4, '|'); }
	|	pipeline BAR_AND newline_list pipeline
			{
			  /* Make cmd1 |& cmd2 equivalent to cmd1 2>&1 | cmd2 */
			  COMMAND *tc;
			  REDIRECTEE rd, sd;
			  REDIRECT *r;

			  tc = $1->type == cm_simple ? (COMMAND *)$1->value.Simple : $1;
			  sd.dest = 2;
			  rd.dest = 1;
			  r = make_redirection (sd, r_duplicating_output, rd, 0);
			  if (tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = r;
			    }
			  else
			    tc->redirects = r;

			  $$ = command_connect ($1, $4, '|');
			}
	|	command
			{ $$ = $1; }
	;

timespec:	TIME
			{ $$ = CMD_TIME_PIPELINE; }
	|	TIME TIMEOPT
			{ $$ = (CMD_TIME_PIPELINE | CMD_TIME_POSIX); }
	|	TIME TIMEIGN
			{ $$ = (CMD_TIME_PIPELINE | CMD_TIME_POSIX); }
	|	TIME TIMEOPT TIMEIGN
			{ $$ = (CMD_TIME_PIPELINE | CMD_TIME_POSIX); }
	;
%%

#if defined (__clang__)
#pragma clang diagnostic pop
#endif
