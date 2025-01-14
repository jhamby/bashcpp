// This file is type_def.cc.
// It implements the builtin "type" in Bash.

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

// $BUILTIN type
// $FUNCTION type_builtin
// $SHORT_DOC type [-afptP] name [name ...]
// Display information about command type.

// For each NAME, indicate how it would be interpreted if used as a
// command name.

// Options:
//   -a	display all locations containing an executable named NAME;
// 		includes aliases, builtins, and functions, if and only if
// 		the `-p' option is not also used
//   -f	suppress shell function lookup
//   -P	force a PATH search for each NAME, even if it is an alias,
// 		builtin, or function, and returns the name of the disk file
// 		that would be executed
//   -p	returns either the name of the disk file that would be executed,
// 		or nothing if `type -t NAME' would not return `file'
//   -t	output a single word which is one of `alias', `keyword',
// 		`function', `builtin', `file' or `', if NAME is an alias,
// 		shell reserved word, shell function, shell builtin, disk file,
// 		or not found, respectively

// Arguments:
//   NAME	Command name to be interpreted.

// Exit Status:
// Returns success if all of the NAMEs are found; fails if any are not found.
// $END

#include "config.h"

#include "bashtypes.hh"
#include "posixstat.hh"

#include <unistd.h>

#include "shell.hh"

#if defined(ALIAS)
#include "alias.hh"
#endif /* ALIAS */

#include "common.hh"

namespace bash
{

// extern int find_reserved_word (char *);

/* For each word in LIST, find out what the shell is going to do with
   it as a simple command. i.e., which file would this shell use to
   execve, or if it is a builtin command, or an alias.  Possible flag
   arguments:
        -t		Returns the "type" of the object, one of
                        `alias', `keyword', `function', `builtin',
                        or `file'.

        -p		Returns the pathname of the file if -type is
                        a file.

        -a		Returns all occurrences of words, whether they
                        be a filename in the path, alias, function,
                        or builtin.

        -f		Suppress shell function lookup, like `command'.

        -P		Force a path search even in the presence of other
                        definitions.

   Order of evaluation:
        alias
        keyword
        function
        builtin
        file
 */

int
Shell::type_builtin (WORD_LIST *list)
{
  if (list == 0)
    return EXECUTION_SUCCESS;

  int dflags = CDESC_SHORTDESC; /* default */
  bool any_failed = false;

  /* Handle the obsolescent `-type', `-path', and `-all' by prescanning
     the arguments and converting those options to the form that
     internal_getopt recognizes. Converts `--type', `--path', and `--all'
     also. THIS SHOULD REALLY GO AWAY. */
  for (WORD_LIST *l = list; l && l->word->word[0] == '-'; l = l->next ())
    {
      char *flag = &(l->word->word[1]);

      if (STREQ (flag, "type") || STREQ (flag, "-type"))
        {
          l->word->word[1] = 't';
          l->word->word[2] = '\0';
        }
      else if (STREQ (flag, "path") || STREQ (flag, "-path"))
        {
          l->word->word[1] = 'p';
          l->word->word[2] = '\0';
        }
      else if (STREQ (flag, "all") || STREQ (flag, "-all"))
        {
          l->word->word[1] = 'a';
          l->word->word[2] = '\0';
        }
    }

  reset_internal_getopt ();
  int opt;
  while ((opt = internal_getopt (list, "afptP")) != -1)
    {
      switch (opt)
        {
        case 'a':
          dflags |= CDESC_ALL;
          break;
        case 'f':
          dflags |= CDESC_NOFUNCS;
          break;
        case 'p':
          dflags |= CDESC_PATH_ONLY;
          dflags &= ~(CDESC_TYPE | CDESC_SHORTDESC);
          break;
        case 't':
          dflags |= CDESC_TYPE;
          dflags &= ~(CDESC_PATH_ONLY | CDESC_SHORTDESC);
          break;
        case 'P': /* shorthand for type -ap */
          dflags |= (CDESC_PATH_ONLY | CDESC_FORCE_PATH);
          dflags &= ~(CDESC_TYPE | CDESC_SHORTDESC);
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  while (list)
    {
      bool found = describe_command (list->word->word, dflags);

      if (!found && (dflags & (CDESC_PATH_ONLY | CDESC_TYPE)) == 0)
        sh_notfound (list->word->word);

      any_failed |= (found == 0);
      list = list->next ();
    }

  opt = (!any_failed) ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
  return sh_chkwrite (opt);
}

/*
 * Describe COMMAND as required by the type and command builtins.
 *
 * Behavior is controlled by DFLAGS.  Flag values are
 *	CDESC_ALL	print all descriptions of a command
 *	CDESC_SHORTDESC	print the description for type and command -V
 *	CDESC_REUSABLE	print in a format that may be reused as input
 *	CDESC_TYPE	print the type for type -t
 *	CDESC_PATH_ONLY	print the path for type -p
 *	CDESC_FORCE_PATH	force a path search for type -P
 *	CDESC_NOFUNCS	skip function lookup for type -f
 *	CDESC_ABSPATH	convert to absolute path, no ./ prefix
 *	CDESC_STDPATH	command -p standard path list
 *
 * CDESC_ALL says whether or not to look for all occurrences of COMMAND, or
 * return after finding it once.
 */
bool
Shell::describe_command (const char *command, int dflags)
{
  bool all = (dflags & CDESC_ALL) != 0;
  bool found = false, found_file = false;
  char *full_path = (char *)NULL;

#if defined(ALIAS)
  /* Command is an alias? */
  alias_t *alias;
  if (((dflags & CDESC_FORCE_PATH) == 0) && expand_aliases
      && (alias = find_alias (command)))
    {
      if (dflags & CDESC_TYPE)
        puts ("alias");
      else if (dflags & CDESC_SHORTDESC)
        printf (_ ("%s is aliased to `%s'\n"), command, alias->value);
      else if (dflags & CDESC_REUSABLE)
        {
          char *x = sh_single_quote (alias->value);
          printf ("alias %s=%s\n", command, x);
          free (x);
        }

      found = true;

      if (!all)
        return true;
    }
#endif /* ALIAS */

  /* Command is a shell reserved word? */
  if (((dflags & CDESC_FORCE_PATH) == 0) && find_reserved_word (command) >= 0)
    {
      if (dflags & CDESC_TYPE)
        puts ("keyword");
      else if (dflags & CDESC_SHORTDESC)
        printf (_ ("%s is a shell keyword\n"), command);
      else if (dflags & CDESC_REUSABLE)
        printf ("%s\n", command);

      found = true;

      if (!all)
        return true;
    }

  /* Command is a function? */
  SHELL_VAR *func;
  if (((dflags & (CDESC_FORCE_PATH | CDESC_NOFUNCS)) == 0)
      && (func = find_function (command)))
    {
      if (dflags & CDESC_TYPE)
        puts ("function");
      else if (dflags & CDESC_SHORTDESC)
        {
          printf (_ ("%s is a function\n"), command);

          /* We're blowing away THE_PRINTED_COMMAND here... */

          char *result = named_function_string (
              command, function_cell (func), FUNC_MULTILINE | FUNC_EXTERNAL);
          printf ("%s\n", result);
        }
      else if (dflags & CDESC_REUSABLE)
        printf ("%s\n", command);

      found = true;

      if (!all)
        return true;
    }

  /* Command is a builtin? */
  if (((dflags & CDESC_FORCE_PATH) == 0) && find_shell_builtin (command))
    {
      if (dflags & CDESC_TYPE)
        puts ("builtin");
      else if (dflags & CDESC_SHORTDESC)
        {
          if (posixly_correct && find_special_builtin (command) != 0)
            printf (_ ("%s is a special shell builtin\n"), command);
          else
            printf (_ ("%s is a shell builtin\n"), command);
        }
      else if (dflags & CDESC_REUSABLE)
        printf ("%s\n", command);

      found = true;

      if (!all)
        return true;
    }

  /* Command is a disk file? */
  /* If the command name given is already an absolute command, just
     check to see if it is executable. */
  if (absolute_program (command))
    {
      int f = file_status (command);
      if (f & FS_EXECABLE)
        {
          if (dflags & CDESC_TYPE)
            puts ("file");
          else if (dflags & CDESC_SHORTDESC)
            printf (_ ("%s is %s\n"), command, command);
          else if (dflags & (CDESC_REUSABLE | CDESC_PATH_ONLY))
            printf ("%s\n", command);

          /* There's no use looking in the hash table or in $PATH,
             because they're not consulted when an absolute program
             name is supplied. */
          return true;
        }
    }

  /* If the user isn't doing "-a", then we might care about
     whether the file is present in our hash table. */
  if (!all || (dflags & CDESC_FORCE_PATH))
    {
      if ((full_path = phash_search (command)))
        {
          if (dflags & CDESC_TYPE)
            puts ("file");
          else if (dflags & CDESC_SHORTDESC)
            printf (_ ("%s is hashed (%s)\n"), command, full_path);
          else if (dflags & (CDESC_REUSABLE | CDESC_PATH_ONLY))
            printf ("%s\n", full_path);

          free (full_path);
          return true;
        }
    }

  /* Now search through $PATH. */
  while (1)
    {
      if (dflags & CDESC_STDPATH) /* command -p, all cannot be non-zero */
        {
          char *pathlist = conf_standard_path ();
          full_path = find_in_path (command, pathlist,
                                    FS_EXEC_PREFERRED | FS_NODIRS);
          free (pathlist);
          /* Will only go through this once, since all == 0 if STDPATH set */
        }
      else if (!all)
        full_path = find_user_command (command);
      else
        full_path = user_command_matches (
            command, FS_EXEC_ONLY,
            found_file); /* XXX - should that be FS_EXEC_PREFERRED? */

      if (full_path == 0)
        break;

      /* If we found the command as itself by looking through $PATH, it
         probably doesn't exist.  Check whether or not the command is an
         executable file.  If it's not, don't report a match.  This is
         the default posix mode behavior */
      if (STREQ (full_path, command) || posixly_correct)
        {
          int f = file_status (full_path);
          if ((f & FS_EXECABLE) == 0)
            {
              free (full_path);
              full_path = (char *)NULL;
              if (!all)
                break;
            }
          else if (ABSPATH (full_path))
            ; /* placeholder; don't need to do anything yet */
          else if (dflags
                   & (CDESC_REUSABLE | CDESC_PATH_ONLY | CDESC_SHORTDESC))
            {
              f = MP_DOCWD | ((dflags & CDESC_ABSPATH) ? MP_RMDOT : 0);
              char *x = sh_makepath ((char *)NULL, full_path, f);
              free (full_path);
              full_path = x;
            }
        }
      /* If we require a full path and don't have one, make one */
      else if ((dflags & CDESC_ABSPATH) && ABSPATH (full_path) == 0)
        {
          char *x = sh_makepath ((char *)NULL, full_path, MP_DOCWD | MP_RMDOT);
          free (full_path);
          full_path = x;
        }

      found_file = true;
      found = true;

      if (dflags & CDESC_TYPE)
        puts ("file");
      else if (dflags & CDESC_SHORTDESC)
        printf (_ ("%s is %s\n"), command, full_path);
      else if (dflags & (CDESC_REUSABLE | CDESC_PATH_ONLY))
        printf ("%s\n", full_path);

      free (full_path);
      full_path = (char *)NULL;

      if (!all)
        break;
    }

  return found;
}

} // namespace bash
