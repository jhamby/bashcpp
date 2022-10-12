/* input.cc -- functions to perform buffered input with synchronization. */

/* Copyright (C) 1992-2020 Free Software Foundation, Inc.

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

#include "bashintl.hh"

#include "input.hh"
#include "shell.hh"
#include "trap.hh"

#include <algorithm>

#if defined(EAGAIN)
#define X_EAGAIN EAGAIN
#else
#define X_EAGAIN -99
#endif

#if defined(EWOULDBLOCK)
#define X_EWOULDBLOCK EWOULDBLOCK
#else
#define X_EWOULDBLOCK -99
#endif

namespace bash
{

/* Functions to handle reading input on systems that don't restart read(2)
   if a signal is received. */

/* Posix and USG systems do not guarantee to restart read () if it is
   interrupted by a signal.  We do the read ourselves, and restart it
   if it returns EINTR. */
int
Shell::getc_with_restart (FILE *stream)
{
  CHECK_TERMSIG;

  /* Try local buffering to reduce the number of read(2) calls. */
  if (local_index == local_bufused || local_bufused == 0)
    {
      while (1)
        {
          QUIT;
          run_pending_traps ();

          local_bufused = static_cast<int> (
              ::read (::fileno (stream), localbuf, sizeof (localbuf)));
          if (local_bufused > 0)
            break;
          else if (local_bufused == 0)
            {
              local_index = 0;
              return EOF;
            }
          else if (errno == X_EAGAIN || errno == X_EWOULDBLOCK)
            {
              if (sh_unset_nodelay_mode (::fileno (stream)) < 0)
                {
                  sys_error (_ ("cannot reset nodelay mode for fd %d"),
                             ::fileno (stream));
                  local_index = local_bufused = 0;
                  return EOF;
                }
              continue;
            }
          else if (errno != EINTR)
            {
              local_index = local_bufused = 0;
              return EOF;
            }
          else if (interrupt_state || terminating_signal) /* QUIT; */
            local_index = local_bufused = 0;
        }
      local_index = 0;
    }
  unsigned char uc = static_cast<unsigned char> (localbuf[local_index++]);
  return uc;
}

#if defined(BUFFERED_INPUT)

/* A facility similar to stdio, but input-only. */

static constexpr off_t MAX_INPUT_BUFFER_SIZE = 8192;

#if !defined(SEEK_CUR)
#define SEEK_CUR 1
#endif /* !SEEK_CUR */

#define ALLOCATE_BUFFERS(n)                                                   \
  do                                                                          \
    {                                                                         \
      if ((n) >= buffers.size ())                                             \
        allocate_buffers (n);                                                 \
    }                                                                         \
  while (0)

/* Construct and return a BUFFERED_STREAM corresponding to file descriptor
   FD, using BUFFER. */
BUFFERED_STREAM *
Shell::make_buffered_stream (int fd, char *buffer, size_t bufsize)
{
  BUFFERED_STREAM *bp = new BUFFERED_STREAM ();
  size_t sfd = static_cast<size_t> (fd);
  ALLOCATE_BUFFERS (sfd);
  buffers[sfd] = bp;
  bp->b_fd = fd;
  bp->b_buffer = buffer;
  bp->b_size = bufsize;
  bp->b_used = bp->b_inputp = bp->b_flag = B_NOFLAGS;
  if (bufsize == 1)
    bp->b_flag |= B_UNBUFF;
  if (O_TEXT && (::fcntl (fd, F_GETFL) & O_TEXT) != 0)
    bp->b_flag |= B_TEXT;
  return bp;
}

/* Save the buffered stream corresponding to file descriptor FD (which bash
   is using to read input) to a buffered stream associated with NEW_FD.  If
   NEW_FD is -1, a new file descriptor is allocated with fcntl.  The new
   file descriptor is returned on success, -1 on error. */
int
Shell::save_bash_input (int fd, int new_fd)
{
  int nfd;

  /* Sync the stream so we can re-read from the new file descriptor.  We
     might be able to avoid this by copying the buffered stream verbatim
     to the new file descriptor. */
  if (buffers[static_cast<size_t> (fd)])
    sync_buffered_stream (fd);

  /* Now take care of duplicating the file descriptor that bash is
     using for input, so we can reinitialize it later. */
  nfd = (new_fd == -1) ? fcntl (fd, F_DUPFD, 10) : new_fd;
  if (nfd == -1)
    {
      if (fcntl (fd, F_GETFD, 0) == 0)
        sys_error (_ ("cannot allocate new file descriptor for bash input "
                      "from fd %d"),
                   fd);
      return -1;
    }

  size_t snfd = static_cast<size_t> (nfd);
  if (snfd < buffers.size () && buffers[snfd])
    {
      /* What's this?  A stray buffer without an associated open file
         descriptor?  Free up the buffer and report the error. */
      internal_error (
          _ ("save_bash_input: buffer already exists for new fd %d"), nfd);
      if (buffers[snfd]->b_flag & B_SHAREDBUF)
        buffers[snfd]->b_buffer = nullptr;
      free_buffered_stream (buffers[snfd]);
    }

  /* Reinitialize bash_input.location. */
  if (bash_input.type == st_bstream)
    {
      bash_input.location.buffered_fd = nfd;
      fd_to_buffered_stream (nfd);
      close_buffered_fd (fd); /* XXX */
    }
  else
    /* If the current input type is not a buffered stream, but the shell
       is not interactive and therefore using a buffered stream to read
       input (e.g. with an `eval exec 3>output' inside a script), note
       that the input fd has been changed.  pop_stream() looks at this
       value and adjusts the input fd to the new value of
       default_buffered_input accordingly. */
    bash_input_fd_changed++;

  if (default_buffered_input == fd)
    default_buffered_input = nfd;

  SET_CLOSE_ON_EXEC (nfd);
  return nfd;
}

/* Check that file descriptor FD is not the one that bash is currently
   using to read input from a script.  FD is about to be duplicated onto,
   which means that the kernel will close it for us.  If FD is the bash
   input file descriptor, we need to seek backwards in the script (if
   possible and necessary -- scripts read from stdin are still unbuffered),
   allocate a new file descriptor to use for bash input, and re-initialize
   the buffered stream.  Make sure the file descriptor used to save bash
   input is set close-on-exec. Returns 0 on success, -1 on failure.  This
   works only if fd is > 0 -- if fd == 0 and bash is reading input from
   fd 0, sync_buffered_stream is used instead, to cooperate with input
   redirection (look at redir.c:add_undo_redirect()). */
int
Shell::check_bash_input (int fd)
{
  if (fd_is_bash_input (fd))
    {
      if (fd > 0)
        return (save_bash_input (fd, -1) == -1) ? -1 : 0;
      else if (fd == 0)
        return (sync_buffered_stream (fd) == -1) ? -1 : 0;
    }
  return 0;
}

/* This is the buffered stream analogue of dup2(fd1, fd2).  The
   BUFFERED_STREAM corresponding to fd2 is deallocated, if one exists.
   BUFFERS[fd1] is copied to BUFFERS[fd2].  This is called by the
   redirect code for constructs like 4<&0 and 3</etc/rc.local. */
int
Shell::duplicate_buffered_stream (int fd1, int fd2)
{
  int is_bash_input, m;

  if (fd1 == fd2)
    return 0;

  m = std::max (fd1, fd2);
  ALLOCATE_BUFFERS (static_cast<size_t> (m));

  /* If FD2 is the file descriptor bash is currently using for shell input,
     we need to do some extra work to make sure that the buffered stream
     actually exists (it might not if fd1 was not active, and the copy
     didn't actually do anything). */
  is_bash_input = (bash_input.type == st_bstream)
                  && (bash_input.location.buffered_fd == fd2);

  size_t sfd1 = static_cast<size_t> (fd1);
  size_t sfd2 = static_cast<size_t> (fd2);

  if (buffers[sfd2])
    {
      /* If the two objects share the same b_buffer, don't free it. */
      if (buffers[sfd1] && buffers[sfd1]->b_buffer
          && buffers[sfd1]->b_buffer == buffers[sfd2]->b_buffer)
        buffers[sfd2] = nullptr;
      /* If this buffer is shared with another fd, don't free the buffer */
      else if (buffers[sfd2]->b_flag & B_SHAREDBUF)
        {
          buffers[sfd2]->b_buffer = nullptr;
          free_buffered_stream (buffers[sfd2]);
        }
      else
        free_buffered_stream (buffers[sfd2]);
    }
  buffers[sfd2] = new BUFFERED_STREAM (*buffers[sfd1]);
  if (buffers[sfd2])
    buffers[sfd2]->b_fd = fd2;

  if (is_bash_input)
    {
      if (!buffers[sfd2])
        fd_to_buffered_stream (fd2);
      buffers[sfd2]->b_flag |= B_WASBASHINPUT;
    }

  if (fd_is_bash_input (fd1)
      || (buffers[sfd1] && (buffers[sfd1]->b_flag & B_SHAREDBUF)))
    buffers[sfd2]->b_flag |= B_SHAREDBUF;

  return fd2;
}

/* Return 1 if a seek on FD will succeed. */
#define fd_is_seekable(fd) (::lseek ((fd), 0L, SEEK_CUR) >= 0)

/* Take FD, a file descriptor, and create and return a buffered stream
   corresponding to it.  If something is wrong and the file descriptor
   is invalid, return a NULL stream. */
BUFFERED_STREAM *
Shell::fd_to_buffered_stream (int fd)
{
  char *buffer;
  size_t size;
  struct stat sb;

  if (::fstat (fd, &sb) < 0)
    {
      ::close (fd);
      return nullptr;
    }

  size = (fd_is_seekable (fd)) ? static_cast<size_t> (
             std::min (sb.st_size, MAX_INPUT_BUFFER_SIZE))
                               : 1;
  if (size == 0)
    size = 1;
  buffer = new char[size];

  return make_buffered_stream (fd, buffer, size);
}

/* Read a buffer full of characters from BP, a buffered stream. */
int
Shell::b_fill_buffer (BUFFERED_STREAM *bp)
{
  ssize_t nr;
  off_t o;

  CHECK_TERMSIG;
  /* In an environment where text and binary files are treated differently,
     compensate for lseek() on text files returning an offset different from
     the count of characters read() returns.  Text-mode streams have to be
     treated as unbuffered. */
  if ((bp->b_flag & (B_TEXT | B_UNBUFF)) == B_TEXT)
    {
      o = lseek (bp->b_fd, 0, SEEK_CUR);
      nr = zread (bp->b_fd, bp->b_buffer, bp->b_size);
      if (nr > 0 && nr < lseek (bp->b_fd, 0, SEEK_CUR) - o)
        {
          lseek (bp->b_fd, o, SEEK_SET);
          bp->b_flag |= B_UNBUFF;
          bp->b_size = 1;
          nr = zread (bp->b_fd, bp->b_buffer, bp->b_size);
        }
    }
  else
    nr = zread (bp->b_fd, bp->b_buffer, bp->b_size);
  if (nr <= 0)
    {
      bp->b_used = bp->b_inputp = 0;
      bp->b_buffer[0] = 0;
      if (nr == 0)
        bp->b_flag |= B_EOF;
      else
        bp->b_flag |= B_ERROR;
      return EOF;
    }

  bp->b_used = static_cast<size_t> (nr);
  bp->b_inputp = 0;
  return bp->b_buffer[bp->b_inputp++] & 0xFF;
}

/* Get a character from buffered stream BP. */
#define bufstream_getc(bp)                                                    \
  (bp->b_inputp == bp->b_used || !bp->b_used)                                 \
      ? b_fill_buffer (bp)                                                    \
      : bp->b_buffer[bp->b_inputp++] & 0xFF

/* Seek backwards on file BFD to synchronize what we've read so far
   with the underlying file pointer. */
int
Shell::sync_buffered_stream (int bfd)
{
  BUFFERED_STREAM *bp;
  off_t chars_left;

  size_t sfd = static_cast<size_t> (bfd);
  if ((bp = buffers[sfd]) == nullptr)
    return -1;

  chars_left = static_cast<off_t> (bp->b_used - bp->b_inputp);
  if (chars_left)
    ::lseek (bp->b_fd, -chars_left, SEEK_CUR);
  bp->b_used = bp->b_inputp = 0;
  return 0;
}

int
Shell::buffered_getchar ()
{
  CHECK_TERMSIG;

  size_t sfd = static_cast<size_t> (bash_input.location.buffered_fd);
  if (bash_input.location.buffered_fd < 0 || buffers[sfd] == nullptr)
    return EOF;

  return bufstream_getc (buffers[sfd]);
}

int
Shell::buffered_ungetchar (int c)
{
  size_t sfd = static_cast<size_t> (bash_input.location.buffered_fd);
  return bufstream_ungetc (c, buffers[sfd]);
}

/* Make input come from file descriptor BFD through a buffered stream. */
void
Shell::with_input_from_buffered_stream (int bfd, string_view name)
{
  INPUT_STREAM location;
  BUFFERED_STREAM *bp;

  location.buffered_fd = bfd;
  /* Make sure the buffered stream exists. */
  bp = fd_to_buffered_stream (bfd);
  init_yy_io (bp == nullptr ? &Shell::return_EOF : &Shell::buffered_getchar,
              &Shell::buffered_ungetchar, st_bstream, name, location);
}

#if defined(TEST)

void
init_yy_io ()
{
}

process (BUFFERED_STREAM *bp)
{
  int c;

  while ((c = bufstream_getc (bp)) != EOF)
    putchar (c);
}

BASH_INPUT bash_input;

struct stat dsb; /* can be used from gdb */

/* imitate /bin/cat */
main (int argc, char **argv)
{
  int i;
  BUFFERED_STREAM *bp;

  if (argc == 1)
    {
      bp = fd_to_buffered_stream (0);
      process (bp);
      exit (0);
    }
  for (i = 1; i < argc; i++)
    {
      if (argv[i][0] == '-' && argv[i][1] == '\0')
        {
          bp = fd_to_buffered_stream (0);
          if (!bp)
            continue;
          process (bp);
          free_buffered_stream (bp);
        }
      else
        {
          bp = open_buffered_stream (argv[i]);
          if (!bp)
            continue;
          process (bp);
          close_buffered_stream (bp);
        }
    }
  exit (0);
}
#endif /* TEST */
#endif /* BUFFERED_INPUT */

} // namespace bash
