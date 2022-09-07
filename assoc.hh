/* assoc.h -- definitions for the interface exported by assoc.c that allows
   the rest of the shell to manipulate associative array variables. */

/* Copyright (C) 2008,2009-2020 Free Software Foundation, Inc.

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

#ifndef _ASSOC_H_
#define _ASSOC_H_

#include "hashlib.hh"

namespace bash
{

const int ASSOC_HASH_BUCKETS = 1024;

void assoc_dispose (HASH_TABLE *);
void assoc_flush (HASH_TABLE *);

int assoc_insert (HASH_TABLE *, char *, const char *);
void *assoc_replace (HASH_TABLE *, char *, const char *);
void assoc_remove (HASH_TABLE *, const char *);

char *assoc_reference (HASH_TABLE *, const char *);

char *assoc_subrange (HASH_TABLE *, arrayind_t, arrayind_t, int, int, int);
char *assoc_patsub (HASH_TABLE *, const char *, const char *, int);
char *assoc_modcase (HASH_TABLE *, const char *, int, int);

HASH_TABLE *assoc_quote (HASH_TABLE *);
HASH_TABLE *assoc_quote_escapes (HASH_TABLE *);
HASH_TABLE *assoc_dequote (HASH_TABLE *);
HASH_TABLE *assoc_dequote_escapes (HASH_TABLE *);
HASH_TABLE *assoc_remove_quoted_nulls (HASH_TABLE *);

char *assoc_to_kvpair (HASH_TABLE *, int);
char *assoc_to_assign (HASH_TABLE *, int);

WORD_LIST *assoc_to_word_list (HASH_TABLE *);
WORD_LIST *assoc_keys_to_word_list (HASH_TABLE *);

char *assoc_to_string (HASH_TABLE *, const char *, int);

}  // namespace bash

#endif /* _ASSOC_H_ */
