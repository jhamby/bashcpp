// This file is enable_def.cc.
// It implements the builtin "enable" in Bash.

// Copyright (C) 1987-2020 Free Software Foundation, Inc.

// This file is part of GNU Bash, the Bourne Again SHell.

// Bash is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Bash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Bash.  If not, see <http://www.gnu.org/licenses/>.

// $BUILTIN enable
// $FUNCTION enable_builtin
// $SHORT_DOC enable [-a] [-dnps] [-f filename] [name ...]
// Enable and disable shell builtins.

// Enables and disables builtin shell commands.  Disabling allows you to
// execute a disk command which has the same name as a shell builtin
// without using a full pathname.

// Options:
//   -a	print a list of builtins showing whether or not each is enabled
//   -n	disable each NAME or display a list of disabled builtins
//   -p	print the list of builtins in a reusable format
//   -s	print only the names of Posix `special' builtins

// Options controlling dynamic loading:
//   -f	Load builtin NAME from shared object FILENAME
//   -d	Remove a builtin loaded with -f

// Without options, each NAME is enabled.

// To use the `test' found in $PATH instead of the shell builtin
// version, type `enable -n test'.

// Exit Status:
// Returns success unless NAME is not a shell builtin or an error occurs.
// $END

#include "config.h"

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "bashintl.hh"

#include "builtins.hh"
#include "common.hh"
#include "flags.hh"
#include "shell.hh"

#if defined(PROGRAMMABLE_COMPLETION)
#include "pcomplete.hh"
#endif

namespace bash
{

#define ENABLED 1
#define DISABLED 2
#define SPECIAL 4

#define AFLAG 0x01
#define DFLAG 0x02
#define FFLAG 0x04
#define NFLAG 0x08
#define PFLAG 0x10
#define SFLAG 0x20

#if defined(HAVE_DLOPEN) && defined(HAVE_DLSYM)
static int dyn_load_builtin (WORD_LIST *, int, const char *);
#endif

#if defined(HAVE_DLCLOSE)
static int dyn_unload_builtin (const char *);
static void delete_builtin (struct builtin *);
static int local_dlclose (void *);
#endif

#define STRUCT_SUFFIX "_struct"
/* for now */
#define LOAD_SUFFIX "_builtin_load"
#define UNLOAD_SUFFIX "_builtin_unload"

static void list_some_builtins (int);
static int enable_shell_command (const char *, bool);

/* Enable/disable shell commands present in LIST.  If list is not specified,
   then print out a list of shell commands showing which are enabled and
   which are disabled. */
int
Shell::enable_builtin (WORD_LIST *list)
{
  int result, flags;
  int opt, filter;
#if defined(HAVE_DLOPEN) && defined(HAVE_DLSYM)
  char *filename;
#endif

  result = EXECUTION_SUCCESS;
  flags = 0;

  reset_internal_getopt ();
  while ((opt = internal_getopt (list, "adnpsf:")) != -1)
    {
      switch (opt)
        {
        case 'a':
          flags |= AFLAG;
          break;
        case 'n':
          flags |= NFLAG;
          break;
        case 'p':
          flags |= PFLAG;
          break;
        case 's':
          flags |= SFLAG;
          break;
        case 'f':
#if defined(HAVE_DLOPEN) && defined(HAVE_DLSYM)
          flags |= FFLAG;
          filename = list_optarg;
          break;
#else
          builtin_error (_ ("dynamic loading not available"));
          return EX_USAGE;
#endif
#if defined(HAVE_DLCLOSE)
        case 'd':
          flags |= DFLAG;
          break;
#else
          builtin_error (_ ("dynamic loading not available"));
          return EX_USAGE;
#endif /* HAVE_DLCLOSE */
          CASE_HELPOPT;
        default:
          builtin_usage ();
          return EX_USAGE;
        }
    }

  list = loptend;

#if defined(RESTRICTED_SHELL)
  /* Restricted shells cannot load new builtins. */
  if (restricted && (flags & (FFLAG | DFLAG)))
    {
      sh_restricted (nullptr);
      return EXECUTION_FAILURE;
    }
#endif

  if (list == 0 || (flags & PFLAG))
    {
      filter = (flags & AFLAG)   ? (ENABLED | DISABLED)
               : (flags & NFLAG) ? DISABLED
                                 : ENABLED;

      if (flags & SFLAG)
        filter |= SPECIAL;

      list_some_builtins (filter);
    }
#if defined(HAVE_DLOPEN) && defined(HAVE_DLSYM)
  else if (flags & FFLAG)
    {
      filter = (flags & NFLAG) ? DISABLED : ENABLED;
      if (flags & SFLAG)
        filter |= SPECIAL;

      result = dyn_load_builtin (list, filter, filename);
#if defined(PROGRAMMABLE_COMPLETION)
      set_itemlist_dirty (&it_builtins);
#endif
    }
#endif
#if defined(HAVE_DLCLOSE)
  else if (flags & DFLAG)
    {
      while (list)
        {
          opt = dyn_unload_builtin (list->word->word);
          if (opt == EXECUTION_FAILURE)
            result = EXECUTION_FAILURE;
          list = list->next ();
        }
#if defined(PROGRAMMABLE_COMPLETION)
      set_itemlist_dirty (&it_builtins);
#endif
    }
#endif
  else
    {
      while (list)
        {
          opt = enable_shell_command (list->word->word, flags & NFLAG);

          if (opt == EXECUTION_FAILURE)
            {
              sh_notbuiltin (list->word->word);
              result = EXECUTION_FAILURE;
            }
          list = list->next ();
        }
    }
  return result;
}

/* List some builtins.
   FILTER is a mask with two slots: ENABLED and DISABLED. */
static void
list_some_builtins (int filter)
{
  for (int i = 0; i < num_shell_builtins; i++)
    {
      if (shell_builtins[i].function == 0
          || (shell_builtins[i].flags & BUILTIN_DELETED))
        continue;

      if ((filter & SPECIAL)
          && (shell_builtins[i].flags & SPECIAL_BUILTIN) == 0)
        continue;

      if ((filter & ENABLED) && (shell_builtins[i].flags & BUILTIN_ENABLED))
        printf ("enable %s\n", shell_builtins[i].name);
      else if ((filter & DISABLED)
               && ((shell_builtins[i].flags & BUILTIN_ENABLED) == 0))
        printf ("enable -n %s\n", shell_builtins[i].name);
    }
}

/* Enable the shell command NAME.  If DISABLE_P is non-zero, then
   disable NAME instead. */
static int
enable_shell_command (const char *name, bool disable_p)
{
  struct builtin *b;

  b = builtin_address_internal (name, 1);
  if (b == 0)
    return EXECUTION_FAILURE;

  if (disable_p)
    b->flags &= ~BUILTIN_ENABLED;
#if defined(RESTRICTED_SHELL)
  else if (restricted && ((b->flags & BUILTIN_ENABLED) == 0))
    {
      sh_restricted ((char *)NULL);
      return EXECUTION_FAILURE;
    }
#endif
  else
    b->flags |= BUILTIN_ENABLED;

#if defined(PROGRAMMABLE_COMPLETION)
  set_itemlist_dirty (&it_enabled);
  set_itemlist_dirty (&it_disabled);
#endif

  return EXECUTION_SUCCESS;
}

#if defined(HAVE_DLOPEN) && defined(HAVE_DLSYM)

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

static int
dyn_load_builtin (WORD_LIST *list, int flags, const char *filename)
{
  void *handle;

  int total, size, r;
  char *struct_name, *name, *funcname;
  sh_load_func_t *loadfunc;
  struct builtin **new_builtins, *b, *new_shell_builtins, *old_builtin;
  char *loadables_path, *load_path;

  if (list == 0)
    return EXECUTION_FAILURE;

#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif

  handle = 0;
  if (absolute_program (filename) == 0)
    {
      loadables_path = get_string_value ("BASH_LOADABLES_PATH");
      if (loadables_path)
        {
          load_path = find_in_path (filename, loadables_path,
                                    FS_NODIRS | FS_EXEC_PREFERRED);
          if (load_path)
            {
#if defined(_AIX)
              handle = dlopen (load_path, RTLD_NOW | RTLD_GLOBAL);
#else
              handle = dlopen (load_path, RTLD_LAZY);
#endif /* !_AIX */
              free (load_path);
            }
        }
    }

  /* Fall back to current directory for now */
  if (handle == 0)
#if defined(_AIX)
    handle = dlopen (filename, RTLD_NOW | RTLD_GLOBAL);
#else
    handle = dlopen (filename, RTLD_LAZY);
#endif /* !_AIX */

  if (handle == 0)
    {
      name = printable_filename (filename, 0);
      builtin_error (_ ("cannot open shared object %s: %s"), name, dlerror ());
      if (name != filename)
        free (name);
      return EXECUTION_FAILURE;
    }

  int new_size = 0;
  for (WORD_LIST *l = list; l; l = (WORD_LIST *)l->next, new_size++)
    ;
  new_builtins
      = (struct builtin **)xmalloc (new_size * sizeof (struct builtin *));

  /* For each new builtin in the shared object, find it and its describing
     structure.  If this is overwriting an existing builtin, do so, otherwise
     save the loaded struct for creating the new list of builtins. */
  int replaced;
  for (replaced = new_size = 0; list; list = list->next ())
    {
      name = list->word->word;

      size = strlen (name);
      struct_name = (char *)xmalloc (size + 8);
      strcpy (struct_name, name);
      strcpy (struct_name + size, STRUCT_SUFFIX);

      old_builtin = builtin_address_internal (name, 1);

      b = (struct builtin *)dlsym (handle, struct_name);
      if (b == 0)
        {
          name = printable_filename (filename, 0);
          builtin_error (_ ("cannot find %s in shared object %s: %s"),
                         struct_name, name, dlerror ());
          if (name != filename)
            free (name);
          free (struct_name);
          continue;
        }

      funcname
          = (char *)xrealloc (struct_name, size + sizeof (LOAD_SUFFIX) + 1);
      strcpy (funcname, name);
      strcpy (funcname + size, LOAD_SUFFIX);

      loadfunc = (sh_load_func_t *)dlsym (handle, funcname);
      if (loadfunc)
        {
          /* Add warning if running an init function more than once */
          if (old_builtin && (old_builtin->flags & STATIC_BUILTIN) == 0)
            builtin_warning (_ ("%s: dynamic builtin already loaded"), name);
          r = (*loadfunc) (name);
          if (r == 0)
            {
              builtin_error (
                  _ ("load function for %s returns failure (%d): not loaded"),
                  name, r);
              free (funcname);
              continue;
            }
        }
      free (funcname);

      b->flags &= ~STATIC_BUILTIN;
      if (flags & SPECIAL)
        b->flags |= SPECIAL_BUILTIN;
      b->handle = handle;

      if (old_builtin)
        {
          replaced++;
          FASTCOPY ((char *)b, (char *)old_builtin, sizeof (struct builtin));
        }
      else
        new_builtins[new_size++] = b;
    }

  if (replaced == 0 && new_size == 0)
    {
      free (new_builtins);
      dlclose (handle);
      return EXECUTION_FAILURE;
    }

  if (new_size)
    {
      total = num_shell_builtins + new_size;
      size = (total + 1) * sizeof (struct builtin);

      new_shell_builtins = (struct builtin *)xmalloc (size);
      FASTCOPY ((char *)shell_builtins, (char *)new_shell_builtins,
                num_shell_builtins * sizeof (struct builtin));
      for (replaced = 0; replaced < new_size; replaced++)
        FASTCOPY ((char *)new_builtins[replaced],
                  (char *)&new_shell_builtins[num_shell_builtins + replaced],
                  sizeof (struct builtin));

      new_shell_builtins[total].name = (char *)0;
      new_shell_builtins[total].function = (sh_builtin_func_t *)0;
      new_shell_builtins[total].flags = 0;

      if (shell_builtins != static_shell_builtins)
        free (shell_builtins);

      shell_builtins = new_shell_builtins;
      num_shell_builtins = total;
      initialize_shell_builtins ();
    }

  free (new_builtins);
  return EXECUTION_SUCCESS;
}
#endif

#if defined(HAVE_DLCLOSE)
static void
delete_builtin (struct builtin *b)
{
  int ind, size;
  struct builtin *new_shell_builtins;

  /* XXX - funky pointer arithmetic - XXX */
  ind = b - shell_builtins;
  size = num_shell_builtins * sizeof (struct builtin);
  new_shell_builtins = (struct builtin *)xmalloc (size);

  /* Copy shell_builtins[0]...shell_builtins[ind - 1] to new_shell_builtins */
  if (ind)
    FASTCOPY ((char *)shell_builtins, (char *)new_shell_builtins,
              ind * sizeof (struct builtin));
  /* Copy shell_builtins[ind+1]...shell_builtins[num_shell_builtins to
     new_shell_builtins, starting at ind. */
  FASTCOPY ((char *)(&shell_builtins[ind + 1]),
            (char *)(&new_shell_builtins[ind]),
            (num_shell_builtins - ind) * sizeof (struct builtin));

  if (shell_builtins != static_shell_builtins)
    free (shell_builtins);

  /* The result is still sorted. */
  num_shell_builtins--;
  shell_builtins = new_shell_builtins;
}

/* Tenon's MachTen has a dlclose that doesn't return a value, so we
   finesse it with a local wrapper. */
static int
local_dlclose (void *handle)
{
#if !defined(__MACHTEN__)
  return dlclose (handle);
#else  /* __MACHTEN__ */
  dlclose (handle);
  return (dlerror () != NULL) ? -1 : 0;
#endif /* __MACHTEN__ */
}

static int
dyn_unload_builtin (const char *name)
{
  struct builtin *b;
  void *handle;
  char *funcname;
  sh_unload_func_t *unloadfunc;
  int ref, i, size;

  b = builtin_address_internal (name, 1);
  if (b == 0)
    {
      sh_notbuiltin (name);
      return EXECUTION_FAILURE;
    }
  if (b->flags & STATIC_BUILTIN)
    {
      builtin_error (_ ("%s: not dynamically loaded"), name);
      return EXECUTION_FAILURE;
    }

  handle = (void *)b->handle;
  for (ref = i = 0; i < num_shell_builtins; i++)
    {
      if (shell_builtins[i].handle == b->handle)
        ref++;
    }

  /* Call any unload function */
  size = strlen (name);
  funcname = (char *)xmalloc (size + sizeof (UNLOAD_SUFFIX) + 1);
  strcpy (funcname, name);
  strcpy (funcname + size, UNLOAD_SUFFIX);

  unloadfunc = (sh_unload_func_t *)dlsym (handle, funcname);
  if (unloadfunc)
    (*unloadfunc) (name); /* void function */
  free (funcname);

  /* Don't remove the shared object unless the reference count of builtins
     using it drops to zero. */
  if (ref == 1 && local_dlclose (handle) != 0)
    {
      builtin_error (_ ("%s: cannot delete: %s"), name, dlerror ());
      return EXECUTION_FAILURE;
    }

  /* Now remove this entry from the builtin table and reinitialize. */
  delete_builtin (b);

  return EXECUTION_SUCCESS;
}
#endif

} // namespace bash
