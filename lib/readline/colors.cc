/* `dir', `vdir' and `ls' directory listing programs for GNU.

   Modified by Chet Ramey for Readline.

   Copyright (C) 1985, 1988, 1990-1991, 1995-2010, 2012, 2015, 2017, 2019
   Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Richard Stallman and David MacKenzie.  */

/* Color support by Peter Anvin <Peter.Anvin@linux.org> and Dennis
   Flaherty <dennisf@denix.elk.miles.com> based on original patches by
   Greg Lee <lee@uhunix.uhcc.hawaii.edu>.  */

#include "readline.hh"

#include "posixstat.hh" // stat related macros (S_ISREG, ...)

#include <fcntl.h> // S_ISUID

#ifdef COLOR_SUPPORT

#include "colors.hh"

namespace readline
{

int
Readline::_rl_print_prefix_color ()
{
  /* What do we want to use for the prefix? Let's try cyan first, see colors.h
   */
  const std::string &s = _rl_color_indicator[C_PREFIX];
  if (!s.empty ())
    {
      if (is_colored (C_NORM))
        restore_default_color ();
      _rl_put_indicator (_rl_color_indicator[C_LEFT]);
      _rl_put_indicator (s);
      _rl_put_indicator (_rl_color_indicator[C_RIGHT]);
      return 0;
    }
  else
    return 1;
}

/* Returns whether any color sequence was printed. */
bool
Readline::_rl_print_color_indicator (const char *f)
{
  enum indicator_no colored_filetype;
  COLOR_EXT_TYPE *ext; /* Color extension */
  size_t len;          /* Length of name */

  const char *name;
  struct stat astat, linkstat;
  mode_t mode = 0;
  int linkok; /* 1 == ok, 0 == dangling symlink, -1 == missing */
  int stat_ok;

  name = f;

  /* This should already have undergone tilde expansion */
  char *filename = nullptr;
  if (rl_filename_stat_hook)
    {
      filename = savestring (f);
      ((*this).*rl_filename_stat_hook) (&filename);
      name = filename;
    }

  stat_ok = lstat (name, &astat);
  if (stat_ok == 0)
    {
      mode = astat.st_mode;
#if defined(HAVE_LSTAT)
      if (S_ISLNK (mode))
        {
          linkok = (stat (name, &linkstat) == 0);
          if (linkok && _rl_color_indicator[C_LINK] == "target")
            mode = linkstat.st_mode;
        }
      else
#endif
        linkok = 1;
    }
  else
    linkok = -1;

  /* Is this a nonexistent file?  If so, linkok == -1.  */

  if (linkok == -1 && !_rl_color_indicator[C_MISSING].empty ())
    colored_filetype = C_MISSING;
  else if (linkok == 0 && !_rl_color_indicator[C_ORPHAN].empty ())
    colored_filetype = C_ORPHAN; /* dangling symlink */
  else if (stat_ok != 0)
    {
      static enum indicator_no filetype_indicator[] = FILETYPE_INDICATORS;
      colored_filetype = filetype_indicator[normal]; // f->filetype];
    }
  else
    {
      if (S_ISREG (mode))
        {
          colored_filetype = C_FILE;

#if defined(S_ISUID)
          if ((mode & S_ISUID) != 0 && is_colored (C_SETUID))
            colored_filetype = C_SETUID;
          else
#endif
#if defined(S_ISGID)
              if ((mode & S_ISGID) != 0 && is_colored (C_SETGID))
            colored_filetype = C_SETGID;
          else
#endif
              if (is_colored (C_CAP) && (0)) // f->has_capability)
            colored_filetype = C_CAP;
          else if ((mode & S_IXUGO) != 0 && is_colored (C_EXEC))
            colored_filetype = C_EXEC;
          else if ((1 < astat.st_nlink) && is_colored (C_MULTIHARDLINK))
            colored_filetype = C_MULTIHARDLINK;
        }
      else if (S_ISDIR (mode))
        {
          colored_filetype = C_DIR;

#if defined(S_ISVTX)
          if ((mode & S_ISVTX) && (mode & S_IWOTH)
              && is_colored (C_STICKY_OTHER_WRITABLE))
            colored_filetype = C_STICKY_OTHER_WRITABLE;
          else
#endif
              if ((mode & S_IWOTH) != 0 && is_colored (C_OTHER_WRITABLE))
            colored_filetype = C_OTHER_WRITABLE;
#if defined(S_ISVTX)
          else if ((mode & S_ISVTX) != 0 && is_colored (C_STICKY))
            colored_filetype = C_STICKY;
#endif
        }
#if defined(S_ISLNK)
      else if (S_ISLNK (mode))
        colored_filetype = C_LINK;
#endif
      else if (S_ISFIFO (mode))
        colored_filetype = C_FIFO;
#if defined(S_ISSOCK)
      else if (S_ISSOCK (mode))
        colored_filetype = C_SOCK;
#endif
      else if (S_ISBLK (mode))
        colored_filetype = C_BLK;
      else if (S_ISCHR (mode))
        colored_filetype = C_CHR;
      else
        {
          /* Classify a file of some other type as C_ORPHAN.  */
          colored_filetype = C_ORPHAN;
        }
    }

  /* Check the file's suffix only if still classified as C_FILE.  */
  ext = nullptr;
  if (colored_filetype == C_FILE)
    {
      /* Test if NAME has a recognized suffix.  */
      len = strlen (name);
      name += len; /* Pointer to final \0.  */
      for (ext = _rl_color_ext_list; ext != nullptr; ext = ext->next)
        {
          if (ext->ext.size () <= len
              && strncmp (name - ext->ext.size (), ext->ext.c_str (),
                          ext->ext.size ())
                     == 0)
            break;
        }
    }

  delete[] filename;

  {
    const std::string *s
        = ext ? &(ext->seq) : &_rl_color_indicator[colored_filetype];
    if (!s->empty ())
      {
        /* Need to reset so not dealing with attribute combinations */
        if (is_colored (C_NORM))
          restore_default_color ();
        _rl_put_indicator (_rl_color_indicator[C_LEFT]);
        _rl_put_indicator (*s);
        _rl_put_indicator (_rl_color_indicator[C_RIGHT]);
        return 0;
      }
    else
      return 1;
  }
}

void
Readline::_rl_prep_non_filename_text ()
{
  if (!_rl_color_indicator[C_END].empty ())
    _rl_put_indicator (_rl_color_indicator[C_END]);
  else
    {
      _rl_put_indicator (_rl_color_indicator[C_LEFT]);
      _rl_put_indicator (_rl_color_indicator[C_RESET]);
      _rl_put_indicator (_rl_color_indicator[C_RIGHT]);
    }
}
#endif /* COLOR_SUPPORT */

} // namespace readline
