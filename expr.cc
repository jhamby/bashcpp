/* expr.cc -- arithmetic expression evaluation. */

/* Copyright (C) 1990-2021 Free Software Foundation, Inc.

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

/*
 All arithmetic is done as int64_t integers with no checking for overflow
 (though division by 0 is caught and flagged as an error).

 The following operators are handled, grouped into a set of levels in
 order of decreasing precedence.

        "id++", "id--"		[post-increment and post-decrement]
        "-", "+"		[(unary operators)]
        "++id", "--id"		[pre-increment and pre-decrement]
        "!", "~"
        "**"			[(exponentiation)]
        "*", "/", "%"
        "+", "-"
        "<<", ">>"
        "<=", ">=", "<", ">"
        "==", "!="
        "&"
        "^"
        "|"
        "&&"
        "||"
        "expr ? expr : expr"
        "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|="
        ,			[comma]

 (Note that most of these operators have special meaning to bash, and an
 entire expression should be quoted, e.g. "a=$a+1" or "a=a+1" to ensure
 that it is passed intact to the evaluator when using `let'.  When using
 the $[] or $(( )) forms, the text between the `[' and `]' or `((' and `))'
 is treated as if in double quotes.)

 Sub-expressions within parentheses have a precedence level greater than
 all of the above levels and are evaluated first.  Within a single prece-
 dence group, evaluation is left-to-right, except for the arithmetic
 assignment operator (`='), which is evaluated right-to-left (as in C).

 The expression evaluator returns the value of the expression (assignment
 statements have as a value what is returned by the RHS).  The `let'
 builtin, on the other hand, returns 0 if the last expression evaluates to
 a non-zero, and 1 otherwise.

 Implementation is a recursive-descent parser.

 Chet Ramey
 chet@po.cwru.edu
*/

#include "config.h"

#include "shell.hh"

namespace bash
{

/* Because of the $((...)) construct, expressions may include newlines.
   Here is a macro which accepts newlines, tabs and spaces as whitespace. */
#define cr_whitespace(c) (whitespace (c) || ((c) == '\n'))

/* Maximum amount of recursion allowed.  This prevents a non-integer
   variable such as "num=num+2" from infinitely adding to itself when
   "let num=num+2" is given. */
#define MAX_EXPR_RECURSION_LEVEL 1024

/* This should be the function corresponding to the operator with the
   lowest precedence. */
#define EXP_LOWEST expcomma

// Standard error message to use when encountering an invalid array subscript
extern const char *bash_badsub_errmsg;

static int64_t ipow (int64_t base, int64_t exp);

/* Push and save away the contents of the globals describing the
   current expression context. */
void
Shell::pushexp ()
{
  if (expr_stack.size () >= MAX_EXPR_RECURSION_LEVEL)
    evalerror (_ ("expression recursion level exceeded"));

  EXPR_CONTEXT *context = new EXPR_CONTEXT (this_expr);
  expr_stack.push_back (context);
}

/* Pop the the contents of the expression context stack into the
   globals describing the current expression context. */
void
Shell::popexp ()
{
  if (expr_stack.empty ())
    {
      /* See the comment at the top of evalexp() for an explanation of why
         this is done. */
      this_expr.expression = nullptr;
      this_expr.lasttp = nullptr;
      evalerror (_ ("recursion stack underflow"));
    }

  this_expr = *(expr_stack.back ());
  delete expr_stack.back ();
  expr_stack.pop_back ();
}

void
Shell::expr_unwind ()
{
  while (!expr_stack.empty ())
    {
      EXPR_CONTEXT *context = expr_stack.back ();
      delete context;
    }

  this_expr.noeval = false; /* XXX */
}

void
Shell::expr_bind_variable (const string &lhs, const string &rhs)
{
  SHELL_VAR *v;
  assign_flags aflags;

  if (lhs.empty ())
    return; /* XXX */

#if defined(ARRAY_VARS)
  aflags
      = (assoc_expand_once && already_expanded) ? ASS_NOEXPAND : ASS_NOFLAGS;
  aflags |= ASS_ALLOWALLSUB; // allow assoc[@]=value
#else
  aflags = ASS_NOFLAGS;
#endif
  v = bind_int_variable (lhs, rhs, aflags);
  if (v && (v->readonly () || v->noassign ()))
    throw bash_exception (FORCE_EOF); /* variable assignment error */

  stupidly_hack_special_variables (lhs);
}

#if defined(ARRAY_VARS)
/* This is similar to the logic in arrayfunc.cc:valid_array_reference when
   you pass VA_NOEXPAND. */
size_t
Shell::expr_skipsubscript (const char *vp, char *cp)
{
  bool isassoc = false;
  SHELL_VAR *entry = nullptr;

  if (assoc_expand_once & already_expanded)
    {
      *cp = '\0';
      isassoc = legal_identifier (vp) && (entry = find_variable (vp))
                && entry->assoc ();
      *cp = '[';
    }

  int flags
      = (isassoc && assoc_expand_once && already_expanded) ? VA_NOEXPAND : 0;

  return skipsubscript (cp, 0, flags);
}

/* Rewrite tok, which is of the form vname[expression], to vname[ind], where
   IND is the already-calculated value of expression. */
void
Shell::expr_bind_array_element (const string &tok, arrayind_t ind,
                                const string &rhs)
{
  string istr = fmtumax (static_cast<uint64_t> (ind), 10);
  string *vname = array_variable_name (tok, 0, nullptr);
  if (vname == nullptr)
    return;

  string lhs (*vname);
  delete vname;
  lhs.push_back ('[');
  lhs += istr;
  lhs.push_back (']');

  /*itrace("expr_bind_array_element: %s=%s", lhs, rhs);*/
  expr_bind_variable (lhs, rhs);
}

#endif /* ARRAY_VARS */

/* Evaluate EXPR, and return the arithmetic result.  If VALIDP is
   non-null, a zero is stored into the location to which it points
   if the expression is invalid, non-zero otherwise.  If a non-zero
   value is returned in *VALIDP, the return value of evalexp() may
   be used.

   The `while' loop after the exception is caught relies on the above
   implementation of pushexp and popexp leaving in expr_stack[0] the
   values that the variables had when the program started.  That is,
   the first things saved are the initial values of the variables that
   were assigned at program startup or by the compiler.  Therefore, it is
   safe to let the loop terminate when expr_depth == 0, without freeing up
   any of the expr_depth[0] stuff. */
int64_t
Shell::evalexp (const string &expr, eval_flags flags, bool *validp)
{
  try
    {
      this_expr.noeval = 0;
      already_expanded = (flags & EXP_EXPANDED);

      int64_t val = subexpr (expr);

      if (validp)
        *validp = true;

      return val;
    }
  catch (const eval_exception &)
    {
      delete[] this_expr.tokstr;
      delete[] this_expr.expression;
      this_expr.tokstr = nullptr;
      this_expr.expression = nullptr;

      expr_unwind ();

      if (validp)
        *validp = false;

      return 0;
    }
}

int64_t
Shell::subexpr (const string &expr)
{
  string::const_iterator it;
  for (it = expr.begin (); it != expr.end () && cr_whitespace (*it); ++it)
    ;

  if (it == expr.end ())
    return 0;

  pushexp ();
  this_expr.expression = savestring (expr);
  this_expr.tp = this_expr.expression;

  this_expr.curtok = this_expr.lasttok = NONE;
  this_expr.tokstr = nullptr;
  this_expr.tokval = 0;
  this_expr.lval.init ();
  lastlval = this_expr.lval;

  readtok ();

  int64_t val = EXP_LOWEST ();

  /*TAG:bash-5.3 make it clear that these are arithmetic syntax errors */
  if (this_expr.curtok != NONE)
    evalerror (_ ("syntax error in expression"));

  delete[] this_expr.tokstr;
  delete[] this_expr.expression;

  popexp ();

  return val;
}

int64_t
Shell::expcomma ()
{
  int64_t value;

  value = expassign ();
  while (this_expr.curtok == COMMA)
    {
      readtok ();
      value = expassign ();
    }

  return value;
}

int64_t
Shell::expassign ()
{
  int64_t value;
  arrayind_t lind;

  value = expcond ();
  if (this_expr.curtok == EQ || this_expr.curtok == OP_ASSIGN)
    {
      token_t op = NONE;
      int64_t lvalue = 0;

      bool special = (this_expr.curtok == OP_ASSIGN);

      if (this_expr.lasttok != STR)
        evalerror (_ ("attempted assignment to non-variable"));

      if (special)
        {
          op = assigntok; /* a OP= b */
          lvalue = value;
        }

      if (this_expr.tokstr == nullptr)
        evalerror (_ ("syntax error in variable assignment"));

      string lhs = this_expr.tokstr;
      /* save ind in case rhs is string var and evaluation overwrites it */
      lind = this_expr.lval.ind;
      readtok ();
      value = expassign ();

      if (special)
        {
          if ((op == DIV || op == MOD) && value == 0)
            {
              if (this_expr.noeval == 0)
                evalerror (_ ("division by 0"));
              else
                value = 1;
            }

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
#endif

          switch (op)
            {
            case MUL:
              /* Handle INTMAX_MIN and INTMAX_MAX * -1 specially here? */
              lvalue *= value;
              break;
            case DIV:
            case MOD:
              if (lvalue == INTMAX_MIN && value == -1)
                lvalue = (op == DIV) ? INTMAX_MIN : 0;
              else
                lvalue = (op == DIV) ? lvalue / value : lvalue % value;
              break;
            case PLUS:
              lvalue += value;
              break;
            case MINUS:
              lvalue -= value;
              break;
            case LSH:
              lvalue <<= value;
              break;
            case RSH:
              lvalue >>= value;
              break;
            case BAND:
              lvalue &= value;
              break;
            case BOR:
              lvalue |= value;
              break;
            case BXOR:
              lvalue ^= value;
              break;
            default:
              evalerror (_ ("bug: bad expassign token"));
              /* NOTREACHED */
            }
          value = lvalue;
        }

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

      string rhs = itos (value);
      if (this_expr.noeval == 0)
        {
#if defined(ARRAY_VARS)
          if (lind != -1)
            expr_bind_array_element (lhs, lind, rhs);
          else
#endif
            expr_bind_variable (lhs, rhs);
        }
      if (this_expr.lval.tokstr == this_expr.tokstr)
        this_expr.lval.init ();

      delete[] this_expr.tokstr;
      this_expr.tokstr = nullptr; /* For freeing on errors. */
    }

  return value;
}

/* Conditional expression (expr?expr:expr) */
int64_t
Shell::expcond ()
{
  int64_t cval, val1, val2, rval;
  bool set_noeval = false;

  rval = cval = explor ();
  if (this_expr.curtok == QUES) /* found conditional expr */
    {
      if (cval == 0)
        {
          set_noeval = true;
          this_expr.noeval++;
        }

      readtok ();
      if (this_expr.curtok == NONE || this_expr.curtok == COL)
        evalerror (_ ("expression expected"));

      val1 = EXP_LOWEST ();

      if (set_noeval)
        this_expr.noeval--;
      if (this_expr.curtok != COL)
        evalerror (_ ("`:' expected for conditional expression"));

      set_noeval = false;
      if (cval)
        {
          set_noeval = true;
          this_expr.noeval++;
        }

      readtok ();
      if (this_expr.curtok == 0)
        evalerror (_ ("expression expected"));
      val2 = expcond ();

      if (set_noeval)
        this_expr.noeval--;
      rval = cval ? val1 : val2;
      this_expr.lasttok = COND;
    }
  return rval;
}

/* Logical OR. */
int64_t
Shell::explor ()
{
  int64_t val1 = expland ();

  while (this_expr.curtok == LOR)
    {
      bool set_noeval = false;
      if (val1 != 0)
        {
          this_expr.noeval++;
          set_noeval = true;
        }
      readtok ();
      int64_t val2 = expland ();
      if (set_noeval)
        this_expr.noeval--;
      val1 = val1 || val2;
      this_expr.lasttok = LOR;
    }

  return val1;
}

/* Logical AND. */
int64_t
Shell::expland ()
{
  int64_t val1 = expbor ();

  while (this_expr.curtok == LAND)
    {
      bool set_noeval = false;
      if (val1 == 0)
        {
          set_noeval = true;
          this_expr.noeval++;
        }
      readtok ();
      int64_t val2 = expbor ();
      if (set_noeval)
        this_expr.noeval--;
      val1 = val1 && val2;
      this_expr.lasttok = LAND;
    }

  return val1;
}

/* Bitwise OR. */
int64_t
Shell::expbor ()
{
  int64_t val1 = expbxor ();

  while (this_expr.curtok == BOR)
    {
      readtok ();
      int64_t val2 = expbxor ();
      val1 = val1 | val2;
      this_expr.lasttok = NUM;
    }

  return val1;
}

/* Bitwise XOR. */
int64_t
Shell::expbxor ()
{
  int64_t val1 = expband ();

  while (this_expr.curtok == BXOR)
    {
      readtok ();
      int64_t val2 = expband ();
      val1 = val1 ^ val2;
      this_expr.lasttok = NUM;
    }

  return val1;
}

/* Bitwise AND. */
int64_t
Shell::expband ()
{
  int64_t val1 = exp5 ();

  while (this_expr.curtok == BAND)
    {
      readtok ();
      int64_t val2 = exp5 ();
      val1 = val1 & val2;
      this_expr.lasttok = NUM;
    }

  return val1;
}

int64_t
Shell::exp5 ()
{
  int64_t val1 = exp4 ();

  while ((this_expr.curtok == EQEQ) || (this_expr.curtok == NEQ))
    {
      token_t op = this_expr.curtok;

      readtok ();
      int64_t val2 = exp4 ();
      if (op == EQEQ)
        val1 = (val1 == val2);
      else if (op == NEQ)
        val1 = (val1 != val2);
      this_expr.lasttok = NUM;
    }
  return val1;
}

int64_t
Shell::exp4 ()
{
  int64_t val1 = expshift ();
  while ((this_expr.curtok == LEQ) || (this_expr.curtok == GEQ)
         || (this_expr.curtok == LT) || (this_expr.curtok == GT))
    {
      token_t op = this_expr.curtok;

      readtok ();
      int64_t val2 = expshift ();

      if (op == LEQ)
        val1 = val1 <= val2;
      else if (op == GEQ)
        val1 = val1 >= val2;
      else if (op == LT)
        val1 = val1 < val2;
      else /* (op == GT) */
        val1 = val1 > val2;
      this_expr.lasttok = NUM;
    }
  return val1;
}

/* Left and right shifts. */
int64_t
Shell::expshift ()
{
  int64_t val1 = exp3 ();

  while ((this_expr.curtok == LSH) || (this_expr.curtok == RSH))
    {
      token_t op = this_expr.curtok;

      readtok ();
      int64_t val2 = exp3 ();

      if (op == LSH)
        val1 = val1 << val2;
      else
        val1 = val1 >> val2;
      this_expr.lasttok = NUM;
    }

  return val1;
}

int64_t
Shell::exp3 ()
{
  int64_t val1 = expmuldiv ();

  while ((this_expr.curtok == PLUS) || (this_expr.curtok == MINUS))
    {
      token_t op = this_expr.curtok;

      readtok ();
      int64_t val2 = expmuldiv ();

      if (op == PLUS)
        val1 += val2;
      else if (op == MINUS)
        val1 -= val2;
      this_expr.lasttok = NUM;
    }
  return val1;
}

int64_t
Shell::expmuldiv ()
{
  int64_t val1 = exppower ();

  while ((this_expr.curtok == MUL) || (this_expr.curtok == DIV)
         || (this_expr.curtok == MOD))
    {
      token_t op = this_expr.curtok;

      char *stp = this_expr.tp;
      readtok ();

      int64_t val2 = exppower ();

      /* Handle division by 0 and twos-complement arithmetic overflow */
      if (((op == DIV) || (op == MOD)) && (val2 == 0))
        {
          if (this_expr.noeval == 0)
            {
              this_expr.lasttp = stp;
              if (stp)
                while (*this_expr.lasttp && whitespace (*this_expr.lasttp))
                  this_expr.lasttp++;

              evalerror (_ ("division by 0"));
            }
          else
            val2 = 1;
        }
      else if (op == MOD && val1 == INTMAX_MIN && val2 == -1)
        {
          val1 = 0;
          continue;
        }
      else if (op == DIV && val1 == INTMAX_MIN && val2 == -1)
        val2 = 1;

      if (op == MUL)
        val1 *= val2;
      else if (op == DIV || op == MOD)
        val1 = (op == DIV) ? val1 / val2 : val1 % val2;

      this_expr.lasttok = NUM;
    }
  return val1;
}

static inline int64_t
ipow (int64_t base, int64_t exp)
{
  int64_t result = 1;
  while (exp)
    {
      if (exp & 1)
        result *= base;
      exp >>= 1;
      base *= base;
    }
  return result;
}

int64_t
Shell::exppower ()
{
  int64_t val1 = exp1 ();
  while (this_expr.curtok == POWER)
    {
      readtok ();
      int64_t val2 = exppower (); /* exponentiation is right-associative */
      this_expr.lasttok = NUM;
      if (val2 == 0)
        return 1;
      if (val2 < 0)
        evalerror (_ ("exponent less than 0"));
      val1 = ipow (val1, val2);
    }
  return val1;
}

int64_t
Shell::exp1 ()
{
  int64_t val;

  if (this_expr.curtok == NOT)
    {
      readtok ();
      val = !exp1 ();
      this_expr.lasttok = NUM;
    }
  else if (this_expr.curtok == BNOT)
    {
      readtok ();
      val = ~exp1 ();
      this_expr.lasttok = NUM;
    }
  else if (this_expr.curtok == MINUS)
    {
      readtok ();
      val = -exp1 ();
      this_expr.lasttok = NUM;
    }
  else if (this_expr.curtok == PLUS)
    {
      readtok ();
      val = exp1 ();
      this_expr.lasttok = NUM;
    }
  else
    val = exp0 ();

  return val;
}

int64_t
Shell::exp0 ()
{
  int64_t val = 0, v2;
  EXPR_CONTEXT ec;

  /* XXX - might need additional logic here to decide whether or not
           pre-increment or pre-decrement is legal at this point. */
  if (this_expr.curtok == PREINC || this_expr.curtok == PREDEC)
    {
      token_t stok = this_expr.lasttok = this_expr.curtok;
      readtok ();
      if (this_expr.curtok != STR)
        /* readtok() catches this */
        evalerror (
            _ ("identifier expected after pre-increment or pre-decrement"));

      v2 = this_expr.tokval + ((stok == PREINC) ? 1 : -1);
      string vincdec = itos (v2);
      if (this_expr.noeval == 0)
        {
#if defined(ARRAY_VARS)
          if (this_expr.lval.ind != -1)
            expr_bind_array_element (this_expr.lval.tokstr, this_expr.lval.ind,
                                     vincdec);
          else
#endif
              if (this_expr.tokstr)
            expr_bind_variable (this_expr.tokstr, vincdec);
        }
      val = v2;

      this_expr.curtok = NUM; /* make sure --x=7 is flagged as an error */
      readtok ();
    }
  else if (this_expr.curtok == LPAR)
    {
      /* XXX - save curlval here?  Or entire expression context? */
      readtok ();
      val = EXP_LOWEST ();

      if (this_expr.curtok != RPAR)
        evalerror (_ ("missing `)'"));

      /* Skip over closing paren. */
      readtok ();
    }
  else if ((this_expr.curtok == NUM) || (this_expr.curtok == STR))
    {
      val = this_expr.tokval;
      if (this_expr.curtok == STR)
        {
          ec = this_expr;
          this_expr.tokstr = nullptr; /* keep it from being freed */
          this_expr.noeval = 1;
          readtok ();
          token_t stok = this_expr.curtok;

          /* post-increment or post-decrement */
          if (stok == POSTINC || stok == POSTDEC)
            {
              /* restore certain portions of EC */
              this_expr.tokstr = ec.tokstr;
              this_expr.noeval = ec.noeval;
              this_expr.lval = ec.lval;
              this_expr.lasttok = STR; /* ec.curtok */

              v2 = val + ((stok == POSTINC) ? 1 : -1);
              string vincdec = itos (v2);
              if (this_expr.noeval == 0)
                {
#if defined(ARRAY_VARS)
                  if (this_expr.lval.ind != -1)
                    expr_bind_array_element (this_expr.lval.tokstr,
                                             this_expr.lval.ind, vincdec);
                  else
#endif
                    expr_bind_variable (this_expr.tokstr, vincdec);
                }
              this_expr.curtok
                  = NUM; /* make sure x++=7 is flagged as an error */
            }
          else
            {
              /* XXX - watch out for pointer aliasing issues here */
              if (stok == STR) /* free new tokstr before old one is restored */
                delete[] this_expr.tokstr;
              this_expr = ec;
            }
        }

      readtok ();
    }
  else
    evalerror (_ ("syntax error: operand expected"));

  return val;
}

int64_t
Shell::expr_streval (char *tok, int e, token_lvalue *lvalue)
{
  SHELL_VAR *v;

  // itrace("expr_streval: %s: noeval = %d expanded=%d", tok.c_str (),
  //        this_expr.noeval, already_expanded);

  /* If we are suppressing evaluation, just short-circuit here instead of
     going through the rest of the evaluator. */
  if (this_expr.noeval)
    return 0;

  size_t initial_depth = expr_stack.size ();

#if defined(ARRAY_VARS)
  bool tflag = assoc_expand_once && already_expanded; /* for a start */
#endif

  /* [[[[[ */
#if defined(ARRAY_VARS)
  av_flags aflag = (tflag) ? AV_NOEXPAND : AV_NOFLAGS;
  v = (e == ']') ? array_variable_part (tok, tflag, nullptr, nullptr)
                 : find_variable (tok);
#else
  v = find_variable (tok);
#endif
  if (v == nullptr && e != ']')
    v = find_variable_last_nameref (tok, 0);

  if ((v == nullptr || v->invisible ()) && unbound_vars_is_error)
    {
      const char *value;
#if defined(ARRAY_VARS)
      string *str_value = nullptr;
      if (e == ']')
        {
          str_value = array_variable_name (tok, tflag, nullptr);
          value = str_value ? str_value->c_str () : tok;
        }
      else
#endif
        value = tok;

      set_exit_status (EXECUTION_FAILURE);
      err_unboundvar (value);

#if defined(ARRAY_VARS)
      if (e == ']')
        delete str_value; /* array_variable_name returns new memory */
#endif

      if (no_throw_on_fatal_error && interactive_shell)
        throw eval_exception ();

      if (interactive_shell)
        {
          expr_unwind ();
          top_level_cleanup ();
          throw bash_exception (DISCARD);
        }
      else
        throw bash_exception (FORCE_EOF);
    }

  string *value;
#if defined(ARRAY_VARS)
  array_eltstate_t es (-1);

  /* If the second argument to get_array_value doesn't include AV_ALLOWALL,
     we don't allow references like array[@].  In this case, get_array_value
     is just like get_variable_value in that it does not return newly-allocated
     memory or quote the results.  AFLAG is set above and is either AV_NOEXPAND
     or AV_NOFLAGS. */
  value = (e == ']') ? get_array_value (tok, aflag, &es)
                     : get_variable_value (v);
  arrayind_t ind = es.ind;
#else
  value = get_variable_value (v);
#endif

  if (expr_stack.size () < initial_depth)
    {
      if (no_throw_on_fatal_error && interactive_shell)
        throw eval_exception ();
      return 0;
    }

  int64_t tval = (value && !value->empty ()) ? subexpr (*value) : 0;

  if (lvalue)
    {
      lvalue->tokstr = tok; /* XXX */
      lvalue->tokval = tval;
      lvalue->tokvar = v; /* XXX */
#if defined(ARRAY_VARS)
      lvalue->ind = ind;
#else
      lvalue->ind = -1;
#endif
    }

  return tval;
}

static inline bool
_is_multiop (char c)
{
  switch (c)
    {
    case EQEQ:
    case NEQ:
    case LEQ:
    case GEQ:
    case LAND:
    case LOR:
    case LSH:
    case RSH:
    case OP_ASSIGN:
    case COND:
    case POWER:
    case PREINC:
    case PREDEC:
    case POSTINC:
    case POSTDEC:
      return true;
    default:
      return false;
    }
}

static inline bool
_is_arithop (char c)
{
  switch (c)
    {
    case EQ:
    case GT:
    case LT:
    case PLUS:
    case MINUS:
    case MUL:
    case DIV:
    case MOD:
    case NOT:
    case LPAR:
    case RPAR:
    case BAND:
    case BOR:
    case BXOR:
    case BNOT:
      return true; /* operator tokens */
    case QUES:
    case COL:
    case COMMA:
      return true; /* questionable */
    default:
      return false; /* anything else is invalid */
    }
}

/* Lexical analyzer/token reader for the expression evaluator.  Reads the
   next token and puts its value into curtok, while advancing past it.
   Updates value of tp.  May also set tokval (for number) or tokstr (for
   string). */
void
Shell::readtok ()
{
  char *cp, *xp;
  char c, c1, e;
  token_lvalue lval;

  /* Skip leading whitespace. */
  cp = this_expr.tp;
  c = e = 0;
  while (cp && (c = *cp) && (cr_whitespace (c)))
    cp++;

  if (c)
    cp++;

  if (c == '\0')
    {
      this_expr.lasttok = this_expr.curtok;
      this_expr.curtok = NONE;
      this_expr.tp = cp;
      return;
    }

  this_expr.lasttp = this_expr.tp = cp - 1;

  if (legal_variable_starter (c))
    {
      /* variable names not preceded with a dollar sign are shell variables. */
      char *savecp;
      EXPR_CONTEXT ec;
      int peektok;

      while (legal_variable_char (c))
        c = *cp++;

      c = *--cp;

#if defined(ARRAY_VARS)
      if (c == '[')
        {
          // XXX - was skipsubscript
          size_t pos = expr_skipsubscript (this_expr.tp, cp);
          if (cp[pos] == ']')
            {
              cp += pos + 1;
              c = *cp;
              e = ']';
            }
          else
            evalerror (bash_badsub_errmsg);
        }
#endif /* ARRAY_VARS */

      *cp = '\0';
      /* XXX - watch out for pointer aliasing issues here */
      if (this_expr.lval.tokstr && this_expr.lval.tokstr == this_expr.tokstr)
        this_expr.lval.init ();

      delete[] this_expr.tokstr;
      this_expr.tokstr = savestring (this_expr.tp);
      *cp = c;

      /* XXX - make peektok part of saved token state? */
      ec = this_expr;
      this_expr.tokstr = nullptr; /* keep it from being freed */
      this_expr.tp = savecp = cp;
      this_expr.noeval = 1;
      this_expr.curtok = STR;
      readtok ();
      peektok = this_expr.curtok;
      if (peektok == STR) /* free new tokstr before old one is restored */
        delete[] this_expr.tokstr;
      this_expr = ec;
      cp = savecp;

      /* The tests for PREINC and PREDEC aren't strictly correct, but they
         preserve old behavior if a construct like --x=9 is given. */
      if (this_expr.lasttok == PREINC || this_expr.lasttok == PREDEC
          || peektok != EQ)
        {
          lastlval = this_expr.lval;
          this_expr.tokval
              = expr_streval (this_expr.tokstr, e, &this_expr.lval);
        }
      else
        this_expr.tokval = 0;

      this_expr.lasttok = this_expr.curtok;
      this_expr.curtok = STR;
    }
  else if (c_isdigit (c))
    {
      while (c_isalnum (c) || c == '#' || c == '@' || c == '_')
        c = *cp++;

      c = *--cp;
      *cp = '\0';

      this_expr.tokval = strlong (this_expr.tp);
      *cp = c;
      this_expr.lasttok = this_expr.curtok;
      this_expr.curtok = NUM;
    }
  else
    {
      c1 = *cp++;
      if ((c == EQ) && (c1 == EQ))
        c = EQEQ;
      else if ((c == NOT) && (c1 == EQ))
        c = NEQ;
      else if ((c == GT) && (c1 == EQ))
        c = GEQ;
      else if ((c == LT) && (c1 == EQ))
        c = LEQ;
      else if ((c == LT) && (c1 == LT))
        {
          if (*cp == '=') /* a <<= b */
            {
              assigntok = LSH;
              c = OP_ASSIGN;
              cp++;
            }
          else
            c = LSH;
        }
      else if ((c == GT) && (c1 == GT))
        {
          if (*cp == '=')
            {
              assigntok = RSH; /* a >>= b */
              c = OP_ASSIGN;
              cp++;
            }
          else
            c = RSH;
        }
      else if ((c == BAND) && (c1 == BAND))
        c = LAND;
      else if ((c == BOR) && (c1 == BOR))
        c = LOR;
      else if ((c == '*') && (c1 == '*'))
        c = POWER;
      else if ((c == '-' || c == '+') && c1 == c && this_expr.curtok == STR)
        c = (c == '-') ? POSTDEC : POSTINC;
#ifdef STRICT_ARITH_PARSING
      else if ((c == '-' || c == '+') && c1 == c && this_expr.curtok == NUM)
#else
      else if ((c == '-' || c == '+') && c1 == c && this_expr.curtok == NUM
               && (this_expr.lasttok == PREINC || this_expr.lasttok == PREDEC))
#endif
        {
          /* This catches something like --FOO++ */
          /* TAG:bash-5.3 add gettext calls here or make this a separate
           * function */
          if (c == '-')
            evalerror ("--: assignment requires lvalue");
          else
            evalerror ("++: assignment requires lvalue");
        }
      else if ((c == '-' || c == '+') && c1 == c)
        {
          /* Quickly scan forward to see if this is followed by optional
             whitespace and an identifier. */
          xp = cp;
          while (xp && *xp && cr_whitespace (*xp))
            xp++;
          if (legal_variable_starter (*xp))
            c = (c == '-') ? PREDEC : PREINC;
          else
          /* Could force parsing as preinc or predec and throw an error */
#ifdef STRICT_ARITH_PARSING
            {
              /* Posix says unary plus and minus have higher priority than
                 preinc and predec. */
              /* This catches something like --4++ */
              if (c == '-')
                evalerror ("--: assignment requires lvalue");
              else
                evalerror ("++: assignment requires lvalue");
            }
#else
            cp--; /* not preinc or predec, so unget the character */
#endif
        }
      else if (c1 == EQ && member (c, "*/%+-&^|"))
        {
          assigntok = static_cast<token_t> (c); /* a OP= b */
          c = OP_ASSIGN;
        }
      else if (!_is_arithop (c))
        {
          cp--;
          /* use curtok, since it hasn't been copied to lasttok yet */
          if (this_expr.curtok == 0 || _is_arithop (this_expr.curtok)
              || _is_multiop (this_expr.curtok))
            evalerror (_ ("syntax error: operand expected"));
          else
            evalerror (_ ("syntax error: invalid arithmetic operator"));
        }
      else
        cp--; /* `unget' the character */

      /* Should check here to make sure that the current character is one
         of the recognized operators and flag an error if not.  Could create
         a character map the first time through and check it on subsequent
         calls. */
      this_expr.lasttok = this_expr.curtok;
      this_expr.curtok = static_cast<token_t> (c);
    }
  this_expr.tp = cp;
}

void
Shell::evalerror (const char *msg)
{
  const char *name, *t;

  name = this_command_name.c_str ();
  for (t = this_expr.expression; t && whitespace (*t); t++)
    ;

  internal_error (_ ("%s%s%s: %s (error token is \"%s\")"), name ? name : "",
                  name ? ": " : "", t ? t : "", msg,
                  (this_expr.lasttp && *this_expr.lasttp) ? this_expr.lasttp
                                                          : "");

  throw bash_exception (FORCE_EOF);
}

/* Convert a string to an int64_t integer, with an arbitrary base.
   0nnn -> base 8
   0[Xx]nn -> base 16
   Anything else: [base#]number (this is implemented to match ksh93)

   Base may be >=2 and <=64.  If base is <= 36, the numbers are drawn
   from [0-9][a-zA-Z], and lowercase and uppercase letters may be used
   interchangeably.  If base is > 36 and <= 64, the numbers are drawn
   from [0-9][a-z][A-Z]_@ (a = 10, z = 35, A = 36, Z = 61, @ = 62, _ = 63 --
   you get the picture). */

#define VALID_NUMCHAR(c) (c_isalnum (c) || ((c) == '_') || ((c) == '@'))

int64_t
Shell::strlong (const char *num)
{
  const char *s = num;

  int base = 10;
  bool foundbase = false;
  if (*s == '0')
    {
      s++;

      if (*s == '\0')
        return 0;

      /* Base 16? */
      if (*s == 'x' || *s == 'X')
        {
          base = 16;
          s++;
#ifdef STRICT_ARITH_PARSING
          if (*s == 0)
            evalerror (_ ("invalid number"));
#endif
        }
      else
        base = 8;
      foundbase = true;
    }

  int64_t val = 0;
  for (char c = *s++; c; c = *s++)
    {
      if (c == '#')
        {
          if (foundbase)
            evalerror (_ ("invalid number"));

          /* Illegal base specifications raise an evaluation error. */
          if (val < 2 || val > 64)
            evalerror (_ ("invalid arithmetic base"));

          base = static_cast<int> (val);
          val = 0;
          foundbase = true;

          /* Make sure a base# is followed by a character that can compose a
             valid integer constant. Jeremy Townshend
             <jeremy.townshend@gmail.com> */
          if (VALID_NUMCHAR (*s) == 0)
            evalerror (_ ("invalid integer constant"));
        }
      else if (VALID_NUMCHAR (c))
        {
          if (c_isdigit (c))
            c = todigit (c);
          else if (c >= 'a' && c <= 'z')
            c -= 'a' - 10;
          else if (c >= 'A' && c <= 'Z')
            c -= 'A' - ((base <= 36) ? 10 : 36);
          else if (c == '@')
            c = 62;
          else if (c == '_')
            c = 63;

          if (c >= base)
            evalerror (_ ("value too great for base"));

#ifdef CHECK_OVERFLOW
          int64_t pval = val;
          val = (val * base) + c;
          if (val < 0 || val < pval) /* overflow */
            return INTMAX_MAX;
#else
          val = (val * base) + c;
#endif
        }
      else
        break;
    }

  return (val);
}

} // namespace bash
