/* mailcheck.cc -- The check is in the mail... */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

#include "config.h"

#include "bashtypes.hh"
#include "posixstat.hh"

#if defined(HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"
#include "posixtime.hh"

#include "shell.hh"

#include "tilde.hh"

namespace bash
{

/* Values for flags word in struct _fileinfo */
#define MBOX_INITIALIZED 0x01

extern time_t shell_start_time;

extern int mailstat (const char *, struct stat *);

struct FileInfo
{
  char *name;
  char *msg;
  time_t access_time;
  time_t mod_time;
  off_t file_size;
  int flags;
};

/* The list of remembered mail files. */
static FILEINFO **mailfiles = (FILEINFO **)NULL;

/* Number of mail files that we have. */
static int mailfiles_count;

/* The last known time that mail was checked. */
static time_t last_time_mail_checked = 0;

/* Non-zero means warn if a mail file has been read since last checked. */
char mail_warning;

static int find_mail_file (char *);
static void init_mail_file (int);
static void update_mail_file (int);
static int add_mail_file (char *, char *);

static FILEINFO *alloc_mail_file (char *, char *);
static void dispose_mail_file (FILEINFO *);

static bool file_mod_date_changed (int);
static bool file_access_date_changed (int);
static bool file_has_grown (int);

static char *parse_mailpath_spec (char *);

/* Returns non-zero if it is time to check mail. */
int
time_to_check_mail ()
{
  char *temp;
  time_t now;
  int64_t seconds;

  temp = get_string_value ("MAILCHECK");

  /* Negative number, or non-numbers (such as empty string) cause no
     checking to take place. */
  if (temp == 0 || legal_number (temp, &seconds) == 0 || seconds < 0)
    return 0;

  now = NOW;
  /* Time to check if MAILCHECK is explicitly set to zero, or if enough
     time has passed since the last check. */
  return seconds == 0 || ((now - last_time_mail_checked) >= seconds);
}

/* Okay, we have checked the mail.  Perhaps I should make this function
   go away. */
void
reset_mail_timer ()
{
  last_time_mail_checked = NOW;
}

/* Locate a file in the list.  Return index of
   entry, or -1 if not found. */
static int
find_mail_file (char *file)
{
  int i;

  for (i = 0; i < mailfiles_count; i++)
    if (STREQ (mailfiles[i]->name, file))
      return i;

  return -1;
}

#define RESET_MAIL_FILE(i)                                                    \
  do                                                                          \
    {                                                                         \
      mailfiles[i]->access_time = mailfiles[i]->mod_time = 0;                 \
      mailfiles[i]->file_size = 0;                                            \
      mailfiles[i]->flags = 0;                                                \
    }                                                                         \
  while (0)

#define UPDATE_MAIL_FILE(i, finfo)                                            \
  do                                                                          \
    {                                                                         \
      mailfiles[i]->access_time = finfo.st_atime;                             \
      mailfiles[i]->mod_time = finfo.st_mtime;                                \
      mailfiles[i]->file_size = finfo.st_size;                                \
      mailfiles[i]->flags |= MBOX_INITIALIZED;                                \
    }                                                                         \
  while (0)

static void
init_mail_file (int i)
{
  mailfiles[i]->access_time = (mailfiles[i]->mod_time = last_time_mail_checked)
                                  ? last_time_mail_checked
                                  : shell_start_time;
  mailfiles[i]->file_size = 0;
  mailfiles[i]->flags = 0;
}

static void
update_mail_file (int i)
{
  char *file;
  struct stat finfo;

  file = mailfiles[i]->name;
  if (mailstat (file, &finfo) == 0)
    UPDATE_MAIL_FILE (i, finfo);
  else
    RESET_MAIL_FILE (i);
}

/* Add this file to the list of remembered files and return its index
   in the list of mail files. */
static int
add_mail_file (char *file, char *msg)
{
  struct stat finfo;
  char *filename;
  int i;

  filename = full_pathname (file);
  i = find_mail_file (filename);
  if (i >= 0)
    {
      if (mailstat (filename, &finfo) == 0)
        UPDATE_MAIL_FILE (i, finfo);

      free (filename);
      return i;
    }

  i = mailfiles_count++;
  mailfiles = (FILEINFO **)xrealloc (mailfiles,
                                     mailfiles_count * sizeof (FILEINFO *));

  mailfiles[i] = alloc_mail_file (filename, msg);
  init_mail_file (i);

  return i;
}

/* Reset the existing mail files access and modification times to zero. */
void
reset_mail_files ()
{
  int i;

  for (i = 0; i < mailfiles_count; i++)
    RESET_MAIL_FILE (i);
}

static FILEINFO *
alloc_mail_file (char *filename, char *msg)
{
  FILEINFO *mf;

  mf = (FILEINFO *)xmalloc (sizeof (FILEINFO));
  mf->name = filename;
  mf->msg = msg ? savestring (msg) : (char *)NULL;
  mf->flags = 0;

  return mf;
}

static void
dispose_mail_file (FILEINFO *mf)
{
  free (mf->name);
  FREE (mf->msg);
  free (mf);
}

/* Free the information that we have about the remembered mail files. */
void
free_mail_files ()
{
  int i;

  for (i = 0; i < mailfiles_count; i++)
    dispose_mail_file (mailfiles[i]);

  if (mailfiles)
    free (mailfiles);

  mailfiles_count = 0;
  mailfiles = (FILEINFO **)NULL;
}

void
init_mail_dates ()
{
  if (mailfiles == 0)
    remember_mail_dates ();
}

/* Return non-zero if FILE's mod date has changed and it has not been
   accessed since modified.  If the size has dropped to zero, reset
   the cached mail file info. */
static bool
file_mod_date_changed (int i)
{
  time_t mtime;
  struct stat finfo;
  char *file;

  file = mailfiles[i]->name;
  mtime = mailfiles[i]->mod_time;

  if (mailstat (file, &finfo) != 0)
    return false;

  if (finfo.st_size > 0)
    return mtime < finfo.st_mtime;

  if (finfo.st_size == 0 && mailfiles[i]->file_size > 0)
    UPDATE_MAIL_FILE (i, finfo);

  return false;
}

/* Return non-zero if FILE's access date has changed. */
static bool
file_access_date_changed (int i)
{
  time_t atime;
  struct stat finfo;
  char *file;

  file = mailfiles[i]->name;
  atime = mailfiles[i]->access_time;

  if (mailstat (file, &finfo) != 0)
    return false;

  if (finfo.st_size > 0)
    return atime < finfo.st_atime;

  return false;
}

/* Return non-zero if FILE's size has increased. */
static bool
file_has_grown (int i)
{
  off_t size;
  struct stat finfo;
  char *file;

  file = mailfiles[i]->name;
  size = mailfiles[i]->file_size;

  return mailstat (file, &finfo) == 0) && (finfo.st_size > size;
}

/* Take an element from $MAILPATH and return the portion from
   the first unquoted `?' or `%' to the end of the string.  This is the
   message to be printed when the file contents change. */
static char *
parse_mailpath_spec (char *str)
{
  char *s;
  int pass_next;

  for (s = str, pass_next = 0; s && *s; s++)
    {
      if (pass_next)
        {
          pass_next = 0;
          continue;
        }
      if (*s == '\\')
        {
          pass_next++;
          continue;
        }
      if (*s == '?' || *s == '%')
        return s;
    }
  return (char *)NULL;
}

char *
make_default_mailpath ()
{
#if defined(DEFAULT_MAIL_DIRECTORY)
  char *mp;

  get_current_user_info ();
  mp = (char *)xmalloc (2 + sizeof (DEFAULT_MAIL_DIRECTORY)
                        + strlen (current_user.user_name));
  strcpy (mp, DEFAULT_MAIL_DIRECTORY);
  mp[sizeof (DEFAULT_MAIL_DIRECTORY) - 1] = '/';
  strcpy (mp + sizeof (DEFAULT_MAIL_DIRECTORY), current_user.user_name);
  return mp;
#else
  return (char *)NULL;
#endif
}

/* Remember the dates of the files specified by MAILPATH, or if there is
   no MAILPATH, by the file specified in MAIL.  If neither exists, use a
   default value, which we randomly concoct from using Unix. */

void
remember_mail_dates ()
{
  char *mailpaths = get_string_value ("MAILPATH");

  /* If no $MAILPATH, but $MAIL, use that as a single filename to check. */
  if (mailpaths == 0 && (mailpaths = get_string_value ("MAIL")))
    {
      add_mail_file (mailpaths, (char *)NULL);
      return;
    }

  if (mailpaths == 0)
    {
      mailpaths = make_default_mailpath ();
      if (mailpaths)
        {
          add_mail_file (mailpaths, (char *)NULL);
          free (mailpaths);
        }
      return;
    }

  char *mailfile;
  int i = 0;
  while ((mailfile = extract_colon_unit (mailpaths, &i)))
    {
      char *mp = parse_mailpath_spec (mailfile);
      if (mp && *mp)
        *mp++ = '\0';
      add_mail_file (mailfile, mp);
      free (mailfile);
    }
}

/* check_mail () is useful for more than just checking mail.  Since it has
   the paranoids dream ability of telling you when someone has read your
   mail, it can just as easily be used to tell you when someones .profile
   file has been read, thus letting one know when someone else has logged
   in.  Pretty good, huh? */

/* Check for mail in some files.  If the modification date of any
   of the files in MAILPATH has changed since we last did a
   remember_mail_dates () then mention that the user has mail.
   Special hack:  If the variable MAIL_WARNING is non-zero and the
   mail file has been accessed since the last time we remembered, then
   the message "The mail in <mailfile> has been read" is printed. */
void
check_mail ()
{
  //   char *current_mail_file, *message;
  //   int i, use_user_notification;
  //   char *dollar_underscore, *temp;

  char *dollar_underscore = get_string_value ("_");
  if (dollar_underscore)
    dollar_underscore = savestring (dollar_underscore);

  for (int i = 0; i < mailfiles_count; i++)
    {
      char *current_mail_file = mailfiles[i]->name;

      if (*current_mail_file == '\0')
        continue;

      if (file_mod_date_changed (i))
        {
          bool use_user_notification = mailfiles[i]->msg != (char *)NULL;
          const char *message = mailfiles[i]->msg ? mailfiles[i]->msg
                                                  : _ ("You have mail in $_");

          bind_variable ("_", current_mail_file, 0);

#define atime mailfiles[i]->access_time
#define mtime mailfiles[i]->mod_time

          /* Have to compute this before the call to update_mail_file, which
             resets all the information. */
          bool file_is_bigger = file_has_grown (i);

          update_mail_file (i);

          /* If the user has just run a program which manipulates the
             mail file, then don't bother explaining that the mail
             file has been manipulated.  Since some systems don't change
             the access time to be equal to the modification time when
             the mail in the file is manipulated, check the size also.  If
             the file has not grown, continue. */
          if ((atime >= mtime) && !file_is_bigger)
            continue;

          /* If the mod time is later than the access time and the file
             has grown, note the fact that this is *new* mail. */
          if (use_user_notification == 0 && (atime < mtime) && file_is_bigger)
            message = _ ("You have new mail in $_");
#undef atime
#undef mtime

          char *temp;
          if ((temp = expand_string_to_string (message, Q_DOUBLE_QUOTES)))
            {
              puts (temp);
              free (temp);
            }
          else
            putchar ('\n');
        }

      if (mail_warning && file_access_date_changed (i))
        {
          update_mail_file (i);
          printf (_ ("The mail in %s has been read\n"), current_mail_file);
        }
    }

  if (dollar_underscore)
    {
      bind_variable ("_", dollar_underscore, 0);
      free (dollar_underscore);
    }
  else
    unbind_variable ("_");
}

} // namespace bash
