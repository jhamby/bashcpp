/*
 * tmpfile.c - functions to create and safely open temp files for the shell.
 */

/* Copyright (C) 2000-2020 Free Software Foundation, Inc.

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

#include "config.hh"

#include "bashtypes.hh"
#include "posixstat.hh"
#include "posixtime.hh"
#include "filecntl.hh"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#include <cstdio>
#include <cerrno>
#include <cstdlib>

#include "shell.hh"

namespace bash
{

#define BASEOPENFLAGS	(O_CREAT | O_TRUNC | O_EXCL | O_BINARY)

#define DEFAULT_TMPDIR		"."	/* bogus default, should be changed */
#define DEFAULT_NAMEROOT	"shtmp"

/* Use ANSI-C rand() interface if random(3) is not available */
#if !defined (HAVE_RANDOM)
#define random() rand()
#endif

#if 0
static const char *get_sys_tmpdir (void);
static const char *sys_tmpdir = nullptr;
static int ntmpfiles;
static int tmpnamelen = -1;
static unsigned long filenum = 1L;
#endif

static const char *
get_sys_tmpdir ()
{
  if (sys_tmpdir)
    return sys_tmpdir;

#ifdef P_tmpdir
  sys_tmpdir = P_tmpdir;
  if (file_iswdir (sys_tmpdir))
    return sys_tmpdir;
#endif

  sys_tmpdir = "/tmp";
  if (file_iswdir (sys_tmpdir))
    return sys_tmpdir;

  sys_tmpdir = "/var/tmp";
  if (file_iswdir (sys_tmpdir))
    return sys_tmpdir;

  sys_tmpdir = "/usr/tmp";
  if (file_iswdir (sys_tmpdir))
    return sys_tmpdir;

  sys_tmpdir = DEFAULT_TMPDIR;

  return sys_tmpdir;
}

const char *
Shell::get_tmpdir (int flags)
{
  const char *tdir;

  tdir = (flags & MT_USETMPDIR) ? get_string_value ("TMPDIR") : nullptr;
  if (tdir && (!file_iswdir (tdir) || std::strlen (tdir) > PATH_MAX))
    tdir = nullptr;

  if (tdir == nullptr)
    tdir = get_sys_tmpdir ();

#if defined (HAVE_PATHCONF) && defined (_PC_NAME_MAX)
  if (tmpnamelen == -1)
    tmpnamelen = ::pathconf (tdir, _PC_NAME_MAX);
#else
  tmpnamelen = 0;
#endif

  return tdir;
}

static void
sh_seedrand ()
{
#if HAVE_RANDOM
  int d;
  static int seeded = 0;
  if (seeded == 0)
    {
      struct timeval tv;

      ::gettimeofday (&tv, nullptr);
      ::srandom (tv.tv_sec ^ tv.tv_usec ^ (::getpid () << 16) ^ (uintptr_t)&d);
      seeded = 1;
    }
#endif
}

char *
sh_mktmpname (char *nameroot, int flags)
{
  char *filename;
  struct stat sb;
  int r, tdlen;
  static int seeded = 0;

  filename = new char[PATH_MAX + 1];
  const char *tdir = get_tmpdir (flags);
  tdlen = std::strlen (tdir);

  const char *lroot = nameroot ? nameroot : DEFAULT_NAMEROOT;
  if (nameroot == nullptr)
    flags &= ~MT_TEMPLATE;

  if ((flags & MT_TEMPLATE) && std::strlen (nameroot) > PATH_MAX)
    flags &= ~MT_TEMPLATE;

#ifdef USE_MKTEMP
  if (flags & MT_TEMPLATE)
    std::strcpy (filename, nameroot);
  else
    std::sprintf (filename, "%s/%s.XXXXXX", tdir, lroot);
  if (::mktemp (filename) == nullptr)
    {
      delete[] filename;
      filename = nullptr;
    }
#else  /* !USE_MKTEMP */
  sh_seedrand ();
  while (1)
    {
      filenum = (filenum << 1) ^
		(unsigned long) std::time (nullptr) ^
		(unsigned long) dollar_dollar_pid ^
		(unsigned long) ((flags & MT_USERANDOM) ? std::random () : ntmpfiles++);
      std::sprintf (filename, "%s/%s-%lu", tdir, lroot, filenum);
      if (tmpnamelen > 0 && tmpnamelen < 32)
	filename[tdlen + 1 + tmpnamelen] = '\0';
#  ifdef HAVE_LSTAT
      r = ::lstat (filename, &sb);
#  else
      r = ::stat (filename, &sb);
#  endif
      if (r < 0 && errno == ENOENT)
	break;
    }
#endif /* !USE_MKTEMP */

  return filename;
}

int
sh_mktmpfd (const char *nameroot, int flags, char **namep)
{
  int fd, tdlen;

  char *filename = new char[PATH_MAX + 1];
  const char *tdir = get_tmpdir (flags);
  tdlen = std::strlen (tdir);

  const char *lroot = nameroot ? nameroot : DEFAULT_NAMEROOT;
  if (nameroot == nullptr)
    flags &= ~MT_TEMPLATE;

  if ((flags & MT_TEMPLATE) && std::strlen (nameroot) > PATH_MAX)
    flags &= ~MT_TEMPLATE;

#ifdef USE_MKSTEMP
  if (flags & MT_TEMPLATE)
    std::strcpy (filename, nameroot);
  else
    std::sprintf (filename, "%s/%s.XXXXXX", tdir, lroot);
  fd = ::mkstemp (filename);
  if (fd < 0 || namep == nullptr)
    {
      delete[] filename;
      filename = nullptr;
    }
  if (namep)
    *namep = filename;
  return fd;
#else /* !USE_MKSTEMP */
  sh_seedrand ();
  do
    {
      filenum = (filenum << 1) ^
		(unsigned long) std::time (nullptr) ^
		(unsigned long) dollar_dollar_pid ^
		(unsigned long) ((flags & MT_USERANDOM) ? std::random () : ntmpfiles++);
      std::sprintf (filename, "%s/%s-%lu", tdir, lroot, filenum);
      if (tmpnamelen > 0 && tmpnamelen < 32)
	filename[tdlen + 1 + tmpnamelen] = '\0';
      fd = ::open (filename, BASEOPENFLAGS | ((flags & MT_READWRITE) ? O_RDWR : O_WRONLY), 0600);
    }
  while (fd < 0 && errno == EEXIST);

  if (namep)
    *namep = filename;
  else
    delete[] filename;

  return fd;
#endif /* !USE_MKSTEMP */
}

FILE *
sh_mktmpfp (const char *nameroot, int flags, char **namep)
{
  int fd;
  FILE *fp;

  fd = sh_mktmpfd (nameroot, flags, namep);
  if (fd < 0)
    return nullptr;
  fp = ::fdopen (fd, (flags & MT_READWRITE) ? "w+" : "w");
  if (fp == nullptr)
    ::close (fd);
  return fp;
}

char *
sh_mktmpdir (char *nameroot, int flags)
{
#ifdef USE_MKDTEMP
  char *filename = new char[PATH_MAX + 1];
  const char *tdir = get_tmpdir (flags);
  int tdlen = std::strlen (tdir);

  const char *lroot = nameroot ? nameroot : DEFAULT_NAMEROOT;
  if (nameroot == nullptr)
    flags &= ~MT_TEMPLATE;

  if ((flags & MT_TEMPLATE) && std::strlen (nameroot) > PATH_MAX)
    flags &= ~MT_TEMPLATE;

  if (flags & MT_TEMPLATE)
    std::strcpy (filename, nameroot);
  else
    std::sprintf (filename, "%s/%s.XXXXXX", tdir, lroot);
  char *dirname = ::mkdtemp (filename);
  if (dirname == nullptr)
    {
      delete[] filename;
      filename = nullptr;
    }
  return dirname;
#else /* !USE_MKDTEMP */
  char *filename = nullptr;
  int fd;
  do
    {
      filename = sh_mktmpname (nameroot, flags);
      fd = ::mkdir (filename, 0700);
      if (fd == 0)
	break;
      delete[] filename;
      filename = nullptr;
    }
  while (fd < 0 && errno == EEXIST);

  return filename;
#endif /* !USE_MKDTEMP */
}

}  // namespace bash
