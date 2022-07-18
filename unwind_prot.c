/* unwind_prot.c - a simple unwind-protect system for internal variables */

/* I can't stand it anymore!  Please can't we just write the
   whole Unix system in lisp or something? */

/* Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

/* **************************************************************** */
/*								    */
/*		      Unwind Protection Scheme for Bash		    */
/*								    */
/* **************************************************************** */
#include "config.h"

#include "bashtypes.h"
#include "bashansi.h"

#if defined (HAVE_UNISTD_H)
#  include <unistd.h>
#endif

#if STDC_HEADERS
#  include <stddef.h>
#endif

#ifndef offsetof
#  define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

#include "command.h"
#include "general.h"
#include "unwind_prot.h"
#include "sig.h"
#include "quit.h"
#include "error.h"	/* for internal_warning */
#include "ocache.h"

/* Structure describing a saved variable and the value to restore it to.  */
typedef struct {
  void *variable;
  int size;
  char desired_setting[1]; /* actual size is `size' */
} SAVED_VAR;

/* Unwind element union with common header and enum type. */
 typedef enum { uwfn_void, uwfn_int, uwfn_ptr, uwfn_tag, uwfn_sv } uwp_type;

 typedef union uwp {
  struct uwp_head {
    union uwp *next;
    uwp_type type;
  } head;
  struct {
    struct uwp_head uwp_head;
    sh_voidfunc_t *func;
  } cleanup;
  struct {
    struct uwp_head uwp_head;
    sh_intfunc_t *func;
    int arg;
  } cleanup_int;
  struct {
    struct uwp_head uwp_head;
    sh_free_func_t *func;
    void *arg;
  } cleanup_ptr;
  struct {
    struct uwp_head uwp_head;
    const char *tag;
  } tag;
  struct {
    struct uwp_head uwp_head;
    SAVED_VAR v;
  } sv;
} UNWIND_ELT;

static void unwind_frame_discard_internal (const char *tag);
static void unwind_frame_run_internal (const char *tag);
static void remove_unwind_protect_internal (char *, char *);
static void run_unwind_protects_internal (char *, char *);
static void clear_unwind_protects_internal (bool);
static inline void restore_variable (SAVED_VAR *);
static void unwind_protect_mem_internal (const char *var, char *psize);

static UNWIND_ELT *unwind_protect_list = (UNWIND_ELT *)NULL;

/* Allocating from a cache of unwind-protect elements */
#define UWCACHESIZE	128

sh_obj_cache_t uwcache = {0, 0, 0};

#if 0
#define uwpalloc(elt)	(elt) = (UNWIND_ELT *)xmalloc (sizeof (UNWIND_ELT))
#define uwpfree(elt)	free(elt)
#else
#define uwpalloc(elt)	ocache_alloc (uwcache, UNWIND_ELT, elt)
#define uwpfree(elt)	ocache_free (uwcache, UNWIND_ELT, elt)
#endif

void
uwp_init ()
{
  ocache_create (uwcache, UNWIND_ELT, UWCACHESIZE);
}

/* Start the beginning of a region. */
void
begin_unwind_frame (const char *tag)
{
  UNWIND_ELT *elt;

  uwpalloc (elt);
  elt->head.next = unwind_protect_list;
  elt->head.type = uwfn_tag;
  elt->tag.tag = tag;
  unwind_protect_list = elt;
}

/* Discard the unwind protects back to TAG. */
void
discard_unwind_frame (const char *tag)
{
  if (unwind_protect_list)
    unwind_frame_discard_internal (tag);
}

/* Run the unwind protects back to TAG. */
void
run_unwind_frame (const char *tag)
{
  if (unwind_protect_list)
    unwind_frame_run_internal (tag);
}

/* Add the function CLEANUP to the list of unwindable things. */
void
add_unwind_protect (sh_voidfunc_t *cleanup)
{
  UNWIND_ELT *elt;

  uwpalloc (elt);
  elt->head.next = unwind_protect_list;
  elt->head.type = uwfn_void;
  elt->cleanup.func = cleanup;
  unwind_protect_list = elt;
}

/* Add the function CLEANUP with int ARG to the list of unwindable things. */
void
add_unwind_protect_int (sh_intfunc_t *cleanup, int arg)
{
  UNWIND_ELT *elt;

  uwpalloc (elt);
  elt->head.next = unwind_protect_list;
  elt->head.type = uwfn_int;
  elt->cleanup_int.func = cleanup;
  elt->cleanup_int.arg = arg;
  unwind_protect_list = elt;
}

/* Add the function CLEANUP with pointer ARG to the list of unwindable things. */
void
add_unwind_protect_ptr (sh_free_func_t *cleanup, void *arg)
{
  UNWIND_ELT *elt;

  uwpalloc (elt);
  elt->head.next = unwind_protect_list;
  elt->head.type = uwfn_ptr;
  elt->cleanup_ptr.func = cleanup;
  elt->cleanup_ptr.arg = arg;
  unwind_protect_list = elt;
}

/* Remove the top unwind protect from the list. */
void
remove_unwind_protect ()
{
  UNWIND_ELT *elt = unwind_protect_list;
  if (elt)
    {
      unwind_protect_list = elt->head.next;
      if (elt->head.type == uwfn_sv && (elt->sv.v.size +
		offsetof (UNWIND_ELT, sv.v.desired_setting[0])) > sizeof (UNWIND_ELT))
	xfree (elt);
      else
	uwpfree (elt);
    }
}

/* Run the list of cleanup functions in unwind_protect_list. */
void
run_unwind_protects ()
{
  if (unwind_protect_list)
    unwind_frame_run_internal (NULL);
}

/* Erase the unwind-protect list.  If free_mem is true, free the elements. */
void
clear_unwind_protect_list (bool free_mem)
{
  if (unwind_protect_list)
    clear_unwind_protects_internal (free_mem);
}

bool
have_unwind_protects ()
{
  return (unwind_protect_list != 0);
}

bool
unwind_protect_tag_on_stack (const char *tag)
{
  UNWIND_ELT *elt;

  elt = unwind_protect_list;
  while (elt)
    {
      if (elt->head.type == uwfn_tag && STREQ (elt->tag.tag, tag))
	return true;
      elt = elt->head.next;
    }
  return false;
}

/* **************************************************************** */
/*								    */
/*			The Actual Functions		 	    */
/*								    */
/* **************************************************************** */

static void
clear_unwind_protects_internal (bool free_mem)
{
  if (free_mem)
    {
      while (unwind_protect_list)
	remove_unwind_protect ();
    }
  unwind_protect_list = (UNWIND_ELT *)NULL;
}

static void
unwind_frame_discard_internal (const char *tag)
{
  bool found = false;
  while (UNWIND_ELT *elt = unwind_protect_list)
    {
      unwind_protect_list = unwind_protect_list->head.next;

      if (elt->head.type == uwfn_tag && (STREQ (elt->tag.tag, tag)))
	{
	  uwpfree (elt);
	  found = true;
	  break;
	}
      else if (elt->head.type == uwfn_sv && (elt->sv.v.size +
		offsetof (UNWIND_ELT, sv.v.desired_setting[0])) > sizeof (UNWIND_ELT))
	xfree (elt);
      else
	uwpfree (elt);
    }

  if (!found)
    internal_warning ("unwind_frame_discard: %s: frame not found", tag);
}

/* Restore the value of a variable, based on the contents of SV.
   sv->desired_setting is a block of memory SIZE bytes long holding the
   value itself.  This block of memory is copied back into the variable. */
static inline void
restore_variable (SAVED_VAR *sv)
{
  FASTCOPY (sv->desired_setting, sv->variable, sv->size);
}

static void
unwind_frame_run_internal (const char *tag)
{
  bool found = false;
  while (UNWIND_ELT *elt = unwind_protect_list)
    {
      unwind_protect_list = elt->head.next;

      switch (elt->head.type)
	{
	  case uwfn_void:
	    (*(elt->cleanup.func)) ();
	    break;

	  case uwfn_int:
	    (void) (*(elt->cleanup_int.func)) (elt->cleanup_int.arg);
	    break;

	  case uwfn_ptr:
	    (*(elt->cleanup_ptr.func)) (elt->cleanup_ptr.arg);
	    break;

	  case uwfn_tag:
	    if (tag && STREQ (elt->tag.tag, tag))
	      {
		uwpfree (elt);
		found = true;
		goto exit;
	      }
	    break;

	  case uwfn_sv:
	    restore_variable (&elt->sv.v);
	    /* Free element directly if it's larger than the standard size. */
	    if ((elt->sv.v.size + offsetof (UNWIND_ELT, sv.v.desired_setting[0])) > sizeof (UNWIND_ELT))
	      {
		xfree (elt);
		continue;	/* skips over uwpfree() to go to next loop */
	      }
	    break;
	}

      uwpfree (elt);
    }

exit:
  if (tag && !found)
    internal_warning ("unwind_frame_run: %s: frame not found", tag);
}

void
unwind_protect_mem (void *var, size_t size)
{
  UNWIND_ELT *elt;

  size_t allocated = size + offsetof (UNWIND_ELT, sv.v.desired_setting[0]);
  if (allocated <= sizeof (UNWIND_ELT))
    uwpalloc (elt);
  else
    elt = (UNWIND_ELT *)xmalloc (allocated);

  elt->head.next = unwind_protect_list;
  elt->head.type = uwfn_sv;
  elt->sv.v.variable = var;
  elt->sv.v.size = size;
  FASTCOPY (var, elt->sv.v.desired_setting, size);
  unwind_protect_list = elt;
}

#if defined (DEBUG)
#include <stdio.h>

void
print_unwind_protect_tags ()
{
  UNWIND_ELT *elt;

  elt = unwind_protect_list;
  while (elt)
    {
      if (elt->head.type == uwfn_tag)
        fprintf(stderr, "tag: %s\n", elt->tag.tag);
      elt = elt->head.next;
    }
}
#endif
