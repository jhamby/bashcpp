/* variables.cc -- Functions for hacking shell variables. */

/* Copyright (C) 1987-2022 Free Software Foundation, Inc.

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

#include "config.h"

#include "shell.hh"

#include "conftypes.hh"
#include "version.hh"

// Process environment variables supplied by the C library.
extern "C" char **environ;

namespace bash
{

static inline char *mk_env_string (string_view, string_view,
                                   var_att_flags = att_noflags);

constexpr int VARIABLES_HASH_BUCKETS = 1024; /* must be power of two */
constexpr int FUNCTIONS_HASH_BUCKETS = 512;
constexpr int TEMPENV_HASH_BUCKETS = 4;

static const char *BASHFUNC_PREFIX = "BASH_FUNC_";
constexpr int BASHFUNC_PREFLEN = 10;

static const char *BASHFUNC_SUFFIX = "%%";
constexpr int BASHFUNC_SUFFLEN = 2;

#if defined(ARRAY_EXPORT)
#define BASHARRAY_PREFIX "BASH_ARRAY_"
#define BASHARRAY_PREFLEN 11
#define BASHARRAY_SUFFIX "%%"
#define BASHARRAY_SUFFLEN 2

#define BASHASSOC_PREFIX "BASH_ASSOC_"
#define BASHASSOC_PREFLEN 11
#define BASHASSOC_SUFFIX "%%" /* needs to be the same as BASHARRAY_SUFFIX */
#define BASHASSOC_SUFFLEN 2
#endif

void
Shell::create_variable_tables ()
{
  if (shell_variables == nullptr)
    {
      shell_variables = global_variables
          = new VAR_CONTEXT (VC_NOFLAGS, variable_context);
      shell_variables->scope = 0;
    }

  if (shell_functions == nullptr)
    shell_functions = new HASH_TABLE<SHELL_VAR> (FUNCTIONS_HASH_BUCKETS);

#if defined(DEBUGGER)
  if (shell_function_defs == nullptr)
    shell_function_defs
        = new HASH_TABLE<FUNCTION_DEF> (FUNCTIONS_HASH_BUCKETS);
#endif
}

/* Initialize the shell variables from the current environment.
   If PRIVMODE is true, don't import functions from ENV or
   parse $SHELLOPTS. */
void
Shell::initialize_shell_variables (char **env, bool privmode)
{
  create_variable_tables ();

  char *name, *string;
  size_t string_index;
  SHELL_VAR *temp_var = nullptr;

  for (string_index = 0; env && (string = env[string_index++]);)
    {
      size_t char_index = 0;
      char c;

      name = string;
      while ((c = *string++) && c != '=')
        ;

      if (string[-1] == '=')
        char_index = static_cast<size_t> (string - name - 1);

      /* If there are weird things in the environment, like `=xxx' or a
         string without an `=', just skip them. */
      if (char_index == 0)
        continue;

      /* ASSERT(name[char_index] == '=') */
      name[char_index] = '\0';

      /* Now, name = env variable name, string = env variable value, and
         char_index == strlen (name) */

#if defined(FUNCTION_IMPORT)
      /* If exported function, define it now.  Don't import functions from
         the environment in privileged mode. */
      if (!privmode && read_but_dont_execute == 0
          && STREQN (BASHFUNC_PREFIX, name, BASHFUNC_PREFLEN)
          && STREQ (BASHFUNC_SUFFIX, name + char_index - BASHFUNC_SUFFLEN)
          && STREQN ("() {", string, 4))
        {
          char *tname; /* desired imported function name */

          size_t namelen = static_cast<size_t> (char_index - BASHFUNC_PREFLEN
                                                - BASHFUNC_SUFFLEN);

          tname = name + BASHFUNC_PREFLEN; /* start of func name */
          tname[namelen] = '\0';           /* now tname == func name */

          size_t string_length = strlen (string);
          char *temp_string = new char[namelen + string_length + 2];

          memcpy (temp_string, tname, static_cast<size_t> (namelen));
          temp_string[namelen] = ' ';
          memcpy (temp_string + namelen + 1, string, string_length + 1);

          /* Don't import function names that are invalid identifiers from the
             environment in posix mode, though we still allow them to be
             defined as shell variables. */
          if (!absolute_program (tname)
              && (!posixly_correct || legal_identifier (tname)))
            parse_and_execute (temp_string, tname,
                               SEVAL_NONINT | SEVAL_NOHIST | SEVAL_FUNCDEF
                                   | SEVAL_ONECMD);
          else
            delete[] temp_string; /* parse_and_execute does this */

          if ((temp_var = find_function (tname)))
            {
              temp_var->set_attr (att_exported | att_imported);
              array_needs_making = true;
            }
          else
            {
              if ((temp_var = bind_invalid_envvar (name, string)))
                {
                  temp_var->set_attr (att_exported | att_imported
                                      | att_invisible);
                  array_needs_making = true;
                }
              last_command_exit_value = EXECUTION_FAILURE;
              report_error (_ ("error importing function definition for `%s'"),
                            tname);
            }

          /* Restore original suffix */
          tname[namelen] = BASHFUNC_SUFFIX[0];
        }
      else
#endif /* FUNCTION_IMPORT */
#if defined(ARRAY_VARS)
#if defined(ARRAY_EXPORT)
        /* Array variables may not yet be exported. */
        if (STREQN (BASHARRAY_PREFIX, name, BASHARRAY_PREFLEN)
            && STREQN (BASHARRAY_SUFFIX, name + char_index - BASHARRAY_SUFFLEN,
                       BASHARRAY_SUFFLEN)
            && *string == '(' && string[1] == '['
            && string[strlen (string) - 1] == ')')
          {
            size_t namelen;
            char *tname; /* desired imported array variable name */

            namelen = char_index - BASHARRAY_PREFLEN - BASHARRAY_SUFFLEN;

            tname = name + BASHARRAY_PREFLEN; /* start of variable name */
            tname[namelen] = '\0';            /* now tname == varname */

            int string_length = 1;
            string temp_string (
                extract_array_assignment_list (string, &string_length));

            temp_var = assign_array_from_string (tname, temp_string, 0);

            if (temp_var)
              {
                temp_var->set_attr (att_exported | att_imported);
                array_needs_making = true;
              }
          }
        else if (STREQN (BASHASSOC_PREFIX, name, BASHASSOC_PREFLEN)
                 && STREQN (BASHASSOC_SUFFIX,
                            name + char_index - BASHASSOC_SUFFLEN,
                            BASHASSOC_SUFFLEN)
                 && *string == '(' && string[1] == '['
                 && string[strlen (string) - 1] == ')')
          {
            size_t namelen;
            char *tname; /* desired imported assoc variable name */

            namelen = char_index - BASHASSOC_PREFLEN - BASHASSOC_SUFFLEN;

            tname = name + BASHASSOC_PREFLEN; /* start of variable name */
            tname[namelen] = '\0';            /* now tname == varname */

            /* need to make sure it exists as an associative array first */
            temp_var = find_or_make_array_variable (tname, 2);
            if (temp_var)
              {
                int string_length = 1;
                string temp_string (
                    extract_array_assignment_list (string, &string_length));

                temp_var
                    = assign_array_var_from_string (temp_var, temp_string, 0);
              }
            if (temp_var)
              {
                temp_var->set_attr (att_exported | att_imported);
                array_needs_making = true;
              }
          }
        else
#endif /* ARRAY_EXPORT */
#endif
          {
            bool ro = false;
            /* If we processed a command-line option that caused SHELLOPTS to
               be set, it may already be set (and read-only) by the time we
               process the shell's environment. */
            if (/* posixly_correct &&*/ STREQ (name, "SHELLOPTS"))
              {
                temp_var = find_variable ("SHELLOPTS");
                ro = temp_var && temp_var->readonly ();
                if (temp_var)
                  temp_var->unset_attr (att_readonly);
              }
            if (legal_identifier (name))
              {
                temp_var = bind_variable (name, string);
                if (temp_var)
                  {
                    temp_var->set_attr (att_exported | att_imported);
                    if (ro)
                      temp_var->set_attr (att_readonly);
                  }
              }
            else
              {
                temp_var = bind_invalid_envvar (name, string);
                if (temp_var)
                  temp_var->set_attr (att_exported | att_imported
                                      | att_invisible);
              }
            if (temp_var)
              array_needs_making = true;
          }

      name[char_index] = '=';

      /* temp_var can be nullptr if it was an exported function with a syntax
         error (a different bug, but it still shouldn't dump core). */
      if (temp_var && !temp_var->function ()) /* XXX not yet */
        temp_var->exportstr = name;
    }

  set_pwd ();

  /* Set up initial value of $_ */
  temp_var = set_if_not ("_", dollar_vars[0]);

  /* Remember this pid. */
  dollar_dollar_pid = getpid ();

  /* Now make our own defaults in case the vars that we think are
     important are missing. */
  temp_var = set_if_not ("PATH", DEFAULT_PATH_VALUE);
  temp_var = set_if_not ("TERM", "dumb");

#if defined(__QNX__)
  /* set node id -- don't import it from the environment */
  {
    char node_name[22];
#if defined(__QNXNTO__)
    netmgr_ndtostr (ND2S_LOCAL_STR, ND_LOCAL_NODE, node_name,
                    sizeof (node_name));
#else
    qnx_nidtostr (getnid (), node_name, sizeof (node_name));
#endif
    temp_var = bind_variable ("NODE", node_name, 0);
    if (temp_var)
      set_auto_export (temp_var);
  }
#endif

  /* set up the prompts. */
  if (interactive_shell)
    {
#if defined(PROMPT_STRING_DECODE)
      set_if_not ("PS1", primary_prompt);
#else
      if (current_user.uid == -1)
        get_current_user_info ();
      set_if_not ("PS1",
                  current_user.euid == 0 ? string ("# ") : primary_prompt);
#endif
      set_if_not ("PS2", secondary_prompt);
    }

  if (current_user.euid == 0)
    bind_variable ("PS4", "+ ");
  else
    set_if_not ("PS4", "+ ");

  /* Don't allow IFS to be imported from the environment. */
  temp_var = bind_variable ("IFS", " \t\n");
  setifs (temp_var);

  /* Magic machine types.  Pretty convenient. */
  set_machine_vars ();

  /* Default MAILCHECK for interactive shells.  Defer the creation of a
     default MAILPATH until the startup files are read, because MAIL
     names a mail file if MAILPATH is not set, and we should provide a
     default only if neither is set. */
  if (interactive_shell)
    {
      temp_var = set_if_not ("MAILCHECK", (posixly_correct ? "600" : "60"));
      temp_var->set_attr (att_integer);
    }

  /* Do some things with shell level. */
  initialize_shell_level ();

  set_ppid ();

  set_argv0 ();

  /* Initialize the `getopts' stuff. */
  temp_var = bind_variable ("OPTIND", "1");
  temp_var->set_attr (att_integer);

  getopts_reset (0);
  bind_variable ("OPTERR", "1");
  sh_opterr = true;

  if (login_shell == 1 && posixly_correct == 0)
    set_home_var ();

  // Get the full pathname to THIS shell, and set the BASH variable to it.
  temp_var = bind_variable ("BASH", get_bash_name ());

  /* Make the exported environment variable SHELL be the user's login
     shell.  Note that the `tset' command looks at this variable
     to determine what style of commands to output; if it ends in "csh",
     then C-shell commands are output, else Bourne shell commands. */
  set_shell_var ();

  /* Make a variable called BASH_VERSION which contains the version info. */
  bind_variable ("BASH_VERSION", shell_version_string ());

#if defined(ARRAY_VARS)
  make_vers_array ();
#endif

  if (command_execution_string)
    bind_variable ("BASH_EXECUTION_STRING", command_execution_string);

  /* Find out if we're supposed to be in Posix.2 mode via an
     environment variable. */
  temp_var = find_variable ("POSIXLY_CORRECT");
  if (!temp_var)
    temp_var = find_variable ("POSIX_PEDANTIC");
  if (temp_var && temp_var->imported ())
    sv_strict_posix (temp_var->name ());

#if defined(HISTORY)
  /* Set history variables to defaults, and then do whatever we would
     do if the variable had just been set.  Do this only in the case
     that we are remembering commands on the history list. */
  if (remember_on_history)
    {
      name = bash_tilde_expand (
          posixly_correct ? "~/.sh_history" : "~/.bash_history", 0);

      set_if_not ("HISTFILE", name);
      delete[] name;
    }
#endif /* HISTORY */

  /* Seed the random number generators. */
  seedrand ();
  seedrand32 ();

  /* Handle some "special" variables that we may have inherited from a
     parent shell. */
  if (interactive_shell)
    {
      temp_var = find_variable ("IGNOREEOF");
      if (!temp_var)
        temp_var = find_variable ("ignoreeof");
      if (temp_var && temp_var->imported ())
        sv_ignoreeof (temp_var->name ());
    }

#if defined(HISTORY)
  if (interactive_shell && remember_on_history)
    {
      sv_history_control ("HISTCONTROL");
      sv_histignore ("HISTIGNORE");
      sv_histtimefmt ("HISTTIMEFORMAT");
    }
#endif /* HISTORY */

#if defined(READLINE) && defined(STRICT_POSIX)
  /* POSIXLY_CORRECT will be 1 here if the shell was compiled
     -DSTRICT_POSIX or if POSIXLY_CORRECT was supplied in the shell's
     environment */
  if (interactive_shell && posixly_correct && no_line_editing == 0)
    rl_prefer_env_winsize = 1;
#endif /* READLINE && STRICT_POSIX */

  /* Get the user's real and effective user ids. */
  uidset ();

  temp_var
      = set_if_not ("BASH_LOADABLES_PATH", DEFAULT_LOADABLE_BUILTINS_PATH);

  temp_var = find_variable ("BASH_XTRACEFD");
  if (temp_var && temp_var->imported ())
    sv_xtracefd (temp_var->name ());

  sv_shcompat ("BASH_COMPAT");

  /* Allow FUNCNEST to be inherited from the environment. */
  sv_funcnest ("FUNCNEST");

  /* Initialize the dynamic variables, and seed their values. */
  initialize_dynamic_variables ();
}

/* **************************************************************** */
/*								    */
/*	     Setting values for special shell variables		    */
/*								    */
/* **************************************************************** */

void
Shell::set_machine_vars ()
{
  (void)set_if_not ("HOSTTYPE", HOSTTYPE);
  (void)set_if_not ("OSTYPE", OSTYPE);
  (void)set_if_not ("MACHTYPE", MACHTYPE);
  (void)set_if_not ("HOSTNAME", current_host_name);
}

/* Set $HOME to the information in the password file if we didn't get
   it from the environment. */

/* Virtual function: don't inline. */
const char *
Shell::sh_get_home_dir ()
{
  if (current_user.home_dir == nullptr)
    get_current_user_info ();

  return current_user.home_dir;
}

void
Shell::set_home_var ()
{
  SHELL_VAR *temp_var;

  temp_var = find_variable ("HOME");
  if (temp_var == nullptr)
    temp_var = bind_variable ("HOME", sh_get_home_dir ());
#if 0
  temp_var->set_attr (att_exported);
#endif
}

/* Set $SHELL to the user's login shell if it is not already set.  Call
   get_current_user_info if we haven't already fetched the shell. */
void
Shell::set_shell_var ()
{
  SHELL_VAR *temp_var;

  temp_var = find_variable ("SHELL");
  if (temp_var == nullptr)
    {
      if (current_user.shell == nullptr)
        get_current_user_info ();
      temp_var = bind_variable ("SHELL", current_user.shell);
    }
#if 0
  temp_var->set_attr (att_exported);
#endif
}

string
Shell::get_bash_name ()
{
  string name;

  if ((login_shell == 1) && RELPATH (shell_name))
    {
      if (current_user.shell == nullptr)
        get_current_user_info ();
      name = current_user.shell;
    }
  else if (ABSPATH (shell_name))
    name = shell_name;
  else if (shell_name[0] == '.' && shell_name[1] == '/')
    {
      /* Fast path for common case. */
      const string *cdir = get_string_value ("PWD");
      if (cdir)
        {
          name = *cdir;
          name += (shell_name + 1);
        }
      else
        name = shell_name;
    }
  else
    {
      char *tname;
      file_stat_flags s;

      tname = find_user_command (shell_name);

      if (tname == nullptr)
        {
          /* Try the current directory.  If there is not an executable
             there, just punt and use the login shell. */
          s = file_status (shell_name);
          if (s & FS_EXECABLE)
            {
              const string *cdir = get_string_value ("PWD");
              tname = make_absolute (shell_name,
                                     cdir ? cdir->c_str () : nullptr);
              if (*shell_name == '.')
                {
                  // Free cname with free (), not delete.
                  char *cname = canonicalize_file_name (tname);
                  if (cname == nullptr)
                    name = tname;
                  else
                    {
                      name = cname;
                      free (cname);
                    }
                  delete[] tname;
                }
              else
                {
                  name = tname;
                  delete[] tname;
                }
            }
          else
            {
              if (current_user.shell == nullptr)
                get_current_user_info ();
              name = savestring (current_user.shell);
            }
        }
      else
        {
          name = full_pathname (tname);
          delete[] tname;
        }
    }

  return name;
}

void
Shell::adjust_shell_level (int change)
{
  char new_level[5];
  int64_t old_level;
  SHELL_VAR *temp_var;

  const string *old_SHLVL = get_string_value ("SHLVL");
  if (old_SHLVL == nullptr || old_SHLVL->empty ()
      || !legal_number (old_SHLVL->c_str (), &old_level))
    old_level = 0;

  shell_level = static_cast<int> (old_level) + change;
  if (shell_level < 0)
    shell_level = 0;
  else if (shell_level >= 1000)
    {
      internal_warning (_ ("shell level (%d) too high, resetting to 1"),
                        shell_level);
      shell_level = 1;
    }

  /* We don't need the full generality of itos here. */
  if (shell_level < 10)
    {
      new_level[0] = static_cast<char> (shell_level + '0');
      new_level[1] = '\0';
    }
  else if (shell_level < 100)
    {
      new_level[0] = static_cast<char> ((shell_level / 10) + '0');
      new_level[1] = static_cast<char> ((shell_level % 10) + '0');
      new_level[2] = '\0';
    }
  else if (shell_level < 1000)
    {
      new_level[0] = static_cast<char> ((shell_level / 100) + '0');
      old_level = static_cast<char> (shell_level % 100);
      new_level[1] = static_cast<char> ((old_level / 10) + '0');
      new_level[2] = static_cast<char> ((old_level % 10) + '0');
      new_level[3] = '\0';
    }

  temp_var = bind_variable ("SHLVL", new_level);
  set_auto_export (temp_var);
}

/* If we got PWD from the environment, update our idea of the current
   working directory.  In any case, make sure that PWD exists before
   checking it.  It is possible for getcwd () to fail on shell startup,
   and in that case, PWD would be undefined.  If this is an interactive
   login shell, see if $HOME is the current working directory, and if
   that's not the same string as $PWD, set PWD=$HOME. */
void
Shell::set_pwd ()
{
  SHELL_VAR *home_var = find_variable ("HOME");
  string *home_string = home_var ? home_var->str_value () : nullptr;

  string *temp_string;
  SHELL_VAR *temp_var = find_variable ("PWD");
  /* Follow posix rules for importing PWD */
  if (temp_var && temp_var->imported ()
      && ((temp_string = temp_var->str_value ()) && (*temp_string)[0] == '/'
          && same_file (temp_string->c_str (), ".", nullptr, nullptr)))
    {
      // Note: call free() on this string, not delete[]
      char *current_dir = canonicalize_file_name (temp_string->c_str ());
      if (current_dir == nullptr)
        current_dir = get_working_directory ("shell_init");
      else
        set_working_directory (current_dir);
      if (posixly_correct && current_dir)
        {
          temp_var = bind_variable ("PWD", current_dir);
          set_auto_export (temp_var);
        }
      free (current_dir);
    }
  else if (home_string && interactive_shell && login_shell
           && same_file (home_string->c_str (), ".", nullptr, nullptr))
    {
      set_working_directory (home_string->c_str ());
      temp_var = bind_variable ("PWD", *home_string);
      set_auto_export (temp_var);
    }
  else
    {
      char *working_dir = get_working_directory ("shell-init");
      if (working_dir)
        {
          temp_var = bind_variable ("PWD", working_dir);
          set_auto_export (temp_var);
          free (working_dir);
        }
    }

  /* According to the Single Unix Specification, v2, $OLDPWD is an
     `environment variable' and therefore should be auto-exported.  If we
     don't find OLDPWD in the environment, or it doesn't name a directory,
     make a dummy invisible variable for OLDPWD, and mark it as exported. */
  temp_var = find_variable ("OLDPWD");
#if defined(OLDPWD_CHECK_DIRECTORY)
  if (temp_var == nullptr || temp_var->str_value () == nullptr
      || !file_isdir (temp_var->str_value ()->c_str ()))
#else
  if (temp_var == nullptr || temp_var->str_value () == nullptr)
#endif
    {
      temp_var = bind_variable ("OLDPWD", string ());
      temp_var->set_attr (att_exported | att_invisible);
    }
}

/* Make a variable $PPID, which holds the pid of the shell's parent.  */
void
Shell::set_ppid ()
{
  SHELL_VAR *temp_var;

  string name (inttostr (getppid ()));

  temp_var = find_variable ("PPID");
  if (temp_var)
    temp_var->unset_attr (att_readonly | att_exported);

  temp_var = bind_variable ("PPID", name);
  temp_var->set_attr (att_readonly | att_integer);
}

void
Shell::uidset ()
{
  SHELL_VAR *v;

  string b (inttostr (current_user.uid));
  v = find_variable ("UID");
  if (v == nullptr)
    {
      v = bind_variable ("UID", b);
      v->set_attr (att_readonly | att_integer);
    }

  if (current_user.euid != current_user.uid)
    b = inttostr (current_user.euid);

  v = find_variable ("EUID");
  if (v == nullptr)
    {
      v = bind_variable ("EUID", b);
      v->set_attr (att_readonly | att_integer);
    }
}

#if defined(ARRAY_VARS)
void
Shell::make_vers_array ()
{
  SHELL_VAR *vv;
  ARRAY *av;
  char d[32];

  unbind_variable_noref ("BASH_VERSINFO");

  vv = make_new_array_variable ("BASH_VERSINFO");
  av = vv->array_value ();
  strcpy (d, dist_version);
  char *s = strchr (d, '.');
  if (s)
    *s++ = '\0';

  av->insert (0, d);
  av->insert (1, s);
  string s2 (inttostr (patch_level));
  av->insert (2, s2);
  s2 = inttostr (BUILDVERSION);
  av->insert (3, s2);
  av->insert (4, RELSTATUS);
  av->insert (5, MACHTYPE);

  vv->set_attr (att_readonly);
}
#endif /* ARRAY_VARS */

/* Set the environment variables $LINES and $COLUMNS in response to
   a window size change. */
void
Shell::sh_set_lines_and_columns (uint32_t lines, uint32_t cols)
{
#if defined(READLINE)
  /* If we are currently assigning to LINES or COLUMNS, don't do anything. */
  if (winsize_assignment)
    return;
#endif

  string v (inttostr (lines));
  bind_variable ("LINES", v);

  v = inttostr (cols);
  bind_variable ("COLUMNS", v);
}

/* **************************************************************** */
/*								    */
/*		   Printing variables and values		    */
/*								    */
/* **************************************************************** */

/* Print LIST (a list of shell variables) to stdout in such a way that
   they can be read back in. */
void
Shell::print_var_list (SHELL_VAR **list)
{
  int i;
  SHELL_VAR *var;

  for (i = 0; list && (var = list[i]); i++)
    if (!var->invisible ())
      print_assignment (var);
}

/* Print LIST (a list of shell functions) to stdout in such a way that
   they can be read back in. */
void
Shell::print_func_list (SHELL_VAR **list)
{
  int i;
  SHELL_VAR *var;

  for (i = 0; list && (var = list[i]); i++)
    {
      printf ("%s ", var->name ().c_str ());
      print_var_function (var);
      printf ("\n");
    }
}

/* Print the value of a single SHELL_VAR.  No newline is
   output, but the variable is printed in such a way that
   it can be read back in. */
void
Shell::print_assignment (SHELL_VAR *var)
{
  if (!var->is_set ())
    return;

  if (var->function ())
    {
      printf ("%s", var->name ().c_str ());
      print_var_function (var);
      printf ("\n");
    }
#if defined(ARRAY_VARS)
  else if (var->array ())
    print_array_assignment (var, 0);
  else if (var->assoc ())
    print_assoc_assignment (var, 0);
#endif /* ARRAY_VARS */
  else
    {
      printf ("%s=", var->name ().c_str ());
      print_var_value (var, true);
      printf ("\n");
    }
}

/* Print the value cell of VAR, a shell variable.  Do not print
   the name, nor leading/trailing newline.  If QUOTE is true,
   and the value contains shell metacharacters, quote the value
   in such a way that it can be read back in. */
void
Shell::print_var_value (SHELL_VAR *var, bool quote)
{
  if (!var->is_set ())
    return;

  const char *var_str = var->str_value ()->c_str ();
  if (quote && !posixly_correct && ansic_shouldquote (var_str))
    {
      string t (ansic_quote (var_str));
      printf ("%s", t.c_str ());
    }
  else if (quote && sh_contains_shell_metas (var_str))
    {
      string t (sh_single_quote (var_str));
      printf ("%s", t.c_str ());
    }
  else
    printf ("%s", var_str);
}

/* Print the function cell of VAR, a shell variable.  Do not
   print the name, nor leading/trailing newline. */
void
Shell::print_var_function (SHELL_VAR *var)
{
  if (var->function () && var->is_set ())
    {
      string x (named_function_string (nullptr, var->func_value (),
                                       FUNC_MULTILINE | FUNC_EXTERNAL));
      printf ("%s", x.c_str ());
    }
}

/* **************************************************************** */
/*								    */
/*		 	Dynamic Variables			    */
/*								    */
/* **************************************************************** */

/* DYNAMIC VARIABLES

   These are variables whose values are generated anew each time they are
   referenced.  These are implemented using a pair of method pointers
   in the struct variable: assign_func, which is called from bind_variable
   and, if arrays are compiled into the shell, some of the functions in
   arrayfunc.c, and dynamic_value, which is called from find_variable.

   assign_func is called from bind_variable_internal, if
   bind_variable_internal discovers that the variable being assigned to
   has such a function.  The function is called as
        SHELL_VAR *temp = (*(entry->assign_func)) (entry, value, ind)
   and the (SHELL_VAR *)temp is returned as the value of bind_variable.  It
   is usually ENTRY (self).  IND is an index for an array variable, and
   unused otherwise.

   dynamic_value is called from find_variable_internal to return a `new'
   value for the specified dynamic variable.  If this function is nullptr,
   the variable is treated as a `normal' shell variable.  If it is not,
   however, then this function is called like this:
        tempvar = (*(var->dynamic_value)) (var);

   Sometimes `tempvar' will replace the value of `var'.  Other times, the
   shell will simply use the string value.  Pretty object-oriented, huh?

   Be warned, though: if you `unset' a special variable, it loses its
   special meaning, even if you subsequently set it.

   The special assignment code would probably have been better put in
   subst.c: do_assignment_internal, in the same style as
   stupidly_hack_special_variables, but I wanted the changes as
   localized as possible.  */

#define INIT_DYNAMIC_VAR(var, val, gfunc, afunc)                              \
  do                                                                          \
    {                                                                         \
      v = bind_variable (var, (val));                                         \
      v->dynamic_value = gfunc;                                               \
      v->assign_func = afunc;                                                 \
    }                                                                         \
  while (0)

#define INIT_DYNAMIC_ARRAY_VAR(var, gfunc, afunc)                             \
  do                                                                          \
    {                                                                         \
      v = make_new_array_variable (var);                                      \
      v->dynamic_value = gfunc;                                               \
      v->assign_func = afunc;                                                 \
    }                                                                         \
  while (0)

#define INIT_DYNAMIC_ASSOC_VAR(var, gfunc, afunc)                             \
  do                                                                          \
    {                                                                         \
      v = make_new_assoc_variable (var);                                      \
      v->dynamic_value = gfunc;                                               \
      v->assign_func = afunc;                                                 \
    }                                                                         \
  while (0)

SHELL_VAR *
Shell::null_assign (SHELL_VAR *self, const string &, arrayind_t,
                    const string &)
{
  return self;
}

#if defined(ARRAY_VARS)
SHELL_VAR *
Shell::null_array_assign (SHELL_VAR *self, const string &, arrayind_t,
                          const string &)
{
  return self;
}
#endif

/* Degenerate `dynamic_value' function; just returns what's passed without
   manipulation. */
SHELL_VAR *
Shell::get_self (SHELL_VAR *self)
{
  return self;
}

#if defined(ARRAY_VARS)
/* A generic dynamic array variable initializer.  Initialize array variable
   NAME with dynamic value function GETFUNC and assignment function SETFUNC. */
SHELL_VAR *
Shell::init_dynamic_array_var (const string &name,
                               SHELL_VAR::value_func_t getfunc,
                               SHELL_VAR::assign_func_t setfunc,
                               var_att_flags attrs)
{
  SHELL_VAR *v;

  v = find_variable (name);
  if (v)
    return v;
  INIT_DYNAMIC_ARRAY_VAR (name, getfunc, setfunc);
  if (attrs)
    v->set_attr (attrs);
  return v;
}

SHELL_VAR *
Shell::init_dynamic_assoc_var (const string &name,
                               SHELL_VAR::value_func_t getfunc,
                               SHELL_VAR::assign_func_t setfunc,
                               var_att_flags attrs)
{
  SHELL_VAR *v;

  v = find_variable (name);
  if (v)
    return v;
  INIT_DYNAMIC_ASSOC_VAR (name, getfunc, setfunc);
  if (attrs)
    v->set_attr (attrs);
  return v;
}
#endif

SHELL_VAR *
Shell::assign_seconds (SHELL_VAR *self, const string &value, arrayind_t,
                       const string &)
{
  int64_t nval;
  bool expok;

  if (self->integer ())
    nval = evalexp (value, EXP_NOFLAGS, &expok);
  else
    expok = legal_number (value.c_str (), &nval);

  seconds_value_assigned = expok ? nval : 0;
  gettimeofday (&shellstart, nullptr);
  shell_start_time = shellstart.tv_sec;
  self->set_int_value (nval, self->integer ());
  return self;
}

SHELL_VAR *
Shell::get_seconds (SHELL_VAR *var)
{
  time_t time_since_start;
  struct timeval tv;

  gettimeofday (&tv, nullptr);
  time_since_start = tv.tv_sec - shell_start_time;
  var->set_int_value (seconds_value_assigned + time_since_start, true);
  return var;
}

SHELL_VAR *
Shell::init_seconds_var ()
{
  SHELL_VAR *v;

  v = find_variable ("SECONDS");
  const char *v_str = v ? v->str_value ()->c_str () : nullptr;
  if (v)
    if (!legal_number (v_str, &seconds_value_assigned))
      seconds_value_assigned = 0;

  INIT_DYNAMIC_VAR ("SECONDS", v_str, &Shell::get_seconds,
                    &Shell::assign_seconds);
  return v;
}

/* Functions for $RANDOM and $SRANDOM */

SHELL_VAR *
Shell::assign_random (SHELL_VAR *self, const string &value, arrayind_t,
                      const string &)
{
  int64_t seedval;
  bool expok;

  if (self->integer ())
    seedval = evalexp (value, EXP_NOFLAGS, &expok);
  else
    expok = legal_number (value.c_str (), &seedval);

  if (!expok)
    return self;

  sbrand (static_cast<uint64_t> (seedval));
  if (subshell_environment)
    seeded_subshell = getpid ();

  self->set_int_value (seedval, self->integer ());
  return self;
}

int
Shell::get_random_number ()
{
  int rv, pid;

  /* Reset for command and process substitution. */
  pid = getpid ();
  if (subshell_environment && seeded_subshell != pid)
    {
      seedrand ();
      seeded_subshell = pid;
    }

  do
    rv = brand ();
  while (rv == last_random_value);

  return last_random_value = rv;
}

SHELL_VAR *
Shell::get_random (SHELL_VAR *var)
{
  var->set_int_value (get_random_number (), true);
  return var;
}

SHELL_VAR *
Shell::get_urandom (SHELL_VAR *var)
{
  uint32_t rv = get_urandom32 ();

  var->set_int_value (rv, true);
  return var;
}

SHELL_VAR *
Shell::assign_lineno (SHELL_VAR *var, const string &value, arrayind_t,
                      const string &)
{
  int64_t new_value;

  if (value.empty () || !legal_number (value.c_str (), &new_value))
    new_value = 0;

  line_number = static_cast<int> (new_value);
  var->set_int_value (line_number, var->integer ());
  return var;
}

/* Function which returns the current line number. */
SHELL_VAR *
Shell::get_lineno (SHELL_VAR *var)
{
  int ln = executing_line_number ();

  var->set_int_value (ln, false);
  return var;
}

SHELL_VAR *
Shell::assign_subshell (SHELL_VAR *var, const string &value, arrayind_t,
                        const string &)
{
  int64_t new_value;

  if (value.empty () || !legal_number (value.c_str (), &new_value))
    new_value = 0;

  subshell_level = static_cast<int> (new_value);
  return var;
}

SHELL_VAR *
Shell::get_subshell (SHELL_VAR *var)
{
  var->set_int_value (subshell_level, false);
  return var;
}

SHELL_VAR *
Shell::get_epochseconds (SHELL_VAR *var)
{
  int64_t now;

  now = NOW;
  var->set_int_value (now, false);
  return var;
}

SHELL_VAR *
Shell::get_epochrealtime (SHELL_VAR *var)
{
  char buf[32];
  struct timeval tv;

  gettimeofday (&tv, nullptr);
  snprintf (buf, sizeof (buf), "%u%c%06u", static_cast<unsigned> (tv.tv_sec),
            locale_decpoint (), static_cast<unsigned> (tv.tv_usec));

  var->set_string_value (buf);
  return var;
}

SHELL_VAR *
Shell::get_bashpid (SHELL_VAR *var)
{
  int64_t pid = static_cast<int64_t> (getpid ());
  var->set_int_value (pid, true);
  return var;
}

SHELL_VAR *
Shell::get_bash_argv0 (SHELL_VAR *var)
{
  var->set_string_value (dollar_vars[0]);
  return var;
}

SHELL_VAR *
Shell::assign_bash_argv0 (SHELL_VAR *var, const string &value, arrayind_t,
                          const string &)
{
  if (value.empty ())
    return var;

  delete[] dollar_vars[0];
  dollar_vars[0] = savestring (value);

  delete[] allocated_shell_name;
  allocated_shell_name = savestring (value);

  shell_name = allocated_shell_name;
  return var;
}

void
Shell::set_argv0 ()
{
  SHELL_VAR *v;

  v = find_variable ("BASH_ARGV0");
  if (v && v->imported ())
    assign_bash_argv0 (v, v->str_value ()->c_str (), 0, nullptr);
}

SHELL_VAR *
Shell::get_bash_command (SHELL_VAR *var)
{
  var->set_string_value (the_printed_command_except_trap);
  return var;
}

#if defined(HISTORY)
SHELL_VAR *
Shell::get_histcmd (SHELL_VAR *var)
{
  /* Do the same adjustment here we do in parser.cc:prompt_history_number,
     assuming that we are in one of two states: decoding this as part of
     the prompt string, in which case we do not want to assume that the
     command has been saved to the history and the history number incremented,
     or the expansion is part of the current command being executed and has
     already been saved to history and the history number incremented.
     Right now we use EXECUTING as the determinant. */
  int64_t n = static_cast<int64_t> (history_number () - executing);

  var->set_int_value (n, false);
  return var;
}
#endif

#if defined(READLINE)
/* When this function returns, VAR->value points to newed memory. */
SHELL_VAR *
Shell::get_comp_wordbreaks (SHELL_VAR *var)
{
  /* If we don't have anything yet, assign a default value. */
  if (rl_completer_word_break_characters == nullptr
      && !bash_readline_initialized)
    enable_hostname_completion (perform_hostname_completion);

  var->set_string_value (rl_completer_word_break_characters);
  return var;
}

/* When this function returns, rl_completer_word_break_characters points to
   newed memory. */
SHELL_VAR *
Shell::assign_comp_wordbreaks (SHELL_VAR *self, const string &value,
                               arrayind_t, const string &)
{
  if (rl_completer_word_break_characters
      && rl_completer_word_break_characters != rl_basic_word_break_characters)
    delete[] rl_completer_word_break_characters;

  rl_completer_word_break_characters = savestring (value);
  return self;
}
#endif /* READLINE */

#if defined(PUSHD_AND_POPD) && defined(ARRAY_VARS)
SHELL_VAR *
Shell::assign_dirstack (SHELL_VAR *self, const string &value, arrayind_t ind,
                        const string &)
{
  set_dirstack_element (ind, 1, value);
  return self;
}

SHELL_VAR *
Shell::get_dirstack (SHELL_VAR *self)
{
  WORD_LIST *l = get_directory_stack (0);
  ARRAY *a = array_from_word_list (l);
  delete self->array_value ();
  delete l;
  self->set_array_value (a);
  return self;
}
#endif /* PUSHD AND POPD && ARRAY_VARS */

#if defined(ARRAY_VARS)
/* We don't want to initialize the group set with a call to getgroups()
   unless we're asked to, but we only want to do it once. */
SHELL_VAR *
Shell::get_groupset (SHELL_VAR *self)
{
  if (group_vector == nullptr)
    {
      get_group_list ();
      ARRAY *a = self->array_value ();

      size_t num_groups = group_vector->size ();
      for (size_t i = 0; i < num_groups; i++)
        a->insert (static_cast<arrayind_t> (i), (*group_vector)[i]);
    }
  return self;
}

#if defined(DEBUGGER)
SHELL_VAR *
Shell::get_bashargcv (SHELL_VAR *self)
{
  static bool self_semaphore = false;

  /* Backwards compatibility: if we refer to BASH_ARGV or BASH_ARGC at the
     top level without enabling debug mode, and we don't have an instance
     of the variable set, initialize the arg arrays.
     This will already have been done if debugging_mode != 0. */
  if (!self_semaphore && variable_context == 0
      && debugging_mode == 0) /* don't do it for shell functions */
    {
      self_semaphore = true;
      init_bash_argv ();
      self_semaphore = false;
    }
  return self;
}
#endif

SHELL_VAR *
Shell::build_hashcmd (SHELL_VAR *self)
{
  HASH_TABLE<PATH_DATA> *h = self->phash_value ();
  if (h)
    delete h;

  if (hashed_filenames == nullptr || hashed_filenames->size () == 0)
    {
      self->set_phash_value (nullptr);
      return self;
    }

  h = new HASH_TABLE<PATH_DATA> (*hashed_filenames);
  self->set_phash_value (h);
  return self;
}

SHELL_VAR *
Shell::get_hashcmd (SHELL_VAR *self)
{
  build_hashcmd (self);
  return self;
}

SHELL_VAR *
Shell::assign_hashcmd (SHELL_VAR *self, const string &value, arrayind_t,
                       const string &key)
{
#if defined(RESTRICTED_SHELL)
  if (restricted)
    {
      if (value.find ('/') != string::npos)
        {
          sh_restricted (value.c_str ());
          return nullptr;
        }

      /* If we are changing the hash table in a restricted shell, make sure the
         target pathname can be found using a $PATH search. */
      char *full_path = find_user_command (value);
      if (full_path == nullptr || *full_path == 0
          || !executable_file (full_path))
        {
          sh_notfound (value.c_str ());
          delete[] full_path;
          return nullptr;
        }
      delete[] full_path;
    }
#endif

  phash_insert (key, value, false, 0);
  return build_hashcmd (self);
}

#if defined(ALIAS)
SHELL_VAR *
Shell::build_aliasvar (SHELL_VAR *self)
{
  HASH_TABLE<alias_t> *h = self->alias_hash_value ();
  if (h)
    delete h;

  if (aliases == nullptr || aliases->size () == 0)
    {
      self->set_alias_hash_value (nullptr);
      return self;
    }

  h = new HASH_TABLE<alias_t> (*aliases);
  self->set_alias_hash_value (h);
  return self;
}

SHELL_VAR *
Shell::assign_aliasvar (SHELL_VAR *self, const string &value, arrayind_t,
                        const string &key)
{
  if (!legal_alias_name (key))
    {
      report_error (_ ("`%s': invalid alias name"), key.c_str ());
      return self;
    }
  add_alias (key, value);
  return build_aliasvar (self);
}
#endif /* ALIAS */
#endif /* ARRAY_VARS */

/* If ARRAY_VARS is not defined, this just returns the name of any
   currently-executing function.  If we have arrays, it's a call stack. */
SHELL_VAR *
Shell::get_funcname (SHELL_VAR *self)
{
#if !defined(ARRAY_VARS)
  if (variable_context && this_shell_function)
    return set_string_value (self, this_shell_function->name, 0);
#endif
  return self;
}

void
Shell::make_funcname_visible (bool on_or_off)
{
  SHELL_VAR *v = find_variable ("FUNCNAME");
  if (v == nullptr || v->dynamic_value == nullptr)
    return;

  if (on_or_off)
    v->unset_attr (att_invisible);
  else
    v->set_attr (att_invisible);
}

SHELL_VAR *
Shell::init_funcname_var ()
{
  SHELL_VAR *v = find_variable ("FUNCNAME");
  if (v)
    return v;

#if defined(ARRAY_VARS)
  INIT_DYNAMIC_ARRAY_VAR ("FUNCNAME", &Shell::get_funcname,
                          &Shell::null_array_assign);
#else
  INIT_DYNAMIC_VAR ("FUNCNAME", nullptr, &Shell::get_funcname,
                    &Shell::null_assign);
#endif

  v->set_attr (att_invisible | att_noassign);
  return v;
}

void
Shell::initialize_dynamic_variables ()
{
  SHELL_VAR *v = init_seconds_var ();

  INIT_DYNAMIC_VAR ("BASH_ARGV0", string (), &Shell::get_bash_argv0,
                    &Shell::assign_bash_argv0);

  INIT_DYNAMIC_VAR ("BASH_COMMAND", string (), &Shell::get_bash_command,
                    nullptr);
  INIT_DYNAMIC_VAR ("BASH_SUBSHELL", string (), &Shell::get_subshell,
                    &Shell::assign_subshell);

  INIT_DYNAMIC_VAR ("RANDOM", string (), &Shell::get_random,
                    &Shell::assign_random);
  v->set_attr (att_integer);

  INIT_DYNAMIC_VAR ("SRANDOM", string (), &Shell::get_urandom, nullptr);
  v->set_attr (att_integer);

  INIT_DYNAMIC_VAR ("LINENO", string (), &Shell::get_lineno,
                    &Shell::assign_lineno);
  v->set_attr (att_regenerate);

  INIT_DYNAMIC_VAR ("BASHPID", string (), &Shell::get_bashpid,
                    &Shell::null_assign);
  v->set_attr (att_integer);

  INIT_DYNAMIC_VAR ("EPOCHSECONDS", string (), &Shell::get_epochseconds,
                    &Shell::null_assign);
  v->set_attr (att_regenerate);
  INIT_DYNAMIC_VAR ("EPOCHREALTIME", string (), &Shell::get_epochrealtime,
                    &Shell::null_assign);
  v->set_attr (att_regenerate);

#if defined(HISTORY)
  INIT_DYNAMIC_VAR ("HISTCMD", string (), &Shell::get_histcmd, nullptr);
  v->set_attr (att_integer);
#endif

#if defined(READLINE)
  INIT_DYNAMIC_VAR ("COMP_WORDBREAKS", string (), &Shell::get_comp_wordbreaks,
                    &Shell::assign_comp_wordbreaks);
#endif

#if defined(PUSHD_AND_POPD) && defined(ARRAY_VARS)
  v = init_dynamic_array_var ("DIRSTACK", &Shell::get_dirstack,
                              &Shell::assign_dirstack, att_noflags);
#endif /* PUSHD_AND_POPD && ARRAY_VARS */

#if defined(ARRAY_VARS)
  v = init_dynamic_array_var ("GROUPS", &Shell::get_groupset,
                              &Shell::null_array_assign, att_noassign);

#if defined(DEBUGGER)
  v = init_dynamic_array_var ("BASH_ARGC", &Shell::get_bashargcv,
                              &Shell::null_array_assign,
                              att_noassign | att_nounset);

  v = init_dynamic_array_var ("BASH_ARGV", &Shell::get_bashargcv,
                              &Shell::null_array_assign,
                              att_noassign | att_nounset);
#endif /* DEBUGGER */

  v = init_dynamic_array_var ("BASH_SOURCE", &Shell::get_self,
                              &Shell::null_array_assign,
                              att_noassign | att_nounset);

  v = init_dynamic_array_var ("BASH_LINENO", &Shell::get_self,
                              &Shell::null_array_assign,
                              att_noassign | att_nounset);

  v = init_dynamic_assoc_var ("BASH_CMDS", &Shell::get_hashcmd,
                              &Shell::assign_hashcmd, att_nofree);

#if defined(ALIAS)
  v = init_dynamic_assoc_var ("BASH_ALIASES", &Shell::build_aliasvar,
                              &Shell::assign_aliasvar, att_nofree);
#endif
#endif

  v = init_funcname_var ();
}

/* **************************************************************** */
/*								    */
/*		Retrieving variables and values			    */
/*								    */
/* **************************************************************** */

/* Look up the variable entry named NAME. If SEARCH_TEMPENV is non-zero,
   then also search the temporarily built list of exported variables.
   The lookup order is:
        temporary_env
        shell_variables list
*/
SHELL_VAR *
Shell::find_variable_internal (string_view name, find_var_flags flags)
{
  bool force_tempenv = (flags & FV_FORCETEMPENV);

  /* If explicitly requested, first look in the temporary environment for
     the variable.  This allows constructs such as "foo=x eval 'echo $foo'"
     to get the `exported' value of $foo.  This happens if we are executing
     a function or builtin, or if we are looking up a variable in a
     "subshell environment". */
  bool search_tempenv
      = force_tempenv || (!expanding_redir && subshell_environment);

  SHELL_VAR *var = nullptr;
  if (search_tempenv && temporary_env)
    var = hash_lookup (name, temporary_env);

  if (var == nullptr)
    {
      if ((flags & FV_SKIPINVISIBLE) == 0)
        var = var_lookup (name, shell_variables);
      else
        {
          VAR_CONTEXT *vc;
          /* essentially var_lookup expanded inline so we can check for
             att_invisible */
          for (vc = shell_variables; vc; vc = vc->down)
            {
              var = hash_lookup (name, &vc->table);
              if (var && var->invisible ())
                var = nullptr;
              if (var)
                break;
            }
        }
    }

  if (var == nullptr)
    return nullptr;

  return var->dynamic_value ? ((*this).*(var->dynamic_value)) (var) : var;
}

/* Look up and resolve the chain of nameref variables starting at V all the
   way to nullptr or non-nameref. */
SHELL_VAR *
Shell::find_variable_nameref (SHELL_VAR *v)
{
  int level = 0;
  SHELL_VAR *orig = v;

  while (v && v->nameref ())
    {
      level++;
      if (level > NAMEREF_MAX)
        return nullptr; /* error message here? */

      string *newname = v->str_value ();
      if (newname == nullptr || newname->empty ())
        return nullptr;

      SHELL_VAR *oldv = v;

      find_var_flags flags = FV_NOFLAGS;
      if (!expanding_redir && (assigning_in_environment || executing_builtin))
        flags = FV_FORCETEMPENV;

      /* We don't handle array subscripts here. */
      v = find_variable_internal (*newname, flags);
      if (v == orig || v == oldv)
        {
          internal_warning (_ ("%s: circular name reference"),
                            orig->name ().c_str ());
#if 1
          /* XXX - provisional change - circular refs go to
             global scope for resolution, without namerefs. */
          if (variable_context && v->context)
            return find_global_variable_noref (v->name ().c_str ());
          else
#endif
            return nullptr;
        }
    }
  return v;
}

// Resolve the chain of nameref variables for NAME.  XXX - could change later
SHELL_VAR *
Shell::find_variable_last_nameref (string_view name, int vflags)
{
  SHELL_VAR *v, *nv;

  nv = v = find_variable_noref (name);
  int level = 0;
  while (v && v->nameref ())
    {
      level++;
      if (level > NAMEREF_MAX)
        return nullptr; /* error message here? */

      string *newname = v->str_value ();
      if (newname == nullptr || newname->empty ())
        return (vflags && v->invisible ()) ? v : nullptr;

      nv = v;
      find_var_flags flags = FV_NOFLAGS;
      if (!expanding_redir && (assigning_in_environment || executing_builtin))
        flags = FV_FORCETEMPENV;

      /* We don't accommodate array subscripts here. */
      v = find_variable_internal (*newname, flags);
    }
  return nv;
}

// Resolve the chain of nameref variables for NAME.  XXX - could change later
SHELL_VAR *
Shell::find_global_variable_last_nameref (string_view name, int vflags)
{
  SHELL_VAR *v, *nv;
  int level;

  nv = v = find_global_variable_noref (name);
  level = 0;
  while (v && v->nameref ())
    {
      level++;
      if (level > NAMEREF_MAX)
        return nullptr; /* error message here? */

      string *newname = v->str_value ();
      if (newname == nullptr || newname->empty ())
        return (vflags && v->invisible ()) ? v : nullptr;

      nv = v;

      /* We don't accommodate array subscripts here. */
      v = find_global_variable_noref (*newname);
    }
  return nv;
}

SHELL_VAR *
Shell::find_nameref_at_context (SHELL_VAR *v, VAR_CONTEXT *vc)
{
  SHELL_VAR *nv = v;
  int level = 1;
  while (nv && nv->nameref ())
    {
      level++;
      if (level > NAMEREF_MAX)
        throw nameref_maxloop_value ();

      string *newname = v->str_value ();
      if (newname == nullptr || newname->empty ())
        return nullptr;

      SHELL_VAR *nv2 = hash_lookup (*newname, &vc->table);
      if (nv2 == nullptr)
        break;

      nv = nv2;
    }
  return nv;
}

/* Do nameref resolution from the VC, which is the local context for some
   function or builtin, `up' the chain to the global variables context.  If
   NVCP is not nullptr, return the variable context where we finally ended the
   nameref resolution (so the bind_variable_internal can use the correct
   variable context and hash table). */
SHELL_VAR *
Shell::find_variable_nameref_context (SHELL_VAR *v, VAR_CONTEXT *vc,
                                      VAR_CONTEXT **nvcp)
{
  SHELL_VAR *nv, *nv2;
  VAR_CONTEXT *nvc;

  /* Look starting at the current context all the way `up' */
  for (nv = v, nvc = vc; nvc; nvc = nvc->down)
    {
      // this may throw nameref_maxloop_value ()
      nv2 = find_nameref_at_context (nv, nvc);
      if (nv2 == nullptr)
        continue;

      nv = nv2;

      if (*nvcp)
        *nvcp = nvc;

      if (!nv->nameref ())
        break;
    }

  return nv->nameref () ? nullptr : nv;
}

/* Do nameref resolution from the VC, which is the local context for some
   function or builtin, `up' the chain to the global variables context.  If
   NVCP is not nullptr, return the variable context where we finally ended the
   nameref resolution (so the bind_variable_internal can use the correct
   variable context and hash table). */
SHELL_VAR *
Shell::find_variable_last_nameref_context (SHELL_VAR *v, VAR_CONTEXT *vc,
                                           VAR_CONTEXT **nvcp)
{
  SHELL_VAR *nv, *nv2;
  VAR_CONTEXT *nvc;

  /* Look starting at the current context all the way `up' */
  for (nv = v, nvc = vc; nvc; nvc = nvc->down)
    {
      // this may throw nameref_maxloop_value ()
      nv2 = find_nameref_at_context (nv, nvc);
      if (nv2 == nullptr)
        continue;

      nv = nv2;

      if (*nvcp)
        *nvcp = nvc;
    }
  return nv->nameref () ? nv : nullptr;
}

SHELL_VAR *
Shell::find_var_nameref_for_create (const string &name, int flags)
{
  /* See if we have a nameref pointing to a variable that hasn't been
     created yet. */
  SHELL_VAR *var = find_variable_last_nameref (name, 1);

  if ((flags & 1) && var && var->nameref () && var->invisible ())
    {
      internal_warning (_ ("%s: removing nameref attribute"), name.c_str ());
      var->unset_attr (att_nameref);
    }

  if (var && var->nameref ())
    {
      if (!var->str_value () || !legal_identifier (*(var->str_value ())))
        {
          sh_invalidid (var->str_value () ? var->str_value ()->c_str () : "");
          throw invalid_nameref_value ();
        }
    }

  return var;
}

SHELL_VAR *
Shell::find_var_nameref_for_assignment (const string &name, int)
{
  /* See if we have a nameref pointing to a variable that hasn't been
     created yet. */
  SHELL_VAR *var = find_variable_last_nameref (name, 1);

  if (var && var->nameref () && var->invisible ()) /* XXX - flags */
    {
      internal_warning (_ ("%s: removing nameref attribute"), name.c_str ());
      var->unset_attr (att_nameref);
    }

  if (var && var->nameref ())
    {
      if (!var->str_value ()
          || !valid_nameref_value (*(var->str_value ()), VA_NOEXPAND))
        {
          sh_invalidid (var->str_value () ? var->str_value ()->c_str () : "");
          throw invalid_nameref_value ();
        }
    }

  return var;
}

/* If find_variable (name) returns nullptr, check that it's not a nameref
   referencing a variable that doesn't exist. If it is, return the new
   name. If not, return the original name. Kind of like the previous
   function, but dealing strictly with names. This takes assignment flags
   so it can deal with the various assignment modes used by `declare'. */
const string &
Shell::nameref_transform_name (const string &name, assign_flags flags)
{
  SHELL_VAR *v = nullptr;

  if (flags & ASS_MKLOCAL)
    {
      v = find_variable_last_nameref (name, 1);
      /* If we're making local variables, only follow namerefs that point to
         non-existent variables at the same variable context. */
      if (v && v->context != variable_context)
        v = nullptr;
    }
  else if (flags & ASS_MKGLOBAL)
    v = (flags & ASS_CHKLOCAL) ? find_variable_last_nameref (name, 1)
                               : find_global_variable_last_nameref (name, 1);

  if (v && v->nameref () && v->str_value ()
      && valid_nameref_value (*v->str_value (), VA_NOEXPAND))
    return *v->str_value ();

  return name;
}

/* Find a variable, forcing a search of the temporary environment first */
SHELL_VAR *
Shell::find_variable_tempenv (string_view name)
{
  SHELL_VAR *var = find_variable_internal (name, FV_FORCETEMPENV);

  if (var && var->nameref ())
    var = find_variable_nameref (var);

  return var;
}

/* Find a variable, not forcing a search of the temporary environment first */
SHELL_VAR *
Shell::find_variable_notempenv (string_view name)
{
  SHELL_VAR *var = find_variable_internal (name, FV_NOFLAGS);

  if (var && var->nameref ())
    var = find_variable_nameref (var);

  return var;
}

SHELL_VAR *
Shell::find_global_variable (string_view name)
{
  SHELL_VAR *var = var_lookup (name, global_variables);
  if (var && var->nameref ())
    var = find_variable_nameref (var); /* XXX - find_global_variable_noref? */

  if (var == nullptr)
    return nullptr;

  return var->dynamic_value ? ((*this).*(var->dynamic_value)) (var) : var;
}

SHELL_VAR *
Shell::find_global_variable_noref (string_view name)
{
  SHELL_VAR *var = var_lookup (name, global_variables);

  if (var == nullptr)
    return nullptr;

  return var->dynamic_value ? ((*this).*(var->dynamic_value)) (var) : var;
}

SHELL_VAR *
Shell::find_shell_variable (string_view name)
{
  SHELL_VAR *var;

  var = var_lookup (name, shell_variables);
  if (var && var->nameref ())
    var = find_variable_nameref (var);

  if (var == nullptr)
    return nullptr;

  return var->dynamic_value ? ((*this).*(var->dynamic_value)) (var) : var;
}

/* Look up the variable entry named NAME.  Returns the entry or nullptr. */
SHELL_VAR *
Shell::find_variable (string_view name)
{
  last_table_searched = nullptr;
  find_var_flags flags = FV_NOFLAGS;

  if (!expanding_redir && (assigning_in_environment || executing_builtin))
    flags = FV_FORCETEMPENV;

  SHELL_VAR *v = find_variable_internal (name, flags);
  if (v && v->nameref ())
    v = find_variable_nameref (v);

  return v;
}

/* Find the first instance of NAME in the variable context chain; return first
   one found without att_invisible set; return 0 if no non-invisible instances
   found. */
SHELL_VAR *
Shell::find_variable_no_invisible (string_view name)
{
  last_table_searched = nullptr;
  find_var_flags flags = FV_SKIPINVISIBLE;

  if (!expanding_redir && (assigning_in_environment || executing_builtin))
    flags |= FV_FORCETEMPENV;

  SHELL_VAR *v = find_variable_internal (name, flags);
  if (v && v->nameref ())
    v = find_variable_nameref (v);

  return v;
}

/* Find the first instance of NAME in the variable context chain; return first
   one found even if att_invisible set. */
SHELL_VAR *
Shell::find_variable_for_assignment (string_view name)
{
  last_table_searched = nullptr;
  find_var_flags flags = FV_NOFLAGS;

  if (!expanding_redir && (assigning_in_environment || executing_builtin))
    flags = FV_FORCETEMPENV;

  SHELL_VAR *v = find_variable_internal (name, flags);
  if (v && v->nameref ())
    v = find_variable_nameref (v);

  return v;
}

SHELL_VAR *
Shell::find_variable_noref (string_view name)
{
  find_var_flags flags = FV_NOFLAGS;

  if (!expanding_redir && (assigning_in_environment || executing_builtin))
    flags = FV_FORCETEMPENV;

  SHELL_VAR *v = find_variable_internal (name, flags);
  return v;
}

/* Return the value of VAR. VAR is assumed to have been the result of a
   lookup without any subscript, if arrays are compiled into the shell. */
string *
Shell::get_variable_value (SHELL_VAR *var)
{
  if (var == nullptr)
    return nullptr;
#if defined(ARRAY_VARS)
  else if (var->array ())
    {
      ARRAY *array = var->array_value ();
      return array->reference (array->first_index ());
    }
  else if (var->assoc ())
    {
      SHELL_VAR *sv = var->assoc_value ()->find ("0");
      return sv ? sv->str_value () : nullptr;
    }
#endif
  else
    return var->str_value ();
}

/* Return the string value of a variable.  Return nullptr if the variable
   doesn't exist.  Don't cons a new string.  This is a potential memory
   leak if the variable is found in the temporary environment, but doesn't
   leak in practice.  Since functions and variables have separate name
   spaces, returns nullptr if var_name is a shell function only. */
string *
Shell::get_string_value (const string &var_name)
{
  SHELL_VAR *var = find_variable (var_name);
  return (var) ? get_variable_value (var) : nullptr;
}

/* This is present for use by the tilde and readline libraries. */
const string *
Shell::sh_get_env_value (const string &v)
{
  return get_string_value (v);
}

/* **************************************************************** */
/*								    */
/*		  Creating and setting variables		    */
/*								    */
/* **************************************************************** */

/* Set NAME to VALUE if NAME has no value. */
SHELL_VAR *
Shell::set_if_not (const string &name, const string &value)
{
  SHELL_VAR *v;

  if (shell_variables == nullptr)
    create_variable_tables ();

  v = find_variable (name);
  if (v == nullptr)
    v = bind_variable_internal (name, value, &global_variables->table,
                                HS_NOSRCH, ASS_NOFLAGS);
  return v;
}

/* Create a local variable referenced by NAME. */
SHELL_VAR *
Shell::make_local_variable (const string &name, mkloc_var_flags flags)
{
  /* We don't want to follow the nameref chain when making local variables; we
     just want to create them. */
  SHELL_VAR *old_ref = find_variable_noref (name);
  if (old_ref && !old_ref->nameref ())
    old_ref = nullptr;

  /* local foo; local foo;  is a no-op. */
  SHELL_VAR *old_var = find_variable (name);
  if (old_ref == nullptr && old_var && old_var->local ()
      && old_var->context == variable_context)
    return old_var;

  /* local -n foo; local -n foo;  is a no-op. */
  if (old_ref && old_ref->local () && old_ref->context == variable_context)
    return old_ref;

  /* From here on, we want to use the refvar, not the variable it references */
  if (old_ref)
    old_var = old_ref;

  bool was_tmpvar = old_var && old_var->tempvar ();
  VAR_CONTEXT *vc;
  SHELL_VAR *new_var;
  string *old_value;

  /* If we're making a local variable in a shell function, the temporary env
     has already been merged into the function's variable context stack.  We
     can assume that a temporary var in the same context appears in the same
     VAR_CONTEXT and can safely be returned without creating a new variable
     (which results in duplicate names in the same VAR_CONTEXT->table */

  /* We can't just test tmpvar_p because variables in the temporary env given
     to a shell function appear in the function's local variable VAR_CONTEXT
     but retain their tempvar attribute.  We want temporary variables that are
     found in temporary_env, hence the test for last_table_searched, which is
     set in hash_lookup and only (so far) checked here. */
  if (was_tmpvar && old_var->context == variable_context
      && last_table_searched != temporary_env)
    {
      old_var->unset_attr (att_invisible); /* XXX */

      /* We still want to flag this variable as local, though, and set things
         up so that it gets treated as a local variable. */
      new_var = old_var;

      /* Since we found the variable in a temporary environment, this will
         succeed. */
      for (vc = shell_variables; vc; vc = vc->down)
        if (vc->func_env () && vc->scope == variable_context)
          break;

      goto set_local_var_flags;
    }

  /* If we want to change to "inherit the old variable's value" semantics,
     here is where to save the old value. */
  old_value = was_tmpvar ? old_var->str_value () : nullptr;

  for (vc = shell_variables; vc; vc = vc->down)
    if (vc->func_env () && vc->scope == variable_context)
      break;

  if (vc == nullptr)
    {
      internal_error (
          _ ("make_local_variable: no function context at current scope"));
      return nullptr;
    }

  /* Since this is called only from the local/declare/typeset code, we can
     call builtin_error here without worry (of course, it will also work
     for anything that sets this_command_name).  Variables with the `noassign'
     attribute may not be made local.  The test against old_var's context
     level is to disallow local copies of readonly global variables (since I
     believe that this could be a security hole).  Readonly copies of calling
     function local variables are OK. */
  if (old_var
      && (old_var->noassign ()
          || (old_var->readonly () && old_var->context == 0)))
    {
      if (old_var->readonly ())
        sh_readonly (name.c_str ());
      else if (old_var->noassign ())
        builtin_error (_ ("%s: variable may not be assigned value"),
                       name.c_str ());
#if 0
      /* Let noassign variables through with a warning */
      if (old_var->readonly ())
#endif
      return nullptr;
    }

  if (old_var == nullptr)
    new_var = new SHELL_VAR (name, &vc->table);
  else
    {
      new_var = new SHELL_VAR (name, &vc->table);

      /* If we found this variable in one of the temporary environments,
         inherit its value.  Watch to see if this causes problems with
         things like `x=4 local x'. XXX - see above for temporary env
         variables with the same context level as variable_context */
      /* XXX - we should only do this if the variable is not an array. */
      /* If we want to change the local variable semantics to "inherit
         the old variable's value" here is where to set it.  And we would
         need to use copy_variable (currently unused) to do it for all
         possible variable values. */
      if (was_tmpvar)
        new_var->set_string_value (*old_value);
      else if (localvar_inherit || (flags & MKLOC_INHERIT))
        {
          /* This may not make sense for nameref variables that are shadowing
             variables with the same name, but we don't know that yet. */
#if defined(ARRAY_VARS)
          if (old_var->assoc ())
            new_var->set_assoc_value (
                new ASSOC_ARRAY (*old_var->assoc_value ()));
          else if (old_var->array ())
            new_var->set_array_value (new ARRAY (*old_var->array_value ()));
          else if (old_var->str_value ())
#else
          if (old_var->str_value ())
#endif
            new_var->set_string_value (*old_var->str_value ());
          else
            new_var->set_string_value (string_view ());
        }

      if (localvar_inherit || (flags & MKLOC_INHERIT))
        {
          /* It doesn't make sense to inherit the nameref attribute */
          new_var->attributes = old_var->attributes & ~att_nameref;
          new_var->dynamic_value = old_var->dynamic_value;
          new_var->assign_func = old_var->assign_func;
        }
      else
        /* We inherit the export attribute, but no others. */
        new_var->attributes
            = old_var->exported () ? att_exported : att_noflags;
    }

set_local_var_flags:
  vc->flags |= VC_HASLOCAL;

  new_var->context = variable_context;
  new_var->set_attr (att_local);

  if (ifsname (name))
    setifs (new_var);

  /* value_cell will be nullptr if localvar_inherit == 0 or there was no old
     variable with the same name or the old variable was invisible */
  if (!was_tmpvar && new_var->str_value () == nullptr)
    new_var->set_attr (att_invisible); /* XXX */

  return new_var;
}

#if defined(ARRAY_VARS)
SHELL_VAR *
Shell::make_new_array_variable (const string &name)
{
  SHELL_VAR *entry = new SHELL_VAR (name, &global_variables->table);
  ARRAY *array = new ARRAY ();

  entry->set_array_value (array);
  entry->set_attr (att_array);
  return entry;
}

SHELL_VAR *
Shell::make_local_array_variable (const string &name, mkloc_var_flags flags)
{
  bool assoc_ok = flags & MKLOC_ASSOCOK;

  SHELL_VAR *var
      = make_local_variable (name, flags & MKLOC_INHERIT); /* XXX for now */

  /* If ASSOC_OK is non-zero, assume that we are ok with letting an assoc
     variable return to the caller without converting it. The caller will
     either flag an error or do the conversion itself. */
  if (var == nullptr || var->array () || (assoc_ok && var->assoc ()))
    return var;

  /* Validate any value we inherited from a variable instance at a previous
     scope and discard anything that's invalid. */
  if (localvar_inherit && var->assoc ())
    {
      internal_warning (_ ("%s: cannot inherit value from incompatible type"),
                        name.c_str ());

      var->unset_attr (att_assoc);
      delete var;

      ARRAY *array = new ARRAY ();
      var->set_array_value (array);
    }
  else if (localvar_inherit)
    var = convert_var_to_array (var); /* XXX */
  else
    {
      delete var;
      ARRAY *array = new ARRAY ();
      var->set_array_value (array);
    }

  var->set_attr (att_array);
  return var;
}

SHELL_VAR *
Shell::make_local_assoc_variable (const string &name, mkloc_var_flags flags)
{
  bool array_ok = flags & MKLOC_ARRAYOK;
  SHELL_VAR *var = make_local_variable (name, flags & MKLOC_INHERIT);

  /* If ARRAY_OK is true, assume that we are ok with letting an array
     variable return to the caller without converting it. The caller will
     either flag an error or do the conversion itself. */
  if (var == nullptr || var->assoc () || (array_ok && var->array ()))
    return var;

  /* Validate any value we inherited from a variable instance at a previous
     scope and discard anything that's invalid. */
  if (localvar_inherit && var->array ())
    {
      internal_warning (_ ("%s: cannot inherit value from incompatible type"),
                        name.c_str ());
      var->unset_attr (att_array);
      delete var;
      ASSOC_ARRAY *hash = new ASSOC_ARRAY ();
      var->set_assoc_value (hash);
    }
  else if (localvar_inherit)
    var = convert_var_to_assoc (var); /* XXX */
  else
    {
      delete var;
      ASSOC_ARRAY *hash = new ASSOC_ARRAY ();
      var->set_assoc_value (hash);
    }

  var->set_attr (att_assoc);
  return var;
}
#endif

string *
Shell::make_variable_value (SHELL_VAR *var, const string &value,
                            assign_flags flags)
{
  /* If this variable has had its type set to integer (via `declare -i'),
     then do expression evaluation on it and store the result.  The
     functions in expr.c (evalexp()) and bind_int_variable() are responsible
     for turning off the integer flag if they don't want further
     evaluation done.  Callers that find it inconvenient to do this can set
     the ASS_NOEVAL flag.  For the special case of arithmetic expression
     evaluation, the caller can set ASS_NOTHROW to avoid jumping out to
     top_level. */
  if ((flags & ASS_NOEVAL) == 0 && var->integer ())
    {
      int64_t lval = 0;
      bool expok = false;
      if (flags & ASS_APPEND)
        {
          // ksh93 seems to do this
          const string *oval = var->str_value ();
          if (oval)
            lval = evalexp (*oval, EXP_NOFLAGS, &expok);

          if (!expok)
            {
              if (flags & ASS_NOTHROW)
                goto make_value;
              else
                {
                  top_level_cleanup ();
                  throw bash_exception (DISCARD);
                }
            }
        }
      int64_t rval = evalexp (value, EXP_NOFLAGS, &expok);
      if (!expok)
        {
          if (flags & ASS_NOTHROW)
            goto make_value;
          else
            {
              top_level_cleanup ();
              throw bash_exception (DISCARD);
            }
        }

      // This can be fooled if the variable's value changes while evaluating
      // `rval'.  We can change it if we move the evaluation of lval to here.
      if (flags & ASS_APPEND)
        rval += lval;

      return new string (itos (rval));
    }
#if defined(CASEMOD_ATTRS)
  else if ((flags & ASS_NOEVAL) == 0
           && (var->capcase () || var->uppercase () || var->lowercase ()))
    {
      string retval;
      if (flags & ASS_APPEND)
        {
          const string *oval = get_variable_value (var);
          if (oval)
            {
              size_t olen = oval->size ();
              retval.reserve (olen + value.size ());
              retval = *oval;
              if (olen)
                retval += value;
            }
          else
            retval = value;
        }
      else
        retval = value;

      sh_modcase_flags op
          = var->capcase () ? CASE_CAPITALIZE
                            : (var->uppercase () ? CASE_UPPER : CASE_LOWER);

      return new string (sh_modcase (retval, nullptr, op));
    }
#endif /* CASEMOD_ATTRS */
  else if (!value.empty ())
    {
    make_value:
      if (flags & ASS_APPEND)
        {
          const string *oval = get_variable_value (var);
          if (oval)
            {
              size_t olen = oval->size ();
              string *retval = new string ();
              retval->reserve (olen + value.size ());
              *retval = *oval;
              if (olen)
                *retval += value;
              return retval;
            }
          else
            return new string (value);
        }
      else
        return new string (value);
    }
  else
    return new string ();
}

/* Bind a variable NAME to VALUE in the HASH_TABLE TABLE, which may be the
   temporary environment (but usually is not).  HFLAGS controls how NAME
   is looked up in TABLE; AFLAGS controls how VALUE is assigned */
SHELL_VAR *
Shell::bind_variable_internal (string_view name, const string &value,
                               HASH_TABLE<SHELL_VAR> *table,
                               hsearch_flags hflags, assign_flags aflags)
{
  SHELL_VAR *entry
      = (hflags & HS_NOSRCH) ? nullptr : hash_lookup (name, table);

  /* Follow the nameref chain here if this is the global variables table */
  if (entry && entry->nameref () && !entry->invisible ()
      && table == &global_variables->table)
    {
      entry = find_global_variable (entry->name ());
      /* Let's see if we have a nameref referencing a variable that hasn't yet
         been created. */
      if (entry == nullptr)
        entry = find_variable_last_nameref (name, 0); /* XXX */

      if (entry == nullptr) /* just in case */
        return entry;
    }

  /* The first clause handles `declare -n ref; ref=x;' or `declare -n ref;
     declare -n ref' */
  if (entry && entry->invisible () && entry->nameref ())
    {
      if ((aflags & ASS_FORCE) == 0
          && !valid_nameref_value (value, VA_NOFLAGS))
        {
          sh_invalidid (value.c_str ());
          return nullptr;
        }
      goto assign_value;
    }
  else if (entry && entry->nameref ())
    {
      string *newval = entry->str_value (); // XXX - can't be nullptr here
      if (valid_nameref_value (*newval, VA_NOFLAGS) == 0)
        {
          sh_invalidid (newval->c_str ());
          return nullptr;
        }
#if defined(ARRAY_VARS)
      /* declare -n foo=x[2] ; foo=bar */
      if (valid_array_reference (*newval, 0))
        {
          string *tname = array_variable_name (*newval, 0, nullptr);
          SHELL_VAR *tentry;
          if (tname && (tentry = find_variable_noref (*tname))
              && tentry->nameref ())
            {
              /* nameref variables can't be arrays */
              internal_warning (_ ("%s: removing nameref attribute"),
                                tentry->name ().c_str ());

              delete[] tentry->str_value (); /* XXX - bash-4.3 compat */
              tentry->set_value (nullptr);
              tentry->unset_attr (att_nameref);
            }
          free (tname);

          /* entry == nameref variable; tentry == array variable;
             newval == x[2]; value = bar
             We don't need to call make_variable_value here, since
             assign_array_element will eventually do it itself based on
             newval and aflags. */

          entry = assign_array_element (*newval, value, aflags | ASS_NAMEREF,
                                        nullptr);
          if (entry == nullptr)
            return entry;
        }
      else
#endif
        {
          entry = make_new_variable (*newval, *table);
          entry->set_value (make_variable_value (entry, value, aflags));
        }
    }
  else if (entry == nullptr)
    {
      entry = make_new_variable (name, *table);
      entry->set_value (make_variable_value (entry, value, aflags)); /* XXX */
    }
  else if (entry->assign_func) /* array vars have assign functions now */
    {
      if ((entry->readonly () && (aflags & ASS_FORCE) == 0)
          || entry->noassign ())
        {
          if (entry->readonly ())
            err_readonly (entry->name ().c_str ());
          return entry;
        }

      entry->invalidate_exportstr ();
      const string *newval = (aflags & ASS_APPEND)
                                 ? make_variable_value (entry, value, aflags)
                                 : &value;

      if (entry->assoc ())
        entry = ((*this).*(entry->assign_func)) (entry, *newval, -1, "0");
      else if (entry->array ())
        entry = ((*this).*(entry->assign_func)) (entry, *newval, 0, nullptr);
      else
        entry = ((*this).*(entry->assign_func)) (entry, *newval, -1, nullptr);

      if (newval != &value)
        delete const_cast<string *> (newval);

      return entry;
    }
  else
    {
    assign_value:
      if ((entry->readonly () && (aflags & ASS_FORCE) == 0)
          || entry->noassign ())
        {
          if (entry->readonly ())
            err_readonly (entry->name ().c_str ());
          return entry;
        }

      /* Variables which are bound are visible. */
      entry->unset_attr (att_invisible);

      /* If we can optimize the assignment, do so and return.  Right now, we
         optimize appends to string variables. */
      if (can_optimize_assignment (entry, aflags))
        {
          entry->invalidate_exportstr ();
          (void)optimized_assignment (entry, value);

          if (mark_modified_vars)
            entry->set_attr (att_exported);

          if (entry->exported ())
            array_needs_making = true;

          return entry;
        }

      string *newval;
#if defined(ARRAY_VARS)
      if (entry->assoc () || entry->array ())
        newval = make_array_variable_value (entry, 0, "0", value, aflags);
      else
#endif
        newval = make_variable_value (entry, value, aflags); /* XXX */

      /* Invalidate any cached export string */
      entry->invalidate_exportstr ();

#if defined(ARRAY_VARS)
      /* XXX -- this bears looking at again -- XXX */
      /* If an existing array variable x is being assigned to with x=b or
         `read x' or something of that nature, silently convert it to
         x[0]=b or `read x[0]'. */
      if (entry->assoc ())
        {
          entry->assoc_value ()->insert ("0", *newval);
          delete newval;
        }
      else if (entry->array ())
        {
          entry->array_value ()->insert (0, *newval);
          delete newval;
        }
      else
#endif
        {
          delete entry->str_value ();
          entry->set_value (newval);
        }
    }

  if (mark_modified_vars)
    entry->set_attr (att_exported);

  if (entry->exported ())
    array_needs_making = true;

  return entry;
}

/* Bind a variable NAME to VALUE. If we have a temporary environment, we bind
   there first, then we bind into shell_variables. */
SHELL_VAR *
Shell::bind_variable (const string &name, const string &value,
                      assign_flags flags)
{
  SHELL_VAR *v, *nv;
  VAR_CONTEXT *vc, *nvc;

  if (shell_variables == nullptr)
    create_variable_tables ();

  /* If we have a temporary environment, look there first for the variable,
     and, if found, modify the value there before modifying it in the
     shell_variables table.  This allows sourced scripts to modify values
     given to them in a temporary environment while modifying the variable
     value that the caller sees. */
  if (temporary_env)
    bind_tempenv_variable (name, value);

  /* XXX -- handle local variables here. */
  for (vc = shell_variables; vc; vc = vc->down)
    {
      if (vc->func_env () || vc->builtin_env ())
        {
          v = hash_lookup (name, &vc->table);
          nvc = vc;
          if (v && v->nameref ())
            {
              try
                {
                  /* This starts at the context where we found the nameref. If
                     we want to start the name resolution over again at the
                     original context, this is where we need to change it */
                  nv = find_variable_nameref_context (v, vc, &nvc);
                  if (nv == nullptr)
                    {
                      nv = find_variable_last_nameref_context (v, vc, &nvc);
                      if (nv && nv->nameref ())
                        {
                          /* If this nameref variable doesn't have a value yet,
                             set the value.  Otherwise, assign using the value
                             as normal. */
                          if (nv->str_value () == nullptr)
                            return bind_variable_internal (nv->name (), value,
                                                           &nvc->table,
                                                           HS_NOFLAGS, flags);
#if defined(ARRAY_VARS)
                          else if (valid_array_reference (*nv->str_value (),
                                                          0))
                            return assign_array_element (
                                *nv->str_value (), value, flags, nullptr);
                          else
#endif
                            return bind_variable_internal (*nv->str_value (),
                                                           value, &nvc->table,
                                                           HS_NOFLAGS, flags);
                        }
                    }
                  else
                    v = nv;
                }
              catch (const nameref_maxloop_value &)
                {
                  internal_warning (_ ("%s: circular name reference"),
                                    v->str_value ()->c_str ());
                  return bind_global_variable (*v->str_value (), value, flags);
                }
            }
          if (v)
            return bind_variable_internal (v->name (), value, &nvc->table,
                                           HS_NOFLAGS, flags);
        }
    }

  /* bind_variable_internal will handle nameref resolution in this case */
  return bind_variable_internal (name, value, &global_variables->table,
                                 HS_NOFLAGS, flags);
}

/* Make VAR, a simple shell variable, have value VALUE.  Once assigned a
   value, variables are no longer invisible.  This is a duplicate of part
   of the internals of bind_variable.  If the variable is exported, or
   all modified variables should be exported, mark the variable for export
   and note that the export environment needs to be recreated. */
SHELL_VAR *
Shell::bind_variable_value (SHELL_VAR *var, const string &value,
                            assign_flags aflags)
{
  bool invis = var->invisible ();
  var->unset_attr (att_invisible);

  if (var->assign_func)
    {
      /* If we're appending, we need the old value, so use
         make_variable_value */
      string *t = (aflags & ASS_APPEND)
                      ? make_variable_value (var, value, aflags)
                      : const_cast<string *> (&value);
      ((*this).*(var->assign_func)) (var, *t, -1, string ());
      if (t != &value && t)
        delete t;
    }
  else
    {
      string *t = make_variable_value (var, value, aflags);
      if ((aflags & (ASS_NAMEREF | ASS_FORCE)) == ASS_NAMEREF
          && check_selfref (var->name (), *t))
        {
          if (variable_context)
            internal_warning (_ ("%s: circular name reference"),
                              var->name ().c_str ());
          else
            {
              internal_error (
                  _ ("%s: nameref variable self references not allowed"),
                  var->name ().c_str ());
              delete t;
              if (invis)
                var->set_attr (att_invisible); /* XXX */
              return nullptr;
            }
        }
      if ((aflags & ASS_NAMEREF) && !valid_nameref_value (*t, VA_NOFLAGS))
        {
          delete t;
          if (invis)
            var->set_attr (att_invisible); /* XXX */
          return nullptr;
        }
      var->dispose_value ();
      var->set_value (t);
    }

  var->invalidate_exportstr ();

  if (mark_modified_vars)
    var->set_attr (att_exported);

  if (var->exported ())
    array_needs_making = true;

  return var;
}

/* Bind/create a shell variable with the name LHS to the RHS.
   This creates or modifies a variable such that it is an integer.

   This used to be in expr.c, but it is here so that all of the
   variable binding stuff is localized.  Since we don't want any
   recursive evaluation from bind_variable() (possible without this code,
   since bind_variable() calls the evaluator for variables with the integer
   attribute set), we temporarily turn off the integer attribute for each
   variable we set here, then turn it back on after binding as necessary. */
SHELL_VAR *
Shell::bind_int_variable (const string &lhs, const string &rhs,
                          assign_flags flags)
{
  bool isint = false, isarr = false, implicitarray = false;

#if defined(ARRAY_VARS)
  /* Don't rely on VA_NOEXPAND being 1, set it explicitly */
  valid_array_flags vflags = (flags & ASS_NOEXPAND) ? VA_NOEXPAND : VA_NOFLAGS;
  if (flags & ASS_ONEWORD)
    vflags |= VA_ONEWORD;

  SHELL_VAR *v = nullptr;
  if (valid_array_reference (lhs, vflags))
    {
      isarr = true;
      av_flags avflags = AV_NOFLAGS;

      /* Common code to translate between assignment and reference flags. */
      if (flags & ASS_NOEXPAND)
        avflags |= AV_NOEXPAND;

      if (flags & ASS_ONEWORD)
        avflags |= AV_ONEWORD;

      v = array_variable_part (lhs, avflags, nullptr, nullptr);
    }
  else if (!legal_identifier (lhs))
    {
      sh_invalidid (lhs.c_str ());
      return nullptr;
    }
  else
#endif
    v = find_variable (lhs);

  if (v)
    {
      isint = v->integer ();
      v->unset_attr (att_integer);

#if defined(ARRAY_VARS)
      if (v->array () && !isarr)
        implicitarray = true;
#endif
    }

#if defined(ARRAY_VARS)
  if (isarr)
    v = assign_array_element (lhs, rhs, flags, nullptr);
  else if (implicitarray)
    v = bind_array_variable (lhs, 0, rhs); /* XXX - check on flags */
  else
#endif
    v = bind_variable (lhs, rhs); /* why not use bind_variable_value? */

  if (v)
    {
      if (isint)
        v->set_attr (att_integer);

      v->unset_attr (att_invisible);
    }

  if (v && v->nameref ())
    internal_warning (_ ("%s: assigning integer to name reference"),
                      lhs.c_str ());

  return v;
}

/* Do a function binding to a variable.  You pass the name and
   the command to bind to.  This conses the name and command. */
SHELL_VAR *
Shell::bind_function (string_view name, COMMAND *value)
{
  SHELL_VAR *entry = find_function (name);
  if (entry == nullptr)
    {
      BUCKET_CONTENTS<SHELL_VAR> *elt;

      elt = shell_functions->insert (name, HS_NOSRCH);
      entry = new SHELL_VAR (name);
      elt->data = entry;
    }
  else
    entry->invalidate_exportstr ();

  if (entry->is_set ())
    delete entry->func_value ();

  if (value)
    entry->set_func_value (value->clone ());
  else
    entry->set_func_value (nullptr);

  entry->set_attr (att_function);

  if (mark_modified_vars)
    entry->set_attr (att_exported);

  entry->unset_attr (att_invisible); /* Just to be sure */

  if (entry->exported ())
    array_needs_making = true;

#if defined(PROGRAMMABLE_COMPLETION)
  set_itemlist_dirty (&it_functions);
#endif

  return entry;
}

#if defined(DEBUGGER)
/* Bind a function definition, which includes source file and line number
   information in addition to the command, into the function hash table.
   If OVERWRITE is set, overwrite any existing definition. Otherwise, leave
   any existing definition alone. */
void
Shell::bind_function_def (string_view name, FUNCTION_DEF *value,
                          bool overwrite)
{
  BUCKET_CONTENTS<FUNCTION_DEF> *bucket;
  bucket = shell_function_defs->search (name, HS_NOFLAGS);
  if (bucket && overwrite)
    {
      delete bucket->data;
      bucket->data = static_cast<FUNCTION_DEF *> (value->clone ());
    }
  else if (bucket)
    return;
  else
    {
      COMMAND *cmd = value->command;
      value->command = nullptr;
      FUNCTION_DEF *entry = static_cast<FUNCTION_DEF *> (value->clone ());
      value->command = cmd;

      bucket = shell_function_defs->insert (name, HS_NOSRCH);
      bucket->data = entry;
    }
}
#endif /* DEBUGGER */

/* Add STRING, which is of the form foo=bar, to the temporary environment
   HASH_TABLE (temporary_env). The functions in execute_cmd.cc are
   responsible for moving the main temporary env to one of the other
   temporary environments. The expansion code in subst.cc calls this. */
int
Shell::assign_in_env (WORD_DESC *word, assign_flags flags)
{
  char *name;
  const char *newname;

  const string &str = word->word;

  assign_flags aflags = ASS_NOFLAGS;
  size_t offset = assignment (str, 0);
  newname = name = savestring (str);
  string *value = nullptr;

  if (name[offset] == '=')
    {
      name[offset] = 0;

      /* don't ignore the `+' when assigning temporary environment */
      if (name[offset - 1] == '+')
        {
          name[offset - 1] = '\0';
          aflags = ASS_APPEND;
        }

      if (!legal_identifier (name))
        {
          sh_invalidid (name);
          delete[] name;
          return 0;
        }

      SHELL_VAR *var = find_variable (name);
      if (var == nullptr)
        {
          var = find_variable_last_nameref (name, 1);
          /* If we're assigning a value to a nameref variable in the temp
             environment, and the value of the nameref is valid for assignment,
             but the variable does not already exist, assign to the nameref
             target and add the target to the temporary environment.  This is
             what ksh93 does */
          /* We use 2 in the call to valid_nameref_value because we don't want
             to allow array references here at all (newname will be used to
             create a variable directly below) */
          if (var && var->nameref ()
              && valid_nameref_value (*var->str_value (), 2))
            {
              newname = var->str_value ()->c_str ();
              var = nullptr; /* don't use it for append */
            }
        }
      else
        newname = var->name ().c_str (); /* no-op if not nameref */

      if (var && (var->readonly () || var->noassign ()))
        {
          if (var->readonly ())
            err_readonly (name);
          delete[] name;
          return 0;
        }

      char *temp = name + offset + 1;

      value = new string (expand_assignment_string_to_string (temp, 0));

      if (var && (aflags & ASS_APPEND))
        {
          string *temp2 = make_variable_value (var, *value, aflags);
          delete value;
          value = temp2;
        }
    }

  if (temporary_env == nullptr)
    temporary_env = new HASH_TABLE<SHELL_VAR> (TEMPENV_HASH_BUCKETS);

  SHELL_VAR *var = hash_lookup (newname, temporary_env);
  if (var == nullptr)
    var = make_new_variable (newname, *temporary_env);
  else
    var->dispose_value ();

  if (value == nullptr)
    value = new string ();

  var->set_value (value);
  var->attributes |= (att_exported | att_tempvar);
  var->context = variable_context; /* XXX */

  var->invalidate_exportstr ();
  var->exportstr = mk_env_string (newname, *value);

  array_needs_making = true;

  if (flags)
    {
      if (STREQ (newname, "POSIXLY_CORRECT")
          || STREQ (newname, "POSIX_PEDANDTIC"))
        save_posix_options (); /* XXX one level of saving right now */
      stupidly_hack_special_variables (newname);
    }

  if (echo_command_at_execute)
    /* The Korn shell prints the `+ ' in front of assignment statements,
        so we do too. */
    xtrace_print_assignment (name, value->c_str (), 0, 1);

  delete[] name;
  return 1;
}

/* **************************************************************** */
/*								    */
/*		  Deleting and unsetting variables		    */
/*								    */
/* **************************************************************** */

/* Unset the shell variable referenced by NAME.  Unsetting a nameref variable
   unsets the variable it resolves to but leaves the nameref alone. */
int
Shell::unbind_variable (const string &name)
{
  SHELL_VAR *v = var_lookup (name, shell_variables);
  SHELL_VAR *nv = (v && v->nameref ()) ? find_variable_nameref (v) : nullptr;

  int r = nv ? makunbound (nv->name (), shell_variables)
             : makunbound (name, shell_variables);
  return r;
}

/* Unbind NAME, where NAME is assumed to be a nameref variable */
int
Shell::unbind_nameref (const string &name)
{
  SHELL_VAR *v = var_lookup (name, shell_variables);
  if (v && v->nameref ())
    return makunbound (name, shell_variables);
  return 0;
}

/* Unbind the first instance of NAME, whether it's a nameref or not */
int
Shell::unbind_variable_noref (const string &name)
{
  SHELL_VAR *v = var_lookup (name, shell_variables);
  if (v)
    return makunbound (name, shell_variables);
  return 0;
}

int
Shell::unbind_global_variable (const string &name)
{
  SHELL_VAR *v, *nv;
  int r;

  v = var_lookup (name, global_variables);

  /* This starts at the current scope, just like find_global_variable; should
     we use find_global_variable_nameref here? */
  nv = (v && v->nameref ()) ? find_variable_nameref (v) : nullptr;

  r = nv ? makunbound (nv->name (), shell_variables)
         : makunbound (name, global_variables);
  return r;
}

int
Shell::unbind_global_variable_noref (const string &name)
{
  SHELL_VAR *v = var_lookup (name, global_variables);
  if (v)
    return makunbound (name, global_variables);
  return 0;
}

int
Shell::check_unbind_variable (const string &name)
{
  SHELL_VAR *v = find_variable (name);
  if (v && v->readonly ())
    {
      internal_error (_ ("%s: cannot unset: readonly %s"), name, "variable");
      return -2;
    }
  else if (v && v->non_unsettable ())
    {
      internal_error (_ ("%s: cannot unset"), name);
      return -2;
    }
  return unbind_variable (name);
}

/* Unset the shell function named NAME. */
int
Shell::unbind_func (const string &name)
{
  BUCKET_CONTENTS<SHELL_VAR> *elt = shell_functions->remove (name);
  if (elt == nullptr)
    return -1;

#if defined(PROGRAMMABLE_COMPLETION)
  set_itemlist_dirty (&it_functions);
#endif

  SHELL_VAR *func = elt->data;
  if (func && func->exported ())
    array_needs_making = true;

  delete elt; // this also deletes the contained data.
  return 0;
}

#if defined(DEBUGGER)
int
Shell::unbind_function_def (const string &name)
{
  BUCKET_CONTENTS<FUNCTION_DEF> *elt = shell_function_defs->remove (name);
  if (elt == nullptr)
    return -1;

  delete elt; // this also deletes the contained data.
  return 0;
}
#endif /* DEBUGGER */

int
Shell::delete_var (const string &name, VAR_CONTEXT *vc)
{
  BUCKET_CONTENTS<SHELL_VAR> *elt;
  VAR_CONTEXT *v;

  for (elt = nullptr, v = vc; v; v = v->down)
    if ((elt = v->table.remove (name)))
      break;

  if (elt == nullptr)
    return -1;

  delete elt; // this also deletes the contained data.
  return 0;
}

/* Make the variable associated with NAME go away.  HASH_LIST is the
   hash table from which this variable should be deleted (either
   shell_variables or shell_functions).
   Returns non-zero if the variable couldn't be found. */
int
Shell::makunbound (const string &name, VAR_CONTEXT *vc)
{
  BUCKET_CONTENTS<SHELL_VAR> *elt;
  VAR_CONTEXT *v;

  for (elt = nullptr, v = vc; v; v = v->down)
    if ((elt = v->table.remove (name)))
      break;

  if (elt == nullptr)
    return -1;

  SHELL_VAR *old_var = elt->data;

  if (old_var && old_var->exported ())
    array_needs_making = true;

  /* If we're unsetting a local variable and we're still executing inside
     the function, just mark the variable as invisible.  The function
     eventually called by pop_var_context() will clean it up later.  This
     must be done so that if the variable is subsequently assigned a new
     value inside the function, the `local' attribute is still present.
     We also need to add it back into the correct hash table. */
  if (old_var && old_var->local ()
      && (old_var->context == variable_context
          || (localvar_unset && old_var->context < variable_context)))
    {
      if (old_var->nofree ())
        {
          old_var->set_value (nullptr);
          old_var->invalidate_exportstr ();
        }
      else
        old_var->dispose_value ();

      /* Reset the attributes.  Preserve the export attribute if the variable
         came from a temporary environment.  Make sure it stays local, and
         make it invisible. */
      old_var->attributes = (old_var->exported () && old_var->tempvar ())
                                ? att_exported
                                : att_noflags;

      old_var->set_attr (att_local | att_invisible);

      BUCKET_CONTENTS<SHELL_VAR> *new_elt = v->table.insert (name, HS_NOFLAGS);
      new_elt->data = old_var;
      stupidly_hack_special_variables (name);

      elt->data = nullptr;
      delete elt;
      return 0;
    }

  delete elt;
  stupidly_hack_special_variables (name);

  return 0;
}

/* Get rid of all of the variables in the current context. */
void
Shell::kill_all_local_variables ()
{
  VAR_CONTEXT *vc;

  for (vc = shell_variables; vc; vc = vc->down)
    if (vc->func_env () && vc->scope == variable_context)
      break;

  if (vc == nullptr)
    return; /* XXX */

  // XXX - hopefully this is equivalent to delete_all_variables ()
  vc->table.flush ();
}

/* **************************************************************** */
/*								    */
/*		     Setting variable attributes		    */
/*								    */
/* **************************************************************** */

#define FIND_OR_MAKE_VARIABLE(name, entry)                                    \
  do                                                                          \
    {                                                                         \
      entry = find_variable (name);                                           \
      if (!entry)                                                             \
        {                                                                     \
          entry = bind_variable (name, string ());                            \
          if (entry)                                                          \
            entry->attributes |= att_invisible;                               \
        }                                                                     \
    }                                                                         \
  while (0)

/* Make the variable associated with NAME be readonly.
   If NAME does not exist yet, create it. */
void
Shell::set_var_read_only (const string &name)
{
  SHELL_VAR *entry;

  FIND_OR_MAKE_VARIABLE (name, entry);
  entry->set_attr (att_readonly);
}

/* **************************************************************** */
/*								    */
/*		     Creating lists of variables		    */
/*								    */
/* **************************************************************** */

VARLIST *
vlist_alloc (int nentries)
{
  VARLIST *vlist;

  vlist = (VARLIST *)xmalloc (sizeof (VARLIST));
  vlist->list = (SHELL_VAR **)xmalloc ((nentries + 1) * sizeof (SHELL_VAR *));
  vlist->list_size = nentries;
  vlist->list_len = 0;
  vlist->list[0] = nullptr;

  return vlist;
}

static VARLIST *
vlist_realloc (VARLIST *vlist, int n)
{
  if (vlist == 0)
    return vlist = vlist_alloc (n);
  if (n > vlist->list_size)
    {
      vlist->list_size = n;
      vlist->list = (SHELL_VAR **)xrealloc (
          vlist->list, (vlist->list_size + 1) * sizeof (SHELL_VAR *));
    }
  return vlist;
}

static void
vlist_add (VARLIST *vlist, SHELL_VAR *var, int flags)
{
  int i;

  for (i = 0; i < vlist->list_len; i++)
    if (STREQ (var->name, vlist->list[i]->name))
      break;
  if (i < vlist->list_len)
    return;

  if (i >= vlist->list_size)
    vlist = vlist_realloc (vlist, vlist->list_size + 16);

  vlist->list[vlist->list_len++] = var;
  vlist->list[vlist->list_len] = nullptr;
}

/* Map FUNCTION over the variables in VARLIST.  Return an array of the
   variables for which FUNCTION returns a non-zero value.  A nullptr value
   for FUNCTION means to use all variables. */
SHELL_VAR **
map_over (sh_var_map_func_t *function, VAR_CONTEXT *vc)
{
  VAR_CONTEXT *v;
  VARLIST *vlist;
  SHELL_VAR **ret;
  int nentries;

  for (nentries = 0, v = vc; v; v = v->down)
    nentries += HASH_ENTRIES (v->table);

  if (nentries == 0)
    return nullptr;

  vlist = vlist_alloc (nentries);

  for (v = vc; v; v = v->down)
    flatten (v->table, function, vlist, 0);

  ret = vlist->list;
  free (vlist);
  return ret;
}

SHELL_VAR **
map_over_funcs (sh_var_map_func_t *function)
{
  VARLIST *vlist;
  SHELL_VAR **ret;

  if (shell_functions == 0 || HASH_ENTRIES (shell_functions) == 0)
    return nullptr;

  vlist = vlist_alloc (HASH_ENTRIES (shell_functions));

  flatten (shell_functions, function, vlist, 0);

  ret = vlist->list;
  free (vlist);
  return ret;
}

/* Flatten VARLIST, applying FUNC to each member and adding those
   elements for which FUNC succeeds to VLIST->list.  FLAGS is reserved
   for future use.  Only unique names are added to VLIST.  If FUNC is
   nullptr, each variable in VARLIST is added to VLIST.  If VLIST is
   nullptr, FUNC is applied to each SHELL_VAR in VARLIST.  If VLIST
   and FUNC are both nullptr, nothing happens. */
static void
flatten (HASH_TABLE *var_hash_table, sh_var_map_func_t *func, VARLIST *vlist,
         int flags)
{
  int i;
  BUCKET_CONTENTS *tlist;
  int r;
  SHELL_VAR *var;

  if (var_hash_table == 0 || (HASH_ENTRIES (var_hash_table) == 0)
      || (vlist == 0 && func == 0))
    return;

  for (i = 0; i < var_hash_table->nbuckets; i++)
    {
      for (tlist = hash_items (i, var_hash_table); tlist; tlist = tlist->next)
        {
          var = (SHELL_VAR *)tlist->data;

          r = func ? (*func) (var) : 1;
          if (r && vlist)
            vlist_add (vlist, var, flags);
        }
    }
}

void
sort_variables (SHELL_VAR **array)
{
  qsort (array, strvec_len ((char **)array), sizeof (SHELL_VAR *),
         (QSFUNC *)qsort_var_comp);
}

static int
qsort_var_comp (const SHELL_VAR **var1, const SHELL_VAR **var2)
{
  int result;

  if ((result = (*var1)->name[0] - (*var2)->name[0]) == 0)
    result = strcmp ((*var1)->name, (*var2)->name);

  return result;
}

/* Apply FUNC to each variable in SHELL_VARIABLES, adding each one for
   which FUNC succeeds to an array of SHELL_VAR *s.  Returns the array. */
static SHELL_VAR **
vapply (sh_var_map_func_t *func)
{
  SHELL_VAR **list;

  list = map_over (func, shell_variables);
  if (list /* && posixly_correct */)
    sort_variables (list);
  return list;
}

/* Apply FUNC to each variable in SHELL_FUNCTIONS, adding each one for
   which FUNC succeeds to an array of SHELL_VAR *s.  Returns the array. */
static SHELL_VAR **
fapply (sh_var_map_func_t *func)
{
  SHELL_VAR **list;

  list = map_over_funcs (func);
  if (list /* && posixly_correct */)
    sort_variables (list);
  return list;
}

/* Create a nullptr terminated array of all the shell variables. */
SHELL_VAR **
all_shell_variables ()
{
  return vapply (nullptr);
}

/* Create a nullptr terminated array of all the shell functions. */
SHELL_VAR **
all_shell_functions ()
{
  return fapply (nullptr);
}

static int
visible_var (SHELL_VAR *var)
{
  return invisible_p (var) == 0;
}

SHELL_VAR **
all_visible_functions ()
{
  return fapply (visible_var);
}

SHELL_VAR **
all_visible_variables ()
{
  return vapply (visible_var);
}

/* Return non-zero if the variable VAR is visible and exported.  Array
   variables cannot be exported. */
static int
visible_and_exported (SHELL_VAR *var)
{
  return invisible_p (var) == 0 && var->exported ();
}

/* Candidate variables for the export environment are either valid variables
   with the export attribute or invalid variables inherited from the initial
   environment and simply passed through. */
static int
export_environment_candidate (SHELL_VAR *var)
{
  return var->exported () && (invisible_p (var) == 0 || imported_p (var));
}

/* Return non-zero if VAR is a local variable in the current context and
   is exported. */
static int
local_and_exported (SHELL_VAR *var)
{
  return invisible_p (var) == 0 && local_p (var)
         && var->context == variable_context && var->exported ();
}

SHELL_VAR **
all_exported_variables ()
{
  return vapply (visible_and_exported);
}

SHELL_VAR **
local_exported_variables ()
{
  return vapply (local_and_exported);
}

static int
variable_in_context (SHELL_VAR *var)
{
  return local_p (var) && var->context == variable_context;
}

static int
visible_variable_in_context (SHELL_VAR *var)
{
  return invisible_p (var) == 0 && local_p (var)
         && var->context == variable_context;
}

SHELL_VAR **
all_local_variables (int visible_only)
{
  VARLIST *vlist;
  SHELL_VAR **ret;
  VAR_CONTEXT *vc;

  vc = shell_variables;
  for (vc = shell_variables; vc; vc = vc->down)
    if (vc_isfuncenv (vc) && vc->scope == variable_context)
      break;

  if (vc == 0)
    {
      internal_error (
          _ ("all_local_variables: no function context at current scope"));
      return nullptr;
    }
  if (vc->table == 0 || HASH_ENTRIES (vc->table) == 0
      || vc_haslocals (vc) == 0)
    return nullptr;

  vlist = vlist_alloc (HASH_ENTRIES (vc->table));

  if (visible_only)
    flatten (vc->table, visible_variable_in_context, vlist, 0);
  else
    flatten (vc->table, variable_in_context, vlist, 0);

  ret = vlist->list;
  free (vlist);
  if (ret)
    sort_variables (ret);
  return ret;
}

#if defined(ARRAY_VARS)
/* Return non-zero if the variable VAR is visible and an array. */
static int
visible_array_vars (SHELL_VAR *var)
{
  return invisible_p (var) == 0 && (array_p (var) || assoc_p (var));
}

SHELL_VAR **
all_array_variables ()
{
  return vapply (visible_array_vars);
}
#endif /* ARRAY_VARS */

char **
all_variables_matching_prefix (const string &prefix)
{
  SHELL_VAR **varlist;
  char **rlist;
  int vind, rind, plen;

  plen = STRLEN (prefix);
  varlist = all_visible_variables ();
  for (vind = 0; varlist && varlist[vind]; vind++)
    ;
  if (varlist == 0 || vind == 0)
    return nullptr;
  rlist = strvec_create (vind + 1);
  for (vind = rind = 0; varlist[vind]; vind++)
    {
      if (plen == 0 || STREQN (prefix, varlist[vind]->name, plen))
        rlist[rind++] = savestring (varlist[vind]->name);
    }
  rlist[rind] = 0;
  free (varlist);

  return rlist;
}

/* **************************************************************** */
/*								    */
/*		 Managing temporary variable scopes		    */
/*								    */
/* **************************************************************** */

/* Make variable NAME have VALUE in the temporary environment. */
static SHELL_VAR *
bind_tempenv_variable (const string &name, const string &value)
{
  SHELL_VAR *var;

  var = temporary_env ? hash_lookup (name, temporary_env) : nullptr;

  if (var)
    {
      FREE (value_cell (var));
      var_setvalue (var, savestring (value));
      entry->invalidate_exportstr ();
    }

  return var;
}

/* Find a variable in the temporary environment that is named NAME.
   Return the SHELL_VAR *, or nullptr if not found. */
SHELL_VAR *
find_tempenv_variable (const string &name)
{
  return temporary_env ? hash_lookup (name, temporary_env) : nullptr;
}

// char **tempvar_list;
// int tvlist_ind;

/* Take a variable from an assignment statement preceding a posix special
   builtin (including `return') and create a global variable from it. This
   is called from merge_temporary_env, which is only called when in posix
   mode. */
static void
push_posix_temp_var (SHELL_VAR *data)
{
  SHELL_VAR *var, *v;
  HASH_TABLE *binding_table;

  var = (SHELL_VAR *)data;

  /* Just like do_assignment_internal(). This makes assignments preceding
     special builtins act like standalone assignment statements when in
     posix mode, satisfying the posix requirement that this affect the
     "current execution environment." */
  v = bind_variable (var->name, value_cell (var), ASS_FORCE | ASS_NOLONGJMP);

  /* XXX - do we need to worry about array variables here? */

  /* If this modifies an existing local variable, v->context will be non-zero.
     If it comes back with v->context == 0, we bound at the global context.
     Set binding_table appropriately. It doesn't matter whether it's correct
     if the variable is local, only that it's not global_variables->table */
  binding_table
      = v->context ? shell_variables->table : global_variables->table;

  /* global variables are no longer temporary and don't need propagating. */
  if (v->context == 0)
    var->attributes &= ~(att_tempvar | att_propagate);

  if (v)
    {
      v->attributes
          |= var->attributes; /* preserve tempvar attribute if appropriate */
      /* If we don't bind a local variable, propagate the value. If we bind a
         local variable (the "current execution environment"), keep it as local
         and don't propagate it to the calling environment. */
      if (v->context > 0 && local_p (v) == 0)
        v->attributes |= att_propagate;
      else
        v->attributes &= ~att_propagate;
    }

  if (find_special_var (var->name) >= 0)
    tempvar_list[tvlist_ind++] = savestring (var->name);

  dispose_variable (var);
}

/* Push the variable described by (SHELL_VAR *)DATA down to the next
   variable context from the temporary environment. This can be called
   from one context:
      1. propagate_temp_var: which is called to propagate variables in
         assignments like `var=value declare -x var' to the surrounding
         scope.

  In this case, the variable should have the att_propagate flag set and
  we can create variables in the current scope.
*/
static void
push_temp_var (void *data)
{
  SHELL_VAR *var, *v;
  HASH_TABLE *binding_table;

  var = (SHELL_VAR *)data;

  binding_table = shell_variables->table;
  if (binding_table == 0)
    {
      if (shell_variables == global_variables)
        /* shouldn't happen */
        binding_table = shell_variables->table = global_variables->table
            = hash_create (VARIABLES_HASH_BUCKETS);
      else
        binding_table = shell_variables->table
            = hash_create (TEMPENV_HASH_BUCKETS);
    }

  v = bind_variable_internal (var->name, value_cell (var), binding_table, 0,
                              ASS_FORCE | ASS_NOLONGJMP);

  /* XXX - should we set the context here?  It shouldn't matter because of how
     assign_in_env works, but we do it anyway. */
  if (v)
    v->context = shell_variables->scope;

  if (binding_table == global_variables->table) /* XXX */
    var->attributes &= ~(att_tempvar | att_propagate);
  else
    {
      var->attributes |= att_propagate; /* XXX - propagate more than once? */
      if (binding_table == shell_variables->table)
        shell_variables->flags |= VC_HASTMPVAR;
    }
  if (v)
    v->attributes |= var->attributes;

  if (find_special_var (var->name) >= 0)
    tempvar_list[tvlist_ind++] = savestring (var->name);

  dispose_variable (var);
}

/* Take a variable described by DATA and push it to the surrounding scope if
   the PROPAGATE attribute is set. That gets set by push_temp_var if we are
   taking a variable like `var=value declare -x var' and propagating it to
   the enclosing scope. */
static void
propagate_temp_var (void *data)
{
  SHELL_VAR *var;

  var = (SHELL_VAR *)data;
  if (tempvar_p (var) && (var->attributes & att_propagate))
    push_temp_var (data);
  else
    {
      if (find_special_var (var->name) >= 0)
        tempvar_list[tvlist_ind++] = savestring (var->name);
      dispose_variable (var);
    }
}

/* Free the storage used in the hash table for temporary
   environment variables.  PUSHF is a function to be called
   to free each hash table entry.  It takes care of pushing variables
   to previous scopes if appropriate.  PUSHF stores names of variables
   that require special handling (e.g., IFS) on tempvar_list, so this
   function can call stupidly_hack_special_variables on all the
   variables in the list when the temporary hash table is destroyed. */
static void
dispose_temporary_env (sh_free_func_t *pushf)
{
  int i;
  HASH_TABLE *disposer;

  tempvar_list = strvec_create (HASH_ENTRIES (temporary_env) + 1);
  tempvar_list[tvlist_ind = 0] = 0;

  disposer = temporary_env;
  temporary_env = nullptr;

  hash_flush (disposer, pushf);
  hash_dispose (disposer);

  tempvar_list[tvlist_ind] = 0;

  array_needs_making = true;

  for (i = 0; i < tvlist_ind; i++)
    stupidly_hack_special_variables (tempvar_list[i]);

  strvec_dispose (tempvar_list);
  tempvar_list = 0;
  tvlist_ind = 0;
}

void
dispose_used_env_vars ()
{
  if (temporary_env)
    {
      dispose_temporary_env (propagate_temp_var);
      maybe_make_export_env ();
    }
}

/* Take all of the shell variables in the temporary environment HASH_TABLE
   and make shell variables from them at the current variable context.
   Right now, this is only called in Posix mode to implement the historical
   accident of creating global variables from assignment statements preceding
   special builtins, but we check in case this acquires another caller later.
 */
void
merge_temporary_env ()
{
  if (temporary_env)
    dispose_temporary_env (posixly_correct ? push_posix_temp_var
                                           : push_temp_var);
}

/* Temporary function to use if we want to separate function and special
   builtin behavior. */
void
merge_function_temporary_env ()
{
  if (temporary_env)
    dispose_temporary_env (push_temp_var);
}

void
flush_temporary_env ()
{
  if (temporary_env)
    {
      hash_flush (temporary_env, free_variable_hash_data);
      hash_dispose (temporary_env);
      temporary_env = nullptr;
    }
}

/* **************************************************************** */
/*								    */
/*	     Creating and manipulating the environment		    */
/*								    */
/* **************************************************************** */

static inline char *
mk_env_string (string_view name, string_view value, var_att_flags attributes)
{
  // XXX - continue merging from bash-5.2 here.
  // size_t name_len, value_len;
  // char *p, *q, *t;
  // bool isfunc, isarray;

  size_t name_len = name.size ();
  size_t value_len = value.size ();

  isfunc = attributes & att_function;
#if defined(ARRAY_VARS) && defined(ARRAY_EXPORT)
  isarray = attributes & (att_array | att_assoc);
#endif

  /* If we are exporting a shell function, construct the encoded function
     name. */
  if (isfunc && value)
    {
      p = xmalloc (BASHFUNC_PREFLEN + name_len + BASHFUNC_SUFFLEN + value_len
                   + 2);
      q = p;
      memcpy (q, BASHFUNC_PREFIX, BASHFUNC_PREFLEN);
      q += BASHFUNC_PREFLEN;
      memcpy (q, name, name_len);
      q += name_len;
      memcpy (q, BASHFUNC_SUFFIX, BASHFUNC_SUFFLEN);
      q += BASHFUNC_SUFFLEN;
    }
#if defined(ARRAY_VARS) && defined(ARRAY_EXPORT)
  else if (isarray && value)
    {
      if (attributes & att_assoc)
        p = xmalloc (BASHASSOC_PREFLEN + name_len + BASHASSOC_SUFFLEN
                     + value_len + 2);
      else
        p = xmalloc (BASHARRAY_PREFLEN + name_len + BASHARRAY_SUFFLEN
                     + value_len + 2);
      q = p;
      if (attributes & att_assoc)
        {
          memcpy (q, BASHASSOC_PREFIX, BASHASSOC_PREFLEN);
          q += BASHASSOC_PREFLEN;
        }
      else
        {
          memcpy (q, BASHARRAY_PREFIX, BASHARRAY_PREFLEN);
          q += BASHARRAY_PREFLEN;
        }
      memcpy (q, name, name_len);
      q += name_len;
      /* These are actually the same currently */
      if (attributes & att_assoc)
        {
          memcpy (q, BASHASSOC_SUFFIX, BASHASSOC_SUFFLEN);
          q += BASHARRAY_SUFFLEN;
        }
      else
        {
          memcpy (q, BASHARRAY_SUFFIX, BASHARRAY_SUFFLEN);
          q += BASHARRAY_SUFFLEN;
        }
    }
#endif
  else
    {
      p = xmalloc (2 + name_len + value_len);
      memcpy (p, name, name_len);
      q = p + name_len;
    }

  q[0] = '=';
  if (value && *value)
    {
      if (isfunc)
        {
          t = dequote_escapes (value);
          value_len = STRLEN (t);
          memcpy (q + 1, t, value_len + 1);
          free (t);
        }
      else
        memcpy (q + 1, value, value_len + 1);
    }
  else
    q[1] = '\0';

  return p;
}

#ifdef DEBUG
/* Debugging */
static int
valid_exportstr (SHELL_VAR *v)
{
  char *s;

  s = v->exportstr;
  if (s == 0)
    {
      internal_error (_ ("%s has null exportstr"), v->name);
      return 0;
    }
  if (legal_variable_starter ((unsigned char)*s) == 0)
    {
      internal_error (_ ("invalid character %d in exportstr for %s"), *s,
                      v->name);
      return 0;
    }
  for (s = v->exportstr + 1; s && *s; s++)
    {
      if (*s == '=')
        break;
      if (legal_variable_char ((unsigned char)*s) == 0)
        {
          internal_error (_ ("invalid character %d in exportstr for %s"), *s,
                          v->name);
          return 0;
        }
    }
  if (*s != '=')
    {
      internal_error (_ ("no `=' in exportstr for %s"), v->name);
      return 0;
    }
  return 1;
}
#endif

#if defined(ARRAY_VARS)
#define USE_EXPORTSTR                                                         \
  (value == var->exportstr && array_p (var) == 0 && assoc_p (var) == 0)
#else
#define USE_EXPORTSTR (value == var->exportstr)
#endif

static char **
make_env_array_from_var_list (SHELL_VAR **vars)
{
  int i, list_index;
  SHELL_VAR *var;
  char **list, *value;

  list = strvec_create ((1 + strvec_len ((char **)vars)));

  for (i = 0, list_index = 0; var = vars[i]; i++)
    {
#if defined(__CYGWIN__)
      /* We don't use the exportstr stuff on Cygwin at all. */
      entry->invalidate_exportstr ();
#endif

      /* If the value is generated dynamically, generate it here. */
      if (regen_p (var) && var->dynamic_value)
        {
          var = (*(var->dynamic_value)) (var);
          entry->invalidate_exportstr ();
        }

      if (var->exportstr)
        value = var->exportstr;
      else if (function_p (var))
        value = named_function_string (nullptr, function_cell (var), 0);
#if defined(ARRAY_VARS)
      else if (array_p (var))
#if defined(ARRAY_EXPORT)
        value = array_to_assign (array_cell (var), 0);
#else
        continue; /* XXX array vars cannot yet be exported */
#endif /* ARRAY_EXPORT */
      else if (assoc_p (var))
#if defined(ARRAY_EXPORT)
        value = assoc_to_assign (assoc_cell (var), 0);
#else
        continue; /* XXX associative array vars cannot yet be exported */
#endif /* ARRAY_EXPORT */
#endif
#endif
      else
        value = value_cell (var);

      if (value)
        {
          /* Gee, I'd like to get away with not using savestring() if we're
             using the cached exportstr... */
          list[list_index] = USE_EXPORTSTR ? savestring (value)
                                           : mk_env_string (var->name, value,
                                                            var->attributes);

          if (USE_EXPORTSTR == 0)
            SAVE_EXPORTSTR (var, list[list_index]);

          list_index++;
#undef USE_EXPORTSTR

#if defined(ARRAY_VARS) && defined(ARRAY_EXPORT)
          if (array_p (var) || assoc_p (var))
            free (value);
#endif
        }
    }

  list[list_index] = nullptr;
  return list;
}

/* Make an array of assignment statements from the hash table
   HASHED_VARS which contains SHELL_VARs.  Only visible, exported
   variables are eligible. */
static char **
make_var_export_array (VAR_CONTEXT *vcxt)
{
  char **list;
  SHELL_VAR **vars;

#if 0
  vars = map_over (visible_and_exported, vcxt);
#else
  vars = map_over (export_environment_candidate, vcxt);
#endif

  if (vars == nullptr)
    return nullptr;

  list = make_env_array_from_var_list (vars);

  free (vars);
  return list;
}

static char **
make_func_export_array ()
{
  char **list;
  SHELL_VAR **vars;

  vars = map_over_funcs (visible_and_exported);
  if (vars == nullptr)
    return nullptr;

  list = make_env_array_from_var_list (vars);

  free (vars);
  return list;
}

/* Add ENVSTR to the end of the exported environment, EXPORT_ENV. */
#define add_to_export_env(envstr, do_alloc)                                   \
  do                                                                          \
    {                                                                         \
      if (export_env_index >= (export_env_size - 1))                          \
        {                                                                     \
          export_env_size += 16;                                              \
          export_env = strvec_resize (export_env, export_env_size);           \
          environ = export_env;                                               \
        }                                                                     \
      export_env[export_env_index++]                                          \
          = (do_alloc) ? savestring (envstr) : envstr;                        \
      export_env[export_env_index] = nullptr;                                 \
    }                                                                         \
  while (0)

/* Add ASSIGN to EXPORT_ENV, or supersede a previous assignment in the
   array with the same left-hand side.  Return the new EXPORT_ENV. */
char **
add_or_supercede_exported_var (char *assign, int do_alloc)
{
  int i;
  int equal_offset;

  equal_offset = assignment (assign, 0);
  if (equal_offset == 0)
    return export_env;

  /* If this is a function, then only supersede the function definition.
     We do this by including the `=() {' in the comparison, like
     initialize_shell_variables does. */
  if (assign[equal_offset + 1] == '('
      && strncmp (assign + equal_offset + 2, ") {", 3) == 0)
    equal_offset += 4;

  for (i = 0; i < export_env_index; i++)
    {
      if (STREQN (assign, export_env[i], equal_offset + 1))
        {
          free (export_env[i]);
          export_env[i] = do_alloc ? savestring (assign) : assign;
          return export_env;
        }
    }
  add_to_export_env (assign, do_alloc);
  return export_env;
}

static void
add_temp_array_to_env (char **temp_array, int do_alloc, int do_supercede)
{
  int i;

  if (temp_array == 0)
    return;

  for (i = 0; temp_array[i]; i++)
    {
      if (do_supercede)
        export_env = add_or_supercede_exported_var (temp_array[i], do_alloc);
      else
        add_to_export_env (temp_array[i], do_alloc);
    }

  free (temp_array);
}

/* Make the environment array for the command about to be executed, if the
   array needs making.  Otherwise, do nothing.  If a shell action could
   change the array that commands receive for their environment, then the
   code should `array_needs_making++'.

   The order to add to the array is:
        temporary_env
        list of var contexts whose head is shell_variables
        shell_functions

  This is the shell variable lookup order.  We add only new variable
  names at each step, which allows local variables and variables in
  the temporary environments to shadow variables in the global (or
  any previous) scope.
*/

static int
n_shell_variables ()
{
  VAR_CONTEXT *vc;
  int n;

  for (n = 0, vc = shell_variables; vc; vc = vc->down)
    n += HASH_ENTRIES (vc->table);
  return n;
}

int
chkexport (const char *name)
{
  SHELL_VAR *v;

  v = find_variable (name);
  if (v && exported_p (v))
    {
      array_needs_making = true;
      maybe_make_export_env ();
      return 1;
    }
  return 0;
}

void
maybe_make_export_env ()
{
  char **temp_array;
  int new_size;
  VAR_CONTEXT *tcxt, *icxt;

  if (array_needs_making)
    {
      if (export_env)
        strvec_flush (export_env);

      /* Make a guess based on how many shell variables and functions we
         have.  Since there will always be array variables, and array
         variables are not (yet) exported, this will always be big enough
         for the exported variables and functions. */
      new_size = n_shell_variables () + HASH_ENTRIES (shell_functions) + 1
                 + HASH_ENTRIES (temporary_env) + HASH_ENTRIES (invalid_env);
      if (new_size > export_env_size)
        {
          export_env_size = new_size;
          export_env = strvec_resize (export_env, export_env_size);
          environ = export_env;
        }
      export_env[export_env_index = 0] = nullptr;

      /* Make a dummy variable context from the temporary_env, stick it on
         the front of shell_variables, call make_var_export_array on the
         whole thing to flatten it, and convert the list of SHELL_VAR *s
         to the form needed by the environment. */
      if (temporary_env)
        {
          tcxt = new_var_context (nullptr, 0);
          tcxt->table = temporary_env;
          tcxt->down = shell_variables;
        }
      else
        tcxt = shell_variables;

      if (invalid_env)
        {
          icxt = new_var_context (nullptr, 0);
          icxt->table = invalid_env;
          icxt->down = tcxt;
        }
      else
        icxt = tcxt;

      temp_array = make_var_export_array (icxt);
      if (temp_array)
        add_temp_array_to_env (temp_array, 0, 0);

      if (icxt != tcxt)
        free (icxt);

      if (tcxt != shell_variables)
        free (tcxt);

#if defined(RESTRICTED_SHELL)
      /* Restricted shells may not export shell functions. */
      temp_array = restricted ? (char **)0 : make_func_export_array ();
#else
      temp_array = make_func_export_array ();
#endif
      if (temp_array)
        add_temp_array_to_env (temp_array, 0, 0);

      array_needs_making = false;
    }
}

/* This is an efficiency hack.  PWD and OLDPWD are auto-exported, so
   we will need to remake the exported environment every time we
   change directories.  `_' is always put into the environment for
   every external command, so without special treatment it will always
   cause the environment to be remade.

   If there is no other reason to make the exported environment, we can
   just update the variables in place and mark the exported environment
   as no longer needing a remake. */
void
update_export_env_inplace (const char *env_prefix, int preflen,
                           const char *value)
{
  char *evar;

  evar = xmalloc (STRLEN (value) + preflen + 1);
  strcpy (evar, env_prefix);
  if (value)
    strcpy (evar + preflen, value);
  export_env = add_or_supercede_exported_var (evar, 0);
}

/* We always put _ in the environment as the name of this command. */
void
put_command_name_into_env (const char *command_name)
{
  update_export_env_inplace ("_=", 2, command_name);
}

/* **************************************************************** */
/*								    */
/*		      Managing variable contexts		    */
/*								    */
/* **************************************************************** */

/* Allocate and return a new variable context with NAME and FLAGS.
   NAME can be nullptr. */

VAR_CONTEXT *
new_var_context (const char *name, int flags)
{
  VAR_CONTEXT *vc;

  vc = (VAR_CONTEXT *)xmalloc (sizeof (VAR_CONTEXT));
  vc->name = name ? savestring (name) : nullptr;
  vc->scope = variable_context;
  vc->flags = flags;

  vc->up = vc->down = nullptr;
  vc->table = nullptr;

  return vc;
}

/* Free a variable context and its data, including the hash table.  Dispose
   all of the variables. */
void
dispose_var_context (VAR_CONTEXT *vc)
{
  FREE (vc->name);

  if (vc->table)
    {
      delete_all_variables (vc->table);
      hash_dispose (vc->table);
    }

  free (vc);
}

/* Set VAR's scope level to the current variable context. */
static int
set_context (SHELL_VAR *var)
{
  return var->context = variable_context;
}

/* Make a new variable context with NAME and FLAGS and a HASH_TABLE of
   temporary variables, and push it onto shell_variables.  This is
   for shell functions. */
VAR_CONTEXT *
push_var_context (const char *name, int flags, HASH_TABLE *tempvars)
{
  VAR_CONTEXT *vc;
  int posix_func_behavior;

  /* As of IEEE Std 1003.1-2017, assignment statements preceding shell
     functions no longer behave like assignment statements preceding
     special builtins, and do not persist in the current shell environment.
     This is austin group interp #654, though nobody implements it yet. */
  posix_func_behavior = 0;

  vc = new_var_context (name, flags);
  /* Posix interp 1009, temporary assignments preceding function calls modify
     the current environment *before* the command is executed. */
  if (posix_func_behavior && (flags & VC_FUNCENV) && tempvars == temporary_env)
    merge_temporary_env ();
  else if (tempvars)
    {
      vc->table = tempvars;
      /* Have to do this because the temp environment was created before
         variable_context was incremented. */
      /* XXX - only need to do it if flags&VC_FUNCENV */
      flatten (tempvars, set_context, nullptr, 0);
      vc->flags |= VC_HASTMPVAR;
    }
  vc->down = shell_variables;
  shell_variables->up = vc;

  return shell_variables = vc;
}

/* This can be called from one of two code paths:
        1. pop_scope, which implements the posix rules for propagating variable
           assignments preceding special builtins to the surrounding scope
           (push_builtin_var -- isbltin == 1);
        2. pop_var_context, which is called from pop_context and implements the
           posix rules for propagating variable assignments preceding function
           calls to the surrounding scope (push_func_var -- isbltin == 0)

  It takes variables out of a temporary environment hash table. We take the
  variable in data.
*/

static inline void
push_posix_tempvar_internal (SHELL_VAR *var, int isbltin)
{
  SHELL_VAR *v;
  int posix_var_behavior;

  /* As of IEEE Std 1003.1-2017, assignment statements preceding shell
     functions no longer behave like assignment statements preceding
     special builtins, and do not persist in the current shell environment.
     This is austin group interp #654, though nobody implements it yet. */
  posix_var_behavior = posixly_correct && isbltin;
  v = 0;

  if (local_p (var) && STREQ (var->name, "-"))
    {
      set_current_options (value_cell (var));
      set_shellopts ();
    }
  /* This takes variable assignments preceding special builtins that can
     execute multiple commands (source, eval, etc.) and performs the equivalent
     of an assignment statement to modify the closest enclosing variable (the
     posix "current execution environment"). This makes the behavior the same
     as push_posix_temp_var; but the circumstances of calling are slightly
     different. */
  else if (tempvar_p (var) && posix_var_behavior)
    {
      /* similar to push_posix_temp_var */
      v = bind_variable (var->name, value_cell (var),
                         ASS_FORCE | ASS_NOLONGJMP);
      if (v)
        {
          v->attributes |= var->attributes;
          if (v->context == 0)
            v->attributes &= ~(att_tempvar | att_propagate);
          /* XXX - set att_propagate here if v->context > 0? */
        }
    }
  else if (tempvar_p (var) && propagate_p (var))
    {
      /* Make sure we have a hash table to store the variable in while it is
         being propagated down to the global variables table.  Create one if
         we have to */
      if ((vc_isfuncenv (shell_variables) || vc_istempenv (shell_variables))
          && shell_variables->table == 0)
        shell_variables->table = hash_create (VARIABLES_HASH_BUCKETS);
      v = bind_variable_internal (var->name, value_cell (var),
                                  shell_variables->table, 0, 0);
      /* XXX - should we set v->context here? */
      if (v)
        v->context = shell_variables->scope;
      if (shell_variables == global_variables)
        var->attributes &= ~(att_tempvar | att_propagate);
      else
        shell_variables->flags |= VC_HASTMPVAR;
      if (v)
        v->attributes |= var->attributes;
    }
  else
    stupidly_hack_special_variables (var->name); /* XXX */

#if defined(ARRAY_VARS)
  if (v && (array_p (var) || assoc_p (var)))
    {
      FREE (value_cell (v));
      if (array_p (var))
        var_setarray (v, array_copy (array_cell (var)));
      else
        var_setassoc (v, assoc_copy (assoc_cell (var)));
    }
#endif

  dispose_variable (var);
}

static void
push_func_var (void *data)
{
  SHELL_VAR *var;

  var = (SHELL_VAR *)data;
  push_posix_tempvar_internal (var, 0);
}

static void
push_builtin_var (void *data)
{
  SHELL_VAR *var;

  var = (SHELL_VAR *)data;
  push_posix_tempvar_internal (var, 1);
}

/* Pop the top context off of VCXT and dispose of it, returning the rest of
   the stack. */
void
pop_var_context ()
{
  VAR_CONTEXT *ret, *vcxt;

  vcxt = shell_variables;
  if (vc_isfuncenv (vcxt) == 0)
    {
      internal_error (_ (
          "pop_var_context: head of shell_variables not a function context"));
      return;
    }

  if ((ret = vcxt->down))
    {
      ret->up = nullptr;
      shell_variables = ret;
      if (vcxt->table)
        hash_flush (vcxt->table, push_func_var);
      dispose_var_context (vcxt);
    }
  else
    internal_error (_ ("pop_var_context: no global_variables context"));
}

static void
delete_local_contexts (VAR_CONTEXT *vcxt)
{
  VAR_CONTEXT *v, *t;

  for (v = vcxt; v != global_variables; v = t)
    {
      t = v->down;
      dispose_var_context (v);
    }
}

/* Delete the HASH_TABLEs for all variable contexts beginning at VCXT, and
   all of the VAR_CONTEXTs except GLOBAL_VARIABLES. */
void delete_all_contexts (vcxt) VAR_CONTEXT *vcxt;
{
  delete_local_contexts (vcxt);
  delete_all_variables (global_variables->table);
  shell_variables = global_variables;
}

/* Reset the context so we are not executing in a shell function. Only call
   this if you are getting ready to exit the shell. */
void
reset_local_contexts ()
{
  delete_local_contexts (shell_variables);
  shell_variables = global_variables;
  variable_context = 0;
}

/* **************************************************************** */
/*								    */
/*	   Pushing and Popping temporary variable scopes	    */
/*								    */
/* **************************************************************** */

VAR_CONTEXT *
push_scope (int flags, HASH_TABLE *tmpvars)
{
  return push_var_context (nullptr, flags, tmpvars);
}

static void
push_exported_var (void *data)
{
  SHELL_VAR *var, *v;

  var = (SHELL_VAR *)data;

  /* If a temp var had its export attribute set, or it's marked to be
     propagated, bind it in the previous scope before disposing it. */
  /* XXX - This isn't exactly right, because all tempenv variables have the
    export attribute set. */
  if (tempvar_p (var) && var->exported () && (var->attributes & att_propagate))
    {
      var->attributes &= ~att_tempvar; /* XXX */
      v = bind_variable_internal (var->name, value_cell (var),
                                  shell_variables->table, 0, 0);
      if (shell_variables == global_variables)
        var->attributes &= ~att_propagate;
      if (v)
        {
          v->attributes |= var->attributes;
          v->context = shell_variables->scope;
        }
    }
  else
    stupidly_hack_special_variables (var->name); /* XXX */

  dispose_variable (var);
}

/* This is called to propagate variables in the temporary environment of a
   special builtin (if IS_SPECIAL != 0) or exported variables that are the
   result of a builtin like `source' or `command' that can operate on the
   variables in its temporary environment. In the first case, we call
   push_builtin_var, which does the right thing. */
int
pop_scope (int is_special)
{
  VAR_CONTEXT *vcxt, *ret;
  int is_bltinenv;

  vcxt = shell_variables;
  if (vc_istempscope (vcxt) == 0)
    {
      internal_error (_ ("pop_scope: head of shell_variables not a temporary "
                         "environment scope"));
      return 0; // unused
    }
  is_bltinenv = vc_isbltnenv (vcxt); /* XXX - for later */

  ret = vcxt->down;
  if (ret)
    ret->up = nullptr;

  shell_variables = ret;

  /* Now we can take care of merging variables in VCXT into set of scopes
     whose head is RET (shell_variables). */
  FREE (vcxt->name);
  if (vcxt->table)
    {
      if (is_special)
        hash_flush (vcxt->table, push_builtin_var);
      else
        hash_flush (vcxt->table, push_exported_var);
      hash_dispose (vcxt->table);
    }
  free (vcxt);

  sv_ifs ("IFS"); /* XXX here for now */
  return 0;       // unused
}

/* **************************************************************** */
/*								    */
/*		 Pushing and Popping function contexts		    */
/*								    */
/* **************************************************************** */

struct saved_dollar_vars
{
  char **first_ten;
  WORD_LIST *rest;
  int count;
};

#if 0
static struct saved_dollar_vars *dollar_arg_stack = nullptr;
static int dollar_arg_stack_slots;
static int dollar_arg_stack_index;
#endif

/* Functions to manipulate dollar_vars array. Need to keep these in sync with
   whatever remember_args() does. */
static char **
save_dollar_vars ()
{
  char **ret;
  int i;

  ret = strvec_create (10);
  for (i = 1; i < 10; i++)
    {
      ret[i] = dollar_vars[i];
      dollar_vars[i] = nullptr;
    }
  return ret;
}

static void
restore_dollar_vars (char **args)
{
  int i;

  for (i = 1; i < 10; i++)
    dollar_vars[i] = args[i];
}

static void
free_dollar_vars ()
{
  int i;

  for (i = 1; i < 10; i++)
    {
      FREE (dollar_vars[i]);
      dollar_vars[i] = nullptr;
    }
}

static void
free_saved_dollar_vars (char **args)
{
  int i;

  for (i = 1; i < 10; i++)
    FREE (args[i]);
}

/* Do what remember_args (xxx, 1) would have done. */
void
clear_dollar_vars ()
{
  free_dollar_vars ();
  dispose_words (rest_of_args);

  rest_of_args = nullptr;
  posparam_count = 0;
}

/* XXX - should always be followed by remember_args () */
void
push_context (const char *name, bool is_subshell, HASH_TABLE *tempvars)
{
  if (!is_subshell)
    push_dollar_vars ();
  variable_context++;
  push_var_context (name, VC_FUNCENV, tempvars);
}

/* Only called when subshell == 0, so we don't need to check, and can
   unconditionally pop the dollar vars off the stack. */
void
pop_context ()
{
  pop_dollar_vars ();
  variable_context--;
  pop_var_context ();

  sv_ifs ("IFS"); /* XXX here for now */
}

/* Save the existing positional parameters on a stack. */
void
push_dollar_vars ()
{
  if (dollar_arg_stack_index + 2 > dollar_arg_stack_slots)
    {
      dollar_arg_stack = (struct saved_dollar_vars *)xrealloc (
          dollar_arg_stack,
          (dollar_arg_stack_slots += 10) * sizeof (struct saved_dollar_vars));
    }

  dollar_arg_stack[dollar_arg_stack_index].count = posparam_count;
  dollar_arg_stack[dollar_arg_stack_index].first_ten = save_dollar_vars ();
  dollar_arg_stack[dollar_arg_stack_index++].rest = rest_of_args;
  rest_of_args = nullptr;
  posparam_count = 0;

  dollar_arg_stack[dollar_arg_stack_index].first_ten = nullptr;
  dollar_arg_stack[dollar_arg_stack_index].rest = nullptr;
}

/* Restore the positional parameters from our stack. */
void
pop_dollar_vars ()
{
  if (dollar_arg_stack == 0 || dollar_arg_stack_index == 0)
    return;

  /* Wipe out current values */
  clear_dollar_vars ();

  rest_of_args = dollar_arg_stack[--dollar_arg_stack_index].rest;
  restore_dollar_vars (dollar_arg_stack[dollar_arg_stack_index].first_ten);
  free (dollar_arg_stack[dollar_arg_stack_index].first_ten);
  posparam_count = dollar_arg_stack[dollar_arg_stack_index].count;

  dollar_arg_stack[dollar_arg_stack_index].first_ten = nullptr;
  dollar_arg_stack[dollar_arg_stack_index].rest = nullptr;
  dollar_arg_stack[dollar_arg_stack_index].count = 0;

  set_dollar_vars_unchanged ();
  invalidate_cached_quoted_dollar_at ();
}

void
dispose_saved_dollar_vars ()
{
  if (dollar_arg_stack == 0 || dollar_arg_stack_index == 0)
    return;

  dispose_words (dollar_arg_stack[--dollar_arg_stack_index].rest);
  free_saved_dollar_vars (dollar_arg_stack[dollar_arg_stack_index].first_ten);
  free (dollar_arg_stack[dollar_arg_stack_index].first_ten);

  dollar_arg_stack[dollar_arg_stack_index].first_ten = nullptr;
  dollar_arg_stack[dollar_arg_stack_index].rest = nullptr;
  dollar_arg_stack[dollar_arg_stack_index].count = 0;
}

/* Initialize BASH_ARGV and BASH_ARGC after turning on extdebug after the
   shell is initialized */
void
init_bash_argv ()
{
  if (bash_argv_initialized == 0)
    {
      save_bash_argv ();
      bash_argv_initialized = 1;
    }
}

void
save_bash_argv ()
{
  WORD_LIST *list;

  list = list_rest_of_args ();
  push_args (list);
  dispose_words (list);
}

/* Manipulate the special BASH_ARGV and BASH_ARGC variables. */

void
push_args (WORD_LIST *list)
{
#if defined(ARRAY_VARS) && defined(DEBUGGER)
  SHELL_VAR *bash_argv_v, *bash_argc_v;
  ARRAY *bash_argv_a, *bash_argc_a;
  WORD_LIST *l;
  arrayind_t i;
  char *t;

  GET_ARRAY_FROM_VAR ("BASH_ARGV", bash_argv_v, bash_argv_a);
  GET_ARRAY_FROM_VAR ("BASH_ARGC", bash_argc_v, bash_argc_a);

  for (l = list, i = 0; l; l = (WORD_LIST *)(l->next), i++)
    array_push (bash_argv_a, l->word->word);

  t = itos (i);
  array_push (bash_argc_a, t);
  free (t);
#endif /* ARRAY_VARS && DEBUGGER */
}

/* Remove arguments from BASH_ARGV array.  Pop top element off BASH_ARGC
   array and use that value as the count of elements to remove from
   BASH_ARGV. */
void
pop_args ()
{
#if defined(ARRAY_VARS) && defined(DEBUGGER)
  SHELL_VAR *bash_argv_v, *bash_argc_v;
  ARRAY *bash_argv_a, *bash_argc_a;
  ARRAY_ELEMENT *ce;
  int64_t i;

  GET_ARRAY_FROM_VAR ("BASH_ARGV", bash_argv_v, bash_argv_a);
  GET_ARRAY_FROM_VAR ("BASH_ARGC", bash_argc_v, bash_argc_a);

  ce = array_unshift_element (bash_argc_a);
  if (ce == 0 || legal_number (element_value (ce), &i) == 0)
    i = 0;

  for (; i > 0; i--)
    array_pop (bash_argv_a);
  array_dispose_element (ce);
#endif /* ARRAY_VARS && DEBUGGER */
}

/*************************************************
 *						 *
 *	Functions to manage special variables	 *
 *						 *
 *************************************************/

/* Extern declarations for variables this code has to manage. */

/* An alist of name.function for each special variable.  Most of the
   functions don't do much, and in fact, this would be faster with a
   switch statement, but by the end of this file, I am sick of switch
   statements. */

#define SET_INT_VAR(name, intvar) intvar = find_variable (name) != 0

/* This table will be sorted with qsort() the first time it's accessed. */
struct name_and_function
{
  const char *name;
  sh_sv_func_t *function;
};

static struct name_and_function special_vars[]
    = { { "BASH_COMPAT", sv_shcompat },
        { "BASH_XTRACEFD", sv_xtracefd },

#if defined(JOB_CONTROL)
        { "CHILD_MAX", sv_childmax },
#endif

#if defined(READLINE)
#if defined(STRICT_POSIX)
        { "COLUMNS", sv_winsize },
#endif
        { "COMP_WORDBREAKS", sv_comp_wordbreaks },
#endif

        { "EXECIGNORE", sv_execignore },

        { "FUNCNEST", sv_funcnest },

        { "GLOBIGNORE", sv_globignore },

#if defined(HISTORY)
        { "HISTCONTROL", sv_history_control },
        { "HISTFILESIZE", sv_histsize },
        { "HISTIGNORE", sv_histignore },
        { "HISTSIZE", sv_histsize },
        { "HISTTIMEFORMAT", sv_histtimefmt },
#endif

#if defined(__CYGWIN__)
        { "HOME", sv_home },
#endif

#if defined(READLINE)
        { "HOSTFILE", sv_hostfile },
#endif

        { "IFS", sv_ifs },
        { "IGNOREEOF", sv_ignoreeof },

        { "LANG", sv_locale },
        { "LC_ALL", sv_locale },
        { "LC_COLLATE", sv_locale },
        { "LC_CTYPE", sv_locale },
        { "LC_MESSAGES", sv_locale },
        { "LC_NUMERIC", sv_locale },
        { "LC_TIME", sv_locale },

#if defined(READLINE) && defined(STRICT_POSIX)
        { "LINES", sv_winsize },
#endif

        { "MAIL", sv_mail },
        { "MAILCHECK", sv_mail },
        { "MAILPATH", sv_mail },

        { "OPTERR", sv_opterr },
        { "OPTIND", sv_optind },

        { "PATH", sv_path },
        { "POSIXLY_CORRECT", sv_strict_posix },

#if defined(READLINE)
        { "TERM", sv_terminal },
        { "TERMCAP", sv_terminal },
        { "TERMINFO", sv_terminal },
#endif /* READLINE */

        { "TEXTDOMAIN", sv_locale },
        { "TEXTDOMAINDIR", sv_locale },

#if defined(HAVE_TZSET)
        { "TZ", sv_tz },
#endif

#if defined(HISTORY) && defined(BANG_HISTORY)
        { "histchars", sv_histchars },
#endif /* HISTORY && BANG_HISTORY */

        { "ignoreeof", sv_ignoreeof },

        { 0, (sh_sv_func_t *)0 } };

#define N_SPECIAL_VARS (sizeof (special_vars) / sizeof (special_vars[0]) - 1)

static int
sv_compare (struct name_and_function *sv1, struct name_and_function *sv2)
{
  int r;

  if ((r = sv1->name[0] - sv2->name[0]) == 0)
    r = strcmp (sv1->name, sv2->name);
  return r;
}

static inline int
find_special_var (const char *name)
{
  int i, r;

  for (i = 0; special_vars[i].name; i++)
    {
      r = special_vars[i].name[0] - name[0];
      if (r == 0)
        r = strcmp (special_vars[i].name, name);
      if (r == 0)
        return i;
      else if (r > 0)
        /* Can't match any of rest of elements in sorted list.  Take this out
           if it causes problems in certain environments. */
        break;
    }
  return -1;
}

/* The variable in NAME has just had its state changed.  Check to see if it
   is one of the special ones where something special happens. */
void
stupidly_hack_special_variables (const char *name)
{
  static int sv_sorted = 0;
  int i;

  if (sv_sorted == 0) /* shouldn't need, but it's fairly cheap. */
    {
      qsort (special_vars, N_SPECIAL_VARS, sizeof (special_vars[0]),
             (QSFUNC *)sv_compare);
      sv_sorted = 1;
    }

  i = find_special_var (name);
  if (i != -1)
    (*(special_vars[i].function)) (name);
}

/* Special variables that need hooks to be run when they are unset as part
   of shell reinitialization should have their sv_ functions run here. */
void
reinit_special_variables ()
{
#if defined(READLINE)
  sv_comp_wordbreaks ("COMP_WORDBREAKS");
#endif
  sv_globignore ("GLOBIGNORE");
  sv_opterr ("OPTERR");
}

void
sv_ifs (const char *ignored)
{
  SHELL_VAR *v;

  v = find_variable ("IFS");
  setifs (v);
}

/* What to do just after the PATH variable has changed. */
void
sv_path (const char *ignored)
{
  /* hash -r */
  phash_flush ();
}

/* What to do just after one of the MAILxxxx variables has changed.  NAME
   is the name of the variable.  This is called with NAME set to one of
   MAIL, MAILCHECK, or MAILPATH.  */
void
sv_mail (const char *name)
{
  /* If the time interval for checking the files has changed, then
     reset the mail timer.  Otherwise, one of the pathname vars
     to the users mailbox has changed, so rebuild the array of
     filenames. */
  if (name[4] == 'C') /* if (strcmp (name, "MAILCHECK") == 0) */
    reset_mail_timer ();
  else
    {
      free_mail_files ();
      remember_mail_dates ();
    }
}

void
sv_funcnest (const char *name)
{
  SHELL_VAR *v;
  int64_t num;

  v = find_variable (name);
  if (v == 0)
    funcnest_max = 0;
  else if (legal_number (value_cell (v), &num) == 0)
    funcnest_max = 0;
  else
    funcnest_max = num;
}

/* What to do when EXECIGNORE changes. */
void
sv_execignore (const char *name)
{
  setup_exec_ignore ();
}

/* What to do when GLOBIGNORE changes. */
void
sv_globignore (const char *name)
{
  if (privileged_mode == 0)
    setup_glob_ignore (name);
}

#if defined(READLINE)
void
sv_comp_wordbreaks (const char *name)
{
  SHELL_VAR *sv;

  sv = find_variable (name);
  if (sv == 0)
    reset_completer_word_break_chars ();
}

/* What to do just after one of the TERMxxx variables has changed.
   If we are an interactive shell, then try to reset the terminal
   information in readline. */
void
sv_terminal (const char *name)
{
  if (interactive_shell && no_line_editing == 0)
    rl_reset_terminal (get_string_value ("TERM"));
}

void
sv_hostfile (const char *name)
{
  SHELL_VAR *v;

  v = find_variable (name);
  if (v == 0)
    clear_hostname_list ();
  else
    hostname_list_initialized = 0;
}

#if defined(STRICT_POSIX)
/* In strict posix mode, we allow assignments to LINES and COLUMNS (and values
   found in the initial environment) to override the terminal size reported by
   the kernel. */
void
sv_winsize (const char *name)
{
  SHELL_VAR *v;
  int64_t xd;
  int d;

  if (posixly_correct == 0 || interactive_shell == 0 || no_line_editing)
    return;

  v = find_variable (name);
  if (v == 0 || var_isset (v) == 0)
    rl_reset_screen_size ();
  else
    {
      if (legal_number (value_cell (v), &xd) == 0)
        return;
      winsize_assignment = 1;
      d = xd;             /* truncate */
      if (name[0] == 'L') /* LINES */
        rl_set_screen_size (d, -1);
      else /* COLUMNS */
        rl_set_screen_size (-1, d);
      winsize_assignment = 0;
    }
}
#endif /* STRICT_POSIX */
#endif /* READLINE */

/* Update the value of HOME in the export environment so tilde expansion will
   work on cygwin. */
#if defined(__CYGWIN__)
sv_home (const char *name)
{
  array_needs_making = true;
  maybe_make_export_env ();
}
#endif

#if defined(HISTORY)
/* What to do after the HISTSIZE or HISTFILESIZE variables change.
   If there is a value for this HISTSIZE (and it is numeric), then stifle
   the history.  Otherwise, if there is NO value for this variable,
   unstifle the history.  If name is HISTFILESIZE, and its value is
   numeric, truncate the history file to hold no more than that many
   lines. */
void
sv_histsize (const char *name)
{
  char *temp;
  int64_t num;
  int hmax;

  temp = get_string_value (name);

  if (temp && *temp)
    {
      if (legal_number (temp, &num))
        {
          hmax = num;
          if (hmax < 0 && name[4] == 'S')
            unstifle_history (); /* unstifle history if HISTSIZE < 0 */
          else if (name[4] == 'S')
            {
              stifle_history (hmax);
              hmax = where_history ();
              if (history_lines_this_session > hmax)
                history_lines_this_session = hmax;
            }
          else if (hmax >= 0) /* truncate HISTFILE if HISTFILESIZE >= 0 */
            {
              history_truncate_file (get_string_value ("HISTFILE"), hmax);
              /* If we just shrank the history file to fewer lines than we've
                 already read, make sure we adjust our idea of how many lines
                 we have read from the file. */
              if (hmax < history_lines_in_file)
                history_lines_in_file = hmax;
            }
        }
    }
  else if (name[4] == 'S')
    unstifle_history ();
}

/* What to do after the HISTIGNORE variable changes. */
void
sv_histignore (const char *name)
{
  setup_history_ignore ();
}

/* What to do after the HISTCONTROL variable changes. */
void
sv_history_control (const char *name)
{
  char *temp;
  char *val;
  int tptr;

  history_control = 0;
  temp = get_string_value (name);

  if (temp == 0 || *temp == 0)
    return;

  tptr = 0;
  while ((val = extract_colon_unit (temp, &tptr)))
    {
      if (STREQ (val, "ignorespace"))
        history_control |= HC_IGNSPACE;
      else if (STREQ (val, "ignoredups"))
        history_control |= HC_IGNDUPS;
      else if (STREQ (val, "ignoreboth"))
        history_control |= HC_IGNBOTH;
      else if (STREQ (val, "erasedups"))
        history_control |= HC_ERASEDUPS;

      free (val);
    }
}

#if defined(BANG_HISTORY)
/* Setting/unsetting of the history expansion character. */
void
sv_histchars (const char *name)
{
  char *temp;

  temp = get_string_value (name);
  if (temp)
    {
      history_expansion_char = *temp;
      if (temp[0] && temp[1])
        {
          history_subst_char = temp[1];
          if (temp[2])
            history_comment_char = temp[2];
        }
    }
  else
    {
      history_expansion_char = '!';
      history_subst_char = '^';
      history_comment_char = '#';
    }
}
#endif /* BANG_HISTORY */

void
sv_histtimefmt (const char *name)
{
  SHELL_VAR *v;

  if ((v = find_variable (name)))
    {
      if (history_comment_char == 0)
        history_comment_char = '#';
    }
  history_write_timestamps = (v != 0);
}
#endif /* HISTORY */

#if defined(HAVE_TZSET)
void
sv_tz (const char *name)
{
  SHELL_VAR *v;

  v = find_variable (name);
  if (v && exported_p (v))
    array_needs_making = true;
  else if (v == 0)
    array_needs_making = true;

  if (array_needs_making)
    {
      maybe_make_export_env ();
      tzset ();
    }
}
#endif

/* If the variable exists, then the value of it can be the number
   of times we actually ignore the EOF.  The default is small,
   (smaller than csh, anyway). */
void
sv_ignoreeof (const char *name)
{
  SHELL_VAR *tmp_var;
  char *temp;

  eof_encountered = 0;

  tmp_var = find_variable (name);
  ignoreeof = tmp_var && var_isset (tmp_var);
  temp = tmp_var ? value_cell (tmp_var) : nullptr;
  if (temp)
    eof_encountered_limit = (*temp && all_digits (temp)) ? atoi (temp) : 10;
  set_shellopts (); /* make sure `ignoreeof' is/is not in $SHELLOPTS */
}

void
sv_optind (const char *name)
{
  SHELL_VAR *var;
  char *tt;
  int s;

  var = find_variable ("OPTIND");
  tt = var ? get_variable_value (var) : nullptr;

  /* Assume that if var->context < variable_context and variable_context > 0
     then we are restoring the variables's previous state while returning
     from a function. */
  if (tt && *tt)
    {
      s = atoi (tt);

      /* According to POSIX, setting OPTIND=1 resets the internal state
         of getopt (). */
      if (s < 0 || s == 1)
        s = 0;
    }
  else
    s = 0;
  getopts_reset (s);
}

void
sv_opterr (const char *name)
{
  char *tt;

  tt = get_string_value ("OPTERR");
  sh_opterr = (tt && *tt) ? atoi (tt) : 1;
}

void
sv_strict_posix (const char *name)
{
  SHELL_VAR *var;

  var = find_variable (name);
  posixly_correct = var && var_isset (var);
  posix_initialize (posixly_correct);
#if defined(READLINE)
  if (interactive_shell)
    posix_readline_initialize (posixly_correct);
#endif              /* READLINE */
  set_shellopts (); /* make sure `posix' is/is not in $SHELLOPTS */
}

void
sv_locale (const char *name)
{
  char *v;
  int r;

  v = get_string_value (name);
  if (name[0] == 'L' && name[1] == 'A') /* LANG */
    r = set_lang (name, v);
  else
    r = set_locale_var (name, v); /* LC_*, TEXTDOMAIN* */

#if 1
  if (r == 0 && posixly_correct)
    set_exit_status (EXECUTION_FAILURE);
#endif
}

#if defined(ARRAY_VARS)
void
set_pipestatus_array (int *ps, int nproc)
{
  SHELL_VAR *v;
  ARRAY *a;
  ARRAY_ELEMENT *ae;
  int i;
  char *t, tbuf[INT_STRLEN_BOUND (int) + 1];

  v = find_variable ("PIPESTATUS");
  if (v == 0)
    v = make_new_array_variable ("PIPESTATUS"); /* XXX can this be freed? */
  if (array_p (v) == 0)
    return; /* Do nothing if not an array variable. */
  a = array_cell (v);

  if (a == 0 || array_num_elements (a) == 0)
    {
      for (i = 0; i < nproc; i++) /* was ps[i] != -1, not i < nproc */
        {
          t = inttostr (ps[i], tbuf, sizeof (tbuf));
          array_insert (a, i, t);
        }
      return;
    }

  /* Fast case */
  if (array_num_elements (a) == nproc && nproc == 1)
    {
#ifndef ALT_ARRAY_IMPLEMENTATION
      ae = element_forw (a->head);
#else
      ae = a->elements[0];
#endif
      ARRAY_ELEMENT_REPLACE (ae, itos (ps[0]));
    }
  else if (array_num_elements (a) <= nproc)
    {
      /* modify in array_num_elements members in place, then add */
#ifndef ALT_ARRAY_IMPLEMENTATION
      ae = a->head;
#endif
      for (i = 0; i < array_num_elements (a); i++)
        {
#ifndef ALT_ARRAY_IMPLEMENTATION
          ae = element_forw (ae);
#else
          ae = a->elements[i];
#endif
          ARRAY_ELEMENT_REPLACE (ae, itos (ps[i]));
        }
      /* add any more */
      for (; i < nproc; i++)
        {
          t = inttostr (ps[i], tbuf, sizeof (tbuf));
          array_insert (a, i, t);
        }
    }
  else
    {
#ifndef ALT_ARRAY_IMPLEMENTATION
      /* deleting elements.  it's faster to rebuild the array. */
      array_flush (a);
      for (i = 0; i < nproc; i++)
        {
          t = inttostr (ps[i], tbuf, sizeof (tbuf));
          array_insert (a, i, t);
        }
#else
      /* deleting elements. replace the first NPROC, free the rest */
      for (i = 0; i < nproc; i++)
        {
          ae = a->elements[i];
          ARRAY_ELEMENT_REPLACE (ae, itos (ps[i]));
        }
      for (; i <= array_max_index (a); i++)
        {
          array_dispose_element (a->elements[i]);
          a->elements[i] = nullptr;
        }

      /* bookkeeping usually taken care of by array_insert */
      set_max_index (a, nproc - 1);
      set_first_index (a, 0);
      set_num_elements (a, nproc);
#endif /* ALT_ARRAY_IMPLEMENTATION */
    }
}

ARRAY *
save_pipestatus_array ()
{
  SHELL_VAR *v;
  ARRAY *a;

  v = find_variable ("PIPESTATUS");
  if (v == 0 || array_p (v) == 0 || array_cell (v) == 0)
    return nullptr;

  a = array_copy (array_cell (v));

  return a;
}

void
restore_pipestatus_array (ARRAY *a)
{
  SHELL_VAR *v;
  ARRAY *a2;

  v = find_variable ("PIPESTATUS");
  /* XXX - should we still assign even if existing value is nullptr? */
  if (v == 0 || array_p (v) == 0 || array_cell (v) == 0)
    return;

  a2 = array_cell (v);
  var_setarray (v, a);

  array_dispose (a2);
}
#endif

void
set_pipestatus_from_exit (int s)
{
#if defined(ARRAY_VARS)
  static int v[2] = { 0, -1 };

  v[0] = s;
  set_pipestatus_array (v, 1);
#endif
}

void
sv_xtracefd (const char *name)
{
  SHELL_VAR *v;
  char *t, *e;
  int fd;
  FILE *fp;

  v = find_variable (name);
  if (v == 0)
    {
      xtrace_reset ();
      return;
    }

  t = value_cell (v);
  if (t == 0 || *t == 0)
    xtrace_reset ();
  else
    {
      fd = (int)strtol (t, &e, 10);
      if (e != t && *e == '\0' && sh_validfd (fd))
        {
          fp = fdopen (fd, "w");
          if (fp == 0)
            internal_error (_ ("%s: %s: cannot open as FILE"), name,
                            value_cell (v));
          else
            xtrace_set (fd, fp);
        }
      else
        internal_error (_ ("%s: %s: invalid value for trace file descriptor"),
                        name, value_cell (v));
    }
}

#define MIN_COMPAT_LEVEL 31

void
sv_shcompat (const char *name)
{
  SHELL_VAR *v;
  char *val;
  int tens, ones, compatval;

  v = find_variable (name);
  if (v == 0)
    {
      shell_compatibility_level = DEFAULT_COMPAT_LEVEL;
      set_compatibility_opts ();
      return;
    }
  val = value_cell (v);
  if (val == 0 || *val == '\0')
    {
      shell_compatibility_level = DEFAULT_COMPAT_LEVEL;
      set_compatibility_opts ();
      return;
    }
  /* Handle decimal-like compatibility version specifications: 4.2 */
  if (ISDIGIT (val[0]) && val[1] == '.' && ISDIGIT (val[2]) && val[3] == 0)
    {
      tens = val[0] - '0';
      ones = val[2] - '0';
      compatval = tens * 10 + ones;
    }
  /* Handle integer-like compatibility version specifications: 42 */
  else if (ISDIGIT (val[0]) && ISDIGIT (val[1]) && val[2] == 0)
    {
      tens = val[0] - '0';
      ones = val[1] - '0';
      compatval = tens * 10 + ones;
    }
  else
    {
    compat_error:
      internal_error (_ ("%s: %s: compatibility value out of range"), name,
                      val);
      shell_compatibility_level = DEFAULT_COMPAT_LEVEL;
      set_compatibility_opts ();
      return;
    }

  if (compatval < MIN_COMPAT_LEVEL || compatval > DEFAULT_COMPAT_LEVEL)
    goto compat_error;

  shell_compatibility_level = compatval;
  set_compatibility_opts ();
}

#if defined(JOB_CONTROL)
void
sv_childmax (const char *name)
{
  char *tt;
  int s;

  tt = get_string_value (name);
  s = (tt && *tt) ? atoi (tt) : 0;
  set_maxchild (s);
}
#endif

} // namespace bash
