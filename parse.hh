// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton interface for Bison LALR(1) parsers in C++

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


/**
 ** \file y.tab.h
 ** Define the bash::parser class.
 */

// C++ LALR(1) parser skeleton written by Akim Demaille.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.

#ifndef YY_YY__BASHCPP_PARSE_HH_INCLUDED
# define YY_YY__BASHCPP_PARSE_HH_INCLUDED


# include <cstdlib> // std::abort
# include <iostream>
# include <stdexcept>
# include <string>
# include <vector>

#if defined __cplusplus
# define YY_CPLUSPLUS __cplusplus
#else
# define YY_CPLUSPLUS 199711L
#endif

// Support move semantics when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_MOVE           std::move
# define YY_MOVE_OR_COPY   move
# define YY_MOVE_REF(Type) Type&&
# define YY_RVREF(Type)    Type&&
# define YY_COPY(Type)     Type
#else
# define YY_MOVE
# define YY_MOVE_OR_COPY   copy
# define YY_MOVE_REF(Type) Type&
# define YY_RVREF(Type)    const Type&
# define YY_COPY(Type)     const Type&
#endif

// Support noexcept when possible.
#if 201103L <= YY_CPLUSPLUS
# define YY_NOEXCEPT noexcept
# define YY_NOTHROW
#else
# define YY_NOEXCEPT
# define YY_NOTHROW throw ()
#endif

// Support constexpr when possible.
#if 201703 <= YY_CPLUSPLUS
# define YY_CONSTEXPR constexpr
#else
# define YY_CONSTEXPR
#endif



#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

#line 26 "../bashcpp/parse.yy"
namespace bash {
#line 183 "../bashcpp/parse.hh"




  /// A Bison parser.
  class parser
  {
  public:
#ifdef YYSTYPE
# ifdef __GNUC__
#  pragma GCC message "bison: do not #define YYSTYPE in C++, use %define api.value.type"
# endif
    typedef YYSTYPE value_type;
#else
  /// A buffer to store and retrieve objects.
  ///
  /// Sort of a variant, but does not keep track of the nature
  /// of the stored data, since that knowledge is available
  /// via the current parser state.
  class value_type
  {
  public:
    /// Type of *this.
    typedef value_type self_type;

    /// Empty construction.
    value_type () YY_NOEXCEPT
      : yyraw_ ()
    {}

    /// Construct and fill.
    template <typename T>
    value_type (YY_RVREF (T) t)
    {
      new (yyas_<T> ()) T (YY_MOVE (t));
    }

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    value_type (const self_type&) = delete;
    /// Non copyable.
    self_type& operator= (const self_type&) = delete;
#endif

    /// Destruction, allowed only if empty.
    ~value_type () YY_NOEXCEPT
    {}

# if 201103L <= YY_CPLUSPLUS
    /// Instantiate a \a T in here from \a t.
    template <typename T, typename... U>
    T&
    emplace (U&&... u)
    {
      return *new (yyas_<T> ()) T (std::forward <U>(u)...);
    }
# else
    /// Instantiate an empty \a T in here.
    template <typename T>
    T&
    emplace ()
    {
      return *new (yyas_<T> ()) T ();
    }

    /// Instantiate a \a T in here from \a t.
    template <typename T>
    T&
    emplace (const T& t)
    {
      return *new (yyas_<T> ()) T (t);
    }
# endif

    /// Instantiate an empty \a T in here.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build ()
    {
      return emplace<T> ();
    }

    /// Instantiate a \a T in here from \a t.
    /// Obsolete, use emplace.
    template <typename T>
    T&
    build (const T& t)
    {
      return emplace<T> (t);
    }

    /// Accessor to a built \a T.
    template <typename T>
    T&
    as () YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Const accessor to a built \a T (for %printer).
    template <typename T>
    const T&
    as () const YY_NOEXCEPT
    {
      return *yyas_<T> ();
    }

    /// Swap the content with \a that, of same type.
    ///
    /// Both variants must be built beforehand, because swapping the actual
    /// data requires reading it (with as()), and this is not possible on
    /// unconstructed variants: it would require some dynamic testing, which
    /// should not be the variant's responsibility.
    /// Swapping between built and (possibly) non-built is done with
    /// self_type::move ().
    template <typename T>
    void
    swap (self_type& that) YY_NOEXCEPT
    {
      std::swap (as<T> (), that.as<T> ());
    }

    /// Move the content of \a that to this.
    ///
    /// Destroys \a that.
    template <typename T>
    void
    move (self_type& that)
    {
# if 201103L <= YY_CPLUSPLUS
      emplace<T> (std::move (that.as<T> ()));
# else
      emplace<T> ();
      swap<T> (that);
# endif
      that.destroy<T> ();
    }

# if 201103L <= YY_CPLUSPLUS
    /// Move the content of \a that to this.
    template <typename T>
    void
    move (self_type&& that)
    {
      emplace<T> (std::move (that.as<T> ()));
      that.destroy<T> ();
    }
#endif

    /// Copy the content of \a that to this.
    template <typename T>
    void
    copy (const self_type& that)
    {
      emplace<T> (that.as<T> ());
    }

    /// Destroy the stored \a T.
    template <typename T>
    void
    destroy ()
    {
      as<T> ().~T ();
    }

  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    value_type (const self_type&);
    /// Non copyable.
    self_type& operator= (const self_type&);
#endif

    /// Accessor to raw memory as \a T.
    template <typename T>
    T*
    yyas_ () YY_NOEXCEPT
    {
      void *yyp = yyraw_;
      return static_cast<T*> (yyp);
     }

    /// Const accessor to raw memory as \a T.
    template <typename T>
    const T*
    yyas_ () const YY_NOEXCEPT
    {
      const void *yyp = yyraw_;
      return static_cast<const T*> (yyp);
     }

    /// An auxiliary type to compute the largest semantic type.
    union union_type
    {
      // COND_CMD
      // inputunit
      // simple_command
      // command
      // shell_command
      // for_command
      // arith_for_command
      // select_command
      // case_command
      // function_def
      // function_body
      // subshell
      // coproc
      // if_command
      // group_command
      // arith_command
      // cond_command
      // elif_clause
      // list
      // compound_list
      // list0
      // list1
      // simple_list
      // simple_list1
      // pipeline_command
      // pipeline
      char dummy1[sizeof (COMMAND*)];

      // simple_command_element
      char dummy2[sizeof (ELEMENT)];

      // case_clause
      // pattern_list
      // case_clause_sequence
      char dummy3[sizeof (PATTERN_LIST*)];

      // redirection
      // redirection_list
      char dummy4[sizeof (REDIRECT*)];

      // WORD
      // ASSIGNMENT_WORD
      // REDIR_WORD
      char dummy5[sizeof (WORD_DESC*)];

      // ARITH_CMD
      // ARITH_FOR_EXPRS
      // word_list
      // pattern
      char dummy6[sizeof (WORD_LIST*)];

      // NUMBER
      // list_terminator
      // timespec
      char dummy7[sizeof (int64_t)];
    };

    /// The size of the largest semantic type.
    enum { size = sizeof (union_type) };

    /// A buffer to store semantic values.
    union
    {
      /// Strongest alignment constraints.
      long double yyalign_me_;
      /// A buffer large enough to store any of the semantic values.
      char yyraw_[size];
    };
  };

#endif
    /// Backward compatibility (Bison 3.8).
    typedef value_type semantic_type;


    /// Syntax errors thrown from user actions.
    struct syntax_error : std::runtime_error
    {
      syntax_error (const std::string& m)
        : std::runtime_error (m)
      {}

      syntax_error (const syntax_error& s)
        : std::runtime_error (s.what ())
      {}

      ~syntax_error () YY_NOEXCEPT YY_NOTHROW;
    };

    /// Token kinds.
    struct token
    {
      enum token_kind_type
      {
        YYEMPTY = -2,
    YYEOF = 0,                     // "end of file"
    YYerror = 256,                 // error
    YYUNDEF = 257,                 // "invalid token"
    IF = 258,                      // IF
    THEN = 259,                    // THEN
    ELSE = 260,                    // ELSE
    ELIF = 261,                    // ELIF
    FI = 262,                      // FI
    CASE = 263,                    // CASE
    ESAC = 264,                    // ESAC
    FOR = 265,                     // FOR
    SELECT = 266,                  // SELECT
    WHILE = 267,                   // WHILE
    UNTIL = 268,                   // UNTIL
    DO = 269,                      // DO
    DONE = 270,                    // DONE
    FUNCTION = 271,                // FUNCTION
    COPROC = 272,                  // COPROC
    COND_START = 273,              // COND_START
    COND_END = 274,                // COND_END
    COND_ERROR = 275,              // COND_ERROR
    IN = 276,                      // IN
    BANG = 277,                    // BANG
    TIME = 278,                    // TIME
    TIMEOPT = 279,                 // TIMEOPT
    TIMEIGN = 280,                 // TIMEIGN
    WORD = 281,                    // WORD
    ASSIGNMENT_WORD = 282,         // ASSIGNMENT_WORD
    REDIR_WORD = 283,              // REDIR_WORD
    NUMBER = 284,                  // NUMBER
    ARITH_CMD = 285,               // ARITH_CMD
    ARITH_FOR_EXPRS = 286,         // ARITH_FOR_EXPRS
    COND_CMD = 287,                // COND_CMD
    AND_AND = 288,                 // AND_AND
    OR_OR = 289,                   // OR_OR
    GREATER_GREATER = 290,         // GREATER_GREATER
    LESS_LESS = 291,               // LESS_LESS
    LESS_AND = 292,                // LESS_AND
    LESS_LESS_LESS = 293,          // LESS_LESS_LESS
    GREATER_AND = 294,             // GREATER_AND
    SEMI_SEMI = 295,               // SEMI_SEMI
    SEMI_AND = 296,                // SEMI_AND
    SEMI_SEMI_AND = 297,           // SEMI_SEMI_AND
    LESS_LESS_MINUS = 298,         // LESS_LESS_MINUS
    AND_GREATER = 299,             // AND_GREATER
    AND_GREATER_GREATER = 300,     // AND_GREATER_GREATER
    LESS_GREATER = 301,            // LESS_GREATER
    GREATER_BAR = 302,             // GREATER_BAR
    BAR_AND = 303,                 // BAR_AND
    yacc_EOF = 304                 // yacc_EOF
      };
      /// Backward compatibility alias (Bison 3.6).
      typedef token_kind_type yytokentype;
    };

    /// Token kind, as returned by yylex.
    typedef token::token_kind_type token_kind_type;

    /// Backward compatibility alias (Bison 3.6).
    typedef token_kind_type token_type;

    /// Symbol kinds.
    struct symbol_kind
    {
      enum symbol_kind_type
      {
        YYNTOKENS = 61, ///< Number of tokens.
        S_YYEMPTY = -2,
        S_YYEOF = 0,                             // "end of file"
        S_YYerror = 1,                           // error
        S_YYUNDEF = 2,                           // "invalid token"
        S_IF = 3,                                // IF
        S_THEN = 4,                              // THEN
        S_ELSE = 5,                              // ELSE
        S_ELIF = 6,                              // ELIF
        S_FI = 7,                                // FI
        S_CASE = 8,                              // CASE
        S_ESAC = 9,                              // ESAC
        S_FOR = 10,                              // FOR
        S_SELECT = 11,                           // SELECT
        S_WHILE = 12,                            // WHILE
        S_UNTIL = 13,                            // UNTIL
        S_DO = 14,                               // DO
        S_DONE = 15,                             // DONE
        S_FUNCTION = 16,                         // FUNCTION
        S_COPROC = 17,                           // COPROC
        S_COND_START = 18,                       // COND_START
        S_COND_END = 19,                         // COND_END
        S_COND_ERROR = 20,                       // COND_ERROR
        S_IN = 21,                               // IN
        S_BANG = 22,                             // BANG
        S_TIME = 23,                             // TIME
        S_TIMEOPT = 24,                          // TIMEOPT
        S_TIMEIGN = 25,                          // TIMEIGN
        S_WORD = 26,                             // WORD
        S_ASSIGNMENT_WORD = 27,                  // ASSIGNMENT_WORD
        S_REDIR_WORD = 28,                       // REDIR_WORD
        S_NUMBER = 29,                           // NUMBER
        S_ARITH_CMD = 30,                        // ARITH_CMD
        S_ARITH_FOR_EXPRS = 31,                  // ARITH_FOR_EXPRS
        S_COND_CMD = 32,                         // COND_CMD
        S_AND_AND = 33,                          // AND_AND
        S_OR_OR = 34,                            // OR_OR
        S_GREATER_GREATER = 35,                  // GREATER_GREATER
        S_LESS_LESS = 36,                        // LESS_LESS
        S_LESS_AND = 37,                         // LESS_AND
        S_LESS_LESS_LESS = 38,                   // LESS_LESS_LESS
        S_GREATER_AND = 39,                      // GREATER_AND
        S_SEMI_SEMI = 40,                        // SEMI_SEMI
        S_SEMI_AND = 41,                         // SEMI_AND
        S_SEMI_SEMI_AND = 42,                    // SEMI_SEMI_AND
        S_LESS_LESS_MINUS = 43,                  // LESS_LESS_MINUS
        S_AND_GREATER = 44,                      // AND_GREATER
        S_AND_GREATER_GREATER = 45,              // AND_GREATER_GREATER
        S_LESS_GREATER = 46,                     // LESS_GREATER
        S_GREATER_BAR = 47,                      // GREATER_BAR
        S_BAR_AND = 48,                          // BAR_AND
        S_49_ = 49,                              // '&'
        S_50_ = 50,                              // ';'
        S_51_n_ = 51,                            // '\n'
        S_yacc_EOF = 52,                         // yacc_EOF
        S_53_ = 53,                              // '|'
        S_54_ = 54,                              // '>'
        S_55_ = 55,                              // '<'
        S_56_ = 56,                              // '-'
        S_57_ = 57,                              // '{'
        S_58_ = 58,                              // '}'
        S_59_ = 59,                              // '('
        S_60_ = 60,                              // ')'
        S_YYACCEPT = 61,                         // $accept
        S_inputunit = 62,                        // inputunit
        S_word_list = 63,                        // word_list
        S_redirection = 64,                      // redirection
        S_simple_command_element = 65,           // simple_command_element
        S_redirection_list = 66,                 // redirection_list
        S_simple_command = 67,                   // simple_command
        S_command = 68,                          // command
        S_shell_command = 69,                    // shell_command
        S_for_command = 70,                      // for_command
        S_arith_for_command = 71,                // arith_for_command
        S_select_command = 72,                   // select_command
        S_case_command = 73,                     // case_command
        S_function_def = 74,                     // function_def
        S_function_body = 75,                    // function_body
        S_subshell = 76,                         // subshell
        S_coproc = 77,                           // coproc
        S_if_command = 78,                       // if_command
        S_group_command = 79,                    // group_command
        S_arith_command = 80,                    // arith_command
        S_cond_command = 81,                     // cond_command
        S_elif_clause = 82,                      // elif_clause
        S_case_clause = 83,                      // case_clause
        S_pattern_list = 84,                     // pattern_list
        S_case_clause_sequence = 85,             // case_clause_sequence
        S_pattern = 86,                          // pattern
        S_list = 87,                             // list
        S_compound_list = 88,                    // compound_list
        S_list0 = 89,                            // list0
        S_list1 = 90,                            // list1
        S_simple_list_terminator = 91,           // simple_list_terminator
        S_list_terminator = 92,                  // list_terminator
        S_newline_list = 93,                     // newline_list
        S_simple_list = 94,                      // simple_list
        S_simple_list1 = 95,                     // simple_list1
        S_pipeline_command = 96,                 // pipeline_command
        S_pipeline = 97,                         // pipeline
        S_timespec = 98                          // timespec
      };
    };

    /// (Internal) symbol kind.
    typedef symbol_kind::symbol_kind_type symbol_kind_type;

    /// The number of tokens.
    static const symbol_kind_type YYNTOKENS = symbol_kind::YYNTOKENS;

    /// A complete symbol.
    ///
    /// Expects its Base type to provide access to the symbol kind
    /// via kind ().
    ///
    /// Provide access to semantic value.
    template <typename Base>
    struct basic_symbol : Base
    {
      /// Alias to Base.
      typedef Base super_type;

      /// Default constructor.
      basic_symbol () YY_NOEXCEPT
        : value ()
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      basic_symbol (basic_symbol&& that)
        : Base (std::move (that))
        , value ()
      {
        switch (this->kind ())
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
        value.move< COMMAND* > (std::move (that.value));
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.move< ELEMENT > (std::move (that.value));
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.move< PATTERN_LIST* > (std::move (that.value));
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.move< REDIRECT* > (std::move (that.value));
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.move< WORD_DESC* > (std::move (that.value));
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.move< WORD_LIST* > (std::move (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.move< int64_t > (std::move (that.value));
        break;

      default:
        break;
    }

      }
#endif

      /// Copy constructor.
      basic_symbol (const basic_symbol& that);

      /// Constructors for typed symbols.
#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t)
        : Base (t)
      {}
#else
      basic_symbol (typename Base::kind_type t)
        : Base (t)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, COMMAND*&& v)
        : Base (t)
        , value (std::move (v))
      {}
#else
      basic_symbol (typename Base::kind_type t, const COMMAND*& v)
        : Base (t)
        , value (v)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, ELEMENT&& v)
        : Base (t)
        , value (std::move (v))
      {}
#else
      basic_symbol (typename Base::kind_type t, const ELEMENT& v)
        : Base (t)
        , value (v)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, PATTERN_LIST*&& v)
        : Base (t)
        , value (std::move (v))
      {}
#else
      basic_symbol (typename Base::kind_type t, const PATTERN_LIST*& v)
        : Base (t)
        , value (v)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, REDIRECT*&& v)
        : Base (t)
        , value (std::move (v))
      {}
#else
      basic_symbol (typename Base::kind_type t, const REDIRECT*& v)
        : Base (t)
        , value (v)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, WORD_DESC*&& v)
        : Base (t)
        , value (std::move (v))
      {}
#else
      basic_symbol (typename Base::kind_type t, const WORD_DESC*& v)
        : Base (t)
        , value (v)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, WORD_LIST*&& v)
        : Base (t)
        , value (std::move (v))
      {}
#else
      basic_symbol (typename Base::kind_type t, const WORD_LIST*& v)
        : Base (t)
        , value (v)
      {}
#endif

#if 201103L <= YY_CPLUSPLUS
      basic_symbol (typename Base::kind_type t, int64_t&& v)
        : Base (t)
        , value (std::move (v))
      {}
#else
      basic_symbol (typename Base::kind_type t, const int64_t& v)
        : Base (t)
        , value (v)
      {}
#endif

      /// Destroy the symbol.
      ~basic_symbol ()
      {
        clear ();
      }



      /// Destroy contents, and record that is empty.
      void clear () YY_NOEXCEPT
      {
        // User destructor.
        symbol_kind_type yykind = this->kind ();
        basic_symbol<Base>& yysym = *this;
        (void) yysym;
        switch (yykind)
        {
       default:
          break;
        }

        // Value type destructor.
switch (yykind)
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
        value.template destroy< COMMAND* > ();
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.template destroy< ELEMENT > ();
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.template destroy< PATTERN_LIST* > ();
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.template destroy< REDIRECT* > ();
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.template destroy< WORD_DESC* > ();
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.template destroy< WORD_LIST* > ();
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.template destroy< int64_t > ();
        break;

      default:
        break;
    }

        Base::clear ();
      }

#if YYDEBUG || 0
      /// The user-facing name of this symbol.
      const char *name () const YY_NOEXCEPT
      {
        return parser::symbol_name (this->kind ());
      }
#endif // #if YYDEBUG || 0


      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// Whether empty.
      bool empty () const YY_NOEXCEPT;

      /// Destructive move, \a s is emptied into this.
      void move (basic_symbol& s);

      /// The semantic value.
      value_type value;

    private:
#if YY_CPLUSPLUS < 201103L
      /// Assignment operator.
      basic_symbol& operator= (const basic_symbol& that);
#endif
    };

    /// Type access provider for token (enum) based symbols.
    struct by_kind
    {
      /// The symbol kind as needed by the constructor.
      typedef token_kind_type kind_type;

      /// Default constructor.
      by_kind () YY_NOEXCEPT;

#if 201103L <= YY_CPLUSPLUS
      /// Move constructor.
      by_kind (by_kind&& that) YY_NOEXCEPT;
#endif

      /// Copy constructor.
      by_kind (const by_kind& that) YY_NOEXCEPT;

      /// Constructor from (external) token numbers.
      by_kind (kind_type t) YY_NOEXCEPT;



      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_kind& that);

      /// The (internal) type number (corresponding to \a type).
      /// \a empty when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// Backward compatibility (Bison 3.6).
      symbol_kind_type type_get () const YY_NOEXCEPT;

      /// The symbol kind.
      /// \a S_YYEMPTY when empty.
      symbol_kind_type kind_;
    };

    /// Backward compatibility for a private implementation detail (Bison 3.6).
    typedef by_kind by_type;

    /// "External" symbols: returned by the scanner.
    struct symbol_type : basic_symbol<by_kind>
    {
      /// Superclass.
      typedef basic_symbol<by_kind> super_type;

      /// Empty symbol.
      symbol_type () YY_NOEXCEPT {}

      /// Constructor for valueless symbols, and symbols from each type.
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok)
        : super_type (token_kind_type (tok))
#else
      symbol_type (int tok)
        : super_type (token_kind_type (tok))
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, COMMAND* v)
        : super_type (token_kind_type (tok), std::move (v))
#else
      symbol_type (int tok, const COMMAND*& v)
        : super_type (token_kind_type (tok), v)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, WORD_DESC* v)
        : super_type (token_kind_type (tok), std::move (v))
#else
      symbol_type (int tok, const WORD_DESC*& v)
        : super_type (token_kind_type (tok), v)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, WORD_LIST* v)
        : super_type (token_kind_type (tok), std::move (v))
#else
      symbol_type (int tok, const WORD_LIST*& v)
        : super_type (token_kind_type (tok), v)
#endif
      {}
#if 201103L <= YY_CPLUSPLUS
      symbol_type (int tok, int64_t v)
        : super_type (token_kind_type (tok), std::move (v))
#else
      symbol_type (int tok, const int64_t& v)
        : super_type (token_kind_type (tok), v)
#endif
      {}
    };

    /// Build a parser object.
    parser ();
    virtual ~parser ();

#if 201103L <= YY_CPLUSPLUS
    /// Non copyable.
    parser (const parser&) = delete;
    /// Non copyable.
    parser& operator= (const parser&) = delete;
#endif

    /// Parse.  An alias for parse ().
    /// \returns  0 iff parsing succeeded.
    int operator() ();

    /// Parse.
    /// \returns  0 iff parsing succeeded.
    virtual int parse ();

#if YYDEBUG
    /// The current debugging stream.
    std::ostream& debug_stream () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging stream.
    void set_debug_stream (std::ostream &);

    /// Type for debugging levels.
    typedef int debug_level_type;
    /// The current debugging level.
    debug_level_type debug_level () const YY_ATTRIBUTE_PURE;
    /// Set the current debugging level.
    void set_debug_level (debug_level_type l);
#endif

    /// Report a syntax error.
    /// \param msg    a description of the syntax error.
    virtual void error (const std::string& msg);

    /// Report a syntax error.
    void error (const syntax_error& err);

#if YYDEBUG || 0
    /// The user-facing name of the symbol whose (internal) number is
    /// YYSYMBOL.  No bounds checking.
    static const char *symbol_name (symbol_kind_type yysymbol);
#endif // #if YYDEBUG || 0


    // Implementation of make_symbol for each token kind.
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYEOF ()
      {
        return symbol_type (token::YYEOF);
      }
#else
      static
      symbol_type
      make_YYEOF ()
      {
        return symbol_type (token::YYEOF);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYerror ()
      {
        return symbol_type (token::YYerror);
      }
#else
      static
      symbol_type
      make_YYerror ()
      {
        return symbol_type (token::YYerror);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_YYUNDEF ()
      {
        return symbol_type (token::YYUNDEF);
      }
#else
      static
      symbol_type
      make_YYUNDEF ()
      {
        return symbol_type (token::YYUNDEF);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IF ()
      {
        return symbol_type (token::IF);
      }
#else
      static
      symbol_type
      make_IF ()
      {
        return symbol_type (token::IF);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_THEN ()
      {
        return symbol_type (token::THEN);
      }
#else
      static
      symbol_type
      make_THEN ()
      {
        return symbol_type (token::THEN);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ELSE ()
      {
        return symbol_type (token::ELSE);
      }
#else
      static
      symbol_type
      make_ELSE ()
      {
        return symbol_type (token::ELSE);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ELIF ()
      {
        return symbol_type (token::ELIF);
      }
#else
      static
      symbol_type
      make_ELIF ()
      {
        return symbol_type (token::ELIF);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FI ()
      {
        return symbol_type (token::FI);
      }
#else
      static
      symbol_type
      make_FI ()
      {
        return symbol_type (token::FI);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_CASE ()
      {
        return symbol_type (token::CASE);
      }
#else
      static
      symbol_type
      make_CASE ()
      {
        return symbol_type (token::CASE);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ESAC ()
      {
        return symbol_type (token::ESAC);
      }
#else
      static
      symbol_type
      make_ESAC ()
      {
        return symbol_type (token::ESAC);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FOR ()
      {
        return symbol_type (token::FOR);
      }
#else
      static
      symbol_type
      make_FOR ()
      {
        return symbol_type (token::FOR);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SELECT ()
      {
        return symbol_type (token::SELECT);
      }
#else
      static
      symbol_type
      make_SELECT ()
      {
        return symbol_type (token::SELECT);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WHILE ()
      {
        return symbol_type (token::WHILE);
      }
#else
      static
      symbol_type
      make_WHILE ()
      {
        return symbol_type (token::WHILE);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_UNTIL ()
      {
        return symbol_type (token::UNTIL);
      }
#else
      static
      symbol_type
      make_UNTIL ()
      {
        return symbol_type (token::UNTIL);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DO ()
      {
        return symbol_type (token::DO);
      }
#else
      static
      symbol_type
      make_DO ()
      {
        return symbol_type (token::DO);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_DONE ()
      {
        return symbol_type (token::DONE);
      }
#else
      static
      symbol_type
      make_DONE ()
      {
        return symbol_type (token::DONE);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_FUNCTION ()
      {
        return symbol_type (token::FUNCTION);
      }
#else
      static
      symbol_type
      make_FUNCTION ()
      {
        return symbol_type (token::FUNCTION);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COPROC ()
      {
        return symbol_type (token::COPROC);
      }
#else
      static
      symbol_type
      make_COPROC ()
      {
        return symbol_type (token::COPROC);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COND_START ()
      {
        return symbol_type (token::COND_START);
      }
#else
      static
      symbol_type
      make_COND_START ()
      {
        return symbol_type (token::COND_START);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COND_END ()
      {
        return symbol_type (token::COND_END);
      }
#else
      static
      symbol_type
      make_COND_END ()
      {
        return symbol_type (token::COND_END);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COND_ERROR ()
      {
        return symbol_type (token::COND_ERROR);
      }
#else
      static
      symbol_type
      make_COND_ERROR ()
      {
        return symbol_type (token::COND_ERROR);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_IN ()
      {
        return symbol_type (token::IN);
      }
#else
      static
      symbol_type
      make_IN ()
      {
        return symbol_type (token::IN);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BANG ()
      {
        return symbol_type (token::BANG);
      }
#else
      static
      symbol_type
      make_BANG ()
      {
        return symbol_type (token::BANG);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TIME ()
      {
        return symbol_type (token::TIME);
      }
#else
      static
      symbol_type
      make_TIME ()
      {
        return symbol_type (token::TIME);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TIMEOPT ()
      {
        return symbol_type (token::TIMEOPT);
      }
#else
      static
      symbol_type
      make_TIMEOPT ()
      {
        return symbol_type (token::TIMEOPT);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_TIMEIGN ()
      {
        return symbol_type (token::TIMEIGN);
      }
#else
      static
      symbol_type
      make_TIMEIGN ()
      {
        return symbol_type (token::TIMEIGN);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_WORD (WORD_DESC* v)
      {
        return symbol_type (token::WORD, std::move (v));
      }
#else
      static
      symbol_type
      make_WORD (const WORD_DESC*& v)
      {
        return symbol_type (token::WORD, v);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ASSIGNMENT_WORD (WORD_DESC* v)
      {
        return symbol_type (token::ASSIGNMENT_WORD, std::move (v));
      }
#else
      static
      symbol_type
      make_ASSIGNMENT_WORD (const WORD_DESC*& v)
      {
        return symbol_type (token::ASSIGNMENT_WORD, v);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_REDIR_WORD (WORD_DESC* v)
      {
        return symbol_type (token::REDIR_WORD, std::move (v));
      }
#else
      static
      symbol_type
      make_REDIR_WORD (const WORD_DESC*& v)
      {
        return symbol_type (token::REDIR_WORD, v);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_NUMBER (int64_t v)
      {
        return symbol_type (token::NUMBER, std::move (v));
      }
#else
      static
      symbol_type
      make_NUMBER (const int64_t& v)
      {
        return symbol_type (token::NUMBER, v);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ARITH_CMD (WORD_LIST* v)
      {
        return symbol_type (token::ARITH_CMD, std::move (v));
      }
#else
      static
      symbol_type
      make_ARITH_CMD (const WORD_LIST*& v)
      {
        return symbol_type (token::ARITH_CMD, v);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_ARITH_FOR_EXPRS (WORD_LIST* v)
      {
        return symbol_type (token::ARITH_FOR_EXPRS, std::move (v));
      }
#else
      static
      symbol_type
      make_ARITH_FOR_EXPRS (const WORD_LIST*& v)
      {
        return symbol_type (token::ARITH_FOR_EXPRS, v);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_COND_CMD (COMMAND* v)
      {
        return symbol_type (token::COND_CMD, std::move (v));
      }
#else
      static
      symbol_type
      make_COND_CMD (const COMMAND*& v)
      {
        return symbol_type (token::COND_CMD, v);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND_AND ()
      {
        return symbol_type (token::AND_AND);
      }
#else
      static
      symbol_type
      make_AND_AND ()
      {
        return symbol_type (token::AND_AND);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_OR_OR ()
      {
        return symbol_type (token::OR_OR);
      }
#else
      static
      symbol_type
      make_OR_OR ()
      {
        return symbol_type (token::OR_OR);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GREATER_GREATER ()
      {
        return symbol_type (token::GREATER_GREATER);
      }
#else
      static
      symbol_type
      make_GREATER_GREATER ()
      {
        return symbol_type (token::GREATER_GREATER);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LESS_LESS ()
      {
        return symbol_type (token::LESS_LESS);
      }
#else
      static
      symbol_type
      make_LESS_LESS ()
      {
        return symbol_type (token::LESS_LESS);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LESS_AND ()
      {
        return symbol_type (token::LESS_AND);
      }
#else
      static
      symbol_type
      make_LESS_AND ()
      {
        return symbol_type (token::LESS_AND);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LESS_LESS_LESS ()
      {
        return symbol_type (token::LESS_LESS_LESS);
      }
#else
      static
      symbol_type
      make_LESS_LESS_LESS ()
      {
        return symbol_type (token::LESS_LESS_LESS);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GREATER_AND ()
      {
        return symbol_type (token::GREATER_AND);
      }
#else
      static
      symbol_type
      make_GREATER_AND ()
      {
        return symbol_type (token::GREATER_AND);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMI_SEMI ()
      {
        return symbol_type (token::SEMI_SEMI);
      }
#else
      static
      symbol_type
      make_SEMI_SEMI ()
      {
        return symbol_type (token::SEMI_SEMI);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMI_AND ()
      {
        return symbol_type (token::SEMI_AND);
      }
#else
      static
      symbol_type
      make_SEMI_AND ()
      {
        return symbol_type (token::SEMI_AND);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_SEMI_SEMI_AND ()
      {
        return symbol_type (token::SEMI_SEMI_AND);
      }
#else
      static
      symbol_type
      make_SEMI_SEMI_AND ()
      {
        return symbol_type (token::SEMI_SEMI_AND);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LESS_LESS_MINUS ()
      {
        return symbol_type (token::LESS_LESS_MINUS);
      }
#else
      static
      symbol_type
      make_LESS_LESS_MINUS ()
      {
        return symbol_type (token::LESS_LESS_MINUS);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND_GREATER ()
      {
        return symbol_type (token::AND_GREATER);
      }
#else
      static
      symbol_type
      make_AND_GREATER ()
      {
        return symbol_type (token::AND_GREATER);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_AND_GREATER_GREATER ()
      {
        return symbol_type (token::AND_GREATER_GREATER);
      }
#else
      static
      symbol_type
      make_AND_GREATER_GREATER ()
      {
        return symbol_type (token::AND_GREATER_GREATER);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_LESS_GREATER ()
      {
        return symbol_type (token::LESS_GREATER);
      }
#else
      static
      symbol_type
      make_LESS_GREATER ()
      {
        return symbol_type (token::LESS_GREATER);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_GREATER_BAR ()
      {
        return symbol_type (token::GREATER_BAR);
      }
#else
      static
      symbol_type
      make_GREATER_BAR ()
      {
        return symbol_type (token::GREATER_BAR);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_BAR_AND ()
      {
        return symbol_type (token::BAR_AND);
      }
#else
      static
      symbol_type
      make_BAR_AND ()
      {
        return symbol_type (token::BAR_AND);
      }
#endif
#if 201103L <= YY_CPLUSPLUS
      static
      symbol_type
      make_yacc_EOF ()
      {
        return symbol_type (token::yacc_EOF);
      }
#else
      static
      symbol_type
      make_yacc_EOF ()
      {
        return symbol_type (token::yacc_EOF);
      }
#endif


  private:
#if YY_CPLUSPLUS < 201103L
    /// Non copyable.
    parser (const parser&);
    /// Non copyable.
    parser& operator= (const parser&);
#endif


    /// Stored state numbers (used for stacks).
    typedef short state_type;

    /// Compute post-reduction state.
    /// \param yystate   the current state
    /// \param yysym     the nonterminal to push on the stack
    static state_type yy_lr_goto_state_ (state_type yystate, int yysym);

    /// Whether the given \c yypact_ value indicates a defaulted state.
    /// \param yyvalue   the value to check
    static bool yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT;

    /// Whether the given \c yytable_ value indicates a syntax error.
    /// \param yyvalue   the value to check
    static bool yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT;

    static const short yypact_ninf_;
    static const signed char yytable_ninf_;

    /// Convert a scanner token kind \a t to a symbol kind.
    /// In theory \a t should be a token_kind_type, but character literals
    /// are valid, yet not members of the token_kind_type enum.
    static symbol_kind_type yytranslate_ (int t) YY_NOEXCEPT;

#if YYDEBUG || 0
    /// For a symbol, its name in clear.
    static const char* const yytname_[];
#endif // #if YYDEBUG || 0


    // Tables.
    // YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
    // STATE-NUM.
    static const short yypact_[];

    // YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
    // Performed when YYTABLE does not specify something else to do.  Zero
    // means the default is an error.
    static const unsigned char yydefact_[];

    // YYPGOTO[NTERM-NUM].
    static const short yypgoto_[];

    // YYDEFGOTO[NTERM-NUM].
    static const short yydefgoto_[];

    // YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
    // positive, shift that token.  If negative, reduce the rule whose
    // number is the opposite.  If YYTABLE_NINF, syntax error.
    static const short yytable_[];

    static const short yycheck_[];

    // YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
    // state STATE-NUM.
    static const signed char yystos_[];

    // YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.
    static const signed char yyr1_[];

    // YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.
    static const signed char yyr2_[];


#if YYDEBUG
    // YYRLINE[YYN] -- Source line where rule number YYN was defined.
    static const short yyrline_[];
    /// Report on the debug stream that the rule \a r is going to be reduced.
    virtual void yy_reduce_print_ (int r) const;
    /// Print the state stack on the debug stream.
    virtual void yy_stack_print_ () const;

    /// Debugging level.
    int yydebug_;
    /// Debug stream.
    std::ostream* yycdebug_;

    /// \brief Display a symbol kind, value and location.
    /// \param yyo    The output stream.
    /// \param yysym  The symbol.
    template <typename Base>
    void yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const;
#endif

    /// \brief Reclaim the memory associated to a symbol.
    /// \param yymsg     Why this token is reclaimed.
    ///                  If null, print nothing.
    /// \param yysym     The symbol.
    template <typename Base>
    void yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const;

  private:
    /// Type access provider for state based symbols.
    struct by_state
    {
      /// Default constructor.
      by_state () YY_NOEXCEPT;

      /// The symbol kind as needed by the constructor.
      typedef state_type kind_type;

      /// Constructor.
      by_state (kind_type s) YY_NOEXCEPT;

      /// Copy constructor.
      by_state (const by_state& that) YY_NOEXCEPT;

      /// Record that this symbol is empty.
      void clear () YY_NOEXCEPT;

      /// Steal the symbol kind from \a that.
      void move (by_state& that);

      /// The symbol kind (corresponding to \a state).
      /// \a symbol_kind::S_YYEMPTY when empty.
      symbol_kind_type kind () const YY_NOEXCEPT;

      /// The state number used to denote an empty symbol.
      /// We use the initial state, as it does not have a value.
      enum { empty_state = 0 };

      /// The state.
      /// \a empty when empty.
      state_type state;
    };

    /// "Internal" symbol: element of the stack.
    struct stack_symbol_type : basic_symbol<by_state>
    {
      /// Superclass.
      typedef basic_symbol<by_state> super_type;
      /// Construct an empty symbol.
      stack_symbol_type ();
      /// Move or copy construction.
      stack_symbol_type (YY_RVREF (stack_symbol_type) that);
      /// Steal the contents from \a sym to build this.
      stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) sym);
#if YY_CPLUSPLUS < 201103L
      /// Assignment, needed by push_back by some old implementations.
      /// Moves the contents of that.
      stack_symbol_type& operator= (stack_symbol_type& that);

      /// Assignment, needed by push_back by other implementations.
      /// Needed by some other old implementations.
      stack_symbol_type& operator= (const stack_symbol_type& that);
#endif
    };

    /// A stack with random access from its top.
    template <typename T, typename S = std::vector<T> >
    class stack
    {
    public:
      // Hide our reversed order.
      typedef typename S::iterator iterator;
      typedef typename S::const_iterator const_iterator;
      typedef typename S::size_type size_type;
      typedef typename std::ptrdiff_t index_type;

      stack (size_type n = 200) YY_NOEXCEPT
        : seq_ (n)
      {}

#if 201103L <= YY_CPLUSPLUS
      /// Non copyable.
      stack (const stack&) = delete;
      /// Non copyable.
      stack& operator= (const stack&) = delete;
#endif

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      const T&
      operator[] (index_type i) const
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Random access.
      ///
      /// Index 0 returns the topmost element.
      T&
      operator[] (index_type i)
      {
        return seq_[size_type (size () - 1 - i)];
      }

      /// Steal the contents of \a t.
      ///
      /// Close to move-semantics.
      void
      push (YY_MOVE_REF (T) t)
      {
        seq_.push_back (T ());
        operator[] (0).move (t);
      }

      /// Pop elements from the stack.
      void
      pop (std::ptrdiff_t n = 1) YY_NOEXCEPT
      {
        for (; 0 < n; --n)
          seq_.pop_back ();
      }

      /// Pop all elements from the stack.
      void
      clear () YY_NOEXCEPT
      {
        seq_.clear ();
      }

      /// Number of elements on the stack.
      index_type
      size () const YY_NOEXCEPT
      {
        return index_type (seq_.size ());
      }

      /// Iterator on top of the stack (going downwards).
      const_iterator
      begin () const YY_NOEXCEPT
      {
        return seq_.begin ();
      }

      /// Bottom of the stack.
      const_iterator
      end () const YY_NOEXCEPT
      {
        return seq_.end ();
      }

      /// Present a slice of the top of a stack.
      class slice
      {
      public:
        slice (const stack& stack, index_type range) YY_NOEXCEPT
          : stack_ (stack)
          , range_ (range)
        {}

        const T&
        operator[] (index_type i) const
        {
          return stack_[range_ - i];
        }

      private:
        const stack& stack_;
        index_type range_;
      };

    private:
#if YY_CPLUSPLUS < 201103L
      /// Non copyable.
      stack (const stack&);
      /// Non copyable.
      stack& operator= (const stack&);
#endif
      /// The wrapped container.
      S seq_;
    };


    /// Stack type.
    typedef stack<stack_symbol_type> stack_type;

    /// The stack.
    stack_type yystack_;

    /// Push a new state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param sym  the symbol
    /// \warning the contents of \a s.value is stolen.
    void yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym);

    /// Push a new look ahead token on the state on the stack.
    /// \param m    a debug message to display
    ///             if null, no trace is output.
    /// \param s    the state
    /// \param sym  the symbol (for its value and location).
    /// \warning the contents of \a sym.value is stolen.
    void yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym);

    /// Pop \a n symbols from the stack.
    void yypop_ (int n = 1) YY_NOEXCEPT;

    /// Constants.
    enum
    {
      yylast_ = 661,     ///< Last index in yytable_.
      yynnts_ = 38,  ///< Number of nonterminal symbols.
      yyfinal_ = 118 ///< Termination state number.
    };



  };

  inline
  parser::symbol_kind_type
  parser::yytranslate_ (int t) YY_NOEXCEPT
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const signed char
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      51,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    49,     2,
      59,    60,     2,     2,     2,    56,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    50,
      55,     2,    54,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    57,    53,    58,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    52
    };
    // Last valid token kind.
    const int code_max = 304;

    if (t <= 0)
      return symbol_kind::S_YYEOF;
    else if (t <= code_max)
      return static_cast <symbol_kind_type> (translate_table[t]);
    else
      return symbol_kind::S_YYUNDEF;
  }

  // basic_symbol.
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value ()
  {
    switch (this->kind ())
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
        value.copy< COMMAND* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.copy< ELEMENT > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.copy< PATTERN_LIST* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.copy< REDIRECT* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.copy< WORD_DESC* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.copy< WORD_LIST* > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.copy< int64_t > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

  }




  template <typename Base>
  parser::symbol_kind_type
  parser::basic_symbol<Base>::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


  template <typename Base>
  bool
  parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return this->kind () == symbol_kind::S_YYEMPTY;
  }

  template <typename Base>
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    switch (this->kind ())
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
        value.move< COMMAND* > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_simple_command_element: // simple_command_element
        value.move< ELEMENT > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_case_clause: // case_clause
      case symbol_kind::S_pattern_list: // pattern_list
      case symbol_kind::S_case_clause_sequence: // case_clause_sequence
        value.move< PATTERN_LIST* > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_redirection: // redirection
      case symbol_kind::S_redirection_list: // redirection_list
        value.move< REDIRECT* > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_WORD: // WORD
      case symbol_kind::S_ASSIGNMENT_WORD: // ASSIGNMENT_WORD
      case symbol_kind::S_REDIR_WORD: // REDIR_WORD
        value.move< WORD_DESC* > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_ARITH_CMD: // ARITH_CMD
      case symbol_kind::S_ARITH_FOR_EXPRS: // ARITH_FOR_EXPRS
      case symbol_kind::S_word_list: // word_list
      case symbol_kind::S_pattern: // pattern
        value.move< WORD_LIST* > (YY_MOVE (s.value));
        break;

      case symbol_kind::S_NUMBER: // NUMBER
      case symbol_kind::S_list_terminator: // list_terminator
      case symbol_kind::S_timespec: // timespec
        value.move< int64_t > (YY_MOVE (s.value));
        break;

      default:
        break;
    }

  }

  // by_kind.
  inline
  parser::by_kind::by_kind () YY_NOEXCEPT
    : kind_ (symbol_kind::S_YYEMPTY)
  {}

#if 201103L <= YY_CPLUSPLUS
  inline
  parser::by_kind::by_kind (by_kind&& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {
    that.clear ();
  }
#endif

  inline
  parser::by_kind::by_kind (const by_kind& that) YY_NOEXCEPT
    : kind_ (that.kind_)
  {}

  inline
  parser::by_kind::by_kind (token_kind_type t) YY_NOEXCEPT
    : kind_ (yytranslate_ (t))
  {}



  inline
  void
  parser::by_kind::clear () YY_NOEXCEPT
  {
    kind_ = symbol_kind::S_YYEMPTY;
  }

  inline
  void
  parser::by_kind::move (by_kind& that)
  {
    kind_ = that.kind_;
    that.clear ();
  }

  inline
  parser::symbol_kind_type
  parser::by_kind::kind () const YY_NOEXCEPT
  {
    return kind_;
  }


  inline
  parser::symbol_kind_type
  parser::by_kind::type_get () const YY_NOEXCEPT
  {
    return this->kind ();
  }


#line 26 "../bashcpp/parse.yy"
} // bash
#line 2455 "../bashcpp/parse.hh"




#endif // !YY_YY__BASHCPP_PARSE_HH_INCLUDED
