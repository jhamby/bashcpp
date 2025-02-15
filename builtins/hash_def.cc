// This file is hash_def.cc.
// It implements the builtin "hash" in Bash.

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

// $BUILTIN hash
// $FUNCTION hash_builtin
// $SHORT_DOC hash [-lr] [-p pathname] [-dt] [name ...]
// Remember or display program locations.

// Determine and remember the full pathname of each command NAME.  If
// no arguments are given, information about remembered commands is displayed.

// Options:
//   -d	forget the remembered location of each NAME
//   -l	display in a format that may be reused as input
//   -p pathname	use PATHNAME as the full pathname of NAME
//   -r	forget all remembered locations
//   -t	print the remembered location of each NAME, preceding
// 		each location with the corresponding NAME if multiple
// 		NAMEs are given
// Arguments:
//   NAME	Each NAME is searched for in $PATH and added to the list
// 		of remembered commands.

// Exit Status:
// Returns success unless NAME is not found or an invalid option is given.
// $END

#include "config.h"

#include "bashtypes.hh"

#include <unistd.h>

#include "builtins.hh"
#include "common.hh"
#include "flags.hh"
#include "hashcmd.hh"
#include "shell.hh"

namespace bash
{

#if 0
extern bool dot_found_in_search;

static int add_hashed_command (char *, int);
static int print_hash_info (BUCKET_CONTENTS *);
static int print_portable_hash_info (BUCKET_CONTENTS *);
static bool print_hashed_commands (bool);
static int list_hashed_filename_targets (WORD_LIST *, bool);
#endif

/* Print statistics on the current state of hashed commands.  If LIST is
   not empty, then rehash (or hash in the first place) the specified
   commands. */
int
Shell::hash_builtin (WORD_LIST *list)
{
  //   int expunge_hash_table, list_targets, list_portably, delete, opt;
  //   char *w, *pathname;

  if (!hashing_enabled)
    {
      builtin_error (_ ("hashing disabled"));
      return EXECUTION_FAILURE;
    }

  bool expunge_hash_table = false;
  bool list_targets = false;
  bool list_portably = false;
  bool delete_ = false;
  char *pathname = (char *)NULL;
  reset_internal_getopt ();
  int opt;
  while ((opt = internal_getopt (list, "dlp:rt")) != -1)
    {
      switch (opt)
        {
        case 'd':
          delete_ = true;
          break;
        case 'l':
          list_portably = true;
          break;
        case 'p':
          pathname = list_optarg;
          break;
        case 'r':
          expunge_hash_table = true;
          break;
        case 't':
          list_targets = true;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  /* hash -t requires at least one argument. */
  if (list == 0 && (delete_ || list_targets))
    {
      sh_needarg (delete_ ? "-d" : "-t");
      return EXECUTION_FAILURE;
    }

  /* We want hash -r to be silent, but hash -- to print hashing info, so
     we test expunge_hash_table. */
  if (list == 0 && !expunge_hash_table)
    {
      opt = print_hashed_commands (list_portably);
      if (!opt && !posixly_correct
          && (!list_portably || shell_compatibility_level <= 50))
        printf (_ ("%s: hash table empty\n"), this_command_name);

      return EXECUTION_SUCCESS;
    }

  if (expunge_hash_table)
    phash_flush ();

  /* If someone runs `hash -r -t xyz' he will be disappointed. */
  if (list_targets)
    return list_hashed_filename_targets (list, list_portably);

#if defined(RESTRICTED_SHELL)
  if (restricted && pathname)
    {
      if (strchr (pathname, '/'))
        {
          sh_restricted (pathname);
          return EXECUTION_FAILURE;
        }
      /* If we are changing the hash table in a restricted shell, make sure the
         target pathname can be found using a $PATH search. */
      char *w = find_user_command (pathname);
      if (w == 0 || *w == 0 || executable_file (w) == 0)
        {
          sh_notfound (pathname);
          free (w);
          return EXECUTION_FAILURE;
        }
      free (w);
    }
#endif

  for (opt = EXECUTION_SUCCESS; list; list = list->next ())
    {
      /* Add, remove or rehash the specified commands. */
      char *w = list->word->word;
      if (absolute_program (w))
        continue;
      else if (pathname)
        {
          if (is_directory (pathname))
            {
#ifdef EISDIR
              builtin_error ("%s: %s", pathname, strerror (EISDIR));
#else
              builtin_error (_ ("%s: is a directory"), pathname);
#endif
              opt = EXECUTION_FAILURE;
            }
          else
            phash_insert (w, pathname, 0, 0);
        }
      else if (delete_)
        {
          if (phash_remove (w))
            {
              sh_notfound (w);
              opt = EXECUTION_FAILURE;
            }
        }
      else if (add_hashed_command (w, 0))
        opt = EXECUTION_FAILURE;
    }

  fflush (stdout);
  return opt;
}

static int
add_hashed_command (char *w, int quiet)
{
  int rv;
  char *full_path;

  rv = 0;
  if (find_function (w) == 0 && find_shell_builtin (w) == 0)
    {
      phash_remove (w);
      full_path = find_user_command (w);
      if (full_path && executable_file (full_path))
        phash_insert (w, full_path, dot_found_in_search, 0);
      else
        {
          if (quiet == 0)
            sh_notfound (w);
          rv++;
        }
      FREE (full_path);
    }
  return rv;
}

/* Print information about current hashed info. */
static int
print_hash_info (BUCKET_CONTENTS *item)
{
  printf ("%4d\t%s\n", item->times_found, pathdata (item)->path);
  return 0;
}

static int
print_portable_hash_info (BUCKET_CONTENTS *item)
{
  char *fp, *fn;

  fp = printable_filename (pathdata (item)->path, 1);
  fn = printable_filename (item->key, 1);
  printf ("builtin hash -p %s %s\n", fp, fn);
  if (fp != pathdata (item)->path)
    free (fp);
  if (fn != item->key)
    free (fn);
  return 0;
}

static bool
print_hashed_commands (bool fmt)
{
  if (hashed_filenames == 0 || HASH_ENTRIES (hashed_filenames) == 0)
    return false;

  if (!fmt)
    printf (_ ("hits\tcommand\n"));
  hash_walk (hashed_filenames,
             fmt ? print_portable_hash_info : print_hash_info);
  return true;
}

static int
list_hashed_filename_targets (WORD_LIST *list, bool fmt)
{
  bool all_found = true;
  bool multiple = list->next != 0;

  for (WORD_LIST *l = list; l; l = (WORD_LIST *)l->next)
    {
      char *target = phash_search (l->word->word);
      if (target == 0)
        {
          all_found = false;
          sh_notfound (l->word->word);
          continue;
        }
      if (fmt)
        printf ("builtin hash -p %s %s\n", target, l->word->word);
      else
        {
          if (multiple)
            printf ("%s\t", l->word->word);
          printf ("%s\n", target);
        }
      free (target);
    }

  return all_found ? EXECUTION_SUCCESS : EXECUTION_FAILURE;
}

} // namespace bash
