// This file is help_def.cc.
// It implements the builtin "help" in Bash.

// Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

// $BUILTIN help
// $FUNCTION help_builtin
// $DEPENDS_ON HELP_BUILTIN
// $SHORT_DOC help [-dms] [pattern ...]
// Display information about builtin commands.

// Displays brief summaries of builtin commands.  If PATTERN is
// specified, gives detailed help on all commands matching PATTERN,
// otherwise the list of help topics is printed.

// Options:
//   -d	output short description for each topic
//   -m	display usage in pseudo-manpage format
//   -s	output only a short usage synopsis for each topic matching
// 		PATTERN

// Arguments:
//   PATTERN	Pattern specifying a help topic

// Exit Status:
// Returns success unless PATTERN is not found or an invalid option is given.
// $END

#include "config.h"

#if defined(HELP_BUILTIN)

#include <unistd.h>

#include "bashintl.hh"

#include "builtins.hh"
#include "common.hh"
#include "pathexp.hh"
#include "shell.hh"

#include <fnmatch.h>
#include <glob.h>

namespace bash
{

extern const char *const bash_copyright;
extern const char *const bash_license;

#if 0
static void show_builtin_command_help (void);
static int open_helpfile (const char *);
static void show_desc (const char *, int);
static void show_manpage (const char *, int);
static void show_longdoc (int);
#endif

/* Print out a list of the known functions in the shell, and what they do.
   If LIST is supplied, print out the list which matches for each pattern
   specified. */
int
Shell::help_builtin (WORD_LIST *list)
{
  int i;
  const char *pattern, *name;
  int plen, match_found, sflag, dflag, mflag, m, pass, this_found;

  dflag = sflag = mflag = 0;
  reset_internal_getopt ();
  while ((i = internal_getopt (list, "dms")) != -1)
    {
      switch (i)
        {
        case 'd':
          dflag = 1;
          break;
        case 'm':
          mflag = 1;
          break;
        case 's':
          sflag = 1;
          break;
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }
  list = loptend;

  if (list == 0)
    {
      show_shell_version (0);
      show_builtin_command_help ();
      return EXECUTION_SUCCESS;
    }

  /* We should consider making `help bash' do something. */

  // XXX: this has to be updated to use Gnulib's 2-arg glob_pattern_p ().
  if (glob_pattern_p (list->word->word.c_str ()) == 1)
    {
      printf ("%s", ngettext ("Shell commands matching keyword `",
                              "Shell commands matching keywords `",
                              (list->next () ? 2 : 1)));
      print_word_list (list, ", ");
      printf ("%s", _ ("'\n\n"));
    }

  for (match_found = 0, pattern = ""; list; list = list->next ())
    {
      pattern = list->word->word;
      plen = strlen (pattern);

      for (pass = 1, this_found = 0; pass < 3; pass++)
        {
          for (i = 0; (name = shell_builtins[i].name); i++)
            {
              QUIT;

              /* First pass: look for exact string or pattern matches.
                 Second pass: look for prefix matches like bash-4.2 */
              if (pass == 1)
                m = (strcmp (pattern, name) == 0)
                    || (strmatch (pattern, name, FNMATCH_EXTFLAG)
                        != FNM_NOMATCH);
              else
                m = strncmp (pattern, name, plen) == 0;

              if (m)
                {
                  this_found = 1;
                  match_found++;
                  if (dflag)
                    {
                      show_desc (name, i);
                      continue;
                    }
                  else if (mflag)
                    {
                      show_manpage (name, i);
                      continue;
                    }

                  printf ("%s: %s\n", name, _ (shell_builtins[i].short_doc));

                  if (sflag == 0)
                    show_longdoc (i);
                }
            }
          if (pass == 1 && this_found == 1)
            break;
        }
    }

  if (match_found == 0)
    {
      builtin_error (_ ("no help topics match `%s'.  Try `help help' or `man "
                        "-k %s' or `info %s'."),
                     pattern, pattern, pattern);
      return EXECUTION_FAILURE;
    }

  fflush (stdout);
  return EXECUTION_SUCCESS;
}

void
builtin_help ()
{
  int ind;
  ptrdiff_t d;

  current_builtin = builtin_address_internal (this_command_name, 0);
  if (current_builtin == 0)
    return;

  d = current_builtin - shell_builtins;

  ind = (int)d;

  printf ("%s: %s\n", this_command_name, _ (shell_builtins[ind].short_doc));
  show_longdoc (ind);
}

static int
open_helpfile (const char *name)
{
  int fd;

  fd = open (name, O_RDONLY);
  if (fd == -1)
    {
      builtin_error (_ ("%s: cannot open: %s"), name, strerror (errno));
      return -1;
    }
  return fd;
}

/* By convention, enforced by mkbuiltins.c, if separate help files are being
   used, the long_doc array contains one string -- the full pathname of the
   help file for this builtin.  */
static void
show_longdoc (int i)
{
  char **doc = (char **)shell_builtins[i].long_doc;

  if (doc && doc[0] && *doc[0] == '/' && doc[1] == (char *)NULL)
    {
      int fd = open_helpfile (doc[0]);
      if (fd < 0)
        return;
      zcatfd (fd, 1, doc[0]);
      close (fd);
    }
  else if (doc)
    for (int j = 0; doc[j]; j++)
      printf ("%*s%s\n", BASE_INDENT, " ", _ (doc[j]));
}

static void
show_desc (const char *name, int i)
{
  const char *const *doc;
  char *line;
  int fd, usefile;

  doc = shell_builtins[i].long_doc;

  usefile = (doc && doc[0] && *doc[0] == '/' && doc[1] == (char *)NULL);
  if (usefile)
    {
      fd = open_helpfile (doc[0]);
      if (fd < 0)
        return;
      int r = zmapfd (fd, &line, doc[0]);
      close (fd);
      /* XXX - handle errors if zmapfd returns < 0 */
    }
  else
    line = (char *)(doc ? doc[0] : NULL);

  printf ("%s - ", name);
  for (int j = 0; line && line[j]; j++)
    {
      putchar (line[j]);
      if (line[j] == '\n')
        break;
    }

  fflush (stdout);

  if (usefile)
    free (line);
}

/* Print builtin help in pseudo-manpage format. */
static void
show_manpage (const char *name, int i)
{
  char **doc, *line;
  int fd, usefile;

  doc = (char **)shell_builtins[i].long_doc;

  usefile = (doc && doc[0] && *doc[0] == '/' && doc[1] == (char *)NULL);
  if (usefile)
    {
      fd = open_helpfile (doc[0]);
      if (fd < 0)
        return;
      zmapfd (fd, &line, doc[0]);
      close (fd);
    }
  else
    line = (char *)(doc ? _ (doc[0]) : NULL);

  /* NAME */
  printf ("NAME\n");
  printf ("%*s%s - ", BASE_INDENT, " ", name);
  for (int j = 0; line && line[j]; j++)
    {
      putchar (line[j]);
      if (line[j] == '\n')
        break;
    }
  printf ("\n");

  /* SYNOPSIS */
  printf ("SYNOPSIS\n");
  printf ("%*s%s\n\n", BASE_INDENT, " ", _ (shell_builtins[i].short_doc));

  /* DESCRIPTION */
  printf ("DESCRIPTION\n");
  if (usefile == 0)
    {
      for (int j = 0; doc[j]; j++)
        printf ("%*s%s\n", BASE_INDENT, " ", _ (doc[j]));
    }
  else
    {
      for (int j = 0; line && line[j]; j++)
        {
          putchar (line[j]);
          if (line[j] == '\n')
            printf ("%*s", BASE_INDENT, " ");
        }
    }
  putchar ('\n');

  /* SEE ALSO */
  printf ("SEE ALSO\n");
  printf ("%*sbash(1)\n\n", BASE_INDENT, " ");

  /* IMPLEMENTATION */
  printf ("IMPLEMENTATION\n");
  printf ("%*s", BASE_INDENT, " ");
  show_shell_version (0);
  printf ("%*s", BASE_INDENT, " ");
  printf ("%s\n", _ (bash_copyright));
  printf ("%*s", BASE_INDENT, " ");
  printf ("%s\n", _ (bash_license));

  fflush (stdout);
  if (usefile)
    free (line);
}

static void
dispcolumn (int i, char *buf, size_t bufsize, int width, int height)
{
  /* first column */
  const char *helpdoc = _ (shell_builtins[i].short_doc);

  buf[0] = (shell_builtins[i].flags & BUILTIN_ENABLED) ? ' ' : '*';
  strncpy (buf + 1, helpdoc, width - 2);
  buf[width - 2] = '>'; /* indicate truncation */
  buf[width - 1] = '\0';
  printf ("%s", buf);
  if (((i << 1) >= num_shell_builtins) || (i + height >= num_shell_builtins))
    {
      printf ("\n");
      return;
    }

  int dispcols = strlen (buf);
  /* two spaces */
  for (int j = dispcols; j < width; j++)
    putc (' ', stdout);

  /* second column */
  helpdoc = _ (shell_builtins[i + height].short_doc);

  buf[0] = (shell_builtins[i + height].flags & BUILTIN_ENABLED) ? ' ' : '*';
  strncpy (buf + 1, helpdoc, width - 3);
  buf[width - 3] = '>'; /* indicate truncation */
  buf[width - 2] = '\0';

  printf ("%s\n", buf);
}

#if defined(HANDLE_MULTIBYTE)
static void
wdispcolumn (int i, char *buf, size_t bufsize, int width, int height)
{
  /* first column */
  const char *helpdoc = _ (shell_builtins[i].short_doc);

  wchar_t *wcstr = 0;
  size_t slen = mbstowcs ((wchar_t *)0, helpdoc, 0);
  if (slen == (size_t)-1)
    {
      dispcolumn (i, buf, bufsize, width, height);
      return;
    }

  /* No bigger than the passed max width */
  if (slen >= width)
    slen = width - 2;
  wcstr = (wchar_t *)xmalloc (sizeof (wchar_t) * (width + 2));
  size_t n = mbstowcs (wcstr + 1, helpdoc, slen + 1);
  wcstr[n + 1] = L'\0';

  /* Turn tabs and newlines into spaces for column display, since wcwidth
     returns -1 for them */
  for (int j = 1; j < n; j++)
    if (wcstr[j] == L'\n' || wcstr[j] == L'\t')
      wcstr[j] = L' ';

  /* dispchars == number of characters that will be displayed */
  int dispchars = wcsnwidth (wcstr + 1, slen, width - 2);
  /* dispcols == number of columns required to display DISPCHARS */
  int dispcols = wcswidth (wcstr + 1, dispchars) + 1; /* +1 for ' ' or '*' */

  wcstr[0] = (shell_builtins[i].flags & BUILTIN_ENABLED) ? L' ' : L'*';

  if (dispcols >= width - 2)
    {
      wcstr[dispchars] = L'>'; /* indicate truncation */
      wcstr[dispchars + 1] = L'\0';
    }

  printf ("%ls", wcstr);
  if (((i << 1) >= num_shell_builtins) || (i + height >= num_shell_builtins))
    {
      printf ("\n");
      free (wcstr);
      return;
    }

  /* at least one space */
  for (int j = dispcols; j < width; j++)
    putc (' ', stdout);

  /* second column */
  helpdoc = _ (shell_builtins[i + height].short_doc);
  slen = mbstowcs ((wchar_t *)0, helpdoc, 0);
  if (slen == (size_t)-1)
    {
      /* for now */
      printf ("%c%s\n",
              (shell_builtins[i + height].flags & BUILTIN_ENABLED) ? ' ' : '*',
              helpdoc);
      free (wcstr);
      return;
    }

  /* Reuse wcstr since it is already width wide chars long */
  if (slen >= width)
    slen = width - 2;
  n = mbstowcs (wcstr + 1, helpdoc, slen + 1);
  wcstr[n + 1] = L'\0'; /* make sure null-terminated */

  /* Turn tabs and newlines into spaces for column display */
  for (int j = 1; j < n; j++)
    if (wcstr[j] == L'\n' || wcstr[j] == L'\t')
      wcstr[j] = L' ';

  /* dispchars == number of characters that will be displayed */
  dispchars = wcsnwidth (wcstr + 1, slen, width - 2);
  dispcols = wcswidth (wcstr + 1, dispchars) + 1; /* +1 for ' ' or '*' */

  wcstr[0]
      = (shell_builtins[i + height].flags & BUILTIN_ENABLED) ? L' ' : L'*';

  /* The dispchars-1 is there for terminals that behave strangely when you
     have \n in the nth column for terminal width n; this is what bash-4.3
     did. */
  if (dispcols >= width - 2)
    {
      wcstr[dispchars - 1] = L'>'; /* indicate truncation */
      wcstr[dispchars] = L'\0';
    }

  printf ("%ls\n", wcstr);

  free (wcstr);
}
#endif /* HANDLE_MULTIBYTE */

static void
show_builtin_command_help ()
{
  printf (_ (
      "These shell commands are defined internally.  Type `help' to see this list.\n\
Type `help name' to find out more about the function `name'.\n\
Use `info bash' to find out more about the shell in general.\n\
Use `man -k' or `info' to find out more about commands not in this list.\n\
\n\
A star (*) next to a name means that the command is disabled.\n\
\n"));

  int width = default_columns ();
  char blurb[128];

  width /= 2;
  if (width > sizeof (blurb))
    width = sizeof (blurb);
  if (width <= 3)
    width = 40;
  int height = (num_shell_builtins + 1) / 2; /* number of rows */

  for (int i = 0; i < height; i++)
    {
      QUIT;

#if defined(HANDLE_MULTIBYTE)
      if (MB_CUR_MAX > 1)
        wdispcolumn (i, blurb, sizeof (blurb), width, height);
      else
#endif
        dispcolumn (i, blurb, sizeof (blurb), width, height);
    }
}

} // namespace bash

#endif /* HELP_BUILTIN */
