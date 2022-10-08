/*
 * array2.cc - functions to create, destroy, access, and manipulate arrays
 *             of strings.
 *
 * Arrays are structs containing an array of elements and bookkeeping
 * information. An element's index is stored with it.
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

#include <cstdio>

#include "array.hh"
#include "builtins/common.hh"
#include "shell.hh"

namespace bash
{

#define ARRAY_MAX_DOUBLE 16777216

#if 0
static ARRAY_ELEMENT **array_copy_elements (ARRAY *);
static char *array_to_string_internal (ARRAY *, arrayind_t, arrayind_t, char *, int);
#endif

void
ARRAY::expand (size_t n)
{
  size_t nsize;

  if (n >= elements.size ())
    {
      nsize = elements.size () ? elements.size () : ARRAY_DEFAULT_SIZE;
      while (n >= nsize)
        nsize <<= 1;

      if (nsize > ARRAY_MAX_DOUBLE)
        nsize = n + ARRAY_DEFAULT_SIZE;

      resize (nsize);
    }
}

void
ARRAY::flush ()
{
  if (empty ())
    {
      max_index = first_index_ = -1; /* paranoia */
      return;
    }

  for (size_t r = static_cast<size_t> (first_index_);
       r <= static_cast<size_t> (max_index); r++)
    if (elements[r])
      {
        delete elements[r];
        elements[r] = nullptr;
      }

  max_index = first_index_ = -1;
  num_elements = 0;
}

// Array copy constructor.
ARRAY::ARRAY (const ARRAY &other)
    : max_index (other.max_index), num_elements (other.num_elements),
      first_index_ (other.first_index_)
{
  elements.resize (other.elements.size ());
  std::vector<ARRAY_ELEMENT *>::const_iterator src;
  std::vector<ARRAY_ELEMENT *>::iterator dst;
  for (src = other.elements.begin (), dst = elements.begin ();
       src != other.elements.end (); ++src, ++dst)
    {
      if (*src)
        *dst = new ARRAY_ELEMENT (**src);
    }
}

/*
 * Make and return a new array composed of the elements in array A from
 * S to E, inclusive. The callers do the bounds checking.
 */
ARRAY *
ARRAY::slice (arrayind_t s, arrayind_t e)
{
  ARRAY_ELEMENT *p, *n;
  size_t nsize;

  ARRAY *a = new ARRAY ();

  nsize = ARRAY_DEFAULT_SIZE;
  while (nsize < elements.size ())
    nsize <<= 1;

  if (nsize > ARRAY_MAX_DOUBLE)
    nsize = elements.size () + ARRAY_DEFAULT_SIZE;

  a->resize (nsize);

  for (arrayind_t i = s; i < e; i++)
    {
      p = elements[static_cast<size_t> (i)];
      n = p ? new ARRAY_ELEMENT (p->ind, p->value) : nullptr;
      a->elements[static_cast<size_t> (i)] = n;
    }

  a->num_elements = static_cast<size_t> (e - s);
  a->max_index = e;
  a->first_index_ = s;

  return a;
}

/*
 * Shift the array A N elements to the left.  Delete the first N elements
 * and subtract N from the indices of the remaining elements.  If FLAGS
 * does not include AS_DISPOSE, this returns a null-terminated array of
 * elements so the caller can dispose of the chain.  If FLAGS includes
 * AS_DISPOSE, this function disposes of the shifted-out elements and
 * returns NULL.
 */
ARRAY_ELEMENT **
ARRAY::shift (size_t n, int flags)
{
  ARRAY_ELEMENT **r, *ae;
  size_t ni, ri;
  arrayind_t i, j;

  if (empty () || n <= 0)
    return nullptr;

  r = new ARRAY_ELEMENT *[n + 1];

  /* Easy case; shifting out all of the elements */
  if (n >= num_elements)
    {
      if (flags & AS_DISPOSE)
        {
          flush ();
          return nullptr;
        }
      for (ri = 0, i = first_index_; i <= max_index; i++)
        if (elements[static_cast<size_t> (i)])
          {
            r[ri++] = elements[static_cast<size_t> (i)];
            elements[static_cast<size_t> (i)] = nullptr;
          }

      first_index_ = max_index = -1;
      num_elements = 0;
      r[ri] = nullptr;
      return r;
    }

  /* Shift out the first N elements, return them in R. Handle sparse
     arrays by skipping over NULL array elements. */
  for (i = first_index_, ri = 0, j = 0; j < static_cast<arrayind_t> (n); i++)
    {
      if ((ae = elements[static_cast<size_t> (i)]) == nullptr)
        continue;

      if (i > max_index)
        break;

      ni = static_cast<size_t> (i) + n;
      j++;

      if (ae)
        r[ri++] = elements[static_cast<size_t> (i)];

      elements[static_cast<size_t> (i)] = elements[ni];

      if (elements[static_cast<size_t> (i)])
        elements[static_cast<size_t> (i)]->ind = i;
    }

  r[ri] = nullptr;

#ifdef DEBUG
  if (j < static_cast<arrayind_t> (n))
    itrace ("array_shift: short count: j = %ld n = %ld", static_cast<long> (j),
            static_cast<long> (n));
#endif

  /* Now shift everything else, modifying the index in each element */
  for (; i <= max_index; i++)
    {
      ni = static_cast<size_t> (i) + n;
      elements[static_cast<size_t> (i)]
          = (ni <= static_cast<size_t> (max_index)) ? elements[ni] : nullptr;
      if (elements[static_cast<size_t> (i)])
        elements[static_cast<size_t> (i)]->ind = i;
    }

  num_elements -= n; /* modify bookkeeping information */
  if (num_elements == 0)
    first_index_ = max_index == -1;
  else
    {
      max_index -= n;
      for (i = 0; i <= max_index; i++)
        if (elements[static_cast<size_t> (i)])
          break;
      first_index_ = i;
    }

  if (flags & AS_DISPOSE)
    {
      for (i = 0; static_cast<size_t> (i) < ri; i++)
        delete r[static_cast<size_t> (i)];

      delete[] r;
      return nullptr;
    }

  return r;
}

/*
 * Shift array A right N indices.  If S is non-null, it becomes the value of
 * the new element 0.  Returns the number of elements in the array after the
 * shift.
 */
size_t
ARRAY::rshift (size_t n, string_view s)
{
  arrayind_t ni;

  if (empty () && s.empty ())
    return 0;

  else if (n <= 0)
    return num_elements;

  if (n >= elements.size ())
    expand (n);

  /* Shift right, adjusting the element indexes as we go */
  for (ni = max_index; ni >= 0; ni--)
    {
      elements[static_cast<size_t> (ni) + n]
          = elements[static_cast<size_t> (ni)];

      if (elements[static_cast<size_t> (ni) + n])
        elements[static_cast<size_t> (ni) + n]->ind
            = ni + static_cast<arrayind_t> (n);

      elements[static_cast<size_t> (ni)] = nullptr;
    }

  max_index += n;

#if 0
 /* Null out all the old indexes we just copied from */
 for (ni = first_index_; ni >= 0 && ni < n; ni++)
  elements[ni] = nullptr;
#endif

  first_index_ += n;

  if (!s.empty ())
    {
      ARRAY_ELEMENT *new_e = new ARRAY_ELEMENT (0, s);
      elements[0] = new_e;
      num_elements++;
      first_index_ = 0;

      if (num_elements == 1) /* array was empty */
        max_index = 0;
    }

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
  arrayind_t s, e;
  int i;
  WORD_LIST *wl;

  if (a->empty () || start > a->max_index)
    return nullptr;

  /*
   * Find element with index START.  If START corresponds to an unset
   * element (arrays can be sparse), use the first element whose index
   * is >= START.  If START is < 0, we count START indices back from
   * the end of A (not elements, even with sparse arrays -- START is an
   * index).
   */
  for (s = start;
       a->elements[static_cast<size_t> (s)] == nullptr && s <= a->max_index;
       s++)
    ;

  if (s > a->max_index)
    return nullptr;

  /* Starting at S, take NELEM elements, inclusive. */
  for (i = 0, e = s; e <= a->max_index && i < nelem; e++)
    {
      if (a->elements[static_cast<size_t> (e)]) /* arrays are sparse */
        i++;
    }

  a2 = a->slice (s, e);

  wl = array_to_word_list (a2);
  delete a2;

  if (wl == nullptr)
    return nullptr;

  std::string t (string_list_pos_params (starsub ? '*' : '@', wl, quoted,
                                         pflags)); /* XXX */
  delete wl;

  return t;
}

std::string
Shell::array_patsub (ARRAY *a, string_view pat, string_view rep,
                     match_flags mflags)
{
  WORD_LIST *wl, *save;

  if (a == nullptr || a->empty ())
    return nullptr;

  wl = array_to_word_list (a);
  if (wl == nullptr)
    return nullptr;

  for (save = wl; wl; wl = wl->next ())
    wl->word->word = pat_subst (wl->word->word, pat, rep, mflags);

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

  if (a == nullptr || a->empty ())
    return nullptr;

  wl = array_to_word_list (a);
  if (wl == nullptr)
    return nullptr;

  for (save = wl; wl; wl = wl->next ())
    wl->word->word = sh_modcase (wl->word->word, pat, modop);

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
  ARRAY_ELEMENT *old;

  if (static_cast<size_t> (i) >= elements.size ())
    expand (static_cast<size_t> (i));

  old = elements[static_cast<size_t> (i)];

  if (i > max_index)
    max_index = i;

  if (first_index_ < 0 || i < first_index_)
    first_index_ = i;

  if (old)
    { /* Replacing an existing element. */
      old->value = to_string (v);
      old->ind = i;
      return 0;
    }
  else
    {
      elements[static_cast<size_t> (i)] = new ARRAY_ELEMENT (i, v);
      num_elements++;
    }

  return -1; /* problem */
}

/*
 * Delete the element with index I from array A and return it so the
 * caller can dispose of it.
 */
ARRAY_ELEMENT *
ARRAY::remove (arrayind_t i)
{
  ARRAY_ELEMENT *ae;
  arrayind_t ind;

  if (empty ())
    return nullptr;

  if (i > max_index || i < first_index_)
    return nullptr;

  ae = elements[static_cast<size_t> (i)];
  elements[static_cast<size_t> (i)] = nullptr;

  if (ae)
    {
      num_elements--;
      if (num_elements == 0)
        first_index_ = max_index == -1;

      if (i == max_index)
        {
          for (ind = i; ind >= first_index_; ind--)
            if (elements[static_cast<size_t> (ind)])
              break;

          max_index = ind;
        }

      if (i == first_index_)
        {
          for (ind = i; ind <= max_index; ind++)
            if (elements[static_cast<size_t> (ind)])
              break;

          first_index_ = ind;
        }
    }

  return ae;
}

/*
 * Return the value of a[i].
 */
const std::string *
ARRAY::reference (arrayind_t i)
{
  ARRAY_ELEMENT *ae;

  if (empty ())
    return nullptr;

  if (i > max_index || i < first_index_)
    return nullptr;

  ae = elements[static_cast<size_t> (i)];

  return ae ? &(ae->value) : nullptr;
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

  for (size_t ind = static_cast<size_t> (a->first_index_);
       ind <= static_cast<size_t> (a->max_index); ind++)
    {
      if ((ae = a->elements[ind]) == nullptr)
        continue;

      list = new WORD_LIST (new WORD_DESC (ae->value), list);
    }

  return list->reverse ();
}

ARRAY *
array_from_word_list (WORD_LIST *list)
{
  ARRAY *a;

  if (list == nullptr)
    return nullptr;

  a = new ARRAY ();
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

  for (size_t ind = static_cast<size_t> (a->first_index_);
       ind <= static_cast<size_t> (a->max_index); ind++)
    {
      if ((ae = a->elements[ind]) == nullptr)
        continue;

      std::string t (itos (ae->ind));
      list = new WORD_LIST (new WORD_DESC (t), list);
    }

  return list->reverse ();
}

WORD_LIST *
array_to_kvpair_list (ARRAY *a)
{
  WORD_LIST *list;
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  list = nullptr;

  for (size_t ind = static_cast<size_t> (a->first_index_);
       ind <= static_cast<size_t> (a->max_index); ind++)
    {
      if ((ae = a->elements[ind]) == nullptr)
        continue;

      std::string k (itos (ae->ind));

      list = new WORD_LIST (new WORD_DESC (k), list);
      list = new WORD_LIST (new WORD_DESC (ae->value), list);
    }

  return list->reverse ();
}

ARRAY *
array_assign_list (ARRAY *array, WORD_LIST *list)
{
  WORD_LIST *l;
  size_t i;

  for (l = list, i = 0; l; l = l->next (), i++)
    array->insert (static_cast<arrayind_t> (i), l->word->word);

  return array;
}

char **
array_to_argv (ARRAY *a, int *countp)
{
  char **ret;

  if (a == nullptr || a->empty ())
    {
      if (countp)
        *countp = 0;

      return nullptr;
    }

  ret = new char *[static_cast<size_t> (a->num_elements + 1)];
  size_t i = 0;

  for (size_t ind = static_cast<size_t> (a->first_index_);
       ind <= static_cast<size_t> (a->max_index); ind++)
    {
      if (a->elements[ind])
        ret[i++] = savestring (a->elements[ind]->value);
    }

  ret[i] = nullptr;

  if (countp)
    *countp = static_cast<int> (i);

  return ret;
}

ARRAY *
array_from_argv (ARRAY *a, char **vec, int count)
{
  size_t i;

  if (a == nullptr || a->num_elements == 0)
    {
      for (i = 0; static_cast<int> (i) < count; i++)
        a->insert (static_cast<arrayind_t> (i), vec[i]);
      return a;
    }

  /* Fast case */
  if (static_cast<int> (a->num_elements) == count && count == 1)
    {
      if (vec[0])
        a->elements[0]->value = vec[0];
      else
        a->elements[0]->value.clear ();
    }
  else if (static_cast<int> (a->num_elements) <= count)
    {
      /* modify in array_num_elements members in place, then add */
      for (i = 0; i < a->num_elements; i++)
        {
          if (vec[i])
            a->elements[i]->value = vec[i];
          else
            a->elements[i]->value.clear ();
        }

      /* add any more */
      for (; static_cast<int> (i) < count; i++)
        a->insert (static_cast<arrayind_t> (i), vec[i]);
    }
  else
    {
      /* deleting elements. replace the first COUNT, free the rest */
      for (i = 0; static_cast<int> (i) < count; i++)
        {
          if (vec[i])
            a->elements[i]->value = vec[i];
          else
            a->elements[i]->value.clear ();
        }

      // We don't need to clear the old values due to the resize below.
      for (; i <= static_cast<size_t> (a->max_index); i++)
        delete a->elements[i];

      /* bookkeeping usually taken care of by array_insert */
      a->elements.resize (static_cast<size_t> (count));
      a->first_index_ = 0;
      a->num_elements = static_cast<size_t> (count);
    }
  return a;
}

std::string
array_to_kvpair (ARRAY *a, bool quoted)
{
  size_t ind;
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  std::string result;

  for (ind = static_cast<size_t> (a->first_index_);
       ind <= static_cast<size_t> (a->max_index); ind++)
    {
      if ((ae = a->elements[ind]) == nullptr)
        continue;

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

      if (ind < static_cast<size_t> (a->max_index))
        result.push_back (' ');
    }

  if (quoted)
    return sh_single_quote (result);

  return result;
}

std::string
array_to_assign (ARRAY *a, bool quoted)
{
  size_t ind;
  ARRAY_ELEMENT *ae;

  if (a == nullptr || a->empty ())
    return nullptr;

  std::string result;

  for (ind = static_cast<size_t> (a->first_index_);
       ind <= static_cast<size_t> (a->max_index); ind++)
    {
      if ((ae = a->elements[ind]) == nullptr)
        continue;

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

      if (ind < static_cast<size_t> (a->max_index))
        result.push_back (' ');
    }

  result.push_back (')');

  if (quoted)
    return sh_single_quote (result);

  return result;
}

#if defined(INCLUDE_UNUSED) || defined(TEST_ARRAY)
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
 *  xmalloc.o syntax.o lib/malloc/libmalloc.a lib/sh/libsh.a
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
  fprintf (stderr, "array_test: fatal memory error\n");
  abort ();
}

void
programming_error (const char *s, ...)
{
  fprintf (stderr, "array_test: fatal programming error\n");
  abort ();
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
  a = strtok (r, t);
  while (a)
    {
      wl = make_word_list (make_bare_word (a), wl);
      a = strtok (nullptr, t);
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
  printf ("array[%s] = %s\n", inttostr (ae->ind).c_str (), ae->value.c_str ());
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
  printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, " ");
  printf ("copy_of_a:");
  print_array (copy_of_a);
  array_dispose (copy_of_a);
  printf ("\n");
  free (s);
  ae = array_remove (a, 4);
  array_dispose_element (ae);
  ae = array_remove (a, 1029);
  array_dispose_element (ae);
  a->insert (16, "sixteen");
  print_array (a);
  s = array_to_string (a, " ", 0);
  printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, " ");
  printf ("copy_of_a:");
  print_array (copy_of_a);
  array_dispose (copy_of_a);
  printf ("\n");
  free (s);
  a->insert (2, "two");
  a->insert (1029, "new one thousand twenty-nine");
  a->insert (0, "zero");
  a->insert (134, "");
  print_array (a);
  s = array_to_string (a, ":", 0);
  printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, ":");
  printf ("copy_of_a:");
  print_array (copy_of_a);
  array_dispose (copy_of_a);
  printf ("\n");
  free (s);
  new_a = array_copy (a);
  print_array (new_a);
  s = array_to_string (new_a, ":", 0);
  printf ("s = %s\n", s);
  copy_of_a = array_from_string (s, ":");
  free (s);
  printf ("copy_of_a:");
  print_array (copy_of_a);
  array_shift (copy_of_a, 2, AS_DISPOSE);
  printf ("copy_of_a shifted by two:");
  print_array (copy_of_a);
  ae = array_shift (copy_of_a, 2, 0);
  printf ("copy_of_a shifted by two:");
  print_array (copy_of_a);
  for (; ae;)
    {
      aew = element_forw (ae);
      array_dispose_element (ae);
      ae = aew;
    }
  array_rshift (copy_of_a, 1, (char *)0);
  printf ("copy_of_a rshift by 1:");
  print_array (copy_of_a);
  array_rshift (copy_of_a, 2, "new element zero");
  printf ("copy_of_a rshift again by 2 with new element zero:");
  print_array (copy_of_a);
  s = array_to_assign (copy_of_a, 0);
  printf ("copy_of_a=%s\n", s);
  free (s);
  ae = array_shift (copy_of_a, array_num_elements (copy_of_a), 0);
  for (; ae;)
    {
      aew = element_forw (ae);
      array_dispose_element (ae);
      ae = aew;
    }
  array_dispose (copy_of_a);
  printf ("\n");
  array_dispose (a);
  array_dispose (new_a);
}

#endif /* TEST_ARRAY */

} // namespace bash

#endif /* ARRAY_VARS */
