/* stringlist.cc - wrapper functions to handle a C++ vector of C strings */

/* Copyright (C) 2000-2019 Free Software Foundation, Inc.

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

#include "shell.hh"

namespace bash
{

/* Allocate and return a new copy of STRINGLIST and its contents. */
STRINGLIST *
strlist_copy (const STRINGLIST *sl)
{
  if (sl == nullptr)
    return nullptr;

  STRINGLIST *new_list = new STRINGLIST (sl->size ());

  STRINGLIST::const_iterator sit;
  STRINGLIST::iterator dit;

  for (sit = sl->begin (), dit = new_list->begin ();
       sit != sl->end () && dit != new_list->end (); ++sit, ++dit)
    *dit = ::strdup (*sit);

  return new_list;
}

/* Return a new STRINGLIST with everything from M1 and M2. */
STRINGLIST *
strlist_merge (const STRINGLIST *m1, const STRINGLIST *m2)
{
  size_t l1 = m1 ? m1->size () : 0;
  size_t l2 = m2 ? m2->size () : 0;

  STRINGLIST *sl = new STRINGLIST (l1 + l2);

  STRINGLIST::const_iterator sit;
  STRINGLIST::iterator dit;

  if (m1)
    for (sit = m1->begin (), dit = sl->begin ();
         sit != m1->end () && dit != sl->end (); ++sit, ++dit)
      *dit = ::strdup (*sit);

  if (m2)
    for (sit = m2->begin (), dit = sl->begin ();
         sit != m2->end () && dit != sl->end (); ++sit, ++dit)
      *dit = ::strdup (*sit);

  return sl;
}

/* Make STRINGLIST M1 contain everything in M1 and M2. */
void
strlist_append (STRINGLIST *m1, const STRINGLIST *m2)
{
  if (m1 == nullptr)
    return;

  size_t len2 = m2 ? m2->size () : 0;

  if (len2)
    {
      size_t len1 = m1->size ();
      m1->resize (len1 + len2);

      STRINGLIST::const_iterator sit;
      STRINGLIST::iterator dit;

      for (sit = m2->begin (),
          dit = (m1->begin () + static_cast<ssize_t> (len1));
           sit != m2->end (); ++sit, ++dit)
        *dit = ::strdup (*sit);
    }
}

void
strlist_prefix_suffix (STRINGLIST *sl, const char *prefix, const char *suffix)
{
  if (sl == nullptr || sl->size () == 0)
    return;

  size_t plen = std::strlen (prefix);
  size_t slen = std::strlen (suffix);

  if (plen == 0 && slen == 0)
    return;

  STRINGLIST::iterator it;
  for (it = sl->begin (); it != sl->end (); ++it)
    {
      size_t llen = std::strlen (*it);
      size_t tlen = plen + llen + slen;

      char *t = new char[tlen + 1];
      if (plen)
        strcpy (t, prefix);

      strcpy (t + plen, *it);

      if (slen)
        strcpy (t + plen + llen, suffix);

      t[tlen] = '\0';

      delete[](*it);
      *it = t;
    }
}

} // namespace bash
