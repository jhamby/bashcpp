/* findcmd.h - functions from findcmd.c. */

/* Copyright (C) 1997-2015,2020 Free Software Foundation, Inc.

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

#if !defined(_FINDCMD_H_)
#define _FINDCMD_H_

namespace bash
{

/* Flags for search_for_command */
#define CMDSRCH_HASH 0x01
#define CMDSRCH_STDPATH 0x02
#define CMDSRCH_TEMPENV 0x04

extern int file_status (string_view);
extern bool executable_file (string_view);
extern bool is_directory (string_view);
extern bool executable_or_directory (string_view);
extern std::string find_user_command (string_view);
extern std::string find_in_path (string_view, string_view, int);
extern std::string find_path_file (string_view);
extern std::string search_for_command (string_view, int);
extern std::string user_command_matches (string_view, int, int);
extern void setup_exec_ignore ();

#if 0
extern bool dot_found_in_search;
/* variables managed via shopt */
extern bool check_hashed_filenames;
#endif

} // namespace bash

#endif /* _FINDCMD_H_ */
