/* bashhist.h -- interface to the bash history functions in bashhist.c. */

/* Copyright (C) 1993-2020 Free Software Foundation,  Inc.

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

#if !defined (_BASHHIST_H_)
#define _BASHHIST_H_

#include "stdc.h"

/* Flag values for history_control */
#define HC_IGNSPACE	0x01
#define HC_IGNDUPS	0x02
#define HC_ERASEDUPS	0x04

#define HC_IGNBOTH	(HC_IGNSPACE|HC_IGNDUPS)

#if defined (STRICT_POSIX)
#  undef HISTEXPAND_DEFAULT
#  define HISTEXPAND_DEFAULT	0
#else
#  if !defined (HISTEXPAND_DEFAULT)
#    define HISTEXPAND_DEFAULT	1
#  endif /* !HISTEXPAND_DEFAULT */
#endif

extern bool remember_on_history;
extern char enable_history_list;		/* value for `set -o history' */
extern char literal_history;			/* controlled by `shopt lithist' */
extern char force_append_history;
extern int history_lines_this_session;
extern int history_lines_in_file;
extern char history_expansion;
extern int history_control;
extern char command_oriented_history;
extern bool current_command_first_line_saved;
extern bool current_command_first_line_comment;
extern bool hist_last_line_added;
extern bool hist_last_line_pushed;

extern char dont_save_function_defs;

#  if defined (READLINE)
extern char hist_verify;
#  endif

#  if defined (BANG_HISTORY)
extern bool history_expansion_inhibited;
extern bool double_quotes_inhibit_history_expansion;
#  endif /* BANG_HISTORY */

extern void bash_initialize_history (void);
extern void bash_history_reinit (bool);
extern void bash_history_disable (void);
extern void bash_history_enable (void);
extern void bash_clear_history (void);
extern bool bash_delete_histent (int);
extern bool bash_delete_history_range (int, int);
extern bool bash_delete_last_history (void);
extern void load_history (void);
extern void save_history (void);
extern int maybe_append_history (const char *);
extern int maybe_save_shell_history (void);
extern char *pre_process_line (char *, bool, bool);
extern void maybe_add_history (char *);
extern void bash_add_history (const char *);
extern bool check_add_history (char *, bool);
extern int history_number (void);

extern void setup_history_ignore ();

extern char *last_history_line (void);

#endif /* _BASHHIST_H_ */
