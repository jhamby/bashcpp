/*
 * array.cc - functions to create, destroy, access, and manipulate arrays
 *	      of strings.
 *
 * Arrays are sparse doubly-linked lists.  An element's index is stored
 * with it.
 *
 * Chet Ramey
 * chet@ins.cwru.edu
 */

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

#include "config.hh"

#if defined(ARRAY_VARS)

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "shell.hh"

#include "builtins/common.hh"

namespace bash
{

#define ADD_BEFORE(ae, new_e)                                                 \
  do                                                                          \
    {                                                                         \
      ae->prev->next = new_e;                                                 \
      new_e->prev = ae->prev;                                                 \
      ae->prev = new_e;                                                       \
      new_e->next = ae;                                                       \
    }                                                                         \
  while (0)

#define ADD_AFTER(ae, new_e)                                                  \
  do                                                                          \
    {                                                                         \
      ae->next->prev = new_e;                                                 \
      new_e->next = ae->next;                                                 \
      new_e->prev = ae;                                                       \
      ae->next = new_e;                                                       \
    }                                                                         \
  while (0)

#define LASTREF(a) ((a)->lastref ? (a)->lastref : ((a)->head->next))

void
ARRAY::flush ()
{
  for (ARRAY_ELEMENT *r = head->next; r != head;)
    {
      ARRAY_ELEMENT *r1 = r->next;
      delete r;
      r = r1;
    }

  head->next = head->prev = head;
  max_index = -1;
  num_elements = 0;
  lastref = nullptr;
}

// Array copy constructor.
ARRAY::ARRAY (const ARRAY &other)
    : max_index (other.max_index), num_elements (other.num_elements)
{
  ARRAY_ELEMENT *ae, *new_e;

  for (ae = other.head->next; ae != other.head; ae = ae->next)
    {
      new_e = new ARRAY_ELEMENT (ae->ind, ae->value);
      ADD_BEFORE (head, new_e);
      if (ae == LASTREF (&other))
        lastref = new_e;
    }
}

/*
 * Make and return a new array composed of the elements in array A from
 * S to E, inclusive.
 */
ARRAY *
ARRAY::slice (ARRAY_ELEMENT *s, ARRAY_ELEMENT *e)
{
  ARRAY_ELEMENT *p, *n;
  size_t i;
  arrayind_t mi;

  ARRAY *a = new ARRAY;

  for (mi = 0, p = s, i = 0; p != e; p = p->next, i++)
    {
      n = new ARRAY_ELEMENT (p->ind, p->value);
      ADD_BEFORE (a->head, n);
      mi = n->ind;
    }

  a->num_elements = i;
  a->max_index = mi;
  return a;
}

/*
 * Shift the array A N elements to the left.  Delete the first N elements
 * and subtract N from the indices of the remaining elements.  If FLAGS
 * does not include AS_DISPOSE, this returns a singly-linked null-terminated
 * list of elements so the caller can dispose of the chain.  If FLAGS
 * includes AS_DISPOSE, this function disposes of the shifted-out elements
 * and returns nullptr.
 */
ARRAY_ELEMENT *
ARRAY::shift (size_t n, int flags)
{
  ARRAY_ELEMENT *ae, *ret;
  size_t i;

  if (empty () || n <= 0)
    return nullptr;

  lastref = nullptr;
  for (i = 0, ret = ae = head->next; ae != head && i < n; ae = ae->next, i++)
    ;

  if (ae == head)
    {
      /* Easy case; shifting out all of the elements */
      if (flags & AS_DISPOSE)
        {
          flush ();
          return nullptr;
        }

      for (ae = ret; ae->next != head; ae = ae->next)
        ;

      ae->next = nullptr;
      head->next = head->prev = head;
      max_index = -1;
      num_elements = 0;
      return ret;
    }
  /*
   * ae now points to the list of elements we want to retain.
   * ret points to the list we want to either destroy or return.
   */
  ae->prev->next = nullptr; /* null-terminate RET */

  head->next = ae; /* slice RET out of the array */
  ae->prev = head;

  for (; ae != head; ae = ae->next)
    ae->ind -= n; /* renumber retained indices */

  num_elements -= n; /* modify bookkeeping information */
  max_index = head->prev->ind;

  if (flags & AS_DISPOSE)
    {
      for (ae = ret; ae;)
        {
          ret = ae->next;
          delete ae;
          ae = ret;
        }
      return nullptr;
    }

  return ret;
}

/*
 * Shift array A right N indices.  If S is non-empty, it becomes the value of
 * the new element 0.  Returns the number of elements in the array after the
 * shift.
 */
size_t
ARRAY::rshift (size_t n, string_view s)
{
  if (empty () && s.empty ())
    return 0;

  else if (n <= 0)
    return num_elements;

  ARRAY_ELEMENT *ae = head->next;
  if (!s.empty ())
    {
      ARRAY_ELEMENT *new_e = new ARRAY_ELEMENT (0, s);
      ADD_BEFORE (ae, new_e);
      num_elements++;
      if (num_elements == 1)
        { /* array was empty */
          max_index = 0;
          return 1;
        }
    }

  /*
   * Renumber all elements in the array except the one we just added.
   */
  for (; ae != head; ae = ae->next)
    ae->ind += n;

  max_index = head->prev->ind;

  lastref = nullptr;
  return num_elements;
}

/*
 * Return a string whose elements are the members of array A beginning at
 * index START and spanning NELEM members.  Null elements are counted.
 * Since arrays are sparse, unset array elements are not counted.
 */
std::string
Shell::array_subrange (ARRAY *a, arrayind_t start, arrayind_t nelem,
                       int starsub, quoted_flags quoted, param_flags pflags)
{
  ARRAY *a2;
  ARRAY_ELEMENT *h, *p;
  arrayind_t i;
  WORD_LIST *wl;

  p = a ? a->head : nullptr;
  if (p == nullptr || a->empty () || start > a->max_index)
    return nullptr;

  /*
   * Find element with index START.  If START corresponds to an unset
   * element (arrays can be sparse), use the first element whose index
   * is >= START.  If START is < 0, we count START indices back from
   * the end of A (not elements, even with sparse arrays -- START is an
   * index).
   */
  for (p = p->next; p != a->head && start > p->ind; p = p->next)
    ;

  if (p == a->head)
    return nullptr;

  /* Starting at P, take NELEM elements, inclusive. */
  for (i = 0, h = p; p != a->head && i < nelem; i++, p = p->next)
    ;

  a2 = a->slice (h, p);

  wl = array_to_word_list (a2);
  delete a2;

  if (wl == nullptr)
    return nullptr;

  std::string t (
      string_list_pos_params (starsub ? '*' : '@', wl, quoted, pflags));
  delete wl;

  return t;
}

std::string
Shell::array_patsub (ARRAY *a, string_view pat, string_view rep,
                     match_flags mflags)
{
  WORD_LIST *wl, *save;

  if (a == nullptr || a->head == nullptr || a->empty ())
    return nullptr;

  wl = array_to_word_list (a);
  if (wl == nullptr)
    return nullptr;

  for (save = wl; wl; wl = wl->next ())
    {
      std::string t (pat_subst (wl->word->word, pat, rep, mflags));
      wl->word->word = t;
    }

  char pchar = (mflags & MATCH_STARSUB) == MATCH_STARSUB ? '*' : '@';
  quoted_flags qflags
      = (mflags & MATCH_QUOTED) == MATCH_QUOTED ? Q_DOUBLE_QUOTES : Q_NOFLAGS;
  param_flags pflags = (mflags & MATCH_ASSIGNRHS) ? PF_ASSIGNRHS : PF_NOFLAGS;

  std::string t (string_list_pos_params (pchar, save, qflags, pflags));
  delete save;

  return t;
}

std::string
Shell::array_modcase (ARRAY *a, string_view pat, sh_modcase_flags modop,
                      match_flags mflags)
{
  WORD_LIST *wl, *save;

  if (a == nullptr || a->head == nullptr || a->empty ())
    return nullptr;

  wl = array_to_word_list (a);
  if (wl == nullptr)
    return nullptr;

  for (save = wl; wl; wl = wl->next ())
    {
      std::string t (sh_modcase (wl->word->word, pat, modop));
      wl->word->word = t;
    }

  char pchar = (mflags & MATCH_STARSUB) == MATCH_STARSUB ? '*' : '@';
  quoted_flags qflags
      = (mflags & MATCH_QUOTED) == MATCH_QUOTED ? Q_DOUBLE_QUOTES : Q_NOFLAGS;
  param_flags pflags = (mflags & MATCH_ASSIGNRHS) ? PF_ASSIGNRHS : PF_NOFLAGS;

  std::string t (string_list_pos_params (pchar, save, qflags, pflags));
  delete save;

  return t;
}

/*
 * Add a new element with index I and value V to array A (a[i] = v).
 */
int
ARRAY::insert (arrayind_t i, string_view v)
{
  ARRAY_ELEMENT *new_e = new ARRAY_ELEMENT (i, v);
  if (i > max_index)
    {
      /*
       * Hook onto the end.  This also works for an empty array.
       * Fast path for the common case of allocating arrays
       * sequentially.
       */
      ADD_BEFORE (head, new_e);
      max_index = i;
      num_elements++;
      lastref = new_e;
      return 0;
    }
  else if (i < first_index ())
    {
      /* Hook at the beginning */
      ADD_AFTER (head, new_e);
      num_elements++;
      lastref = new_e;
      return 0;
    }

#if OPTIMIZE_SEQUENTIAL_ARRAY_ASSIGNMENT
  /*
   * Otherwise we search for the spot to insert it.  The lastref
   * handle optimizes the case of sequential or almost-sequential
   * assignments that are not at the end of the array.
   */
  ARRAY_ELEMENT *start = LASTREF (this);

  /* Use same strategy as array_reference to avoid paying large penalty
     for semi-random assignment pattern. */
  arrayind_t startind = start->ind;
  int direction;
  if (i < startind / 2)
    {
      start = head->next;
      startind = start->ind;
      direction = 1;
    }
  else if (i >= startind)
    {
      direction = 1;
    }
  else
    {
      direction = -1;
    }
#else
  start = element_forw (ae->head);
  startind = element_index (start);
  direction = 1;
#endif

  for (ARRAY_ELEMENT *ae = start; ae != head;)
    {
      if (ae->ind == i)
        {
          /*
           * Replacing an existing element.
           */
          /* Just swap in the new value */
          ae->value = new_e->value;
          delete new_e;
          lastref = ae;
          return 0;
        }
      else if (direction == 1 && ae->ind > i)
        {
          ADD_BEFORE (ae, new_e);
          num_elements++;
          lastref = ae;
          return 0;
        }
      else if (direction == -1 && ae->ind < i)
        {
          ADD_AFTER (ae, new_e);
          num_elements++;
          lastref = ae;
          return 0;
        }
      ae = direction == 1 ? ae->next : ae->prev;
    }

  delete new_e;
  lastref = nullptr;

  return -1; /* problem */
}

/*
 * Delete the element with index I from array A and return it so the
 * caller can dispose of it.
 */
ARRAY_ELEMENT *
ARRAY::remove (arrayind_t i)
{
  if (empty ())
    return nullptr;

  if (i > max_index || i < first_index ())
    return nullptr;

  // Keep roving pointer into array to optimize sequential access
  ARRAY_ELEMENT *start = LASTREF (this);

  /* Use same strategy as array_reference to avoid paying large penalty
     for semi-random assignment pattern. */
  arrayind_t startind = start->ind;
  int direction;
  if (i < startind / 2)
    {
      start = head->next;
      startind = start->ind;
      direction = 1;
    }
  else if (i >= startind)
    {
      direction = 1;
    }
  else
    {
      direction = -1;
    }
  for (ARRAY_ELEMENT *ae = start; ae != head;)
    {
      if (ae->ind == i)
        {
          ae->next->prev = ae->prev;
          ae->prev->next = ae->next;
          num_elements--;

          if (i == max_index)
            max_index = ae->prev->ind;
#if 0
			INVALIDATE_LASTREF(a);
#else
          if (ae->next != head)
            lastref = ae->next;
          else if (ae->prev != head)
            lastref = ae->prev;
          else
            lastref = nullptr;
#endif
          return ae;
        }

      ae = (direction == 1) ? ae->next : ae->prev;
      if (direction == 1 && ae->ind > i)
        break;
      else if (direction == -1 && ae->ind < i)
        break;
    }

  return nullptr;
}

/*
 * Return the value of a[i], or nullptr. The caller must copy the value
 * from the pointed-to std::string if it's needed past the lifetime of
 * the ARRAY_ELEMENT object, or if the array element value may change.
 */
const std::string *
ARRAY::reference (arrayind_t i)
{
  if (empty ())
    return nullptr;

  if (i > max_index || i < first_index ())
    return nullptr;

  // Keep roving pointer into array to optimize sequential access.
  ARRAY_ELEMENT *start = LASTREF (this); /* lastref pointer */

  arrayind_t startind = start->ind;
  int direction;
  if (i < startind / 2)
    { /* XXX - guess */
      start = head->next;
      startind = start->ind;
      direction = 1;
    }
  else if (i >= startind)
    {
      direction = 1;
    }
  else
    {
      direction = -1;
    }

  for (ARRAY_ELEMENT *ae = start; ae != head;)
    {
      if (ae->ind == i)
        {
          lastref = ae;
          return &(ae->value);
        }
      ae = (direction == 1) ? ae->next : ae->prev;
      /* Take advantage of index ordering to short-circuit */
      /* If we don't find it, set the lastref pointer to the element
         that's `closest', assuming that the unsuccessful reference
         will quickly be followed by an assignment.  No worse than
         not changing it from the previous value or resetting it. */
      if (direction == 1 && ae->ind > i)
        {
          start = ae; /* use for SET_LASTREF below */
          break;
        }
      else if (direction == -1 && ae->ind < i)
        {
          start = ae; /* use for SET_LASTREF below */
          break;
        }
    }

  lastref = start;
  return nullptr;
}

/* Convenience routines for the shell to translate to and from the form used
   by the rest of the code. */

WORD_LIST *
array_to_word_list (ARRAY *a)
{
  WORD_LIST *list;
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  list = nullptr;
  for (ae = a->head->next; ae != a->head; ae = ae->next)
    list = new WORD_LIST (new WORD_DESC (ae->value), list);

  return list ? list->reverse () : nullptr;
}

ARRAY *
array_from_word_list (WORD_LIST *list)
{
  if (list == nullptr)
    return nullptr;

  ARRAY *a = new ARRAY ();
  return array_assign_list (a, list);
}

WORD_LIST *
array_keys_to_word_list (ARRAY *a)
{
  WORD_LIST *list;
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  list = nullptr;

  for (ae = a->head->next; ae != a->head; ae = ae->next)
    {
      std::string t (itos (ae->ind));
      list = new WORD_LIST (new WORD_DESC (t), list);
    }

  return list ? list->reverse () : nullptr;
}

WORD_LIST *
array_to_kvpair_list (ARRAY *a)
{
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  WORD_LIST *list = nullptr;

  for (ae = a->head->next; ae != a->head; ae = ae->next)
    {
      std::string k (itos (ae->ind));
      list = new WORD_LIST (new WORD_DESC (k), list);
      list = new WORD_LIST (new WORD_DESC (ae->value), list);
    }

  return list ? list->reverse () : nullptr;
}

ARRAY *
array_assign_list (ARRAY *array, WORD_LIST *list)
{
  WORD_LIST *l;
  arrayind_t i;

  for (l = list, i = 0; l; l = l->next (), i++)
    array->insert (i, l->word->word);

  return array;
}

char **
array_to_argv (ARRAY *a, int *countp)
{
  char **ret;
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    {
      if (countp)
        *countp = 0;
      return nullptr;
    }

  ret = new char *[static_cast<size_t> (a->num_elements + 1)];

  int i = 0;

  for (ae = a->head->next; ae != a->head; ae = ae->next)
    {
      if (!ae->value.empty ())
        ret[i++] = savestring (ae->value);
    }

  ret[i] = nullptr;

  if (countp)
    *countp = i;

  return ret;
}

ARRAY *
array_from_argv (ARRAY *a, char **vec, int count)
{
  arrayind_t i;
  ARRAY_ELEMENT *ae;

  if (a->num_elements == 0)
    {
      for (i = 0; i < count; i++)
        a->insert (i, string_view ());
      return a;
    }

  /* Fast case */
  if (static_cast<int> (a->num_elements) == count && count == 1)
    {
      ae = a->head->next;
      if (vec[0])
        ae->value = vec[0];
      else
        ae->value.clear ();
    }
  else if (static_cast<int> (a->num_elements) <= count)
    {
      /* modify in array_num_elements members in place, then add */
      ae = a->head;
      for (i = 0; i < static_cast<arrayind_t> (a->num_elements); i++)
        {
          ae = ae->next;
          if (vec[0])
            ae->value = vec[0];
          else
            ae->value.clear ();
        }
      /* add any more */
      for (; i < count; i++)
        a->insert (i, vec[i]);
    }
  else
    {
      /* deleting elements.  it's faster to rebuild the array. */
      a->flush ();
      for (i = 0; i < count; i++)
        a->insert (i, vec[i]);
    }

  return a;
}

std::string
array_to_kvpair (ARRAY *a, bool quoted)
{
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  std::string result;

  for (ae = a->head->next; ae != a->head; ae = ae->next)
    {
      std::string is (inttostr (ae->ind));
      std::string valstr (!ae->value.empty ()
                              ? (ansic_shouldquote (ae->value)
                                     ? ansic_quote (ae->value)
                                     : sh_double_quote (ae->value))
                              : std::string ());

      result += is;
      result.push_back (' ');

      if (!valstr.empty ())
        result += valstr;
      else
        result += "\"\"";

      if (ae->next != a->head)
        result.push_back (' ');
    }

  if (quoted)
    return sh_single_quote (result);

  return result;
}

std::string
array_to_assign (ARRAY *a, bool quoted)
{
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  std::string result;

  for (ae = a->head->next; ae != a->head; ae = ae->next)
    {
      std::string is (inttostr (ae->ind));
      std::string valstr (!ae->value.empty ()
                              ? (ansic_shouldquote (ae->value)
                                     ? ansic_quote (ae->value)
                                     : sh_double_quote (ae->value))
                              : std::string ());

      result.push_back ('[');
      result += is;
      result.push_back (']');
      result.push_back ('=');

      if (!valstr.empty ())
        result += valstr;

      if (ae->next != a->head)
        result.push_back (' ');
    }

  result.push_back (')');

  if (quoted)
    return sh_single_quote (result);

  return result;
}

#if defined(TEST_ARRAY)
/*
 * Return an array consisting of elements in S, separated by SEP
 */
ARRAY *
array_from_string (char *s, char *sep)
{
  ARRAY *a;
  WORD_LIST *w;

  if (s == 0)
    return nullptr;
  w = list_string (s, sep, 0);
  if (w == 0)
    return nullptr;
  a = array_from_word_list (w);
  return a;
}
#endif

#if defined(TEST_ARRAY)
/*
 * To make a running version, compile -DTEST_ARRAY and link with:
 * 	xmalloc.o syntax.o lib/malloc/libmalloc.a lib/sh/libsh.a
 */
int interrupt_immediately = 0;

int
signal_is_trapped (int s)
{
  return 0;
}

void
fatal_error (const char *s, ...)
{
  std::fprintf (stderr, "array_test: fatal memory error\n");
  std::abort ();
}

void
programming_error (const char *s, ...)
{
  std::fprintf (stderr, "array_test: fatal programming error\n");
  std::abort ();
}

WORD_DESC *
make_bare_word (const char *s)
{
  WORD_DESC *w;

  w = (WORD_DESC *)xmalloc (sizeof (WORD_DESC));
  w->word = s ? savestring (s) : savestring ("");
  w->flags = 0;
  return w;
}

WORD_LIST *
make_word_list (WORD_DESC *x, WORD_LIST *l)
{
  WORD_LIST *w;

  w = (WORD_LIST *)xmalloc (sizeof (WORD_LIST));
  w->word = x;
  w->next = l;
  return w;
}

WORD_LIST *
list_string (char *s, char *t, int i)
{
  char *r, *a;
  WORD_LIST *wl;

  if (s == 0)
    return nullptr;
  r = savestring (s);
  wl = nullptr;
  a = std::strtok (r, t);
  while (a)
    {
      wl = make_word_list (make_bare_word (a), wl);
      a = std::strtok (nullptr, t);
    }
  return REVERSE_LIST (wl, WORD_LIST *);
}

GENERIC_LIST *
list_reverse (GENERIC_LIST *list)
{
  GENERIC_LIST *next, *prev;

  for (prev = 0; list;)
    {
      next = list->next;
      list->next = prev;
      prev = list;
      list = next;
    }
  return prev;
}

char *
pat_subst (char *s, char *t, char *u, int i)
{
  return nullptr;
}

char *
quote_string (char *s)
{
  return savestring (s);
}

print_element (ARRAY_ELEMENT *ae)
{
  char lbuf[INT_STRLEN_BOUND (int64_t) + 1];

  printf ("array[%s] = %s\n", inttostr (ae->ind, lbuf, sizeof (lbuf)),
          ae->value);
}

print_array (ARRAY *a)
{
  printf ("\n");
  array_walk (a, print_element, nullptr);
}

main ()
{
  ARRAY *a, *new_a, *copy_of_a;
  ARRAY_ELEMENT *ae, *aew;
  char *s;

  a = array_create ();
  a->insert (1, "one");
  a->insert (7, "seven");
  a->insert (4, "four");
  a->insert (1029, "one thousand twenty-nine");
  a->insert (12, "twelve");
  a->insert (42, "forty-two");
  print_array (a);
  s = array_to_string (a, " ", 0);
  std::printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, " ");
  std::printf ("copy_of_a:");
  print_array (copy_of_a);
  array_dispose (copy_of_a);
  std::printf ("\n");
  std::free (s);
  ae = array_remove (a, 4);
  array_dispose_element (ae);
  ae = array_remove (a, 1029);
  array_dispose_element (ae);
  a->insert (16, "sixteen");
  print_array (a);
  s = array_to_string (a, " ", 0);
  std::printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, " ");
  std::printf ("copy_of_a:");
  print_array (copy_of_a);
  array_dispose (copy_of_a);
  std::printf ("\n");
  std::free (s);
  a->insert (2, "two");
  a->insert (1029, "new one thousand twenty-nine");
  a->insert (0, "zero");
  a->insert (134, "");
  print_array (a);
  s = array_to_string (a, ":", 0);
  std::printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, ":");
  std::printf ("copy_of_a:");
  print_array (copy_of_a);
  array_dispose (copy_of_a);
  std::printf ("\n");
  std::free (s);
  new_a = array_copy (a);
  print_array (new_a);
  s = array_to_string (new_a, ":", 0);
  std::printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, ":");
  std::free (s);
  std::printf ("copy_of_a:");
  print_array (copy_of_a);
  array_shift (copy_of_a, 2, AS_DISPOSE);
  std::printf ("copy_of_a shifted by two:");
  print_array (copy_of_a);
  ae = array_shift (copy_of_a, 2, 0);
  std::printf ("copy_of_a shifted by two:");
  print_array (copy_of_a);
  for (; ae;)
    {
      aew = ae->next;
      array_dispose_element (ae);
      ae = aew;
    }
  array_rshift (copy_of_a, 1, (char *)0);
  std::printf ("copy_of_a rshift by 1:");
  print_array (copy_of_a);
  array_rshift (copy_of_a, 2, "new element zero");
  std::printf ("copy_of_a rshift again by 2 with new element zero:");
  print_array (copy_of_a);
  s = array_to_assign (copy_of_a, 0);
  std::printf ("copy_of_a=%s\n", s);
  std::free (s);
  ae = array_shift (copy_of_a, array_num_elements (copy_of_a), 0);
  for (; ae;)
    {
      aew = ae->next;
      array_dispose_element (ae);
      ae = aew;
    }
  array_dispose (copy_of_a);
  std::printf ("\n");
  array_dispose (a);
  array_dispose (new_a);
}

#endif /* TEST_ARRAY */

} // namespace bash

#endif /* ARRAY_VARS */
