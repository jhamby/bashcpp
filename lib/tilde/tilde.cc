/* tilde.cc -- Tilde expansion code (~/foo := $HOME/foo). */

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

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#if defined(HAVE_PWD_H)
#include <pwd.h>
#endif

#if !defined(HAVE_GETPW_DECLS)
#if defined(HAVE_GETPWUID)
extern "C" struct passwd *getpwuid (uid_t);
#endif
#if defined(HAVE_GETPWNAM)
extern "C" struct passwd *getpwnam (const char *);
#endif
#endif /* !HAVE_GETPW_DECLS */

#include "shell.hh"

namespace bash
{

static char *isolate_tilde_prefix (const char *, size_t *);
static char *glue_prefix_and_suffix (const char *, const char *, size_t);

/* Find the start of a tilde expansion in STRING, and return the index of
   the tilde which starts the expansion.  Place the length of the text
   which identified this tilde starter in LEN, excluding the tilde itself. */
size_t
Shell::tilde_find_prefix (const char *string, size_t *len)
{
  char **prefixes = tilde_additional_prefixes;

  size_t string_len = strlen (string);
  *len = 0;

  if (*string == '\0' || *string == '~')
    return 0;

  if (prefixes)
    {
      for (size_t i = 0; i < string_len; i++)
        {
          for (size_t j = 0; prefixes[j]; j++)
            {
              if (strncmp (string + i, prefixes[j], strlen (prefixes[j])) == 0)
                {
                  *len = strlen (prefixes[j]) - 1;
                  return (i + *len);
                }
            }
        }
    }
  return string_len;
}

/* Find the end of a tilde expansion in STRING, and return the index of
   the character which ends the tilde definition.  */
size_t
Shell::tilde_find_suffix (const char *string)
{
  char **suffixes = tilde_additional_suffixes;
  size_t string_len = strlen (string);
  size_t i;

  for (i = 0; i < string_len; i++)
    {
#if defined(__MSDOS__)
      if (string[i] == '/' || string[i] == '\\' /* || !string[i] */)
#else
      if (string[i] == '/' /* || !string[i] */)
#endif
        break;

      for (size_t j = 0; suffixes && suffixes[j]; j++)
        {
          if (strncmp (string + i, suffixes[j], strlen (suffixes[j])) == 0)
            return (i);
        }
    }
  return (i);
}

/* Return a new string which is the result of tilde expanding STRING. */
char *
Shell::tilde_expand (const char *str)
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
      result.append (str, start);

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
      strncpy (tilde_word, str, end);
      tilde_word[end] = '\0';
      str += end;

      expansion = tilde_expand_word (tilde_word);

      if (expansion == nullptr)
        expansion = tilde_word;
      else
        delete[] tilde_word;

      len = strlen (expansion);
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

  return savestring (result);
}

/* Take FNAME and return the tilde prefix we want expanded.  If LENP is
   non-null, the index of the end of the prefix into FNAME is returned in
   the location it points to. */
static inline char *
isolate_tilde_prefix (const char *fname, size_t *lenp)
{
  char *ret;
  size_t i;

  ret = new char[strlen (fname)];
#if defined(__MSDOS__)
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
glue_prefix_and_suffix (const char *prefix, const char *suffix, size_t suffind)
{
  size_t plen = (prefix && *prefix) ? strlen (prefix) : 0;
  size_t slen = strlen (suffix + suffind);
  char *ret = new char[plen + slen + 1];
  if (plen)
    strcpy (ret, prefix);
  strcpy (ret + plen, suffix + suffind);
  return ret;
}

/* Do the work of tilde expansion on FILENAME.  FILENAME starts with a
   tilde.  If there is no expansion, call tilde_expansion_failure_hook.
   This always returns a newly-allocated string, never static storage. */
char *
Shell::tilde_expand_word (const char *filename)
{
  const char *expansion;
  struct passwd *user_entry;

  if (filename == nullptr)
    return nullptr;

  if (*filename != '~')
    return (savestring (filename));

  /* A leading `~/' or a bare `~' is *always* translated to the value of
     $HOME or the home directory of the current user, regardless of any
     preexpansion hook. */
  if (filename[1] == '\0' || filename[1] == '/')
    {
      /* Prefix $HOME to the rest of the string. */
      expansion = sh_get_env_value ("HOME");
#if defined(_WIN32)
      if (expansion == nullptr)
        expansion = sh_get_env_value ("APPDATA");
#endif

      /* If there is no HOME variable, look up the directory in
         the password database. */
      if (expansion == nullptr)
        expansion = sh_get_home_dir ().c_str ();

      return glue_prefix_and_suffix (expansion, filename, 1);
    }

  size_t user_len;
  char *username = isolate_tilde_prefix (filename, &user_len);

  if (tilde_expansion_preexpansion_hook)
    {
      expansion = ((*this).*tilde_expansion_preexpansion_hook) (username);
      if (expansion)
        {
          char *dirname
              = glue_prefix_and_suffix (expansion, filename, user_len);
          delete[] username;
          delete[] expansion;
          return dirname;
        }
    }

  /* No preexpansion hook, or the preexpansion hook failed.  Look in the
     password database. */
  char *dirname = nullptr;

#if defined(HAVE_GETPWNAM)
  user_entry = getpwnam (username);
#else
  user_entry = 0;
#endif

  if (user_entry == nullptr)
    {
      /* If the calling program has a special syntax for expanding tildes,
         and we couldn't find a standard expansion, then let them try. */
      if (tilde_expansion_failure_hook)
        {
          expansion = ((*this).*tilde_expansion_failure_hook) (username);
          if (expansion)
            {
              dirname = glue_prefix_and_suffix (expansion, filename, user_len);
              delete[] expansion;
            }
        }
      /* If we don't have a failure hook, or if the failure hook did not
         expand the tilde, return a copy of what we were passed. */
      if (dirname == nullptr)
        dirname = savestring (filename);
    }
#if defined(HAVE_GETPWENT)
  else
    dirname = glue_prefix_and_suffix (user_entry->pw_dir, filename, user_len);
#endif

  delete[] username;
#if defined(HAVE_GETPWENT)
  endpwent ();
#endif
  return (dirname);
}

} // namespace bash

#if defined(TEST)
#undef nullptr
#include <stdio.h>

main (int argc, char **argv)
{
  char *result, line[512];
  int done = 0;

  while (!done)
    {
      printf ("~expand: ");
      fflush (stdout);

      if (!gets (line))
        strcpy (line, "done");

      if ((strcmp (line, "done") == 0) || (strcmp (line, "quit") == 0)
          || (strcmp (line, "exit") == 0))
        {
          done = 1;
          break;
        }

      result = tilde_expand (line);
      printf ("  --> %s\n", result);
      delete[] result;
    }
  exit (0);
}

/*
 * Local variables:
 * compile-command: "gcc -g -DTEST -o tilde tilde.c"
 * end:
 */
#endif /* TEST */
