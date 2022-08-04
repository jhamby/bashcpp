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

namespace bash
{

/* This variable means to not expand associative array subscripts more than
   once, when performing variable expansion. */
// char assoc_expand_once;

/* The analog for indexed array subscripts */
// char array_expand_once;

/* Flags for array_value_internal and callers array_value/get_array_value */
enum av_flags {
  AV_ALLOWALL =		0x001,
  AV_QUOTED =		0x002,
  AV_USEIND =		0x004,
  AV_USEVAL =		0x008,	/* XXX - should move this */
  AV_ASSIGNRHS =	0x010,	/* no splitting, special case ${a[@]} */
  AV_NOEXPAND =		0x020	/* don't run assoc subscripts through word expansion */
};

/* Flags for valid_array_reference. Value 1 is reserved for skipsubscript() */
enum valid_array_flags {
  VA_NOEXPAND =	0x001,
  VA_ONEWORD =	0x002
};

SHELL_VAR *convert_var_to_array (SHELL_VAR *);
SHELL_VAR *convert_var_to_assoc (SHELL_VAR *);

char *make_array_variable_value (SHELL_VAR *, arrayind_t, const char *, const char *, int);

SHELL_VAR *bind_array_variable (const char *, arrayind_t, const char *, int);
SHELL_VAR *bind_array_element (SHELL_VAR *, arrayind_t, const char *, int);
SHELL_VAR *assign_array_element (const char *, const char *, int);

SHELL_VAR *bind_assoc_variable (SHELL_VAR *, const char *, char *, const char *, int);

SHELL_VAR *find_or_make_array_variable (const char *, int);

SHELL_VAR *assign_array_from_string  (const char *, const char *, int);
SHELL_VAR *assign_array_var_from_word_list (SHELL_VAR *, WORD_LIST *, int);

WORD_LIST *expand_compound_array_assignment (SHELL_VAR *, const char *, int);
void assign_compound_array_list (SHELL_VAR *, WORD_LIST *, int);
SHELL_VAR *assign_array_var_from_string (SHELL_VAR *, const char *, int);

char *expand_and_quote_assoc_word (const char *, int);
void quote_compound_array_list (WORD_LIST *, int);

bool kvpair_assignment_p (WORD_LIST *);
char *expand_and_quote_kvpair_word (const char *);

int unbind_array_element (SHELL_VAR *, char *, int);
int skipsubscript (const char *, int, int);

void print_array_assignment (SHELL_VAR *, int);
void print_assoc_assignment (SHELL_VAR *, int);

arrayind_t array_expand_index (SHELL_VAR *, const char *, int, int);
bool valid_array_reference (const char *, int);
char *array_value (const char *, int, int, int *, arrayind_t *);
char *get_array_value (const char *, int, int *, arrayind_t *);

char *array_keys (const char *, int, int);

char *array_variable_name (const char *, int, char **, int *);
SHELL_VAR *array_variable_part (const char *, int, char **, int *);

}  // namespace bash

#else

#define AV_ALLOWALL	0
#define AV_QUOTED	0
#define AV_USEIND	0
#define AV_ASSIGNRHS	0

#define VA_ONEWORD	0

#endif

#endif /* !_ARRAYFUNC_H_ */
