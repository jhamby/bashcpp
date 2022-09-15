/* trap.h -- data structures used in the trap mechanism. */

/* Copyright (C) 1993-2013 Free Software Foundation, Inc.

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

#if !defined(_TRAP_H_)
#define _TRAP_H_

#include "bashtypes.hh"

#include <csignal>

namespace bash
{

#if !defined(NSIG)
#define NSIG 64
#endif /* !NSIG */

#define NO_SIG -1
#define DEFAULT_SIG SIG_DFL
#define IGNORE_SIG SIG_IGN

/* Special shell trap names. */
#define DEBUG_TRAP NSIG
#define ERROR_TRAP NSIG + 1
#define RETURN_TRAP NSIG + 2
#define EXIT_TRAP 0

/* system signals plus special bash traps */
#define BASH_NSIG NSIG + 3

/* Flags values for decode_signal() */
enum decode_signal_flags
{
  DSIG_NOFLAGS = 0,
  DSIG_SIGPREFIX = 0x01, /* don't allow `SIG' PREFIX */
  DSIG_NOCASE = 0x02     /* case-insensitive comparison */
};

int decode_signal (const char *, decode_signal_flags);

} // namespace bash

#endif /* _TRAP_H_ */
