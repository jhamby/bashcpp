/* rlshell.h -- utility functions normally provided by bash. */

/* Copyright (C) 1999-2009 Free Software Foundation, Inc.

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

#if !defined(_RL_SHELL_H_)
#define _RL_SHELL_H_

namespace readline
{

// Useful helper functions used by history and readline.

// Create a new copy of null-terminated string s. Free with delete[].
static inline char *
savestring (const char *s)
{
  return strcpy (new char[1 + strlen (s)], s);
}

// Create a new copy of C++ string s. Free with delete[].
static inline char *
savestring (const std::string &s)
{
  return strcpy (new char[1 + s.size ()], s.c_str ());
}

// Create a new copy of C++ string_view s. Free with delete[].
static inline char *
savestring (string_view s)
{
  char *result = strcpy (new char[1 + s.size ()], s.data ());
  result[s.size ()] = '\0';
  return result;
}

#if 0
// Compare two strings for equality.
static inline bool
STREQ (const char *a, const char *b)
{
  return strcmp (a, b) == 0;
}

// Compare two strings for equality, up to n characters.
static inline bool
STREQN (const char *a, const char *b, size_t n)
{
  return strncmp (a, b, n) == 0;
}
#endif

// This can't be an abstract class because we need to cast pointers to it.

class ReadlineShell
{
public:
  virtual ~ReadlineShell ();

  virtual std::string sh_single_quote (const std::string &);

  virtual void sh_set_lines_and_columns (uint32_t, uint32_t);
  virtual const std::string *sh_get_env_value (const std::string &);
  virtual const char *sh_get_home_dir ();
  virtual int sh_unset_nodelay_mode (int);
};

} // namespace readline

#endif /* _RL_SHELL_H_ */
