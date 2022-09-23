// This file is setattr.def, from which is created setattr.c.
// It implements the builtins "export" and "readonly", in Bash.

// Copyright (C) 1987-2020 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

// $PRODUCES setattr.c

#include "config.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "bashgetopt.hh"
#include "common.hh"
#include "flags.hh"
#include "shell.hh"

#define READONLY_OR_EXPORT                                                    \
  (this_shell_builtin == readonly_builtin                                     \
   || this_shell_builtin == export_builtin)

// $BUILTIN export
// $FUNCTION export_builtin
// $SHORT_DOC export [-fn] [name[=value] ...] or export -p
// Set export attribute for shell variables.

// Marks each NAME for automatic export to the environment of subsequently
// executed commands.  If VALUE is supplied, assign VALUE before exporting.

// Options:
//   -f	refer to shell functions
//   -n	remove the export property from each NAME
//   -p	display a list of all exported variables and functions

// An argument of `--' disables further option processing.

// Exit Status:
// Returns success unless an invalid option is given or NAME is invalid.
// $END

namespace bash
{

/* For each variable name in LIST, make that variable appear in the
   environment passed to simple commands.  If there is no LIST, then
   print all such variables.  An argument of `-n' says to remove the
   exported attribute from variables named in LIST.  An argument of
  -f indicates that the names present in LIST refer to functions. */
int
Shell::export_builtin (WORD_LIST *list)
{
  return set_or_show_attributes (list, att_exported, 0);
}

// $BUILTIN readonly
// $FUNCTION readonly_builtin
// $SHORT_DOC readonly [-aAf] [name[=value] ...] or readonly -p
// Mark shell variables as unchangeable.

// Mark each NAME as read-only; the values of these NAMEs may not be
// changed by subsequent assignment.  If VALUE is supplied, assign VALUE
// before marking as read-only.

// Options:
//   -a	refer to indexed array variables
//   -A	refer to associative array variables
//   -f	refer to shell functions
//   -p	display a list of all readonly variables or functions,
// 		depending on whether or not the -f option is given

// An argument of `--' disables further option processing.

// Exit Status:
// Returns success unless an invalid option is given or NAME is invalid.
// $END

/* For each variable name in LIST, make that variable readonly.  Given an
   empty LIST, print out all existing readonly variables. */
int
Shell::readonly_builtin (WORD_LIST *list)
{
  return set_or_show_attributes (list, att_readonly, 0);
}

#if defined(ARRAY_VARS)
#define ATTROPTS "aAfnp"
#else
#define ATTROPTS "fnp"
#endif

/* For each variable name in LIST, make that variable have the specified
   ATTRIBUTE.  An arg of `-n' says to remove the attribute from the the
   remaining names in LIST (doesn't work for readonly). */
int
Shell::set_or_show_attributes (WORD_LIST *list, int attribute, int nodefs)
{
  int assign, undo, any_failed, assign_error, opt;
  int functions_only, arrays_only, assoc_only;
  int aflags;
  char *name;
#if defined(ARRAY_VARS)
  WORD_LIST *nlist, *tlist;
  WORD_DESC *w;
  char optw[8];
  int opti;
#endif

  functions_only = arrays_only = assoc_only = 0;
  undo = any_failed = assign_error = 0;
  /* Read arguments from the front of the list. */
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, ATTROPTS)) != -1)
    {
      switch (opt)
        {
        case 'n':
          undo = 1;
          break;
        case 'f':
          functions_only = 1;
          break;
#if defined(ARRAY_VARS)
        case 'a':
          arrays_only = 1;
          break;
        case 'A':
          assoc_only = 1;
          break;
#endif
        case 'p':
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  if (list)
    {
      if (attribute & att_exported)
        array_needs_making = 1;

      /* Cannot undo readonly status, silently disallowed. */
      if (undo && (attribute & att_readonly))
        attribute &= ~att_readonly;

      while (list)
        {
          name = list->word->word;

          if (functions_only) /* xxx -f name */
            {
              SHELL_VAR *var = find_function (name);
              if (var == 0)
                {
                  builtin_error (_ ("%s: not a function"), name);
                  any_failed++;
                }
              else if ((attribute & att_exported) && undo == 0
                       && exportable_function_name (name) == 0)
                {
                  builtin_error (_ ("%s: cannot export"), name);
                  any_failed++;
                }
              else
                SETVARATTR (var, attribute, undo);

              list = (WORD_LIST *)list->next;
              continue;
            }

          /* xxx [-np] name[=value] */
          assign = assignment (name, 0);

          aflags = 0;
          if (assign)
            {
              name[assign] = '\0';
              if (name[assign - 1] == '+')
                {
                  aflags |= ASS_APPEND;
                  name[assign - 1] = '\0';
                }
            }

          if (legal_identifier (name) == 0)
            {
              sh_invalidid (name);
              if (assign)
                assign_error++;
              else
                any_failed++;
              list = (WORD_LIST *)list->next;
              continue;
            }

          if (assign) /* xxx [-np] name=value */
            {
              name[assign] = '=';
              if (aflags & ASS_APPEND)
                name[assign - 1] = '+';
#if defined(ARRAY_VARS)
              /* Let's try something here.  Turn readonly -a xxx=yyy into
                 declare -ra xxx=yyy and see what that gets us. */
              if (arrays_only || assoc_only)
                {
                  tlist = (WORD_LIST *)list->next;
                  list->next = (WORD_LIST *)NULL;
                  /* Add -g to avoid readonly/export creating local variables:
                     only local/declare/typeset create local variables */
                  opti = 0;
                  optw[opti++] = '-';
                  optw[opti++] = 'g';
                  if (attribute & att_readonly)
                    optw[opti++] = 'r';
                  if (attribute & att_exported)
                    optw[opti++] = 'x';
                  if (arrays_only)
                    optw[opti++] = 'a';
                  else
                    optw[opti++] = 'A';
                  optw[opti] = '\0';

                  w = make_word (optw);
                  nlist = make_word_list (w, list);

                  opt = declare_builtin (nlist);
                  if (opt != EXECUTION_SUCCESS)
                    assign_error++;
                  list->next = tlist;
                  dispose_word (w);
                  free (nlist);
                }
              else
#endif
                /* This word has already been expanded once with command
                   and parameter expansion.  Call do_assignment_no_expand (),
                   which does not do command or parameter substitution.  If
                   the assignment is not performed correctly, flag an error. */
                if (do_assignment_no_expand (name) == 0)
                  assign_error++;
              name[assign] = '\0';
              if (aflags & ASS_APPEND)
                name[assign - 1] = '\0';
            }

          set_var_attribute (name, attribute, undo);
          if (assign) /* restore word */
            {
              name[assign] = '=';
              if (aflags & ASS_APPEND)
                name[assign - 1] = '+';
            }
          list = (WORD_LIST *)list->next;
        }
    }
  else
    {
      SHELL_VAR **variable_list;
      int i;

      if ((attribute & att_function) || functions_only)
        {
          variable_list = all_shell_functions ();
          if (attribute != att_function)
            attribute &= ~att_function; /* so declare -xf works, for example */
        }
      else
        variable_list = all_shell_variables ();

#if defined(ARRAY_VARS)
      if (attribute & att_array)
        {
          arrays_only++;
          if (attribute != att_array)
            attribute &= ~att_array;
        }
      else if (attribute & att_assoc)
        {
          assoc_only++;
          if (attribute != att_assoc)
            attribute &= ~att_assoc;
        }
#endif

      if (variable_list)
        {
          SHELL_VAR *var;
          for (i = 0; (var = variable_list[i]); i++)
            {
#if defined(ARRAY_VARS)
              if (arrays_only && array_p (var) == 0)
                continue;
              else if (assoc_only && assoc_p (var) == 0)
                continue;
#endif

              /* If we imported a variable that's not a valid identifier, don't
                 show it in any lists. */
              if ((var->attributes & (att_invisible | att_imported))
                  == (att_invisible | att_imported))
                continue;

              if ((var->attributes & attribute))
                {
                  show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
                  if ((any_failed = sh_chkwrite (any_failed)))
                    break;
                }
            }
          free (variable_list);
        }
    }

  return (assign_error
              ? EX_BADASSIGN
              : ((any_failed == 0) ? EXECUTION_SUCCESS : EXECUTION_FAILURE));
}

/* Show all variable variables (v == 1) or functions (v == 0) with
   attributes. */
int
show_all_var_attributes (int v, int nodefs)
{
  SHELL_VAR **variable_list, *var;
  int any_failed;
  int i;

  variable_list = v ? all_shell_variables () : all_shell_functions ();
  if (variable_list == 0)
    return EXECUTION_SUCCESS;

  for (i = any_failed = 0; (var = variable_list[i]); i++)
    {
      show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
      if ((any_failed = sh_chkwrite (any_failed)))
        break;
    }
  free (variable_list);
  return any_failed == 0 ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
}

/* Show all local variable variables with their attributes. This shows unset
   local variables (all_local_variables called with 0 argment). */
int
show_local_var_attributes (int v, int nodefs)
{
  SHELL_VAR **variable_list, *var;
  int any_failed;
  int i;

  variable_list = all_local_variables (0);
  if (variable_list == 0)
    return EXECUTION_SUCCESS;

  for (i = any_failed = 0; (var = variable_list[i]); i++)
    {
      show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
      if ((any_failed = sh_chkwrite (any_failed)))
        break;
    }
  free (variable_list);
  return any_failed == 0 ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
}

int
var_attribute_string (SHELL_VAR *var, int pattr,
                      char *flags) /* filled in with attributes */
{
  int i;

  i = 0;

  /* pattr == 0 means we are called from `declare'. */
  if (pattr == 0 || posixly_correct == 0)
    {
#if defined(ARRAY_VARS)
      if (array_p (var))
        flags[i++] = 'a';

      if (assoc_p (var))
        flags[i++] = 'A';
#endif

      if (function_p (var))
        flags[i++] = 'f';

      if (integer_p (var))
        flags[i++] = 'i';

      if (nameref_p (var))
        flags[i++] = 'n';

      if (readonly_p (var))
        flags[i++] = 'r';

      if (trace_p (var))
        flags[i++] = 't';

      if (exported_p (var))
        flags[i++] = 'x';

      if (capcase_p (var))
        flags[i++] = 'c';

      if (lowercase_p (var))
        flags[i++] = 'l';

      if (uppercase_p (var))
        flags[i++] = 'u';
    }
  else
    {
#if defined(ARRAY_VARS)
      if (array_p (var))
        flags[i++] = 'a';

      if (assoc_p (var))
        flags[i++] = 'A';
#endif

      if (function_p (var))
        flags[i++] = 'f';
    }

  flags[i] = '\0';
  return i;
}

/* Show the attributes for shell variable VAR.  If NODEFS is non-zero,
   don't show function definitions along with the name.  If PATTR is
   non-zero, it indicates we're being called from `export' or `readonly'.
   In POSIX mode, this prints the name of the calling builtin (`export'
   or `readonly') instead of `declare', and doesn't print function defs
   when called by `export' or `readonly'. */
int
show_var_attributes (SHELL_VAR *var, int pattr, int nodefs)
{
  char flags[MAX_ATTRIBUTES], *x;
  int i;

  i = var_attribute_string (var, pattr, flags);

  /* If we're printing functions with definitions, print the function def
     first, then the attributes, instead of printing output that can't be
     reused as input to recreate the current state. */
  if (function_p (var) && nodefs == 0 && (pattr == 0 || posixly_correct == 0))
    {
      printf ("%s\n", named_function_string (var->name, function_cell (var),
                                             FUNC_MULTILINE | FUNC_EXTERNAL));
      nodefs++;
      if (pattr == 0 && i == 1 && flags[0] == 'f')
        return 0; /* don't print `declare -f name' */
    }

  if (pattr == 0 || posixly_correct == 0)
    printf ("declare -%s ", i ? flags : "-");
  else if (i)
    printf ("%s -%s ", this_command_name, flags);
  else
    printf ("%s ", this_command_name);

#if defined(ARRAY_VARS)
  if (invisible_p (var) && (array_p (var) || assoc_p (var)))
    printf ("%s\n", var->name);
  else if (array_p (var))
    print_array_assignment (var, 0);
  else if (assoc_p (var))
    print_assoc_assignment (var, 0);
  else
#endif
    /* force `readonly' and `export' to not print out function definitions
       when in POSIX mode. */
    if (nodefs || (function_p (var) && pattr != 0 && posixly_correct))
      printf ("%s\n", var->name);
    else if (function_p (var))
      printf ("%s\n", named_function_string (var->name, function_cell (var),
                                             FUNC_MULTILINE | FUNC_EXTERNAL));
    else if (invisible_p (var) || var_isset (var) == 0)
      printf ("%s\n", var->name);
    else
      {
        x = sh_double_quote (value_cell (var));
        printf ("%s=%s\n", var->name, x);
        free (x);
      }
  return 0;
}

int
show_name_attributes (const char *name, int nodefs)
{
  SHELL_VAR *var;

  var = find_variable_noref (name);

  if (var) /* show every variable with attributes, even unset ones */
    {
      show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
      return 0;
    }
  else
    return 1;
}

int
show_localname_attributes (const char *name, int nodefs)
{
  SHELL_VAR *var;

  var = find_variable_noref (name);

  if (var && local_p (var)
      && var->context == variable_context) /* show every variable with
                                              attributes, even unset ones */
    {
      show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
      return 0;
    }
  else
    return 1;
}

int
show_func_attributes (const char *name, int nodefs)
{
  SHELL_VAR *var;

  var = find_function (name);

  if (var)
    {
      show_var_attributes (var, READONLY_OR_EXPORT, nodefs);
      return 0;
    }
  else
    return 1;
}

void
set_var_attribute (const char *name, int attribute, int undo)
{
  SHELL_VAR *var, *tv, *v, *refvar;
  char *tvalue;

  if (undo)
    var = find_variable (name);
  else
    {
      tv = find_tempenv_variable (name);
      /* XXX -- need to handle case where tv is a temp variable in a
         function-scope context, since function_env has been merged into
         the local variables table. */
      if (tv && tempvar_p (tv))
        {
          tvalue = var_isset (tv) ? savestring (value_cell (tv))
                                  : savestring ("");

          var = bind_variable (tv->name, tvalue, 0);
          if (var == 0)
            {
              free (tvalue);
              return; /* XXX - no error message here */
            }
          var->attributes |= tv->attributes & ~att_tempvar;
          /* This avoids an error message when propagating a read-only var
             later on. */
          if (posixly_correct || shell_compatibility_level <= 44)
            {
              if (var->context == 0 && (attribute & att_readonly))
                {
                  /* Don't bother to set the `propagate to the global variables
                     table' flag if we've just bound the variable in that
                     table */
                  v = find_global_variable (tv->name);
                  if (v != var)
                    VSETATTR (tv, att_propagate);
                }
              else
                VSETATTR (tv, att_propagate);
              if (var->context != 0)
                VSETATTR (var, att_propagate);
            }

          SETVARATTR (tv, attribute, undo); /* XXX */

          stupidly_hack_special_variables (tv->name);

          free (tvalue);
        }
      else
        {
          var = find_variable_notempenv (name);
          if (var == 0)
            {
              /* We might have a nameref pointing to something that we can't
                 resolve to a shell variable.  If we do, skip it.  We do a
                 little checking just so we can print an error message. */
              refvar = find_variable_nameref_for_create (name, 0);
              if (refvar == INVALID_NAMEREF_VALUE)
                return;
              /* Otherwise we probably have a nameref pointing to a variable
                 that hasn't been created yet. bind_variable will take care
                 of that. */
            }
          if (var == 0)
            {
              var = bind_variable (name, (char *)NULL, 0);
              if (var)
                VSETATTR (var, att_invisible);
            }
          else if (var->context != 0)
            VSETATTR (var, att_propagate);
        }
    }

  if (var)
    SETVARATTR (var, attribute, undo);

  if (var && (exported_p (var) || (attribute & att_exported)))
    array_needs_making = true; /* XXX */
}

} // namespace bash
