// This file is shopt_def.cc.
// It implements the Bash `shopt' builtin.

// Copyright (C) 1994-2020 Free Software Foundation, Inc.

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

// $BUILTIN shopt
// $FUNCTION shopt_builtin
// $SHORT_DOC shopt [-pqsu] [-o] [optname ...]
// Set and unset shell options.

// Change the setting of each shell option OPTNAME.  Without any option
// arguments, list each supplied OPTNAME, or all shell options if no
// OPTNAMEs are given, with an indication of whether or not each is set.

// Options:
//   -o	restrict OPTNAMEs to those defined for use with `set -o'
//   -p	print each shell option with an indication of its status
//   -q	suppress output
//   -s	enable (set) each OPTNAME
//   -u	disable (unset) each OPTNAME

// Exit Status:
// Returns success if OPTNAME is enabled; fails if an invalid option is
// given or OPTNAME is disabled.
// $END

#include "config.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "version.hh"

#include "bashintl.hh"

#include "common.hh"
#include "flags.hh"
#include "shell.hh"

namespace bash
{

#define UNSETOPT 0
#define SETOPT 1

/* If you add a new variable name here, make sure to set the default value
   appropriately in reset_shopt_options. */

void
Shell::initialize_shopt_vars_table ()
{
  shopt_vars.push_back (shopt_var_t ("autocd", &autocd));
#if defined(ARRAY_VARS)
  shopt_vars.push_back (shopt_var_t ("assoc_expand_once", &assoc_expand_once));
#endif
  shopt_vars.push_back (shopt_var_t ("cdable_vars", &cdable_vars));
  shopt_vars.push_back (shopt_var_t ("cdspell", &cdspelling));
  shopt_vars.push_back (shopt_var_t ("checkhash", &check_hashed_filenames));
#if defined(JOB_CONTROL)
  shopt_vars.push_back (shopt_var_t ("checkjobs", &check_jobs_at_exit));
#endif
  shopt_vars.push_back (shopt_var_t ("checkwinsize", &check_window_size));
#if defined(HISTORY)
  shopt_vars.push_back (shopt_var_t ("cmdhist", &command_oriented_history));
#endif
  shopt_vars.push_back (shopt_var_t ("compat31", &shopt_compat31,
                                     &Shell::set_compatibility_level));
  shopt_vars.push_back (shopt_var_t ("compat32", &shopt_compat32,
                                     &Shell::set_compatibility_level));
  shopt_vars.push_back (shopt_var_t ("compat40", &shopt_compat40,
                                     &Shell::set_compatibility_level));
  shopt_vars.push_back (shopt_var_t ("compat41", &shopt_compat41,
                                     &Shell::set_compatibility_level));
  shopt_vars.push_back (shopt_var_t ("compat42", &shopt_compat42,
                                     &Shell::set_compatibility_level));
  shopt_vars.push_back (shopt_var_t ("compat43", &shopt_compat43,
                                     &Shell::set_compatibility_level));
  shopt_vars.push_back (shopt_var_t ("compat44", &shopt_compat44,
                                     &Shell::set_compatibility_level));
#if defined(READLINE)
  shopt_vars.push_back (
      shopt_var_t ("complete_fullquote", &complete_fullquote));
  shopt_vars.push_back (shopt_var_t ("direxpand", &dircomplete_expand,
                                     &Shell::shopt_set_complete_direxpand));
  shopt_vars.push_back (shopt_var_t ("dirspell", &dircomplete_spelling));
#endif
  shopt_vars.push_back (shopt_var_t ("dotglob", &glob_dot_filenames));
  shopt_vars.push_back (shopt_var_t ("execfail", &no_exit_on_failed_exec));
  shopt_vars.push_back (shopt_var_t ("expand_aliases", &expand_aliases));
#if defined(DEBUGGER)
  shopt_vars.push_back (
      shopt_var_t ("extdebug", &debugging_mode, &Shell::shopt_set_debug_mode));
#endif
#if defined(EXTENDED_GLOB)
  shopt_vars.push_back (shopt_var_t ("extglob", &extended_glob));
#endif
  shopt_vars.push_back (shopt_var_t ("extquote", &extended_quote));
  shopt_vars.push_back (shopt_var_t ("failglob", &fail_glob_expansion));
#if defined(READLINE)
  shopt_vars.push_back (shopt_var_t ("force_fignore", &force_fignore));
#endif
  shopt_vars.push_back (shopt_var_t ("globasciiranges", &glob_asciirange));
  shopt_vars.push_back (shopt_var_t ("globstar", &glob_star));
  shopt_vars.push_back (shopt_var_t ("gnu_errfmt", &gnu_error_format));
#if defined(HISTORY)
  shopt_vars.push_back (shopt_var_t ("histappend", &force_append_history));
#endif
#if defined(READLINE)
  shopt_vars.push_back (shopt_var_t ("histreedit", &history_reediting));
  shopt_vars.push_back (shopt_var_t ("histverify", &hist_verify));
  shopt_vars.push_back (
      shopt_var_t ("hostcomplete", &perform_hostname_completion,
                   &Shell::shopt_enable_hostname_completion));
#endif
  shopt_vars.push_back (shopt_var_t ("huponexit", &hup_on_exit));
  shopt_vars.push_back (shopt_var_t ("inherit_errexit", &inherit_errexit));
  shopt_vars.push_back (shopt_var_t ("interactive_comments",
                                     &interactive_comments,
                                     &Shell::set_shellopts_after_change));
  shopt_vars.push_back (shopt_var_t ("lastpipe", &lastpipe_opt));
#if defined(HISTORY)
  shopt_vars.push_back (shopt_var_t ("lithist", &literal_history));
#endif
  shopt_vars.push_back (shopt_var_t ("localvar_inherit", &localvar_inherit));
  shopt_vars.push_back (shopt_var_t ("localvar_unset", &localvar_unset));
  shopt_vars.push_back (shopt_var_t ("login_shell", &shopt_login_shell,
                                     &Shell::set_login_shell));
  shopt_vars.push_back (shopt_var_t ("mailwarn", &mail_warning));
#if defined(READLINE)
  shopt_vars.push_back (
      shopt_var_t ("no_empty_cmd_completion", &no_empty_command_completion));
#endif
  shopt_vars.push_back (shopt_var_t ("nocaseglob", &glob_ignore_case));
  shopt_vars.push_back (shopt_var_t ("nocasematch", &match_ignore_case));
  shopt_vars.push_back (shopt_var_t ("nullglob", &allow_null_glob_expansion));
#if defined(PROGRAMMABLE_COMPLETION)
  shopt_vars.push_back (shopt_var_t ("progcomp", &prog_completion_enabled));
#if defined(ALIAS)
  shopt_vars.push_back (shopt_var_t ("progcomp_alias", &progcomp_alias));
#endif
#endif
  shopt_vars.push_back (shopt_var_t ("promptvars", &promptvars));
#if defined(RESTRICTED_SHELL)
  shopt_vars.push_back (shopt_var_t ("restricted_shell", &restricted_shell,
                                     &Shell::set_restricted_shell));
#endif
  shopt_vars.push_back (shopt_var_t ("shift_verbose", &print_shift_error));
  shopt_vars.push_back (shopt_var_t ("sourcepath", &source_uses_path));
#if defined(SYSLOG_HISTORY) && defined(SYSLOG_SHOPT)
  shopt_vars.push_back (shopt_var_t ("syslog_history", &syslog_history));
#endif
  shopt_vars.push_back (shopt_var_t ("xpg_echo", &xpg_echo));

#if __cplusplus < 201103L
  shopt_vars.reserve (shopt_vars.size ());
#else
  shopt_vars.shrink_to_fit ();
#endif
}

// We only need the enum operators in this file.

static inline Shell::shopt_flags &
operator|= (Shell::shopt_flags &a, const Shell::shopt_flags &b)
{
  a = static_cast<Shell::shopt_flags> (static_cast<uint32_t> (a)
                                       | static_cast<uint32_t> (b));
  return a;
}

int
Shell::shopt_builtin (WORD_LIST *list)
{
  int opt, rval;

  shopt_flags flags = SHOPT_NOFLAGS;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "psuoq")) != -1)
    {
      switch (opt)
        {
        case 's':
          flags |= SFLAG;
          break;
        case 'u':
          flags |= UFLAG;
          break;
        case 'q':
          flags |= QFLAG;
          break;
        case 'o':
          flags |= OFLAG;
          break;
        case 'p':
          flags |= PFLAG;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }

  list = loptend;

  if ((flags & (SFLAG | UFLAG)) == (SFLAG | UFLAG))
    {
      builtin_error (_ ("cannot set and unset shell options simultaneously"));
      return EXECUTION_FAILURE;
    }

  rval = EXECUTION_SUCCESS;
  if ((flags & OFLAG) && ((flags & (SFLAG | UFLAG)) == 0)) /* shopt -o */
    rval = list_shopt_o_options (list, flags);
  else if (list && (flags & OFLAG)) /* shopt -so args */
    rval = set_shopt_o_options ((flags & SFLAG) ? FLAG_ON : FLAG_OFF, list);
  else if (flags & OFLAG) /* shopt -so */
    rval = list_some_o_options ((flags & SFLAG) ? 1 : 0, flags);
  else if (list && (flags & (SFLAG | UFLAG))) /* shopt -su args */
    rval = toggle_shopts ((flags & SFLAG) ? SETOPT : UNSETOPT, list);
  else if ((flags & (SFLAG | UFLAG)) == 0) /* shopt [args] */
    rval = list_shopts (list, flags);
  else /* shopt -su */
    rval = list_some_shopts ((flags & SFLAG) ? SETOPT : UNSETOPT, flags);
  return rval;
}

/* Reset the options managed by `shopt' to the values they would have at
   shell startup.  Variables from shopt_vars. */
void
Shell::reset_shopt_options ()
{
  autocd = cdable_vars = cdspelling = 0;
  check_hashed_filenames = CHECKHASH_DEFAULT;
  check_window_size = CHECKWINSIZE_DEFAULT;
  allow_null_glob_expansion = glob_dot_filenames = 0;
  no_exit_on_failed_exec = 0;
  expand_aliases = 0;
  extended_quote = 1;
  fail_glob_expansion = 0;
  glob_asciirange = GLOBASCII_DEFAULT;
  glob_star = 0;
  gnu_error_format = 0;
  hup_on_exit = 0;
  inherit_errexit = 0;
  interactive_comments = 1;
  lastpipe_opt = 0;
  localvar_inherit = localvar_unset = 0;
  mail_warning = 0;
  glob_ignore_case = match_ignore_case = 0;
  print_shift_error = 0;
  source_uses_path = promptvars = 1;

#if defined(JOB_CONTROL)
  check_jobs_at_exit = 0;
#endif

#if defined(EXTENDED_GLOB)
  extended_glob = EXTGLOB_DEFAULT;
#endif

#if defined(ARRAY_VARS)
  assoc_expand_once = 0;
#endif

#if defined(HISTORY)
  literal_history = 0;
  force_append_history = 0;
  command_oriented_history = 1;
#endif

#if defined(SYSLOG_HISTORY)
#if defined(SYSLOG_SHOPT)
  syslog_history = SYSLOG_SHOPT;
#else
  syslog_history = 1;
#endif /* SYSLOG_SHOPT */
#endif

#if defined(READLINE)
  complete_fullquote = 1;
  force_fignore = 1;
  hist_verify = history_reediting = 0;
  perform_hostname_completion = 1;
#if DIRCOMPLETE_EXPAND_DEFAULT
  dircomplete_expand = 1;
#else
  dircomplete_expand = 0;
#endif
  dircomplete_spelling = 0;
  no_empty_command_completion = 0;
#endif

#if defined(PROGRAMMABLE_COMPLETION)
  prog_completion_enabled = 1;
#if defined(ALIAS)
  progcomp_alias = 0;
#endif
#endif

#if defined(DEFAULT_ECHO_TO_XPG) || defined(STRICT_POSIX)
  xpg_echo = 1;
#else
  xpg_echo = 0;
#endif /* DEFAULT_ECHO_TO_XPG */

  shopt_login_shell = login_shell;
}

int
Shell::find_shopt (const char *name)
{
  std::vector<shopt_var_t>::iterator it;

  for (it = shopt_vars.begin (); it != shopt_vars.end (); ++it)
    if (STREQ (name, (*it).name))
      return static_cast<int> (it - shopt_vars.begin ());

  return -1;
}

int
Shell::toggle_shopts (char mode, WORD_LIST *list)
{
  WORD_LIST *l;
  int ind, rval;
  SHELL_VAR *v;

  for (l = list, rval = EXECUTION_SUCCESS; l; l = l->next ())
    {
      ind = find_shopt (l->word->word.c_str ());
      if (ind < 0)
        {
          shopt_error (l->word->word.c_str ());
          rval = EXECUTION_FAILURE;
        }
      else
        {
          *shopt_vars[ind].value = mode; /* 1 for set, 0 for unset */
          if (shopt_vars[ind].set_func)
            ((*this).*shopt_vars[ind].set_func) (shopt_vars[ind].name, mode);
        }
    }

  /* Don't set $BASHOPTS here if it hasn't already been initialized */
  if ((v = find_variable ("BASHOPTS")))
    set_bashopts ();
  return rval;
}

/* List the values of all or any of the `shopt' options.  Returns 0 if
   all were listed or all variables queried were on; 1 otherwise. */
int
Shell::list_shopts (WORD_LIST *list, shopt_flags flags)
{
  WORD_LIST *l;
  int rval;

  if (list == nullptr)
    {
      std::vector<shopt_var_t>::iterator it;
      for (it = shopt_vars.begin (); it != shopt_vars.end (); ++it)
        {
          char val = *(*it).value;
          if ((flags & QFLAG) == 0)
            print_shopt ((*it).name, val, flags);
        }
      return sh_chkwrite (EXECUTION_SUCCESS);
    }

  for (l = list, rval = EXECUTION_SUCCESS; l; l = l->next ())
    {
      int i = find_shopt (l->word->word.c_str ());
      if (i < 0)
        {
          shopt_error (l->word->word.c_str ());
          rval = EXECUTION_FAILURE;
          continue;
        }

      char val = *shopt_vars[static_cast<size_t> (i)].value;
      if (val == 0)
        rval = EXECUTION_FAILURE;

      if ((flags & QFLAG) == 0)
        print_shopt (l->word->word.c_str (), val, flags);
    }

  return sh_chkwrite (rval);
}

int
Shell::list_some_shopts (char mode, shopt_flags flags)
{
  std::vector<shopt_var_t>::iterator it;

  for (it = shopt_vars.begin (); it != shopt_vars.end (); ++it)
    {
      char val = *(*it).value;
      if (((flags & QFLAG) == 0) && mode == val)
        print_shopt ((*it).name, val, flags);
    }

  return sh_chkwrite (EXECUTION_SUCCESS);
}

int
Shell::list_shopt_o_options (WORD_LIST *list, shopt_flags flags)
{
  WORD_LIST *l;
  int val, rval;

  if (list == 0)
    {
      if ((flags & QFLAG) == 0)
        list_minus_o_opts (-1, (flags & PFLAG));
      return sh_chkwrite (EXECUTION_SUCCESS);
    }

  for (l = list, rval = EXECUTION_SUCCESS; l; l = l->next ())
    {
      val = minus_o_option_value (l->word->word.c_str ());
      if (val == -1)
        {
          sh_invalidoptname (l->word->word.c_str ());
          rval = EXECUTION_FAILURE;
          continue;
        }
      if (val == 0)
        rval = EXECUTION_FAILURE;
      if ((flags & QFLAG) == 0)
        {
          if (flags & PFLAG)
            printf ("set %co %s\n", val ? '-' : '+', l->word->word.c_str ());
          else
            printf (OPTFMT, l->word->word.c_str (), val ? on : off);
        }
    }
  return sh_chkwrite (rval);
}

int
Shell::list_some_o_options (char mode, shopt_flags flags)
{
  if ((flags & QFLAG) == 0)
    list_minus_o_opts (mode, (flags & PFLAG));
  return sh_chkwrite (EXECUTION_SUCCESS);
}

int
Shell::set_shopt_o_options (char mode, WORD_LIST *list)
{
  WORD_LIST *l;
  int rval;

  for (l = list, rval = EXECUTION_SUCCESS; l; l = l->next ())
    {
      if (set_minus_o_option (mode, l->word->word.c_str ())
          == EXECUTION_FAILURE)
        rval = EXECUTION_FAILURE;
    }
  set_shellopts ();
  return rval;
}

/* If we set or unset interactive_comments with shopt, make sure the
   change is reflected in $SHELLOPTS. */
int
Shell::set_shellopts_after_change (const char *option_name, char mode)
{
  set_shellopts ();
  return 0;
}

#if defined(DEBUGGER)
int
Shell::shopt_set_debug_mode (const char *option_name, char mode)
{
  error_trace_mode = function_trace_mode = debugging_mode;
  set_shellopts ();
  if (debugging_mode)
    init_bash_argv ();
  return 0;
}
#endif

#if defined(READLINE)
int
Shell::shopt_enable_hostname_completion (const char *option_name, char mode)
{
  return enable_hostname_completion (mode);
}
#endif

int
Shell::set_compatibility_level (const char *option_name, char mode)
{
  int ind;
  char *rhs;

  /* If we're setting something, redo some of the work we did above in
     toggle_shopt().  Unset everything and reset the appropriate option
     based on OPTION_NAME. */
  if (mode)
    {
      shopt_compat31 = shopt_compat32 = 0;
      shopt_compat40 = shopt_compat41 = shopt_compat42 = shopt_compat43 = 0;
      shopt_compat44 = 0;
      ind = find_shopt (option_name);
      *shopt_vars[ind].value = mode;
    }

  /* Then set shell_compatibility_level based on what remains */
  if (shopt_compat31)
    shell_compatibility_level = 31;
  else if (shopt_compat32)
    shell_compatibility_level = 32;
  else if (shopt_compat40)
    shell_compatibility_level = 40;
  else if (shopt_compat41)
    shell_compatibility_level = 41;
  else if (shopt_compat42)
    shell_compatibility_level = 42;
  else if (shopt_compat43)
    shell_compatibility_level = 43;
  else if (shopt_compat44)
    shell_compatibility_level = 44;
  else
    shell_compatibility_level = DEFAULT_COMPAT_LEVEL;

  /* Make sure the current compatibility level is reflected in BASH_COMPAT */
  rhs = itos (shell_compatibility_level);
  bind_variable ("BASH_COMPAT", rhs, 0);
  free (rhs);

  return 0;
}

/* Set and unset the various compatibility options from the value of
   shell_compatibility_level; used by sv_shcompat */
void
Shell::set_compatibility_opts ()
{
  shopt_compat31 = shopt_compat32 = 0;
  shopt_compat40 = shopt_compat41 = shopt_compat42 = shopt_compat43 = 0;
  shopt_compat44 = 0;
  switch (shell_compatibility_level)
    {
    case DEFAULT_COMPAT_LEVEL:
      break;
    case 44:
      shopt_compat44 = 1;
      break;
    case 43:
      shopt_compat43 = 1;
      break;
    case 42:
      shopt_compat42 = 1;
      break;
    case 41:
      shopt_compat41 = 1;
      break;
    case 40:
      shopt_compat40 = 1;
      break;
    case 32:
      shopt_compat32 = 1;
      break;
    case 31:
      shopt_compat31 = 1;
      break;
    }
}

#if defined(READLINE)
int
Shell::shopt_set_complete_direxpand (const char *option_name, char mode)
{
  set_directory_hook ();
  return 0;
}
#endif

#if defined(RESTRICTED_SHELL)
/* Don't allow the value of restricted_shell to be modified. */
int
Shell::set_restricted_shell (const char *option_name, char mode)
{
  if (save_restricted == -1)
    save_restricted = shell_is_restricted (shell_name);

  restricted_shell = save_restricted;
  return 0;
}
#endif /* RESTRICTED_SHELL */

int
Shell::set_login_shell (const char *option_name, char mode)
{
  shopt_login_shell = (login_shell != 0);
  return 0;
}

STRINGLIST *
Shell::get_shopt_options ()
{
  char **ret;
  int n, i;

  n = sizeof (shopt_vars) / sizeof (shopt_vars[0]);
  ret = strvec_create (n + 1);
  for (i = 0; shopt_vars[i].name; i++)
    ret[i] = savestring (shopt_vars[i].name);
  ret[i] = nullptr;
  return ret;
}

/*
 * External interface for other parts of the shell.  NAME is a string option;
 * MODE is 0 if we want to unset an option; 1 if we want to set an option.
 * REUSABLE is 1 if we want to print output in a form that may be reused.
 */
int
Shell::shopt_setopt (const char *name, char mode)
{
  WORD_LIST *wl;
  int r;

  wl = add_string_to_list (name, nullptr);
  r = toggle_shopts (mode, wl);
  dispose_words (wl);
  return r;
}

int
Shell::shopt_listopt (const char *name, bool reusable)
{
  int i;

  if (name == 0)
    return list_shopts (nullptr, reusable ? PFLAG : SHOPT_NOFLAGS);

  i = find_shopt (name);
  if (i < 0)
    {
      shopt_error (name);
      return EXECUTION_FAILURE;
    }

  print_shopt (name, *shopt_vars[i].value, reusable ? PFLAG : SHOPT_NOFLAGS);
  return sh_chkwrite (EXECUTION_SUCCESS);
}

void
Shell::set_bashopts ()
{
  char *value;
  char tflag[N_SHOPT_OPTIONS];
  int vsize, i, vptr, *ip, exported;
  SHELL_VAR *v;

  for (vsize = i = 0; shopt_vars[i].name; i++)
    {
      tflag[i] = 0;
      if (GET_SHOPT_OPTION_VALUE (i))
        {
          vsize += strlen (shopt_vars[i].name) + 1;
          tflag[i] = 1;
        }
    }

  value = (char *)xmalloc (vsize + 1);

  for (i = vptr = 0; shopt_vars[i].name; i++)
    {
      if (tflag[i])
        {
          strcpy (value + vptr, shopt_vars[i].name);
          vptr += strlen (shopt_vars[i].name);
          value[vptr++] = ':';
        }
    }

  if (vptr)
    vptr--; /* cut off trailing colon */
  value[vptr] = '\0';

  v = find_variable ("BASHOPTS");

  /* Turn off the read-only attribute so we can bind the new value, and
     note whether or not the variable was exported. */
  if (v)
    {
      VUNSETATTR (v, att_readonly);
      exported = exported_p (v);
    }
  else
    exported = 0;

  v = bind_variable ("BASHOPTS", value, 0);

  /* Turn the read-only attribute back on, and turn off the export attribute
     if it was set implicitly by mark_modified_vars and SHELLOPTS was not
     exported before we bound the new value. */
  VSETATTR (v, att_readonly);
  if (mark_modified_vars && exported == 0 && exported_p (v))
    VUNSETATTR (v, att_exported);

  free (value);
}

void
Shell::parse_bashopts (const char *value)
{
  char *vname;

  size_t vptr = 0;
  while ((vname = extract_colon_unit (value, &vptr)))
    {
      int ind = find_shopt (vname);
      if (ind >= 0)
        {
          *shopt_vars[ind].value = 1;
          if (shopt_vars[ind].set_func)
            ((*this).*shopt_vars[ind].set_func) (shopt_vars[ind].name, 1);
        }
      delete[] vname;
    }
}

void
Shell::initialize_bashopts (bool no_bashopts)
{
  if (!no_bashopts)
    {
      SHELL_VAR *var = find_variable ("BASHOPTS");
      /* set up any shell options we may have inherited. */
      if (var && var->imported ())
        {
          const char *temp
              = (var->array () || var->assoc ()) ? nullptr : var->str_value ();
          if (temp)
            parse_bashopts (temp);
        }
    }

  /* Set up the $BASHOPTS variable. */
  set_bashopts ();
}

} // namespace bash
