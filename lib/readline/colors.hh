/* `dir', `vdir' and `ls' directory listing programs for GNU.

   Modified by Chet Ramey for Readline.

   Copyright (C) 1985, 1988, 1990-1991, 1995-2010, 2012, 2015
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

#ifndef _COLORS_H_
#define _COLORS_H_

namespace readline
{

/* The LS_COLORS variable is in a termcap-like format. */
struct COLOR_EXT_TYPE
{
  std::string ext;      /* The extension we're looking for */
  std::string seq;      /* The sequence to output when we do */
  COLOR_EXT_TYPE *next; /* Next in list */
};

/* file extensions indicators (.txt, .log, .jpg, ...)
   Values are taken from $LS_COLORS in rl_parse_colors(). */
extern COLOR_EXT_TYPE *_rl_color_ext_list;

#define FILETYPE_INDICATORS                                                   \
  {                                                                           \
    C_ORPHAN, C_FIFO, C_CHR, C_DIR, C_BLK, C_FILE, C_LINK, C_SOCK, C_FILE,    \
        C_DIR                                                                 \
  }

#if !defined(S_IXUGO)
#define S_IXUGO (S_IXUSR | S_IXGRP | S_IXOTH)
#endif

enum filetype
{
  unknown,
  fifo,
  chardev,
  directory,
  blockdev,
  normal,
  symbolic_link,
  sock,
  whiteout,
  arg_directory
};

/* Prefix color, currently same as socket */
#define C_PREFIX C_SOCK

} // namespace readline

#endif /* !_COLORS_H_ */
