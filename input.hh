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

#if !defined(_INPUT_H_)
#define _INPUT_H_

namespace bash
{

enum stream_type
{
  st_none,
  st_stdin,
  st_stream,
  st_string,
  st_bstream
};

#if defined(BUFFERED_INPUT)

/* Possible values for b_flag. */
enum bstream_flags
{
  B_NOFLAGS = 0,
  B_EOF = 0x01,
  B_ERROR = 0x02,
  B_UNBUFF = 0x04,
  B_WASBASHINPUT = 0x08,
  B_TEXT = 0x10,
  B_SHAREDBUF = 0x20 /* shared input buffer */
};

static inline bstream_flags &
operator|= (bstream_flags &a, const bstream_flags &b)
{
  a = static_cast<bstream_flags> (static_cast<uint32_t> (a)
                                  | static_cast<uint32_t> (b));
  return a;
}

static inline bstream_flags
operator| (const bstream_flags &a, const bstream_flags &b)
{
  return static_cast<bstream_flags> (static_cast<uint32_t> (a)
                                     | static_cast<uint32_t> (b));
}

/* A buffered stream.  Like a FILE *, but with our own buffering and
   synchronization.  Look in input.cc for the implementation. */
struct BUFFERED_STREAM
{
  char *b_buffer;  /* The buffer that holds characters read. */
  size_t b_size;   /* How big the buffer is. */
  size_t b_used;   /* How much of the buffer we're using, */
  size_t b_inputp; /* The input pointer, index into b_buffer. */
  int b_fd;
  bstream_flags b_flag; /* Flag values. */
};

#endif /* BUFFERED_INPUT */

union INPUT_STREAM
{
  FILE *file;
  char *string; /* written to by the parser */
#if defined(BUFFERED_INPUT)
  int buffered_fd;
#endif
};

} // namespace bash

#endif /* _INPUT_H_ */
