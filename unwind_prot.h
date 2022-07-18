/* unwind_prot.h - Macros and functions for hacking unwind protection. */

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

#if !defined (_UNWIND_PROT_H)
#define _UNWIND_PROT_H

extern void uwp_init ();

/* Run a function without interrupts. */
extern void begin_unwind_frame (const char *);
extern void discard_unwind_frame (const char *);
extern void run_unwind_frame (const char *);
extern void add_unwind_protect (sh_voidfunc_t *cleanup);
extern void add_unwind_protect_int (sh_intfunc_t *cleanup, int arg);
extern void add_unwind_protect_ptr (sh_free_func_t *cleanup, void *arg);
extern void remove_unwind_protect ();
extern void run_unwind_protects ();
extern void clear_unwind_protect_list (bool);
extern bool have_unwind_protects ();
extern bool unwind_protect_tag_on_stack (const char *);
extern void uwp_init ();

/* Define for people who like their code to look a certain way. */
#define end_unwind_frame()

/* How to protect a variable (note: casts away volatile).  */
#define unwind_protect_var(X) unwind_protect_mem ((void *)&(X), sizeof (X))
extern void unwind_protect_mem (void *, size_t);

#endif /* _UNWIND_PROT_H */
