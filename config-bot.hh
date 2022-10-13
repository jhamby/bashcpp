/* config-bot.h */
/* modify settings or make new ones based on what autoconf tells us. */

/* Copyright (C) 1989-2021 Free Software Foundation, Inc.

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

/*********************************************************/
/* Modify or set defines based on the configure results. */
/*********************************************************/

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmacro-redefined"
#endif

#if defined(HAVE_SYS_RESOURCE_H) && defined(HAVE_GETRLIMIT)
#define HAVE_RESOURCE
#endif

#define PREFER_STDARG
#define USE_VARARGS

#if defined(HAVE_SYS_SOCKET_H) && defined(HAVE_GETPEERNAME)                   \
    && defined(HAVE_NETINET_IN_H)
#define HAVE_NETWORK
#endif

#if defined(HAVE_REGEX_H) && defined(HAVE_REGCOMP) && defined(HAVE_REGEXEC)
#define HAVE_POSIX_REGEXP
#endif

/***********************************************************************/
/* Unset defines based on what configure reports as missing or broken. */
/***********************************************************************/

#if !defined(HAVE_DEV_FD) && defined(NAMED_PIPES_MISSING)
#undef PROCESS_SUBSTITUTION
#endif

#if defined(JOB_CONTROL_MISSING)
#undef JOB_CONTROL
#endif

#if !defined(HAVE_POSIX_REGEXP)
#undef COND_REGEXP
#endif

#if !defined(HAVE_MKSTEMP)
#undef USE_MKSTEMP
#endif

#if !defined(HAVE_MKDTEMP)
#undef USE_MKDTMP
#endif

/* If the shell is called by this name, it will become restricted. */
#if defined(RESTRICTED_SHELL)
#define RESTRICTED_SHELL_NAME "rbash"
#endif

/***********************************************************/
/* Make sure feature defines have necessary prerequisites. */
/***********************************************************/

/* BANG_HISTORY requires HISTORY. */
#if defined(BANG_HISTORY) && !defined(HISTORY)
#define HISTORY
#endif /* BANG_HISTORY && !HISTORY */

#if defined(READLINE) && !defined(HISTORY)
#define HISTORY
#endif

#if defined(PROGRAMMABLE_COMPLETION) && !defined(READLINE)
#undef PROGRAMMABLE_COMPLETION
#endif

#if !defined(V9_ECHO)
#undef DEFAULT_ECHO_TO_XPG
#endif

#if !defined(PROMPT_STRING_DECODE)
#undef PPROMPT
#define PPROMPT "$ "
#endif

#if !defined(HAVE_SYSLOG) || !defined(HAVE_SYSLOG_H)
#undef SYSLOG_HISTORY
#endif

#if !defined(PREFER_POSIX_SPAWN) && !defined(HAVE_FORK)
#define PREFER_POSIX_SPAWN
#elif defined(PREFER_POSIX_SPAWN) && !defined(HAVE_VFORK)                     \
    && (!defined(POSIX_SPAWN) || !defined(POSIX_SPAWNATTR_INIT)               \
        || !defined(POSIX_SPAWN_FILE_ACTIONS_INIT))
#undef PREFER_POSIX_SPAWN
#endif

/************************************************/
/* check multibyte capability for I18N code	*/
/************************************************/

/* For platforms which support the ISO C amendment 1 functionality we
   support user defined character classes.  */
#include <cwchar>
#include <cwctype>

/* If we don't want multibyte chars even on a system that supports them, let
   the configuring user turn multibyte support off. */
#if defined(NO_MULTIBYTE_SUPPORT)
#undef HANDLE_MULTIBYTE
#endif

/************************************************/
/* end of multibyte capability checks for I18N	*/
/************************************************/

/******************************************************************/
/* Placeholder for builders to #undef any unwanted features from  */
/* config-top.h or created by configure (such as the default mail */
/* file for mail checking).					  */
/******************************************************************/

/* If you don't want bash to provide a default mail file to check. */
/* #undef DEFAULT_MAIL_DIRECTORY */

#if defined(__clang__)
#pragma clang diagnostic pop
#endif
