/* tilde.c -- Tilde expansion code (~/foo := $HOME/foo). */

/* Copyright (C) 1988-2020 Free Software Foundation, Inc.

   This file is part of the GNU Readline Library (Readline), a library
   for reading lines of text with interactive input and history editing.

   Readline is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Readline is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Readline.  If not, see <http://www.gnu.org/licenses/>.
*/

#if defined (HAVE_CONFIG_H)
#  include <config.h>
#endif

#if defined (HAVE_UNISTD_H)
#  ifdef _MINIX
#    include <sys/types.h>
#  endif
#  include <unistd.h>
#endif

#include <cstring>
#include <cstdlib>

#include <string>

#include <sys/types.h>
#if defined (HAVE_PWD_H)
#include <pwd.h>
#endif

#include "tilde.h"

#if !defined (HAVE_GETPW_DECLS)
#  if defined (HAVE_GETPWUID)
extern struct passwd *getpwuid (uid_t);
#  endif
#  if defined (HAVE_GETPWNAM)
extern struct passwd *getpwnam (const char *);
#  endif
#endif /* !HAVE_GETPW_DECLS */

/* If being compiled as part of bash, these will be satisfied from
   variables.o.  If being compiled as part of readline, they will
   be satisfied from shell.o. */
extern char *sh_get_home_dir (void);
extern char *sh_get_env_value (const char *);

/* The default value of tilde_additional_prefixes.  This is set to
   whitespace preceding a tilde so that simple programs which do not
   perform any word separation get desired behaviour. */
static const char *default_prefixes[] =
  { " ~", "\t~", NULL };

/* The default value of tilde_additional_suffixes.  This is set to
   whitespace or newline so that simple programs which do not
   perform any word separation get desired behaviour. */
static const char *default_suffixes[] =
  { " ", "\n", NULL };

/* If non-null, this contains the address of a function that the application
   wants called before trying the standard tilde expansions.  The function
   is called with the text sans tilde, and returns a new'd string
   which is the expansion, or a NULL pointer if the expansion fails. */
tilde_hook_func_t *tilde_expansion_preexpansion_hook = NULL;

/* If non-null, this contains the address of a function to call if the
   standard meaning for expanding a tilde fails.  The function is called
   with the text (sans tilde, as in "foo"), and returns a new'd string
   which is the expansion, or a NULL pointer if there is no expansion. */
tilde_hook_func_t *tilde_expansion_failure_hook = NULL;

/* When non-null, this is a NULL terminated array of strings which
   are duplicates for a tilde prefix.  Bash uses this to expand
   `=~' and `:~'. */
char **tilde_additional_prefixes = const_cast<char **> (default_prefixes);

/* When non-null, this is a NULL terminated array of strings which match
   the end of a username, instead of just "/".  Bash sets this to
   `:' and `=~'. */
char **tilde_additional_suffixes = const_cast<char **> (default_suffixes);

static size_t tilde_find_prefix (const char *, size_t *);
static size_t tilde_find_suffix (const char *);
static char *isolate_tilde_prefix (const char *, size_t *);
static char *glue_prefix_and_suffix (char *, const char *, size_t);

// Create a new copy of null-terminated string s. Free with delete[].
// Copied from general.h.
static inline char *savestring(const char *s) {
  return std::strcpy(new char[1 + std::strlen(s)], s);
}

// Create a new copy of C++ string s. Free with delete[].
// Copied from general.h.
static inline char *savestring(const std::string &s) {
  return std::strcpy(new char[1 + s.size()], s.c_str());
}

/* Find the start of a tilde expansion in STRING, and return the index of
   the tilde which starts the expansion.  Place the length of the text
   which identified this tilde starter in LEN, excluding the tilde itself. */
static size_t
tilde_find_prefix (const char *string, size_t *len)
{
  char **prefixes = tilde_additional_prefixes;

  size_t string_len = std::strlen (string);
  *len = 0;

  if (*string == '\0' || *string == '~')
    return 0;

  if (prefixes)
    {
      for (size_t i = 0; i < string_len; i++)
	{
	  for (size_t j = 0; prefixes[j]; j++)
	    {
	      if (std::strncmp (string + i, prefixes[j], std::strlen (prefixes[j])) == 0)
		{
		  *len = std::strlen (prefixes[j]) - 1;
		  return (i + *len);
		}
	    }
	}
    }
  return string_len;
}

/* Find the end of a tilde expansion in STRING, and return the index of
   the character which ends the tilde definition.  */
static size_t
tilde_find_suffix (const char *string)
{
  char **suffixes = tilde_additional_suffixes;
  size_t string_len = std::strlen (string);
  size_t i;

  for (i = 0; i < string_len; i++)
    {
#if defined (__MSDOS__)
      if (string[i] == '/' || string[i] == '\\' /* || !string[i] */)
#else
      if (string[i] == '/' /* || !string[i] */)
#endif
	break;

      for (size_t j = 0; suffixes && suffixes[j]; j++)
	{
	  if (std::strncmp (string + i, suffixes[j], std::strlen (suffixes[j])) == 0)
	    return (i);
	}
    }
  return (i);
}

/* Return a new string which is the result of tilde expanding STRING. */
char *
tilde_expand (const char *str)
{
  std::string result;

  /* Scan through STRING expanding tildes as we come to them. */
  while (1)
    {
      char *tilde_word, *expansion;
      size_t len;

      /* Make START point to the tilde which starts the expansion. */
      size_t start = tilde_find_prefix (str, &len);

      /* Copy the skipped text into the result. */
      result.append(str, start);

      /* Advance STRING to the starting tilde. */
      str += start;

      /* Make END be the index of one after the last character of the
	 username. */
      size_t end = tilde_find_suffix (str);

      /* If both START and END are zero, we are all done. */
      if (!start && !end)
	break;

      /* Expand the entire tilde word, and copy it into RESULT. */
      tilde_word = new char[1 + end];
      std::strncpy (tilde_word, str, end);
      tilde_word[end] = '\0';
      str += end;

      expansion = tilde_expand_word (tilde_word);

      if (expansion == NULL)
	expansion = tilde_word;
      else
	delete[] tilde_word;

      len = std::strlen (expansion);
#ifdef __CYGWIN__
      /* Fix for Cygwin to prevent ~user/xxx from expanding to //xxx when
	 $HOME for `user' is /.  On cygwin, // denotes a network drive. */
      if (len > 1 || *expansion != '/' || *string != '/')
#endif
	{
	  result += expansion;
	}
      delete[] expansion;
    }

  return savestring(result);
}

/* Take FNAME and return the tilde prefix we want expanded.  If LENP is
   non-null, the index of the end of the prefix into FNAME is returned in
   the location it points to. */
static char *
isolate_tilde_prefix (const char *fname, size_t *lenp)
{
  char *ret;
  size_t i;

  ret = new char[std::strlen (fname)];
#if defined (__MSDOS__)
  for (i = 1; fname[i] && fname[i] != '/' && fname[i] != '\\'; i++)
#else
  for (i = 1; fname[i] && fname[i] != '/'; i++)
#endif
    ret[i - 1] = fname[i];
  ret[i - 1] = '\0';
  if (lenp)
    *lenp = i;

  return ret;
}

/* Return a string that is PREFIX concatenated with SUFFIX starting at
   SUFFIND. */
static char *
glue_prefix_and_suffix (char *prefix, const char *suffix, size_t suffind)
{
  size_t plen = (prefix && *prefix) ? std::strlen (prefix) : 0;
  size_t slen = std::strlen (suffix + suffind);
  char *ret = new char[plen + slen + 1];
  if (plen)
    std::strcpy (ret, prefix);
  std::strcpy (ret + plen, suffix + suffind);
  return ret;
}

/* Do the work of tilde expansion on FILENAME.  FILENAME starts with a
   tilde.  If there is no expansion, call tilde_expansion_failure_hook.
   This always returns a newly-allocated string, never static storage. */
char *
tilde_expand_word (const char *filename)
{
  char *dirname, *expansion, *username;
  struct passwd *user_entry;

  if (filename == NULL)
    return NULL;

  if (*filename != '~')
    return (savestring (filename));

  /* A leading `~/' or a bare `~' is *always* translated to the value of
     $HOME or the home directory of the current user, regardless of any
     preexpansion hook. */
  if (filename[1] == '\0' || filename[1] == '/')
    {
      /* Prefix $HOME to the rest of the string. */
      expansion = sh_get_env_value ("HOME");
#if defined (_WIN32)
      if (expansion == NULL)
	expansion = sh_get_env_value ("APPDATA");
#endif

      /* If there is no HOME variable, look up the directory in
	 the password database. */
      if (expansion == NULL)
	expansion = sh_get_home_dir ();

      return (glue_prefix_and_suffix (expansion, filename, 1));
    }

  size_t user_len;
  username = isolate_tilde_prefix (filename, &user_len);

  if (tilde_expansion_preexpansion_hook)
    {
      expansion = (*tilde_expansion_preexpansion_hook) (username);
      if (expansion)
	{
	  dirname = glue_prefix_and_suffix (expansion, filename, user_len);
	  delete[] username;
	  delete[] expansion;
	  return (dirname);
	}
    }

  /* No preexpansion hook, or the preexpansion hook failed.  Look in the
     password database. */
  dirname = NULL;

#if defined (HAVE_GETPWNAM)
  user_entry = ::getpwnam (username);
#else
  user_entry = 0;
#endif

  if (user_entry == NULL)
    {
      /* If the calling program has a special syntax for expanding tildes,
	 and we couldn't find a standard expansion, then let them try. */
      if (tilde_expansion_failure_hook)
	{
	  expansion = (*tilde_expansion_failure_hook) (username);
	  if (expansion)
	    {
	      dirname = glue_prefix_and_suffix (expansion, filename, user_len);
	      delete[] expansion;
	    }
	}
      /* If we don't have a failure hook, or if the failure hook did not
	 expand the tilde, return a copy of what we were passed. */
      if (dirname == NULL)
	dirname = savestring (filename);
    }
#if defined (HAVE_GETPWENT)
  else
    dirname = glue_prefix_and_suffix (user_entry->pw_dir, filename, user_len);
#endif

  delete[] username;
#if defined (HAVE_GETPWENT)
  ::endpwent ();
#endif
  return (dirname);
}

#if defined (TEST)
#undef NULL
#include <stdio.h>

main (int argc, char **argv)
{
  char *result, line[512];
  int done = 0;

  while (!done)
    {
      std::printf ("~expand: ");
      std::fflush (stdout);

      if (!std::gets (line))
	std::strcpy (line, "done");

      if ((std::strcmp (line, "done") == 0) ||
	  (std::strcmp (line, "quit") == 0) ||
	  (std::strcmp (line, "exit") == 0))
	{
	  done = 1;
	  break;
	}

      result = tilde_expand (line);
      std::printf ("  --> %s\n", result);
      delete[] result;
    }
  std::exit (0);
}

/*
 * Local variables:
 * compile-command: "gcc -g -DTEST -o tilde tilde.c"
 * end:
 */
#endif /* TEST */
