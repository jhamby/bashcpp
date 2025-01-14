/* pathchk - check pathnames for validity and portability */

/* Usage: pathchk [-p] path ...

   For each PATH, print a message if any of these conditions are false:
   * all existing leading directories in PATH have search (execute) permission
   * strlen (PATH) <= PATH_MAX
   * strlen (each_directory_in_PATH) <= NAME_MAX

   Exit status:
   0			All PATH names passed all of the tests.
   1			An error occurred.

   Options:
   -p			Instead of performing length checks on the
                        underlying filesystem, test the length of the
                        pathname and its components against the POSIX.1
                        minimum limits for portability, _POSIX_NAME_MAX
                        and _POSIX_PATH_MAX in 2.9.2.  Also check that
                        the pathname contains no character not in the
                        portable filename character set. */

/* See Makefile for compilation details. */

/*
   Copyright (C) 1999-2009 Free Software Foundation, Inc.

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

#include "config.h"

#include "posixstat.hh"

#include <sys/types.h>

#include <limits.h>
#include <unistd.h>

#include "builtins.hh"
#include "common.hh"
#include "maxpath.hh"
#include "shell.hh"

#if !defined(_POSIX_PATH_MAX)
#define _POSIX_PATH_MAX 255
#endif
#if !defined(_POSIX_NAME_MAX)
#define _POSIX_NAME_MAX 14
#endif

/* How do we get PATH_MAX? */
#if defined(_POSIX_VERSION) && !defined(PATH_MAX)
#define PATH_MAX_FOR(p) pathconf ((p), _PC_PATH_MAX)
#endif

/* How do we get NAME_MAX? */
#if defined(_POSIX_VERSION) && !defined(NAME_MAX)
#define NAME_MAX_FOR(p) pathconf ((p), _PC_NAME_MAX)
#endif

#if !defined(PATH_MAX_FOR)
#define PATH_MAX_FOR(p) PATH_MAX
#endif

#if !defined(NAME_MAX_FOR)
#define NAME_MAX_FOR(p) NAME_MAX
#endif

namespace bash
{

// Loadable class for "pathchk".
class ShellLoadable : public Shell
{
public:
  int pathchk_builtin (WORD_LIST *);

private:
};

static int validate_path ();

int
ShellLoadable::pathchk_builtin (WORD_LIST *list)
{
  int retval, pflag, opt;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "p")) != -1)
    {
      switch (opt)
        {
        case 'p':
          pflag = 1;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  if (list == 0)
    {
      builtin_usage ();
      return EX_USAGE;
    }

  for (retval = 0; list; list = list->next ())
    retval |= validate_path (list->word->word, pflag);

  return retval ? EXECUTION_FAILURE : EXECUTION_SUCCESS;
}

static const char *const pathchk_doc[] = {
  "Check pathnames for validity.",
  "",
  "Check each pathname argument for validity (i.e., it may be used to",
  "create or access a file without causing syntax errors) and portability",
  "(i.e., no filename truncation will result).  If the `-p' option is",
  "supplied, more extensive portability checks are performed.",
  nullptr
};

Shell::builtin pathchk_struct (
    static_cast<Shell::sh_builtin_func_t> (&ShellLoadable::pathchk_builtin),
    pathchk_doc, "pathchk [-p] pathname ...", nullptr, BUILTIN_ENABLED);

/* The remainder of this file is stolen shamelessly from `pathchk.c' in
   the sh-utils-1.12 distribution, by

   David MacKenzie <djm@gnu.ai.mit.edu>
   and Jim Meyering <meyering@cs.utexas.edu> */

/* Each element is nonzero if the corresponding ASCII character is
   in the POSIX portable character set, and zero if it is not.
   In addition, the entry for `/' is nonzero to simplify checking. */
static char const portable_chars[256]
    = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0-15 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 16-31 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, /* 32-47 */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 48-63 */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 64-79 */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 80-95 */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 96-111 */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 112-127 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/* If PATH contains only portable characters, return 1, else 0.  */

static int
portable_chars_only (const char *path)
{
  const char *p;

  for (p = path; *p; ++p)
    if (portable_chars[(const unsigned char)*p] == 0)
      {
        builtin_error ("path `%s' contains nonportable character `%c'", path,
                       *p);
        return 0;
      }
  return 1;
}

/* On some systems, stat can return EINTR.  */

#ifndef EINTR
#define SAFE_STAT(name, buf) stat (name, buf)
#else
#define SAFE_STAT(name, buf) safe_stat (name, buf)

static inline int
safe_stat (const char *name, struct stat *buf)
{
  int ret;

  do
    ret = stat (name, buf);
  while (ret < 0 && errno == EINTR);

  return ret;
}
#endif

/* Return 1 if PATH is a usable leading directory, 0 if not,
   2 if it doesn't exist.  */

static int
dir_ok (const char *path)
{
  struct stat stats;

  if (SAFE_STAT (path, &stats))
    return 2;

  if (!S_ISDIR (stats.st_mode))
    {
      builtin_error ("`%s' is not a directory", path);
      return 0;
    }

  /* Use access to test for search permission because
     testing permission bits of st_mode can lose with new
     access control mechanisms.  Of course, access loses if you're
     running setuid. */
  if (access (path, X_OK) != 0)
    {
      if (errno == EACCES)
        builtin_error ("directory `%s' is not searchable", path);
      else
        builtin_error ("%s: %s", path, strerror (errno));
      return 0;
    }

  return 1;
}

/* Make sure that
   strlen (PATH) <= PATH_MAX
   && strlen (each-existing-directory-in-PATH) <= NAME_MAX

   If PORTABILITY is nonzero, compare against _POSIX_PATH_MAX and
   _POSIX_NAME_MAX instead, and make sure that PATH contains no
   characters not in the POSIX portable filename character set, which
   consists of A-Z, a-z, 0-9, ., _, -.

   Make sure that all leading directories along PATH that exist have
   `x' permission.

   Return 0 if all of these tests are successful, 1 if any fail. */

static int
validate_path (const char *path, bool portability)
{
  int path_max;
  int last_elem;  /* Nonzero if checking last element of path. */
  int exists = 0; /* 2 if the path element exists.  */
  char *slash;
  char *parent; /* Last existing leading directory so far.  */

  if (portability && !portable_chars_only (path))
    return 1;

  if (*path == '\0')
    return 0;

  /* Figure out the parent of the first element in PATH.  */
  parent = xstrdup (*path == '/' ? "/" : ".");

  slash = path;
  last_elem = 0;
  while (1)
    {
      int name_max;
      int length;  /* Length of partial path being checked. */
      char *start; /* Start of path element being checked. */

      /* Find the end of this element of the path.
         Then chop off the rest of the path after this element. */
      while (*slash == '/')
        slash++;
      start = slash;
      slash = strchr (slash, '/');
      if (slash != NULL)
        *slash = '\0';
      else
        {
          last_elem = 1;
          slash = strchr (start, '\0');
        }

      if (!last_elem)
        {
          exists = dir_ok (path);
          if (exists == 0)
            {
              free (parent);
              return 1;
            }
        }

      length = slash - start;
      /* Since we know that `parent' is a directory, it's ok to call
         pathconf with it as the argument.  (If `parent' isn't a directory
         or doesn't exist, the behavior of pathconf is undefined.)
         But if `parent' is a directory and is on a remote file system,
         it's likely that pathconf can't give us a reasonable value
         and will return -1.  (NFS and tempfs are not POSIX . . .)
         In that case, we have no choice but to assume the pessimal
         POSIX minimums.  */
      name_max = portability ? _POSIX_NAME_MAX : NAME_MAX_FOR (parent);
      if (name_max < 0)
        name_max = _POSIX_NAME_MAX;
      if (length > name_max)
        {
          builtin_error ("name `%s' has length %d; exceeds limit of %d", start,
                         length, name_max);
          free (parent);
          return 1;
        }

      if (last_elem)
        break;

      if (exists == 1)
        {
          free (parent);
          parent = xstrdup (path);
        }

      *slash++ = '/';
    }

  /* `parent' is now the last existing leading directory in the whole path,
     so it's ok to call pathconf with it as the argument.  */
  path_max = portability ? _POSIX_PATH_MAX : PATH_MAX_FOR (parent);
  if (path_max < 0)
    path_max = _POSIX_PATH_MAX;
  free (parent);
  if (strlen (path) > path_max)
    {
      builtin_error ("path `%s' has length %lu; exceeds limit of %d", path,
                     (unsigned long)strlen (path), path_max);
      return 1;
    }

  return 0;
}

} // namespace bash
