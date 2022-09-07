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

#if !defined (_PCOMPLETE_H_)
#  define _PCOMPLETE_H_

#include "hashlib.h"

namespace bash
{

struct COMPSPEC {
  char *globpat;
  char *words;
  char *prefix;
  char *suffix;
  char *funcname;
  char *command;
  char *lcommand;
  char *filterpat;
  uint32_t refcount;
  uint32_t actions;
  uint32_t options;
};

/* Values for COMPSPEC actions.  These are things the shell knows how to
   build internally. */
const uint32_t CA_ALIAS =		(1<<0);
const uint32_t CA_ARRAYVAR =		(1<<1);
const uint32_t CA_BINDING =		(1<<2);
const uint32_t CA_BUILTIN =		(1<<3);
const uint32_t CA_COMMAND =		(1<<4);
const uint32_t CA_DIRECTORY =		(1<<5);
const uint32_t CA_DISABLED =		(1<<6);
const uint32_t CA_ENABLED =		(1<<7);
const uint32_t CA_EXPORT =		(1<<8);
const uint32_t CA_FILE =		(1<<9);
const uint32_t CA_FUNCTION =		(1<<10);
const uint32_t CA_GROUP =		(1<<11);
const uint32_t CA_HELPTOPIC =		(1<<12);
const uint32_t CA_HOSTNAME =		(1<<13);
const uint32_t CA_JOB =			(1<<14);
const uint32_t CA_KEYWORD =		(1<<15);
const uint32_t CA_RUNNING =		(1<<16);
const uint32_t CA_SERVICE =		(1<<17);
const uint32_t CA_SETOPT =		(1<<18);
const uint32_t CA_SHOPT =		(1<<19);
const uint32_t CA_SIGNAL =		(1<<20);
const uint32_t CA_STOPPED =		(1<<21);
const uint32_t CA_USER =		(1<<22);
const uint32_t CA_VARIABLE =		(1<<23);

/* Values for COMPSPEC options field. */
const uint32_t COPT_RESERVED =		(1<<0);		/* reserved for other use */
const uint32_t COPT_DEFAULT =		(1<<1);
const uint32_t COPT_FILENAMES =		(1<<2);
const uint32_t COPT_DIRNAMES =		(1<<3);
const uint32_t COPT_NOQUOTE =		(1<<4);
const uint32_t COPT_NOSPACE =		(1<<5);
const uint32_t COPT_BASHDEFAULT =	(1<<6);
const uint32_t COPT_PLUSDIRS =		(1<<7);
const uint32_t COPT_NOSORT =		(1<<8);

const uint32_t COPT_LASTUSER =		COPT_NOSORT;

const uint32_t PCOMP_RETRYFAIL =	(COPT_LASTUSER << 1);
const uint32_t PCOMP_NOTFOUND =		(COPT_LASTUSER << 2);


/* List of items is used by the code that implements the programmable
   completions. */
typedef struct _list_of_items {
  uint32_t flags;
  int (*list_getter) (struct _list_of_items *);	/* function to call to get the list */

  STRINGLIST *slist;

  /* These may or may not be used. */
  STRINGLIST *genlist;	/* for handing to the completion code one item at a time */
  int genindex;		/* index of item last handed to completion code */

} ITEMLIST;

/* Values for ITEMLIST -> flags */
const uint32_t LIST_DYNAMIC =		0x001;
const uint32_t LIST_DIRTY =		0x002;
const uint32_t LIST_INITIALIZED =	0x004;
const uint32_t LIST_MUSTSORT =		0x008;
const uint32_t LIST_DONTFREE =		0x010;
const uint32_t LIST_DONTFREEMEMBERS =	0x020;

const char *EMPTYCMD =		"_EmptycmD_";
const char *DEFAULTCMD =	"_DefaultCmD_";
const char *INITIALWORD =	"_InitialWorD_";

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

extern STRINGLIST *
gen_compspec_completions (COMPSPEC *, const std::string &, const std::string &, int, int, int *);

extern char **programmable_completions (const std::string &, const std::string &, int, int, int *);

extern void pcomp_set_readline_variables (int, int);
extern void pcomp_set_compspec_options (COMPSPEC *, int, int);

}  // namespace bash

#endif /* _PCOMPLETE_H_ */
