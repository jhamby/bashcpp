/* hashlib.c -- functions to manage and access hash tables for bash. */

/* Copyright (C) 1987,1989,1991,1995,1998,2001,2003,2005,2006,2008,2009 Free
   Software Foundation, Inc.

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

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "hashlib.hh"
#include "shell.hh"

namespace bash
{

static BUCKET_CONTENTS *copy_bucket_array (BUCKET_CONTENTS *,
                                           sh_string_func_t *);

static void hash_rehash (HASH_TABLE *, int);
static void hash_grow (HASH_TABLE *);
static void hash_shrink (HASH_TABLE *);

int
hash_size (HASH_TABLE *table)
{
  return HASH_ENTRIES (table);
}

static BUCKET_CONTENTS *
copy_bucket_array (BUCKET_CONTENTS *ba,
                   sh_string_func_t *cpdata) /* data copy function */
{
  BUCKET_CONTENTS *new_bucket, *n, *e;

  if (ba == 0)
    return (BUCKET_CONTENTS *)0;

  for (n = (BUCKET_CONTENTS *)0, e = ba; e; e = e->next)
    {
      if (n == 0)
        {
          new_bucket = (BUCKET_CONTENTS *)xmalloc (sizeof (BUCKET_CONTENTS));
          n = new_bucket;
        }
      else
        {
          n->next = (BUCKET_CONTENTS *)xmalloc (sizeof (BUCKET_CONTENTS));
          n = n->next;
        }

      n->key = savestring (e->key);
      n->data = e->data ? (cpdata ? (*cpdata) ((char *)(e->data))
                                  : savestring ((char *)(e->data)))
                        : NULL;
      n->khash = e->khash;
      n->times_found = e->times_found;
      n->next = (BUCKET_CONTENTS *)NULL;
    }

  return new_bucket;
}

static void
hash_rehash (HASH_TABLE *table, int nsize)
{
  int osize, i, j;
  BUCKET_CONTENTS **old_bucket_array, *item, *next;

  if (table == NULL || nsize == table->nbuckets)
    return;

  osize = table->nbuckets;
  old_bucket_array = table->bucket_array;

  table->nbuckets = nsize;
  table->bucket_array = (BUCKET_CONTENTS **)xmalloc (
      table->nbuckets * sizeof (BUCKET_CONTENTS *));
  for (i = 0; i < table->nbuckets; i++)
    table->bucket_array[i] = (BUCKET_CONTENTS *)NULL;

  for (j = 0; j < osize; j++)
    {
      for (item = old_bucket_array[j]; item; item = next)
        {
          next = item->next;
          i = item->khash & (table->nbuckets - 1);
          item->next = table->bucket_array[i];
          table->bucket_array[i] = item;
        }
    }

  free (old_bucket_array);
}

static void
hash_grow (HASH_TABLE *table)
{
  int nsize;

  nsize = table->nbuckets * HASH_REHASH_MULTIPLIER;
  if (nsize > 0) /* overflow */
    hash_rehash (table, nsize);
}

static void
hash_shrink (HASH_TABLE *table)
{
  int nsize;

  nsize = table->nbuckets / HASH_REHASH_MULTIPLIER;
  hash_rehash (table, nsize);
}

HASH_TABLE *
hash_copy (HASH_TABLE *table, sh_string_func_t *cpdata)
{
  HASH_TABLE *new_table;
  int i;

  if (table == 0)
    return (HASH_TABLE *)NULL;

  new_table = hash_create (table->nbuckets);

  for (i = 0; i < table->nbuckets; i++)
    new_table->bucket_array[i]
        = copy_bucket_array (table->bucket_array[i], cpdata);

  new_table->nentries = table->nentries;
  return new_table;
}

/* Return the location of the bucket which should contain the data
   for STRING.  TABLE is a pointer to a HASH_TABLE. */

int
hash_bucket (const char *string, HASH_TABLE *table)
{
  unsigned int h;

  return HASH_BUCKET (string, table, h);
}

/* Return a pointer to the hashed item.  If the HASH_CREATE flag is passed,
   create a new hash table entry for STRING, otherwise return NULL. */
BUCKET_CONTENTS *
hash_search (const char *string, HASH_TABLE *table, int flags)
{
  BUCKET_CONTENTS *list;
  int bucket;
  unsigned int hv;

  if (table == 0 || ((flags & HASH_CREATE) == 0 && HASH_ENTRIES (table) == 0))
    return (BUCKET_CONTENTS *)NULL;

  bucket = HASH_BUCKET (string, table, hv);

  for (list = table->bucket_array ? table->bucket_array[bucket] : 0; list;
       list = list->next)
    {
      /* This is the comparison function */
      if (hv == list->khash && STREQ (list->key, string))
        {
          list->times_found++;
          return list;
        }
    }

  if (flags & HASH_CREATE)
    {
      if (HASH_SHOULDGROW (table))
        {
          hash_grow (table);
          bucket = HASH_BUCKET (string, table, hv);
        }

      list = (BUCKET_CONTENTS *)xmalloc (sizeof (BUCKET_CONTENTS));
      list->next = table->bucket_array[bucket];
      table->bucket_array[bucket] = list;

      list->data = NULL;
      list->key = (char *)string; /* XXX fix later */
      list->khash = hv;
      list->times_found = 0;

      table->nentries++;
      return list;
    }

  return (BUCKET_CONTENTS *)NULL;
}

/* Remove the item specified by STRING from the hash table TABLE.
   The item removed is returned, so you can free its contents.  If
   the item isn't in this table NULL is returned. */
BUCKET_CONTENTS *
hash_remove (const char *string, HASH_TABLE *table, int flags)
{
  int bucket;
  BUCKET_CONTENTS *prev, *temp;
  unsigned int hv;

  if (table == 0 || HASH_ENTRIES (table) == 0)
    return (BUCKET_CONTENTS *)NULL;

  bucket = HASH_BUCKET (string, table, hv);
  prev = (BUCKET_CONTENTS *)NULL;
  for (temp = table->bucket_array[bucket]; temp; temp = temp->next)
    {
      if (hv == temp->khash && STREQ (temp->key, string))
        {
          if (prev)
            prev->next = temp->next;
          else
            table->bucket_array[bucket] = temp->next;

          table->nentries--;
          return temp;
        }
      prev = temp;
    }
  return (BUCKET_CONTENTS *)NULL;
}

/* Create an entry for STRING, in TABLE.  If the entry already
   exists, then return it (unless the HASH_NOSRCH flag is set). */
BUCKET_CONTENTS *
hash_insert (char *string, HASH_TABLE *table, int flags)
{
  BUCKET_CONTENTS *item;
  int bucket;
  unsigned int hv;

  if (table == 0)
    table = hash_create (0);

  item = (flags & HASH_NOSRCH) ? (BUCKET_CONTENTS *)NULL
                               : hash_search (string, table, 0);

  if (item == 0)
    {
      if (HASH_SHOULDGROW (table))
        hash_grow (table);

      bucket = HASH_BUCKET (string, table, hv);

      item = (BUCKET_CONTENTS *)xmalloc (sizeof (BUCKET_CONTENTS));
      item->next = table->bucket_array[bucket];
      table->bucket_array[bucket] = item;

      item->data = NULL;
      item->key = string;
      item->khash = hv;
      item->times_found = 0;

      table->nentries++;
    }

  return item;
}

/* Remove and discard all entries in TABLE.  If FREE_DATA is non-null, it
   is a function to call to dispose of a hash item's data.  Otherwise,
   free() is called. */
void
hash_flush (HASH_TABLE *table, sh_free_func_t *free_data)
{
  int i;
  BUCKET_CONTENTS *bucket, *item;

  if (table == 0 || HASH_ENTRIES (table) == 0)
    return;

  for (i = 0; i < table->nbuckets; i++)
    {
      bucket = table->bucket_array[i];

      while (bucket)
        {
          item = bucket;
          bucket = bucket->next;

          if (free_data)
            (*free_data) (item->data);
          else
            free (item->data);
          free (item->key);
          free (item);
        }
      table->bucket_array[i] = (BUCKET_CONTENTS *)NULL;
    }

  table->nentries = 0;
}

/* Free the hash table pointed to by TABLE. */
void
hash_dispose (HASH_TABLE *table)
{
  free (table->bucket_array);
  free (table);
}

void
hash_walk (HASH_TABLE *table, hash_wfunc *func)
{
  int i;
  BUCKET_CONTENTS *item;

  if (table == 0 || HASH_ENTRIES (table) == 0)
    return;

  for (i = 0; i < table->nbuckets; i++)
    {
      for (item = hash_items (i, table); item; item = item->next)
        if ((*func) (item) < 0)
          return;
    }
}

#if defined(DEBUG) || defined(TEST_HASHING)
void
hash_pstats (HASH_TABLE *table, char *name)
{
  int slot, bcount;
  BUCKET_CONTENTS *bc;

  if (name == 0)
    name = "unknown hash table";

  fprintf (stderr, "%s: %d buckets; %d items\n", name, table->nbuckets,
           table->nentries);

  /* Print out a count of how many strings hashed to each bucket, so we can
     see how even the distribution is. */
  for (slot = 0; slot < table->nbuckets; slot++)
    {
      bc = hash_items (slot, table);

      fprintf (stderr, "\tslot %3d: ", slot);
      for (bcount = 0; bc; bc = bc->next)
        bcount++;

      fprintf (stderr, "%d\n", bcount);
    }
}
#endif

#ifdef TEST_HASHING

/* link with xmalloc.o and lib/malloc/libmalloc.a */
#undef NULL
#include <stdio.h>

#ifndef NULL
#define NULL 0
#endif

HASH_TABLE *table, *ntable;

int interrupt_immediately = 0;
int running_trap = 0;

int
signal_is_trapped (int s)
{
  return 0;
}

void
programming_error (const char *format, ...)
{
  abort ();
}

void
fatal_error (const char *format, ...)
{
  abort ();
}

void
internal_warning (const char *format, ...)
{
}

int
main ()
{
  char string[256];
  int count = 0;
  BUCKET_CONTENTS *tt;

#if defined(TEST_NBUCKETS)
  table = hash_create (TEST_NBUCKETS);
#else
  table = hash_create (0);
#endif

  for (;;)
    {
      char *temp_string;
      if (fgets (string, sizeof (string), stdin) == 0)
        break;
      if (!*string)
        break;
      temp_string = savestring (string);
      tt = hash_insert (temp_string, table, 0);
      if (tt->times_found)
        {
          fprintf (stderr, "You have already added item `%s'\n", string);
          free (temp_string);
        }
      else
        {
          count++;
        }
    }

  hash_pstats (table, "hash test");

  ntable = hash_copy (table, (sh_string_func_t *)NULL);
  hash_flush (table, (sh_free_func_t *)NULL);
  hash_pstats (ntable, "hash copy test");

  exit (0);
}

#endif /* TEST_HASHING */

} // namespace bash