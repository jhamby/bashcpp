/* hashlib.hh -- the data structures used in hashing in Bash. */

/* Copyright (C) 1993-2020 Free Software Foundation, Inc.

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

#if !defined(_HASHLIB_H_)
#define _HASHLIB_H_

#include "general.hh"

namespace bash
{

/* Default number of buckets in the hash table. */
#define DEFAULT_HASH_BUCKETS 128 /* must be power of two */

/* tunable constants for rehashing */
#define HASH_REHASH_MULTIPLIER 4
#define HASH_REHASH_FACTOR 2

// flags for search () and insert ().
enum hsearch_flags
{
  HS_NOFLAGS = 0,
  HS_NOSRCH = 0x01,
  HS_CREATE = 0x02
};

// Key/value container for HASH_TABLE items. The destructor
// will delete all entries in the list.
template <typename T> class BUCKET_CONTENTS : public GENERIC_LIST
{
public:
  // Constructor to create an empty bucket with the specified key and
  // optional next element to link with.
  BUCKET_CONTENTS (string_view key_, uint32_t khash_, T *data_ = nullptr,
                   BUCKET_CONTENTS<T> *next = nullptr)
      : GENERIC_LIST (next), key (to_string (key_)), data (data_),
        khash (khash_)
  {
  }

  // Destructor deletes the entire linked list.
  ~BUCKET_CONTENTS () noexcept
  {
    delete next ();
    delete data;
  }

  // Copy the entire list.
  explicit BUCKET_CONTENTS (const BUCKET_CONTENTS<T> &);

  BUCKET_CONTENTS *
  append (BUCKET_CONTENTS *new_item)
  {
    append_ (new_item);
    return this;
  }

  BUCKET_CONTENTS *
  next ()
  {
    return static_cast<BUCKET_CONTENTS *> (next_);
  }

  std::string key;    // What we look up.
  T *data;            // Pointer to what we really want.
  size_t times_found; // Number of times this item has been found.
  uint32_t khash;     // What key hashes to
};

class Shell;

// Hash table holding linked lists of pointers to type T.
template <typename T> class HASH_TABLE
{
public:
  // Make a new hash table with 'num_buckets' number of buckets. Initialize
  // each slot in the table to nullptr.
  HASH_TABLE (size_t num_buckets = DEFAULT_HASH_BUCKETS)
      : buckets (num_buckets, nullptr), nentries (0)
  {
  }

  // The destructor is equivalent to the old hash_dispose ().
  ~HASH_TABLE () noexcept {}

  // Clone the hash table, including all of the buckets.
  explicit HASH_TABLE (const HASH_TABLE<T> &);

  /* Remove and discard all entries in TABLE. If FREE_DATA is non-null, it
     is a function to call to dispose of a hash item's data. Otherwise,
     delete is called. */
  void
  flush () noexcept
  {
    typename std::vector<BUCKET_CONTENTS<T> *>::iterator it;
    for (it = buckets.begin (); it != buckets.end (); ++it)
      if (*it)
        {
          delete *it;
          *it = nullptr;
        }
  }

  size_t
  size () noexcept
  {
    return nentries;
  }

  size_t
  num_buckets () noexcept
  {
    return buckets.size ();
  }

  BUCKET_CONTENTS<T> *insert (string_view, hsearch_flags);

  BUCKET_CONTENTS<T> *search (string_view, hsearch_flags);

  T *find (string_view);

  BUCKET_CONTENTS<T> *remove (string_view);

  void rehash (size_t);

  void
  grow ()
  {
    size_t new_size = num_buckets () * HASH_REHASH_MULTIPLIER;
    // Test for both integer wraparound and uint32_t overflow
    if (new_size > num_buckets () && new_size < UINT32_MAX)
      rehash (new_size);
  }

  void
  shrink ()
  {
    int new_size = num_buckets () / HASH_REHASH_MULTIPLIER;
    if (new_size)
      rehash (new_size);
  }

  typedef int (Shell::*hash_wfunc) (BUCKET_CONTENTS<T> *);

  void
  walk (hash_wfunc &func)
  {
    typename std::vector<BUCKET_CONTENTS<T> *>::iterator it;
    for (it = buckets.begin (); it != buckets.end (); ++it)
      {
        BUCKET_CONTENTS<T> *bucket;
        for (bucket = *it; bucket; bucket = bucket->next ())
          {
            if (((*this).*func) (bucket) < 0)
              return;
          }
      }
  }

  std::vector<BUCKET_CONTENTS<T> *> buckets; // Where the data is kept.
  size_t nentries; // How many entries this table has.

private:
  bool
  should_grow ()
  {
    return nentries >= (num_buckets () * HASH_REHASH_FACTOR);
  }

  // an initial approximation
  bool
  should_shrink ()
  {
    return (num_buckets () > DEFAULT_HASH_BUCKETS)
           && (nentries < (num_buckets () / HASH_REHASH_MULTIPLIER));
  }

  /* This is the best 32-bit string hash function I found. It's one of the
     Fowler-Noll-Vo family (FNV-1).

     The magic is in the interesting relationship between the special prime
     16777619 (2^24 + 403) and 2^32 and 2^8. */

#define FNV_OFFSET 2166136261
#define FNV_PRIME 16777619

  // If you want to use 64 bits, use:
  // FNV_OFFSET 14695981039346656037
  // FNV_PRIME  1099511628211

  /* The `khash' equality check requires that strings that compare equally with
     strcmp hash to the same value. */
  static CONSTEXPR uint32_t
  hash_value (string_view key)
  {
    // Initialize the variables to make C++17 happy.
    uint32_t i = FNV_OFFSET;
    string_view::iterator it = key.begin ();

    for (; it != key.end (); ++it)
      {
        // FNV-1a has the XOR first, traditional FNV-1 has the multiply first.
        i *= FNV_PRIME;
        i ^= static_cast<uint32_t> (*it);
      }

    return i;
  }

  /* Return the location of the bucket which should contain the data
     for the specified hash value. TABLE is a pointer to a HASH_TABLE. */
  uint32_t
  hash_bucket (uint32_t hv)
  {
    // This requires the bucket size to be a power of two.
    return hv & (buckets.size () - 1);
  }
};

// Recursively copy all the list elements.
template <typename T>
BUCKET_CONTENTS<T>::BUCKET_CONTENTS (const BUCKET_CONTENTS<T> &o)
    : GENERIC_LIST (), key (o.key), data (new T (o.data)),
      times_found (o.times_found), khash (o.khash)
{
  if (o.next_)
    set_next (new T (*o.next ()));
}

template <typename T>
void
HASH_TABLE<T>::rehash (size_t new_size)
{
  BUCKET_CONTENTS<T> *all_buckets = nullptr;

  typename std::vector<BUCKET_CONTENTS<T> *>::iterator it;
  for (it = buckets.begin (); it != buckets.end (); ++it)
    {
      // remove all elements and add them to all_buckets.
      if (*it)
        {
          BUCKET_CONTENTS<T> *bucket = *it;
          // *it = nullptr; // we'll clear the entire list later
          while (bucket)
            {
              BUCKET_CONTENTS<T> *next = bucket->next ();
              bucket->set_next (all_buckets);
              all_buckets = bucket;
              bucket = next;
            }
        }
    }

  buckets.clear (); // we don't need to preserve the old values
  buckets.resize (new_size, nullptr);

  // Add all of the buckets at their new indices.
  while (all_buckets)
    {
      BUCKET_CONTENTS<T> *bucket = all_buckets;
      all_buckets = bucket->next ();

      size_t i = bucket->khash & (new_size - 1);
      bucket->set_next (buckets[i]);
      buckets[i] = bucket;
    }
}

// Recursively copy all of the buckets.
template <typename T>
HASH_TABLE<T>::HASH_TABLE (const HASH_TABLE<T> &o)
    : buckets (o.buckets.size (), nullptr), nentries (o.nentries)
{
  typename std::vector<BUCKET_CONTENTS<T> >::iterator sit, dit;
  for (sit = o.buckets.begin (), dit = buckets.begin ();
       sit != o.buckets.end (); ++sit, ++dit)
    {
      if (*sit)
        *dit = new BUCKET_CONTENTS<T> (*sit);
    }
}

/* Return a pointer to the hashed item. If the HASH_CREATE flag is passed,
   create a new hash table entry for STRING, otherwise return nullptr. */
template <typename T>
BUCKET_CONTENTS<T> *
HASH_TABLE<T>::search (string_view string, hsearch_flags flags)
{
  uint32_t hv = hash_value (string);
  uint32_t index = hash_bucket (hv);

  if ((flags & HS_CREATE) == 0 && !buckets[index])
    return nullptr;

  BUCKET_CONTENTS<T> *bucket;
  for (bucket = buckets[index]; bucket; bucket = bucket->next ())
    {
      /* Compare hashes first, then the string itself. */
      if (hv == bucket->khash && bucket->key == string)
        {
          bucket->times_found++;
          return bucket;
        }
    }

  if (flags & HS_CREATE)
    {
      if (should_grow ())
        {
          grow ();
          index = hash_bucket (hv);
        }

      bucket = new BUCKET_CONTENTS<T> (string, hv, nullptr, buckets[index]);
      buckets[index] = bucket;
      nentries++;
      return bucket;
    }

  return nullptr;
}

/* Remove the item specified by STRING from the hash table TABLE.
   The item removed is returned, so you can free its contents. If
   the item isn't in this table, nullptr is returned. */
template <typename T>
BUCKET_CONTENTS<T> *
HASH_TABLE<T>::remove (string_view string)
{
  uint32_t hv = hash_value (string);
  uint32_t index = hash_bucket (hv);

  if (!buckets[index])
    return nullptr;

  BUCKET_CONTENTS<T> *bucket, *prev;
  prev = nullptr;
  for (bucket = buckets[index]; bucket; bucket = bucket->next ())
    {
      /* Compare hashes first, then the string itself. */
      if (hv == bucket->khash && bucket->key == string)
        {
          if (prev)
            prev->set_next (bucket->next ());
          else
            buckets[index] = bucket->next ();

          bucket->set_next (nullptr);
          nentries--;
          return bucket;
        }
      prev = bucket;
    }

  return nullptr;
}

/* Create an entry for STRING, in TABLE. If the entry already
   exists, then return it (unless the HS_NOSRCH flag is set). */
template <typename T>
BUCKET_CONTENTS<T> *
HASH_TABLE<T>::insert (string_view string, hsearch_flags flags)
{
  uint32_t hv = hash_value (string);
  uint32_t index = hash_bucket (hv);

  BUCKET_CONTENTS<T> *bucket
      = (flags & HS_NOSRCH) ? nullptr : search (string, HS_NOFLAGS);

  if (!bucket)
    {
      if (should_grow ())
        {
          grow ();
          index = hash_bucket (hv);
        }

      bucket = new BUCKET_CONTENTS<T> (string, hv, nullptr, buckets[index]);
      buckets[index] = bucket;
      nentries++;
    }

  return bucket;
}

/* Redefine the function as a macro for speed. */
#define hash_items(bucket, table)                                             \
  ((bucket < table.size ()) ? table.buckets[bucket] : nullptr)

} // namespace bash

#endif /* _HASHLIB_H */
