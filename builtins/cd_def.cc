// This file is cd_def.cc.
// It implements the builtins "cd" and "pwd" in Bash.

// Copyright (C) 1987-2022 Free Software Foundation, Inc.

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

#include "bashtypes.hh"
#include "posixdir.hh"
#include "posixstat.hh"

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#include <cerrno>
#include <fcntl.h>

#include "bashintl.hh"

#include "tilde.hh"

#include "common.hh"
#include "flags.hh"
#include "maxpath.hh"
#include "shell.hh"

#if 0
extern const char * const bash_getcwd_errstr;

static int bindpwd (bool);
static int setpwd (const char *);
static char *resetpwd (const char *);
static bool change_to_directory (const char *, bool, bool);

static int cdxattr (char *, char **);
static void resetxattr (void);

/* Change this to 1 to get cd spelling correction by default. */
bool cdspelling = false;

bool cdable_vars;

static bool eflag;	/* file scope so bindpwd() can see it */
static bool xattrflag;	/* O_XATTR support for openat */
static int xattrfd = -1;
#endif

// $BUILTIN cd
// $FUNCTION cd_builtin
// $SHORT_DOC cd [-L|[-P [-e]] [-@]] [dir]
// Change the shell working directory.

// Change the current directory to DIR.  The default DIR is the value of the
// HOME shell variable. If DIR is "-", it is converted to $OLDPWD.

// The variable CDPATH defines the search path for the directory containing
// DIR.  Alternative directory names in CDPATH are separated by a colon (:).
// A null directory name is the same as the current directory.  If DIR begins
// with a slash (/), then CDPATH is not used.

// If the directory is not found, and the shell option `cdable_vars' is set,
// the word is assumed to be  a variable name.  If that variable has a value,
// its value is used for DIR.

// Options:
//   -L	force symbolic links to be followed: resolve symbolic
// 		links in DIR after processing instances of `..'
//   -P	use the physical directory structure without following
// 		symbolic links: resolve symbolic links in DIR before
// 		processing instances of `..'
//   -e	if the -P option is supplied, and the current working
// 		directory cannot be determined successfully, exit with
// 		a non-zero status
#if defined(O_XATTR)
//   -@	on systems that support it, present a file with extended
// 		attributes as a directory containing the file attributes
#endif

// The default is to follow symbolic links, as if `-L' were specified.
// `..' is processed by removing the immediately previous pathname component
// back to a slash or the beginning of DIR.

// Exit Status:
// Returns 0 if the directory is changed, and if $PWD is set successfully when
// -P is used; non-zero otherwise.
// $END

namespace bash
{

/* Just set $PWD, don't change OLDPWD.  Used by `pwd -P' in posix mode. */
int
Shell::setpwd (const char *dirname)
{
  bool old_anm;
  SHELL_VAR *tvar;

  old_anm = array_needs_making;
  tvar = bind_variable ("PWD", (char *)(dirname ? dirname : ""), 0);
  if (tvar && tvar->readonly ())
    return EXECUTION_FAILURE;
  if (tvar && !old_anm && array_needs_making && tvar->exported ())
    {
      update_export_env_inplace ("PWD=", 4, dirname ? dirname : "");
      array_needs_making = false;
    }
  return EXECUTION_SUCCESS;
}

int
Shell::bindpwd (bool no_symlinks)
{
  int r = sh_chkwrite (EXECUTION_SUCCESS);

#define tcwd the_current_working_directory
  char *dirname = tcwd ? (no_symlinks ? sh_physpath (tcwd) : tcwd)
                       : get_working_directory ("cd");
#undef tcwd

  // If canonicalization fails, reset dirname to the_current_working_directory
  bool canon_failed = false;
  if (dirname == nullptr)
    {
      canon_failed = true;
      dirname = the_current_working_directory;
    }

  bool old_anm = array_needs_making;
  char *pwdvar = get_string_value ("PWD");

  SHELL_VAR *tvar = bind_variable ("OLDPWD", pwdvar, 0);
  if (tvar && tvar->readonly ())
    r = EXECUTION_FAILURE;

  if (!old_anm && array_needs_making && tvar->exported ())
    {
      update_export_env_inplace ("OLDPWD=", 7, pwdvar);
      array_needs_making = false;
    }

  if (setpwd (dirname) == EXECUTION_FAILURE)
    r = EXECUTION_FAILURE;
  if (canon_failed && eflag)
    r = EXECUTION_FAILURE;

  if (dirname && dirname != the_current_working_directory)
    delete[] dirname;

  return r;
}

/* Call get_working_directory to reset the value of
   the_current_working_directory () */
char *
Shell::resetpwd (const char *caller)
{
  delete[] the_current_working_directory;
  the_current_working_directory = nullptr;
  return get_working_directory (caller);
}

int
Shell::cdxattr (char *dir,    /* don't assume we can always free DIR */
                char **ndirp) /* return new constructed directory name */
{
#if defined(O_XATTR)
  int apfd, fd, r, e;
  char buf[11 + 40 + 40]; /* construct new `fake' path for pwd */

  apfd = openat (AT_FDCWD, dir, O_RDONLY | O_NONBLOCK);
  if (apfd < 0)
    return -1;
  fd = openat (apfd, ".", O_XATTR);
  e = errno;
  close (apfd); /* ignore close error for now */
  errno = e;
  if (fd < 0)
    return -1;
  r = fchdir (fd); /* assume fchdir exists everywhere with O_XATTR */
  if (r < 0)
    {
      close (fd);
      return -1;
    }
  /* NFSv4 and ZFS extended attribute directories do not have names which are
     visible in the standard Unix directory tree structure.  To ensure we have
     a valid name for $PWD, we synthesize one under /proc, but to keep that
     path valid, we need to keep the file descriptor open as long as we are in
     this directory.  This imposes a certain structure on /proc. */
  if (ndirp)
    {
      sprintf (buf, "/proc/%d/fd/%d", getpid (), fd);
      *ndirp = savestring (buf);
    }

  if (xattrfd >= 0)
    close (xattrfd);
  xattrfd = fd;

  return r;
#else
  return -1;
#endif
}

/* Clean up the O_XATTR baggage.  Currently only closes xattrfd */
void
Shell::resetxattr ()
{
#if defined(O_XATTR)
  if (xattrfd >= 0)
    {
      close (xattrfd);
      xattrfd = -1;
    }
#else
  xattrfd = -1; /* not strictly necessary */
#endif
}

#define LCD_DOVARS 0x001
#define LCD_DOSPELL 0x002
#define LCD_PRINTPATH 0x004
#define LCD_FREEDIRNAME 0x008

/* This builtin is ultimately the way that all user-visible commands should
   change the current working directory.  It is called by cd_to_string (),
   so the programming interface is simple, and it handles errors and
   restrictions properly. */
int
Shell::cd_builtin (WORD_LIST *list)
{
  const char *dirname, *cdpath;
  char *path;
  int opt, lflag, e;

#if defined(RESTRICTED_SHELL)
  if (restricted)
    {
      sh_restricted ((char *)NULL);
      return EXECUTION_FAILURE;
    }
#endif /* RESTRICTED_SHELL */

  eflag = false;
  char no_symlinks = no_symbolic_links;
  xattrflag = false;
  reset_internal_getopt ();
#if defined(O_XATTR)
  while ((opt = internal_getopt (list, "eLP@")) != -1)
#else
  while ((opt = internal_getopt (list, "eLP")) != -1)
#endif
    {
      switch (opt)
        {
        case 'P':
          no_symlinks = 1;
          break;
        case 'L':
          no_symlinks = 0;
          break;
        case 'e':
          eflag = 1;
          break;
#if defined(O_XATTR)
        case '@':
          xattrflag = 1;
          break;
#endif
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  lflag = (cdable_vars ? LCD_DOVARS : 0)
          | ((interactive && cdspelling) ? LCD_DOSPELL : 0);
  if (eflag && no_symlinks == 0)
    eflag = 0;

  if (list == 0)
    {
      /* `cd' without arguments is equivalent to `cd $HOME' */
      dirname = get_string_value ("HOME");

      if (dirname == nullptr)
        {
          builtin_error (_ ("HOME not set"));
          return EXECUTION_FAILURE;
        }
      lflag = 0;
    }
#if defined(CD_COMPLAINS)
  else if (list->next ())
    {
      builtin_error (_ ("too many arguments"));
      return EXECUTION_FAILURE;
    }
#endif
#if 0
  else if (list->word->word[0] == '\0')
    {
      builtin_error (_("null directory"));
      return EXECUTION_FAILURE;
    }
#endif
  else if (list->word->word[0] == '-' && list->word->word[1] == '\0')
    {
      /* This is `cd -', equivalent to `cd $OLDPWD' */
      dirname = get_string_value ("OLDPWD");

      if (dirname == 0)
        {
          builtin_error (_ ("OLDPWD not set"));
          return EXECUTION_FAILURE;
        }
#if 0
      lflag = interactive ? LCD_PRINTPATH : 0;
#else
      lflag = LCD_PRINTPATH; /* According to SUSv3 */
#endif
    }
  else if (absolute_pathname (list->word->word.c_str ()))
    dirname = list->word->word.c_str ();
  else if (privileged_mode == 0 && (cdpath = get_string_value ("CDPATH")))
    {
      dirname = list->word->word.c_str ();

      /* Find directory in $CDPATH. */
      size_t path_index = 0;
      while ((path = extract_colon_unit (cdpath, &path_index)))
        {
          /* OPT is 1 if the path element is non-empty */
          opt = path[0] != '\0';
          char *temp = sh_makepath (path, dirname, MP_DOTILDE);
          delete[] path;

          if (change_to_directory (temp, no_symlinks, xattrflag))
            {
              /* POSIX.2 says that if a nonempty directory from CDPATH
                 is used to find the directory to change to, the new
                 directory name is echoed to stdout, whether or not
                 the shell is interactive. */
              if (opt
                  && (path
                      = no_symlinks ? temp : the_current_working_directory))
                printf ("%s\n", path);

              delete[] temp;
#if 0
	      /* Posix.2 says that after using CDPATH, the resultant
		 value of $PWD will not contain `.' or `..'. */
	      return bindpwd (posixly_correct || no_symlinks);
#else
              return bindpwd (no_symlinks);
#endif
            }
          else
            delete[] temp;
        }

#if 0
      /* changed for bash-4.2 Posix cd description steps 5-6 */
      /* POSIX.2 says that if `.' does not appear in $CDPATH, we don't
	 try the current directory, so we just punt now with an error
	 message if POSIXLY_CORRECT is non-zero.  The check for cdpath[0]
	 is so we don't mistakenly treat a CDPATH value of "" as not
	 specifying the current directory. */
      if (posixly_correct && cdpath[0])
	{
	  builtin_error ("%s: %s", dirname, strerror (ENOENT));
	  return EXECUTION_FAILURE;
	}
#endif
    }
  else
    dirname = list->word->word.c_str ();

  /* When we get here, DIRNAME is the directory to change to.  If we
     chdir successfully, just return. */
  if (change_to_directory (dirname, no_symlinks, xattrflag))
    {
      if (lflag & LCD_PRINTPATH)
        printf ("%s\n", dirname);
      return bindpwd (no_symlinks);
    }

  /* If the user requests it, then perhaps this is the name of
     a shell variable, whose value contains the directory to
     change to. */
  if (lflag & LCD_DOVARS)
    {
      temp = get_string_value (dirname);
      if (temp && change_to_directory (temp, no_symlinks, xattrflag))
        {
          printf ("%s\n", temp);
          return bindpwd (no_symlinks);
        }
    }

  /* If the user requests it, try to find a directory name similar in
     spelling to the one requested, in case the user made a simple
     typo.  This is similar to the UNIX 8th and 9th Edition shells. */
  if (lflag & LCD_DOSPELL)
    {
      temp = dirspell (dirname);
      if (temp && change_to_directory (temp, no_symlinks, xattrflag))
        {
          printf ("%s\n", temp);
          delete[] temp;
          return bindpwd (no_symlinks);
        }
      else
        delete[] temp;
    }

  e = errno;
  std::string temp = printable_filename (std::string (dirname), 0);
  builtin_error ("%s: %s", temp, strerror (e));
  return EXECUTION_FAILURE;
}

// $BUILTIN pwd
// $FUNCTION pwd_builtin
// $SHORT_DOC pwd [-LP]
// Print the name of the current working directory.

// Options:
//   -L	print the value of $PWD if it names the current working
// 		directory
//   -P	print the physical directory, without any symbolic links

// By default, `pwd' behaves as if `-L' were specified.

// Exit Status:
// Returns 0 unless an invalid option is given or the current directory
// cannot be read.
// $END

/* Non-zero means that pwd always prints the physical directory, without
   symbolic links. */
// static int verbatim_pwd;

/* Print the name of the current working directory. */
int
Shell::pwd_builtin (WORD_LIST *list)
{
  char *directory;
  int opt, pflag;

  verbatim_pwd = no_symbolic_links;
  pflag = 0;
  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "LP")) != -1)
    {
      switch (opt)
        {
        case 'P':
          verbatim_pwd = pflag = 1;
          break;
        case 'L':
          verbatim_pwd = 0;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

#define tcwd the_current_working_directory

  directory = tcwd ? (verbatim_pwd ? sh_physpath (tcwd, 0) : tcwd)
                   : get_working_directory ("pwd");

  /* Try again using getcwd() if canonicalization fails (for instance, if
     the file system has changed state underneath bash). */
  if ((tcwd && directory == 0)
      || (posixly_correct && same_file (".", tcwd, nullptr, nullptr) == 0))
    {
      if (directory && directory != tcwd)
        delete[] directory;
      directory = resetpwd ("pwd");
    }

#undef tcwd

  if (directory)
    {
      opt = EXECUTION_SUCCESS;
      printf ("%s\n", directory);
      /* This is dumb but posix-mandated. */
      if (posixly_correct && pflag)
        opt = setpwd (directory);
      if (directory != the_current_working_directory)
        delete[] directory;
      return sh_chkwrite (opt);
    }
  else
    return EXECUTION_FAILURE;
}

/* Do the work of changing to the directory NEWDIR.  Handle symbolic
   link following, etc.  This function *must* return with
   the_current_working_directory either set to NULL (in which case
   getcwd() will eventually be called), or set to a string corresponding
   to the working directory.  Return 1 on success, 0 on failure. */

bool
Shell::change_to_directory (const char *newdir, bool nolinks, bool xattr)
{
  char *t;

  if (the_current_working_directory == nullptr)
    {
      t = get_working_directory ("chdir");
      delete[] t;
    }

  t = make_absolute (newdir, the_current_working_directory);

  /* TDIR is either the canonicalized absolute pathname of NEWDIR
     (nolinks == 0) or the absolute physical pathname of NEWDIR
     (nolinks != 0). */
  char *tdir = nolinks ? sh_physpath (t)
                       : sh_canonpath (t, PATH_CHECKDOTDOT | PATH_CHECKEXISTS);

  int ndlen = strlen (newdir);

  /* Use the canonicalized version of NEWDIR, or, if canonicalization
     failed, use the non-canonical form. */
  bool canon_failed = false;
  if (tdir && *tdir)
    delete[] t;
  else
    {
      delete[] tdir;
      tdir = t;
      canon_failed = true;
    }

    /* In POSIX mode, if we're resolving symlinks logically and sh_canonpath
       returns NULL (because it checks the path, it will return NULL if the
       resolved path doesn't exist), fail immediately. */
#if defined(ENAMETOOLONG)
  if (posixly_correct && !nolinks && canon_failed
      && (errno != ENAMETOOLONG || ndlen > PATH_MAX))
#else
  if (posixly_correct && !nolinks && canon_failed && ndlen > PATH_MAX)
#endif
    {
#if defined ENAMETOOLONG
      if (errno != ENOENT && errno != ENAMETOOLONG)
#else
      if (errno != ENOENT)
#endif
        errno = ENOTDIR;
      delete[] tdir;
      return false;
    }

  int r;
#if defined(O_XATTR)
  if (xattrflag)
    {
      r = cdxattr (nolinks ? newdir : tdir, &ndir);
      if (r >= 0)
        {
          canon_failed = false;
          delete[] tdir;
          tdir = ndir;
        }
      else
        {
          err = errno;
          delete[] tdir;
          errno = err;
          return false; /* no xattr */
        }
    }
  else
#endif
    {
      r = chdir (nolinks ? newdir : tdir);
      if (r >= 0)
        resetxattr ();
    }

  /* If the chdir succeeds, update the_current_working_directory. */
  if (r == 0)
    {
      /* If canonicalization failed, but the chdir succeeded, reset the
         shell's idea of the_current_working_directory. */
      if (canon_failed)
        {
          t = resetpwd ("cd");
          if (t == 0)
            set_working_directory (tdir);
          else
            delete[] t;
        }
      else
        set_working_directory (tdir);

      delete[] tdir;
      return true;
    }

  /* We failed to change to the appropriate directory name.  If we tried
     what the user passed (nolinks != 0), punt now. */
  if (nolinks)
    {
      delete[] tdir;
      return false;
    }

  int err = errno;

  /* We're not in physical mode (nolinks == 0), but we failed to change to
     the canonicalized directory name (TDIR).  Try what the user passed
     verbatim. If we succeed, reinitialize the_current_working_directory.
     POSIX requires that we just fail here, so we do in posix mode. */
  if (!posixly_correct && chdir (newdir) == 0)
    {
      t = resetpwd ("cd");
      if (t == 0)
        set_working_directory (tdir);
      else
        delete[] t;

      r = 1;
    }
  else
    {
      errno = err;
      r = 0;
    }

  delete[] tdir;
  return r != 0;
}

} // namespace bash
