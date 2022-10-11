/* evalfile.cc - read and evaluate commands from a file or file descriptor */

/* Copyright (C) 1996-2017 Free Software Foundation, Inc.

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

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashtypes.hh"
#include "filecntl.hh"
#include "posixstat.hh"

#include "shell.hh"

namespace bash
{

// Unwind handler class for _evalfile () to restore the shell state before
// returning or throwing.
class EvalFileUnwindHandler
{
public:
  // Unwind handler constructor stores the state to be restored.
  EvalFileUnwindHandler (Shell &s, char *string_, evalfile_flags_t flags_,
                         func_array_state *fa_)
      : shell (s), string (string_), fa (fa_), flags (flags_)
  {
    return_catch_flag = shell.return_catch_flag;
    sourcelevel = shell.sourcelevel;

    if (flags & FEVAL_NONINT)
      {
        interactive = shell.interactive;
        shell.interactive = false;
      }

    shell.return_catch_flag++;
    shell.sourcelevel++;
  }

  // Destructor to delete the string and restore the original values.
  ~EvalFileUnwindHandler ()
  {
    shell.sourcelevel--;
    shell.return_catch_flag--;

#if defined(ARRAY_VARS)
    shell.restore_funcarray_state (fa);
#endif

    delete[] string;

    if (flags & FEVAL_NONINT)
      shell.interactive = interactive;

    shell.sourcelevel = sourcelevel;
    shell.return_catch_flag = return_catch_flag;
  }

private:
  Shell &shell;
  char *string;
  func_array_state *fa;

  evalfile_flags_t flags;
  int return_catch_flag;
  int sourcelevel;
  bool interactive;
};

int
Shell::_evalfile (const char *filename, evalfile_flags_t flags)
{
#if defined(ARRAY_VARS)
  SHELL_VAR *funcname_v, *bash_source_v, *bash_lineno_v;
  ARRAY *funcname_a, *bash_source_a, *bash_lineno_a;
#endif

#if defined(ARRAY_VARS)
  GET_ARRAY_FROM_VAR ("FUNCNAME", funcname_v, funcname_a);
  GET_ARRAY_FROM_VAR ("BASH_SOURCE", bash_source_v, bash_source_a);
  GET_ARRAY_FROM_VAR ("BASH_LINENO", bash_lineno_v, bash_lineno_a);
#endif

  struct stat finfo;

  int fd = open (filename, O_RDONLY);
  int i;

  if (fd < 0 || (fstat (fd, &finfo) == -1))
    {
      i = errno;
      if (fd >= 0)
        (void)close (fd);

      errno = i;

    file_error_and_exit:
      if (((flags & FEVAL_ENOENTOK) == 0) || errno != ENOENT)
        file_error (filename);

      if (flags & FEVAL_THROW)
        {
          last_command_exit_value = EXECUTION_FAILURE;
          throw bash_exception (EXITPROG);
        }

      return (
          (flags & FEVAL_BUILTIN)
              ? EXECUTION_FAILURE
              : ((errno == ENOENT && (flags & FEVAL_ENOENTOK) != 0) ? 0 : -1));
    }

  sh_vmsg_func_t errfunc;
  errfunc = ((flags & FEVAL_BUILTIN) ? &Shell::builtin_error
                                     : &Shell::internal_error);

  if (S_ISDIR (finfo.st_mode))
    {
      ((*this).*errfunc) (_ ("%s: is a directory"), filename);
      close (fd);
      return (flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE : -1;
    }
  else if ((flags & FEVAL_REGFILE) && S_ISREG (finfo.st_mode) == 0)
    {
      ((*this).*errfunc) (_ ("%s: not a regular file"), filename);
      close (fd);
      return (flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE : -1;
    }

  ssize_t file_size = static_cast<ssize_t> (finfo.st_size);

  /* Check for overflow with large files. */
  if (static_cast<off_t> (file_size) != finfo.st_size)
    {
      ((*this).*errfunc) (_ ("%s: file is too large"), filename);
      close (fd);
      return (flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE : -1;
    }

  char *string = nullptr;
  ssize_t nr; /* return value from read(2) */

  if (S_ISREG (finfo.st_mode) && file_size <= SSIZE_MAX)
    {
      string = new char[1 + static_cast<size_t> (file_size)];
      nr = 0;
      // XXX - loop for systems that require multiple reads for large files
      while (nr < file_size)
        {
          ssize_t this_nr
              = read (fd, &(string[nr]), static_cast<size_t> (file_size - nr));
          if (this_nr <= 0)
            {
              if (this_nr < 0)
                nr = this_nr; // propagate error return
              break;
            }
          else
            nr += this_nr;
        }
      if (nr > file_size)
        nr = file_size;

      string[nr] = '\0';
    }
  else
    nr = zmapfd (fd, &string);

  int result;
  result = errno;
  (void)close (fd);
  errno = result;

  if (nr < 0)
    {
      delete[] string;
      goto file_error_and_exit;
    }

  if (nr == 0)
    {
      delete[] string;
      return (flags & FEVAL_BUILTIN) ? EXECUTION_SUCCESS : 1;
    }

  if ((flags & FEVAL_CHECKBINARY)
      && check_binary_file (string, (nr > 80) ? 80 : static_cast<size_t> (nr)))
    {
      delete[] string;
      ((*this).*errfunc) (_ ("%s: cannot execute binary file"), filename);
      return (flags & FEVAL_BUILTIN) ? EX_BINARY_FILE : -1;
    }

  size_t pos = strlen (string);
  if (pos < static_cast<size_t> (nr))
    {
      size_t nnull;
      for (nnull = pos = 0; pos < static_cast<size_t> (nr); pos++)
        if (string[pos] == '\0')
          {
            memmove (string + pos, string + pos + 1,
                     static_cast<size_t> (nr) - pos);
            nr--;
            /* Even if the `check binary' flag is not set, we want to avoid
               sourcing files with more than 256 null characters -- that
               probably indicates a binary file. */
            if ((flags & FEVAL_BUILTIN) && ++nnull > 256)
              {
                delete[] string;
                ((*this).*errfunc) (_ ("%s: cannot execute binary file"),
                                    filename);
                return (flags & FEVAL_BUILTIN) ? EX_BINARY_FILE : -1;
              }
          }
    }

  func_array_state *fa = nullptr;

#if defined(ARRAY_VARS)
  bash_source_a->shift_element (filename);
  bash_lineno_a->shift_element (itos (executing_line_number ()));
  funcname_a->shift_element ("source"); /* not exactly right */

  fa = new func_array_state ();
  fa->source_a = bash_source_a;
  fa->source_v = bash_source_v;
  fa->lineno_a = bash_lineno_a;
  fa->lineno_v = bash_lineno_v;
  fa->funcname_a = funcname_a;
  fa->funcname_v = funcname_v;
#endif

  // set the flags to be passed to parse_and_execute ().
  parse_flags pflags = SEVAL_RESETLINE | SEVAL_NOFREE;

  if ((flags & FEVAL_HISTORY) == 0)
    pflags |= SEVAL_NOHIST;

  // Make a local unwind handler to delete the string and restore variable and
  // parser state on return or throw.
  EvalFileUnwindHandler unwind_handler (*this, string, flags, fa);

  try
    {
      result = parse_and_execute (string, filename, pflags);
    }
  catch (const return_catch_exception &e)
    {
      result = e.return_catch_value;
    }

  /* If we end up with EOF after sourcing a file, which can happen when the
     file doesn't end with a newline, pretend that it did. */
  if (current_token == parser::token::yacc_EOF)
    push_token (static_cast<parser::token::token_kind_type> ('\n')); /* XXX */

  return (flags & FEVAL_BUILTIN) ? result : 1;
}

} // namespace bash
