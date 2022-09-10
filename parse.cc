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

#ifdef DEBUG
#  define YYDEBUG 1
#else
#  define YYDEBUG 0
#endif

static inline bash::parser::token::token_kind_type
yylex () {
  return bash::the_shell->yylex ();
}


#line 58 "parse.cc"


#include "parse.hh"




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
#line 137 "parse.cc"

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
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_list: // list
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.YY_MOVE_OR_COPY< COMMAND* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.YY_MOVE_OR_COPY< ELEMENT* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.YY_MOVE_OR_COPY< PATTERN_LIST* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.YY_MOVE_OR_COPY< REDIRECT* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.YY_MOVE_OR_COPY< WORD_DESC* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.YY_MOVE_OR_COPY< WORD_LIST* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.YY_MOVE_OR_COPY< int > (YY_MOVE (that.value));
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
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_list: // list
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.move< COMMAND* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.move< ELEMENT* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.move< PATTERN_LIST* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.move< REDIRECT* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.move< WORD_DESC* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.move< WORD_LIST* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.move< int > (YY_MOVE (that.value));
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
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_list: // list
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.copy< COMMAND* > (that.value);
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.copy< ELEMENT* > (that.value);
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.copy< PATTERN_LIST* > (that.value);
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.copy< REDIRECT* > (that.value);
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.copy< WORD_DESC* > (that.value);
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.copy< WORD_LIST* > (that.value);
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.copy< int > (that.value);
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
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_list: // list
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        value.move< COMMAND* > (that.value);
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.move< ELEMENT* > (that.value);
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.move< PATTERN_LIST* > (that.value);
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.move< REDIRECT* > (that.value);
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.move< WORD_DESC* > (that.value);
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.move< WORD_LIST* > (that.value);
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.move< int > (that.value);
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
      case symbol_kind::S_coproc: // coproc
      case symbol_kind::S_if_command: // if_command
      case symbol_kind::S_group_command: // group_command
      case symbol_kind::S_arith_command: // arith_command
      case symbol_kind::S_cond_command: // cond_command
      case symbol_kind::S_elif_clause: // elif_clause
      case symbol_kind::S_list: // list
      case symbol_kind::S_compound_list: // compound_list
      case symbol_kind::S_list0: // list0
      case symbol_kind::S_list1: // list1
      case symbol_kind::S_simple_list: // simple_list
      case symbol_kind::S_simple_list1: // simple_list1
      case symbol_kind::S_pipeline_command: // pipeline_command
      case symbol_kind::S_pipeline: // pipeline
        yylhs.value.emplace< COMMAND* > ();
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        yylhs.value.emplace< ELEMENT* > ();
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        yylhs.value.emplace< PATTERN_LIST* > ();
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        yylhs.value.emplace< REDIRECT* > ();
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        yylhs.value.emplace< WORD_DESC* > ();
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        yylhs.value.emplace< WORD_LIST* > ();
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        yylhs.value.emplace< int > ();
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
#line 89 "../bashcpp/parse.yy"
                        {
			  /* Case of regular command.  Discard the error
			     safety net,and return the command just parsed. */
			  global_command = yystack_[1].value.as < COMMAND* > ();
			  eof_encountered = 0;
			  /* discard_parser_constructs (0); */
			  if (parser_state & PST_CMDSUBST)
			    parser_state |= PST_EOFTOKEN;
			  YYACCEPT;
			}
#line 842 "parse.cc"
    break;

  case 3: // inputunit: '\n'
#line 100 "../bashcpp/parse.yy"
                        {
			  /* Case of regular command, but not a very
			     interesting one.  Return a NULL command. */
			  global_command = (COMMAND *)NULL;
			  if (parser_state & PST_CMDSUBST)
			    parser_state |= PST_EOFTOKEN;
			  YYACCEPT;
			}
#line 855 "parse.cc"
    break;

  case 4: // inputunit: error '\n'
#line 109 "../bashcpp/parse.yy"
                        {
			  /* Error during parsing.  Return NULL command. */
			  global_command = (COMMAND *)NULL;
			  eof_encountered = 0;
			  /* discard_parser_constructs (1); */
			  if (interactive && parse_and_execute_level == 0)
			    {
			      YYACCEPT;
			    }
			  else
			    {
			      YYABORT;
			    }
			}
#line 874 "parse.cc"
    break;

  case 5: // inputunit: error yacc_EOF
#line 124 "../bashcpp/parse.yy"
                        {
			  /* EOF after an error.  Do ignoreeof or not.  Really
			     only interesting in non-interactive shells */
			  global_command = (COMMAND *)NULL;
			  if (last_command_exit_value == 0)
			    last_command_exit_value = EX_BADUSAGE;	/* force error return */
			  if (interactive && parse_and_execute_level == 0)
			    {
			      handle_eof_input_unit ();
			      YYACCEPT;
			    }
			  else
			    {
			      YYABORT;
			    }
			}
#line 895 "parse.cc"
    break;

  case 6: // inputunit: yacc_EOF
#line 141 "../bashcpp/parse.yy"
                        {
			  /* Case of EOF seen by itself.  Do ignoreeof or
			     not. */
			  global_command = (COMMAND *)NULL;
			  handle_eof_input_unit ();
			  YYACCEPT;
			}
#line 907 "parse.cc"
    break;

  case 7: // word_list: WORD
#line 151 "../bashcpp/parse.yy"
                        { yylhs.value.as < WORD_LIST* > () = make_word_list (yystack_[0].value.as < WORD_DESC* > (), (WORD_LIST *)NULL); }
#line 913 "parse.cc"
    break;

  case 8: // word_list: word_list WORD
#line 153 "../bashcpp/parse.yy"
                        { yylhs.value.as < WORD_LIST* > () = make_word_list (yystack_[0].value.as < WORD_DESC* > (), yystack_[1].value.as < WORD_LIST* > ()); }
#line 919 "parse.cc"
    break;

  case 9: // redirection: '>' WORD
#line 157 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_output_direction, redir, 0);
			}
#line 929 "parse.cc"
    break;

  case 10: // redirection: '<' WORD
#line 163 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_input_direction, redir, 0);
			}
#line 939 "parse.cc"
    break;

  case 11: // redirection: NUMBER '>' WORD
#line 169 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_output_direction, redir, 0);
			}
#line 949 "parse.cc"
    break;

  case 12: // redirection: NUMBER '<' WORD
#line 175 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_input_direction, redir, 0);
			}
#line 959 "parse.cc"
    break;

  case 13: // redirection: REDIR_WORD '>' WORD
#line 181 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_output_direction, redir, REDIR_VARASSIGN);
			}
#line 969 "parse.cc"
    break;

  case 14: // redirection: REDIR_WORD '<' WORD
#line 187 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_input_direction, redir, REDIR_VARASSIGN);
			}
#line 979 "parse.cc"
    break;

  case 15: // redirection: GREATER_GREATER WORD
#line 193 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_appending_to, redir, 0);
			}
#line 989 "parse.cc"
    break;

  case 16: // redirection: NUMBER GREATER_GREATER WORD
#line 199 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_appending_to, redir, 0);
			}
#line 999 "parse.cc"
    break;

  case 17: // redirection: REDIR_WORD GREATER_GREATER WORD
#line 205 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_appending_to, redir, REDIR_VARASSIGN);
			}
#line 1009 "parse.cc"
    break;

  case 18: // redirection: GREATER_BAR WORD
#line 211 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_output_force, redir, 0);
			}
#line 1019 "parse.cc"
    break;

  case 19: // redirection: NUMBER GREATER_BAR WORD
#line 217 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_output_force, redir, 0);
			}
#line 1029 "parse.cc"
    break;

  case 20: // redirection: REDIR_WORD GREATER_BAR WORD
#line 223 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_output_force, redir, REDIR_VARASSIGN);
			}
#line 1039 "parse.cc"
    break;

  case 21: // redirection: LESS_GREATER WORD
#line 229 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_input_output, redir, 0);
			}
#line 1049 "parse.cc"
    break;

  case 22: // redirection: NUMBER LESS_GREATER WORD
#line 235 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_input_output, redir, 0);
			}
#line 1059 "parse.cc"
    break;

  case 23: // redirection: REDIR_WORD LESS_GREATER WORD
#line 241 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_input_output, redir, REDIR_VARASSIGN);
			}
#line 1069 "parse.cc"
    break;

  case 24: // redirection: LESS_LESS WORD
#line 247 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_reading_until, redir, 0);
			  push_heredoc (yylhs.value.as < REDIRECT* > ());
			}
#line 1080 "parse.cc"
    break;

  case 25: // redirection: NUMBER LESS_LESS WORD
#line 254 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_reading_until, redir, 0);
			  push_heredoc (yylhs.value.as < REDIRECT* > ());
			}
#line 1091 "parse.cc"
    break;

  case 26: // redirection: REDIR_WORD LESS_LESS WORD
#line 261 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_reading_until, redir, REDIR_VARASSIGN);
			  push_heredoc (yylhs.value.as < REDIRECT* > ());
			}
#line 1102 "parse.cc"
    break;

  case 27: // redirection: LESS_LESS_MINUS WORD
#line 268 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_deblank_reading_until, redir, 0);
			  push_heredoc (yylhs.value.as < REDIRECT* > ());
			}
#line 1113 "parse.cc"
    break;

  case 28: // redirection: NUMBER LESS_LESS_MINUS WORD
#line 275 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_deblank_reading_until, redir, 0);
			  push_heredoc (yylhs.value.as < REDIRECT* > ());
			}
#line 1124 "parse.cc"
    break;

  case 29: // redirection: REDIR_WORD LESS_LESS_MINUS WORD
#line 282 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_deblank_reading_until, redir, REDIR_VARASSIGN);
			  push_heredoc (yylhs.value.as < REDIRECT* > ());
			}
#line 1135 "parse.cc"
    break;

  case 30: // redirection: LESS_LESS_LESS WORD
#line 289 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_reading_string, redir, 0);
			}
#line 1145 "parse.cc"
    break;

  case 31: // redirection: NUMBER LESS_LESS_LESS WORD
#line 295 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_reading_string, redir, 0);
			}
#line 1155 "parse.cc"
    break;

  case 32: // redirection: REDIR_WORD LESS_LESS_LESS WORD
#line 301 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_reading_string, redir, REDIR_VARASSIGN);
			}
#line 1165 "parse.cc"
    break;

  case 33: // redirection: LESS_AND NUMBER
#line 307 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.dest = yystack_[0].value.as < int > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_input, redir, 0);
			}
#line 1175 "parse.cc"
    break;

  case 34: // redirection: NUMBER LESS_AND NUMBER
#line 313 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.dest = yystack_[0].value.as < int > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_input, redir, 0);
			}
#line 1185 "parse.cc"
    break;

  case 35: // redirection: REDIR_WORD LESS_AND NUMBER
#line 319 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.dest = yystack_[0].value.as < int > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_input, redir, REDIR_VARASSIGN);
			}
#line 1195 "parse.cc"
    break;

  case 36: // redirection: GREATER_AND NUMBER
#line 325 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.dest = yystack_[0].value.as < int > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_output, redir, 0);
			}
#line 1205 "parse.cc"
    break;

  case 37: // redirection: NUMBER GREATER_AND NUMBER
#line 331 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.dest = yystack_[0].value.as < int > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_output, redir, 0);
			}
#line 1215 "parse.cc"
    break;

  case 38: // redirection: REDIR_WORD GREATER_AND NUMBER
#line 337 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.dest = yystack_[0].value.as < int > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_output, redir, REDIR_VARASSIGN);
			}
#line 1225 "parse.cc"
    break;

  case 39: // redirection: LESS_AND WORD
#line 343 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_input_word, redir, 0);
			}
#line 1235 "parse.cc"
    break;

  case 40: // redirection: NUMBER LESS_AND WORD
#line 349 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_input_word, redir, 0);
			}
#line 1245 "parse.cc"
    break;

  case 41: // redirection: REDIR_WORD LESS_AND WORD
#line 355 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_input_word, redir, REDIR_VARASSIGN);
			}
#line 1255 "parse.cc"
    break;

  case 42: // redirection: GREATER_AND WORD
#line 361 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_output_word, redir, 0);
			}
#line 1265 "parse.cc"
    break;

  case 43: // redirection: NUMBER GREATER_AND WORD
#line 367 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_output_word, redir, 0);
			}
#line 1275 "parse.cc"
    break;

  case 44: // redirection: REDIR_WORD GREATER_AND WORD
#line 373 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_duplicating_output_word, redir, REDIR_VARASSIGN);
			}
#line 1285 "parse.cc"
    break;

  case 45: // redirection: GREATER_AND '-'
#line 379 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.dest = 0;
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_close_this, redir, 0);
			}
#line 1295 "parse.cc"
    break;

  case 46: // redirection: NUMBER GREATER_AND '-'
#line 385 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.dest = 0;
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_close_this, redir, 0);
			}
#line 1305 "parse.cc"
    break;

  case 47: // redirection: REDIR_WORD GREATER_AND '-'
#line 391 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.dest = 0;
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_close_this, redir, REDIR_VARASSIGN);
			}
#line 1315 "parse.cc"
    break;

  case 48: // redirection: LESS_AND '-'
#line 397 "../bashcpp/parse.yy"
                        {
			  source.dest = 0;
			  redir.dest = 0;
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_close_this, redir, 0);
			}
#line 1325 "parse.cc"
    break;

  case 49: // redirection: NUMBER LESS_AND '-'
#line 403 "../bashcpp/parse.yy"
                        {
			  source.dest = yystack_[2].value.as < int > ();
			  redir.dest = 0;
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_close_this, redir, 0);
			}
#line 1335 "parse.cc"
    break;

  case 50: // redirection: REDIR_WORD LESS_AND '-'
#line 409 "../bashcpp/parse.yy"
                        {
			  source.filename = yystack_[2].value.as < WORD_DESC* > ();
			  redir.dest = 0;
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_close_this, redir, REDIR_VARASSIGN);
			}
#line 1345 "parse.cc"
    break;

  case 51: // redirection: AND_GREATER WORD
#line 415 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_err_and_out, redir, 0);
			}
#line 1355 "parse.cc"
    break;

  case 52: // redirection: AND_GREATER_GREATER WORD
#line 421 "../bashcpp/parse.yy"
                        {
			  source.dest = 1;
			  redir.filename = yystack_[0].value.as < WORD_DESC* > ();
			  yylhs.value.as < REDIRECT* > () = make_redirection (source, r_append_err_and_out, redir, 0);
			}
#line 1365 "parse.cc"
    break;

  case 53: // simple_command_element: WORD
#line 429 "../bashcpp/parse.yy"
                        { yylhs.value.as < ELEMENT* > ().word = yystack_[0].value.as < WORD_DESC* > (); yylhs.value.as < ELEMENT* > ().redirect = 0; }
#line 1371 "parse.cc"
    break;

  case 54: // simple_command_element: ASSIGNMENT_WORD
#line 431 "../bashcpp/parse.yy"
                        { yylhs.value.as < ELEMENT* > ().word = yystack_[0].value.as < WORD_DESC* > (); yylhs.value.as < ELEMENT* > ().redirect = 0; }
#line 1377 "parse.cc"
    break;

  case 55: // simple_command_element: redirection
#line 433 "../bashcpp/parse.yy"
                        { yylhs.value.as < ELEMENT* > ().redirect = yystack_[0].value.as < REDIRECT* > (); yylhs.value.as < ELEMENT* > ().word = 0; }
#line 1383 "parse.cc"
    break;

  case 56: // redirection_list: redirection
#line 437 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < REDIRECT* > () = yystack_[0].value.as < REDIRECT* > ();
			}
#line 1391 "parse.cc"
    break;

  case 57: // redirection_list: redirection_list redirection
#line 441 "../bashcpp/parse.yy"
                        {
			  REDIRECT *t;

			  for (t = yystack_[1].value.as < REDIRECT* > (); t->next; t = t->next)
			    ;
			  t->next = yystack_[0].value.as < REDIRECT* > ();
			  yylhs.value.as < REDIRECT* > () = yystack_[1].value.as < REDIRECT* > ();
			}
#line 1404 "parse.cc"
    break;

  case 58: // simple_command: simple_command_element
#line 452 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_simple_command (yystack_[0].value.as < ELEMENT* > (), (COMMAND *)NULL); }
#line 1410 "parse.cc"
    break;

  case 59: // simple_command: simple_command simple_command_element
#line 454 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_simple_command (yystack_[0].value.as < ELEMENT* > (), yystack_[1].value.as < COMMAND* > ()); }
#line 1416 "parse.cc"
    break;

  case 60: // command: simple_command
#line 458 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = clean_simple_command (yystack_[0].value.as < COMMAND* > ()); }
#line 1422 "parse.cc"
    break;

  case 61: // command: shell_command
#line 460 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1428 "parse.cc"
    break;

  case 62: // command: shell_command redirection_list
#line 462 "../bashcpp/parse.yy"
                        {
			  COMMAND *tc;

			  tc = yystack_[1].value.as < COMMAND* > ();
			  if (tc && tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = yystack_[0].value.as < REDIRECT* > ();
			    }
			  else if (tc)
			    tc->redirects = yystack_[0].value.as < REDIRECT* > ();
			  yylhs.value.as < COMMAND* > () = yystack_[1].value.as < COMMAND* > ();
			}
#line 1448 "parse.cc"
    break;

  case 63: // command: function_def
#line 478 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1454 "parse.cc"
    break;

  case 64: // command: coproc
#line 480 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1460 "parse.cc"
    break;

  case 65: // shell_command: for_command
#line 484 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1466 "parse.cc"
    break;

  case 66: // shell_command: case_command
#line 486 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1472 "parse.cc"
    break;

  case 67: // shell_command: WHILE compound_list DO compound_list DONE
#line 488 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_while_command (yystack_[3].value.as < COMMAND* > (), yystack_[1].value.as < COMMAND* > ()); }
#line 1478 "parse.cc"
    break;

  case 68: // shell_command: UNTIL compound_list DO compound_list DONE
#line 490 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_until_command (yystack_[3].value.as < COMMAND* > (), yystack_[1].value.as < COMMAND* > ()); }
#line 1484 "parse.cc"
    break;

  case 69: // shell_command: select_command
#line 492 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1490 "parse.cc"
    break;

  case 70: // shell_command: if_command
#line 494 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1496 "parse.cc"
    break;

  case 71: // shell_command: subshell
#line 496 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1502 "parse.cc"
    break;

  case 72: // shell_command: group_command
#line 498 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1508 "parse.cc"
    break;

  case 73: // shell_command: arith_command
#line 500 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1514 "parse.cc"
    break;

  case 74: // shell_command: cond_command
#line 502 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1520 "parse.cc"
    break;

  case 75: // shell_command: arith_for_command
#line 504 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1526 "parse.cc"
    break;

  case 76: // for_command: FOR WORD newline_list DO compound_list DONE
#line 508 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[4].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1535 "parse.cc"
    break;

  case 77: // for_command: FOR WORD newline_list '{' compound_list '}'
#line 513 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[4].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1544 "parse.cc"
    break;

  case 78: // for_command: FOR WORD ';' newline_list DO compound_list DONE
#line 518 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[5].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1553 "parse.cc"
    break;

  case 79: // for_command: FOR WORD ';' newline_list '{' compound_list '}'
#line 523 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[5].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1562 "parse.cc"
    break;

  case 80: // for_command: FOR WORD newline_list IN word_list list_terminator newline_list DO compound_list DONE
#line 528 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[8].value.as < WORD_DESC* > (), REVERSE_LIST (yystack_[5].value.as < WORD_LIST* > (), WORD_LIST *), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1571 "parse.cc"
    break;

  case 81: // for_command: FOR WORD newline_list IN word_list list_terminator newline_list '{' compound_list '}'
#line 533 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[8].value.as < WORD_DESC* > (), REVERSE_LIST (yystack_[5].value.as < WORD_LIST* > (), WORD_LIST *), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1580 "parse.cc"
    break;

  case 82: // for_command: FOR WORD newline_list IN list_terminator newline_list DO compound_list DONE
#line 538 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[7].value.as < WORD_DESC* > (), (WORD_LIST *)NULL, yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1589 "parse.cc"
    break;

  case 83: // for_command: FOR WORD newline_list IN list_terminator newline_list '{' compound_list '}'
#line 543 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_for_command (yystack_[7].value.as < WORD_DESC* > (), (WORD_LIST *)NULL, yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1598 "parse.cc"
    break;

  case 84: // arith_for_command: FOR ARITH_FOR_EXPRS list_terminator newline_list DO compound_list DONE
#line 550 "../bashcpp/parse.yy"
                                {
				  yylhs.value.as < COMMAND* > () = make_arith_for_command (yystack_[5].value.as < WORD_LIST* > (), yystack_[1].value.as < COMMAND* > (), arith_for_lineno);
				  if (yylhs.value.as < COMMAND* > () == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
#line 1608 "parse.cc"
    break;

  case 85: // arith_for_command: FOR ARITH_FOR_EXPRS list_terminator newline_list '{' compound_list '}'
#line 556 "../bashcpp/parse.yy"
                                {
				  yylhs.value.as < COMMAND* > () = make_arith_for_command (yystack_[5].value.as < WORD_LIST* > (), yystack_[1].value.as < COMMAND* > (), arith_for_lineno);
				  if (yylhs.value.as < COMMAND* > () == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
#line 1618 "parse.cc"
    break;

  case 86: // arith_for_command: FOR ARITH_FOR_EXPRS DO compound_list DONE
#line 562 "../bashcpp/parse.yy"
                                {
				  yylhs.value.as < COMMAND* > () = make_arith_for_command (yystack_[3].value.as < WORD_LIST* > (), yystack_[1].value.as < COMMAND* > (), arith_for_lineno);
				  if (yylhs.value.as < COMMAND* > () == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
#line 1628 "parse.cc"
    break;

  case 87: // arith_for_command: FOR ARITH_FOR_EXPRS '{' compound_list '}'
#line 568 "../bashcpp/parse.yy"
                                {
				  yylhs.value.as < COMMAND* > () = make_arith_for_command (yystack_[3].value.as < WORD_LIST* > (), yystack_[1].value.as < COMMAND* > (), arith_for_lineno);
				  if (yylhs.value.as < COMMAND* > () == 0) YYERROR;
				  if (word_top > 0) word_top--;
				}
#line 1638 "parse.cc"
    break;

  case 88: // select_command: SELECT WORD newline_list DO list DONE
#line 576 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[4].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1647 "parse.cc"
    break;

  case 89: // select_command: SELECT WORD newline_list '{' list '}'
#line 581 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[4].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1656 "parse.cc"
    break;

  case 90: // select_command: SELECT WORD ';' newline_list DO list DONE
#line 586 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[5].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1665 "parse.cc"
    break;

  case 91: // select_command: SELECT WORD ';' newline_list '{' list '}'
#line 591 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[5].value.as < WORD_DESC* > (), add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1674 "parse.cc"
    break;

  case 92: // select_command: SELECT WORD newline_list IN word_list list_terminator newline_list DO list DONE
#line 596 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[8].value.as < WORD_DESC* > (), REVERSE_LIST (yystack_[5].value.as < WORD_LIST* > (), WORD_LIST *), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1683 "parse.cc"
    break;

  case 93: // select_command: SELECT WORD newline_list IN word_list list_terminator newline_list '{' list '}'
#line 601 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[8].value.as < WORD_DESC* > (), REVERSE_LIST (yystack_[5].value.as < WORD_LIST* > (), WORD_LIST *), yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1692 "parse.cc"
    break;

  case 94: // select_command: SELECT WORD newline_list IN list_terminator newline_list DO compound_list DONE
#line 606 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[7].value.as < WORD_DESC* > (), (WORD_LIST *)NULL, yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1701 "parse.cc"
    break;

  case 95: // select_command: SELECT WORD newline_list IN list_terminator newline_list '{' compound_list '}'
#line 611 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_select_command (yystack_[7].value.as < WORD_DESC* > (), (WORD_LIST *)NULL, yystack_[1].value.as < COMMAND* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1710 "parse.cc"
    break;

  case 96: // case_command: CASE WORD newline_list IN newline_list ESAC
#line 618 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_case_command (yystack_[4].value.as < WORD_DESC* > (), (PATTERN_LIST *)NULL, word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1719 "parse.cc"
    break;

  case 97: // case_command: CASE WORD newline_list IN case_clause_sequence newline_list ESAC
#line 623 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_case_command (yystack_[5].value.as < WORD_DESC* > (), yystack_[2].value.as < PATTERN_LIST* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1728 "parse.cc"
    break;

  case 98: // case_command: CASE WORD newline_list IN case_clause ESAC
#line 628 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_case_command (yystack_[4].value.as < WORD_DESC* > (), yystack_[1].value.as < PATTERN_LIST* > (), word_lineno[word_top]);
			  if (word_top > 0) word_top--;
			}
#line 1737 "parse.cc"
    break;

  case 99: // function_def: WORD '(' ')' newline_list function_body
#line 635 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_function_def (yystack_[4].value.as < WORD_DESC* > (), yystack_[0].value.as < COMMAND* > (), function_dstart, function_bstart); }
#line 1743 "parse.cc"
    break;

  case 100: // function_def: FUNCTION WORD '(' ')' newline_list function_body
#line 637 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_function_def (yystack_[4].value.as < WORD_DESC* > (), yystack_[0].value.as < COMMAND* > (), function_dstart, function_bstart); }
#line 1749 "parse.cc"
    break;

  case 101: // function_def: FUNCTION WORD function_body
#line 639 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_function_def (yystack_[1].value.as < WORD_DESC* > (), yystack_[0].value.as < COMMAND* > (), function_dstart, function_bstart); }
#line 1755 "parse.cc"
    break;

  case 102: // function_def: FUNCTION WORD '\n' newline_list function_body
#line 641 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_function_def (yystack_[3].value.as < WORD_DESC* > (), yystack_[0].value.as < COMMAND* > (), function_dstart, function_bstart); }
#line 1761 "parse.cc"
    break;

  case 103: // function_body: shell_command
#line 645 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 1767 "parse.cc"
    break;

  case 104: // function_body: shell_command redirection_list
#line 647 "../bashcpp/parse.yy"
                        {
			  COMMAND *tc;

			  tc = yystack_[1].value.as < COMMAND* > ();
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
			      t->next = yystack_[0].value.as < REDIRECT* > ();
			    }
			  else if (tc)
			    tc->redirects = yystack_[0].value.as < REDIRECT* > ();
			  yylhs.value.as < COMMAND* > () = yystack_[1].value.as < COMMAND* > ();
			}
#line 1800 "parse.cc"
    break;

  case 105: // subshell: '(' compound_list ')'
#line 678 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_subshell_command (yystack_[1].value.as < COMMAND* > ());
			  yylhs.value.as < COMMAND* > ()->flags |= CMD_WANT_SUBSHELL;
			}
#line 1809 "parse.cc"
    break;

  case 106: // coproc: COPROC shell_command
#line 685 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_coproc_command ("COPROC", yystack_[0].value.as < COMMAND* > ());
			  yylhs.value.as < COMMAND* > ()->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
#line 1818 "parse.cc"
    break;

  case 107: // coproc: COPROC shell_command redirection_list
#line 690 "../bashcpp/parse.yy"
                        {
			  COMMAND *tc;

			  tc = yystack_[1].value.as < COMMAND* > ();
			  if (tc && tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = yystack_[0].value.as < REDIRECT* > ();
			    }
			  else if (tc)
			    tc->redirects = yystack_[0].value.as < REDIRECT* > ();
			  yylhs.value.as < COMMAND* > () = make_coproc_command ("COPROC", yystack_[1].value.as < COMMAND* > ());
			  yylhs.value.as < COMMAND* > ()->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
#line 1839 "parse.cc"
    break;

  case 108: // coproc: COPROC WORD shell_command
#line 707 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_coproc_command (yystack_[1].value.as < WORD_DESC* > ()->word, yystack_[0].value.as < COMMAND* > ());
			  yylhs.value.as < COMMAND* > ()->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
#line 1848 "parse.cc"
    break;

  case 109: // coproc: COPROC WORD shell_command redirection_list
#line 712 "../bashcpp/parse.yy"
                        {
			  COMMAND *tc;

			  tc = yystack_[1].value.as < COMMAND* > ();
			  if (tc && tc->redirects)
			    {
			      REDIRECT *t;
			      for (t = tc->redirects; t->next; t = t->next)
				;
			      t->next = yystack_[0].value.as < REDIRECT* > ();
			    }
			  else if (tc)
			    tc->redirects = yystack_[0].value.as < REDIRECT* > ();
			  yylhs.value.as < COMMAND* > () = make_coproc_command (yystack_[2].value.as < WORD_DESC* > ()->word, yystack_[1].value.as < COMMAND* > ());
			  yylhs.value.as < COMMAND* > ()->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
#line 1869 "parse.cc"
    break;

  case 110: // coproc: COPROC simple_command
#line 729 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = make_coproc_command ("COPROC", clean_simple_command (yystack_[0].value.as < COMMAND* > ()));
			  yylhs.value.as < COMMAND* > ()->flags |= (CMD_WANT_SUBSHELL | CMD_COPROC_SUBSHELL);
			}
#line 1878 "parse.cc"
    break;

  case 111: // if_command: IF compound_list THEN compound_list FI
#line 736 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_if_command (yystack_[3].value.as < COMMAND* > (), yystack_[1].value.as < COMMAND* > (), (COMMAND *)NULL); }
#line 1884 "parse.cc"
    break;

  case 112: // if_command: IF compound_list THEN compound_list ELSE compound_list FI
#line 738 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_if_command (yystack_[5].value.as < COMMAND* > (), yystack_[3].value.as < COMMAND* > (), yystack_[1].value.as < COMMAND* > ()); }
#line 1890 "parse.cc"
    break;

  case 113: // if_command: IF compound_list THEN compound_list elif_clause FI
#line 740 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_if_command (yystack_[4].value.as < COMMAND* > (), yystack_[2].value.as < COMMAND* > (), yystack_[1].value.as < COMMAND* > ()); }
#line 1896 "parse.cc"
    break;

  case 114: // group_command: '{' compound_list '}'
#line 745 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_group_command (yystack_[1].value.as < COMMAND* > ()); }
#line 1902 "parse.cc"
    break;

  case 115: // arith_command: ARITH_CMD
#line 749 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_arith_command (yystack_[0].value.as < WORD_LIST* > ()); }
#line 1908 "parse.cc"
    break;

  case 116: // cond_command: COND_START COND_CMD COND_END
#line 753 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[1].value.as < COMMAND* > (); }
#line 1914 "parse.cc"
    break;

  case 117: // elif_clause: ELIF compound_list THEN compound_list
#line 757 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_if_command (yystack_[2].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), (COMMAND *)NULL); }
#line 1920 "parse.cc"
    break;

  case 118: // elif_clause: ELIF compound_list THEN compound_list ELSE compound_list
#line 759 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_if_command (yystack_[4].value.as < COMMAND* > (), yystack_[2].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > ()); }
#line 1926 "parse.cc"
    break;

  case 119: // elif_clause: ELIF compound_list THEN compound_list elif_clause
#line 761 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = make_if_command (yystack_[3].value.as < COMMAND* > (), yystack_[1].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > ()); }
#line 1932 "parse.cc"
    break;

  case 120: // case_clause: pattern_list
#line 764 "../bashcpp/parse.yy"
                { yylhs.value.as < PATTERN_LIST* > () = yystack_[0].value.as < PATTERN_LIST* > (); }
#line 1938 "parse.cc"
    break;

  case 121: // case_clause: case_clause_sequence pattern_list
#line 766 "../bashcpp/parse.yy"
                        { yystack_[0].value.as < PATTERN_LIST* > ()->next = yystack_[1].value.as < PATTERN_LIST* > (); yylhs.value.as < PATTERN_LIST* > () = yystack_[0].value.as < PATTERN_LIST* > (); }
#line 1944 "parse.cc"
    break;

  case 122: // pattern_list: newline_list pattern ')' compound_list
#line 770 "../bashcpp/parse.yy"
                        { yylhs.value.as < PATTERN_LIST* > () = make_pattern_list (yystack_[2].value.as < WORD_LIST* > (), yystack_[0].value.as < COMMAND* > ()); }
#line 1950 "parse.cc"
    break;

  case 123: // pattern_list: newline_list pattern ')' newline_list
#line 772 "../bashcpp/parse.yy"
                        { yylhs.value.as < PATTERN_LIST* > () = make_pattern_list (yystack_[2].value.as < WORD_LIST* > (), (COMMAND *)NULL); }
#line 1956 "parse.cc"
    break;

  case 124: // pattern_list: newline_list '(' pattern ')' compound_list
#line 774 "../bashcpp/parse.yy"
                        { yylhs.value.as < PATTERN_LIST* > () = make_pattern_list (yystack_[2].value.as < WORD_LIST* > (), yystack_[0].value.as < COMMAND* > ()); }
#line 1962 "parse.cc"
    break;

  case 125: // pattern_list: newline_list '(' pattern ')' newline_list
#line 776 "../bashcpp/parse.yy"
                        { yylhs.value.as < PATTERN_LIST* > () = make_pattern_list (yystack_[2].value.as < WORD_LIST* > (), (COMMAND *)NULL); }
#line 1968 "parse.cc"
    break;

  case 126: // case_clause_sequence: pattern_list SEMI_SEMI
#line 780 "../bashcpp/parse.yy"
                        { yylhs.value.as < PATTERN_LIST* > () = yystack_[1].value.as < PATTERN_LIST* > (); }
#line 1974 "parse.cc"
    break;

  case 127: // case_clause_sequence: case_clause_sequence pattern_list SEMI_SEMI
#line 782 "../bashcpp/parse.yy"
                        { yystack_[1].value.as < PATTERN_LIST* > ()->next = yystack_[2].value.as < PATTERN_LIST* > (); yylhs.value.as < PATTERN_LIST* > () = yystack_[1].value.as < PATTERN_LIST* > (); }
#line 1980 "parse.cc"
    break;

  case 128: // case_clause_sequence: pattern_list SEMI_AND
#line 784 "../bashcpp/parse.yy"
                        { yystack_[1].value.as < PATTERN_LIST* > ()->flags |= CASEPAT_FALLTHROUGH; yylhs.value.as < PATTERN_LIST* > () = yystack_[1].value.as < PATTERN_LIST* > (); }
#line 1986 "parse.cc"
    break;

  case 129: // case_clause_sequence: case_clause_sequence pattern_list SEMI_AND
#line 786 "../bashcpp/parse.yy"
                        { yystack_[1].value.as < PATTERN_LIST* > ()->flags |= CASEPAT_FALLTHROUGH; yystack_[1].value.as < PATTERN_LIST* > ()->next = yystack_[2].value.as < PATTERN_LIST* > (); yylhs.value.as < PATTERN_LIST* > () = yystack_[1].value.as < PATTERN_LIST* > (); }
#line 1992 "parse.cc"
    break;

  case 130: // case_clause_sequence: pattern_list SEMI_SEMI_AND
#line 788 "../bashcpp/parse.yy"
                        { yystack_[1].value.as < PATTERN_LIST* > ()->flags |= CASEPAT_TESTNEXT; yylhs.value.as < PATTERN_LIST* > () = yystack_[1].value.as < PATTERN_LIST* > (); }
#line 1998 "parse.cc"
    break;

  case 131: // case_clause_sequence: case_clause_sequence pattern_list SEMI_SEMI_AND
#line 790 "../bashcpp/parse.yy"
                        { yystack_[1].value.as < PATTERN_LIST* > ()->flags |= CASEPAT_TESTNEXT; yystack_[1].value.as < PATTERN_LIST* > ()->next = yystack_[2].value.as < PATTERN_LIST* > (); yylhs.value.as < PATTERN_LIST* > () = yystack_[1].value.as < PATTERN_LIST* > (); }
#line 2004 "parse.cc"
    break;

  case 132: // pattern: WORD
#line 794 "../bashcpp/parse.yy"
                        { yylhs.value.as < WORD_LIST* > () = make_word_list (yystack_[0].value.as < WORD_DESC* > (), (WORD_LIST *)NULL); }
#line 2010 "parse.cc"
    break;

  case 133: // pattern: pattern '|' WORD
#line 796 "../bashcpp/parse.yy"
                        { yylhs.value.as < WORD_LIST* > () = make_word_list (yystack_[0].value.as < WORD_DESC* > (), yystack_[2].value.as < WORD_LIST* > ()); }
#line 2016 "parse.cc"
    break;

  case 134: // list: newline_list list0
#line 805 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > ();
			  if (need_here_doc)
			    gather_here_documents ();
			 }
#line 2026 "parse.cc"
    break;

  case 135: // compound_list: list
#line 812 "../bashcpp/parse.yy"
                { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 2032 "parse.cc"
    break;

  case 136: // compound_list: newline_list list1
#line 814 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > ();
			}
#line 2040 "parse.cc"
    break;

  case 137: // list0: list1 '\n' newline_list
#line 819 "../bashcpp/parse.yy"
                { yylhs.value.as < COMMAND* > () = yystack_[2].value.as < COMMAND* > (); }
#line 2046 "parse.cc"
    break;

  case 138: // list0: list1 '&' newline_list
#line 821 "../bashcpp/parse.yy"
                        {
			  if (yystack_[2].value.as < COMMAND* > ()->type == cm_connection)
			    yylhs.value.as < COMMAND* > () = connect_async_list (yystack_[2].value.as < COMMAND* > (), (COMMAND *)NULL, '&');
			  else
			    yylhs.value.as < COMMAND* > () = command_connect (yystack_[2].value.as < COMMAND* > (), (COMMAND *)NULL, '&');
			}
#line 2057 "parse.cc"
    break;

  case 139: // list0: list1 ';' newline_list
#line 827 "../bashcpp/parse.yy"
                { yylhs.value.as < COMMAND* > () = yystack_[2].value.as < COMMAND* > (); }
#line 2063 "parse.cc"
    break;

  case 140: // list1: list1 AND_AND newline_list list1
#line 832 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), AND_AND); }
#line 2069 "parse.cc"
    break;

  case 141: // list1: list1 OR_OR newline_list list1
#line 834 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), OR_OR); }
#line 2075 "parse.cc"
    break;

  case 142: // list1: list1 '&' newline_list list1
#line 836 "../bashcpp/parse.yy"
                        {
			  if (yystack_[3].value.as < COMMAND* > ()->type == cm_connection)
			    yylhs.value.as < COMMAND* > () = connect_async_list (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), '&');
			  else
			    yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), '&');
			}
#line 2086 "parse.cc"
    break;

  case 143: // list1: list1 ';' newline_list list1
#line 843 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), ';'); }
#line 2092 "parse.cc"
    break;

  case 144: // list1: list1 '\n' newline_list list1
#line 845 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), ';'); }
#line 2098 "parse.cc"
    break;

  case 145: // list1: pipeline_command
#line 847 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 2104 "parse.cc"
    break;

  case 148: // list_terminator: '\n'
#line 855 "../bashcpp/parse.yy"
                { yylhs.value.as < int > () = '\n'; }
#line 2110 "parse.cc"
    break;

  case 149: // list_terminator: ';'
#line 857 "../bashcpp/parse.yy"
                { yylhs.value.as < int > () = ';'; }
#line 2116 "parse.cc"
    break;

  case 150: // list_terminator: yacc_EOF
#line 859 "../bashcpp/parse.yy"
                { yylhs.value.as < int > () = yacc_EOF; }
#line 2122 "parse.cc"
    break;

  case 153: // simple_list: simple_list1
#line 873 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > ();
			  if (need_here_doc)
			    gather_here_documents ();
			  if ((parser_state & PST_CMDSUBST) && current_token == shell_eof_token)
			    {
			      global_command = yystack_[0].value.as < COMMAND* > ();
			      eof_encountered = 0;
			      rewind_input_string ();
			      YYACCEPT;
			    }
			}
#line 2139 "parse.cc"
    break;

  case 154: // simple_list: simple_list1 '&'
#line 886 "../bashcpp/parse.yy"
                        {
			  if (yystack_[1].value.as < COMMAND* > ()->type == cm_connection)
			    yylhs.value.as < COMMAND* > () = connect_async_list (yystack_[1].value.as < COMMAND* > (), (COMMAND *)NULL, '&');
			  else
			    yylhs.value.as < COMMAND* > () = command_connect (yystack_[1].value.as < COMMAND* > (), (COMMAND *)NULL, '&');
			  if (need_here_doc)
			    gather_here_documents ();
			  if ((parser_state & PST_CMDSUBST) && current_token == shell_eof_token)
			    {
			      global_command = yystack_[1].value.as < COMMAND* > ();
			      eof_encountered = 0;
			      rewind_input_string ();
			      YYACCEPT;
			    }
			}
#line 2159 "parse.cc"
    break;

  case 155: // simple_list: simple_list1 ';'
#line 902 "../bashcpp/parse.yy"
                        {
			  yylhs.value.as < COMMAND* > () = yystack_[1].value.as < COMMAND* > ();
			  if (need_here_doc)
			    gather_here_documents ();
			  if ((parser_state & PST_CMDSUBST) && current_token == shell_eof_token)
			    {
			      global_command = yystack_[1].value.as < COMMAND* > ();
			      eof_encountered = 0;
			      rewind_input_string ();
			      YYACCEPT;
			    }
			}
#line 2176 "parse.cc"
    break;

  case 156: // simple_list1: simple_list1 AND_AND newline_list simple_list1
#line 917 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), AND_AND); }
#line 2182 "parse.cc"
    break;

  case 157: // simple_list1: simple_list1 OR_OR newline_list simple_list1
#line 919 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), OR_OR); }
#line 2188 "parse.cc"
    break;

  case 158: // simple_list1: simple_list1 '&' simple_list1
#line 921 "../bashcpp/parse.yy"
                        {
			  if (yystack_[2].value.as < COMMAND* > ()->type == cm_connection)
			    yylhs.value.as < COMMAND* > () = connect_async_list (yystack_[2].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), '&');
			  else
			    yylhs.value.as < COMMAND* > () = command_connect (yystack_[2].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), '&');
			}
#line 2199 "parse.cc"
    break;

  case 159: // simple_list1: simple_list1 ';' simple_list1
#line 928 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[2].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), ';'); }
#line 2205 "parse.cc"
    break;

  case 160: // simple_list1: pipeline_command
#line 931 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 2211 "parse.cc"
    break;

  case 161: // pipeline_command: pipeline
#line 935 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 2217 "parse.cc"
    break;

  case 162: // pipeline_command: BANG pipeline_command
#line 937 "../bashcpp/parse.yy"
                        {
			  if (yystack_[0].value.as < COMMAND* > ())
			    yystack_[0].value.as < COMMAND* > ()->flags ^= CMD_INVERT_RETURN;	/* toggle */
			  yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > ();
			}
#line 2227 "parse.cc"
    break;

  case 163: // pipeline_command: timespec pipeline_command
#line 943 "../bashcpp/parse.yy"
                        {
			  if (yystack_[0].value.as < COMMAND* > ())
			    yystack_[0].value.as < COMMAND* > ()->flags |= yystack_[1].value.as < int > ();
			  yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > ();
			}
#line 2237 "parse.cc"
    break;

  case 164: // pipeline_command: timespec list_terminator
#line 949 "../bashcpp/parse.yy"
                        {
			  ELEMENT x;

			  /* Boy, this is unclean.  `time' by itself can
			     time a null command.  We cheat and push a
			     newline back if the list_terminator was a newline
			     to avoid the double-newline problem (one to
			     terminate this, one to terminate the command) */
			  x.word = 0;
			  x.redirect = 0;
			  yylhs.value.as < COMMAND* > () = make_simple_command (x, (COMMAND *)NULL);
			  yylhs.value.as < COMMAND* > ()->flags |= yystack_[1].value.as < int > ();
			  /* XXX - let's cheat and push a newline back */
			  if (yystack_[0].value.as < int > () == '\n')
			    token_to_read = '\n';
			  else if (yystack_[0].value.as < int > () == ';')
			    token_to_read = ';';
			  parser_state &= ~PST_REDIRLIST;	/* make_simple_command sets this */
			}
#line 2261 "parse.cc"
    break;

  case 165: // pipeline_command: BANG list_terminator
#line 969 "../bashcpp/parse.yy"
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
			  yylhs.value.as < COMMAND* > () = make_simple_command (x, (COMMAND *)NULL);
			  yylhs.value.as < COMMAND* > ()->flags |= CMD_INVERT_RETURN;
			  /* XXX - let's cheat and push a newline back */
			  if (yystack_[0].value.as < int > () == '\n')
			    token_to_read = '\n';
			  if (yystack_[0].value.as < int > () == ';')
			    token_to_read = ';';
			  parser_state &= ~PST_REDIRLIST;	/* make_simple_command sets this */
			}
#line 2286 "parse.cc"
    break;

  case 166: // pipeline: pipeline '|' newline_list pipeline
#line 992 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), '|'); }
#line 2292 "parse.cc"
    break;

  case 167: // pipeline: pipeline BAR_AND newline_list pipeline
#line 994 "../bashcpp/parse.yy"
                        {
			  /* Make cmd1 |& cmd2 equivalent to cmd1 2>&1 | cmd2 */
			  COMMAND *tc;
			  REDIRECTEE rd, sd;
			  REDIRECT *r;

			  tc = yystack_[3].value.as < COMMAND* > ()->type == cm_simple ? (COMMAND *)yystack_[3].value.as < COMMAND* > ()->value.Simple : yystack_[3].value.as < COMMAND* > ();
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

			  yylhs.value.as < COMMAND* > () = command_connect (yystack_[3].value.as < COMMAND* > (), yystack_[0].value.as < COMMAND* > (), '|');
			}
#line 2319 "parse.cc"
    break;

  case 168: // pipeline: command
#line 1017 "../bashcpp/parse.yy"
                        { yylhs.value.as < COMMAND* > () = yystack_[0].value.as < COMMAND* > (); }
#line 2325 "parse.cc"
    break;

  case 169: // timespec: TIME
#line 1021 "../bashcpp/parse.yy"
                        { yylhs.value.as < int > () = CMD_TIME_PIPELINE; }
#line 2331 "parse.cc"
    break;

  case 170: // timespec: TIME TIMEOPT
#line 1023 "../bashcpp/parse.yy"
                        { yylhs.value.as < int > () = (CMD_TIME_PIPELINE | CMD_TIME_POSIX); }
#line 2337 "parse.cc"
    break;

  case 171: // timespec: TIME TIMEIGN
#line 1025 "../bashcpp/parse.yy"
                        { yylhs.value.as < int > () = (CMD_TIME_PIPELINE | CMD_TIME_POSIX); }
#line 2343 "parse.cc"
    break;

  case 172: // timespec: TIME TIMEOPT TIMEIGN
#line 1027 "../bashcpp/parse.yy"
                        { yylhs.value.as < int > () = (CMD_TIME_PIPELINE | CMD_TIME_POSIX); }
#line 2349 "parse.cc"
    break;


#line 2353 "parse.cc"

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









  const short parser::yypact_ninf_ = -204;

  const signed char parser::yytable_ninf_ = -1;

  const short
  parser::yypact_[] =
  {
     313,   108,  -204,    -6,     8,     2,  -204,  -204,    10,   513,
      17,   363,   153,   -21,  -204,   593,   606,  -204,    14,    26,
     113,    41,   127,    72,    85,    92,    95,    98,  -204,  -204,
     100,   105,  -204,  -204,    65,  -204,  -204,   551,  -204,   572,
    -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,
    -204,   146,   140,  -204,    67,   363,  -204,  -204,  -204,   133,
     413,  -204,    93,    55,   104,   156,   161,    11,    45,   551,
     572,   163,  -204,  -204,  -204,  -204,  -204,   167,  -204,   152,
     208,   217,   129,   220,   150,   221,   223,   225,   233,   234,
     238,   239,   158,   240,   162,   241,   243,   244,   252,   253,
    -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,
    -204,  -204,  -204,  -204,  -204,  -204,   194,   227,  -204,  -204,
    -204,  -204,   572,  -204,  -204,  -204,  -204,  -204,   463,   463,
    -204,  -204,  -204,  -204,  -204,  -204,  -204,    -7,  -204,    59,
    -204,    52,  -204,  -204,  -204,  -204,    62,  -204,  -204,  -204,
     235,   572,  -204,   572,   572,  -204,  -204,  -204,  -204,  -204,
    -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,
    -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,
    -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,  -204,   413,
     413,   191,   191,   245,   245,   203,  -204,  -204,  -204,  -204,
    -204,  -204,    37,  -204,   176,  -204,   270,   228,    76,    79,
    -204,   176,  -204,   278,   282,   563,  -204,   572,   572,   563,
    -204,  -204,    67,    67,  -204,  -204,  -204,   291,   413,   413,
     413,   413,   413,   294,   175,  -204,    28,  -204,  -204,   292,
    -204,   187,  -204,   250,  -204,  -204,  -204,  -204,  -204,  -204,
     295,   413,   187,  -204,   251,  -204,  -204,  -204,   563,  -204,
     304,   314,  -204,  -204,  -204,   196,   196,   196,  -204,  -204,
    -204,  -204,   179,    38,  -204,  -204,   296,   -28,   302,   274,
    -204,  -204,  -204,    87,  -204,   318,   276,   322,   280,  -204,
      -7,  -204,   111,  -204,  -204,  -204,  -204,  -204,  -204,  -204,
    -204,    39,   319,  -204,  -204,  -204,   114,  -204,  -204,  -204,
    -204,  -204,  -204,   115,  -204,  -204,   226,  -204,  -204,  -204,
     413,  -204,  -204,   329,   288,  -204,  -204,   332,   297,  -204,
    -204,  -204,   413,   338,   303,  -204,  -204,   339,   305,  -204,
    -204,  -204,  -204,  -204,  -204,  -204
  };

  const unsigned char
  parser::yydefact_[] =
  {
       0,     0,   151,     0,     0,     0,   151,   151,     0,     0,
       0,     0,   169,    53,    54,     0,     0,   115,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     3,     6,
       0,     0,   151,   151,     0,    55,    58,    60,   168,    61,
      65,    75,    69,    66,    63,    71,    64,    70,    72,    73,
      74,     0,   153,   160,   161,     0,     4,     5,   135,     0,
       0,   151,   151,     0,   151,     0,     0,     0,    53,   110,
     106,     0,   149,   148,   150,   165,   162,   170,   171,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      15,    24,    39,    33,    48,    30,    42,    36,    45,    27,
      51,    52,    21,    18,     9,    10,     0,     0,     1,    53,
      59,    56,    62,   146,   147,     2,   151,   151,   154,   155,
     151,   151,   164,   163,   151,   152,   134,   136,   145,     0,
     151,     0,   151,   151,   151,   151,     0,   151,   151,   151,
     151,   103,   101,   108,   107,   116,   172,   151,    17,    26,
      41,    35,    50,    32,    44,    38,    47,    29,    23,    20,
      13,    14,    16,    25,    40,    34,    49,    31,    43,    37,
      46,    28,    22,    19,    11,    12,   114,   105,    57,     0,
       0,   158,   159,     0,     0,     0,   151,   151,   151,   151,
     151,   151,     0,   151,     0,   151,     0,     0,     0,     0,
     151,     0,   151,     0,     0,     0,   151,   104,   109,     0,
     156,   157,   167,   166,   151,   151,   111,     0,     0,     0,
     138,   139,   137,     0,   120,   151,     0,   151,   151,     0,
       7,     0,   151,     0,    86,    87,   151,   151,   151,   151,
       0,     0,     0,   151,     0,    67,    68,   102,     0,    99,
       0,     0,   113,   140,   141,   142,   143,   144,    98,   126,
     128,   130,   121,     0,    96,   132,     0,     0,     0,     0,
      76,     8,   151,     0,    77,     0,     0,     0,     0,    88,
       0,   151,     0,    89,   100,   112,   151,   127,   129,   131,
      97,     0,     0,   151,    78,    79,     0,   151,   151,    84,
      85,    90,    91,     0,   151,   151,   117,   151,   133,   122,
     123,   151,   151,     0,     0,   151,   151,     0,     0,   151,
     119,   124,   125,     0,     0,    82,    83,     0,     0,    94,
      95,   118,    80,    81,    92,    93
  };

  const short
  parser::yypgoto_[] =
  {
    -204,  -204,   117,   -37,   -19,   -67,   353,  -204,    -8,  -204,
    -204,  -204,  -204,  -204,  -184,  -204,  -204,  -204,  -204,  -204,
    -204,    53,  -204,   142,  -204,   102,  -203,    -2,  -204,   283,
    -204,   -47,   -49,  -204,  -118,     6,    47,  -204
  };

  const short
  parser::yydefgoto_[] =
  {
       0,    34,   241,    35,    36,   122,    37,    38,    39,    40,
      41,    42,    43,    44,   152,    45,    46,    47,    48,    49,
      50,   227,   233,   234,   235,   277,    58,   117,   136,   137,
     125,    75,    60,    51,    52,   138,    54,    55
  };

  const short
  parser::yytable_[] =
  {
      59,    70,   121,   154,    65,    66,    53,   250,   132,   254,
     191,   192,   139,   141,     2,   146,   144,    76,   120,     3,
      61,     4,     5,     6,     7,   302,   196,   197,    64,    10,
     116,   257,   303,   121,    62,   259,    67,   274,    79,    63,
     100,    17,   198,   199,   200,   287,   288,   300,     2,    71,
     120,   237,   101,     3,   275,     4,     5,     6,     7,   151,
     153,   133,   149,    10,   275,   118,   203,   105,    32,   142,
     150,   220,   221,   204,   294,    17,   210,   189,   190,   135,
     201,   193,   194,   211,   217,   188,   218,   276,   135,   135,
     246,   202,   302,   248,   238,   208,   209,   276,   109,   317,
     215,   307,    32,   135,    33,    72,    73,    74,   219,   205,
     135,   110,   143,   135,   121,   130,   121,   188,   111,   212,
     131,   112,   337,   338,   113,   314,   114,   135,   321,   325,
     135,   115,   195,   247,    53,    53,   249,   134,   135,   102,
     206,   207,   103,   140,   308,   213,   214,   228,   229,   230,
     231,   232,   236,   106,   145,   160,   107,   242,   161,    56,
      57,   251,   135,   251,   253,   135,   135,   258,   315,   104,
     147,   322,   326,   126,   127,   148,   164,    77,    78,   165,
     188,   188,   155,   108,   174,   162,   273,   175,   178,   128,
     129,   179,   156,   283,   282,    53,    53,   123,   124,   251,
     251,   239,   240,   243,   292,   291,   166,   151,   224,   225,
     226,   151,   157,   281,   176,   269,   270,   271,   180,   297,
     298,   299,   260,   261,   126,   127,    72,    73,    74,   196,
     197,   329,   225,   306,   158,   278,   279,    72,    73,    74,
     222,   223,   313,   159,   285,   286,   163,   167,     2,   168,
     151,   169,   186,     3,   320,     4,     5,     6,     7,   170,
     171,     8,     9,    10,   172,   173,   177,   181,   332,   182,
     183,    13,    14,    15,    16,    17,   251,   251,   184,   185,
      18,    19,    20,    21,    22,   244,   245,   187,    23,    24,
      25,    26,    27,   255,   316,   216,   135,   256,   262,    30,
      31,   319,    32,   268,    33,   323,   324,   280,   284,   293,
     289,   295,   327,   328,     1,   331,     2,   304,   296,   333,
     334,     3,   275,     4,     5,     6,     7,   341,   252,     8,
       9,    10,   305,   309,   310,    11,    12,   311,   312,    13,
      14,    15,    16,    17,   335,   318,   336,   339,    18,    19,
      20,    21,    22,   342,   344,   340,    23,    24,    25,    26,
      27,   343,    69,   345,    28,    29,     2,    30,    31,   330,
      32,     3,    33,     4,     5,     6,     7,   272,   301,     8,
       9,    10,     0,     0,     0,    11,    12,     0,     0,    13,
      14,    15,    16,    17,     0,     0,     0,     0,    18,    19,
      20,    21,    22,     0,     0,     0,    23,    24,    25,    26,
      27,     0,     0,    72,    73,    74,     2,    30,    31,     0,
      32,     3,    33,     4,     5,     6,     7,     0,     0,     8,
       9,    10,     0,     0,     0,    11,    12,     0,     0,    13,
      14,    15,    16,    17,     0,     0,     0,     0,    18,    19,
      20,    21,    22,     0,     0,     0,    23,    24,    25,    26,
      27,     0,     0,     0,   135,     0,     2,    30,    31,     0,
      32,     3,    33,     4,     5,     6,     7,     0,     0,     8,
       9,    10,     0,     0,     0,    11,    12,     0,     0,    13,
      14,    15,    16,    17,     0,     0,     0,     0,    18,    19,
      20,    21,    22,     0,     0,     0,    23,    24,    25,    26,
      27,   263,   264,   265,   266,   267,     2,    30,    31,     0,
      32,     3,    33,     4,     5,     6,     7,     0,     0,     0,
       0,    10,     0,     0,   290,     0,     0,     0,     0,    68,
      14,    15,    16,    17,     0,     0,     0,     0,    18,    19,
      20,    21,    22,     0,     0,     0,    23,    24,    25,    26,
      27,     0,     0,     0,     0,     0,     2,    30,    31,     0,
      32,     3,    33,     4,     5,     6,     7,   119,    14,    15,
      16,    10,     0,     0,     0,     0,    18,    19,    20,    21,
      22,     0,     0,    17,    23,    24,    25,    26,    27,     0,
      15,    16,     0,     0,     0,    30,    31,    18,    19,    20,
      21,    22,     0,     0,   135,    23,    24,    25,    26,    27,
      32,     0,    33,     0,     0,     0,    30,    31,    80,    81,
      82,    83,    84,     0,     0,     0,    85,     0,     0,    86,
      87,    90,    91,    92,    93,    94,     0,    88,    89,    95,
       0,     0,    96,    97,     0,     0,     0,     0,     0,     0,
      98,    99
  };

  const short
  parser::yycheck_[] =
  {
       2,     9,    39,    70,     6,     7,     0,   210,    55,   212,
     128,   129,    61,    62,     3,    64,    63,    11,    37,     8,
      26,    10,    11,    12,    13,    53,    33,    34,    26,    18,
      32,   215,    60,    70,    26,   219,    26,     9,    59,    31,
      26,    30,    49,    50,    51,   248,   249,     9,     3,    32,
      69,    14,    26,     8,    26,    10,    11,    12,    13,    67,
      68,    55,    51,    18,    26,     0,    14,    26,    57,    14,
      59,   189,   190,    21,   258,    30,    14,   126,   127,    51,
      21,   130,   131,    21,   151,   122,   153,    59,    51,    51,
      14,   140,    53,    14,    57,   144,   145,    59,    26,    60,
     149,    14,    57,    51,    59,    50,    51,    52,   157,    57,
      51,    26,    57,    51,   151,    48,   153,   154,    26,    57,
      53,    26,   325,   326,    26,    14,    26,    51,    14,    14,
      51,    26,   134,    57,   128,   129,    57,     4,    51,    26,
     142,   143,    29,    50,    57,   147,   148,   196,   197,   198,
     199,   200,   201,    26,    50,    26,    29,   204,    29,    51,
      52,   210,    51,   212,   211,    51,    51,   216,    57,    56,
      14,    57,    57,    33,    34,    14,    26,    24,    25,    29,
     217,   218,    19,    56,    26,    56,   235,    29,    26,    49,
      50,    29,    25,   242,   241,   189,   190,    51,    52,   248,
     249,   203,    26,   205,   253,   252,    56,   215,     5,     6,
       7,   219,    60,    26,    56,    40,    41,    42,    56,    40,
      41,    42,   224,   225,    33,    34,    50,    51,    52,    33,
      34,     5,     6,   282,    26,   237,   238,    50,    51,    52,
     193,   194,   291,    26,   246,   247,    26,    26,     3,    26,
     258,    26,    58,     8,   303,    10,    11,    12,    13,    26,
      26,    16,    17,    18,    26,    26,    26,    26,   317,    26,
      26,    26,    27,    28,    29,    30,   325,   326,    26,    26,
      35,    36,    37,    38,    39,    15,    58,    60,    43,    44,
      45,    46,    47,    15,   296,    60,    51,    15,     7,    54,
      55,   303,    57,     9,    59,   307,   308,    15,    58,    58,
      15,     7,   314,   315,     1,   317,     3,    15,     4,   321,
     322,     8,    26,    10,    11,    12,    13,   329,   211,    16,
      17,    18,    58,    15,    58,    22,    23,    15,    58,    26,
      27,    28,    29,    30,    15,    26,    58,    15,    35,    36,
      37,    38,    39,    15,    15,    58,    43,    44,    45,    46,
      47,    58,     9,    58,    51,    52,     3,    54,    55,   316,
      57,     8,    59,    10,    11,    12,    13,   235,   276,    16,
      17,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      27,    28,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    50,    51,    52,     3,    54,    55,    -1,
      57,     8,    59,    10,    11,    12,    13,    -1,    -1,    16,
      17,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      27,    28,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    51,    -1,     3,    54,    55,    -1,
      57,     8,    59,    10,    11,    12,    13,    -1,    -1,    16,
      17,    18,    -1,    -1,    -1,    22,    23,    -1,    -1,    26,
      27,    28,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    43,    44,    45,    46,
      47,   228,   229,   230,   231,   232,     3,    54,    55,    -1,
      57,     8,    59,    10,    11,    12,    13,    -1,    -1,    -1,
      -1,    18,    -1,    -1,   251,    -1,    -1,    -1,    -1,    26,
      27,    28,    29,    30,    -1,    -1,    -1,    -1,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    43,    44,    45,    46,
      47,    -1,    -1,    -1,    -1,    -1,     3,    54,    55,    -1,
      57,     8,    59,    10,    11,    12,    13,    26,    27,    28,
      29,    18,    -1,    -1,    -1,    -1,    35,    36,    37,    38,
      39,    -1,    -1,    30,    43,    44,    45,    46,    47,    -1,
      28,    29,    -1,    -1,    -1,    54,    55,    35,    36,    37,
      38,    39,    -1,    -1,    51,    43,    44,    45,    46,    47,
      57,    -1,    59,    -1,    -1,    -1,    54,    55,    35,    36,
      37,    38,    39,    -1,    -1,    -1,    43,    -1,    -1,    46,
      47,    35,    36,    37,    38,    39,    -1,    54,    55,    43,
      -1,    -1,    46,    47,    -1,    -1,    -1,    -1,    -1,    -1,
      54,    55
  };

  const signed char
  parser::yystos_[] =
  {
       0,     1,     3,     8,    10,    11,    12,    13,    16,    17,
      18,    22,    23,    26,    27,    28,    29,    30,    35,    36,
      37,    38,    39,    43,    44,    45,    46,    47,    51,    52,
      54,    55,    57,    59,    62,    64,    65,    67,    68,    69,
      70,    71,    72,    73,    74,    76,    77,    78,    79,    80,
      81,    94,    95,    96,    97,    98,    51,    52,    87,    88,
      93,    26,    26,    31,    26,    88,    88,    26,    26,    67,
      69,    32,    50,    51,    52,    92,    96,    24,    25,    59,
      35,    36,    37,    38,    39,    43,    46,    47,    54,    55,
      35,    36,    37,    38,    39,    43,    46,    47,    54,    55,
      26,    26,    26,    29,    56,    26,    26,    29,    56,    26,
      26,    26,    26,    26,    26,    26,    88,    88,     0,    26,
      65,    64,    66,    51,    52,    91,    33,    34,    49,    50,
      48,    53,    92,    96,     4,    51,    89,    90,    96,    93,
      50,    93,    14,    57,    92,    50,    93,    14,    14,    51,
      59,    69,    75,    69,    66,    19,    25,    60,    26,    26,
      26,    29,    56,    26,    26,    29,    56,    26,    26,    26,
      26,    26,    26,    26,    26,    29,    56,    26,    26,    29,
      56,    26,    26,    26,    26,    26,    58,    60,    64,    93,
      93,    95,    95,    93,    93,    88,    33,    34,    49,    50,
      51,    21,    93,    14,    21,    57,    88,    88,    93,    93,
      14,    21,    57,    88,    88,    93,    60,    66,    66,    93,
      95,    95,    97,    97,     5,     6,     7,    82,    93,    93,
      93,    93,    93,    83,    84,    85,    93,    14,    57,    88,
      26,    63,    92,    88,    15,    58,    14,    57,    14,    57,
      87,    93,    63,    92,    87,    15,    15,    75,    93,    75,
      88,    88,     7,    90,    90,    90,    90,    90,     9,    40,
      41,    42,    84,    93,     9,    26,    59,    86,    88,    88,
      15,    26,    92,    93,    58,    88,    88,    87,    87,    15,
      90,    92,    93,    58,    75,     7,     4,    40,    41,    42,
       9,    86,    53,    60,    15,    58,    93,    14,    57,    15,
      58,    15,    58,    93,    14,    57,    88,    60,    26,    88,
      93,    14,    57,    88,    88,    14,    57,    88,    88,     5,
      82,    88,    93,    88,    88,    15,    58,    87,    87,    15,
      58,    88,    15,    58,    15,    58
  };

  const signed char
  parser::yyr1_[] =
  {
       0,    61,    62,    62,    62,    62,    62,    63,    63,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    64,    64,    64,    64,    64,    64,    64,
      64,    64,    64,    65,    65,    65,    66,    66,    67,    67,
      68,    68,    68,    68,    68,    69,    69,    69,    69,    69,
      69,    69,    69,    69,    69,    69,    70,    70,    70,    70,
      70,    70,    70,    70,    71,    71,    71,    71,    72,    72,
      72,    72,    72,    72,    72,    72,    73,    73,    73,    74,
      74,    74,    74,    75,    75,    76,    77,    77,    77,    77,
      77,    78,    78,    78,    79,    80,    81,    82,    82,    82,
      83,    83,    84,    84,    84,    84,    85,    85,    85,    85,
      85,    85,    86,    86,    87,    88,    88,    89,    89,    89,
      90,    90,    90,    90,    90,    90,    91,    91,    92,    92,
      92,    93,    93,    94,    94,    94,    95,    95,    95,    95,
      95,    96,    96,    96,    96,    96,    97,    97,    97,    98,
      98,    98,    98
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     2,     1,     2,     2,     1,     1,     2,     2,
       2,     3,     3,     3,     3,     2,     3,     3,     2,     3,
       3,     2,     3,     3,     2,     3,     3,     2,     3,     3,
       2,     3,     3,     2,     3,     3,     2,     3,     3,     2,
       3,     3,     2,     3,     3,     2,     3,     3,     2,     3,
       3,     2,     2,     1,     1,     1,     1,     2,     1,     2,
       1,     1,     2,     1,     1,     1,     1,     5,     5,     1,
       1,     1,     1,     1,     1,     1,     6,     6,     7,     7,
      10,    10,     9,     9,     7,     7,     5,     5,     6,     6,
       7,     7,    10,    10,     9,     9,     6,     7,     6,     5,
       6,     3,     5,     1,     2,     3,     2,     3,     3,     4,
       2,     5,     7,     6,     3,     1,     3,     4,     6,     5,
       1,     2,     4,     4,     5,     5,     2,     3,     2,     3,
       2,     3,     1,     3,     2,     1,     2,     3,     3,     3,
       4,     4,     4,     4,     4,     1,     1,     1,     1,     1,
       1,     0,     2,     1,     2,     2,     4,     4,     3,     3,
       1,     1,     2,     2,     2,     2,     4,     4,     1,     1,
       2,     2,     3
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
  "LESS_GREATER", "GREATER_BAR", "BAR_AND", "'&'", "';'", "'\\n'",
  "yacc_EOF", "'|'", "'>'", "'<'", "'-'", "'{'", "'}'", "'('", "')'",
  "$accept", "inputunit", "word_list", "redirection",
  "simple_command_element", "redirection_list", "simple_command",
  "command", "shell_command", "for_command", "arith_for_command",
  "select_command", "case_command", "function_def", "function_body",
  "subshell", "coproc", "if_command", "group_command", "arith_command",
  "cond_command", "elif_clause", "case_clause", "pattern_list",
  "case_clause_sequence", "pattern", "list", "compound_list", "list0",
  "list1", "simple_list_terminator", "list_terminator", "newline_list",
  "simple_list", "simple_list1", "pipeline_command", "pipeline",
  "timespec", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
  parser::yyrline_[] =
  {
       0,    88,    88,    99,   108,   123,   140,   150,   152,   156,
     162,   168,   174,   180,   186,   192,   198,   204,   210,   216,
     222,   228,   234,   240,   246,   253,   260,   267,   274,   281,
     288,   294,   300,   306,   312,   318,   324,   330,   336,   342,
     348,   354,   360,   366,   372,   378,   384,   390,   396,   402,
     408,   414,   420,   428,   430,   432,   436,   440,   451,   453,
     457,   459,   461,   477,   479,   483,   485,   487,   489,   491,
     493,   495,   497,   499,   501,   503,   507,   512,   517,   522,
     527,   532,   537,   542,   549,   555,   561,   567,   575,   580,
     585,   590,   595,   600,   605,   610,   617,   622,   627,   634,
     636,   638,   640,   644,   646,   677,   684,   689,   706,   711,
     728,   735,   737,   739,   744,   748,   752,   756,   758,   760,
     764,   765,   769,   771,   773,   775,   779,   781,   783,   785,
     787,   789,   793,   795,   804,   812,   813,   819,   820,   827,
     831,   833,   835,   842,   844,   846,   850,   851,   854,   856,
     858,   862,   863,   872,   885,   901,   916,   918,   920,   927,
     930,   934,   936,   942,   948,   968,   991,   993,  1016,  1020,
    1022,  1024,  1026
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
#line 2959 "parse.cc"

#line 1029 "../bashcpp/parse.yy"

