/* glob.c -- file-name wildcard pattern matching for Bash.

   Copyright (C) 1985-2020 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne-Again SHell.

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

/* To whomever it may concern: I have never seen the code which most
   Unix programs use to perform this function.  I wrote this from scratch
   based on specifications for the pattern matching.  --RMS.  */

#include "config.hh"

#include "bashtypes.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "posixdir.hh"
#include "posixstat.hh"
#include "shmbutil.hh"

#include "filecntl.hh"
#if !defined(F_OK)
#define F_OK 0
#endif

#include <csignal>

#include "general.hh"
#include "shell.hh"

#include "glob.hh"
#include "strmatch.hh"

namespace bash
{

struct globval
{
  struct globval *next;
  char *name;
};

// extern int sh_eaccess (const char *, int);
// extern char *sh_makepath (const char *, const char *, int);
// extern int signal_is_pending (int);
// extern void run_pending_traps ();

// extern bool extended_glob;	// FIXME: link to ShellOptions

/* Global variable which controls whether or not * matches .*.
   Non-zero means don't match .*.  */
bool noglob_dot_filenames = true;

/* Global variable which controls whether or not filename globbing
   is done without regard to case. */
bool glob_ignore_case = false;

/* Global variable controlling whether globbing ever returns . or ..
   regardless of the pattern. If set to 1, no glob pattern will ever
   match `.' or `..'. Disabled by default. */
bool glob_always_skip_dot_and_dotdot = false;

/* Global variable to return to signify an error in globbing. */
char *glob_error_return;

// Private exception for finddirs() failures.
class finddirs_exception : public std::exception
{
};

/* Some forward declarations. */
static bool skipname (const char *, const char *, glob_flags);
#if HANDLE_MULTIBYTE
static bool mbskipname (const char *, const char *, glob_flags);
#endif
void udequote_pathname (unsigned char *);
#if HANDLE_MULTIBYTE
void wcdequote_pathname (wchar_t *);
static void wdequote_pathname (char *);
#else
#define dequote_pathname udequote_pathname
#endif
static void dequote_pathname (unsigned char *);
static int glob_testdir (const char *, glob_flags);
static char **glob_dir_to_array (const char *, char **, glob_flags);

/* Make sure these names continue to agree with what's in smatch.c */
extern const unsigned char *glob_patscan (unsigned const char *,
                                          unsigned const char *, int);
extern const wchar_t *glob_patscan_wc (const wchar_t *, const wchar_t *,
                                       wint_t);

/* And this from gmisc.cc/gm_loop.hh */
extern int wextglob_pattern_p (const wchar_t *);

extern char *glob_dirscan (const char *, char);

/* Compile `glob_loop.hh' for single-byte characters. */
#define GCHAR unsigned char
#define CHAR char
#define INT int
#define L(CS) CS
#define INTERNAL_GLOB_PATTERN_P internal_glob_pattern_p
#include "glob_loop.hh"

/* Compile `glob_loop.hh' again for multibyte characters. */
#if HANDLE_MULTIBYTE

#define GCHAR wchar_t
#define CHAR wchar_t
#define INT wint_t
#define L(CS) L##CS
#define INTERNAL_GLOB_PATTERN_P internal_glob_wpattern_p
#include "glob_loop.hh"

#endif /* HANDLE_MULTIBYTE */

/* And now a function that calls either the single-byte or multibyte version
   of internal_glob_pattern_p. */
bool
glob_pattern_p (const char *pattern)
{
#if HANDLE_MULTIBYTE
  size_t n;
  wchar_t *wpattern;
  bool r;

  if (MB_CUR_MAX == 1 || mbsmbchar (pattern) == 0)
    return internal_glob_pattern_p ((unsigned char *)pattern);

  /* Convert strings to wide chars, and call the multibyte version. */
  n = xdupmbstowcs (&wpattern, NULL, pattern);
  if (n == (size_t)-1)
    /* Oops.  Invalid multibyte sequence.  Try it as single-byte sequence. */
    return internal_glob_pattern_p ((unsigned char *)pattern);

  r = internal_glob_wpattern_p (wpattern);
  free (wpattern);

  return r;
#else
  return internal_glob_pattern_p ((unsigned char *)pattern);
#endif
}

#if EXTENDED_GLOB
/* Return 1 if all subpatterns in the extended globbing pattern PAT indicate
   that the name should be skipped.  XXX - doesn't handle pattern negation,
   not sure if it should */
static bool
extglob_skipname (const char *pat, const char *dname, glob_flags flags)
{
  bool r;

  int negate = *pat == '!';
  int wild = *pat == '*' || *pat == '?';
  const char *pp = pat + 2;
  const char *se = pp + std::strlen (pp) - 1; /* end of string */
  char *pe = (char *)glob_patscan ((unsigned char *)pp, (unsigned char *)se,
                                   GX_NOFLAGS); /* end of extglob pattern (( */
  /* we should check for invalid extglob pattern here */
  if (pe == 0)
    return 0;

  /* if pe != se we have more of the pattern at the end of the extglob
     pattern. Check the easy case first ( */
  if (pe == se && *pe == ')' && strchr (pp, '|') == 0)
    {
      *pe = '\0';
#if defined(HANDLE_MULTIBYTE)
      r = mbskipname (pp, dname, flags);
#else
      r = skipname (pp, dname, flags); /*(*/
#endif
      *pe = ')';
      if (wild && pe[1]) /* if we can match zero instances, check further */
        return skipname (pe + 1, dname, flags);
      return r;
    }

  /* Is the extglob pattern between the parens the null pattern?  The null
     pattern can match nothing, so should we check any remaining portion of
     the pattern? */
  int nullpat = pe >= (pat + 2) && pe[-2] == '(' && pe[-1] == ')';

  /* check every subpattern */
  char *t;
  while ((t = (char *)glob_patscan ((unsigned char *)pp, (unsigned char *)pe,
                                    '|')))
    {
      int n = t[-1]; /* ( */
      if (extglob_pattern_p (pp) && n == ')')
        t[-1] = n; /* no-op for now */
      else
        t[-1] = '\0';
#if defined(HANDLE_MULTIBYTE)
      r = mbskipname (pp, dname, flags);
#else
      r = skipname (pp, dname, flags);
#endif
      t[-1] = n;
      if (r == 0) /* if any pattern says not skip, we don't skip */
        return r;
      pp = t;
    } /*(*/

  /* glob_patscan might find end of string */
  if (pp == se)
    return r;

  /* but if it doesn't then we didn't match a leading dot */
  if (wild && *pe) /* if we can match zero instances, check further */
    return skipname (pe, dname, flags);
  return 1;
}
#endif

/* Return 1 if DNAME should be skipped according to PAT.  Mostly concerned
   with matching leading `.'. */
static int
skipname (const char *pat, const char *dname, glob_flags flags)
{
#if EXTENDED_GLOB
  if (extglob_pattern_p (pat)) /* XXX */
    return extglob_skipname (pat, dname, flags);
#endif

  if (glob_always_skip_dot_and_dotdot && DOT_OR_DOTDOT (dname))
    return 1;

  /* If a leading dot need not be explicitly matched, and the pattern
     doesn't start with a `.', don't match `.' or `..' */
  if (noglob_dot_filenames == 0 && pat[0] != '.'
      && (pat[0] != '\\' || pat[1] != '.') && DOT_OR_DOTDOT (dname))
    return 1;

  /* If a dot must be explicitly matched, check to see if they do. */
  else if (noglob_dot_filenames && dname[0] == '.' && pat[0] != '.'
           && (pat[0] != '\\' || pat[1] != '.'))
    return 1;

  return 0;
}

#if HANDLE_MULTIBYTE

static int
wskipname (const wchar_t *pat, const wchar_t *dname, glob_flags flags)
{
  if (glob_always_skip_dot_and_dotdot && WDOT_OR_DOTDOT (dname))
    return 1;

  /* If a leading dot need not be explicitly matched, and the
     pattern doesn't start with a `.', don't match `.' or `..' */
  if (noglob_dot_filenames == 0 && pat[0] != L'.'
      && (pat[0] != L'\\' || pat[1] != L'.') && WDOT_OR_DOTDOT (dname))
    return 1;

  /* If a leading dot must be explicitly matched, check to see if the
     pattern and dirname both have one. */
  else if (noglob_dot_filenames && dname[0] == L'.' && pat[0] != L'.'
           && (pat[0] != L'\\' || pat[1] != L'.'))
    return 1;

  return 0;
}

static int
wextglob_skipname (const wchar_t *pat, const wchar_t *dname, glob_flags flags)
{
#if EXTENDED_GLOB
  int r;

  int negate = *pat == L'!';
  int wild = *pat == L'*' || *pat == L'?';
  const wchar_t *pp = pat + 2;
  const wchar_t *se = pp + wcslen (pp) - 1; /*(*/
  const wchar_t *pe = glob_patscan_wc (pp, se, 0);

  if (pe == se && *pe == ')' && wcschr (pp, L'|') == 0)
    {
      *(char *)pe = L'\0';
      r = wskipname (pp, dname, flags); /*(*/
      *(char *)pe = L')';
      if (wild && pe[1] != L'\0')
        return wskipname (pe + 1, dname, flags);
      return r;
    }

  /* Is the extglob pattern between the parens the null pattern?  The null
     pattern can match nothing, so should we check any remaining portion of
     the pattern? */
  int nullpat = pe >= (pat + 2) && pe[-2] == L'(' && pe[-1] == L')';

  /* check every subpattern */
  wchar_t *t;
  while ((t = (wchar_t *)glob_patscan_wc (pp, pe, '|')))
    {
      wchar_t n = t[-1]; /* ( */
      if (wextglob_pattern_p (pp) && n == L')')
        t[-1] = n; /* no-op for now */
      else
        t[-1] = L'\0';
      r = wskipname (pp, dname, flags);
      t[-1] = n;
      if (r == 0)
        return 0;
      pp = t;
    }

  if (pp == pe) /* glob_patscan_wc might find end of pattern */
    return r;

  /* but if it doesn't then we didn't match a leading dot */
  if (wild && *pe != L'\0')
    return wskipname (pe, dname, flags);
  return 1;
#else
  return wskipname (pat, dname, flags);
#endif
}

/* Return 1 if DNAME should be skipped according to PAT.  Handles multibyte
   characters in PAT and DNAME.  Mostly concerned with matching leading `.'. */
static int
mbskipname (const char *pat, const char *dname, glob_flags flags)
{
  int ret, ext;
  wchar_t *pat_wc, *dn_wc;
  size_t pat_n, dn_n;

  if (mbsmbchar (dname) == 0 && mbsmbchar (pat) == 0)
    return skipname (pat, dname, flags);

  ext = 0;
#if EXTENDED_GLOB
  ext = extglob_pattern_p (pat);
#endif

  pat_wc = dn_wc = (wchar_t *)NULL;

  pat_n = xdupmbstowcs (&pat_wc, NULL, pat);
  if (pat_n != (size_t)-1)
    dn_n = xdupmbstowcs (&dn_wc, NULL, dname);

  ret = 0;
  if (pat_n != (size_t)-1 && dn_n != (size_t)-1)
    ret = ext ? wextglob_skipname (pat_wc, dn_wc, flags)
              : wskipname (pat_wc, dn_wc, flags);
  else
    ret = skipname (pat, dname, flags);

  FREE (pat_wc);
  FREE (dn_wc);

  return ret;
}
#endif /* HANDLE_MULTIBYTE */

/* Remove backslashes quoting characters in PATHNAME by modifying PATHNAME. */
void
udequote_pathname (unsigned char *pathname)
{
  int i, j;

  for (i = j = 0; pathname && pathname[i];)
    {
      if (pathname[i] == '\\')
        i++;

      pathname[j++] = pathname[i++];

      if (pathname[i - 1] == 0)
        break;
    }
  if (pathname)
    pathname[j] = '\0';
}

#if HANDLE_MULTIBYTE
/* Remove backslashes quoting characters in PATHNAME by modifying PATHNAME. */
void
wcdequote_pathname (wchar_t *wpathname)
{
  int i, j;

  for (i = j = 0; wpathname && wpathname[i];)
    {
      if (wpathname[i] == L'\\')
        i++;

      wpathname[j++] = wpathname[i++];

      if (wpathname[i - 1] == L'\0')
        break;
    }
  if (wpathname)
    wpathname[j] = L'\0';
}

static void
wdequote_pathname (char *pathname)
{
  size_t len, n;
  wchar_t *wpathname;

  if (mbsmbchar (pathname) == 0)
    {
      udequote_pathname ((unsigned char *)pathname);
      return;
    }

  len = strlen (pathname);
  /* Convert the strings into wide characters.  */
  n = xdupmbstowcs (&wpathname, NULL, pathname);
  if (n == (size_t)-1)
    {
      /* Something wrong.  Fall back to single-byte */
      udequote_pathname ((unsigned char *)pathname);
      return;
    }
  wchar_t *orig_wpathname = wpathname;

  wcdequote_pathname (wpathname);

  /* Convert the wide character string into unibyte character set. */
  mbstate_t ps;
  memset (&ps, '\0', sizeof (mbstate_t));
  n = wcsrtombs (pathname, (const wchar_t **)&wpathname, len, &ps);
  if (n == (size_t)-1
      || (wpathname && *wpathname != 0)) /* what? now you tell me? */
    {
      wpathname = orig_wpathname;
      memset (&ps, '\0', sizeof (mbstate_t));
      n = xwcsrtombs (pathname, (const wchar_t **)&wpathname, len, &ps);
    }
  pathname[len] = '\0';

  /* Can't just free wpathname here; wcsrtombs changes it in many cases. */
  free (orig_wpathname);
}

static void
dequote_pathname (unsigned char *pathname)
{
  if (MB_CUR_MAX > 1)
    wdequote_pathname ((char *)pathname);
  else
    udequote_pathname ((unsigned char *)pathname);
}
#endif /* HANDLE_MULTIBYTE */

/* Test whether NAME exists. */

#if defined(HAVE_LSTAT)
#define GLOB_TESTNAME(name) (lstat (name, &finfo))
#else /* !HAVE_LSTAT */
#define GLOB_TESTNAME(name) (sh_eaccess (name, F_OK))
#endif /* !HAVE_LSTAT */

/* Return 0 if DIR is a directory, -2 if DIR is a symlink,  -1 otherwise. */
static int
glob_testdir (const char *dir, glob_flags flags)
{
  struct stat finfo;
  int r;

/*itrace("glob_testdir: testing %s" flags = %d, dir, flags);*/
#if defined(HAVE_LSTAT)
  r = (flags & GX_ALLDIRS) ? lstat (dir, &finfo) : stat (dir, &finfo);
#else
  r = stat (dir, &finfo);
#endif
  if (r < 0)
    return -1;

#if defined(S_ISLNK)
  if (S_ISLNK (finfo.st_mode))
    return -2;
#endif

  if (S_ISDIR (finfo.st_mode) == 0)
    return -1;

  return 0;
}

/* Recursively scan SDIR for directories matching PAT (PAT is always `**').
   FLAGS is simply passed down to the recursive call to glob_vector.  Returns
   a list of matching directory names.  EP, if non-null, is set to the last
   element of the returned list.  NP, if non-null, is set to the number of
   directories in the returned list.  These two variables exist for the
   convenience of the caller (always glob_vector). */
static struct globval *
finddirs (const char *pat, const char *sdir, glob_flags flags,
          struct globval **ep, int *np)
{
  char **r, *n;
  int ndirs;
  struct globval *ret, *e, *g;

  /*itrace("finddirs: pat = `%s' sdir = `%s' flags = 0x%x", pat, sdir,
   * flags);*/
  e = ret = 0;
  r = glob_vector (pat, sdir, flags);
  if (r == 0 || r[0] == 0)
    {
      if (np)
        *np = 0;
      if (ep)
        *ep = 0;
      if (r && r != &glob_error_return)
        delete r;
      return (struct globval *)0;
    }
  for (ndirs = 0; r[ndirs] != 0; ndirs++)
    {
      g = new globval;
      if (g == 0)
        {
          while (ret) /* free list built so far */
            {
              g = ret->next;
              delete ret;
              ret = g;
            }

          delete r;
          if (np)
            *np = 0;
          if (ep)
            *ep = 0;
          return &finddirs_error_return;
        }
      if (e == 0)
        e = g;

      g->next = ret;
      ret = g;

      g->name = r[ndirs];
    }

  delete r;
  if (ep)
    *ep = e;
  if (np)
    *np = ndirs;

  return ret;
}

/* Return a vector of names of files in directory DIR
   whose names match glob pattern PAT.
   The names are not in any particular order.
   Wildcards at the beginning of PAT do not match an initial period.

   The vector is terminated by an element that is a null pointer.

   To free the space allocated, first free the vector's elements,
   then free the vector.

   Throws std::bad_alloc if it can't allocate memory.

   Return -1 if cannot access directory DIR.
   Look in errno for more information.  */

char **
glob_vector (const char *pat, const char *dir, glob_flags flags)
{
  struct globval *lastlink = NULL;
  bool skip = false;
  int count = 0;

  /*itrace("glob_vector: pat = `%s' dir = `%s' flags = 0x%x", pat, dir,
   * flags);*/
  /* If PAT is empty, skip the loop, but return one (empty) filename. */
  if (pat == 0 || *pat == '\0')
    {
      if (glob_testdir (dir, 0) < 0)
        return (char **)&glob_error_return;

      struct globval *nextlink = new globval;
      nextlink->next = (struct globval *)0;
      char *nextname = new char[1];

      lastlink = nextlink;
      nextlink->name = nextname;
      nextname[0] = '\0';
      count = 1;

      skip = true;
    }

  size_t patlen = pat ? strlen (pat) : 0;

  /* If the filename pattern (PAT) does not contain any globbing characters,
     we can dispense with reading the directory, and just see if there is
     a filename `DIR/PAT'.  If there is, and we can access it, just make the
     vector to return and bail immediately. */
  if (!skip && !(glob_pattern_p (pat)))
    {
      int dirlen;
      struct stat finfo;

      if (glob_testdir (dir, 0) < 0)
        return (char **)&glob_error_return;

      dirlen = strlen (dir);
      char *nextname = new char[dirlen + patlen + 2];
      char *npat = new char[patlen + 1];

      strcpy (npat, pat);
      dequote_pathname ((unsigned char *)npat);

      strcpy (nextname, dir);
      nextname[dirlen++] = '/';
      strcpy (nextname + dirlen, npat);

      if (GLOB_TESTNAME (nextname) >= 0)
        {
          delete[] nextname;
          struct globval *nextlink = new globval;
          nextlink->next = (struct globval *)0;
          lastlink = nextlink;
          nextlink->name = npat;
          count = 1;
        }
      else
        {
          delete[] nextname;
          delete[] npat;
        }

      skip = true;
    }

  struct globval *firstnew = NULL;
  bool add_current = false;
  if (!skip)
    {
      /* Open the directory, punting immediately if we cannot.  If opendir
         is not robust (i.e., it opens non-directories successfully), test
         that DIR is a directory and punt if it's not. */
#if defined(OPENDIR_NOT_ROBUST)
      if (glob_testdir (dir, 0) < 0)
        return (char **)&glob_error_return;
#endif

      DIR *d = opendir (dir);
      if (d == NULL)
        return (char **)&glob_error_return;

      /* Compute the flags that will be passed to strmatch().  We don't
         need to do this every time through the loop. */
      int mflags = (noglob_dot_filenames ? FNM_PERIOD : 0) | FNM_PATHNAME;

#ifdef FNM_CASEFOLD
      if (glob_ignore_case)
        mflags |= FNM_CASEFOLD;
#endif

      if (extended_glob)
        mflags |= FNM_EXTMATCH;

      add_current = ((flags & (GX_ALLDIRS | GX_ADDCURDIR))
                     == (GX_ALLDIRS | GX_ADDCURDIR));

      bool lose = false;

      /* Scan the directory, finding all names that match.
         For each name that matches, allocate a struct globval
         on the stack and store the name in it.
         Chain those structs together; lastlink is the front of the chain.  */
      while (1)
        {
          /* Make globbing interruptible in the shell. */
          if (interrupt_state || terminating_signal)
            {
              lose = true;
              break;
            }
          else if (signal_is_pending (
                       SIGINT)) /* XXX - make SIGINT traps responsive */
            {
              lose = true;
              break;
            }

          struct dirent *dp = readdir (d);
          if (dp == NULL)
            break;

          /* If this directory entry is not to be used, try again. */
          if (REAL_DIR_ENTRY (dp) == 0)
            continue;

#if 0
	  if (dp->d_name == 0 || *dp->d_name == 0)
	    continue;
#endif

#if HANDLE_MULTIBYTE
          if (MB_CUR_MAX > 1 && mbskipname (pat, dp->d_name, flags))
            continue;
          else
#endif
              if (skipname (pat, dp->d_name, flags))
            continue;

          /* If we're only interested in directories, don't bother with files
           */
          char *subdir = 0;
          int isdir = 0;
          if (flags & (GX_MATCHDIRS | GX_ALLDIRS))
            {
              int pflags = (flags & GX_ALLDIRS) ? MP_RMDOT : 0;
              if (flags & GX_NULLDIR)
                pflags |= MP_IGNDOT;
              subdir = sh_makepath (dir, dp->d_name, pflags);
              isdir = glob_testdir (subdir, flags);
              if (isdir < 0 && (flags & GX_MATCHDIRS))
                {
                  delete[] subdir;
                  continue;
                }
            }

          if (flags & GX_ALLDIRS)
            {
              if (isdir == 0)
                {
                  struct globval *e;
                  int ndirs;
                  struct globval *dirlist = finddirs (
                      pat, subdir, (flags & ~GX_ADDCURDIR), &e, &ndirs);
                  if (dirlist == &finddirs_error_return)
                    {
                      delete[] subdir;
                      lose = true;
                      break;
                    }
                  if (ndirs) /* add recursive directories to list */
                    {
                      if (firstmalloc == 0)
                        firstmalloc = e;
                      e->next = lastlink;
                      lastlink = dirlist;
                      count += ndirs;
                    }
                }

              /* XXX - should we even add this if it's not a directory? */
              struct globval *nextlink = new globval;
              if (firstmalloc == 0)
                firstmalloc = nextlink;
              int sdlen = strlen (subdir);

              char *nextname = new char[sdlen + 1];
              nextlink->next = lastlink;
              lastlink = nextlink;
              nextlink->name = nextname;
              bcopy (subdir, nextname, sdlen + 1);
              delete[] subdir;
              ++count;
              continue;
            }
          else if (flags & GX_MATCHDIRS)
            delete[] subdir;

          char *convfn = fnx_fromfs (dp->d_name, D_NAMLEN (dp));
          if (strmatch (pat, convfn, mflags) != FNM_NOMATCH)
            {
              struct globval *nextlink;
              if (nalloca < ALLOCA_MAX)
                {
                  nextlink
                      = (struct globval *)alloca (sizeof (struct globval));
                  nalloca += sizeof (struct globval);
                }
              else
                {
                  nextlink
                      = (struct globval *)malloc (sizeof (struct globval));
                  if (firstmalloc == 0)
                    firstmalloc = nextlink;
                }

              char *nextname = (char *)malloc (D_NAMLEN (dp) + 1);
              nextlink->next = lastlink;
              lastlink = nextlink;
              nextlink->name = nextname;
              bcopy (dp->d_name, nextname, D_NAMLEN (dp) + 1);
              ++count;
            }
        }

      (void)::closedir (d);
    }

  /* compat: if GX_ADDCURDIR, add the passed directory also.  Add an empty
     directory name as a placeholder if GX_NULLDIR (in which case the passed
     directory name is "."). */
  if (add_current)
    {
      int sdlen = strlen (dir);
      char *nextname = (char *)malloc (sdlen + 1);
      struct globval *nextlink = new globval;
      if (nextlink == 0 || nextname == 0)
        {
          delete nextlink;
          delete nextname;
          lose = true;
        }
      else
        {
          nextlink->name = nextname;
          nextlink->next = lastlink;
          lastlink = nextlink;
          if (flags & GX_NULLDIR)
            nextname[0] = '\0';
          else
            bcopy (dir, nextname, sdlen + 1);
          ++count;
        }
    }

  char **name_vector;
  if (!lose)
    {
      name_vector = new char *[count + 1];
      lose |= name_vector == NULL;
    }

  /* Have we run out of memory?	 */
  if (lose)
    {
      /* Here free the strings we have got.  */
      while (lastlink)
        {
          /* Since we build the list in reverse order, the first N entries
             will be allocated with malloc, if firstmalloc is set, from
             lastlink to firstmalloc. */
          struct globval *tmplink;
          if (firstmalloc)
            {
              if (lastlink == firstmalloc)
                firstmalloc = 0;
              tmplink = lastlink;
            }
          else
            tmplink = 0;

          delete[] lastlink->name;
          lastlink = lastlink->next;
          delete[] tmplink;
        }

      /* Don't call QUIT; here; let higher layers deal with it. */

      return NULL;
    }

  /* Copy the name pointers from the linked list into the vector.  */
  struct globval *tmplink = lastlink;
  for (int i = 0; i < count; ++i)
    {
      name_vector[i] = tmplink->name;
      tmplink = tmplink->next;
    }

  name_vector[count] = NULL;

  /* If we allocated some of the struct globvals, free them now. */
  if (firstmalloc)
    {
      tmplink = 0;
      while (lastlink)
        {
          tmplink = lastlink;
          if (lastlink == firstmalloc)
            lastlink = firstmalloc = 0;
          else
            lastlink = lastlink->next;
          delete[] tmplink;
        }
    }

  return name_vector;
}

/* Return a new array which is the concatenation of each string in ARRAY
   to DIR.  This function expects you to pass in an allocated ARRAY, and
   it takes care of deleting that array.  Thus, you might think of this
   function as side-effecting ARRAY.  This should handle GX_MARKDIRS. */
static char **
glob_dir_to_array (const char *dir, char **array, glob_flags flags)
{
  size_t l = std::strlen (dir);
  if (l == 0)
    {
      if (flags & GX_MARKDIRS)
        for (int i = 0; array[i]; i++)
          {
            struct stat sb;
            if ((::stat (array[i], &sb) == 0) && S_ISDIR (sb.st_mode))
              {
                l = std::strlen (array[i]);
                char *new_ = (char *)realloc (array[i], l + 2);
                if (new_ == 0)
                  return NULL;
                new_[l] = '/';
                new_[l + 1] = '\0';
                array[i] = new_;
              }
          }
      return array;
    }

  bool add_slash = dir[l - 1] != '/';

  int size = 0;
  while (array[size] != NULL)
    ++size;

  char **result = (char **)malloc ((size + 1) * sizeof (char *));
  if (result == NULL)
    return NULL;

  for (int i = 0; i < size; i++)
    {
      /* 3 == 1 for NUL, 1 for slash at end of DIR, 1 for GX_MARKDIRS */
      result[i] = (char *)malloc (l + strlen (array[i]) + 3);

      if (result[i] == NULL)
        {
          int ind;
          for (ind = 0; ind < i; ind++)
            free (result[ind]);
          free (result);
          return NULL;
        }

      strcpy (result[i], dir);
      if (add_slash)
        result[i][l] = '/';
      if (array[i][0])
        {
          strcpy (result[i] + l + add_slash, array[i]);
          if (flags & GX_MARKDIRS)
            {
              struct stat sb;
              if ((stat (result[i], &sb) == 0) && S_ISDIR (sb.st_mode))
                {
                  size_t rlen;
                  rlen = strlen (result[i]);
                  result[i][rlen] = '/';
                  result[i][rlen + 1] = '\0';
                }
            }
        }
      else
        result[i][l + add_slash] = '\0';
    }
  result[size] = NULL;

  /* Free the input array.  */
  for (int i = 0; array[i] != NULL; i++)
    free (array[i]);
  free ((char *)array);

  return result;
}

/* Do globbing on PATHNAME.  Return an array of pathnames that match,
   marking the end of the array with a null-pointer as an element.
   If no pathnames match, then the array is empty (first element is null).
   If there isn't enough memory, then return NULL.
   If a file system error occurs, return -1; `errno' has the error code.  */
char **
glob_filename (const char *pathname, glob_flags flags)
{
  char **result = (char **)malloc (sizeof (char *));
  unsigned int result_size = 1;
  if (result == NULL)
    return NULL;

  result[0] = NULL;

  char *directory_name = NULL;

  /* Find the filename.  */
  const char *filename = strrchr (pathname, '/');
#if defined(EXTENDED_GLOB)
  if (filename && extended_glob)
    {
      char *fn = glob_dirscan (pathname, '/');
#if DEBUG_MATCHING
      if (fn != filename)
        fprintf (stderr,
                 "glob_filename: glob_dirscan: fn (%s) != filename (%s)\n",
                 fn ? fn : "(null)", filename);
#endif
      filename = fn;
    }
#endif

  unsigned int directory_len;
  bool free_dirname = false;

  if (filename == NULL)
    {
      filename = pathname;
      directory_name = (char *)"";
      directory_len = 0;
      free_dirname = false;
    }
  else
    {
      directory_len = (filename - pathname) + 1;
      directory_name = (char *)malloc (directory_len + 1);

      if (directory_name == 0) /* allocation failed? */
        {
          free (result);
          return NULL;
        }

      bcopy (pathname, directory_name, directory_len);
      directory_name[directory_len] = '\0';
      ++filename;
      free_dirname = true;
    }

  bool hasglob = false;
  /* If directory_name contains globbing characters, then we
     have to expand the previous levels.  Just recurse.
     If glob_pattern_p returns != [0,1] we have a pattern that has backslash
     quotes but no unquoted glob pattern characters. We dequote it below. */
  if (directory_len > 0 && (hasglob = glob_pattern_p (directory_name)) == 1)
    {
      char **directories, *d, *p;
      bool all_starstar = false, last_starstar = false;

      d = directory_name;
      int dflags = (int)flags & ~GX_MARKDIRS;
      /* Collapse a sequence of ** patterns separated by one or more slashes
         to a single ** terminated by a slash or NUL */
      if ((flags & GX_GLOBSTAR) && d[0] == '*' && d[1] == '*'
          && (d[2] == '/' || d[2] == '\0'))
        {
          p = d;
          while (d[0] == '*' && d[1] == '*' && (d[2] == '/' || d[2] == '\0'))
            {
              p = d;
              if (d[2])
                {
                  d += 3;
                  while (*d == '/')
                    d++;
                  if (*d == 0)
                    break;
                }
            }
          if (*d == 0)
            all_starstar = true;
          d = p;
          dflags |= (GX_ALLDIRS | GX_ADDCURDIR);
          directory_len = strlen (d);
        }

      /* If there is a non [star][star]/ component in directory_name, we
         still need to collapse trailing sequences of [star][star]/ into
         a single one and note that the directory name ends with [star][star],
         so we can compensate if filename is [star][star] */
      if ((flags & GX_GLOBSTAR) && !(all_starstar))
        {
          int dl, prev;
          prev = dl = directory_len;
          while (dl >= 4 && d[dl - 1] == '/' && d[dl - 2] == '*'
                 && d[dl - 3] == '*' && d[dl - 4] == '/')
            prev = dl, dl -= 3;
          if (dl != directory_len)
            last_starstar = true;
          directory_len = prev;
        }

      /* If the directory name ends in [star][star]/ but the filename is
         [star][star], just remove the final [star][star] from the directory
         so we don't have to scan everything twice. */
      if (last_starstar && directory_len > 4 && filename[0] == '*'
          && filename[1] == '*' && filename[2] == 0)
        {
          directory_len -= 3;
        }

      if (d[directory_len - 1] == '/')
        d[directory_len - 1] = '\0';

      directories = glob_filename (d, (dflags | GX_RECURSE));

      if (free_dirname)
        {
          free (directory_name);
          directory_name = NULL;
        }

      if (directories == NULL)
        goto memory_error;
      else if (directories == (char **)&glob_error_return)
        {
          free ((char *)result);
          return (char **)&glob_error_return;
        }
      else if (*directories == NULL)
        {
          free ((char *)directories);
          free ((char *)result);
          return (char **)&glob_error_return;
        }

      /* If we have something like [star][star]/[star][star], it's no use to
         glob **, then do it again, and throw half the results away.  */
      if (all_starstar && filename[0] == '*' && filename[1] == '*'
          && filename[2] == 0)
        {
          free ((char *)directories);
          free (directory_name);
          directory_name = NULL;
          directory_len = 0;
          goto only_filename;
        }

      /* We have successfully globbed the preceding directory name.
         For each name in DIRECTORIES, call glob_vector on it and
         FILENAME.  Concatenate the results together.  */
      for (int i = 0; directories[i] != NULL; ++i)
        {
          char **temp_results;
          bool shouldbreak = false;

          /* XXX -- we've recursively scanned any directories resulting from
             a `**', so turn off the flag.  We turn it on again below if
             filename is `**' */
          /* Scan directory even on a NULL filename.  That way, `*h/'
             returns only directories ending in `h', instead of all
             files ending in `h' with a `/' appended. */
          const char *dname = directories[i];
          dflags = flags & ~(GX_MARKDIRS | GX_ALLDIRS | GX_ADDCURDIR);
          /* last_starstar? */
          if ((flags & GX_GLOBSTAR) && filename[0] == '*' && filename[1] == '*'
              && filename[2] == '\0')
            dflags |= GX_ALLDIRS | GX_ADDCURDIR;
          if (dname[0] == '\0' && filename[0])
            {
              dflags |= GX_NULLDIR;
              dname = "."; /* treat null directory name and non-null filename
                              as current directory */
            }

          /* Special handling for symlinks to directories with globstar on */
          if (all_starstar && (dflags & GX_NULLDIR) == 0)
            {
              /* If we have a directory name that is not null (GX_NULLDIR
                 above) and is a symlink to a directory, we return the symlink
                 if we're not `descending' into it (filename[0] == 0) and
                 return glob_error_return (which causes the code below to skip
                 the name) otherwise. I should fold this into a test that does
                 both checks instead of calling stat twice. */
              if (glob_testdir (dname, flags | GX_ALLDIRS) == -2
                  && glob_testdir (dname, 0) == 0)
                {
                  if (filename[0] != 0)
                    temp_results = (char **)&glob_error_return; /* skip */
                  else
                    {
                      /* Construct array to pass to glob_dir_to_array */
                      temp_results = (char **)malloc (2 * sizeof (char *));
                      if (temp_results == NULL)
                        goto memory_error;
                      temp_results[0] = (char *)malloc (1);
                      if (temp_results[0] == 0)
                        {
                          free (temp_results);
                          goto memory_error;
                        }
                      **temp_results = '\0';
                      temp_results[1] = NULL;
                      dflags |= GX_SYMLINK; /* mostly for debugging */
                    }
                }
              else
                temp_results = glob_vector (filename, dname, dflags);
            }
          else
            temp_results = glob_vector (filename, dname, dflags);

          /* Handle error cases. */
          if (temp_results == NULL)
            goto memory_error;
          else if (temp_results == (char **)&glob_error_return)
            /* This filename is probably not a directory.  Ignore it.  */
            ;
          else
            {
              char **array;

              /* If we're expanding **, we don't need to glue the directory
                 name to the results; we've already done it in glob_vector */
              if ((dflags & GX_ALLDIRS) && filename[0] == '*'
                  && filename[1] == '*'
                  && (filename[2] == '\0' || filename[2] == '/'))
                {
                  /* When do we remove null elements from temp_results?  And
                     how to avoid duplicate elements in the final result? */
                  /* If (dflags & GX_NULLDIR) glob_filename potentially left a
                     NULL placeholder in the temp results just in case
                     glob_vector/glob_dir_to_array did something with it, but
                     if it didn't, and we're not supposed to be passing them
                     through for some reason ((flags & GX_NULLDIR) == 0) we
                     need to remove all the NULL elements from the beginning
                     of TEMP_RESULTS. */
                  /* If we have a null directory name and ** as the filename,
                     we have just searched for everything from the current
                     directory on down. Break now (shouldbreak = 1) to avoid
                     duplicate entries in the final result. */
#define NULL_PLACEHOLDER(x) ((x) && *(x) && **(x) == 0)
                  if ((dflags & GX_NULLDIR) && (flags & GX_NULLDIR) == 0
                      && NULL_PLACEHOLDER (temp_results))
#undef NULL_PLACEHOLDER
                    {
                      int i, n;
                      for (n = 0; temp_results[n] && *temp_results[n] == 0;
                           n++)
                        ;
                      i = n;
                      do
                        temp_results[i - n] = temp_results[i];
                      while (temp_results[i++] != 0);
                      array = temp_results;
                      shouldbreak = true;
                    }
                  else
                    array = temp_results;
                }
              else if (dflags & GX_SYMLINK)
                array
                    = glob_dir_to_array (directories[i], temp_results, flags);
              else
                array
                    = glob_dir_to_array (directories[i], temp_results, flags);
              int l = 0;
              while (array[l] != NULL)
                ++l;

              char **new_result = (char **)realloc (
                  result, (result_size + l) * sizeof (char *));

              if (new_result == NULL)
                {
                  for (l = 0; array[l]; ++l)
                    free (array[l]);
                  free ((char *)array);
                  goto memory_error;
                }
              result = new_result;

              for (l = 0; array[l] != NULL; ++l)
                result[result_size++ - 1] = array[l];

              result[result_size - 1] = NULL;

              /* Note that the elements of ARRAY are not freed.  */
              if (array != temp_results)
                free ((char *)array);
              else if ((dflags & GX_ALLDIRS) && filename[0] == '*'
                       && filename[1] == '*' && filename[2] == '\0')
                free (temp_results); /* expanding ** case above */

              if (shouldbreak)
                break;
            }
        }
      /* Free the directories.  */
      for (int i = 0; directories[i]; i++)
        free (directories[i]);

      free ((char *)directories);

      return result;
    }

only_filename:
  /* If there is only a directory name, return it. */
  if (*filename == '\0')
    {
      result = (char **)realloc ((char *)result, 2 * sizeof (char *));
      if (result == NULL)
        {
          if (free_dirname)
            free (directory_name);
          return NULL;
        }
#if 0
	/* Note: hasglob can no longer return 2. */
      /* If we have a directory name with quoted characters, and we are
	 being called recursively to glob the directory portion of a pathname,
	 we need to dequote the directory name before returning it so the
	 caller can read the directory */
      if (directory_len > 0 && hasglob == 2 && (flags & GX_RECURSE) != 0)
	{
	  dequote_pathname (directory_name);
	  directory_len = strlen (directory_name);
	}

      /* We could check whether or not the dequoted directory_name is a
	 directory and return it here, returning the original directory_name
	 if not, but we don't do that. We do return the dequoted directory
	 name if we're not being called recursively and the dequoted name
	 corresponds to an actual directory. For better backwards compatibility,
	 we can return &glob_error_return unconditionally in this case. */

      if (directory_len > 0 && hasglob == 2 && (flags & GX_RECURSE) == 0)
	{
	  dequote_pathname (directory_name);
	  if (glob_testdir (directory_name, 0) < 0)
	    {
	      if (free_dirname)
		free (directory_name);
	      return (char **)&glob_error_return;
	    }
	}
#endif

      /* Handle GX_MARKDIRS here. */
      result[0] = (char *)malloc (directory_len + 1);
      if (result[0] == NULL)
        goto memory_error;
      bcopy (directory_name, result[0], directory_len + 1);
      if (free_dirname)
        free (directory_name);
      result[1] = NULL;
      return result;
    }
  else
    {
      char **temp_results;

      /* There are no unquoted globbing characters in DIRECTORY_NAME.
         Dequote it before we try to open the directory since there may
         be quoted globbing characters which should be treated verbatim. */
      if (directory_len > 0)
        dequote_pathname ((unsigned char *)directory_name);

      /* We allocated a small array called RESULT, which we won't be using.
         Free that memory now. */
      free (result);

      /* Just return what glob_vector () returns appended to the
         directory name. */
      /* If flags & GX_ALLDIRS, we're called recursively */
      int dflags = flags & ~GX_MARKDIRS;
      if (directory_len == 0)
        dflags |= GX_NULLDIR;
      if ((flags & GX_GLOBSTAR) && filename[0] == '*' && filename[1] == '*'
          && filename[2] == '\0')
        {
          dflags |= GX_ALLDIRS | GX_ADDCURDIR;
#if 0
	  /* If we want all directories (dflags & GX_ALLDIRS) and we're not
	     being called recursively as something like `echo [star][star]/[star].o'
	     ((flags & GX_ALLDIRS) == 0), we want to prevent glob_vector from
	     adding a null directory name to the front of the temp_results
	     array.  We turn off ADDCURDIR if not called recursively and
	     dlen == 0 */
#endif
          if (directory_len == 0 && (flags & GX_ALLDIRS) == 0)
            dflags &= ~GX_ADDCURDIR;
        }
      temp_results = glob_vector (
          filename, (directory_len == 0 ? "." : directory_name), dflags);

      if (temp_results == NULL || temp_results == (char **)&glob_error_return)
        {
          if (free_dirname)
            free (directory_name);
          QUIT; /* XXX - shell */
          run_pending_traps ();
          return temp_results;
        }

      result = glob_dir_to_array ((dflags & GX_ALLDIRS) ? "" : directory_name,
                                  temp_results, flags);

      if (free_dirname)
        free (directory_name);
      return result;
    }

  /* We get to memory_error if the program has run out of memory, or
     if this is the shell, and we have been interrupted. */
memory_error:
  if (result != NULL)
    {
      for (int i = 0; result[i] != NULL; ++i)
        free (result[i]);
      free ((char *)result);
    }

  if (free_dirname && directory_name)
    free (directory_name);

  QUIT;
  run_pending_traps ();

  return NULL;
}

#if defined(TEST)

main (int argc, char **argv)
{
  unsigned int i;

  for (i = 1; i < argc; ++i)
    {
      char **value = glob_filename (argv[i], 0);
      if (value == NULL)
        puts ("Out of memory.");
      else if (value == &glob_error_return)
        perror (argv[i]);
      else
        for (i = 0; value[i] != NULL; i++)
          puts (value[i]);
    }

  exit (0);
}
#endif /* TEST.  */

} // namespace bash