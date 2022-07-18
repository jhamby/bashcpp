/* stdc.h -- macros to make source compile on both ANSI C and K&R C
   compilers. */

/* Copyright (C) 1993 Free Software Foundation, Inc.

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

#if !defined (_STDC_H_)
#define _STDC_H_

/* Adapted from BSD /usr/include/sys/cdefs.h. */

/* Fortify, at least, has trouble with this definition */
#define CPP_STRING(x) #x

#ifndef __attribute__
#  if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 8)
#    define __attribute__(x)
#  endif
#endif

/* For those situations when gcc handles inlining a particular function but
   other compilers complain. */
#define INLINE inline

#define SH_VA_START(va, arg)  va_start(va, arg)

/* All structs which contain a `next' field should have that field
   as the first field in the struct.  This means that functions
   can be written to handle the general case for linked lists. */
typedef struct g_list {
  struct g_list *next;
} GENERIC_LIST;

#endif /* !_STDC_H_ */
