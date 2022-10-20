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

#if !defined(_VARIABLES_H_)
#define _VARIABLES_H_

#include "array.hh"
#include "assoc.hh"
#include "command.hh"
#include "shell.hh"

/* Shell variables and functions are stored in hash tables. */
#include "hashlib.hh"

#include "conftypes.hh"

namespace bash
{

/* Flags for VAR_CONTEXT->flags. */
enum vc_flags
{
  VC_NOFLAGS = 0,
  VC_HASLOCAL = 0x01,
  VC_HASTMPVAR = 0x02,
  VC_FUNCENV = 0x04, /* also function if name != NULL */
  VC_BLTNENV = 0x08, /* builtin_env */
  VC_TEMPENV = 0x10, /* temporary_env */
  VC_TEMPFLAGS = (VC_FUNCENV | VC_BLTNENV | VC_TEMPENV)
};

static inline vc_flags &
operator|= (vc_flags &a, const vc_flags &b)
{
  a = static_cast<vc_flags> (static_cast<uint32_t> (a)
                             | static_cast<uint32_t> (b));
  return a;
}

class SHELL_VAR;

/* A variable context. */
struct VAR_CONTEXT
{
  VAR_CONTEXT (string_view name_, vc_flags flags_, int scope_)
      : name (to_string (name_)), flags (flags_), scope (scope_)
  {
  }

  VAR_CONTEXT (vc_flags flags_, int scope_) : flags (flags_), scope (scope_) {}

  std::string name;              // empty string means global context
  VAR_CONTEXT *up;               // previous function calls
  VAR_CONTEXT *down;             // down towards global context
  HASH_TABLE<SHELL_VAR *> table; // variables at this scope
  vc_flags flags;
  int scope; // 0 means global context

  /* Inline access methods. */

  bool
  func_env ()
  {
    return (flags & VC_FUNCENV);
  }
  bool
  builtin_env ()
  {
    return (flags & VC_BLTNENV);
  }
  bool
  temp_env ()
  {
    return ((flags & VC_TEMPFLAGS) == VC_TEMPENV);
  }
  bool
  temp_scope ()
  {
    return (flags & (VC_TEMPENV | VC_BLTNENV));
  }
  bool
  has_locals ()
  {
    return (flags & VC_HASLOCAL);
  }
  bool
  has_temp_vars ()
  {
    return (flags & VC_HASTMPVAR);
  }
};

/* The various attributes that a given variable can have. */
/* First, the user-visible attributes */
enum var_att_flags
{
  att_noflags = 0,
  att_exported = 0x0000001,  /* export to environment */
  att_readonly = 0x0000002,  /* cannot change */
  att_array = 0x0000004,     /* value is an array */
  att_function = 0x0000008,  /* value is a function */
  att_integer = 0x0000010,   /* internal representation is int */
  att_local = 0x0000020,     /* variable is local to a function */
  att_assoc = 0x0000040,     /* variable is an associative array */
  att_trace = 0x0000080,     /* function is traced with DEBUG trap */
  att_uppercase = 0x0000100, /* word converted to uppercase on assignment */
  att_lowercase = 0x0000200, /* word converted to lowercase on assignment */
  att_capcase = 0x0000400,   /* word capitalized on assignment */
  att_nameref = 0x0000800,   /* word is a name reference */

  user_attrs
  = (att_exported | att_readonly | att_integer | att_local | att_trace
     | att_uppercase | att_lowercase | att_capcase | att_nameref),

  attmask_user = 0x0000fff,

  /* Internal attributes used for bookkeeping */
  att_invisible = 0x0001000,  /* cannot see */
  att_nounset = 0x0002000,    /* cannot unset */
  att_noassign = 0x0004000,   /* assignment not allowed */
  att_imported = 0x0008000,   /* came from environment */
  att_special = 0x0010000,    /* requires special handling */
  att_nofree = 0x0020000,     /* do not free value on unset */
  att_regenerate = 0x0040000, /* regenerate when exported */

  attmask_int = 0x00ff000,

  /* Internal attributes used for variable scoping. */
  att_tempvar = 0x0100000,   /* variable came from the temp environment */
  att_propagate = 0x0200000, /* propagate to previous scope */

  attmask_scope = 0x0f00000
};

static inline var_att_flags &
operator|= (var_att_flags &a, const var_att_flags &b)
{
  a = static_cast<var_att_flags> (static_cast<uint32_t> (a)
                                  | static_cast<uint32_t> (b));
  return a;
}

static inline var_att_flags
operator| (const var_att_flags &a, const var_att_flags &b)
{
  return static_cast<var_att_flags> (static_cast<uint32_t> (a)
                                     | static_cast<uint32_t> (b));
}

static inline var_att_flags &
operator&= (var_att_flags &a, const var_att_flags &b)
{
  a = static_cast<var_att_flags> (static_cast<uint32_t> (a)
                                  & static_cast<uint32_t> (b));
  return a;
}

static inline var_att_flags
operator& (const var_att_flags &a, const var_att_flags &b)
{
  return static_cast<var_att_flags> (static_cast<uint32_t> (a)
                                     & static_cast<uint32_t> (b));
}

static inline var_att_flags
operator~(const var_att_flags &a)
{
  return static_cast<var_att_flags> (~static_cast<uint32_t> (a));
}

/* Union of supported types for a shell variable. */
union Value
{
  char *s;                     /* string value */
  COMMAND *cmd;                /* function */
  ARRAY *a;                    /* array */
  HASH_TABLE<alias_t *> *ah;   /* hashed aliases */
  HASH_TABLE<PATH_DATA *> *ph; /* hashed paths */
  HASH_TABLE<SHELL_VAR *> *sh; /* associative array */
};

/* Forward declaration of shell class for callback functions. */
class Shell;

/* What a shell variable looks like. */
class SHELL_VAR
{
public:
  /* Create a new shell variable with name NAME. */
  SHELL_VAR (string_view name) : name_ (name) {}

  /* Create a new shell variable and add it to the specified hash table. */
  SHELL_VAR (string_view name, HASH_TABLE<SHELL_VAR *> &shash)
      : name_ (name)
  {
    value_.sh = &shash;
  }

  typedef SHELL_VAR *(Shell::*value_func_t) (SHELL_VAR *);

  typedef SHELL_VAR *(Shell::*assign_func_t) (SHELL_VAR *, string_view,
                                              arrayind_t, string_view);

  std::string &
  name ()
  {
    return name_;
  }

  void
  set_name (string_view new_name)
  {
    name_ = to_string (new_name);
  }

  char *
  str_value ()
  {
    return value_.s;
  }

  void
  set_str_value (char *s)
  {
    value_.s = s;
  }

  COMMAND *
  cmd_value ()
  {
    return value_.cmd;
  }

  void
  set_cmd_value (COMMAND *cmd)
  {
    value_.cmd = cmd;
  }

  ARRAY *
  array_value ()
  {
    return value_.a;
  }

  void
  set_array_value (ARRAY *a)
  {
    value_.a = a;
  }

  HASH_TABLE<alias_t *> *
  alias_hash_value ()
  {
    return value_.ah;
  }

  HASH_TABLE<PATH_DATA *> *
  phash_value ()
  {
    return value_.ph;
  }

  HASH_TABLE<SHELL_VAR *> *
  assoc_value ()
  {
    return value_.sh;
  }

  void
  set_alias_hash_value (HASH_TABLE<alias_t *> *h)
  {
    value_.ah = h;
  }

  void
  set_assoc_value (HASH_TABLE<SHELL_VAR *> *h)
  {
    value_.sh = h;
  }

  void
  set_phash_value (HASH_TABLE<PATH_DATA *> *h)
  {
    value_.ph = h;
  }

  char *
  name_value ()
  {
    return value_.s;
  } /* so it can change later */

  /* Set the string value of VAR to the string representation of VALUE.
     Right now this takes an INT64_T because that's what itos needs. If
     FORCE_ATTRIBUTE is set, force the integer attribute on. */
  void
  set_int_value (int64_t value, bool force_attribute)
  {
    std::string p (itos (value));
    delete[] str_value ();
    set_str_value (savestring (p));
    if (force_attribute)
      set_attr (att_integer);
  }

  void
  set_string_value (string_view value)
  {
    char *p = savestring (value);
    delete[] str_value ();
    set_str_value (p);
  }

  /* Inline access methods. */

  bool
  exported ()
  {
    return (attributes & att_exported);
  }
  bool
  readonly ()
  {
    return (attributes & att_readonly);
  }
  bool
  array ()
  {
    return (attributes & att_array);
  }
  bool
  function ()
  {
    return (attributes & att_function);
  }
  bool
  integer ()
  {
    return (attributes & att_integer);
  }
  bool
  local ()
  {
    return (attributes & att_local);
  }
  bool
  assoc ()
  {
    return (attributes & att_assoc);
  }
  bool
  trace ()
  {
    return (attributes & att_trace);
  }
  bool
  uppercase ()
  {
    return (attributes & att_uppercase);
  }
  bool
  lowercase ()
  {
    return (attributes & att_lowercase);
  }
  bool
  capcase ()
  {
    return (attributes & att_capcase);
  }
  bool
  nameref ()
  {
    return (attributes & att_nameref);
  }
  bool
  invisible ()
  {
    return (attributes & att_invisible);
  }
  bool
  non_unsettable ()
  {
    return (attributes & att_nounset);
  }
  bool
  noassign ()
  {
    return (attributes & att_noassign);
  }
  bool
  imported ()
  {
    return (attributes & att_imported);
  }
  bool
  special ()
  {
    return (attributes & att_special);
  }
  bool
  nofree ()
  {
    return (attributes & att_nofree);
  }
  bool
  regen ()
  {
    return (attributes & att_regenerate);
  }
  bool
  tempvar ()
  {
    return (attributes & att_tempvar);
  }
  bool
  propagate ()
  {
    return (attributes & att_propagate);
  }
  bool
  is_set ()
  {
    return (value_.s != nullptr);
  }
  bool
  is_unset ()
  {
    return (value_.s == nullptr);
  }
  bool
  is_null ()
  {
    return value_.s && value_.s[0] == '\0';
  }

  void
  set_attr (var_att_flags flags)
  {
    attributes |= flags;
  }

  void
  unset_attr (var_att_flags flags)
  {
    attributes &= ~flags;
  }

  std::string name_;          // Symbol that the user types.
  Value value_;               // Value that is returned.
  std::string exportstr;      // String for the environment.
  value_func_t dynamic_value; // Function called to return a `dynamic'
                              // value for a variable, like $SECONDS
                              // or $RANDOM.
  assign_func_t assign_func;  // Function called when this `special
                              // variable' is assigned a value in
                              // bind_variable.
  var_att_flags attributes;   // export, readonly, array, invisible...
  int context;                // Which context this variable belongs to.
};

typedef std::vector<SHELL_VAR *> VARLIST;

constexpr int NAMEREF_MAX = 8; /* only 8 levels of nameref indirection */

/* Make VAR be auto-exported. */
#define set_auto_export(var)                                                  \
  do                                                                          \
    {                                                                         \
      (var)->attributes |= att_exported;                                      \
      array_needs_making = true;                                              \
    }                                                                         \
  while (0)

#if 0
#define SETVARATTR(var, attr, undo)                                           \
  ((undo == 0) ? ((var)->attributes |= (attr))                                \
               : ((var)->attributes &= ~(attr)))

#define VSETATTR(var, attr) ((var)->attributes |= (attr))
#define VUNSETATTR(var, attr) ((var)->attributes &= ~(attr))

#define VGETFLAGS(var) ((var)->attributes)

#define VSETFLAGS(var, flags) ((var)->attributes = (flags))
#define VCLRFLAGS(var) ((var)->attributes = 0)

/* Macros to perform various operations on `exportstr' member of a SHELL_VAR. */
#define CLEAR_EXPORTSTR(var) (var)->exportstr = (char *)NULL
#define COPY_EXPORTSTR(var)                                                   \
  ((var)->exportstr) ? savestring ((var)->exportstr) : (char *)NULL
#define SET_EXPORTSTR(var, value) (var)->exportstr = (value)
#define SAVE_EXPORTSTR(var, value)                                            \
  (var)->exportstr = (value) ? savestring (value) : (char *)NULL

#define FREE_EXPORTSTR(var)                                                   \
  do                                                                          \
    {                                                                         \
      if ((var)->exportstr)                                                   \
        free ((var)->exportstr);                                              \
    }                                                                         \
  while (0)

#define CACHE_IMPORTSTR(var, value) (var)->exportstr = savestring (value)

#define INVALIDATE_EXPORTSTR(var)                                             \
  do                                                                          \
    {                                                                         \
      if ((var)->exportstr)                                                   \
        {                                                                     \
          free ((var)->exportstr);                                            \
          (var)->exportstr = (char *)NULL;                                    \
        }                                                                     \
    }                                                                         \
  while (0)
#endif

static inline bool
ifsname (string_view s)
{
  return s == "IFS";
}

static inline bool
ifsname (const char *s)
{
  return s[0] == 'I' && s[1] == 'F' && s[2] == 'S' && s[3] == '\0';
}

/* Flag values for make_local_variable and its array counterparts */
enum mkloc_var_flags
{
  MKLOC_ASSOCOK = 0x01,
  MKLOC_ARRAYOK = 0x02,
  MKLOC_INHERIT = 0x04
};

// Definitions previously in arrayfunc.hh.

/* values for `type' field */
enum av_type
{
  ARRAY_INVALID = -1,
  ARRAY_SCALAR = 0,
  ARRAY_INDEXED = 1,
  ARRAY_ASSOC = 2
};

struct array_eltstate_t
{
  av_type type; // assoc or indexed, says which fields are valid
  arrayind_t ind;
  std::string key; // can be allocated memory
  std::string value;
  short subtype; // `*', `@', or something else
};

/* Flags for array_value_internal and callers array_value/get_array_value */
enum av_flags
{
  AV_NOFLAGS = 0,
  AV_ALLOWALL = 0x001, // treat a[@] like $@ and a[*] like $*
  AV_QUOTED = 0x002,
  AV_USEIND = 0x004,
  AV_USEVAL = 0x008,    // XXX - should move this
  AV_ASSIGNRHS = 0x010, // no splitting, special case ${a[@]}
  AV_NOEXPAND = 0x020,  // don't run assoc subscripts through word expansion
  AV_ONEWORD = 0x040,   // not used yet
  AV_ATSTARKEYS
  = 0x080 // accept a[@] and a[*] but use them as keys, not special values
};

/* Flags for valid_array_reference. Value 1 is reserved for skipsubscript() */
enum valid_array_flags
{
  VA_NOFLAGS = 0,
  VA_NOEXPAND = 0x001,
  VA_ONEWORD = 0x002
};

static inline valid_array_flags
operator| (const valid_array_flags &a, const valid_array_flags &b)
{
  return static_cast<valid_array_flags> (static_cast<uint32_t> (a)
                                         | static_cast<uint32_t> (b));
}

// Definitions previously in execute_cmd.hh.

#if defined(ARRAY_VARS)
struct func_array_state
{
  ARRAY *funcname_a;
  SHELL_VAR *funcname_v;
  ARRAY *source_a;
  SHELL_VAR *source_v;
  ARRAY *lineno_a;
  SHELL_VAR *lineno_v;
};
#endif

/* Placeholder for later expansion to include more execution state */
/* XXX - watch out for pid_t */
struct execstate
{
  pid_t pid;
  int subshell_env;
};

} // namespace bash

#endif /* !_VARIABLES_H_ */
