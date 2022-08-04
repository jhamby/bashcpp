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

#if !defined (_RL_SHELL_H_)
#define _RL_SHELL_H_

namespace readline
{

// Useful helper functions used by history and readline.

// Create a new copy of null-terminated string s. Free with delete[].
static inline char *
savestring(const char *s) {
  return std::strcpy(new char[1 + std::strlen(s)], s);
}

// Create a new copy of C++ string s. Free with delete[].
static inline char *
savestring(const std::string &s) {
  return std::strcpy(new char[1 + s.size()], s.c_str());
}

// Compare two strings for equality.
static inline bool STREQ (const char *a, const char *b) {
  return std::strcmp (a, b) == 0;
}

// Compare two strings for equality, up to n characters.
static inline bool STREQN (const char *a, const char *b, size_t n) {
  return std::strncmp (a, b, n) == 0;
}

// This is either a pure virtual class or has a default implementation,
// depending on whether we compile this library as part of bash or not.

#ifdef SHELL

class ReadlineShell {
public:
  virtual ~ReadlineShell();

  virtual char *sh_single_quote (const char *) = 0;
  virtual void sh_set_lines_and_columns (unsigned int, unsigned int) = 0;
  virtual char *sh_get_env_value (const char *) = 0;
  virtual char *sh_get_home_dir () = 0;
  virtual int sh_unset_nodelay_mode (int) = 0;
};

#else

class ReadlineShell {
public:
  virtual ~ReadlineShell();

  virtual char *sh_single_quote (const char *);
  virtual void sh_set_lines_and_columns (unsigned int, unsigned int);
  virtual char *sh_get_env_value (const char *);
  virtual char *sh_get_home_dir ();
  virtual int sh_unset_nodelay_mode (int);
};

#endif

}  // namespace readline

#endif /* _RL_SHELL_H_ */
