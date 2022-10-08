/* error.cc -- Functions for handling errors. */

/* Copyright (C) 1993-2021 Free Software Foundation, Inc.

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

#include <fcntl.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <cstdarg>

#include "shell.hh"

namespace bash
{

void
Shell::error_prolog (int print_lineno)
{
  const char *ename;
  int line;

  ename = get_name_for_error ();
  line = (print_lineno && !interactive_shell) ? executing_line_number () : -1;

  if (line > 0)
    std::fprintf (stderr, "%s:%s%d: ", ename,
                  gnu_error_format ? "" : _ (" line "), line);
  else
    std::fprintf (stderr, "%s: ", ename);
}

/* Return the name of the shell or the shell script for error reporting. */
const char *
Shell::get_name_for_error ()
{
  const char *name;
#if defined(ARRAY_VARS)
  SHELL_VAR *bash_source_v;
  ARRAY *bash_source_a;
#endif

  name = nullptr;
  if (!interactive_shell)
    {
#if defined(ARRAY_VARS)
      bash_source_v = find_variable ("BASH_SOURCE");
      if (bash_source_v && bash_source_v->array ()
          && (bash_source_a = bash_source_v->array_value ()))
        name = array_reference (bash_source_a, 0);
      if (name == nullptr || *name == '\0') /* XXX - was just name == 0 */
#endif
        name = dollar_vars[0];
    }
  if (name == nullptr && shell_name && *shell_name)
    name = base_pathname (shell_name);
  if (name == nullptr)
#if defined(PROGRAM)
    name = PROGRAM;
#else
    name = "bash";
#endif

  return name;
}

/* Report an error having to do with FILENAME.  This does not use
   sys_error so the filename is not interpreted as a printf-style
   format string. */
void
Shell::file_error (const char *filename)
{
  report_error ("%s: %s", filename, std::strerror (errno));
}

void
Shell::programming_error (const char *format, ...)
{
  va_list args;
  char *h;

#if defined(JOB_CONTROL)
  give_terminal_to (shell_pgrp, 0);
#endif /* JOB_CONTROL */

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");
  va_end (args);

#if defined(HISTORY)
  if (remember_on_history)
    {
      h = last_history_line ();
      std::fprintf (stderr, _ ("last command: %s\n"), h ? h : "(null)");
    }
#endif

#if 0
  std::fprintf (stderr, "Report this to %s\n", the_current_maintainer);
#endif

  std::fprintf (stderr, _ ("Aborting..."));
  std::fflush (stderr);

  std::abort ();
}

/* Print an error message and, if `set -e' has been executed, exit the
   shell.  Used in this file by file_error and programming_error.  Used
   outside this file mostly to report substitution and expansion errors,
   and for bad invocation options. */
void
Shell::report_error (const char *format, ...)
{
  va_list args;

  error_prolog (1);

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);
  if (exit_immediately_on_error)
    {
      if (last_command_exit_value == 0)
        last_command_exit_value = EXECUTION_FAILURE;
      exit_shell (last_command_exit_value);
    }
}

void
Shell::fatal_error (const char *format, ...)
{
  va_list args;

  error_prolog (0);

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);
  sh_exit (2);
}

void
Shell::internal_error (const char *format, ...)
{
  va_list args;

  error_prolog (1);

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);
}

void
Shell::internal_warning (const char *format, ...)
{
  va_list args;

  error_prolog (1);
  std::fprintf (stderr, _ ("warning: "));

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);
}

void
Shell::internal_inform (const char *format, ...)
{
  va_list args;

  error_prolog (1);
  /* TRANSLATORS: this is a prefix for informational messages. */
  std::fprintf (stderr, _ ("INFORM: "));

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);
}

#ifdef DEBUG
void
Shell::internal_debug (const char *format, ...)
{
  va_list args;

  error_prolog (1);
  std::fprintf (stderr, _ ("DEBUG warning: "));

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);
}
#endif

void
Shell::sys_error (const char *format, ...)
{
  int e;
  va_list args;

  e = errno;
  error_prolog (0);

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, ": %s\n", strerror (e));

  va_end (args);
}

/* An error from the parser takes the general form

        shell_name: input file name: line number: message

   The input file name and line number are omitted if the shell is
   currently interactive.  If the shell is not currently interactive,
   the input file name is inserted only if it is different from the
   shell name. */
void
Shell::parser_error (int lineno, const char *format, ...)
{
  va_list args;

  const char *ename = get_name_for_error ();
  std::string iname = yy_input_name ();

  if (interactive)
    std::fprintf (stderr, "%s: ", ename);
  else if (interactive_shell)
    std::fprintf (stderr, "%s: %s:%s%d: ", ename, iname.c_str (),
                  gnu_error_format ? "" : _ (" line "), lineno);
  else if (STREQ (ename, iname.c_str ()))
    std::fprintf (stderr, "%s:%s%d: ", ename,
                  gnu_error_format ? "" : _ (" line "), lineno);
  else
    std::fprintf (stderr, "%s: %s:%s%d: ", ename, iname.c_str (),
                  gnu_error_format ? "" : _ (" line "), lineno);

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);

  if (exit_immediately_on_error)
    exit_shell (last_command_exit_value = 2);
}

#ifdef DEBUG

void
itrace (const char *format, ...)
{
  va_list args;

  std::fprintf (stderr, "TRACE: pid %ld: ", static_cast<long> (getpid ()));

  va_start (args, format);

  std::vfprintf (stderr, format, args);
  std::fprintf (stderr, "\n");

  va_end (args);

  std::fflush (stderr);
}

#endif /* DEBUG */

/* **************************************************************** */
/*								    */
/*  		    Common error reporting			    */
/*								    */
/* **************************************************************** */

static const char *const cmd_error_table[]
    = { N_ ("unknown command error"), /* CMDERR_DEFAULT */
        N_ ("bad command type"),      /* CMDERR_BADTYPE */
        N_ ("bad connector"),         /* CMDERR_BADCONN */
        N_ ("bad jump"),              /* CMDERR_BADJUMP */
        nullptr };

void
Shell::command_error (const char *func, cmd_err_type code, int e)
{
  if (code > CMDERR_LAST)
    code = CMDERR_DEFAULT;

  programming_error ("%s: %s: %d", func, _ (cmd_error_table[code]), e);
}

#ifdef ARRAY_VARS
void
Shell::err_badarraysub (const char *s)
{
  // Note: this string is untranslated in the original code.
  report_error ("%s: %s", s, N_ ("bad array subscript"));
}
#endif

void
Shell::err_unboundvar (const char *s)
{
  report_error (_ ("%s: unbound variable"), s);
}

void
Shell::err_readonly (const char *s)
{
  report_error (_ ("%s: readonly variable"), s);
}

} // namespace bash
