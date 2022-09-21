/* Copyright (C) 1999-2020 Free Software Foundation, Inc. */

/* This file is part of GNU Bash, the Bourne Again SHell.

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

/*
 * shtty.h -- include the correct system-dependent files to manipulate the
 *	      tty
 */

#ifndef __SH_TTY_H_
#define __SH_TTY_H_

#if defined(_POSIX_VERSION) && defined(HAVE_TERMIOS_H)                        \
    && defined(HAVE_TCGETATTR) && !defined(TERMIOS_MISSING)
#define TERMIOS_TTY_DRIVER
#else
#if defined(HAVE_TERMIO_H)
#define TERMIO_TTY_DRIVER
#else
#define NEW_TTY_DRIVER
#endif
#endif

/*
 * The _POSIX_SOURCE define is to avoid multiple symbol definitions
 * between sys/ioctl.h and termios.h.  Ditto for the test against SunOS4
 * and the undefining of several symbols.
 */

#ifdef TERMIOS_TTY_DRIVER
#if (defined(SunOS4) || defined(SunOS5)) && !defined(_POSIX_SOURCE)
#define _POSIX_SOURCE
#endif
#if defined(SunOS4)
#undef ECHO
#undef NOFLSH
#undef TOSTOP
#endif /* SunOS4 */
#include <termios.h>
#define TTYSTRUCT struct termios
#else
#ifdef TERMIO_TTY_DRIVER
#include <termio.h>
#define TTYSTRUCT struct termio
#else /* NEW_TTY_DRIVER */
#include <sgtty.h>
#define TTYSTRUCT struct sgttyb
#endif
#endif

#endif