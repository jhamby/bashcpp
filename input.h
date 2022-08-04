/* input.h -- Structures and unions used for reading input. */

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

#if !defined (_INPUT_H_)
#define _INPUT_H_

#include <string>
using std::string;

/* Function pointers can be declared as (Function *)foo. */
// #if !defined (_FUNCTION_DEF)
// #  define _FUNCTION_DEF
// typedef int Function ();
// typedef void VFunction ();
// #endif /* _FUNCTION_DEF */

typedef int sh_cget_func_t ();		/* sh_ivoidfunc_t */
typedef int sh_cunget_func_t (int);	/* sh_intfunc_t */

enum stream_type {st_none, st_stdin, st_stream, st_string, st_bstream};

#if defined (BUFFERED_INPUT)

/* Possible values for b_flag. */
#undef B_EOF
#undef B_ERROR		/* There are some systems with this define */
#undef B_UNBUFF

#define B_EOF		0x01
#define B_ERROR		0x02
#define B_UNBUFF	0x04
#define B_WASBASHINPUT	0x08
#define B_TEXT		0x10
#define B_SHAREDBUF	0x20	/* shared input buffer */

/* A buffered stream.  Like a FILE *, but with our own buffering and
   synchronization.  Look in input.c for the implementation. */
struct BufferedStream
{
  int	 b_fd;
  char	*b_buffer;		/* The buffer that holds characters read. */
  size_t b_size;		/* How big the buffer is. */
  size_t b_used;		/* How much of the buffer we're using, */
  int	 b_flag;		/* Flag values. */
  size_t b_inputp;		/* The input pointer, index into b_buffer. */
};

extern int default_buffered_input;
extern int bash_input_fd_changed;

#endif /* BUFFERED_INPUT */

union InputStream {
  FILE *file;
  char *string;			/* written to by the parser */
#if defined (BUFFERED_INPUT)
  int buffered_fd;
#endif
};

struct BashInput {
  enum stream_type type;
  char *name;			/* freed by the parser */
  InputStream location;
  sh_cget_func_t *getter;
  sh_cunget_func_t *ungetter;
};

extern BashInput bash_input;

/* Functions from parse.y whose use directly or indirectly depends on the
   definitions in this file. */
extern void initialize_bash_input ();
extern void init_yy_io (sh_cget_func_t *, sh_cunget_func_t *, enum stream_type, const char *, InputStream);
extern const char *yy_input_name ();
extern void with_input_from_stdin ();
extern void with_input_from_string (const string &, const string &);
extern void with_input_from_stream (FILE *, const string &);
extern void push_stream (int);
extern void pop_stream ();
extern int stream_on_stack (enum stream_type);
extern char *read_secondary_line (int);
extern int find_reserved_word (const string &);
extern void gather_here_documents ();
extern void execute_variable_command (const string &, const string &);

extern int *save_token_state ();
extern void restore_token_state (int *);

/* Functions from input.c */
extern int getc_with_restart (FILE *);
extern int ungetc_with_restart (int, FILE *);

#if defined (BUFFERED_INPUT)
/* Functions from input.c. */
extern int fd_is_bash_input (int);
extern int set_bash_input_fd (int);
extern int save_bash_input (int, int);
extern int check_bash_input (int);
extern int duplicate_buffered_stream (int, int);
extern BufferedStream *fd_to_buffered_stream (int);
extern BufferedStream *set_buffered_stream (int, BufferedStream *);
extern BufferedStream *open_buffered_stream (string);
extern void free_buffered_stream (BufferedStream *);
extern int close_buffered_stream (BufferedStream *);
extern int close_buffered_fd (int);
extern int sync_buffered_stream (int);
extern int buffered_getchar ();
extern int buffered_ungetchar (int);
extern void with_input_from_buffered_stream (int, string);
#endif /* BUFFERED_INPUT */

#endif /* _INPUT_H_ */
