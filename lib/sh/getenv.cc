/* getenv.cc - get environment variable value from the shell's variable
              list. */

/* Copyright (C) 1997-2002 Free Software Foundation, Inc.

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

#if defined(CAN_REDEFINE_GETENV)

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "shell.hh"

extern "C" char *
getenv (const char *name)
{
  return bash::the_shell->getenv (name);
}

/* Some versions of Unix use _getenv instead. */
extern "C" char *
_getenv (const char *name)
{
  return bash::the_shell->getenv (name);
}

/* We supply our own version of getenv () because we want library
   routines to get the changed values of exported variables. */
char *
bash::Shell::getenv (const char *name)
{
  SHELL_VAR *var;

  if (name == nullptr || *name == '\0')
    return nullptr;

  var = find_tempenv_variable (name);
  if (var)
    {
      FREE (last_tempenv_value);

      last_tempenv_value
          = value_cell (var) ? savestring (value_cell (var)) : nullptr;
      return last_tempenv_value;
    }
  else if (shell_variables)
    {
      var = find_variable (name);
      if (var && exported_p (var))
        return value_cell (var);
    }
  else if (environ)
    {
      int i, len;

      /* In some cases, s5r3 invokes getenv() before main(); BSD systems
         using gprof also exhibit this behavior.  This means that
         shell_variables will be 0 when this is invoked.  We look up the
         variable in the real environment in that case. */

      for (i = 0, len = strlen (name); environ[i]; i++)
        {
          if ((STREQN (environ[i], name, len)) && (environ[i][len] == '='))
            return environ[i] + len + 1;
        }
    }

  return nullptr;
}

/* SUSv3 says argument is a `char *'; BSD implementations disagree */
int
#ifndef HAVE_STD_PUTENV
putenv (const char *str)
#else
putenv (char *str)
#endif
{
  SHELL_VAR *var;
  char *name, *value;
  int offset;

  if (str == 0 || *str == '\0')
    {
      errno = EINVAL;
      return -1;
    }

  offset = assignment (str, 0);
  if (str[offset] != '=')
    {
      errno = EINVAL;
      return -1;
    }
  name = savestring (str);
  name[offset] = 0;

  value = name + offset + 1;

  /* XXX - should we worry about readonly here? */
  var = bind_variable (name, value, 0);
  if (var == 0)
    {
      errno = EINVAL;
      return -1;
    }

  VUNSETATTR (var, att_invisible);
  VSETATTR (var, att_exported);

  return 0;
}

#if 0
int
#ifndef HAVE_STD_PUTENV
_putenv (const char *name)
#else
_putenv (char *name)
#endif
{
  return putenv (name);
}
#endif

int
setenv (const char *name, const char *value, int rewrite)
{
  SHELL_VAR *var;
  char *v;

  if (name == 0 || *name == '\0' || strchr (name, '=') != 0)
    {
      errno = EINVAL;
      return -1;
    }

  var = 0;
  v = (char *)value; /* some compilers need explicit cast */
  /* XXX - should we worry about readonly here? */
  if (rewrite == 0)
    var = find_variable (name);

  if (var == 0)
    var = bind_variable (name, v, 0);

  if (var == 0)
    return -1;

  VUNSETATTR (var, att_invisible);
  VSETATTR (var, att_exported);

  return 0;
}

#if 0
int
_setenv (const char *name, const char *value, int rewrite)
{
  return setenv (name, value, rewrite);
}
#endif

/* SUSv3 says unsetenv returns int; existing implementations (BSD) disagree. */

#ifdef HAVE_STD_UNSETENV
#define UNSETENV_RETURN(N) return (N)
#define UNSETENV_RETTYPE int
#else
#define UNSETENV_RETURN(N) return
#define UNSETENV_RETTYPE void
#endif

UNSETENV_RETTYPE
unsetenv (const char *name)
{
  if (name == 0 || *name == '\0' || strchr (name, '=') != 0)
    {
      errno = EINVAL;
      UNSETENV_RETURN (-1);
    }

    /* XXX - should we just remove the export attribute here? */
#if 1
  unbind_variable (name);
#else
  SHELL_VAR *v;

  v = find_variable (name);
  if (v)
    VUNSETATTR (v, att_exported);
#endif

  UNSETENV_RETURN (0);
}

#endif /* CAN_REDEFINE_GETENV */
