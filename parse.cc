// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.



// First part of user prologue.
#line 28 "../bashcpp/parse.yy"

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

#line 59 "../bashcpp/parse.cc"


#include "../bashcpp/parse.hh"




#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif



// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 26 "../bashcpp/parse.yy"
namespace bash {
#line 138 "../bashcpp/parse.cc"

  /// Build a parser object.
  parser::parser ()
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr)
#else

#endif
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/



  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_kind_type
  parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_COND_CMD: // COND_CMD
      case symbol_kind::S_inputunit: // inputunit
      case symbol_kind::S_simple_command: // simple_command
      case symbol_kind::S_command: // command
      case symbol_kind::S_shell_command: // shell_command
      case symbol_kind::S_for_command: // for_command
      case symbol_kind::S_arith_for_command: // arith_for_command
      case symbol_kind::S_select_command: // select_command
      case symbol_kind::S_case_command: // case_command
      case symbol_kind::S_function_def: // function_def
      case symbol_kind::S_function_body: // function_body
      case symbol_kind::S_subshell: // subshell
      case symbol_kind::S_comsub: // comsub
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.YY_MOVE_OR_COPY< COMMAND_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.YY_MOVE_OR_COPY< ELEMENT > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.YY_MOVE_OR_COPY< PATTERN_LIST_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.YY_MOVE_OR_COPY< REDIRECT_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.YY_MOVE_OR_COPY< WORD_DESC_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.YY_MOVE_OR_COPY< WORD_LIST_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.YY_MOVE_OR_COPY< int64_t > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s)
  {
    switch (that.kind ())
    {
      case symbol_kind::S_COND_CMD: // COND_CMD
      case symbol_kind::S_inputunit: // inputunit
      case symbol_kind::S_simple_command: // simple_command
      case symbol_kind::S_command: // command
      case symbol_kind::S_shell_command: // shell_command
      case symbol_kind::S_for_command: // for_command
      case symbol_kind::S_arith_for_command: // arith_for_command
      case symbol_kind::S_select_command: // select_command
      case symbol_kind::S_case_command: // case_command
      case symbol_kind::S_function_def: // function_def
      case symbol_kind::S_function_body: // function_body
      case symbol_kind::S_subshell: // subshell
      case symbol_kind::S_comsub: // comsub
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.move< COMMAND_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.move< ELEMENT > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.move< PATTERN_LIST_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.move< REDIRECT_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.move< WORD_DESC_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.move< WORD_LIST_PTR > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.move< int64_t > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_COND_CMD: // COND_CMD
      case symbol_kind::S_inputunit: // inputunit
      case symbol_kind::S_simple_command: // simple_command
      case symbol_kind::S_command: // command
      case symbol_kind::S_shell_command: // shell_command
      case symbol_kind::S_for_command: // for_command
      case symbol_kind::S_arith_for_command: // arith_for_command
      case symbol_kind::S_select_command: // select_command
      case symbol_kind::S_case_command: // case_command
      case symbol_kind::S_function_def: // function_def
      case symbol_kind::S_function_body: // function_body
      case symbol_kind::S_subshell: // subshell
      case symbol_kind::S_comsub: // comsub
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.copy< COMMAND_PTR > (that.value);
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.copy< ELEMENT > (that.value);
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.copy< PATTERN_LIST_PTR > (that.value);
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.copy< REDIRECT_PTR > (that.value);
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.copy< WORD_DESC_PTR > (that.value);
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.copy< WORD_LIST_PTR > (that.value);
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.copy< int64_t > (that.value);
        break;

      default:
        break;
    }

    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_COND_CMD: // COND_CMD
      case symbol_kind::S_inputunit: // inputunit
      case symbol_kind::S_simple_command: // simple_command
      case symbol_kind::S_command: // command
      case symbol_kind::S_shell_command: // shell_command
      case symbol_kind::S_for_command: // for_command
      case symbol_kind::S_arith_for_command: // arith_for_command
      case symbol_kind::S_select_command: // select_command
      case symbol_kind::S_case_command: // case_command
      case symbol_kind::S_function_def: // function_def
      case symbol_kind::S_function_body: // function_body
      case symbol_kind::S_subshell: // subshell
      case symbol_kind::S_comsub: // comsub
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.move< COMMAND_PTR > (that.value);
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.move< ELEMENT > (that.value);
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.move< PATTERN_LIST_PTR > (that.value);
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.move< REDIRECT_PTR > (that.value);
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.move< WORD_DESC_PTR > (that.value);
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.move< WORD_LIST_PTR > (that.value);
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.move< int64_t > (that.value);
        break;

      default:
        break;
    }

    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " (";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex ());
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_COND_CMD: // COND_CMD
      case symbol_kind::S_inputunit: // inputunit
      case symbol_kind::S_simple_command: // simple_command
      case symbol_kind::S_command: // command
      case symbol_kind::S_shell_command: // shell_command
      case symbol_kind::S_for_command: // for_command
      case symbol_kind::S_arith_for_command: // arith_for_command
      case symbol_kind::S_select_command: // select_command
      case symbol_kind::S_case_command: // case_command
      case symbol_kind::S_function_def: // function_def
      case symbol_kind::S_function_body: // function_body
      case symbol_kind::S_subshell: // subshell
      case symbol_kind::S_comsub: // comsub
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        yylhs.value.emplace< COMMAND_PTR > ();
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        yylhs.value.emplace< ELEMENT > ();
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        yylhs.value.emplace< PATTERN_LIST_PTR > ();
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        yylhs.value.emplace< REDIRECT_PTR > ();
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        yylhs.value.emplace< WORD_DESC_PTR > ();
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        yylhs.value.emplace< WORD_LIST_PTR > ();
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        yylhs.value.emplace< int64_t > ();
        break;

      default:
        break;
    }



      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 2: // inputunit: simple_list simple_list_terminator
#line 96 "../bashcpp/parse.yy"
                {
                  /* Case of regular command.  Discard the error
                     safety net, and return the command just parsed. */
                  Shell &sh = *the_shell;
                  sh.global_command = yystack_[1].value.as < COMMAND_PTR > ().value;
                  sh.eof_encountered = 0;
                  /* discard_parser_constructs (0); */
                  if (sh.parser_state & PST_CMDSUBST)
                    sh.parser_state |= PST_EOFTOKEN;
                  YYACCEPT;
                }
#line 844 "../bashcpp/parse.cc"
    break;

  case 3: // inputunit: comsub
#line 108 "../bashcpp/parse.yy"
                {
                  /* This is special; look at the production and how
                     parse_comsub sets token_to_read */
                  Shell &sh = *the_shell;
                  sh.global_command = yystack_[0].value.as < COMMAND_PTR > ().value;
                  sh.eof_encountered = 0;
                  YYACCEPT;
                }
#line 857 "../bashcpp/parse.cc"
    break;

  case 4: // inputunit: '\n'
#line 117 "../bashcpp/parse.yy"
                {
                  /* Case of regular command, but not a very
                     interesting one.  Return a nullptr command. */
                  Shell &sh = *the_shell;
                  sh.global_command = nullptr;
                  if (sh.parser_state & PST_CMDSUBST)
                    sh.parser_state |= PST_EOFTOKEN;
                  YYACCEPT;
                }
#line 871 "../bashcpp/parse.cc"
    break;

  case 5: // inputunit: error '\n'
#line 127 "../bashcpp/parse.yy"
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
#line 891 "../bashcpp/parse.cc"
    break;

  case 6: // inputunit: error yacc_EOF
#line 143 "../bashcpp/parse.yy"
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
#line 914 "../bashcpp/parse.cc"
    break;

  case 7: // inputunit: yacc_EOF
#line 162 "../bashcpp/parse.yy"
                {
                  // Case of EOF seen by itself.  Do ignoreeof or not.
                  Shell &sh = *the_shell;
                  sh.global_command = nullptr;
                  sh.handle_eof_input_unit ();
                  YYACCEPT;
                }
#line 926 "../bashcpp/parse.cc"
    break;

  case 8: // word_list: WORD
#line 172 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < WORD_LIST_PTR > () = WORD_LIST_PTR (yystack_[0].value.as < WORD_DESC_PTR > ());
                }
#line 934 "../bashcpp/parse.cc"
    break;

  case 9: // word_list: word_list WORD
#line 176 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < WORD_LIST_PTR > () = WORD_LIST_PTR (yystack_[0].value.as < WORD_DESC_PTR > (), yystack_[1].value.as < WORD_LIST_PTR > ());
                }
#line 942 "../bashcpp/parse.cc"
    break;

  case 10: // redirection: '>' WORD
#line 182 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_output_direction, redir, REDIR_NOFLAGS);
                }
#line 953 "../bashcpp/parse.cc"
    break;

  case 11: // redirection: '<' WORD
#line 189 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_input_direction, redir, REDIR_NOFLAGS);
                }
#line 964 "../bashcpp/parse.cc"
    break;

  case 12: // redirection: NUMBER '>' WORD
#line 196 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_output_direction, redir, REDIR_NOFLAGS);
                }
#line 975 "../bashcpp/parse.cc"
    break;

  case 13: // redirection: NUMBER '<' WORD
#line 203 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_input_direction, redir, REDIR_NOFLAGS);
                }
#line 986 "../bashcpp/parse.cc"
    break;

  case 14: // redirection: REDIR_WORD '>' WORD
#line 210 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_output_direction, redir, REDIR_VARASSIGN);
                }
#line 997 "../bashcpp/parse.cc"
    break;

  case 15: // redirection: REDIR_WORD '<' WORD
#line 217 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_input_direction, redir, REDIR_VARASSIGN);
                }
#line 1008 "../bashcpp/parse.cc"
    break;

  case 16: // redirection: GREATER_GREATER WORD
#line 224 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_appending_to, redir, REDIR_NOFLAGS);
                }
#line 1019 "../bashcpp/parse.cc"
    break;

  case 17: // redirection: NUMBER GREATER_GREATER WORD
#line 231 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_appending_to, redir, REDIR_NOFLAGS);
                }
#line 1030 "../bashcpp/parse.cc"
    break;

  case 18: // redirection: REDIR_WORD GREATER_GREATER WORD
#line 238 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_appending_to, redir, REDIR_VARASSIGN);
                }
#line 1041 "../bashcpp/parse.cc"
    break;

  case 19: // redirection: GREATER_BAR WORD
#line 245 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_output_force, redir, REDIR_NOFLAGS);
                }
#line 1052 "../bashcpp/parse.cc"
    break;

  case 20: // redirection: NUMBER GREATER_BAR WORD
#line 252 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_output_force, redir, REDIR_NOFLAGS);
                }
#line 1063 "../bashcpp/parse.cc"
    break;

  case 21: // redirection: REDIR_WORD GREATER_BAR WORD
#line 259 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_output_force, redir, REDIR_VARASSIGN);
                }
#line 1074 "../bashcpp/parse.cc"
    break;

  case 22: // redirection: LESS_GREATER WORD
#line 266 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_input_output, redir, REDIR_NOFLAGS);
                }
#line 1085 "../bashcpp/parse.cc"
    break;

  case 23: // redirection: NUMBER LESS_GREATER WORD
#line 273 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_input_output, redir, REDIR_NOFLAGS);
                }
#line 1096 "../bashcpp/parse.cc"
    break;

  case 24: // redirection: REDIR_WORD LESS_GREATER WORD
#line 280 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_input_output, redir, REDIR_VARASSIGN);
                }
#line 1107 "../bashcpp/parse.cc"
    break;

  case 25: // redirection: LESS_LESS WORD
#line 287 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc (yylhs.value.as < REDIRECT_PTR > ().value);
                }
#line 1119 "../bashcpp/parse.cc"
    break;

  case 26: // redirection: NUMBER LESS_LESS WORD
#line 295 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc (yylhs.value.as < REDIRECT_PTR > ().value);
                }
#line 1131 "../bashcpp/parse.cc"
    break;

  case 27: // redirection: REDIR_WORD LESS_LESS WORD
#line 303 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_reading_until, redir, REDIR_VARASSIGN);
                  the_shell->push_heredoc (yylhs.value.as < REDIRECT_PTR > ().value);
                }
#line 1143 "../bashcpp/parse.cc"
    break;

  case 28: // redirection: LESS_LESS_MINUS WORD
#line 311 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_deblank_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc (yylhs.value.as < REDIRECT_PTR > ().value);
                }
#line 1155 "../bashcpp/parse.cc"
    break;

  case 29: // redirection: NUMBER LESS_LESS_MINUS WORD
#line 319 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_deblank_reading_until, redir, REDIR_NOFLAGS);
                  the_shell->push_heredoc (yylhs.value.as < REDIRECT_PTR > ().value);
                }
#line 1167 "../bashcpp/parse.cc"
    break;

  case 30: // redirection: REDIR_WORD LESS_LESS_MINUS WORD
#line 327 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_deblank_reading_until, redir, REDIR_VARASSIGN);
                  the_shell->push_heredoc (yylhs.value.as < REDIRECT_PTR > ().value);
                }
#line 1179 "../bashcpp/parse.cc"
    break;

  case 31: // redirection: LESS_LESS_LESS WORD
#line 335 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_reading_string, redir, REDIR_NOFLAGS);
                }
#line 1190 "../bashcpp/parse.cc"
    break;

  case 32: // redirection: NUMBER LESS_LESS_LESS WORD
#line 342 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_reading_string, redir, REDIR_NOFLAGS);
                }
#line 1201 "../bashcpp/parse.cc"
    break;

  case 33: // redirection: REDIR_WORD LESS_LESS_LESS WORD
#line 349 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_reading_string, redir, REDIR_VARASSIGN);
                }
#line 1212 "../bashcpp/parse.cc"
    break;

  case 34: // redirection: LESS_AND NUMBER
#line 356 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (yystack_[0].value.as < int64_t > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_input, redir, REDIR_NOFLAGS);
                }
#line 1223 "../bashcpp/parse.cc"
    break;

  case 35: // redirection: NUMBER LESS_AND NUMBER
#line 363 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < int64_t > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_input, redir, REDIR_NOFLAGS);
                }
#line 1234 "../bashcpp/parse.cc"
    break;

  case 36: // redirection: REDIR_WORD LESS_AND NUMBER
#line 370 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < int64_t > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_input, redir, REDIR_VARASSIGN);
                }
#line 1245 "../bashcpp/parse.cc"
    break;

  case 37: // redirection: GREATER_AND NUMBER
#line 377 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (yystack_[0].value.as < int64_t > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_output, redir, REDIR_NOFLAGS);
                }
#line 1256 "../bashcpp/parse.cc"
    break;

  case 38: // redirection: NUMBER GREATER_AND NUMBER
#line 384 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < int64_t > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_output, redir, REDIR_NOFLAGS);
                }
#line 1267 "../bashcpp/parse.cc"
    break;

  case 39: // redirection: REDIR_WORD GREATER_AND NUMBER
#line 391 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < int64_t > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_output, redir, REDIR_VARASSIGN);
                }
#line 1278 "../bashcpp/parse.cc"
    break;

  case 40: // redirection: LESS_AND WORD
#line 398 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_input_word, redir, REDIR_NOFLAGS);
                }
#line 1289 "../bashcpp/parse.cc"
    break;

  case 41: // redirection: NUMBER LESS_AND WORD
#line 405 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_input_word, redir, REDIR_NOFLAGS);
                }
#line 1300 "../bashcpp/parse.cc"
    break;

  case 42: // redirection: REDIR_WORD LESS_AND WORD
#line 412 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > ()
                      = REDIRECT_PTR (source, r_duplicating_input_word, redir,
                                      REDIR_VARASSIGN);
                }
#line 1312 "../bashcpp/parse.cc"
    break;

  case 43: // redirection: GREATER_AND WORD
#line 420 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_output_word, redir, REDIR_NOFLAGS);
                }
#line 1323 "../bashcpp/parse.cc"
    break;

  case 44: // redirection: NUMBER GREATER_AND WORD
#line 427 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_duplicating_output_word, redir, REDIR_NOFLAGS);
                }
#line 1334 "../bashcpp/parse.cc"
    break;

  case 45: // redirection: REDIR_WORD GREATER_AND WORD
#line 434 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > ()
                      = REDIRECT_PTR (source, r_duplicating_output_word, redir,
                                      REDIR_VARASSIGN);
                }
#line 1346 "../bashcpp/parse.cc"
    break;

  case 46: // redirection: GREATER_AND '-'
#line 442 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (0);
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
#line 1357 "../bashcpp/parse.cc"
    break;

  case 47: // redirection: NUMBER GREATER_AND '-'
#line 449 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (0);
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
#line 1368 "../bashcpp/parse.cc"
    break;

  case 48: // redirection: REDIR_WORD GREATER_AND '-'
#line 456 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (0);
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_VARASSIGN);
                }
#line 1379 "../bashcpp/parse.cc"
    break;

  case 49: // redirection: LESS_AND '-'
#line 463 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (0);
                  REDIRECTEE redir (0);
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
#line 1390 "../bashcpp/parse.cc"
    break;

  case 50: // redirection: NUMBER LESS_AND '-'
#line 470 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < int64_t > ());
                  REDIRECTEE redir (0);
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_NOFLAGS);
                }
#line 1401 "../bashcpp/parse.cc"
    break;

  case 51: // redirection: REDIR_WORD LESS_AND '-'
#line 477 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (yystack_[2].value.as < WORD_DESC_PTR > ());
                  REDIRECTEE redir (0);
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_close_this, redir, REDIR_VARASSIGN);
                }
#line 1412 "../bashcpp/parse.cc"
    break;

  case 52: // redirection: AND_GREATER WORD
#line 484 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_err_and_out, redir, REDIR_NOFLAGS);
                }
#line 1423 "../bashcpp/parse.cc"
    break;

  case 53: // redirection: AND_GREATER_GREATER WORD
#line 491 "../bashcpp/parse.yy"
                {
                  REDIRECTEE source (1);
                  REDIRECTEE redir (yystack_[0].value.as < WORD_DESC_PTR > ());
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (
                      source, r_append_err_and_out, redir, REDIR_NOFLAGS);
                }
#line 1434 "../bashcpp/parse.cc"
    break;

  case 54: // simple_command_element: WORD
#line 500 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < ELEMENT > ().word = yystack_[0].value.as < WORD_DESC_PTR > ().value;
                  yylhs.value.as < ELEMENT > ().redirect = nullptr;
                }
#line 1443 "../bashcpp/parse.cc"
    break;

  case 55: // simple_command_element: ASSIGNMENT_WORD
#line 505 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < ELEMENT > ().word = yystack_[0].value.as < WORD_DESC_PTR > ().value;
                  yylhs.value.as < ELEMENT > ().redirect = nullptr;
                }
#line 1452 "../bashcpp/parse.cc"
    break;

  case 56: // simple_command_element: redirection
#line 510 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < ELEMENT > ().redirect = yystack_[0].value.as < REDIRECT_PTR > ().value;
                  yylhs.value.as < ELEMENT > ().word = nullptr;
                }
#line 1461 "../bashcpp/parse.cc"
    break;

  case 57: // redirection_list: redirection
#line 517 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < REDIRECT_PTR > () = yystack_[0].value.as < REDIRECT_PTR > ();
                }
#line 1469 "../bashcpp/parse.cc"
    break;

  case 58: // redirection_list: redirection_list redirection
#line 521 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < REDIRECT_PTR > () = REDIRECT_PTR (yystack_[1].value.as < REDIRECT_PTR > ().value->append (yystack_[0].value.as < REDIRECT_PTR > ().value));
                }
#line 1477 "../bashcpp/parse.cc"
    break;

  case 59: // simple_command: simple_command_element
#line 527 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = the_shell->make_simple_command (yystack_[0].value.as < ELEMENT > ());
                }
#line 1485 "../bashcpp/parse.cc"
    break;

  case 60: // simple_command: simple_command simple_command_element
#line 531 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = the_shell->make_simple_command (yystack_[0].value.as < ELEMENT > (), yystack_[1].value.as < COMMAND_PTR > ());
                }
#line 1493 "../bashcpp/parse.cc"
    break;

  case 61: // command: simple_command
#line 537 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = the_shell->clean_simple_command (yystack_[0].value.as < COMMAND_PTR > ());
                }
#line 1501 "../bashcpp/parse.cc"
    break;

  case 62: // command: shell_command
#line 541 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1509 "../bashcpp/parse.cc"
    break;

  case 63: // command: shell_command redirection_list
#line 545 "../bashcpp/parse.yy"
                {
                  COMMAND *tc = yystack_[1].value.as < COMMAND_PTR > ().value;
                  if (tc->redirects)
                    {
                      tc->redirects->append (yystack_[0].value.as < REDIRECT_PTR > ().value);
                    }
                  else
                    {
                      tc->redirects = yystack_[0].value.as < REDIRECT_PTR > ().value;
                    }
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (tc);
                }
#line 1526 "../bashcpp/parse.cc"
    break;

  case 64: // command: function_def
#line 558 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1534 "../bashcpp/parse.cc"
    break;

  case 65: // command: coproc
#line 562 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1542 "../bashcpp/parse.cc"
    break;

  case 66: // shell_command: for_command
#line 568 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1550 "../bashcpp/parse.cc"
    break;

  case 67: // shell_command: case_command
#line 572 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1558 "../bashcpp/parse.cc"
    break;

  case 68: // shell_command: WHILE compound_list DO compound_list DONE
#line 576 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new UNTIL_WHILE_COM (
                      cm_while, yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[1].value.as < COMMAND_PTR > ().value));
                }
#line 1567 "../bashcpp/parse.cc"
    break;

  case 69: // shell_command: UNTIL compound_list DO compound_list DONE
#line 581 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new UNTIL_WHILE_COM (
                      cm_until, yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[1].value.as < COMMAND_PTR > ().value));
                }
#line 1576 "../bashcpp/parse.cc"
    break;

  case 70: // shell_command: select_command
#line 586 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1584 "../bashcpp/parse.cc"
    break;

  case 71: // shell_command: if_command
#line 590 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1592 "../bashcpp/parse.cc"
    break;

  case 72: // shell_command: subshell
#line 594 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1600 "../bashcpp/parse.cc"
    break;

  case 73: // shell_command: group_command
#line 598 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1608 "../bashcpp/parse.cc"
    break;

  case 74: // shell_command: arith_command
#line 602 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1616 "../bashcpp/parse.cc"
    break;

  case 75: // shell_command: cond_command
#line 606 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1624 "../bashcpp/parse.cc"
    break;

  case 76: // shell_command: arith_for_command
#line 610 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 1632 "../bashcpp/parse.cc"
    break;

  case 77: // for_command: FOR WORD newline_list DO compound_list DONE
#line 616 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[4].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1647 "../bashcpp/parse.cc"
    break;

  case 78: // for_command: FOR WORD newline_list '{' compound_list '}'
#line 627 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[4].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1662 "../bashcpp/parse.cc"
    break;

  case 79: // for_command: FOR WORD ';' newline_list DO compound_list DONE
#line 638 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[5].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1677 "../bashcpp/parse.cc"
    break;

  case 80: // for_command: FOR WORD ';' newline_list '{' compound_list '}'
#line 649 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[5].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1692 "../bashcpp/parse.cc"
    break;

  case 81: // for_command: FOR WORD newline_list IN word_list list_terminator newline_list DO compound_list DONE
#line 660 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[8].value.as < WORD_DESC_PTR > ().value,
                      yystack_[5].value.as < WORD_LIST_PTR > ().value->reverse (),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1707 "../bashcpp/parse.cc"
    break;

  case 82: // for_command: FOR WORD newline_list IN word_list list_terminator newline_list '{' compound_list '}'
#line 671 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[8].value.as < WORD_DESC_PTR > ().value,
                      yystack_[5].value.as < WORD_LIST_PTR > ().value->reverse (),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1722 "../bashcpp/parse.cc"
    break;

  case 83: // for_command: FOR WORD newline_list IN list_terminator newline_list DO compound_list DONE
#line 682 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[7].value.as < WORD_DESC_PTR > ().value, nullptr,
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1736 "../bashcpp/parse.cc"
    break;

  case 84: // for_command: FOR WORD newline_list IN list_terminator newline_list '{' compound_list '}'
#line 692 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_for, yystack_[7].value.as < WORD_DESC_PTR > ().value, nullptr,
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1750 "../bashcpp/parse.cc"
    break;

  case 85: // arith_for_command: FOR ARITH_FOR_EXPRS list_terminator newline_list DO compound_list DONE
#line 704 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_arith_for_command (
                      yystack_[5].value.as < WORD_LIST_PTR > (),
                      yystack_[1].value.as < COMMAND_PTR > (), sh.arith_for_lineno);
                  if (yylhs.value.as < COMMAND_PTR > ().value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1765 "../bashcpp/parse.cc"
    break;

  case 86: // arith_for_command: FOR ARITH_FOR_EXPRS list_terminator newline_list '{' compound_list '}'
#line 715 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_arith_for_command (
                      yystack_[5].value.as < WORD_LIST_PTR > (),
                      yystack_[1].value.as < COMMAND_PTR > (), sh.arith_for_lineno);
                  if (yylhs.value.as < COMMAND_PTR > ().value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1780 "../bashcpp/parse.cc"
    break;

  case 87: // arith_for_command: FOR ARITH_FOR_EXPRS DO compound_list DONE
#line 726 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_arith_for_command (
                      yystack_[3].value.as < WORD_LIST_PTR > (),
                      yystack_[1].value.as < COMMAND_PTR > (), sh.arith_for_lineno);
                  if (yylhs.value.as < COMMAND_PTR > ().value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1795 "../bashcpp/parse.cc"
    break;

  case 88: // arith_for_command: FOR ARITH_FOR_EXPRS '{' compound_list '}'
#line 737 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_arith_for_command (
                      yystack_[3].value.as < WORD_LIST_PTR > (),
                      yystack_[1].value.as < COMMAND_PTR > (), sh.arith_for_lineno);
                  if (yylhs.value.as < COMMAND_PTR > ().value == nullptr)
                    YYERROR;
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1810 "../bashcpp/parse.cc"
    break;

  case 89: // select_command: SELECT WORD newline_list DO compound_list DONE
#line 750 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[4].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1825 "../bashcpp/parse.cc"
    break;

  case 90: // select_command: SELECT WORD newline_list '{' compound_list '}'
#line 761 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[4].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1840 "../bashcpp/parse.cc"
    break;

  case 91: // select_command: SELECT WORD ';' newline_list DO compound_list DONE
#line 772 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[5].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1855 "../bashcpp/parse.cc"
    break;

  case 92: // select_command: SELECT WORD ';' newline_list '{' compound_list '}'
#line 783 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[5].value.as < WORD_DESC_PTR > ().value,
                      new WORD_LIST (new WORD_DESC ("\"$@\"")),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1870 "../bashcpp/parse.cc"
    break;

  case 93: // select_command: SELECT WORD newline_list IN word_list list_terminator newline_list DO compound_list DONE
#line 794 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[8].value.as < WORD_DESC_PTR > ().value,
                      yystack_[5].value.as < WORD_LIST_PTR > ().value->reverse (),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1885 "../bashcpp/parse.cc"
    break;

  case 94: // select_command: SELECT WORD newline_list IN word_list list_terminator newline_list '{' compound_list '}'
#line 805 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[8].value.as < WORD_DESC_PTR > ().value,
                      yystack_[5].value.as < WORD_LIST_PTR > ().value->reverse (),
                      yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1900 "../bashcpp/parse.cc"
    break;

  case 95: // select_command: SELECT WORD newline_list IN list_terminator newline_list DO compound_list DONE
#line 816 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[7].value.as < WORD_DESC_PTR > ().value,
                      nullptr, yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1914 "../bashcpp/parse.cc"
    break;

  case 96: // select_command: SELECT WORD newline_list IN list_terminator newline_list '{' compound_list '}'
#line 826 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new FOR_SELECT_COM (
                      cm_select, yystack_[7].value.as < WORD_DESC_PTR > ().value,
                      nullptr, yystack_[1].value.as < COMMAND_PTR > ().value,
                      sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1928 "../bashcpp/parse.cc"
    break;

  case 97: // case_command: CASE WORD newline_list IN newline_list ESAC
#line 838 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CASE_COM (yystack_[4].value.as < WORD_DESC_PTR > ().value, nullptr,
                                    sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1940 "../bashcpp/parse.cc"
    break;

  case 98: // case_command: CASE WORD newline_list IN case_clause_sequence newline_list ESAC
#line 846 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CASE_COM (yystack_[5].value.as < WORD_DESC_PTR > ().value, yystack_[2].value.as < PATTERN_LIST_PTR > ().value,
                                    sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1952 "../bashcpp/parse.cc"
    break;

  case 99: // case_command: CASE WORD newline_list IN case_clause ESAC
#line 854 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CASE_COM (yystack_[4].value.as < WORD_DESC_PTR > ().value, yystack_[1].value.as < PATTERN_LIST_PTR > ().value,
                                    sh.word_lineno[sh.word_top]));
                  if (sh.word_top > 0)
                    sh.word_top--;
                }
#line 1964 "../bashcpp/parse.cc"
    break;

  case 100: // function_def: WORD '(' ')' newline_list function_body
#line 864 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_function_def (
                      yystack_[4].value.as < WORD_DESC_PTR > (), yystack_[0].value.as < COMMAND_PTR > (), sh.function_dstart,
                      sh.function_bstart);
                }
#line 1975 "../bashcpp/parse.cc"
    break;

  case 101: // function_def: FUNCTION WORD '(' ')' newline_list function_body
#line 871 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_function_def (
                      yystack_[4].value.as < WORD_DESC_PTR > (), yystack_[0].value.as < COMMAND_PTR > (), sh.function_dstart,
                      sh.function_bstart);
                }
#line 1986 "../bashcpp/parse.cc"
    break;

  case 102: // function_def: FUNCTION WORD function_body
#line 878 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_function_def (
                      yystack_[1].value.as < WORD_DESC_PTR > (), yystack_[0].value.as < COMMAND_PTR > (), sh.function_dstart,
                      sh.function_bstart);
                }
#line 1997 "../bashcpp/parse.cc"
    break;

  case 103: // function_def: FUNCTION WORD '\n' newline_list function_body
#line 885 "../bashcpp/parse.yy"
                {
                  Shell &sh = *the_shell;
                  yylhs.value.as < COMMAND_PTR > () = sh.make_function_def (
                      yystack_[3].value.as < WORD_DESC_PTR > (), yystack_[0].value.as < COMMAND_PTR > (), sh.function_dstart,
                      sh.function_bstart);
                }
#line 2008 "../bashcpp/parse.cc"
    break;

  case 104: // function_body: shell_command
#line 894 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2016 "../bashcpp/parse.cc"
    break;

  case 105: // function_body: shell_command redirection_list
#line 898 "../bashcpp/parse.yy"
                {
                  COMMAND *tc = yystack_[1].value.as < COMMAND_PTR > ().value;

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
                    tc->redirects->append (yystack_[0].value.as < REDIRECT_PTR > ().value);
                  else
                    tc->redirects = yystack_[0].value.as < REDIRECT_PTR > ().value;

                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (yystack_[1].value.as < COMMAND_PTR > ());
                }
#line 2044 "../bashcpp/parse.cc"
    break;

  case 106: // subshell: '(' compound_list ')'
#line 924 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new SUBSHELL_COM (yystack_[1].value.as < COMMAND_PTR > ().value));
                }
#line 2052 "../bashcpp/parse.cc"
    break;

  case 107: // comsub: DOLPAREN compound_list ')'
#line 930 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[1].value.as < COMMAND_PTR > ();
                }
#line 2060 "../bashcpp/parse.cc"
    break;

  case 108: // comsub: DOLPAREN newline_list ')'
#line 934 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = nullptr;
                }
#line 2068 "../bashcpp/parse.cc"
    break;

  case 109: // coproc: COPROC shell_command
#line 940 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new COPROC_COM ("COPROC", yystack_[0].value.as < COMMAND_PTR > ().value));
                }
#line 2076 "../bashcpp/parse.cc"
    break;

  case 110: // coproc: COPROC shell_command redirection_list
#line 944 "../bashcpp/parse.yy"
                {
                  COMMAND *tc = yystack_[1].value.as < COMMAND_PTR > ().value;

                  if (tc->redirects)
                    tc->redirects->append (yystack_[0].value.as < REDIRECT_PTR > ().value);
                  else
                    tc->redirects = yystack_[0].value.as < REDIRECT_PTR > ().value;

                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new COPROC_COM ("COPROC", tc));
                }
#line 2091 "../bashcpp/parse.cc"
    break;

  case 111: // coproc: COPROC WORD shell_command
#line 955 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new COPROC_COM (yystack_[1].value.as < WORD_DESC_PTR > ().value->word, yystack_[0].value.as < COMMAND_PTR > ().value));
                }
#line 2099 "../bashcpp/parse.cc"
    break;

  case 112: // coproc: COPROC WORD shell_command redirection_list
#line 959 "../bashcpp/parse.yy"
                {
                  COMMAND *tc = yystack_[1].value.as < COMMAND_PTR > ().value;

                  if (tc->redirects)
                    tc->redirects->append (yystack_[0].value.as < REDIRECT_PTR > ().value);
                  else
                    tc->redirects = yystack_[0].value.as < REDIRECT_PTR > ().value;

                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new COPROC_COM (yystack_[2].value.as < WORD_DESC_PTR > ().value->word, tc));
                }
#line 2114 "../bashcpp/parse.cc"
    break;

  case 113: // coproc: COPROC simple_command
#line 970 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new COPROC_COM (
                      "COPROC", the_shell->clean_simple_command (yystack_[0].value.as < COMMAND_PTR > ()).value));
                }
#line 2123 "../bashcpp/parse.cc"
    break;

  case 114: // if_command: IF compound_list THEN compound_list FI
#line 977 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new IF_COM (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[1].value.as < COMMAND_PTR > ().value, nullptr));
                }
#line 2131 "../bashcpp/parse.cc"
    break;

  case 115: // if_command: IF compound_list THEN compound_list ELSE compound_list FI
#line 981 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new IF_COM (yystack_[5].value.as < COMMAND_PTR > ().value, yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[1].value.as < COMMAND_PTR > ().value));
                }
#line 2139 "../bashcpp/parse.cc"
    break;

  case 116: // if_command: IF compound_list THEN compound_list elif_clause FI
#line 985 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new IF_COM (yystack_[4].value.as < COMMAND_PTR > ().value, yystack_[2].value.as < COMMAND_PTR > ().value, yystack_[1].value.as < COMMAND_PTR > ().value));
                }
#line 2147 "../bashcpp/parse.cc"
    break;

  case 117: // group_command: '{' compound_list '}'
#line 992 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new GROUP_COM (yystack_[1].value.as < COMMAND_PTR > ().value));
                }
#line 2155 "../bashcpp/parse.cc"
    break;

  case 118: // arith_command: ARITH_CMD
#line 998 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new ARITH_COM (yystack_[0].value.as < WORD_LIST_PTR > ().value));
                }
#line 2163 "../bashcpp/parse.cc"
    break;

  case 119: // cond_command: COND_START COND_CMD COND_END
#line 1004 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[1].value.as < COMMAND_PTR > ();
                }
#line 2171 "../bashcpp/parse.cc"
    break;

  case 120: // elif_clause: ELIF compound_list THEN compound_list
#line 1010 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new IF_COM (yystack_[2].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value, nullptr));
                }
#line 2179 "../bashcpp/parse.cc"
    break;

  case 121: // elif_clause: ELIF compound_list THEN compound_list ELSE compound_list
#line 1014 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new IF_COM (yystack_[4].value.as < COMMAND_PTR > ().value, yystack_[2].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value));
                }
#line 2187 "../bashcpp/parse.cc"
    break;

  case 122: // elif_clause: ELIF compound_list THEN compound_list elif_clause
#line 1018 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new IF_COM (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[1].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value));
                }
#line 2195 "../bashcpp/parse.cc"
    break;

  case 123: // case_clause: pattern_list
#line 1023 "../bashcpp/parse.yy"
             { yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[0].value.as < PATTERN_LIST_PTR > (); }
#line 2201 "../bashcpp/parse.cc"
    break;

  case 124: // case_clause: case_clause_sequence pattern_list
#line 1025 "../bashcpp/parse.yy"
                {
                  yystack_[0].value.as < PATTERN_LIST_PTR > ().value->set_next (yystack_[1].value.as < PATTERN_LIST_PTR > ().value);
                  yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[0].value.as < PATTERN_LIST_PTR > ();
                }
#line 2210 "../bashcpp/parse.cc"
    break;

  case 125: // pattern_list: newline_list pattern ')' compound_list
#line 1032 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < PATTERN_LIST_PTR > () = PATTERN_LIST_PTR (
                        new PATTERN_LIST (yystack_[2].value.as < WORD_LIST_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value));
                }
#line 2219 "../bashcpp/parse.cc"
    break;

  case 126: // pattern_list: newline_list pattern ')' newline_list
#line 1037 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < PATTERN_LIST_PTR > () = PATTERN_LIST_PTR (
                        new PATTERN_LIST (yystack_[2].value.as < WORD_LIST_PTR > ().value, nullptr));
                }
#line 2228 "../bashcpp/parse.cc"
    break;

  case 127: // pattern_list: newline_list '(' pattern ')' compound_list
#line 1042 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < PATTERN_LIST_PTR > () = PATTERN_LIST_PTR (
                        new PATTERN_LIST (yystack_[2].value.as < WORD_LIST_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value));
                }
#line 2237 "../bashcpp/parse.cc"
    break;

  case 128: // pattern_list: newline_list '(' pattern ')' newline_list
#line 1047 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < PATTERN_LIST_PTR > () = PATTERN_LIST_PTR (
                        new PATTERN_LIST (yystack_[2].value.as < WORD_LIST_PTR > ().value, nullptr));
                }
#line 2246 "../bashcpp/parse.cc"
    break;

  case 129: // case_clause_sequence: pattern_list SEMI_SEMI
#line 1054 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[1].value.as < PATTERN_LIST_PTR > ();
                }
#line 2254 "../bashcpp/parse.cc"
    break;

  case 130: // case_clause_sequence: case_clause_sequence pattern_list SEMI_SEMI
#line 1058 "../bashcpp/parse.yy"
                {
                  yystack_[1].value.as < PATTERN_LIST_PTR > ().value->set_next (yystack_[2].value.as < PATTERN_LIST_PTR > ().value);
                  yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[1].value.as < PATTERN_LIST_PTR > ();
                }
#line 2263 "../bashcpp/parse.cc"
    break;

  case 131: // case_clause_sequence: pattern_list SEMI_AND
#line 1063 "../bashcpp/parse.yy"
                {
                  yystack_[1].value.as < PATTERN_LIST_PTR > ().value->flags |= CASEPAT_FALLTHROUGH;
                  yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[1].value.as < PATTERN_LIST_PTR > ();
                }
#line 2272 "../bashcpp/parse.cc"
    break;

  case 132: // case_clause_sequence: case_clause_sequence pattern_list SEMI_AND
#line 1068 "../bashcpp/parse.yy"
                {
                  yystack_[1].value.as < PATTERN_LIST_PTR > ().value->flags |= CASEPAT_FALLTHROUGH;
                  yystack_[1].value.as < PATTERN_LIST_PTR > ().value->set_next (yystack_[2].value.as < PATTERN_LIST_PTR > ().value);
                  yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[1].value.as < PATTERN_LIST_PTR > ();
                }
#line 2282 "../bashcpp/parse.cc"
    break;

  case 133: // case_clause_sequence: pattern_list SEMI_SEMI_AND
#line 1074 "../bashcpp/parse.yy"
                {
                  yystack_[1].value.as < PATTERN_LIST_PTR > ().value->flags |= CASEPAT_TESTNEXT;
                  yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[1].value.as < PATTERN_LIST_PTR > ();
                }
#line 2291 "../bashcpp/parse.cc"
    break;

  case 134: // case_clause_sequence: case_clause_sequence pattern_list SEMI_SEMI_AND
#line 1079 "../bashcpp/parse.yy"
                {
                  yystack_[1].value.as < PATTERN_LIST_PTR > ().value->flags |= CASEPAT_TESTNEXT;
                  yystack_[1].value.as < PATTERN_LIST_PTR > ().value->set_next (yystack_[2].value.as < PATTERN_LIST_PTR > ().value);
                  yylhs.value.as < PATTERN_LIST_PTR > () = yystack_[1].value.as < PATTERN_LIST_PTR > ();
                }
#line 2301 "../bashcpp/parse.cc"
    break;

  case 135: // pattern: WORD
#line 1087 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < WORD_LIST_PTR > () = WORD_LIST_PTR (yystack_[0].value.as < WORD_DESC_PTR > ());
                }
#line 2309 "../bashcpp/parse.cc"
    break;

  case 136: // pattern: pattern '|' WORD
#line 1091 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < WORD_LIST_PTR > () = WORD_LIST_PTR (yystack_[0].value.as < WORD_DESC_PTR > (), yystack_[2].value.as < WORD_LIST_PTR > ());
                }
#line 2317 "../bashcpp/parse.cc"
    break;

  case 137: // compound_list: newline_list list0
#line 1102 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                  if (the_shell->need_here_doc && the_shell->last_read_token == '\n')
                    the_shell->gather_here_documents ();
                }
#line 2327 "../bashcpp/parse.cc"
    break;

  case 138: // compound_list: newline_list list1
#line 1108 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2335 "../bashcpp/parse.cc"
    break;

  case 139: // list0: list1 '\n' newline_list
#line 1113 "../bashcpp/parse.yy"
       { yylhs.value.as < COMMAND_PTR > () = yystack_[2].value.as < COMMAND_PTR > (); }
#line 2341 "../bashcpp/parse.cc"
    break;

  case 140: // list0: list1 '&' newline_list
#line 1115 "../bashcpp/parse.yy"
                {
                  COMMAND *cmd = yystack_[2].value.as < COMMAND_PTR > ().value;
                  if (cmd->type == cm_connection)
                    yylhs.value.as < COMMAND_PTR > () = connect_async_list (cmd, nullptr, '&');
                  else
                    yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (cmd, nullptr, '&'));
                }
#line 2353 "../bashcpp/parse.cc"
    break;

  case 141: // list0: list1 ';' newline_list
#line 1122 "../bashcpp/parse.yy"
          { yylhs.value.as < COMMAND_PTR > () = yystack_[2].value.as < COMMAND_PTR > (); }
#line 2359 "../bashcpp/parse.cc"
    break;

  case 142: // list1: list1 AND_AND newline_list list1
#line 1126 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value,
                                        static_cast<int> (token::AND_AND)));
                }
#line 2368 "../bashcpp/parse.cc"
    break;

  case 143: // list1: list1 OR_OR newline_list list1
#line 1131 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value,
                                        static_cast<int> (token::OR_OR)));
                }
#line 2377 "../bashcpp/parse.cc"
    break;

  case 144: // list1: list1 '&' newline_list list1
#line 1136 "../bashcpp/parse.yy"
                {
                  COMMAND *cmd = yystack_[3].value.as < COMMAND_PTR > ().value;
                  if (cmd->type == cm_connection)
                    yylhs.value.as < COMMAND_PTR > () = connect_async_list (cmd, yystack_[0].value.as < COMMAND_PTR > ().value, '&');
                  else
                    yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (cmd, yystack_[0].value.as < COMMAND_PTR > ().value, '&'));
                }
#line 2389 "../bashcpp/parse.cc"
    break;

  case 145: // list1: list1 ';' newline_list list1
#line 1144 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value, ';'));
                }
#line 2397 "../bashcpp/parse.cc"
    break;

  case 146: // list1: list1 '\n' newline_list list1
#line 1148 "../bashcpp/parse.yy"
                {
                  if (the_shell->parser_state & PST_CMDSUBST)
                    yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value, '\n'));
                  else
                    yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value, ';'));
                }
#line 2408 "../bashcpp/parse.cc"
    break;

  case 147: // list1: pipeline_command
#line 1155 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2416 "../bashcpp/parse.cc"
    break;

  case 150: // list_terminator: '\n'
#line 1165 "../bashcpp/parse.yy"
                { yylhs.value.as < int64_t > () = '\n'; }
#line 2422 "../bashcpp/parse.cc"
    break;

  case 151: // list_terminator: ';'
#line 1167 "../bashcpp/parse.yy"
                { yylhs.value.as < int64_t > () = ';'; }
#line 2428 "../bashcpp/parse.cc"
    break;

  case 152: // list_terminator: yacc_EOF
#line 1169 "../bashcpp/parse.yy"
                { yylhs.value.as < int64_t > () = token::yacc_EOF; }
#line 2434 "../bashcpp/parse.cc"
    break;

  case 155: // simple_list: simple_list1
#line 1183 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                  Shell &sh = *the_shell;
                  if (sh.need_here_doc)
                    sh.gather_here_documents (); /* XXX */
                  if ((sh.parser_state & PST_CMDSUBST)
                      && sh.current_token == sh.shell_eof_token)
                    {
        sh.internal_debug ("LEGACY: parser: command substitution simple_list1 -> simple_list");
                      sh.global_command = yystack_[0].value.as < COMMAND_PTR > ().value;
                      sh.eof_encountered = 0;
                      if (sh.bash_input.type == st_string)
                        sh.rewind_input_string ();
                      YYACCEPT;
                    }
                }
#line 2455 "../bashcpp/parse.cc"
    break;

  case 156: // simple_list: simple_list1 '&'
#line 1200 "../bashcpp/parse.yy"
                {
                  COMMAND *cmd = yystack_[1].value.as < COMMAND_PTR > ().value;
                  if (cmd->type == cm_connection)
                    yylhs.value.as < COMMAND_PTR > () = connect_async_list (cmd, nullptr, '&');
                  else
                    yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (cmd, nullptr, '&'));
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
#line 2480 "../bashcpp/parse.cc"
    break;

  case 157: // simple_list: simple_list1 ';'
#line 1221 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[1].value.as < COMMAND_PTR > ();
                  Shell &sh = *the_shell;
                  if (sh.need_here_doc)
                    sh.gather_here_documents (); /* XXX */
                  if ((sh.parser_state & PST_CMDSUBST)
                      && sh.current_token == sh.shell_eof_token)
                    {
        sh.internal_debug ("LEGACY: parser: command substitution simple_list1 ';' -> simple_list");
                      sh.global_command = yystack_[1].value.as < COMMAND_PTR > ().value;
                      sh.eof_encountered = 0;
                      if (sh.bash_input.type == st_string)
                        sh.rewind_input_string ();
                      YYACCEPT;
                    }
                }
#line 2501 "../bashcpp/parse.cc"
    break;

  case 158: // simple_list1: simple_list1 AND_AND newline_list simple_list1
#line 1240 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value,
                                        static_cast<int> (token::AND_AND)));
                }
#line 2510 "../bashcpp/parse.cc"
    break;

  case 159: // simple_list1: simple_list1 OR_OR newline_list simple_list1
#line 1245 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value,
                                        static_cast<int> (token::OR_OR)));
                }
#line 2519 "../bashcpp/parse.cc"
    break;

  case 160: // simple_list1: simple_list1 '&' simple_list1
#line 1250 "../bashcpp/parse.yy"
                {
                  COMMAND *cmd = yystack_[2].value.as < COMMAND_PTR > ().value;
                  if (cmd->type == cm_connection)
                    yylhs.value.as < COMMAND_PTR > () = connect_async_list (cmd, yystack_[0].value.as < COMMAND_PTR > ().value, '&');
                  else
                    yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (cmd, yystack_[0].value.as < COMMAND_PTR > ().value, '&'));
                }
#line 2531 "../bashcpp/parse.cc"
    break;

  case 161: // simple_list1: simple_list1 ';' simple_list1
#line 1258 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[2].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value, ';'));
                }
#line 2539 "../bashcpp/parse.cc"
    break;

  case 162: // simple_list1: pipeline_command
#line 1262 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2547 "../bashcpp/parse.cc"
    break;

  case 163: // pipeline_command: pipeline
#line 1268 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2555 "../bashcpp/parse.cc"
    break;

  case 164: // pipeline_command: BANG pipeline_command
#line 1272 "../bashcpp/parse.yy"
                {
                  if (yystack_[0].value.as < COMMAND_PTR > ().value)
                    yystack_[0].value.as < COMMAND_PTR > ().value->flags ^= CMD_INVERT_RETURN; /* toggle */
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2565 "../bashcpp/parse.cc"
    break;

  case 165: // pipeline_command: timespec pipeline_command
#line 1278 "../bashcpp/parse.yy"
                {
                  if (yystack_[0].value.as < COMMAND_PTR > ().value)
                    yystack_[0].value.as < COMMAND_PTR > ().value->flags |= static_cast<cmd_flags> (yystack_[1].value.as < int64_t > ());

                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2576 "../bashcpp/parse.cc"
    break;

  case 166: // pipeline_command: timespec list_terminator
#line 1285 "../bashcpp/parse.yy"
                {
                  ELEMENT x;

                  /* Boy, this is unclean.  `time' by itself can
                     time a null command.  We cheat and push a
                     newline back if the list_terminator was a newline
                     to avoid the double-newline problem (one to
                     terminate this, one to terminate the command) */
                  x.word = nullptr;
                  x.redirect = nullptr;
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new SIMPLE_COM (x, nullptr));
                  yylhs.value.as < COMMAND_PTR > ().value->flags |= static_cast<cmd_flags> (yystack_[1].value.as < int64_t > ());

                  /* XXX - let's cheat and push a newline back */
                  Shell &sh = *the_shell;
                  if (yystack_[0].value.as < int64_t > () == '\n')
                    sh.token_to_read = static_cast<parser::token_kind_type> ('\n');
                  else if (yystack_[0].value.as < int64_t > () == ';')
                    sh.token_to_read = static_cast<parser::token_kind_type> (';');
                  sh.parser_state
                      &= ~PST_REDIRLIST; /* SIMPLE_COM constructor sets this */
                }
#line 2603 "../bashcpp/parse.cc"
    break;

  case 167: // pipeline_command: BANG list_terminator
#line 1308 "../bashcpp/parse.yy"
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
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new SIMPLE_COM (x, nullptr));
                  yylhs.value.as < COMMAND_PTR > ().value->flags |= CMD_INVERT_RETURN;
                  /* XXX - let's cheat and push a newline back */
                  Shell &sh = *the_shell;
                  if (yystack_[0].value.as < int64_t > () == '\n')
                    sh.token_to_read = static_cast<parser::token_kind_type> ('\n');
                  if (yystack_[0].value.as < int64_t > () == ';')
                    sh.token_to_read = static_cast<parser::token_kind_type> (';');
                  sh.parser_state
                      &= ~PST_REDIRLIST; /* SIMPLE_COM constructor sets this */
                }
#line 2630 "../bashcpp/parse.cc"
    break;

  case 168: // pipeline: pipeline '|' newline_list pipeline
#line 1333 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value, '|'));
                }
#line 2638 "../bashcpp/parse.cc"
    break;

  case 169: // pipeline: pipeline BAR_AND newline_list pipeline
#line 1337 "../bashcpp/parse.yy"
                {
                  /* Make cmd1 |& cmd2 equivalent to cmd1 2>&1 | cmd2 */
                  COMMAND *tc;
                  REDIRECT *r;
                  REDIRECT **rp; // pointer to "redirects" to update

                  tc = yystack_[3].value.as < COMMAND_PTR > ().value;
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

                  yylhs.value.as < COMMAND_PTR > () = COMMAND_PTR (new CONNECTION (yystack_[3].value.as < COMMAND_PTR > ().value, yystack_[0].value.as < COMMAND_PTR > ().value, '|'));
                }
#line 2667 "../bashcpp/parse.cc"
    break;

  case 170: // pipeline: command
#line 1362 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < COMMAND_PTR > () = yystack_[0].value.as < COMMAND_PTR > ();
                }
#line 2675 "../bashcpp/parse.cc"
    break;

  case 171: // timespec: TIME
#line 1368 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < int64_t > () = CMD_TIME_PIPELINE;
                }
#line 2683 "../bashcpp/parse.cc"
    break;

  case 172: // timespec: TIME TIMEOPT
#line 1372 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < int64_t > () = (CMD_TIME_PIPELINE | CMD_TIME_POSIX);
                }
#line 2691 "../bashcpp/parse.cc"
    break;

  case 173: // timespec: TIME TIMEIGN
#line 1376 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < int64_t > () = (CMD_TIME_PIPELINE | CMD_TIME_POSIX);
                }
#line 2699 "../bashcpp/parse.cc"
    break;

  case 174: // timespec: TIME TIMEOPT TIMEIGN
#line 1380 "../bashcpp/parse.yy"
                {
                  yylhs.value.as < int64_t > () = (CMD_TIME_PIPELINE | CMD_TIME_POSIX);
                }
#line 2707 "../bashcpp/parse.cc"
    break;


#line 2711 "../bashcpp/parse.cc"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        std::string msg = YY_("syntax error");
        error (YY_MOVE (msg));
      }


    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;


      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.what ());
  }

#if YYDEBUG || 0
  const char *
  parser::symbol_name (symbol_kind_type yysymbol)
  {
    return yytname_[yysymbol];
  }
#endif // #if YYDEBUG || 0









  const short parser::yypact_ninf_ = -152;

  const signed char parser::yytable_ninf_ = -1;

  const short
  parser::yypact_[] =
  {
     328,    80,  -152,   -11,    -1,     3,  -152,  -152,    15,   637,
      -5,   433,   149,   -28,  -152,   187,   684,  -152,    18,    28,
     130,    38,   139,    50,    52,    60,    65,    74,  -152,  -152,
    -152,    89,   104,  -152,  -152,    97,  -152,  -152,   246,  -152,
     670,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,
    -152,  -152,  -152,   146,   211,  -152,     1,   433,  -152,  -152,
     135,   484,  -152,    59,    61,    90,   167,   171,    10,    71,
     246,   670,   144,  -152,  -152,  -152,  -152,  -152,   165,  -152,
     142,   179,   192,   140,   194,   160,   227,   245,   252,   253,
     260,   261,   262,   162,   269,   178,   270,   272,   273,   274,
     277,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,
    -152,  -152,  -152,  -152,  -152,   168,   379,  -152,  -152,   173,
     244,  -152,  -152,  -152,  -152,   670,  -152,  -152,  -152,  -152,
    -152,   535,   535,  -152,  -152,  -152,  -152,  -152,  -152,  -152,
     205,  -152,    14,  -152,    36,  -152,  -152,  -152,  -152,    84,
    -152,  -152,  -152,   249,   670,  -152,   670,   670,  -152,  -152,
    -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,
    -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,
    -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,  -152,
    -152,  -152,  -152,  -152,   484,   484,   203,   203,   586,   586,
     145,  -152,  -152,  -152,  -152,  -152,  -152,     0,  -152,   119,
    -152,   291,   248,    66,    88,  -152,   119,  -152,   296,   297,
      35,  -152,   670,   670,    35,  -152,  -152,     1,     1,  -152,
    -152,  -152,   306,   484,   484,   484,   484,   484,   305,   169,
    -152,     7,  -152,  -152,   302,  -152,   131,  -152,   265,  -152,
    -152,  -152,  -152,  -152,  -152,   304,   131,  -152,   266,  -152,
    -152,  -152,    35,  -152,   313,   317,  -152,  -152,  -152,   225,
     225,   225,  -152,  -152,  -152,  -152,   206,    25,  -152,  -152,
     307,   -42,   319,   276,  -152,  -152,  -152,    95,  -152,   322,
     283,   332,   284,  -152,  -152,   102,  -152,  -152,  -152,  -152,
    -152,  -152,  -152,  -152,    45,   323,  -152,  -152,  -152,   106,
    -152,  -152,  -152,  -152,  -152,  -152,   109,  -152,  -152,   264,
    -152,  -152,  -152,   484,  -152,  -152,   333,   293,  -152,  -152,
     338,   300,  -152,  -152,  -152,   484,   345,   303,  -152,  -152,
     346,   309,  -152,  -152,  -152,  -152,  -152,  -152,  -152
  };

  const unsigned char
  parser::yydefact_[] =
  {
       0,     0,   153,     0,     0,     0,   153,   153,     0,     0,
       0,     0,   171,    54,    55,     0,     0,   118,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   153,     4,
       7,     0,     0,   153,   153,     0,    56,    59,    61,   170,
      62,    66,    76,    70,    67,    64,    72,     3,    65,    71,
      73,    74,    75,     0,   155,   162,   163,     0,     5,     6,
       0,     0,   153,   153,     0,   153,     0,     0,     0,    54,
     113,   109,     0,   151,   150,   152,   167,   164,   172,   173,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    16,    25,    40,    34,    49,    31,    43,    37,    46,
      28,    52,    53,    22,    19,     0,     0,    10,    11,     0,
       0,     1,    54,    60,    57,    63,   148,   149,     2,   153,
     153,   156,   157,   153,   153,   166,   165,   153,   154,   137,
     138,   147,     0,   153,     0,   153,   153,   153,   153,     0,
     153,   153,   153,   153,   104,   102,   111,   110,   119,   174,
     153,    18,    27,    42,    36,    51,    33,    45,    39,    48,
      30,    24,    21,    14,    15,    17,    26,    41,    35,    50,
      32,    44,    38,    47,    29,    23,    20,    12,    13,   107,
     108,   117,   106,    58,     0,     0,   160,   161,     0,     0,
       0,   153,   153,   153,   153,   153,   153,     0,   153,     0,
     153,     0,     0,     0,     0,   153,     0,   153,     0,     0,
       0,   153,   105,   112,     0,   158,   159,   169,   168,   153,
     153,   114,     0,     0,     0,   140,   141,   139,     0,   123,
     153,     0,   153,   153,     0,     8,     0,   153,     0,    87,
      88,   153,   153,   153,   153,     0,     0,   153,     0,    68,
      69,   103,     0,   100,     0,     0,   116,   142,   143,   144,
     145,   146,    99,   129,   131,   133,   124,     0,    97,   135,
       0,     0,     0,     0,    77,     9,   153,     0,    78,     0,
       0,     0,     0,    89,   153,     0,    90,   101,   115,   153,
     130,   132,   134,    98,     0,     0,   153,    79,    80,     0,
     153,   153,    85,    86,    91,    92,     0,   153,   153,   120,
     153,   136,   125,   126,   153,   153,     0,     0,   153,   153,
       0,     0,   153,   122,   127,   128,     0,     0,    83,    84,
       0,     0,    95,    96,   121,    81,    82,    93,    94
  };

  const short
  parser::yypgoto_[] =
  {
    -152,  -152,   112,   -29,   -14,   -64,   360,  -152,    -8,  -152,
    -152,  -152,  -152,  -152,  -151,  -152,  -152,  -152,  -152,  -152,
    -152,  -152,    13,  -152,   136,  -152,    98,    -2,  -152,    30,
    -152,   -54,   -26,  -152,  -123,     6,    78,  -152
  };

  const short
  parser::yydefgoto_[] =
  {
       0,    35,   246,    36,    37,   125,    38,    39,    40,    41,
      42,    43,    44,    45,   155,    46,    47,    48,    49,    50,
      51,    52,   232,   238,   239,   240,   281,   120,   139,   140,
     128,    76,    61,    53,    54,   141,    56,    57
  };

  const short
  parser::yytable_[] =
  {
      60,    71,   116,   135,    66,    67,    55,   157,   196,   197,
     147,   124,   305,     2,   242,    62,   278,    77,     3,   306,
       4,     5,     6,     7,   123,    63,   115,    72,    10,    65,
      64,   119,    80,   279,   303,   206,   142,   144,     2,   149,
      17,    68,   124,     3,   101,     4,     5,     6,     7,   133,
     208,   279,   138,    10,   102,   134,   123,   209,   243,   138,
     154,   156,   152,   136,   106,    17,   138,   280,    33,   261,
     153,   225,   226,   263,     2,   145,   110,   138,   111,     3,
     251,     4,     5,     6,     7,   280,   112,   138,   138,    10,
     222,   113,   223,    33,   210,    34,   193,   121,   215,   305,
     114,    17,   253,   194,   195,   216,   320,   198,   199,   310,
     143,   297,    73,    74,    75,   117,   317,   207,   138,   146,
     324,   213,   214,   328,   252,   124,   220,   124,   193,    33,
     118,    34,    58,    59,   224,   200,   138,    55,    55,   137,
     138,   148,   217,   211,   212,   245,   254,   138,   218,   219,
     229,   230,   231,   311,   138,   247,   103,   285,   138,   104,
     318,   138,   257,   158,   325,   107,   163,   329,   108,   164,
      73,    74,    75,    78,    79,   233,   234,   235,   236,   237,
     241,   150,    73,    74,    75,   151,   167,   105,   177,   168,
     159,   178,   286,   193,   193,   262,   109,   165,   126,   127,
      55,    55,   294,   160,   181,   161,   244,   182,   248,   273,
     274,   275,   154,   255,   277,   258,   154,   169,   162,   179,
     166,   287,    81,    82,    83,    84,    85,   264,   265,   189,
      86,   295,   191,    87,    88,   183,   129,   130,   201,   202,
     282,   283,    89,    90,   129,   130,   300,   301,   302,   289,
     290,   291,   292,   170,   154,   203,   204,   205,   201,   202,
     309,   131,   132,   267,   268,   269,   270,   271,   316,   332,
     230,   171,   122,    14,    15,    16,   227,   228,   172,   173,
     323,    18,    19,    20,    21,    22,   174,   175,   176,    23,
      24,    25,    26,    27,   335,   180,   184,   319,   185,   186,
     187,    31,    32,   188,   322,   192,   249,   250,   326,   327,
     221,   259,   260,   266,   272,   330,   331,   284,   334,   293,
     298,   299,   336,   337,   288,   296,   340,   341,   256,     1,
     344,     2,   333,   279,   307,   308,     3,   312,     4,     5,
       6,     7,   313,   315,     8,     9,    10,   314,   338,   321,
      11,    12,   339,   342,    13,    14,    15,    16,    17,   343,
     345,   347,   346,    18,    19,    20,    21,    22,   348,    70,
       0,    23,    24,    25,    26,    27,   276,    28,   304,     0,
      29,    30,     2,    31,    32,     0,    33,     3,    34,     4,
       5,     6,     7,     0,     0,     8,     9,    10,     0,     0,
       0,    11,    12,     0,     0,    13,    14,    15,    16,    17,
       0,     0,     0,     0,    18,    19,    20,    21,    22,     0,
       0,     0,    23,    24,    25,    26,    27,     0,     0,     0,
       0,   138,     0,     0,    31,    32,     2,    33,     0,    34,
     190,     3,     0,     4,     5,     6,     7,     0,     0,     8,
       9,    10,     0,     0,     0,    11,    12,     0,     0,    13,
      14,    15,    16,    17,     0,     0,     0,     0,    18,    19,
      20,    21,    22,     0,     0,     0,    23,    24,    25,    26,
      27,     0,     0,     0,    73,    74,    75,     2,    31,    32,
       0,    33,     3,    34,     4,     5,     6,     7,     0,     0,
       8,     9,    10,     0,     0,     0,    11,    12,     0,     0,
      13,    14,    15,    16,    17,     0,     0,     0,     0,    18,
      19,    20,    21,    22,     0,     0,     0,    23,    24,    25,
      26,    27,     0,     0,     0,     0,   138,     0,     2,    31,
      32,     0,    33,     3,    34,     4,     5,     6,     7,     0,
       0,     8,     9,    10,     0,     0,     0,    11,    12,     0,
       0,    13,    14,    15,    16,    17,     0,     0,     0,     0,
      18,    19,    20,    21,    22,     0,     0,     0,    23,    24,
      25,    26,    27,     0,     0,     0,     0,     0,     0,     2,
      31,    32,     0,    33,     3,    34,     4,     5,     6,     7,
       0,     0,     8,     9,    10,     0,     0,     0,     0,     0,
       0,     0,    13,    14,    15,    16,    17,     0,     0,     0,
       0,    18,    19,    20,    21,    22,     0,     0,     0,    23,
      24,    25,    26,    27,     0,     0,     0,     0,   138,     0,
       2,    31,    32,     0,    33,     3,    34,     4,     5,     6,
       7,     0,     0,     0,     0,    10,     0,     0,     0,     0,
       0,     0,     0,    69,    14,    15,    16,    17,     0,     0,
       0,     0,    18,    19,    20,    21,    22,     0,     0,     0,
      23,    24,    25,    26,    27,     0,     0,     0,     0,     0,
       0,     0,    31,    32,     0,    33,     0,    34,    15,    16,
       0,     0,     0,     0,     0,    18,    19,    20,    21,    22,
       0,     0,     0,    23,    24,    25,    26,    27,     0,    91,
      92,    93,    94,    95,     0,    31,    32,    96,     0,     0,
      97,    98,     0,     0,     0,     0,     0,     0,     0,    99,
     100
  };

  const short
  parser::yycheck_[] =
  {
       2,     9,    28,    57,     6,     7,     0,    71,   131,   132,
      64,    40,    54,     3,    14,    26,     9,    11,     8,    61,
      10,    11,    12,    13,    38,    26,    28,    32,    18,    26,
      31,    33,    60,    26,     9,    21,    62,    63,     3,    65,
      30,    26,    71,     8,    26,    10,    11,    12,    13,    48,
      14,    26,    52,    18,    26,    54,    70,    21,    58,    52,
      68,    69,    52,    57,    26,    30,    52,    60,    58,   220,
      60,   194,   195,   224,     3,    14,    26,    52,    26,     8,
      14,    10,    11,    12,    13,    60,    26,    52,    52,    18,
     154,    26,   156,    58,    58,    60,   125,     0,    14,    54,
      26,    30,    14,   129,   130,    21,    61,   133,   134,    14,
      51,   262,    51,    52,    53,    26,    14,   143,    52,    58,
      14,   147,   148,    14,    58,   154,   152,   156,   157,    58,
      26,    60,    52,    53,   160,   137,    52,   131,   132,     4,
      52,    51,    58,   145,   146,    26,    58,    52,   150,   151,
       5,     6,     7,    58,    52,   209,    26,    26,    52,    29,
      58,    52,   216,    19,    58,    26,    26,    58,    29,    29,
      51,    52,    53,    24,    25,   201,   202,   203,   204,   205,
     206,    14,    51,    52,    53,    14,    26,    57,    26,    29,
      25,    29,   246,   222,   223,   221,    57,    57,    52,    53,
     194,   195,   256,    61,    26,    26,   208,    29,   210,    40,
      41,    42,   220,   215,   240,   217,   224,    57,    26,    57,
      26,   247,    35,    36,    37,    38,    39,   229,   230,    61,
      43,   257,    59,    46,    47,    57,    33,    34,    33,    34,
     242,   243,    55,    56,    33,    34,    40,    41,    42,   251,
     252,   253,   254,    26,   262,    50,    51,    52,    33,    34,
     286,    50,    51,   233,   234,   235,   236,   237,   294,     5,
       6,    26,    26,    27,    28,    29,   198,   199,    26,    26,
     306,    35,    36,    37,    38,    39,    26,    26,    26,    43,
      44,    45,    46,    47,   320,    26,    26,   299,    26,    26,
      26,    55,    56,    26,   306,    61,    15,    59,   310,   311,
      61,    15,    15,     7,     9,   317,   318,    15,   320,    15,
       7,     4,   324,   325,    59,    59,   328,   329,   216,     1,
     332,     3,   319,    26,    15,    59,     8,    15,    10,    11,
      12,    13,    59,    59,    16,    17,    18,    15,    15,    26,
      22,    23,    59,    15,    26,    27,    28,    29,    30,    59,
      15,    15,    59,    35,    36,    37,    38,    39,    59,     9,
      -1,    43,    44,    45,    46,    47,   240,    49,   280,    -1,
      52,    53,     3,    55,    56,    -1,    58,     8,    60,    10,
      11,    12,    13,    -1,    -1,    16,    17,    18,    -1,    -1,
      -1,    22,    23,    -1,    -1,    26,    27,    28,    29,    30,
      -1,    -1,    -1,    -1,    35,    36,    37,    38,    39,    -1,
      -1,    -1,    43,    44,    45,    46,    47,    -1,    -1,    -1,
      -1,    52,    -1,    -1,    55,    56,     3,    58,    -1,    60,
      61,     8,    -1,    10,    11,    12,    13,    -1,    -1,    16,
      17,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      27,    28,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    51,    52,    53,     3,    55,    56,
      -1,    58,     8,    60,    10,    11,    12,    13,    -1,    -1,
      16,    17,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,
      26,    27,    28,    29,    30,    -1,    -1,    -1,    -1,    35,
      36,    37,    38,    39,    -1,    -1,    -1,    43,    44,    45,
      46,    47,    -1,    -1,    -1,    -1,    52,    -1,     3,    55,
      56,    -1,    58,     8,    60,    10,    11,    12,    13,    -1,
      -1,    16,    17,    18,    -1,    -1,    -1,    22,    23,    -1,
      -1,    26,    27,    28,    29,    30,    -1,    -1,    -1,    -1,
      35,    36,    37,    38,    39,    -1,    -1,    -1,    43,    44,
      45,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,     3,
      55,    56,    -1,    58,     8,    60,    10,    11,    12,    13,
      -1,    -1,    16,    17,    18,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    26,    27,    28,    29,    30,    -1,    -1,    -1,
      -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,    43,
      44,    45,    46,    47,    -1,    -1,    -1,    -1,    52,    -1,
       3,    55,    56,    -1,    58,     8,    60,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    26,    27,    28,    29,    30,    -1,    -1,
      -1,    -1,    35,    36,    37,    38,    39,    -1,    -1,    -1,
      43,    44,    45,    46,    47,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    56,    -1,    58,    -1,    60,    28,    29,
      -1,    -1,    -1,    -1,    -1,    35,    36,    37,    38,    39,
      -1,    -1,    -1,    43,    44,    45,    46,    47,    -1,    35,
      36,    37,    38,    39,    -1,    55,    56,    43,    -1,    -1,
      46,    47,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,
      56
  };

  const signed char
  parser::yystos_[] =
  {
       0,     1,     3,     8,    10,    11,    12,    13,    16,    17,
      18,    22,    23,    26,    27,    28,    29,    30,    35,    36,
      37,    38,    39,    43,    44,    45,    46,    47,    49,    52,
      53,    55,    56,    58,    60,    63,    65,    66,    68,    69,
      70,    71,    72,    73,    74,    75,    77,    78,    79,    80,
      81,    82,    83,    95,    96,    97,    98,    99,    52,    53,
      89,    94,    26,    26,    31,    26,    89,    89,    26,    26,
      68,    70,    32,    51,    52,    53,    93,    97,    24,    25,
      60,    35,    36,    37,    38,    39,    43,    46,    47,    55,
      56,    35,    36,    37,    38,    39,    43,    46,    47,    55,
      56,    26,    26,    26,    29,    57,    26,    26,    29,    57,
      26,    26,    26,    26,    26,    89,    94,    26,    26,    89,
      89,     0,    26,    66,    65,    67,    52,    53,    92,    33,
      34,    50,    51,    48,    54,    93,    97,     4,    52,    90,
      91,    97,    94,    51,    94,    14,    58,    93,    51,    94,
      14,    14,    52,    60,    70,    76,    70,    67,    19,    25,
      61,    26,    26,    26,    29,    57,    26,    26,    29,    57,
      26,    26,    26,    26,    26,    26,    26,    26,    29,    57,
      26,    26,    29,    57,    26,    26,    26,    26,    26,    61,
      61,    59,    61,    65,    94,    94,    96,    96,    94,    94,
      89,    33,    34,    50,    51,    52,    21,    94,    14,    21,
      58,    89,    89,    94,    94,    14,    21,    58,    89,    89,
      94,    61,    67,    67,    94,    96,    96,    98,    98,     5,
       6,     7,    84,    94,    94,    94,    94,    94,    85,    86,
      87,    94,    14,    58,    89,    26,    64,    93,    89,    15,
      59,    14,    58,    14,    58,    89,    64,    93,    89,    15,
      15,    76,    94,    76,    89,    89,     7,    91,    91,    91,
      91,    91,     9,    40,    41,    42,    86,    94,     9,    26,
      60,    88,    89,    89,    15,    26,    93,    94,    59,    89,
      89,    89,    89,    15,    93,    94,    59,    76,     7,     4,
      40,    41,    42,     9,    88,    54,    61,    15,    59,    94,
      14,    58,    15,    59,    15,    59,    94,    14,    58,    89,
      61,    26,    89,    94,    14,    58,    89,    89,    14,    58,
      89,    89,     5,    84,    89,    94,    89,    89,    15,    59,
      89,    89,    15,    59,    89,    15,    59,    15,    59
  };

  const signed char
  parser::yyr1_[] =
  {
       0,    62,    63,    63,    63,    63,    63,    63,    64,    64,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    65,    65,    65,    65,    65,    65,
      65,    65,    65,    65,    66,    66,    66,    67,    67,    68,
      68,    69,    69,    69,    69,    69,    70,    70,    70,    70,
      70,    70,    70,    70,    70,    70,    70,    71,    71,    71,
      71,    71,    71,    71,    71,    72,    72,    72,    72,    73,
      73,    73,    73,    73,    73,    73,    73,    74,    74,    74,
      75,    75,    75,    75,    76,    76,    77,    78,    78,    79,
      79,    79,    79,    79,    80,    80,    80,    81,    82,    83,
      84,    84,    84,    85,    85,    86,    86,    86,    86,    87,
      87,    87,    87,    87,    87,    88,    88,    89,    89,    90,
      90,    90,    91,    91,    91,    91,    91,    91,    92,    92,
      93,    93,    93,    94,    94,    95,    95,    95,    96,    96,
      96,    96,    96,    97,    97,    97,    97,    97,    98,    98,
      98,    99,    99,    99,    99
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     2,     1,     1,     2,     2,     1,     1,     2,
       2,     2,     3,     3,     3,     3,     2,     3,     3,     2,
       3,     3,     2,     3,     3,     2,     3,     3,     2,     3,
       3,     2,     3,     3,     2,     3,     3,     2,     3,     3,
       2,     3,     3,     2,     3,     3,     2,     3,     3,     2,
       3,     3,     2,     2,     1,     1,     1,     1,     2,     1,
       2,     1,     1,     2,     1,     1,     1,     1,     5,     5,
       1,     1,     1,     1,     1,     1,     1,     6,     6,     7,
       7,    10,    10,     9,     9,     7,     7,     5,     5,     6,
       6,     7,     7,    10,    10,     9,     9,     6,     7,     6,
       5,     6,     3,     5,     1,     2,     3,     3,     3,     2,
       3,     3,     4,     2,     5,     7,     6,     3,     1,     3,
       4,     6,     5,     1,     2,     4,     4,     5,     5,     2,
       3,     2,     3,     2,     3,     1,     3,     2,     2,     3,
       3,     3,     4,     4,     4,     4,     4,     1,     1,     1,
       1,     1,     1,     0,     2,     1,     2,     2,     4,     4,
       3,     3,     1,     1,     2,     2,     2,     2,     4,     4,
       1,     1,     2,     2,     3
  };


#if YYDEBUG
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"end of file\"", "error", "\"invalid token\"", "IF", "THEN", "ELSE",
  "ELIF", "FI", "CASE", "ESAC", "FOR", "SELECT", "WHILE", "UNTIL", "DO",
  "DONE", "FUNCTION", "COPROC", "COND_START", "COND_END", "COND_ERROR",
  "IN", "BANG", "TIME", "TIMEOPT", "TIMEIGN", "WORD", "ASSIGNMENT_WORD",
  "REDIR_WORD", "NUMBER", "ARITH_CMD", "ARITH_FOR_EXPRS", "COND_CMD",
  "AND_AND", "OR_OR", "GREATER_GREATER", "LESS_LESS", "LESS_AND",
  "LESS_LESS_LESS", "GREATER_AND", "SEMI_SEMI", "SEMI_AND",
  "SEMI_SEMI_AND", "LESS_LESS_MINUS", "AND_GREATER", "AND_GREATER_GREATER",
  "LESS_GREATER", "GREATER_BAR", "BAR_AND", "DOLPAREN", "'&'", "';'",
  "'\\n'", "yacc_EOF", "'|'", "'>'", "'<'", "'-'", "'{'", "'}'", "'('",
  "')'", "$accept", "inputunit", "word_list", "redirection",
  "simple_command_element", "redirection_list", "simple_command",
  "command", "shell_command", "for_command", "arith_for_command",
  "select_command", "case_command", "function_def", "function_body",
  "subshell", "comsub", "coproc", "if_command", "group_command",
  "arith_command", "cond_command", "elif_clause", "case_clause",
  "pattern_list", "case_clause_sequence", "pattern", "compound_list",
  "list0", "list1", "simple_list_terminator", "list_terminator",
  "newline_list", "simple_list", "simple_list1", "pipeline_command",
  "pipeline", "timespec", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  parser::yyrline_[] =
  {
       0,    95,    95,   107,   116,   126,   142,   161,   171,   175,
     181,   188,   195,   202,   209,   216,   223,   230,   237,   244,
     251,   258,   265,   272,   279,   286,   294,   302,   310,   318,
     326,   334,   341,   348,   355,   362,   369,   376,   383,   390,
     397,   404,   411,   419,   426,   433,   441,   448,   455,   462,
     469,   476,   483,   490,   499,   504,   509,   516,   520,   526,
     530,   536,   540,   544,   557,   561,   567,   571,   575,   580,
     585,   589,   593,   597,   601,   605,   609,   615,   626,   637,
     648,   659,   670,   681,   691,   703,   714,   725,   736,   749,
     760,   771,   782,   793,   804,   815,   825,   837,   845,   853,
     863,   870,   877,   884,   893,   897,   923,   929,   933,   939,
     943,   954,   958,   969,   976,   980,   984,   991,   997,  1003,
    1009,  1013,  1017,  1023,  1024,  1031,  1036,  1041,  1046,  1053,
    1057,  1062,  1067,  1073,  1078,  1086,  1090,  1101,  1107,  1113,
    1114,  1122,  1125,  1130,  1135,  1143,  1147,  1154,  1160,  1161,
    1164,  1166,  1168,  1172,  1173,  1182,  1199,  1220,  1239,  1244,
    1249,  1257,  1261,  1267,  1271,  1277,  1284,  1307,  1332,  1336,
    1361,  1367,  1371,  1375,  1379
  };

  void
  parser::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
  parser::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


#line 26 "../bashcpp/parse.yy"
} // bash
#line 3333 "../bashcpp/parse.cc"

#line 1384 "../bashcpp/parse.yy"


#if defined(__clang__)
#pragma clang diagnostic pop
#endif
