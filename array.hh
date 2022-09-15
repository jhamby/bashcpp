/* array.h -- definitions for the interface exported by array.c that allows
   the rest of the shell to manipulate array variables. */

/* Copyright (C) 1997-2020 Free Software Foundation, Inc.

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

#ifndef _ARRAY_H_
#define _ARRAY_H_

namespace bash
{

// Type of array index.
typedef int64_t arrayind_t;

// Forward declaration of WORD_LIST type.
class WORD_LIST;

enum atype
{
  array_indexed,
  array_assoc
}; /* only array_indexed used */

struct ARRAY_ELEMENT
{
  arrayind_t ind;
  char *value;
  ARRAY_ELEMENT *next, *prev;
};

struct ARRAY
{
  arrayind_t max_index;
  unsigned int num_elements;
  atype type;
  ARRAY_ELEMENT *head;
  ARRAY_ELEMENT *lastref;
};

typedef int sh_ae_map_func_t (ARRAY_ELEMENT *, void *);

/* Basic operations on entire arrays */
ARRAY *array_create ();
void array_flush (ARRAY *);
void array_dispose (ARRAY *);
ARRAY *array_copy (ARRAY *);
ARRAY *array_slice (ARRAY *, ARRAY_ELEMENT *, ARRAY_ELEMENT *);
void array_walk (ARRAY *, sh_ae_map_func_t *, void *);

ARRAY_ELEMENT *array_shift (ARRAY *, int, int);
int array_rshift (ARRAY *, int, const char *);
ARRAY_ELEMENT *array_unshift_element (ARRAY *);
int array_shift_element (ARRAY *, char *);

ARRAY *array_quote (ARRAY *);
ARRAY *array_quote_escapes (ARRAY *);
ARRAY *array_dequote (ARRAY *);
ARRAY *array_dequote_escapes (ARRAY *);
ARRAY *array_remove_quoted_nulls (ARRAY *);

char *array_subrange (ARRAY *, arrayind_t, arrayind_t, int, int, int);
char *array_patsub (ARRAY *, char *, char *, int);
char *array_modcase (ARRAY *, char *, int, int);

/* Basic operations on array elements. */
ARRAY_ELEMENT *array_create_element (arrayind_t, const char *);
ARRAY_ELEMENT *array_copy_element (ARRAY_ELEMENT *);
void array_dispose_element (ARRAY_ELEMENT *);

int array_insert (ARRAY *, arrayind_t, const char *);
ARRAY_ELEMENT *array_remove (ARRAY *, arrayind_t);
char *array_reference (ARRAY *, arrayind_t);

/* Converting to and from arrays */
WORD_LIST *array_to_word_list (ARRAY *);
ARRAY *array_from_word_list (WORD_LIST *);
WORD_LIST *array_keys_to_word_list (ARRAY *);

ARRAY *array_assign_list (ARRAY *, WORD_LIST *);

char **array_to_argv (ARRAY *, int *);

char *array_to_kvpair (ARRAY *, int);
char *array_to_assign (ARRAY *, int);
char *array_to_string (ARRAY *, const char *, int);
ARRAY *array_from_string (char *, char *);

/* Flags for array_shift */
#define AS_DISPOSE 0x01

#define array_num_elements(a) ((a)->num_elements)
#define array_max_index(a) ((a)->max_index)
#define array_first_index(a) ((a)->head->next->ind)
#define array_head(a) ((a)->head)
#define array_empty(a) ((a)->num_elements == 0)

#define element_value(ae) ((ae)->value)
#define element_index(ae) ((ae)->ind)
#define element_forw(ae) ((ae)->next)
#define element_back(ae) ((ae)->prev)

#define set_element_value(ae, val) ((ae)->value = (val))

/* Convenience */
#define array_push(a, v)                                                      \
  do                                                                          \
    {                                                                         \
      array_rshift ((a), 1, (v));                                             \
    }                                                                         \
  while (0)
#define array_pop(a)                                                          \
  do                                                                          \
    {                                                                         \
      array_dispose_element (array_shift ((a), 1, 0));                        \
    }                                                                         \
  while (0)

#define GET_ARRAY_FROM_VAR(n, v, a)                                           \
  do                                                                          \
    {                                                                         \
      (v) = find_variable (n);                                                \
      (a) = ((v) && ((v)->array ())) ? (v)->array_value() : nullptr;          \
    }                                                                         \
  while (0)

#define ALL_ELEMENT_SUB(c) ((c) == '@' || (c) == '*')

/* In eval.c, but uses ARRAY * */
int execute_array_command (ARRAY *, void *);

} // namespace bash

#endif /* _ARRAY_H_ */
