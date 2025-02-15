// This file is set_def.cc.
// It implements the "set" and "unset" builtins in Bash.

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

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "common.hh"
#include "flags.hh"
#include "parser.hh"
#include "shell.hh"

#if defined(READLINE)
#include "input.hh"
#include "readline.hh"
#endif

// $BUILTIN set
// $FUNCTION set_builtin
// $SHORT_DOC set [-abefhkmnptuvxBCHP] [-o option-name] [--] [arg ...]
// Set or unset values of shell options and positional parameters.

// Change the value of shell attributes and positional parameters, or
// display the names and values of shell variables.

// Options:
//   -a  Mark variables which are modified or created for export.
//   -b  Notify of job termination immediately.
//   -e  Exit immediately if a command exits with a non-zero status.
//   -f  Disable file name generation (globbing).
//   -h  Remember the location of commands as they are looked up.
//   -k  All assignment arguments are placed in the environment for a
//       command, not just those that precede the command name.
//   -m  Job control is enabled.
//   -n  Read commands but do not execute them.
//   -o option-name
//       Set the variable corresponding to option-name:
//           allexport    same as -a
//           braceexpand  same as -B
#if defined(READLINE)
//           emacs        use an emacs-style line editing interface
#endif /* READLINE */
//           errexit      same as -e
//           errtrace     same as -E
//           functrace    same as -T
//           hashall      same as -h
#if defined(BANG_HISTORY)
//           histexpand   same as -H
#endif /* BANG_HISTORY */
#if defined(HISTORY)
//           history      enable command history
#endif
//           ignoreeof    the shell will not exit upon reading EOF
//           interactive-comments
//                        allow comments to appear in interactive commands
//           keyword      same as -k
#if defined(JOB_CONTROL)
//           monitor      same as -m
#endif
//           noclobber    same as -C
//           noexec       same as -n
//           noglob       same as -f
//           nolog        currently accepted but ignored
#if defined(JOB_CONTROL)
//           notify       same as -b
#endif
//           nounset      same as -u
//           onecmd       same as -t
//           physical     same as -P
//           pipefail     the return value of a pipeline is the status of
//                        the last command to exit with a non-zero status,
//                        or zero if no command exited with a non-zero status
//           posix        change the behavior of bash where the default
//                        operation differs from the Posix standard to
//                        match the standard
//           privileged   same as -p
//           verbose      same as -v
#if defined(READLINE)
//           vi           use a vi-style line editing interface
#endif /* READLINE */
//           xtrace       same as -x
//   -p  Turned on whenever the real and effective user ids do not match.
//       Disables processing of the $ENV file and importing of shell
//       functions.  Turning this option off causes the effective uid and
//       gid to be set to the real uid and gid.
//   -t  Exit after reading and executing one command.
//   -u  Treat unset variables as an error when substituting.
//   -v  Print shell input lines as they are read.
//   -x  Print commands and their arguments as they are executed.
#if defined(BRACE_EXPANSION)
//   -B  the shell will perform brace expansion
#endif /* BRACE_EXPANSION */
//   -C  If set, disallow existing regular files to be overwritten
//       by redirection of output.
//   -E  If set, the ERR trap is inherited by shell functions.
#if defined(BANG_HISTORY)
//   -H  Enable ! style history substitution.  This flag is on
//       by default when the shell is interactive.
#endif /* BANG_HISTORY */
//   -P  If set, do not resolve symbolic links when executing commands
//       such as cd which change the current directory.
//   -T  If set, the DEBUG and RETURN traps are inherited by shell functions.
//   --  Assign any remaining arguments to the positional parameters.
//       If there are no remaining arguments, the positional parameters
//       are unset.
//   -   Assign any remaining arguments to the positional parameters.
//       The -x and -v options are turned off.

// Using + rather than - causes these flags to be turned off.  The
// flags can also be used upon invocation of the shell.  The current
// set of flags may be found in $-.  The remaining n ARGs are positional
// parameters and are assigned, in order, to $1, $2, .. $n.  If no
// ARGs are given, all shell variables are printed.

// Exit Status:
// Returns success unless an invalid option is given.
// $END

namespace bash
{

/* A struct used to match long options for set -o to the corresponding
   option letter or internal variable.  The functions can be called to
   dynamically generate values.  If you add a new variable name here
   that doesn't have a corresponding single-character option letter, make
   sure to set the value appropriately in reset_shell_options. */
const struct
{
  const char *name;
  int letter;
  char *variable;
  Shell::setopt_set_func_t set_func;
  Shell::setopt_get_func_t get_func;
} o_options[] = {
  { "allexport", 'a', nullptr, nullptr, nullptr },
#if defined(BRACE_EXPANSION)
  { "braceexpand", 'B', nullptr, nullptr, nullptr },
#endif
#if defined(READLINE)
  { "emacs", '\0', nullptr, &Shell::set_edit_mode, &Shell::get_edit_mode },
#endif
  { "errexit", 'e', nullptr, nullptr, nullptr },
  { "errtrace", 'E', nullptr, nullptr, nullptr },
  { "functrace", 'T', nullptr, nullptr, nullptr },
  { "hashall", 'h', nullptr, nullptr, nullptr },
#if defined(BANG_HISTORY)
  { "histexpand", 'H', nullptr, nullptr, nullptr },
#endif /* BANG_HISTORY */
#if defined(HISTORY)
  { "history", '\0', &enable_history_list, &Shell::bash_set_history, nullptr },
#endif
  { "ignoreeof", '\0', &ignoreeof, &Shell::set_ignoreeof, nullptr },
  { "interactive-comments", '\0', &interactive_comments, nullptr, nullptr },
  { "keyword", 'k', nullptr, nullptr, nullptr },
#if defined(JOB_CONTROL)
  { "monitor", 'm', nullptr, nullptr, nullptr },
#endif
  { "noclobber", 'C', nullptr, nullptr, nullptr },
  { "noexec", 'n', nullptr, nullptr, nullptr },
  { "noglob", 'f', nullptr, nullptr, nullptr },
#if defined(HISTORY)
  { "nolog", '\0', &dont_save_function_defs, nullptr, nullptr },
#endif
#if defined(JOB_CONTROL)
  { "notify", 'b', nullptr, nullptr, nullptr },
#endif /* JOB_CONTROL */
  { "nounset", 'u', nullptr, nullptr, nullptr },
  { "onecmd", 't', nullptr, nullptr, nullptr },
  { "physical", 'P', nullptr, nullptr, nullptr },
  { "pipefail", '\0', &pipefail_opt, nullptr, nullptr },
  { "posix", '\0', &posixly_correct, &Shell::set_posix_mode, nullptr },
  { "privileged", 'p', nullptr, nullptr, nullptr },
  { "verbose", 'v', nullptr, nullptr, nullptr },
#if defined(READLINE)
  { "vi", '\0', nullptr, &Shell::set_edit_mode, &Shell::get_edit_mode },
#endif
  { "xtrace", 'x', nullptr, nullptr, nullptr },
  { nullptr, 0, nullptr, nullptr, nullptr },
};

#define N_O_OPTIONS (sizeof (o_options) / sizeof (o_options[0]))

#define GET_BINARY_O_OPTION_VALUE(i, name)                                    \
  ((o_options[i].get_func) ? (*o_options[i].get_func) (name)                  \
                           : (*o_options[i].variable))

#define SET_BINARY_O_OPTION_VALUE(i, onoff, name)                             \
  ((o_options[i].set_func) ? (*o_options[i].set_func) (onoff, name)           \
                           : (*o_options[i].variable = (onoff == FLAG_ON)))

static int
find_minus_o_option (const char *name)
{
  int i;

  for (i = 0; o_options[i].name; i++)
    if (STREQ (name, o_options[i].name))
      return i;
  return -1;
}

char
minus_o_option_value (const char *name)
{
  int i;

  i = find_minus_o_option (name);
  if (i < 0)
    return -1;

  if (o_options[i].letter)
    {
      char *on_or_off = find_flag (o_options[i].letter);
      return (on_or_off == FLAG_UNKNOWN) ? -1 : *on_or_off;
    }
  else
    return GET_BINARY_O_OPTION_VALUE (i, name);
}

#define MINUS_O_FORMAT "%-15s\t%s\n"

static void
print_minus_o_option (const char *name, int value, int pflag)
{
  if (pflag == 0)
    printf (MINUS_O_FORMAT, name, value ? on : off);
  else
    printf ("set %co %s\n", value ? '-' : '+', name);
}

void
list_minus_o_opts (int mode, int reusable)
{
  for (int i = 0; o_options[i].name; i++)
    {
      if (o_options[i].letter)
        {
          char value = 0;
          char *on_or_off = find_flag (o_options[i].letter);
          if (on_or_off == FLAG_UNKNOWN)
            on_or_off = &value;
          if (mode == -1 || mode == *on_or_off)
            print_minus_o_option (o_options[i].name, *on_or_off, reusable);
        }
      else
        {
          char value = GET_BINARY_O_OPTION_VALUE (i, o_options[i].name);
          if (mode == -1 || mode == value)
            print_minus_o_option (o_options[i].name, value, reusable);
        }
    }
}

const char **
get_minus_o_opts ()
{
  const char **ret;
  int i;

  ret = (const char **)strvec_create (N_O_OPTIONS + 1);
  for (i = 0; o_options[i].name; i++)
    ret[i] = o_options[i].name;
  ret[i] = nullptr;
  return ret;
}

char *
get_current_options ()
{
  char *temp;
  int i, posixopts;

  posixopts = num_posix_options (); /* shopts modified by posix mode */
  /* Make the buffer big enough to hold the set -o options and the shopt
     options modified by posix mode. */
  temp = (char *)xmalloc (1 + N_O_OPTIONS + posixopts);
  for (i = 0; o_options[i].name; i++)
    {
      if (o_options[i].letter)
        temp[i] = *(find_flag (o_options[i].letter));
      else
        temp[i] = GET_BINARY_O_OPTION_VALUE (i, o_options[i].name);
    }

  /* Add the shell options that are modified by posix mode to the end of the
     bitmap. They will be handled in set_current_options() */
  get_posix_options (temp + i);
  temp[i + posixopts] = '\0';
  return temp;
}

void
set_current_options (const char *bitmap)
{
  if (bitmap == 0)
    return;

  int i;
  for (i = 0; o_options[i].name; i++)
    {
      char v = bitmap[i] ? FLAG_ON : FLAG_OFF;
      if (o_options[i].letter)
        {
          /* We should not get FLAG_UNKNOWN here */
          char *on_or_off = find_flag (o_options[i].letter);
          char cv = *on_or_off ? FLAG_ON : FLAG_OFF;
          if (v != cv)
            change_flag (o_options[i].letter, v);
        }
      else
        {
          char cv = GET_BINARY_O_OPTION_VALUE (i, o_options[i].name);
          cv = cv ? FLAG_ON : FLAG_OFF;
          if (v != cv)
            SET_BINARY_O_OPTION_VALUE (i, v, o_options[i].name);
        }
    }

  /* Now reset the variables changed by posix mode */
  set_posix_options (bitmap + i);
}

static char
set_ignoreeof (char on_or_off, const char *option_name)
{
  ignoreeof = on_or_off == FLAG_ON;
  unbind_variable_noref ("ignoreeof");
  if (ignoreeof)
    bind_variable ("IGNOREEOF", "10", 0);
  else
    unbind_variable_noref ("IGNOREEOF");
  sv_ignoreeof ("IGNOREEOF");
  return 0;
}

static char
set_posix_mode (char on_or_off, const char *option_name)
{
  /* short-circuit on no-op */
  if ((on_or_off == FLAG_ON && posixly_correct)
      || (on_or_off == FLAG_OFF && posixly_correct == 0))
    return 0;

  posixly_correct = (on_or_off == FLAG_ON);
  if (!posixly_correct)
    unbind_variable_noref ("POSIXLY_CORRECT");
  else
    bind_variable ("POSIXLY_CORRECT", "y", 0);
  sv_strict_posix ("POSIXLY_CORRECT");
  return 0;
}

#if defined(READLINE)
/* Magic.  This code `knows' how readline handles rl_editing_mode. */
static char
set_edit_mode (char on_or_off, const char *option_name)
{
  if (on_or_off == FLAG_ON)
    {
      rl_variable_bind ("editing-mode", option_name);

      if (interactive)
        with_input_from_stdin ();
      no_line_editing = false;
    }
  else
    {
      bool isemacs = rl_editing_mode == 1;
      if ((isemacs && *option_name == 'e')
          || (!isemacs && *option_name == 'v'))
        {
          if (interactive)
            with_input_from_stream (stdin, "stdin");
          no_line_editing = true;
        }
    }
  return !no_line_editing;
}

static char
get_edit_mode (const char *name)
{
  return (*name == 'e' ? no_line_editing == 0 && rl_editing_mode == 1
                       : no_line_editing == 0 && rl_editing_mode == 0);
}
#endif /* READLINE */

#if defined(HISTORY)
static char
bash_set_history (char on_or_off, const char *option_name)
{
  if (on_or_off == FLAG_ON)
    {
      enable_history_list = 1;
      bash_history_enable ();
      if (history_lines_this_session == 0)
        load_history ();
    }
  else
    {
      enable_history_list = 0;
      bash_history_disable ();
    }
  return 1 - enable_history_list;
}
#endif

int
set_minus_o_option (int on_or_off, const char *option_name)
{
  int i;

  i = find_minus_o_option (option_name);
  if (i < 0)
    {
      sh_invalidoptname (option_name);
      return EX_USAGE;
    }

  if (o_options[i].letter == 0)
    {
      previous_option_value = GET_BINARY_O_OPTION_VALUE (i, o_options[i].name);
      SET_BINARY_O_OPTION_VALUE (i, on_or_off, option_name);
      return EXECUTION_SUCCESS;
    }
  else
    {
      if ((previous_option_value
           = change_flag (o_options[i].letter, on_or_off))
          == FLAG_ERROR)
        {
          sh_invalidoptname (option_name);
          return EXECUTION_FAILURE;
        }
      else
        return EXECUTION_SUCCESS;
    }
}

static void
print_all_shell_variables ()
{
  SHELL_VAR **vars;

  vars = all_shell_variables ();
  if (vars)
    {
      print_var_list (vars);
      free (vars);
    }

  /* POSIX.2 does not allow function names and definitions to be output when
     `set' is invoked without options (PASC Interp #202). */
  if (posixly_correct == 0)
    {
      vars = all_shell_functions ();
      if (vars)
        {
          print_func_list (vars);
          free (vars);
        }
    }
}

void
set_shellopts ()
{
  char *value;
  char tflag[N_O_OPTIONS];
  int vsize, i, vptr, exported;
  SHELL_VAR *v;

  for (vsize = i = 0; o_options[i].name; i++)
    {
      tflag[i] = 0;
      if (o_options[i].letter)
        {
          char *ip = find_flag (o_options[i].letter);
          if (ip && *ip)
            {
              vsize += strlen (o_options[i].name) + 1;
              tflag[i] = 1;
            }
        }
      else if (GET_BINARY_O_OPTION_VALUE (i, o_options[i].name))
        {
          vsize += strlen (o_options[i].name) + 1;
          tflag[i] = 1;
        }
    }

  value = (char *)xmalloc (vsize + 1);

  for (i = vptr = 0; o_options[i].name; i++)
    {
      if (tflag[i])
        {
          strcpy (value + vptr, o_options[i].name);
          vptr += strlen (o_options[i].name);
          value[vptr++] = ':';
        }
    }

  if (vptr)
    vptr--; /* cut off trailing colon */
  value[vptr] = '\0';

  v = find_variable ("SHELLOPTS");

  /* Turn off the read-only attribute so we can bind the new value, and
     note whether or not the variable was exported. */
  if (v)
    {
      VUNSETATTR (v, att_readonly);
      exported = exported_p (v);
    }
  else
    exported = 0;

  v = bind_variable ("SHELLOPTS", value, 0);

  /* Turn the read-only attribute back on, and turn off the export attribute
     if it was set implicitly by mark_modified_vars and SHELLOPTS was not
     exported before we bound the new value. */
  VSETATTR (v, att_readonly);
  if (mark_modified_vars && exported == 0 && exported_p (v))
    VUNSETATTR (v, att_exported);

  free (value);
}

void
parse_shellopts (char *value)
{
  char *vname;
  int vptr;

  vptr = 0;
  while ((vname = extract_colon_unit (value, &vptr)))
    {
      set_minus_o_option (FLAG_ON, vname);
      free (vname);
    }
}

void
Shell::initialize_shell_options (bool no_shellopts)
{
  char *temp;
  SHELL_VAR *var;

  if (!no_shellopts)
    {
      var = find_variable ("SHELLOPTS");
      /* set up any shell options we may have inherited. */
      if (var && imported_p (var))
        {
          temp = (array_p (var) || assoc_p (var))
                     ? nullptr
                     : savestring (value_cell (var));
          if (temp)
            {
              parse_shellopts (temp);
              free (temp);
            }
        }
    }

  /* Set up the $SHELLOPTS variable. */
  set_shellopts ();
}

/* Reset the values of the -o options that are not also shell flags.  This is
   called from execute_cmd.c:initialize_subshell() when setting up a subshell
   to run an executable shell script without a leading `#!'. */
void
Shell::reset_shell_options ()
{
  pipefail_opt = 0;
  ignoreeof = 0;

#if defined(STRICT_POSIX)
  posixly_correct = true;
#else
  posixly_correct = false;
#endif
#if defined(HISTORY)
  dont_save_function_defs = 0;
  remember_on_history = enable_history_list = 1; /* XXX */
#endif
}

/* Set some flags from the word values in the input list.  If LIST is empty,
   then print out the values of the variables instead.  If LIST contains
   non-flags, then set $1 - $9 to the successive words of LIST. */
int
Shell::set_builtin (WORD_LIST *list)
{
  int on_or_off, flag_name, force_assignment, opts_changed, rv, r;
  char *arg;
  char s[3];

  if (list == 0)
    {
      print_all_shell_variables ();
      return sh_chkwrite (EXECUTION_SUCCESS);
    }

  /* Check validity of flag arguments. */
  rv = EXECUTION_SUCCESS;
  reset_internal_getopt ();
  while ((flag_name = internal_getopt (list, optflags)) != -1)
    {
      switch (flag_name)
        {
        case 'i': /* don't allow set -i */
          s[0] = list_opttype;
          s[1] = 'i';
          s[2] = '\0';
          sh_invalidopt (s);
          builtin_usage ();
          return EX_USAGE;
          CASE_HELPOPT;
        case '?':
          builtin_usage ();
          return list_optopt == '?' ? EXECUTION_SUCCESS : EX_USAGE;
        default:
          break;
        }
    }

  /* Do the set command.  While the list consists of words starting with
     '-' or '+' treat them as flags, otherwise, start assigning them to
     $1 ... $n. */
  for (force_assignment = opts_changed = 0; list;)
    {
      arg = list->word->word;

      /* If the argument is `--' or `-' then signal the end of the list
         and remember the remaining arguments. */
      if (arg[0] == '-' && (!arg[1] || (arg[1] == '-' && !arg[2])))
        {
          list = (WORD_LIST *)list->next;

          /* `set --' unsets the positional parameters. */
          if (arg[1] == '-')
            force_assignment = 1;

          /* Until told differently, the old shell behaviour of
             `set - [arg ...]' being equivalent to `set +xv [arg ...]'
             stands.  Posix.2 says the behaviour is marked as obsolescent. */
          else
            {
              change_flag ('x', '+');
              change_flag ('v', '+');
              opts_changed = 1;
            }

          break;
        }

      if ((on_or_off = *arg) && (on_or_off == '-' || on_or_off == '+'))
        {
          while ((flag_name = *++arg))
            {
              if (flag_name == '?')
                {
                  builtin_usage ();
                  return EXECUTION_SUCCESS;
                }
              else if (flag_name == 'o') /* -+o option-name */
                {
                  char *option_name;
                  WORD_LIST *opt;

                  opt = (WORD_LIST *)list->next;

                  if (opt == 0)
                    {
                      list_minus_o_opts (-1, (on_or_off == '+'));
                      rv = sh_chkwrite (rv);
                      continue;
                    }

                  option_name = opt->word->word;

                  if (option_name == 0 || *option_name == '\0'
                      || *option_name == '-' || *option_name == '+')
                    {
                      list_minus_o_opts (-1, (on_or_off == '+'));
                      continue;
                    }
                  list = (WORD_LIST *)list->next; /* Skip over option name. */

                  opts_changed = 1;
                  if ((r = set_minus_o_option (on_or_off, option_name))
                      != EXECUTION_SUCCESS)
                    {
                      set_shellopts ();
                      return r;
                    }
                }
              else if (change_flag (flag_name, on_or_off) == FLAG_ERROR)
                {
                  s[0] = on_or_off;
                  s[1] = flag_name;
                  s[2] = '\0';
                  sh_invalidopt (s);
                  builtin_usage ();
                  set_shellopts ();
                  return EXECUTION_FAILURE;
                }
              opts_changed = 1;
            }
        }
      else
        {
          break;
        }
      list = (WORD_LIST *)list->next;
    }

  /* Assigning $1 ... $n */
  if (list || force_assignment)
    remember_args (list, true);
  /* Set up new value of $SHELLOPTS */
  if (opts_changed)
    set_shellopts ();
  return rv;
}

} // namespace bash

// $BUILTIN unset
// $FUNCTION unset_builtin
// $SHORT_DOC unset [-f] [-v] [-n] [name ...]
// Unset values and attributes of shell variables and functions.

// For each NAME, remove the corresponding variable or function.

// Options:
//   -f    treat each NAME as a shell function
//   -v    treat each NAME as a shell variable
//   -n    treat each NAME as a name reference and unset the variable itself
//                 rather than the variable it references

// Without options, unset first tries to unset a variable, and if that fails,
// tries to unset a function.

// Some variables cannot be unset; also see `readonly'.

// Exit Status:
// Returns success unless an invalid option is given or a NAME is read-only.
// $END

#define NEXT_VARIABLE()                                                       \
  any_failed++;                                                               \
  list = (WORD_LIST *)list->next;                                             \
  continue;

namespace bash
{

int
Shell::unset_builtin (WORD_LIST *list)
{
  int unset_function, unset_variable, unset_array, opt, nameref, any_failed;
  int global_unset_func, global_unset_var, vflags, valid_id;
  char *name, *tname;

  unset_function = unset_variable = unset_array = nameref = any_failed = 0;
  global_unset_func = global_unset_var = 0;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "fnv")) != -1)
    {
      switch (opt)
        {
        case 'f':
          global_unset_func = 1;
          break;
        case 'v':
          global_unset_var = 1;
          break;
        case 'n':
          nameref = 1;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }

  list = loptend;

  if (global_unset_func && global_unset_var)
    {
      builtin_error (
          _ ("cannot simultaneously unset a function and a variable"));
      return EXECUTION_FAILURE;
    }
  else if (unset_function && nameref)
    nameref = 0;

#if defined(ARRAY_VARS)
  vflags = assoc_expand_once ? (VA_NOEXPAND | VA_ONEWORD) : 0;
#endif

  while (list)
    {
      SHELL_VAR *var;
      int tem;
#if defined(ARRAY_VARS)
      char *t;
#endif

      name = list->word->word;

      unset_function = global_unset_func;
      unset_variable = global_unset_var;

#if defined(ARRAY_VARS)
      unset_array = 0;
      /* XXX valid array reference second arg was 0 */
      if (!unset_function && nameref == 0
          && valid_array_reference (name, vflags))
        {
          t = strchr (name, '[');
          *t++ = '\0';
          unset_array++;
        }
#endif
      /* Get error checking out of the way first.  The low-level functions
         just perform the unset, relying on the caller to verify. */
      valid_id = legal_identifier (name);

      /* Whether or not we are in posix mode, if neither -f nor -v appears,
         skip over trying to unset variables with invalid names and just
         treat them as potential shell function names. */
      if (global_unset_func == 0 && global_unset_var == 0 && valid_id == 0)
        {
          unset_variable = unset_array = 0;
          unset_function = 1;
        }

      /* Bash allows functions with names which are not valid identifiers
         to be created when not in posix mode, so check only when in posix
         mode when unsetting a function. */
      if (unset_function == 0 && valid_id == 0)
        {
          sh_invalidid (name);
          NEXT_VARIABLE ();
        }

      /* Search for functions here if -f supplied or if NAME cannot be a
         variable name. */
      var = unset_function ? find_function (name)
                           : (nameref ? find_variable_last_nameref (name, 0)
                                      : find_variable (name));

      /* Some variables (but not functions yet) cannot be unset, period. */
      if (var && unset_function == 0 && non_unsettable_p (var))
        {
          builtin_error (_ ("%s: cannot unset"), name);
          NEXT_VARIABLE ();
        }

      /* if we have a nameref we want to use it */
      if (var && unset_function == 0 && nameref == 0
          && STREQ (name, name_cell (var)) == 0)
        name = name_cell (var);

      /* Posix.2 says try variables first, then functions.  If we would
         find a function after unsuccessfully searching for a variable,
         note that we're acting on a function now as if -f were
         supplied.  The readonly check below takes care of it. */
      if (var == 0 && nameref == 0 && unset_variable == 0
          && unset_function == 0)
        {
          if ((var = find_function (name)))
            unset_function = 1;
        }

      /* Posix.2 says that unsetting readonly variables is an error. */
      if (var && readonly_p (var))
        {
          builtin_error (_ ("%s: cannot unset: readonly %s"), var->name,
                         unset_function ? "function" : "variable");
          NEXT_VARIABLE ();
        }

        /* Unless the -f option is supplied, the name refers to a variable.
         */
#if defined(ARRAY_VARS)
      if (var && unset_array)
        {
          /* Let unbind_array_element decide what to do with non-array vars
           */
          tem = unbind_array_element (var, t, vflags); /* XXX new third arg */
          if (tem == -2 && array_p (var) == 0 && assoc_p (var) == 0)
            {
              builtin_error (_ ("%s: not an array variable"), var->name);
              NEXT_VARIABLE ();
            }
          else if (tem < 0)
            any_failed++;
        }
      else
#endif /* ARRAY_VARS */
        /* If we're trying to unset a nameref variable whose value isn't a
           set variable, make sure we still try to unset the nameref's value
         */
        if (var == 0 && nameref == 0 && unset_function == 0)
          {
            var = find_variable_last_nameref (name, 0);
            if (var && nameref_p (var))
              {
#if defined(ARRAY_VARS)
                if (valid_array_reference (nameref_cell (var), 0))
                  {
                    tname = savestring (nameref_cell (var));
                    if ((var = array_variable_part (tname, 0, &t, (int *)0)))
                      tem = unbind_array_element (
                          var, t, vflags); /* XXX new third arg */
                    free (tname);
                  }
                else
#endif
                  tem = unbind_variable (nameref_cell (var));
              }
            else
              tem = unbind_variable (name);
          }
        else
          tem = unset_function ? unbind_func (name)
                               : (nameref ? unbind_nameref (name)
                                          : unbind_variable (name));

      /* This is what Posix.2 says:  ``If neither -f nor -v
         is specified, the name refers to a variable; if a variable by
         that name does not exist, a function by that name, if any,
         shall be unset.'' */
      if (tem == -1 && nameref == 0 && unset_function == 0
          && unset_variable == 0)
        tem = unbind_func (name);

      name = list->word->word; /* reset above for namerefs */

      /* SUSv3, POSIX.1-2001 say:  ``Unsetting a variable or function that
         was not previously set shall not be considered an error.'' */

      if (unset_function == 0)
        stupidly_hack_special_variables (name);

      list = (WORD_LIST *)list->next;
    }

  return any_failed ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
}

} // namespace bash
