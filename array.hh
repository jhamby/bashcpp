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

#include "bashtypes.hh"

#include "command.hh"

namespace bash
{

// Type of array index.
typedef int64_t arrayind_t;

struct ARRAY_ELEMENT
{
  // Construct a new array element with index and value.
  ARRAY_ELEMENT (arrayind_t ind_, string_view value_ = string_view ())
      : ind (ind_), value (savestring (value_))
  {
  }

  ~ARRAY_ELEMENT ()
  {
    delete[] value;
  }

  arrayind_t ind;
  char *value;

#ifndef ALT_ARRAY_IMPLEMENTATION
  ARRAY_ELEMENT *next, *prev;
#endif
};

typedef int (*sh_ae_map_func_t) (ARRAY_ELEMENT *, void *);

/* Flags for array_shift */
#define AS_DISPOSE 0x01

struct ARRAY
{
#ifndef ALT_ARRAY_IMPLEMENTATION
  ARRAY () : max_index (-1)
  {
    head = new ARRAY_ELEMENT (-1); /* dummy head */
    head->prev = head->next = head;
  }

  ~ARRAY ()
  {
    flush ();
    delete head;
  }

  // Clone the array elements.
  ARRAY (const ARRAY &);

  arrayind_t
  first_index ()
  {
    return head->next->ind;
  }

  // Walk the array, calling FUNC once for each element, with the array
  // element and user data pointer as the arguments.
  void
  walk (sh_ae_map_func_t func, void *udata)
  {
    if (empty ())
      return;

    for (ARRAY_ELEMENT *ae = head->next; ae != head; ae = ae->next)
      {
        if ((*func) (ae, udata) < 0)
          return;
      }
  }

  ARRAY *slice (ARRAY_ELEMENT *, ARRAY_ELEMENT *);

  ARRAY_ELEMENT *shift (size_t, int);

  ARRAY_ELEMENT *
  unshift_element ()
  {
    return shift (1, 0);
  }

  size_t
  shift_element (string_view v)
  {
    return rshift (1, v);
  }

  void
  push (string_view v)
  {
    (void)rshift (1, v);
  }

  void
  pop ()
  {
    (void)shift (1, AS_DISPOSE);
  }

#else // ALT_ARRAY_IMPLEMENTATION

  ARRAY () : max_index (-1), num_elements (0), first_index_ (-1) {}

  ~ARRAY ()
  {
    std::vector<ARRAY_ELEMENT *>::iterator it;
    for (it = elements.begin (); it != elements.end (); ++it)
      delete *it;
  }

  ARRAY (const ARRAY &);

  arrayind_t
  first_index ()
  {
    return first_index_;
  }

  void
  resize (size_t n)
  {
    if (elements.size () > 0 && n >= static_cast<size_t> (max_index)
        && n <= elements.size ())
      return;

    elements.resize (n);
  }

  void expand (size_t);

  ARRAY *slice (arrayind_t, arrayind_t);

  // Walk the array, calling FUNC once for each element, with the array
  // element and user data pointer as the arguments.
  void
  walk (sh_ae_map_func_t func, void *udata)
  {
    size_t i;
    ARRAY_ELEMENT *ae;

    if (empty ())
      return;

    for (i = static_cast<size_t> (first_index_);
         i <= static_cast<size_t> (max_index); i++)
      {
        if ((ae = elements[i]) == nullptr)
          continue;

        if ((*func) (ae, udata) < 0)
          return;
      }
  }

  ARRAY_ELEMENT **shift (size_t, int);

  ARRAY_ELEMENT *
  unshift_element ()
  {
    ARRAY_ELEMENT **r, *ret;

    r = shift (1, 0);
    ret = r[0];
    delete[] r;
    return ret;
  }

  size_t
  shift_element (string_view v)
  {
    return rshift (1, v);
  }

  // Return the next non-null array element after A[IND]
  arrayind_t
  element_forw (arrayind_t ind)
  {
    size_t i;

    for (i = static_cast<size_t> (ind + 1);
         i <= static_cast<size_t> (max_index); i++)
      if (elements[i])
        break;

    if (elements[i])
      return static_cast<arrayind_t> (i);

    return max_index;
  }

  // Return the previous non-null array element before A[IND]
  arrayind_t
  element_back (arrayind_t ind)
  {
    size_t i;

    for (i = static_cast<size_t> (ind - 1);
         i >= static_cast<size_t> (first_index_); i--)
      if (elements[i])
        break;

    if (elements[i] && i >= static_cast<size_t> (first_index_))
      return static_cast<arrayind_t> (i);

    return first_index_;
  }

#endif

  void flush ();

  bool
  empty ()
  {
    return num_elements == 0;
  }

  size_t rshift (size_t, string_view);

  char *reference (arrayind_t);

  int insert (arrayind_t, string_view);
  ARRAY_ELEMENT *remove (arrayind_t);

  arrayind_t max_index;
  size_t num_elements;

#ifdef ALT_ARRAY_IMPLEMENTATION
  arrayind_t first_index_;
  std::vector<ARRAY_ELEMENT *> elements;
#else
  ARRAY_ELEMENT *head;
  ARRAY_ELEMENT *lastref;
#endif
};

#define ARRAY_DEFAULT_SIZE 1024

/* Converting to and from arrays */

WORD_LIST *array_to_word_list (ARRAY *);
ARRAY *array_from_word_list (WORD_LIST *);
WORD_LIST *array_keys_to_word_list (ARRAY *);
WORD_LIST *array_to_kvpair_list (ARRAY *);

ARRAY *array_assign_list (ARRAY *, WORD_LIST *);

char **array_to_argv (ARRAY *, int *);
ARRAY *array_from_argv (ARRAY *, char **, int);

std::string array_to_kvpair (ARRAY *, bool);
std::string array_to_assign (ARRAY *, bool);
std::string array_to_string (ARRAY *, string_view, bool);
ARRAY *array_from_string (char *, char *);

/* Convenience */

#define GET_ARRAY_FROM_VAR(n, v, a)                                           \
  do                                                                          \
    {                                                                         \
      (v) = find_variable (n);                                                \
      (a) = ((v) && ((v)->array ())) ? (v)->array_value () : nullptr;         \
    }                                                                         \
  while (0)

#define ALL_ELEMENT_SUB(c) ((c) == '@' || (c) == '*')

} // namespace bash

#endif /* _ARRAY_H_ */
