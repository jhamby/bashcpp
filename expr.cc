/* expr.c -- arithmetic expression evaluation. */

/* Copyright (C) 1990-2020 Free Software Foundation, Inc.

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
 All arithmetic is done as intmax_t integers with no checking for overflow
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

#include "config.hh"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include "chartypes.hh"
#include "bashintl.hh"

#include "shell.hh"
#include "arrayfunc.hh"
#include "execute_cmd.hh"
#include "flags.hh"
#include "subst.hh"
#include "typemax.hh"		/* INTMAX_MAX, INTMAX_MIN */

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
   highest precedence. */
#define EXP_HIGHEST	expcomma

#ifndef MAX_INT_LEN
#  define MAX_INT_LEN 32
#endif

static intmax_t ipow (intmax_t base, intmax_t exp);


#if 0
static std::string	expression;	/* The current expression */
static std::string	tp;		/* token lexical position */
static std::string	lasttp;		/* pointer to last token position */
static token_t		curtok;		/* the current token */
static token_t		lasttok;	/* the previous token */
static int		assigntok;	/* the OP in OP= */
static std::string	tokstr;		/* current token string */
static intmax_t		tokval;		/* current token value */
static int		noeval;		/* set to non-zero if no assignment to be done */

// static procenv_t evalbuf;

static struct lvalue curlval = {0, 0, 0, -1};
static struct lvalue lastlval = {0, 0, 0, -1};

static bool	_is_arithop (int);
static void	readtok ();	/* lexical analyzer */

static void	init_lvalue (struct lvalue *);
static struct lvalue *alloc_lvalue ();
static void	free_lvalue (struct lvalue *);

static intmax_t	expr_streval (std::string &, int, struct lvalue *);
static intmax_t	strlong (const std::string &);
static void	evalerror (const std::string &);

static void	popexp ();
static void	expr_unwind ();
static void	expr_bind_variable (std::string &, std::string &);
#if defined (ARRAY_VARS)
static void	expr_bind_array_element (const std::string &, arrayind_t, std::string &);
#endif

static intmax_t subexpr (const std::string &);

static intmax_t	expcomma ();
static intmax_t expassign ();
static intmax_t	expcond ();
static intmax_t explor ();
static intmax_t expland ();
static intmax_t	expbor ();
static intmax_t	expbxor ();
static intmax_t	expband ();
static intmax_t exp5 ();
static intmax_t exp4 ();
static intmax_t expshift ();
static intmax_t exp3 ();
static intmax_t expmuldiv ();
static intmax_t	exppower ();
static intmax_t exp1 ();
static intmax_t exp0 ();


#if defined (ARRAY_VARS)
extern const std::string bash_badsub_errmsg;
#endif

#define SAVETOK(X) \
  do { \
    (X)->curtok = curtok; \
    (X)->lasttok = lasttok; \
    (X)->tp = tp; \
    (X)->lasttp = lasttp; \
    (X)->tokval = tokval; \
    (X)->tokstr = tokstr; \
    (X)->noeval = noeval; \
    (X)->lval = curlval; \
  } while (0)

#define RESTORETOK(X) \
  do { \
    curtok = (X)->curtok; \
    lasttok = (X)->lasttok; \
    tp = (X)->tp; \
    lasttp = (X)->lasttp; \
    tokval = (X)->tokval; \
    tokstr = (X)->tokstr; \
    noeval = (X)->noeval; \
    curlval = (X)->lval; \
  } while (0)
#endif



/* Push and save away the contents of the globals describing the
   current expression context. */
void
Shell::pushexp ()
{
  if (expr_stack.size () >= MAX_EXPR_RECURSION_LEVEL)
    evalerror (_("expression recursion level exceeded"));

  EXPR_CONTEXT *context = new EXPR_CONTEXT (expr_current);
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
      expr_current.expression.clear();
      expr_current.lasttp.clear();
      evalerror (_("recursion stack underflow"));
    }

  expr_current = *(expr_stack.back ());
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

  expr_current.noeval = false;	/* XXX */
}

void
Shell::expr_bind_variable (const std::string &lhs, const std::string &rhs)
{
  SHELL_VAR *v;
  assign_flags aflags;

  if (lhs.empty ())
    return;		/* XXX */

#if defined (ARRAY_VARS)
  aflags = (assoc_expand_once && already_expanded) ? ASS_NOEXPAND : ASS_NOFLAGS;
#else
  aflags = ASS_NOFLAGS;
#endif
  v = bind_int_variable (lhs, rhs, aflags);
  if (v && v->readonly () || v->noassign ())
    throw force_eof_exception ();	/* variable assignment error */

  stupidly_hack_special_variables (lhs);
}

#if defined (ARRAY_VARS)
/* This is similar to the logic in arrayfunc.c:valid_array_subscript when
   you pass VA_NOEXPAND. */
int
Shell::expr_skipsubscript (const char *vp, char *cp)
{
  bool isassoc = false;
  SHELL_VAR *entry = nullptr;
  if (assoc_expand_once & already_expanded)
    {
      *cp = '\0';
      isassoc = legal_identifier (vp) && (entry = find_variable (vp)) && entry->assoc ();
      *cp = '[';	/* ] */
    }
  valid_array_flags flags = (isassoc && assoc_expand_once && already_expanded)
				? VA_NOEXPAND : VA_NOFLAGS;
  return skipsubscript (cp, 0, flags);
}

/* Rewrite tok, which is of the form vname[expression], to vname[ind], where
   IND is the already-calculated value of expression. */
void
Shell::expr_bind_array_element (const std::string &tok, arrayind_t ind, const std::string &rhs)
{
  char ibuf[INT_STRLEN_BOUND (arrayind_t) + 1];

  char *istr = fmtumax (ind, 10, ibuf, sizeof (ibuf), 0);
  char *vname = array_variable_name (tok.c_str (), 0, nullptr, nullptr);

  std::string lhs (vname);
  lhs.push_back ('[');
  lhs += istr;
  lhs.push_back (']');

/*itrace("expr_bind_array_element: %s=%s", lhs, rhs);*/
  expr_bind_variable (lhs, rhs);
  delete[] vname;
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
intmax_t
Shell::evalexp (const std::string &expr, eval_flags flags, bool *validp)
{
  try
    {
      expr_current.noeval = 0;
      already_expanded = (flags & EXP_EXPANDED);

      intmax_t val = subexpr (expr);

      if (validp)
	*validp = true;

      return val;
    }
  catch (const force_eof_exception& e)
    {
      expr_current.tokstr.clear ();
      expr_current.expression.clear ();

      expr_unwind ();

      if (validp)
	*validp = false;
      return 0;
    }
}

intmax_t
Shell::subexpr (const std::string &expr)
{
  intmax_t val;
  const char *p;

  for (p = expr.c_str (); p && *p && cr_whitespace (*p); p++)
    ;

  if (*p == '\0')
    return 0;

  pushexp ();
  expr_current.expression = expr;
  expr_current.tp = expr_current.expression;

  expr_current.curtok = expr_current.lasttok = NONE;
  expr_current.tokstr = nullptr;
  expr_current.tokval = 0;
  expr_current.lval.init ();
  lastlval = expr_current.lval;

  readtok ();

  val = EXP_HIGHEST ();

  if (expr_current.curtok != NONE)
    evalerror (_("syntax error in expression"));

  expr_current.tokstr.clear ();
  expr_current.expression.clear ();

  popexp ();

  return val;
}

intmax_t
Shell::expcomma ()
{
  intmax_t value;

  value = expassign ();
  while (expr_current.curtok == COMMA)
    {
      readtok ();
      value = expassign ();
    }

  return value;
}

intmax_t
Shell::expassign ()
{
  intmax_t value;
//   std::string &lhs, *rhs;
  arrayind_t lind;
#if defined (HAVE_IMAXDIV)
  imaxdiv_t idiv;
#endif

  value = expcond ();
  if (expr_current.curtok == EQ || expr_current.curtok == OP_ASSIGN)
    {
      token_t op;
      intmax_t lvalue;

      bool special = (expr_current.curtok == OP_ASSIGN);

      if (expr_current.lasttok != STR)
	evalerror (_("attempted assignment to non-variable"));

      if (special)
	{
	  op = assigntok;		/* a OP= b */
	  lvalue = value;
	}

      if (expr_current.tokstr.empty ())
	evalerror (_("syntax error in variable assignment"));

      std::string lhs (expr_current.tokstr);
      /* save ind in case rhs is string var and evaluation overwrites it */
      lind = expr_current.lval.ind;
      readtok ();
      value = expassign ();

      if (special)
	{
	  if ((op == DIV || op == MOD) && value == 0)
	    {
	      if (expr_current.noeval == 0)
		evalerror (_("division by 0"));
	      else
	        value = 1;
	    }

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
#if HAVE_IMAXDIV
		{
		  idiv = imaxdiv (lvalue, value);
		  lvalue = (op == DIV) ? idiv.quot : idiv.rem;
		}
#else
	        lvalue = (op == DIV) ? lvalue / value : lvalue % value;
#endif
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
	      evalerror (_("bug: bad expassign token"));
	      break;
	    }
	  value = lvalue;
	}

      char *rhs = itos (value);
      if (expr_current.noeval == 0)
	{
#if defined (ARRAY_VARS)
	  if (lind != -1)
	    expr_bind_array_element (lhs, lind, rhs);
	  else
#endif
	    expr_bind_variable (lhs, rhs);
	}
      if (expr_current.lval.tokstr == expr_current.tokstr)
	expr_current.lval.init ();

      delete[] rhs;
      expr_current.tokstr.clear ();		/* For freeing on errors. */
    }

  return value;
}

/* Conditional expression (expr?expr:expr) */
intmax_t
Shell::expcond ()
{
  intmax_t cval, val1, val2, rval;
  bool set_noeval = false;

  rval = cval = explor ();
  if (expr_current.curtok == QUES)		/* found conditional expr */
    {
      if (cval == 0)
	{
	  set_noeval = true;
	  expr_current.noeval++;
	}

      readtok ();
      if (expr_current.curtok == NONE || expr_current.curtok == COL)
	evalerror (_("expression expected"));

      val1 = EXP_HIGHEST ();

      if (set_noeval)
	expr_current.noeval--;
      if (expr_current.curtok != COL)
	evalerror (_("`:' expected for conditional expression"));

      set_noeval = false;
      if (cval)
 	{
 	  set_noeval = true;
	  expr_current.noeval++;
 	}

      readtok ();
      if (expr_current.curtok == 0)
	evalerror (_("expression expected"));
      val2 = expcond ();

      if (set_noeval)
	expr_current.noeval--;
      rval = cval ? val1 : val2;
      expr_current.lasttok = COND;
    }
  return rval;
}

/* Logical OR. */
intmax_t
Shell::explor ()
{
  intmax_t val1, val2;

  val1 = expland ();

  while (expr_current.curtok == LOR)
    {
      bool set_noeval = false;
      if (val1 != 0)
	{
	  expr_current.noeval++;
	  set_noeval = true;
	}
      readtok ();
      val2 = expland ();
      if (set_noeval)
	expr_current.noeval--;
      val1 = val1 || val2;
      expr_current.lasttok = LOR;
    }

  return val1;
}

/* Logical AND. */
intmax_t
Shell::expland ()
{
  intmax_t val1, val2;

  val1 = expbor ();

  while (expr_current.curtok == LAND)
    {
      bool set_noeval = false;
      if (val1 == 0)
	{
	  set_noeval = true;
	  expr_current.noeval++;
	}
      readtok ();
      val2 = expbor ();
      if (set_noeval)
	expr_current.noeval--;
      val1 = val1 && val2;
      expr_current.lasttok = LAND;
    }

  return val1;
}

/* Bitwise OR. */
intmax_t
Shell::expbor ()
{
  intmax_t val1, val2;

  val1 = expbxor ();

  while (expr_current.curtok == BOR)
    {
      readtok ();
      val2 = expbxor ();
      val1 = val1 | val2;
      expr_current.lasttok = NUM;
    }

  return val1;
}

/* Bitwise XOR. */
intmax_t
Shell::expbxor ()
{
  intmax_t val1, val2;

  val1 = expband ();

  while (expr_current.curtok == BXOR)
    {
      readtok ();
      val2 = expband ();
      val1 = val1 ^ val2;
      expr_current.lasttok = NUM;
    }

  return val1;
}

/* Bitwise AND. */
intmax_t
Shell::expband ()
{
  intmax_t val1, val2;

  val1 = exp5 ();

  while (expr_current.curtok == BAND)
    {
      readtok ();
      val2 = exp5 ();
      val1 = val1 & val2;
      expr_current.lasttok = NUM;
    }

  return val1;
}

intmax_t
Shell::exp5 ()
{
  intmax_t val1, val2;

  val1 = exp4 ();

  while ((expr_current.curtok == EQEQ) || (expr_current.curtok == NEQ))
    {
      token_t op = expr_current.curtok;

      readtok ();
      val2 = exp4 ();
      if (op == EQEQ)
	val1 = (val1 == val2);
      else if (op == NEQ)
	val1 = (val1 != val2);
      expr_current.lasttok = NUM;
    }
  return val1;
}

intmax_t
Shell::exp4 ()
{
  intmax_t val1, val2;

  val1 = expshift ();
  while ((expr_current.curtok == LEQ) ||
	 (expr_current.curtok == GEQ) ||
	 (expr_current.curtok == LT) ||
	 (expr_current.curtok == GT))
    {
      token_t op = expr_current.curtok;

      readtok ();
      val2 = expshift ();

      if (op == LEQ)
	val1 = val1 <= val2;
      else if (op == GEQ)
	val1 = val1 >= val2;
      else if (op == LT)
	val1 = val1 < val2;
      else			/* (op == GT) */
	val1 = val1 > val2;
      expr_current.lasttok = NUM;
    }
  return val1;
}

/* Left and right shifts. */
intmax_t
Shell::expshift ()
{
  intmax_t val1, val2;

  val1 = exp3 ();

  while ((expr_current.curtok == LSH) || (expr_current.curtok == RSH))
    {
      token_t op = expr_current.curtok;

      readtok ();
      val2 = exp3 ();

      if (op == LSH)
	val1 = val1 << val2;
      else
	val1 = val1 >> val2;
      expr_current.lasttok = NUM;
    }

  return val1;
}

intmax_t
Shell::exp3 ()
{
  intmax_t val1, val2;

  val1 = expmuldiv ();

  while ((expr_current.curtok == PLUS) || (expr_current.curtok == MINUS))
    {
      token_t op = expr_current.curtok;

      readtok ();
      val2 = expmuldiv ();

      if (op == PLUS)
	val1 += val2;
      else if (op == MINUS)
	val1 -= val2;
      expr_current.lasttok = NUM;
    }
  return val1;
}

intmax_t
Shell::expmuldiv ()
{
  intmax_t val1, val2;
#if defined (HAVE_IMAXDIV)
  imaxdiv_t idiv;
#endif

  val1 = exppower ();

  while ((expr_current.curtok == MUL) ||
	 (expr_current.curtok == DIV) ||
	 (expr_current.curtok == MOD))
    {
      token_t op = expr_current.curtok;

      std::string &stp = expr_current.tp;
      readtok ();

      val2 = exppower ();

      /* Handle division by 0 and twos-complement arithmetic overflow */
      if (((op == DIV) || (op == MOD)) && (val2 == 0))
	{
	  if (expr_current.noeval == 0)
	    {
	      std::string sltp (expr_current.lasttp);
	      expr_current.lasttp = stp;
	      while (!expr_current.lasttp.empty () &&
		     whitespace (expr_current.lasttp[0]))
		expr_current.lasttp.erase(0, 1);
	      evalerror (_("division by 0"));
	      expr_current.lasttp = sltp;
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
#if defined (HAVE_IMAXDIV)
	{
	  idiv = imaxdiv (val1, val2);
	  val1 = (op == DIV) ? idiv.quot : idiv.rem;
	}
#else
	val1 = (op == DIV) ? val1 / val2 : val1 % val2;
#endif
      expr_current.lasttok = NUM;
    }
  return val1;
}

static inline intmax_t
ipow (intmax_t base, intmax_t exp)
{
  intmax_t result;

  result = 1;
  while (exp)
    {
      if (exp & 1)
	result *= base;
      exp >>= 1;
      base *= base;
    }
  return result;
}

intmax_t
Shell::exppower ()
{
  intmax_t val1, val2, c;

  val1 = exp1 ();
  while (expr_current.curtok == POWER)
    {
      readtok ();
      val2 = exppower ();	/* exponentiation is right-associative */
      expr_current.lasttok = NUM;
      if (val2 == 0)
	return 1;
      if (val2 < 0)
	evalerror (_("exponent less than 0"));
      val1 = ipow (val1, val2);
    }
  return val1;
}

intmax_t
Shell::exp1 ()
{
  intmax_t val;

  if (expr_current.curtok == NOT)
    {
      readtok ();
      val = !exp1 ();
      expr_current.lasttok = NUM;
    }
  else if (expr_current.curtok == BNOT)
    {
      readtok ();
      val = ~exp1 ();
      expr_current.lasttok = NUM;
    }
  else if (expr_current.curtok == MINUS)
    {
      readtok ();
      val = - exp1 ();
      expr_current.lasttok = NUM;
    }
  else if (expr_current.curtok == PLUS)
    {
      readtok ();
      val = exp1 ();
      expr_current.lasttok = NUM;
    }
  else
    val = exp0 ();

  return val;
}

intmax_t
Shell::exp0 ()
{
  intmax_t val = 0, v2;
  char *vincdec;
  EXPR_CONTEXT ec;

  /* XXX - might need additional logic here to decide whether or not
	   pre-increment or pre-decrement is legal at this point. */
  if (expr_current.curtok == PREINC || expr_current.curtok == PREDEC)
    {
      token_t stok = expr_current.lasttok = expr_current.curtok;
      readtok ();
      if (expr_current.curtok != STR)
	/* readtok() catches this */
	evalerror (_("identifier expected after pre-increment or pre-decrement"));

      v2 = expr_current.tokval + ((stok == PREINC) ? 1 : -1);
      vincdec = itos (v2);
      if (expr_current.noeval == 0)
	{
#if defined (ARRAY_VARS)
	  if (expr_current.lval.ind != -1)
	    expr_bind_array_element (expr_current.lval.tokstr, expr_current.lval.ind, vincdec);
	  else
#endif
	    if (!expr_current.tokstr.empty ())
	      expr_bind_variable (expr_current.tokstr, vincdec);
	}
      delete[] vincdec;
      val = v2;

      curtok = NUM;	/* make sure --x=7 is flagged as an error */
      readtok ();
    }
  else if (curtok == LPAR)
    {
      /* XXX - save curlval here?  Or entire expression context? */
      readtok ();
      val = EXP_HIGHEST ();

      if (curtok != RPAR) /* ( */
	evalerror (_("missing `)'"));

      /* Skip over closing paren. */
      readtok ();
    }
  else if ((curtok == NUM) || (curtok == STR))
    {
      val = expr_current.tokval;
      if (curtok == STR)
	{
	  SAVETOK (&ec);
	  expr_current.tokstr = nullptr;	/* keep it from being freed */
          expr_current.noeval = 1;
          readtok ();
          stok = curtok;

	  /* post-increment or post-decrement */
 	  if (stok == POSTINC || stok == POSTDEC)
 	    {
 	      /* restore certain portions of EC */
	      expr_current.tokstr = ec.tokstr;
	      expr_current.noeval = ec.noeval;
	      expr_current.lval = ec.lval;
	      expr_current.lasttok = STR;	/* ec.curtok */

	      v2 = val + ((stok == POSTINC) ? 1 : -1);
	      vincdec = itos (v2);
	      if (expr_current.noeval == 0)
		{
#if defined (ARRAY_VARS)
		  if (expr_current.lval.ind != -1)
		    expr_bind_array_element (expr_current.lval.tokstr, expr_current.lval.ind, vincdec);
		  else
#endif
		    expr_bind_variable (expr_current.tokstr, vincdec);
		}
	      free (vincdec);
	      curtok = NUM;	/* make sure x++=7 is flagged as an error */
 	    }
 	  else
 	    {
	      /* XXX - watch out for pointer aliasing issues here */
	      if (stok == STR)	/* free new tokstr before old one is restored */
		FREE (tokstr);
	      RESTORETOK (&ec);
 	    }
	}

      readtok ();
    }
  else
    evalerror (_("syntax error: operand expected"));

  return val;
}

#if 0
void
Shell::init_lvalue (struct lvalue *lv)
{
  lv->tokstr = 0;
  lv->tokvar = 0;
  lv->tokval = lv->ind = -1;
}

static struct lvalue *
alloc_lvalue ()
{
  struct lvalue *lv;

  lv = (struct lvalue *)xmalloc (sizeof (struct lvalue));
  init_lvalue (lv);
  return lv;
}

static void
free_lvalue (struct lvalue *lv)
{
  free (lv);		/* should be inlined */
}
#endif

intmax_t
Shell::expr_streval (std::string &tok, int e, struct lvalue *lvalue)
{
  SHELL_VAR *v;
//   std::string &value;
//   intmax_t tval;

/*itrace("expr_streval: %s: noeval = %d expanded=%d", tok, noeval, already_expanded);*/
  /* If we are suppressing evaluation, just short-circuit here instead of
     going through the rest of the evaluator. */
  if (expr_current.noeval)
    return 0;

  size_t initial_depth = expr_stack.size ();

#if defined (ARRAY_VARS)
  int tflag = assoc_expand_once && already_expanded;	/* for a start */
#endif

  /* [[[[[ */
#if defined (ARRAY_VARS)
  av_flags aflag = (tflag) ? AV_NOEXPAND : AV_NOFLAGS;
  v = (e == ']') ? array_variable_part (tok, tflag, (std::string &*)0, (int *)0) : find_variable (tok);
#else
  v = find_variable (tok);
#endif
  if (v == 0 && e != ']')
    v = find_variable_last_nameref (tok, 0);

  if ((v == 0 || invisible_p (v)) && unbound_vars_is_error)
    {
#if defined (ARRAY_VARS)
      value = (e == ']') ? array_variable_name (tok, tflag, (std::string &*)0, (int *)0) : tok;
#else
      value = tok;
#endif

      set_exit_status (EXECUTION_FAILURE);
      err_unboundvar (value);

#if defined (ARRAY_VARS)
      if (e == ']')
	FREE (value);	/* array_variable_name returns new memory */
#endif

      if (no_longjmp_on_fatal_error && interactive_shell)
	sh_longjmp (evalbuf, 1);

      if (interactive_shell)
	{
	  expr_unwind ();
	  top_level_cleanup ();
	  jump_to_top_level (DISCARD);
	}
      else
	jump_to_top_level (FORCE_EOF);
    }

#if defined (ARRAY_VARS)
  ind = -1;
  /* If the second argument to get_array_value doesn't include AV_ALLOWALL,
     we don't allow references like array[@].  In this case, get_array_value
     is just like get_variable_value in that it does not return newly-allocated
     memory or quote the results.  AFLAG is set above and is either AV_NOEXPAND
     or 0. */
  value = (e == ']') ? get_array_value (tok, aflag, nullptr, &ind) : get_variable_value (v);
#else
  value = get_variable_value (v);
#endif

  if (expr_depth < initial_depth)
    {
      if (no_longjmp_on_fatal_error && interactive_shell)
	sh_longjmp (evalbuf, 1);
      return 0;
    }

  tval = (value && *value) ? subexpr (value) : 0;

  if (lvalue)
    {
      lvalue->tokstr = tok;	/* XXX */
      lvalue->tokval = tval;
      lvalue->tokvar = v;	/* XXX */
#if defined (ARRAY_VARS)
      lvalue->ind = ind;
#else
      lvalue->ind = -1;
#endif
    }

  return tval;
}

static inline bool
_is_multiop (token_t c)
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
_is_arithop (token_t c)
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
      return true;		/* operator tokens */
    case QUES:
    case COL:
    case COMMA:
      return true;		/* questionable */
    default:
      return false;		/* anything else is invalid */
    }
}

/* Lexical analyzer/token reader for the expression evaluator.  Reads the
   next token and puts its value into curtok, while advancing past it.
   Updates value of tp.  May also set tokval (for number) or tokstr (for
   string). */
void
Shell::readtok ()
{
  std::string &cp, *xp;
  unsigned char c, c1;
  int e;
  struct lvalue lval;

  /* Skip leading whitespace. */
  cp = tp;
  c = e = 0;
  while (cp && (c = *cp) && (cr_whitespace (c)))
    cp++;

  if (c)
    cp++;

  if (c == '\0')
    {
      lasttok = curtok;
      curtok = 0;
      tp = cp;
      return;
    }
  lasttp = tp = cp - 1;

  if (legal_variable_starter (c))
    {
      /* variable names not preceded with a dollar sign are shell variables. */
      std::string &savecp;
      EXPR_CONTEXT ec;
      int peektok;

      while (legal_variable_char (c))
	c = *cp++;

      c = *--cp;

#if defined (ARRAY_VARS)
      if (c == '[')
	{
	  e = expr_skipsubscript (tp, cp);		/* XXX - was skipsubscript */
	  if (cp[e] == ']')
	    {
	      cp += e + 1;
	      c = *cp;
	      e = ']';
	    }
	  else
	    evalerror (bash_badsub_errmsg);
	}
#endif /* ARRAY_VARS */

      *cp = '\0';
      /* XXX - watch out for pointer aliasing issues here */
      if (curlval.tokstr && curlval.tokstr == tokstr)
	init_lvalue (&curlval);

      FREE (tokstr);
      tokstr = savestring (tp);
      *cp = c;

      /* XXX - make peektok part of saved token state? */
      SAVETOK (&ec);
      tokstr = nullptr;	/* keep it from being freed */
      tp = savecp = cp;
      noeval = 1;
      curtok = STR;
      readtok ();
      peektok = curtok;
      if (peektok == STR)	/* free new tokstr before old one is restored */
	FREE (tokstr);
      RESTORETOK (&ec);
      cp = savecp;

      /* The tests for PREINC and PREDEC aren't strictly correct, but they
	 preserve old behavior if a construct like --x=9 is given. */
      if (lasttok == PREINC || lasttok == PREDEC || peektok != EQ)
        {
          lastlval = curlval;
	  tokval = expr_streval (tokstr, e, &curlval);
        }
      else
	tokval = 0;

      lasttok = curtok;
      curtok = STR;
    }
  else if (DIGIT(c))
    {
      while (ISALNUM (c) || c == '#' || c == '@' || c == '_')
	c = *cp++;

      c = *--cp;
      *cp = '\0';

      tokval = strlong (tp);
      *cp = c;
      lasttok = curtok;
      curtok = NUM;
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
	  if (*cp == '=')	/* a <<= b */
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
	      assigntok = RSH;	/* a >>= b */
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
      else if ((c == '-' || c == '+') && c1 == c && curtok == STR)
	c = (c == '-') ? POSTDEC : POSTINC;
      else if ((c == '-' || c == '+') && c1 == c && curtok == NUM && (lasttok == PREINC || lasttok == PREDEC))
	{
	  /* This catches something like --FOO++ */
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
	  if (legal_variable_starter ((unsigned char)*xp))
	    c = (c == '-') ? PREDEC : PREINC;
	  else
	    /* Could force parsing as preinc or predec and throw an error */
#if 0
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
	    cp--;	/* not preinc or predec, so unget the character */
#endif
	}
      else if (c1 == EQ && member (c, "*/%+-&^|"))
	{
	  assigntok = c;	/* a OP= b */
	  c = OP_ASSIGN;
	}
      else if (_is_arithop (c) == 0)
	{
	  cp--;
	  /* use curtok, since it hasn't been copied to lasttok yet */
	  if (curtok == 0 || _is_arithop (curtok) || _is_multiop (curtok))
	    evalerror (_("syntax error: operand expected"));
	  else
	    evalerror (_("syntax error: invalid arithmetic operator"));
	}
      else
	cp--;			/* `unget' the character */

      /* Should check here to make sure that the current character is one
	 of the recognized operators and flag an error if not.  Could create
	 a character map the first time through and check it on subsequent
	 calls. */
      lasttok = curtok;
      curtok = c;
    }
  tp = cp;
}

void
Shell::evalerror (const std::string &msg)
{
  const std::string &name, *t;

  name = this_command_name;
  for (t = expression; t && whitespace (*t); t++)
    ;
  internal_error (_("%s%s%s: %s (error token is \"%s\")"),
		   name ? name : "", name ? ": " : "",
		   t ? t : "", msg, (lasttp && *lasttp) ? lasttp : "");
  sh_longjmp (evalbuf, 1);
}

/* Convert a string to an intmax_t integer, with an arbitrary base.
   0nnn -> base 8
   0[Xx]nn -> base 16
   Anything else: [base#]number (this is implemented to match ksh93)

   Base may be >=2 and <=64.  If base is <= 36, the numbers are drawn
   from [0-9][a-zA-Z], and lowercase and uppercase letters may be used
   interchangeably.  If base is > 36 and <= 64, the numbers are drawn
   from [0-9][a-z][A-Z]_@ (a = 10, z = 35, A = 36, Z = 61, @ = 62, _ = 63 --
   you get the picture). */

#define VALID_NUMCHAR(c)	(ISALNUM(c) || ((c) == '_') || ((c) == '@'))

intmax_t
Shell::strlong (const std::string &num)
{
  const std::string &s;
  unsigned char c;
  int base, foundbase;
  intmax_t val;

  s = num;

  base = 10;
  foundbase = 0;
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
	}
      else
	base = 8;
      foundbase++;
    }

  val = 0;
  for (c = *s++; c; c = *s++)
    {
      if (c == '#')
	{
	  if (foundbase)
	    evalerror (_("invalid number"));

	  /* Illegal base specifications raise an evaluation error. */
	  if (val < 2 || val > 64)
	    evalerror (_("invalid arithmetic base"));

	  base = val;
	  val = 0;
	  foundbase++;

	  /* Make sure a base# is followed by a character that can compose a
	     valid integer constant. Jeremy Townshend <jeremy.townshend@gmail.com> */
	  if (VALID_NUMCHAR (*s) == 0)
	    evalerror (_("invalid integer constant"));
	}
      else if (VALID_NUMCHAR (c))
	{
	  if (DIGIT(c))
	    c = TODIGIT(c);
	  else if (c >= 'a' && c <= 'z')
	    c -= 'a' - 10;
	  else if (c >= 'A' && c <= 'Z')
	    c -= 'A' - ((base <= 36) ? 10 : 36);
	  else if (c == '@')
	    c = 62;
	  else if (c == '_')
	    c = 63;

	  if (c >= base)
	    evalerror (_("value too great for base"));

	  val = (val * base) + c;
	}
      else
	break;
    }

  return val;
}

}  //namespace bash

#if defined (EXPR_TEST)

SHELL_VAR *find_variable () { return 0;}
SHELL_VAR *bind_variable () { return 0; }

char *get_string_value () { return 0; }

procenv_t top_level;

main (int argc, char **argv)
{
  int i;
  intmax_t v;
  int expok;

  if (setjmp (top_level))
    exit (0);

  for (i = 1; i < argc; i++)
    {
      v = evalexp (argv[i], 0, &expok);
      if (expok == 0)
	fprintf (stderr, _("%s: expression error\n"), argv[i]);
      else
	printf ("'%s' -> %ld\n", argv[i], v);
    }
  exit (0);
}

int
builtin_error (const char *format, int arg1, int arg2, int arg3, int arg4, int arg5)
{
  fprintf (stderr, "expr: ");
  fprintf (stderr, format, arg1, arg2, arg3, arg4, arg5);
  fprintf (stderr, "\n");
  return 0;
}

char *
itos (intmax_t n)
{
  return "42";
}

#endif /* EXPR_TEST */
