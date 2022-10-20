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

#include <vector>

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
template <class T> class BUCKET_CONTENTS : public GENERIC_LIST
{
public:
  // Constructor to create an empty bucket with the specified key and
  // optional next element to link with.
  BUCKET_CONTENTS (const std::string &key_, uint32_t khash_, T data_ = nullptr,
                   BUCKET_CONTENTS next = nullptr)
      : GENERIC_LIST (next), key (key_), khash (khash_), data (data_)
  {
  }

  // Destructor deletes the entire linked list.
  ~BUCKET_CONTENTS () noexcept
  {
    delete next ();
    delete data;
  }

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
  T data;             // What we really want.
  size_t times_found; // Number of times this item has been found.
  uint32_t khash;     // What key hashes to
};

template <class T> class HASH_TABLE
{
public:
  // Make a new hash table with 'num_buckets' number of buckets. Initialize
  // each slot in the table to nullptr.
  HASH_TABLE (size_t num_buckets = DEFAULT_HASH_BUCKETS)
      : buckets (num_buckets, nullptr), nentries (0)
  {
  }

  // Destructor deletes all of the objects in the hash table.
  ~HASH_TABLE () noexcept { flush (); }

  // Clone the hash table.
  explicit HASH_TABLE (const HASH_TABLE<T> &);

  // Empty the hash table, deleting the pointed-to elements.
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

  void insert (string_view, T);

  BUCKET_CONTENTS<T> *search (string_view, hsearch_flags);

  T find (string_view);

  std::vector<BUCKET_CONTENTS<T> *> buckets; // Where the data is kept.
  size_t nentries; // How many entries this table has.

private:
  bool
  hash_shouldgrow ()
  {
    return nentries >= (buckets.size () * HASH_REHASH_FACTOR);
  }

  // an initial approximation
  bool
  hash_shouldshrink ()
  {
    return (buckets.size () > DEFAULT_HASH_BUCKETS)
           && (nentries < (buckets.size () / HASH_REHASH_MULTIPLIER));
  }

  /* This is the best 32-bit string hash function I found. It's one of the
     Fowler-Noll-Vo family (FNV-1).

     The magic is in the interesting relationship between the special prime
     16777619 (2^24 + 403) and 2^32 and 2^8. */

#define FNV_OFFSET 2166136261
#define FNV_PRIME 16777619

  /* If you want to use 64 bits, use
  FNV_OFFSET	14695981039346656037
  FNV_PRIME	1099511628211
  */

  /* The `khash' check below requires that strings that compare equally with
     strcmp hash to the same value. */
  static uint32_t
  hash_string (const std::string &key)
  {
    uint32_t i;

    std::string::const_iterator it;
    for (i = FNV_OFFSET, it = key.begin (); it != key.end (); ++it)
      {
        /* FNV-1a has the XOR first, traditional FNV-1 has the multiply first
         */

        /* was i *= FNV_PRIME */
        i += (i << 1) + (i << 4) + (i << 7) + (i << 8) + (i << 24);
        i ^= static_cast<uint32_t> (*it);
      }

    return i;
  }

  // Rely on properties of unsigned division (unsigned/int -> unsigned) and
  // don't discard the upper 32 bits of the value, if present.
  uint32_t
  hash_bucket (const std::string &key)
  {
    // This requires the bucket size to be a power of two.
    return hash_string (key) & (buckets.size () - 1);
  }
};

#if 0
/* Redefine the function as a macro for speed. */
// FIXME: make this a member function.
#define hash_items(bucket, table)                                             \
  ((table && (bucket < table->nbuckets)) ? table->bucket_array[bucket]        \
                                         : nullptr)

#define HASH_ENTRIES(ht) ((ht) ? (ht)->nentries : 0)

/* flags for hash_search and hash_insert */
#define HASH_NOSRCH 0x01
#define HASH_CREATE 0x02
#endif

} // namespace bash

#endif /* _HASHLIB_H */
