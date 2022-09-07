/* variables.h -- data structures for shell variables. */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

#if !defined (_VARIABLES_H_)
#define _VARIABLES_H_

#include "array.h"
#include "assoc.h"
#include "command.h"
#include "shell.h"

/* Shell variables and functions are stored in hash tables. */
#include "hashlib.h"

#include "conftypes.h"

namespace bash
{

/* Flags for VAR_CONTEXT->flags. */
enum vc_flags {
  VC_HASLOCAL =		0x01,
  VC_HASTMPVAR =	0x02,
  VC_FUNCENV =		0x04,	/* also function if name != NULL */
  VC_BLTNENV =		0x08,	/* builtin_env */
  VC_TEMPENV =		0x10,	/* temporary_env */
  VC_TEMPFLAGS =	(VC_FUNCENV | VC_BLTNENV | VC_TEMPENV)
};

/* A variable context. */
struct VAR_CONTEXT {
  char *name;		/* empty or NULL means global context */
  int scope;		/* 0 means global context */
  vc_flags flags;
  VAR_CONTEXT *up;	/* previous function calls */
  VAR_CONTEXT *down;	/* down towards global context */
  HASH_TABLE *table;	/* variables at this scope */

  /* Inline access methods. */
  bool is_func_env()	{ return (flags & VC_FUNCENV); }
  bool is_builtin_env()	{ return (flags & VC_BLTNENV); }
  bool is_temp_env()	{ return ((flags & VC_TEMPFLAGS) == VC_TEMPENV); }
  bool is_temp_scope()	{ return (flags & (VC_TEMPENV | VC_BLTNENV)); }
  bool has_locals()	{ return (flags & VC_HASLOCAL); }
  bool has_temp_vars()	{ return (flags & VC_HASTMPVAR); }
};

/* The various attributes that a given variable can have. */
/* First, the user-visible attributes */
enum var_att_flags {
  att_exported =	0x0000001,	/* export to environment */
  att_readonly =	0x0000002,	/* cannot change */
  att_array =		0x0000004,	/* value is an array */
  att_function =	0x0000008,	/* value is a function */
  att_integer =		0x0000010,	/* internal representation is int */
  att_local =		0x0000020,	/* variable is local to a function */
  att_assoc =		0x0000040,	/* variable is an associative array */
  att_trace =		0x0000080,	/* function is traced with DEBUG trap */
  att_uppercase =	0x0000100,	/* word converted to uppercase on assignment */
  att_lowercase =	0x0000200,	/* word converted to lowercase on assignment */
  att_capcase =		0x0000400,	/* word capitalized on assignment */
  att_nameref =		0x0000800,	/* word is a name reference */

  user_attrs =	(att_exported | att_readonly | att_integer | att_local
			 | att_trace | att_uppercase | att_lowercase | att_capcase
			 | att_nameref ),

  attmask_user =	0x0000fff,

/* Internal attributes used for bookkeeping */
  att_invisible =	0x0001000,	/* cannot see */
  att_nounset =		0x0002000,	/* cannot unset */
  att_noassign =	0x0004000,	/* assignment not allowed */
  att_imported =	0x0008000,	/* came from environment */
  att_special =		0x0010000,	/* requires special handling */
  att_nofree =		0x0020000,	/* do not free value on unset */
  att_regenerate =	0x0040000,	/* regenerate when exported */

  attmask_int =		0x00ff000,

/* Internal attributes used for variable scoping. */
  att_tempvar =		0x0100000,	/* variable came from the temp environment */
  att_propagate =	0x0200000,	/* propagate to previous scope */

  attmask_scope =	0x0f00000
};

/* Union of supported types for a shell variable. */
union Value {
  char *s;			/* string value */
  COMMAND *cmd;			/* function */
  ARRAY *a;			/* array */
  HASH_TABLE *h;		/* associative array */
};

/* Forward declaration of shell class for callback functions. */
class Shell;

/* What a shell variable looks like. */
class SHELL_VAR {
public:
  SHELL_VAR();

  typedef SHELL_VAR *(Shell::*sh_var_value_func_t) (SHELL_VAR *);
  typedef SHELL_VAR *(Shell::*sh_var_assign_func_t) (SHELL_VAR *, const char *, arrayind_t, const char *);

  char *name() { return name_; }
  void set_name(char *new_name) { name_ = new_name; }

  char *value() { return value_.s; }

  COMMAND *command() { return value_.cmd; }

  ARRAY *array() { return value_.a; }

  HASH_TABLE *assoc() { return value_.h; }

  char *name_ref() { return value_.s; }		/* so it can change later */

  /* Inline access methods. */
  bool is_exported()		{ return (attributes & att_exported); }
  bool is_readonly()		{ return (attributes & att_readonly); }
  bool is_array()		{ return (attributes & att_array); }
  bool is_function()		{ return (attributes & att_function); }
  bool is_integer()		{ return (attributes & att_integer); }
  bool is_local()		{ return (attributes & att_local); }
  bool is_assoc()		{ return (attributes & att_assoc); }
  bool is_trace()		{ return (attributes & att_trace); }
  bool is_uppercase()		{ return (attributes & att_uppercase); }
  bool is_lowercase()		{ return (attributes & att_lowercase); }
  bool is_capcase()		{ return (attributes & att_capcase); }
  bool is_nameref()		{ return (attributes & att_nameref); }

  bool is_invisible()		{ return (attributes & att_invisible); }
  bool is_non_unsettable()	{ return (attributes & att_nounset); }
  bool is_noassign()		{ return (attributes & att_noassign); }
  bool is_imported()		{ return (attributes & att_imported); }
  bool is_special()		{ return (attributes & att_special); }
  bool is_nofree()		{ return (attributes & att_nofree); }
  bool is_regen()		{ return (attributes & att_regenerate); }

  bool is_tempvar()		{ return (attributes & att_tempvar); }
  bool is_propagate()		{ return (attributes & att_propagate); }

  bool is_set()			{ return (value_.s != nullptr); }
  bool is_unset()		{ return (value_.s == nullptr); }
  bool is_null()		{ return value_.s && value_.s[0] == '\0'; }

private:
  char *name_;				// Symbol that the user types.
  Value value_;				// Value that is returned.
  char *exportstr;			// String for the environment.
  sh_var_value_func_t dynamic_value;	// Function called to return a `dynamic'
					// value for a variable, like $SECONDS
					// or $RANDOM.
  sh_var_assign_func_t assign_func;	// Function called when this `special
					// variable' is assigned a value in
					// bind_variable.
  int attributes;			// export, readonly, array, invisible...
  int context;				// Which context this variable belongs to.
};

typedef std::vector<SHELL_VAR *> VARLIST;

constexpr int NAMEREF_MAX = 8;	/* only 8 levels of nameref indirection */

#if 0
/* Assigning variable values: lvalues */
#define var_setvalue(var, str)	((var)->value.s = (str))
#define var_setfunc(var, func)	((var)->value.f = (func))
#define var_setarray(var, arr)	((var)->value.a = (arr))
#define var_setassoc(var, arr)	((var)->value.h = (arr))
#define var_setref(var, str)	((var)->value.s = (str))

/* Make VAR be auto-exported. */
#define set_auto_export(var) \
  do { (var)->attributes |= att_exported; array_needs_making = 1; } while (0)

#define SETVARATTR(var, attr, undo) \
	((undo == 0) ? ((var)->attributes |= (attr)) \
		     : ((var)->attributes &= ~(attr)))

#define VSETATTR(var, attr)	((var)->attributes |= (attr))
#define VUNSETATTR(var, attr)	((var)->attributes &= ~(attr))

#define VGETFLAGS(var)		((var)->attributes)

#define VSETFLAGS(var, flags)	((var)->attributes = (flags))
#define VCLRFLAGS(var)		((var)->attributes = 0)

/* Macros to perform various operations on `exportstr' member of a SHELL_VAR. */
#define CLEAR_EXPORTSTR(var)	(var)->exportstr = (char *)NULL
#define COPY_EXPORTSTR(var)	((var)->exportstr) ? savestring ((var)->exportstr) : (char *)NULL
#define SET_EXPORTSTR(var, value)  (var)->exportstr = (value)
#define SAVE_EXPORTSTR(var, value) (var)->exportstr = (value) ? savestring (value) : (char *)NULL

#define FREE_EXPORTSTR(var) \
	do { if ((var)->exportstr) free ((var)->exportstr); } while (0)

#define CACHE_IMPORTSTR(var, value) \
	(var)->exportstr = savestring (value)

#define INVALIDATE_EXPORTSTR(var) \
	do { \
	  if ((var)->exportstr) \
	    { \
	      free ((var)->exportstr); \
	      (var)->exportstr = (char *)NULL; \
	    } \
	} while (0)
#endif

#define ifsname(s)	((s)[0] == 'I' && (s)[1] == 'F' && (s)[2] == 'S' && (s)[3] == '\0')

/* Flag values for make_local_variable and its array counterparts */
enum mkloc_var_flags {
  MKLOC_ASSOCOK =		0x01,
  MKLOC_ARRAYOK =		0x02,
  MKLOC_INHERIT =		0x04
};

}  // namespace bash

#endif /* !_VARIABLES_H_ */
