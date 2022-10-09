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

#include "config.hh"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashtypes.hh"
#include "filecntl.hh"
#include "posixstat.hh"

#include "shell.hh"

namespace bash
{

// Unwind handler class for _evalfile () to restore shell state before
// return or during a throw.
class EvalFileUnwindHandler
{
public:
  // The constructor stores the state to be restored on exit or throw.
  EvalFileUnwindHandler (Shell &s, char *string, evalfile_flags_t flags_)
      : shell (s), the_printed_command_except_trap (
                       shell.the_printed_command_except_trap),
        flags (flags_)
  {
    parse_and_execute_level = shell.parse_and_execute_level;
    indirection_level = shell.indirection_level;
    line_number = shell.line_number;
    line_number_for_err_trap = shell.line_number_for_err_trap;
    loop_level = shell.loop_level;
    executing_list = shell.executing_list;
    comsub_ignore_return = shell.comsub_ignore_return;

    if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
      {
        interactive = shell.interactive;
        shell.interactive = (flags & SEVAL_INTERACT);
      }

#if defined(HISTORY)
    remember_on_history = shell.remember_on_history;
#if defined(BANG_HISTORY)
    history_expansion_inhibited = shell.history_expansion_inhibited;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

    interactive_shell = shell.interactive_shell;
    if (interactive_shell)
      current_prompt_level = shell.get_current_prompt_level ();

    if (shell.parser_expanding_alias ())
      parser_expanding_alias = true;

    if (string && ((flags & SEVAL_NOFREE) == 0))
      orig_string_to_free = string;

#if defined(HISTORY)
    if (flags & SEVAL_NOHIST)
      shell.bash_history_disable ();

#if defined(BANG_HISTORY)
    if (flags & SEVAL_NOHISTEXP)
      shell.history_expansion_inhibited = true;
#endif /* BANG_HISTORY */
#endif /* HISTORY */

#if 0
  if (flags & FEVAL_UNWINDPROT)
    {
      begin_unwind_frame ("_evalfile");

      unwind_protect_var (return_catch_flag);
      unwind_protect_var (return_catch);
      if (flags & FEVAL_NONINT)
        unwind_protect_var (interactive);
      unwind_protect_var (sourcelevel);
    }
  else
    {
      COPY_PROCENV (return_catch, old_return_catch);
      if (flags & FEVAL_NONINT)
        old_interactive = interactive;
    }

  if (flags & FEVAL_NONINT)
    interactive = 0;

  return_catch_flag++;
  sourcelevel++;

#if defined(ARRAY_VARS)
  array_push (bash_source_a, filename);
  t = itos (executing_line_number ());
  array_push (bash_lineno_a, t);
  delete[] t;
  array_push (funcname_a, "source"); /* not exactly right */

  fa = new func_array_state ();
  fa->source_a = bash_source_a;
  fa->source_v = bash_source_v;
  fa->lineno_a = bash_lineno_a;
  fa->lineno_v = bash_lineno_v;
  fa->funcname_a = funcname_a;
  fa->funcname_v = funcname_v;

  if (flags & FEVAL_UNWINDPROT)
    add_unwind_protect_ptr (restore_funcarray_state, fa);

#if defined(DEBUGGER)
  /* Have to figure out a better way to do this when `source' is supplied
     arguments */
  if ((flags & FEVAL_NOPUSHARGS) == 0)
    {
      if (shell_compatibility_level <= 44)
        init_bash_argv ();
      array_push (bash_argv_a, filename); /* XXX - unconditionally? */
      tt[0] = '1';
      tt[1] = '\0';
      array_push (bash_argc_a, tt);

      if (flags & FEVAL_UNWINDPROT)
        add_unwind_protect (pop_args);
    }
#endif
#endif

  return_val = setjmp_nosigs (return_catch);
#endif
  }

  // Unwind all values stored in this object on object destruction.
  ~EvalFileUnwindHandler ()
  {
    if (unwound)
      return;

    // Delete the original string if this is non-nullptr.
    delete[] orig_string_to_free;

    if (parser_expanding_alias)
      shell.parser_restore_alias ();

    // Note: this call doesn't have a corresponding push in the constructor.
    shell.pop_stream ();

#if defined(HISTORY)
#if defined(BANG_HISTORY)
    shell.history_expansion_inhibited = history_expansion_inhibited;
#endif /* BANG_HISTORY */
    if (parse_and_execute_level == 0)
      shell.remember_on_history = shell.enable_history_list;
    else
      shell.remember_on_history = remember_on_history;
#endif /* HISTORY */

    if (flags & (SEVAL_NONINT | SEVAL_INTERACT))
      shell.interactive = interactive;

    if (!the_printed_command_except_trap.empty ())
      shell.the_printed_command_except_trap = the_printed_command_except_trap;

    shell.comsub_ignore_return = comsub_ignore_return;
    shell.executing_list = executing_list;
    shell.loop_level = loop_level;
    shell.line_number_for_err_trap = line_number_for_err_trap;
    shell.line_number = line_number;
    shell.indirection_level = indirection_level;
    shell.parse_and_execute_level = parse_and_execute_level;
    unwound = true;

#if 0
  if (flags & FEVAL_UNWINDPROT)
    run_unwind_frame ("_evalfile");
  else
    {
      if (flags & FEVAL_NONINT)
        interactive = old_interactive;
#if defined(ARRAY_VARS)
      restore_funcarray_state (fa);
#if defined(DEBUGGER)
      if ((flags & FEVAL_NOPUSHARGS) == 0)
        {
          /* Don't need to call pop_args here until we do something better
             when source is passed arguments (see above). */
          array_pop (bash_argc_a);
          array_pop (bash_argv_a);
        }
#endif
#endif
      return_catch_flag--;
      sourcelevel--;
      COPY_PROCENV (old_return_catch, return_catch);
    }
#endif
  }

private:
  Shell &shell;
  std::string the_printed_command_except_trap;
  char *orig_string_to_free;
  evalfile_flags_t flags;
  int parse_and_execute_level;
  int indirection_level;
  int line_number;
  int line_number_for_err_trap;
  int loop_level;
  int executing_list;
  int current_prompt_level;
  bool unwound;
  bool comsub_ignore_return;
  bool interactive;
  char remember_on_history;
  bool history_expansion_inhibited;
  bool interactive_shell;
  bool parser_expanding_alias;
};

int
Shell::_evalfile (const char *filename, evalfile_flags_t flags)
{
  int return_val, result, i, nnull;
  ssize_t nr; /* return value from read(2) */
  char *string;
  struct stat finfo;
  size_t file_size;
  sh_vmsg_func_t errfunc;
#if defined(ARRAY_VARS)
  SHELL_VAR *funcname_v, *bash_source_v, *bash_lineno_v;
  ARRAY *funcname_a, *bash_source_a, *bash_lineno_a;
#if defined(DEBUGGER)
  SHELL_VAR *bash_argv_v, *bash_argc_v;
  ARRAY *bash_argv_a, *bash_argc_a;
#endif
#endif

#if defined(ARRAY_VARS)
  GET_ARRAY_FROM_VAR ("FUNCNAME", funcname_v, funcname_a);
  GET_ARRAY_FROM_VAR ("BASH_SOURCE", bash_source_v, bash_source_a);
  GET_ARRAY_FROM_VAR ("BASH_LINENO", bash_lineno_v, bash_lineno_a);
#if defined(DEBUGGER)
  GET_ARRAY_FROM_VAR ("BASH_ARGV", bash_argv_v, bash_argv_a);
  GET_ARRAY_FROM_VAR ("BASH_ARGC", bash_argc_v, bash_argc_a);
#endif
#endif

  int fd = open (filename, O_RDONLY);

  if (fd < 0 || (fstat (fd, &finfo) == -1))
    {
      i = errno;
      if (fd >= 0)
        close (fd);

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

  file_size = static_cast<size_t> (finfo.st_size);
  /* Check for overflow with large files. */
  if (file_size != finfo.st_size || file_size + 1 < file_size)
    {
      ((*this).*errfunc) (_ ("%s: file is too large"), filename);
      close (fd);
      return (flags & FEVAL_BUILTIN) ? EXECUTION_FAILURE : -1;
    }

  if (S_ISREG (finfo.st_mode) && file_size <= SSIZE_MAX)
    {
      string = new char[1 + file_size];
      nr = read (fd, string, file_size);
      if (nr >= 0)
        string[nr] = '\0';
    }
  else
    nr = zmapfd (fd, &string);

  return_val = errno;
  close (fd);
  errno = return_val;

  if (nr < 0) /* XXX was != file_size, not < 0 */
    goto file_error_and_exit;

  if (nr == 0)
    return (flags & FEVAL_BUILTIN) ? EXECUTION_SUCCESS : 1;

  if ((flags & FEVAL_CHECKBINARY)
      && check_binary_file (string, (nr > 80) ? 80 : nr))
    {
      ((*this).*errfunc) (_ ("%s: cannot execute binary file"), filename);
      return (flags & FEVAL_BUILTIN) ? EX_BINARY_FILE : -1;
    }

  size_t pos = strlen (string);
  if (pos < nr)
    {
      for (nnull = pos = 0; pos < nr; pos++)
        if (string[pos] == '\0')
          {
            memmove (string + pos, string + pos + 1, nr - pos);
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

#if 0
  /* If `return' was seen outside of a function, but in the script, then
     force parse_and_execute () to clean up. */
  if (return_val)
    {
      parse_and_execute_cleanup (-1);
      result = return_catch_value;
    }
  else
#endif

  // Construct an unwind handler to restore the state on return or throw.
  EvalFileUnwindHandler unwind_handler (*this, string, flags);

  /* set the flags to be passed to parse_and_execute */
  parse_flags pflags = SEVAL_RESETLINE;
  if ((flags & FEVAL_HISTORY) == 0)
    pflags |= SEVAL_NOHIST;

  result = parse_and_execute (string, filename, pflags);

  /* If we end up with EOF after sourcing a file, which can happen when the
     file doesn't end with a newline, pretend that it did. */
  if (current_token == parser::token::yacc_EOF)
    push_token (static_cast<parser::token::token_kind_type> ('\n')); /* XXX */

  return (flags & FEVAL_BUILTIN) ? result : 1;
}

int
Shell::maybe_execute_file (const char *fname, bool force_noninteractive)
{
  char *filename = bash_tilde_expand (fname, 0);

  evalfile_flags_t flags = FEVAL_ENOENTOK;
  if (force_noninteractive)
    flags |= FEVAL_NONINT;

  try
  {
    int result = _evalfile (filename, flags);
    delete[] filename;
    return result;
  }
  catch (const bash_exception &)
  {
    delete[] filename;
    throw;
  }
}

int
Shell::force_execute_file (const char *fname, bool force_noninteractive)
{
  char *filename = bash_tilde_expand (fname, 0);

  evalfile_flags_t flags = FEVAL_NOFLAGS;
  if (force_noninteractive)
    flags |= FEVAL_NONINT;

  try
  {
    int result = _evalfile (filename, flags);
    delete[] filename;
    return result;
  }
  catch (const bash_exception &)
  {
    delete[] filename;
    throw;
  }
}

#if defined(HISTORY)
int
Shell::fc_execute_file (const char *filename)
{
  evalfile_flags_t flags;

  /* We want these commands to show up in the history list if
     remember_on_history is set.  We use FEVAL_BUILTIN to return
     the result of parse_and_execute. */
  flags = FEVAL_ENOENTOK | FEVAL_HISTORY | FEVAL_REGFILE | FEVAL_BUILTIN;
  return _evalfile (filename, flags);
}
#endif /* HISTORY */

int
Shell::source_file (const char *filename, int sflags)
{
  evalfile_flags_t flags = FEVAL_BUILTIN | FEVAL_UNWINDPROT | FEVAL_NONINT;

  if (sflags)
    flags |= FEVAL_NOPUSHARGS;

  /* POSIX shells exit if non-interactive and file error. */
  if (posixly_correct && !interactive_shell && !executing_command_builtin)
    flags |= FEVAL_THROW;

  int rval = _evalfile (filename, flags);

  run_return_trap ();
  return rval;
}

} // namespace bash
