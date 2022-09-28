/* test.h -- external interface to the conditional command code. */

/* Copyright (C) 1997-2021 Free Software Foundation, Inc.

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

#ifndef _TEST_H_
#define _TEST_H_

#include <string>

/* Values for the flags argument to binary_test */
enum binary_test_flags
{
  TEST_NOFLAGS = 0,
  TEST_PATMATCH = 0x01,
  TEST_ARITHEXP = 0x02,
  TEST_LOCALE = 0x04,
  TEST_ARRAYEXP = 0x08 // array subscript expansion
};

extern bool test_unop (const std::string &);
extern bool test_binop (const std::string &);

extern bool unary_test (const std::string &, const std::string &);
extern bool binary_test (const std::string &, const std::string &,
                         const std::string &, binary_test_flags);

extern int test_command (int, std::string *);

#endif /* _TEST_H_ */
