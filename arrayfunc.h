/* arrayfunc.h -- declarations for miscellaneous array functions in arrayfunc.c */

/* Copyright (C) 2001-2020 Free Software Foundation, Inc.

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

#if !defined (_ARRAYFUNC_H_)
#define _ARRAYFUNC_H_

/* Must include variables.h before including this file. */

#if defined (ARRAY_VARS)

/* This variable means to not expand associative array subscripts more than
   once, when performing variable expansion. */
extern char assoc_expand_once;

/* The analog for indexed array subscripts */
extern char array_expand_once;

/* Flags for array_value_internal and callers array_value/get_array_value */
#define AV_ALLOWALL	0x001
#define AV_QUOTED	0x002
#define AV_USEIND	0x004
#define AV_USEVAL	0x008	/* XXX - should move this */
#define AV_ASSIGNRHS	0x010	/* no splitting, special case ${a[@]} */
#define AV_NOEXPAND	0x020	/* don't run assoc subscripts through word expansion */

/* Flags for valid_array_reference. Value 1 is reserved for skipsubscript() */
#define VA_NOEXPAND	0x001
#define VA_ONEWORD	0x002

extern SHELL_VAR *convert_var_to_array (SHELL_VAR *);
extern SHELL_VAR *convert_var_to_assoc (SHELL_VAR *);

extern char *make_array_variable_value (SHELL_VAR *, arrayind_t, const char *, const char *, int);

extern SHELL_VAR *bind_array_variable (const char *, arrayind_t, const char *, int);
extern SHELL_VAR *bind_array_element (SHELL_VAR *, arrayind_t, const char *, int);
extern SHELL_VAR *assign_array_element (const char *, const char *, int);

extern SHELL_VAR *bind_assoc_variable (SHELL_VAR *, const char *, char *, const char *, int);

extern SHELL_VAR *find_or_make_array_variable (const char *, int);

extern SHELL_VAR *assign_array_from_string  (const char *, const char *, int);
extern SHELL_VAR *assign_array_var_from_word_list (SHELL_VAR *, WORD_LIST *, int);

extern WORD_LIST *expand_compound_array_assignment (SHELL_VAR *, const char *, int);
extern void assign_compound_array_list (SHELL_VAR *, WORD_LIST *, int);
extern SHELL_VAR *assign_array_var_from_string (SHELL_VAR *, const char *, int);

extern char *expand_and_quote_assoc_word (const char *, int);
extern void quote_compound_array_list (WORD_LIST *, int);

extern bool kvpair_assignment_p (WORD_LIST *);
extern char *expand_and_quote_kvpair_word (const char *);

extern int unbind_array_element (SHELL_VAR *, char *, int);
extern int skipsubscript (const char *, int, int);

extern void print_array_assignment (SHELL_VAR *, int);
extern void print_assoc_assignment (SHELL_VAR *, int);

extern arrayind_t array_expand_index (SHELL_VAR *, const char *, int, int);
extern bool valid_array_reference (const char *, int);
extern char *array_value (const char *, int, int, int *, arrayind_t *);
extern char *get_array_value (const char *, int, int *, arrayind_t *);

extern char *array_keys (const char *, int, int);

extern char *array_variable_name (const char *, int, char **, int *);
extern SHELL_VAR *array_variable_part (const char *, int, char **, int *);

#else

#define AV_ALLOWALL	0
#define AV_QUOTED	0
#define AV_USEIND	0
#define AV_ASSIGNRHS	0

#define VA_ONEWORD	0

#endif

#endif /* !_ARRAYFUNC_H_ */
