/* pcomplete.h - structure definitions and other stuff for programmable
                 completion. */

/* Copyright (C) 1999-2020 Free Software Foundation, Inc.

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

#if !defined(_PCOMPLETE_H_)
#define _PCOMPLETE_H_

#include "hashlib.hh"

namespace bash
{

/* Values for COMPSPEC actions.  These are things the shell knows how to
   build internally. */
enum compspec_action_t
{
  CA_NOACTION = 0,
  CA_ALIAS = (1 << 0),
  CA_ARRAYVAR = (1 << 1),
  CA_BINDING = (1 << 2),
  CA_BUILTIN = (1 << 3),
  CA_COMMAND = (1 << 4),
  CA_DIRECTORY = (1 << 5),
  CA_DISABLED = (1 << 6),
  CA_ENABLED = (1 << 7),
  CA_EXPORT = (1 << 8),
  CA_FILE = (1 << 9),
  CA_FUNCTION = (1 << 10),
  CA_GROUP = (1 << 11),
  CA_HELPTOPIC = (1 << 12),
  CA_HOSTNAME = (1 << 13),
  CA_JOB = (1 << 14),
  CA_KEYWORD = (1 << 15),
  CA_RUNNING = (1 << 16),
  CA_SERVICE = (1 << 17),
  CA_SETOPT = (1 << 18),
  CA_SHOPT = (1 << 19),
  CA_SIGNAL = (1 << 20),
  CA_STOPPED = (1 << 21),
  CA_USER = (1 << 22),
  CA_VARIABLE = (1 << 23)
};

/* Values for COMPSPEC options field. */
enum compspec_option_t
{
  COPT_NONE = 0,
  COPT_RESERVED = (1 << 0), /* reserved for other use */
  COPT_DEFAULT = (1 << 1),
  COPT_FILENAMES = (1 << 2),
  COPT_DIRNAMES = (1 << 3),
  COPT_NOQUOTE = (1 << 4),
  COPT_NOSPACE = (1 << 5),
  COPT_BASHDEFAULT = (1 << 6),
  COPT_PLUSDIRS = (1 << 7),
  COPT_NOSORT = (1 << 8),

  COPT_LASTUSER = COPT_NOSORT,

  PCOMP_RETRYFAIL = (COPT_LASTUSER << 1),
  PCOMP_NOTFOUND = (COPT_LASTUSER << 2)
};

struct COMPSPEC
{
  char *globpat;
  char *words;
  char *prefix;
  char *suffix;
  char *funcname;
  char *command;
  char *lcommand;
  char *filterpat;
  unsigned int refcount;
  compspec_action_t actions;
  compspec_option_t options;
};

/* List of items is used by the code that implements the programmable
   completions. */
typedef struct _list_of_items
{
  uint32_t flags;
  int (*list_getter) (
      struct _list_of_items *); /* function to call to get the list */

  STRINGLIST *slist;

  /* These may or may not be used. */
  STRINGLIST
  *genlist;     /* for handing to the completion code one item at a time */
  int genindex; /* index of item last handed to completion code */

} ITEMLIST;

/* Values for ITEMLIST -> flags */
enum itemlist_flags
{
  LIST_DYNAMIC = 0x001,
  LIST_DIRTY = 0x002,
  LIST_INITIALIZED = 0x004,
  LIST_MUSTSORT = 0x008,
  LIST_DONTFREE = 0x010,
  LIST_DONTFREEMEMBERS = 0x020
};

static const char *EMPTYCMD = "_EmptycmD_";
static const char *DEFAULTCMD = "_DefaultCmD_";
static const char *INITIALWORD = "_InitialWorD_";

#if 0
extern HASH_TABLE *prog_completes;

extern char *pcomp_line;
extern int pcomp_ind;

extern int prog_completion_enabled;
extern int progcomp_alias;

/* Not all of these are used yet. */
extern ITEMLIST it_aliases;
extern ITEMLIST it_arrayvars;
extern ITEMLIST it_bindings;
extern ITEMLIST it_builtins;
extern ITEMLIST it_commands;
extern ITEMLIST it_directories;
extern ITEMLIST it_disabled;
extern ITEMLIST it_enabled;
extern ITEMLIST it_exports;
extern ITEMLIST it_files;
extern ITEMLIST it_functions;
extern ITEMLIST it_groups;
extern ITEMLIST it_helptopics;
extern ITEMLIST it_hostnames;
extern ITEMLIST it_jobs;
extern ITEMLIST it_keywords;
extern ITEMLIST it_running;
extern ITEMLIST it_services;
extern ITEMLIST it_setopts;
extern ITEMLIST it_shopts;
extern ITEMLIST it_signals;
extern ITEMLIST it_stopped;
extern ITEMLIST it_users;
extern ITEMLIST it_variables;

extern COMPSPEC *pcomp_curcs;
extern const char *pcomp_curcmd;
#endif

/* Functions from pcomplib.c */
extern COMPSPEC *compspec_create (void);
extern void compspec_dispose (COMPSPEC *);
extern COMPSPEC *compspec_copy (COMPSPEC *);

extern void progcomp_create (void);
extern void progcomp_flush (void);
extern void progcomp_dispose (void);

extern int progcomp_size (void);

extern int progcomp_insert (char *, COMPSPEC *);
extern int progcomp_remove (char *);

extern COMPSPEC *progcomp_search (const std::string &);

extern void progcomp_walk (hash_wfunc *);

/* Functions from pcomplete.c */
extern void set_itemlist_dirty (ITEMLIST *);

extern STRINGLIST *completions_to_stringlist (const std::string &);

extern STRINGLIST *gen_compspec_completions (COMPSPEC *, const std::string &,
                                             const std::string &, int, int,
                                             int *);

extern char **programmable_completions (const std::string &,
                                        const std::string &, int, int, int *);

extern void pcomp_set_readline_variables (int, int);
extern void pcomp_set_compspec_options (COMPSPEC *, int, int);

} // namespace bash

#endif /* _PCOMPLETE_H_ */
