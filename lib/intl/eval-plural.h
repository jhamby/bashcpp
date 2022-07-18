/* eval-plural.c - Plural expression evaluation. */

/* Copyright (C) 2000-2002, 2006-2009 Free Software Foundation, Inc.

   This file is part of GNU Bash.

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

#ifndef STATIC
#define STATIC static
#endif

/* Evaluate the plural expression and return an index value.  */
STATIC unsigned long plural_eval (struct expression *pexp, unsigned long n)
     internal_function;

STATIC
unsigned long
internal_function
plural_eval (struct expression *pexp, unsigned long n)
{
  switch (pexp->nargs)
    {
    case 0:
      switch (pexp->operation)
	{
	case expression::var:
	  return n;
	case expression::num:
	  return pexp->val.num;
	default:
	  break;
	}
      /* NOTREACHED */
      break;
    case 1:
      {
	/* pexp->operation must be lnot.  */
	unsigned long int arg = plural_eval (pexp->val.args[0], n);
	return ! arg;
      }
    case 2:
      {
	unsigned long int leftarg = plural_eval (pexp->val.args[0], n);
	if (pexp->operation == expression::lor)
	  return leftarg || plural_eval (pexp->val.args[1], n);
	else if (pexp->operation == expression::land)
	  return leftarg && plural_eval (pexp->val.args[1], n);
	else
	  {
	    unsigned long int rightarg = plural_eval (pexp->val.args[1], n);

	    switch (pexp->operation)
	      {
	      case expression::mult:
		return leftarg * rightarg;
	      case expression::divide:
#if !INTDIV0_RAISES_SIGFPE
		if (rightarg == 0)
		  raise (SIGFPE);
#endif
		return leftarg / rightarg;
	      case expression::module:
#if !INTDIV0_RAISES_SIGFPE
		if (rightarg == 0)
		  raise (SIGFPE);
#endif
		return leftarg % rightarg;
	      case expression::plus:
		return leftarg + rightarg;
	      case expression::minus:
		return leftarg - rightarg;
	      case expression::less_than:
		return leftarg < rightarg;
	      case expression::greater_than:
		return leftarg > rightarg;
	      case expression::less_or_equal:
		return leftarg <= rightarg;
	      case expression::greater_or_equal:
		return leftarg >= rightarg;
	      case expression::equal:
		return leftarg == rightarg;
	      case expression::not_equal:
		return leftarg != rightarg;
	      default:
		break;
	      }
	  }
	/* NOTREACHED */
	break;
      }
    case 3:
      {
	/* pexp->operation must be qmop.  */
	bool boolarg = plural_eval (pexp->val.args[0], n);
	return plural_eval (pexp->val.args[boolarg ? 1 : 2], n);
      }
    }
  /* NOTREACHED */
  return 0;
}
