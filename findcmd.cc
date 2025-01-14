/* findcmd.cc -- Functions to search for commands by name. */

/* Copyright (C) 1997-2022 Free Software Foundation, Inc.

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

#include "bashtypes.hh"
#include "chartypes.hh"

#if defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif

#include "posixstat.hh"

#include <unistd.h>

#include "flags.hh"
#include "hashcmd.hh"
#include "hashlib.hh"
#include "pathexp.hh"
#include "shell.hh"

#include <fnmatch.h>

namespace bash
{

#if 0

/* The file name which we would try to execute, except that it isn't
   possible to execute it.  This is the first file that matches the
   name that we are looking for while we are searching $PATH for a
   suitable one to execute.  If we cannot find a suitable executable
   file, then we use this one. */
static char *file_to_lose_on;

/* Non-zero if we should stat every command found in the hash table to
   make sure it still exists. */
bool check_hashed_filenames = CHECKHASH_DEFAULT;

/* DOT_FOUND_IN_SEARCH becomes non-zero when find_user_command ()
   encounters a `.' as the directory pathname while scanning the
   list of possible pathnames; i.e., if `.' comes before the directory
   containing the file of interest. */
bool dot_found_in_search = false;

/* Set up EXECIGNORE; a blacklist of patterns that executable files should not
   match. */
static struct ignorevar execignore = { "EXECIGNORE", nullptr, 0, nullptr, nullptr };
#endif

void
setup_exec_ignore ()
{
  setup_ignore_patterns (&execignore);
}

bool
Shell::exec_name_should_ignore (const char *name)
{
  struct ign *p;

  for (p = execignore.ignores; p && p->val; p++)
    if (fnmatch (p->val, name, FNMATCH_EXTFLAG | FNM_CASEFOLD) != FNM_NOMATCH)
      return true;

  return false;
}

/* Return some flags based on information about this file.
   The EXISTS bit is non-zero if the file is found.
   The EXECABLE bit is non-zero the file is executable.
   Zero is returned if the file is not found. */
file_stat_flags
Shell::file_status (const char *name)
{
  struct stat finfo;
  int r;

  /* Determine whether this file exists or not. */
  if (stat (name, &finfo) < 0)
    return FS_NONE;

  /* If the file is a directory, then it is not "executable" in the
     sense of the shell. */
  if (S_ISDIR (finfo.st_mode))
    return FS_EXISTS | FS_DIRECTORY;

  r = FS_EXISTS;

  /* Use euidaccess from OS or Gnulib to take things like ACLs and other
     file access mechanisms into account.  euidaccess uses the effective
     user and group IDs, not the real ones.  We could use sh_eaccess,
     but we don't want any special treatment for /dev/fd. */
  if (exec_name_should_ignore (name) == 0 && euidaccess (name, X_OK) == 0)
    r |= FS_EXECABLE;
  if (euidaccess (name, R_OK) == 0)
    r |= FS_READABLE;

  return r;
}

/* Return non-zero if FILE exists and is executable.
   Note that this function is the definition of what an
   executable file is; do not change this unless YOU know
   what an executable file is. */
bool
Shell::executable_file (const char *file)
{
  int s;

  s = file_status (file);
#if defined(EISDIR)
  if (s & FS_DIRECTORY)
    errno = EISDIR; /* let's see if we can improve error messages */
#endif
  return (s & FS_EXECABLE) && ((s & FS_DIRECTORY) == 0);
}

bool
Shell::is_directory (const char *file)
{
  return file_status (file) & FS_DIRECTORY;
}

bool
Shell::executable_or_directory (const char *file)
{
  int s;

  s = file_status (file);
  return (s & FS_EXECABLE) || (s & FS_DIRECTORY);
}

/* Locate the executable file referenced by NAME, searching along
   the contents of the shell PATH variable.  Return a new string
   which is the full pathname to the file, or nullptr if the file
   couldn't be found.  If a file is found that isn't executable,
   and that is the only match, then return that. */
char *
Shell::find_user_command (const char *name)
{
  return find_user_command_internal (name, FS_EXEC_PREFERRED | FS_NODIRS);
}

/* Locate the file referenced by NAME, searching along the contents
   of the shell PATH variable.  Return a new string which is the full
   pathname to the file, or nullptr if the file couldn't be found.  This
   returns the first readable file found; designed to be used to look
   for shell scripts or files to source. */
char *
Shell::find_path_file (const char *name)
{
  return find_user_command_internal (name, FS_READABLE);
}

char *
Shell::_find_user_command_internal (const char *name, int flags)
{
  char *path_list, *cmd;
  SHELL_VAR *var;

  /* Search for the value of PATH in both the temporary environments and
     in the regular list of variables. */
  if ((var = find_variable_tempenv ("PATH"))) /* XXX could be array? */
    path_list = value_cell (var);
  else
    path_list = nullptr;

  if (path_list == 0 || *path_list == '\0')
    return savestring (name);

  cmd = find_user_command_in_path (name, path_list, flags, (int *)0);

  return cmd;
}

char *
Shell::find_user_command_internal (const char *name, int flags)
{
#ifdef __WIN32__
  char *res, *dotexe;

  dotexe = (char *)xmalloc (strlen (name) + 5);
  strcpy (dotexe, name);
  strcat (dotexe, ".exe");
  res = _find_user_command_internal (dotexe, flags);
  free (dotexe);
  if (res == 0)
    res = _find_user_command_internal (name, flags);
  return res;
#else
  return _find_user_command_internal (name, flags);
#endif
}

/* Return the next element from PATH_LIST, a colon separated list of
   paths.  PATH_INDEX_POINTER is the address of an index into PATH_LIST;
   the index is modified by this function.
   Return the next element of PATH_LIST or nullptr if there are no more. */
char *
Shell::get_next_path_element (const char *path_list,
                              size_t *path_index_pointer)
{
  char *path;

  path = extract_colon_unit (path_list, path_index_pointer);

  if (path == 0)
    return path;

  if (*path == '\0')
    {
      free (path);
      path = savestring (".");
    }

  return path;
}

/* Look for PATHNAME in $PATH.  Returns either the hashed command
   corresponding to PATHNAME or the first instance of PATHNAME found
   in $PATH.  If (FLAGS & CMDSRCH_HASH) is non-zero, insert the instance of
   PATHNAME found in $PATH into the command hash table.
   If (FLAGS & CMDSRCH_STDPATH) is non-zero, we are running in a `command -p'
   environment and should use the Posix standard path.
   Returns a newly-allocated string. */
char *
Shell::search_for_command (const char *pathname, cmd_search_flags flags)
{
  char *hashed_file, *command, *path_list;
  int temp_path, st;
  SHELL_VAR *path;

  hashed_file = command = nullptr;

  /* If PATH is in the temporary environment for this command, don't use the
     hash table to search for the full pathname. */
  path = find_variable_tempenv ("PATH");
  temp_path = path && path->tempvar ();

  /* Don't waste time trying to find hashed data for a pathname
     that is already completely specified or if we're using a command-
     specific value for PATH. */
  if (temp_path == 0 && (flags & CMDSRCH_STDPATH) == 0
      && absolute_program (pathname) == 0)
    hashed_file = phash_search (pathname);

  /* If a command found in the hash table no longer exists, we need to
     look for it in $PATH.  Thank you Posix.2.  This forces us to stat
     every command found in the hash table. */

  if (hashed_file && (posixly_correct || check_hashed_filenames))
    {
      st = file_status (hashed_file);
      if ((st & (FS_EXISTS | FS_EXECABLE)) != (FS_EXISTS | FS_EXECABLE))
        {
          phash_remove (pathname);
          free (hashed_file);
          hashed_file = nullptr;
        }
    }

  if (hashed_file)
    command = hashed_file;
  else if (absolute_program (pathname))
    /* A command containing a slash is not looked up in PATH or saved in
       the hash table. */
    command = savestring (pathname);
  else
    {
      if (flags & CMDSRCH_STDPATH)
        path_list = conf_standard_path ();
      else if (temp_path || path)
        path_list = value_cell (path);
      else
        path_list = 0;

      command = find_user_command_in_path (pathname, path_list,
                                           FS_EXEC_PREFERRED | FS_NODIRS);

      if (command && hashing_enabled && temp_path == 0
          && (flags & CMDSRCH_HASH))
        {
          /* If we found the full pathname the same as the command name, the
             command probably doesn't exist.  Don't put it into the hash
             table unless it's an executable file in the current directory. */
          if (STREQ (command, pathname))
            {
              if (st & FS_EXECABLE)
                phash_insert (pathname, command, dot_found_in_search, 1);
            }
          /* If we're in posix mode, don't add files without the execute bit
             to the hash table. */
          else if (posixly_correct || check_hashed_filenames)
            {
              if (st & FS_EXECABLE)
                phash_insert (pathname, command, dot_found_in_search, 1);
            }
          else
            phash_insert (pathname, command, dot_found_in_search, 1);
        }

      if (flags & CMDSRCH_STDPATH)
        free (path_list);
    }

  return command;
}

char *
Shell::user_command_matches (const char *name, int flags, int state)
{
  int i;
  int path_index, name_len;
  char *path_list, *path_element, *match;
  struct stat dotinfo;
  static char **match_list = nullptr;
  static int match_list_size = 0;
  static int match_index = 0;

  if (state == 0)
    {
      /* Create the list of matches. */
      if (match_list == 0)
        {
          match_list_size = 5;
          match_list = strvec_create (match_list_size);
        }

      /* Clear out the old match list. */
      for (i = 0; i < match_list_size; i++)
        match_list[i] = 0;

      /* We haven't found any files yet. */
      match_index = 0;

      if (absolute_program (name))
        {
          match_list[0] = find_absolute_program (name, flags);
          match_list[1] = nullptr;
          path_list = nullptr;
        }
      else
        {
          name_len = strlen (name);
          file_to_lose_on = nullptr;
          dot_found_in_search = 0;
          if (stat (".", &dotinfo) < 0)
            dotinfo.st_dev = dotinfo.st_ino = 0; /* so same_file won't match */
          path_list = get_string_value ("PATH");
          path_index = 0;
        }

      while (path_list && path_list[path_index])
        {
          path_element = get_next_path_element (path_list, &path_index);

          if (path_element == 0)
            break;

          match = find_in_path_element (name, path_element, flags, name_len,
                                        &dotinfo, (int *)0);
          free (path_element);

          if (match == 0)
            continue;

          if (match_index + 1 == match_list_size)
            {
              match_list_size += 10;
              match_list = strvec_resize (match_list, (match_list_size + 1));
            }

          match_list[match_index++] = match;
          match_list[match_index] = nullptr;
          FREE (file_to_lose_on);
          file_to_lose_on = nullptr;
        }

      /* We haven't returned any strings yet. */
      match_index = 0;
    }

  match = match_list[match_index];

  if (match)
    match_index++;

  return match;
}

char *
Shell::find_absolute_program (const char *name, int flags)
{
  int st;

  st = file_status (name);

  /* If the file doesn't exist, quit now. */
  if ((st & FS_EXISTS) == 0)
    return nullptr;

  /* If we only care about whether the file exists or not, return
     this filename.  Otherwise, maybe we care about whether this
     file is executable.  If it is, and that is what we want, return it. */
  if ((flags & FS_EXISTS) || ((flags & FS_EXEC_ONLY) && (st & FS_EXECABLE)))
    return savestring (name);

  return nullptr;
}

char *
Shell::find_in_path_element (const char *name, const char *path, int flags,
                             int name_len, struct stat *dotinfop)
{
  int status;
  char *full_path, *xpath;

  xpath = (!posixly_correct && *path == '~') ? bash_tilde_expand (path, 0)
                                             : path;

  /* Remember the location of "." in the path, in all its forms
     (as long as they begin with a `.', e.g. `./.') */
  /* We could also do this or something similar for all relative pathnames
     found while searching PATH. */
  if (dot_found_in_search == 0 && *xpath == '.')
    dot_found_in_search
        = same_file (".", xpath, dotinfop, (struct stat *)nullptr);

  full_path = sh_makepath (xpath, name, 0);

  status = file_status (full_path);

  if (xpath != path)
    free (xpath);

  if (rflagsp)
    *rflagsp = status;

  if ((status & FS_EXISTS) == 0)
    {
      free (full_path);
      return nullptr;
    }

  /* The file exists.  If the caller simply wants the first file, here it is.
   */
  if (flags & FS_EXISTS)
    return full_path;

  /* If we have a readable file, and the caller wants a readable file, this
     is it. */
  if ((flags & FS_READABLE) && (status & FS_READABLE))
    return full_path;

  /* If the file is executable, then it satisfies the cases of
      EXEC_ONLY and EXEC_PREFERRED.  Return this file unconditionally. */
  if ((status & FS_EXECABLE) && (flags & (FS_EXEC_ONLY | FS_EXEC_PREFERRED))
      && (((flags & FS_NODIRS) == 0) || ((status & FS_DIRECTORY) == 0)))
    {
      FREE (file_to_lose_on);
      file_to_lose_on = nullptr;
      return full_path;
    }

  /* The file is not executable, but it does exist.  If we prefer
     an executable, then remember this one if it is the first one
     we have found. */
  if ((flags & FS_EXEC_PREFERRED) && file_to_lose_on == 0
      && exec_name_should_ignore (full_path) == 0)
    file_to_lose_on = savestring (full_path);

  /* If we want only executable files, or we don't want directories and
     this file is a directory, or we want a readable file and this file
     isn't readable, fail. */
  if ((flags & (FS_EXEC_ONLY | FS_EXEC_PREFERRED))
      || ((flags & FS_NODIRS) && (status & FS_DIRECTORY))
      || ((flags & FS_READABLE) && (status & FS_READABLE) == 0))
    {
      free (full_path);
      return nullptr;
    }
  else
    return full_path;
}

/* This does the dirty work for find_user_command_internal () and
   user_command_matches ().
   NAME is the name of the file to search for.
   PATH_LIST is a colon separated list of directories to search.
   FLAGS contains bit fields which control the files which are eligible.
   Some values are:
      FS_EXEC_ONLY:		The file must be an executable to be found.
      FS_EXEC_PREFERRED:	If we can't find an executable, then the
                                the first file matching NAME will do.
      FS_EXISTS:		The first file found will do.
      FS_NODIRS:		Don't find any directories.
*/
char *
Shell::find_user_command_in_path (const char *name, const char *path_list,
                                  file_stat_flags flags)
{
  char *full_path, *path;
  int path_index, name_len, rflags;
  struct stat dotinfo;

  // We haven't started looking, so we certainly haven't seen a `.' as the
  // directory path yet.
  dot_found_in_search = 0;

  if (rflagsp)
    *rflagsp = 0;

  if (absolute_program (name))
    {
      full_path = find_absolute_program (name, flags);
      return full_path;
    }

  if (path_list == 0 || *path_list == '\0')
    return savestring (name); /* XXX */

  file_to_lose_on = nullptr;
  name_len = strlen (name);
  if (stat (".", &dotinfo) < 0)
    dotinfo.st_dev = dotinfo.st_ino = 0;
  path_index = 0;

  while (path_list[path_index])
    {
      /* Allow the user to interrupt out of a lengthy path search. */
      QUIT;

      path = get_next_path_element (path_list, &path_index);
      if (path == 0)
        break;

      /* Side effects: sets dot_found_in_search, possibly sets
         file_to_lose_on. */
      full_path = find_in_path_element (name, path, flags, name_len, &dotinfo,
                                        &rflags);
      free (path);

      /* We use the file status flag bits to check whether full_path is a
         directory, which we reject here. */
      if (full_path && (rflags & FS_DIRECTORY))
        {
          free (full_path);
          continue;
        }

      if (full_path)
        {
          if (rflagsp)
            *rflagsp = rflags;
          FREE (file_to_lose_on);
          return full_path;
        }
    }

  /* We didn't find exactly what the user was looking for.  Return
     the contents of FILE_TO_LOSE_ON which is nullptr when the search
     required an executable, or non-nullptr if a file was found and the
     search would accept a non-executable as a last resort.  If the
     caller specified FS_NODIRS, and file_to_lose_on is a directory,
     return nullptr. */
  if (file_to_lose_on && (flags & FS_NODIRS) && file_isdir (file_to_lose_on))
    {
      free (file_to_lose_on);
      file_to_lose_on = nullptr;
    }

  return file_to_lose_on;
}

/* External interface to find a command given a $PATH.  Separate from
   find_user_command_in_path to allow future customization. */
char *
Shell::find_in_path (const char *name, const char *path_list, int flags)
{
  return find_user_command_in_path (name, path_list, flags, (int *)0);
}

} // namespace bash
