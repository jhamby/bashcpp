// This file is echo_def.cc
// It implements the builtin "echo" in Bash.

// Copyright (C) 1987-2018 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "shell.hh"

#include "common.hh"

// $BUILTIN echo
// $FUNCTION echo_builtin
// $DEPENDS_ON V9_ECHO
// $SHORT_DOC echo [-neE] [arg ...]
// Write arguments to the standard output.

// Display the ARGs, separated by a single space character and followed by a
// newline, on the standard output.

// Options:
//   -n	do not append a newline
//   -e	enable interpretation of the following backslash escapes
//   -E	explicitly suppress interpretation of backslash escapes

// `echo' interprets the following backslash-escaped characters:
//   \a	alert (bell)
//   \b	backspace
//   \c	suppress further output
//   \e	escape character
//   \E	escape character
//   \f	form feed
//   \n	new line
//   \r	carriage return
//   \t	horizontal tab
//   \v	vertical tab
//   \\	backslash
//   \0nnn	the character whose ASCII code is NNN (octal).  NNN can be
// 		0 to 3 octal digits
//   \xHH	the eight-bit character whose value is HH (hexadecimal).  HH
// 		can be one or two hex digits
//   \uHHHH	the Unicode character whose value is the hexadecimal value
//   HHHH.
// 		HHHH can be one to four hex digits.
//   \UHHHHHHHH the Unicode character whose value is the hexadecimal value
// 		HHHHHHHH. HHHHHHHH can be one to eight hex digits.

// Exit Status:
// Returns success unless a write error occurs.
// $END

// $BUILTIN echo
// $FUNCTION echo_builtin
// $DEPENDS_ON !V9_ECHO
// $SHORT_DOC echo [-n] [arg ...]
// Write arguments to the standard output.

// Display the ARGs on the standard output followed by a newline.

// Options:
//   -n	do not append a newline

// Exit Status:
// Returns success unless a write error occurs.
// $END

namespace bash
{

#if defined(V9_ECHO)
#define VALID_ECHO_OPTIONS "neE"
#else /* !V9_ECHO */
#define VALID_ECHO_OPTIONS "n"
#endif /* !V9_ECHO */

/* System V machines already have a /bin/sh with a v9 behaviour.  We
   give Bash the identical behaviour for these machines so that the
   existing system shells won't barf.  Regrettably, the SUS v2 has
   standardized the Sys V echo behavior.  This variable is external
   so that we can have a `shopt' variable to control it at runtime. */
#if defined(DEFAULT_ECHO_TO_XPG) || defined(STRICT_POSIX)
int xpg_echo = 1;
#else
int xpg_echo = 0;
#endif /* DEFAULT_ECHO_TO_XPG */

/* Print the words in LIST to standard output.  If the first word is
   `-n', then don't print a trailing newline.  We also support the
   echo syntax from Version 9 Unix systems. */
int
Shell::echo_builtin (WORD_LIST *list)
{
//   int display_return, do_v9, i, len;
  size_t i;
  const char *temp, *s;

  bool do_v9 = xpg_echo;
  bool display_return = true;

  if (posixly_correct && xpg_echo)
    goto just_echo;

  for (; list && (temp = list->word->word.c_str ()) && *temp == '-';
       list = list->next ())
    {
      /* If it appears that we are handling options, then make sure that
         all of the options specified are actually valid.  Otherwise, the
         string should just be echoed. */
      temp++;

      for (i = 0; temp[i]; i++)
        {
          if (strchr (VALID_ECHO_OPTIONS, temp[i]) == 0)
            break;
        }

      /* echo - and echo -<nonopt> both mean to just echo the arguments. */
      if (*temp == 0 || temp[i])
        break;

      /* All of the options in TEMP are valid options to ECHO.
         Handle them. */
      while ((i = *temp++))
        {
          switch (i)
            {
            case 'n':
              display_return = 0;
              break;
#if defined(V9_ECHO)
            case 'e':
              do_v9 = 1;
              break;
            case 'E':
              do_v9 = 0;
              break;
#endif /* V9_ECHO */
            default:
              goto just_echo; /* XXX */
            }
        }
    }

just_echo:

  clearerr (stdout); /* clear error before writing and testing success */

  while (list)
    {
      i = 0;
      temp = do_v9 ? ansicstr (list->word->word, STRLEN (list->word->word), 1,
                               &i, &len)
                   : list->word->word;
      if (temp)
        {
          if (do_v9)
            {
              for (s = temp; len > 0; len--)
                putchar (*s++);
            }
          else
            printf ("%s", temp);
        }

      QUIT;
      list = list->next ();
      if (i)
        {
          display_return = false;
          break;
        }
      if (list)
        putchar (' ');
      QUIT;
    }

  if (display_return)
    putchar ('\n');

  return sh_chkwrite (EXECUTION_SUCCESS);
}

} // namespace bash
