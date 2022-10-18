// eaccess.cc - eaccess replacement for the shell, plus other access functions.

/* Copyright (C) 2006-2020 Free Software Foundation, Inc.

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

#include <config.h>

#include <stdio.h>

#include "bashtypes.hh"

#include <unistd.h>

#if !defined(_POSIX_VERSION) && defined(HAVE_SYS_FILE_H)
#include <sys/file.h>
#endif /* !_POSIX_VERSION */

#include "shell.hh"

#if !defined(R_OK)
#define R_OK 4
#define W_OK 2
#define X_OK 1
#define F_OK 0
#endif /* R_OK */

namespace bash
{

bool
Shell::path_is_devfd (const char *path)
{
  if (STREQN (path, "/dev/fd/", 8))
    return true;
  else if (STREQN (path, "/dev/std", 8))
    {
      if (STREQ (path + 8, "in") || STREQ (path + 8, "out")
          || STREQ (path + 8, "err"))
        return true;
      else
        return false;
    }
  else
    return false;
}

/* A wrapper for stat () which disallows pathnames that are empty strings
   and handles /dev/fd emulation on systems that don't have it. */
int
Shell::sh_stat (const char *path, struct stat *finfo)
{
  if (*path == '\0')
    {
      errno = ENOENT;
      return -1;
    }
  if (path[0] == '/' && path[1] == 'd' && strncmp (path, "/dev/fd/", 8) == 0)
    {
      /* If stating /dev/fd/n doesn't produce the same results as fstat of
         FD N, then define DEV_FD_STAT_BROKEN */
#if !defined(HAVE_DEV_FD) || defined(DEV_FD_STAT_BROKEN)
      int64_t fd;
      int r;

      if (legal_number (path + 8, &fd) && fd == static_cast<int> (fd))
        {
          r = fstat (static_cast<int> (fd), finfo);
          if (r == 0 || errno != EBADF)
            return r;
        }
      errno = ENOENT;
      return -1;
#else
      // Let's assume we aren't on old Linux that only has /proc/self/fd/*
      // where we have to rewrite the path, like the original C code does.
      return stat (path, finfo);
#endif /* !HAVE_DEV_FD */
    }
#if !defined(HAVE_DEV_STDIN)
  else if (STREQN (path, "/dev/std", 8))
    {
      if (STREQ (path + 8, "in"))
        return fstat (0, finfo);
      else if (STREQ (path + 8, "out"))
        return fstat (1, finfo);
      else if (STREQ (path + 8, "err"))
        return fstat (2, finfo);
      else
        return stat (path, finfo);
    }
#endif /* !HAVE_DEV_STDIN */
  return stat (path, finfo);
}

/* Do the same thing access(2) does, but use the effective uid and gid,
   and don't make the mistake of telling root that any file is
   executable.  This version uses stat(2). */
int
Shell::sh_stataccess (const char *path, int mode)
{
  struct stat st;

  if (sh_stat (path, &st) < 0)
    return -1;

  if (current_user.euid == 0)
    {
      /* Root can read or write any file. */
      if ((mode & X_OK) == 0)
        return 0;

      /* Root can execute any file that has any one of the execute
         bits set. */
      if (st.st_mode & S_IXUGO)
        return 0;
    }

  if (st.st_uid == current_user.euid) /* owner */
    mode <<= 6;
  else if (group_member (st.st_gid))
    mode <<= 3;

  if (st.st_mode & static_cast<mode_t> (mode))
    return 0;

  errno = EACCES;
  return -1;
}

} // namespace bash
