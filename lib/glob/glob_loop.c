/* Copyright (C) 1991-2020 Free Software Foundation, Inc.

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

static bool INTERNAL_GLOB_PATTERN_P (const GCHAR *);

/* Return nonzero if PATTERN has any special globbing chars in it.
   Compiled twice, once each for single-byte and multibyte characters. */
static bool
INTERNAL_GLOB_PATTERN_P (const GCHAR *pattern)
{
  const GCHAR *p = pattern;
  bool bopen = false;
  GCHAR c;

  while ((c = *p++) != L('\0'))
    switch (c)
      {
      case L('?'):
      case L('*'):
	return true;

      case L('['):      /* Only accept an open brace if there is a close */
	bopen = true;   /* brace to match it.  Bracket expressions must be */
	continue;       /* complete, according to Posix.2 */
      case L(']'):
	if (bopen)
	  return true;
	continue;

      case L('+'):         /* extended matching operators */
      case L('@'):
      case L('!'):
	if (*p == L('('))  /*) */
	  return true;
	continue;

      case L('\\'):
	/* Don't let the pattern end in a backslash (GMATCH returns no match
	   if the pattern ends in a backslash anyway), but otherwise note that
	   we have seen this, since the matching engine uses backslash as an
	   escape character and it can be removed. We return 2 later if we
	   have seen only backslash-escaped characters, so interested callers
	   know they can shortcut and just dequote the pathname. */
	if (*p != L('\0'))
	  {
	    p++;
	    continue;
	  }
	else 	/* (*p == L('\0')) */
	  return false;
      }

  return false;
}

#undef INTERNAL_GLOB_PATTERN_P
#undef L
#undef INT
#undef CHAR
#undef GCHAR
