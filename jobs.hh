/* jobs.h -- structures and definitions used by the jobs.c file. */

/* Copyright (C) 1993-2019  Free Software Foundation, Inc.

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

#if !defined (_JOBS_H_)
#  define _JOBS_H_

#include "quit.hh"
#include "siglist.hh"

#include "posixwait.hh"

namespace bash
{

/* Defines controlling the fashion in which jobs are listed. */
#define JLIST_STANDARD       0
#define JLIST_LONG	     1
#define JLIST_PID_ONLY	     2
#define JLIST_CHANGED_ONLY   3
#define JLIST_NONINTERACTIVE 4

/* I looked it up.  For pretty_print_job ().  The real answer is 24. */
#define LONGEST_SIGNAL_DESC 24

/* Defines for the wait_for_* functions and for the wait builtin to use */
#define JWAIT_PERROR		(1 << 0)
#define JWAIT_FORCE		(1 << 1)
#define JWAIT_NOWAIT		(1 << 2) /* don't waitpid(), just return status if already exited */
#define JWAIT_WAITING		(1 << 3) /* wait for jobs marked J_WAITING only */

/* flags for wait_for */
#define JWAIT_NOTERM		(1 << 8) /* wait_for doesn't give terminal away */

/* The max time to sleep while retrying fork() on EAGAIN failure */
#define FORKSLEEP_MAX	16

/* We keep an array of jobs.  Each entry in the array is a linked list
   of processes that are piped together.  The first process encountered is
   the group leader. */

/* Values for the `running' field of a struct process. */
#define PS_DONE		0
#define PS_RUNNING	1
#define PS_STOPPED	2
#define PS_RECYCLED	4

/* Each child of the shell is remembered in a STRUCT PROCESS.  A circular
   chain of such structures is a pipeline. */
struct PROCESS {
  char *command;	/* The particular program that is running. */
  pid_t pid;		/* Process ID. */
  WAIT status;		/* The status of this command as returned by wait. */
  int running;		/* Non-zero if this process is running. */
  int _pad;		// silence clang -Wpadded warning
};

/* PALIVE really means `not exited' */
#define PSTOPPED(p)	(WIFSTOPPED((p)->status))
#define PRUNNING(p)	((p)->running == PS_RUNNING)
#define PALIVE(p)	(PRUNNING(p) || PSTOPPED(p))

#define PEXITED(p)	((p)->running == PS_DONE)
#if defined (RECYCLES_PIDS)
#  define PRECYCLED(p)	((p)->running == PS_RECYCLED)
#else
#  define PRECYCLED(p)	(0)
#endif
#define PDEADPROC(p)	(PEXITED(p) || PRECYCLED(p))

#define get_job_by_jid(ind)	(jobs[(ind)])

/* A description of a pipeline's state. */
enum JOB_STATE {
  JNONE = -1,
  JRUNNING = 1,
  JSTOPPED = 2,
  JDEAD = 4,
  JMIXED = 8
};

#define JOBSTATE(job)	(jobs[(job)]->state)
#define J_JOBSTATE(j)	((j)->state)

#define STOPPED(j)	(jobs[(j)]->state == JSTOPPED)
#define RUNNING(j)	(jobs[(j)]->state == JRUNNING)
#define DEADJOB(j)	(jobs[(j)]->state == JDEAD)

#define INVALID_JOB(j)	((j) < 0 || (j) >= js.j_jobslots || get_job_by_jid(j) == 0)

/* Values for the FLAGS field in the JOB struct below. */
enum job_flags {
  J_NOFLAGS =		   0,
  J_FOREGROUND =	0x01, /* Non-zero if this is running in the foreground.  */
  J_NOTIFIED =		0x02, /* Non-zero if already notified about job state.   */
  J_JOBCONTROL =	0x04, /* Non-zero if this job started under job control. */
  J_NOHUP =		0x08, /* Don't send SIGHUP to job if shell gets SIGHUP. */
  J_STATSAVED =		0x10, /* A process in this job had status saved via $! */
  J_ASYNC =		0x20, /* Job was started asynchronously */
  J_PIPEFAIL =		0x40, /* pipefail set when job was started */
  J_WAITING =		0x80 /* one of a list of jobs for which we are waiting */
};

#define IS_FOREGROUND(j)	((jobs[j]->flags & J_FOREGROUND) != 0)
#define IS_NOTIFIED(j)		((jobs[j]->flags & J_NOTIFIED) != 0)
#define IS_JOBCONTROL(j)	((jobs[j]->flags & J_JOBCONTROL) != 0)
#define IS_ASYNC(j)		((jobs[j]->flags & J_ASYNC) != 0)
#define IS_WAITING(j)		((jobs[j]->flags & J_WAITING) != 0)

/* Revised to accommodate new hash table bgpids implementation. */
typedef pid_t ps_index_t;

struct pidstat {
  ps_index_t bucket_next;
  ps_index_t bucket_prev;

  pid_t pid;
  bits16_t status;		/* only 8 bits really needed */
  char _pad[2];			// silence clang -Wpadded warning
};

struct bgpids {
  struct pidstat *storage;	/* storage arena */

  ps_index_t head;
  ps_index_t nalloc;

  int npid;
  int _pad;			// silence clang -Wpadded warning
};

#define NO_PIDSTAT (static_cast<ps_index_t> (-1))

/* standalone process status struct, without bgpids indexes */
struct procstat {
  pid_t pid;
  bits16_t status;
};

/* A standalone linked list of PROCESS *, used in various places
   including keeping track of process substitutions. */
typedef std::list<PROCESS> procchain;

/* A value which cannot be a process ID. */
#define NO_PID (static_cast<pid_t> (-1))

/* A value representing any process ID. */
#define ANY_PID (static_cast<pid_t> (-1))

/* flags for make_child () */
enum make_child_flags {
  FORK_SYNC =	0,		/* normal synchronous process */
  FORK_ASYNC =	1,		/* background process */
  FORK_NOJOB =	2,		/* don't put process in separate pgrp */
  FORK_NOTERM =	4		/* don't give terminal to any pgrp */
};

}  // namespace bash

#endif /* _JOBS_H_ */
