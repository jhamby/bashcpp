dnl
dnl Configure script for bashcpp-5.1
dnl
dnl report bugs to jake.hamby@gmail.com
dnl
dnl Process this file with autoconf to produce a configure script.

# Copyright (C) 1987-2020 Free Software Foundation, Inc.
# Copyright 2022, Jake Hamby.

#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

AC_REVISION([for Bash 5.1, version 5.022-cpp])dnl

define(bashvers, 5.1)
define(relstatus, dev)

dnl make sure we are using a recent autoconf version
AC_PREREQ([2.69])

AC_CONFIG_SRCDIR(siglist.cc)

dnl where to find install.sh, config.sub, and config.guess
AC_CONFIG_AUX_DIR(support)
AC_CONFIG_HEADERS(config.hh)
AC_CONFIG_MACRO_DIRS([m4])

AC_INIT([bash],[bashvers-relstatus],[jake.hamby@gmail.com])
AM_INIT_AUTOMAKE([subdir-objects])

dnl checks for version info
BASHVERS=bashvers
RELSTATUS=relstatus

dnl defaults for debug settings
case "$RELSTATUS" in
alp*|bet*|dev*|rc*|releng*|maint*)	opt_debug=yes ;;
*)	opt_debug=no ;;
esac

dnl configure defaults
opt_curses=no
opt_with_posix_spawn=no

dnl
dnl macros for the bash debugger
dnl
dnl AM_PATH_LISPDIR
AC_ARG_VAR(DEBUGGER_START_FILE, [location of bash debugger initialization file])

dnl autoheader support
AH_TOP([#include "config-top.hh"])
AH_BOTTOM([#include "config-bot.hh"])

dnl arguments to configure
dnl packages

AC_ARG_WITH(curses, AS_HELP_STRING([--with-curses],[use the curses library instead of the termcap library]), opt_curses=$withval)

if test "$opt_curses" = yes; then
	prefer_curses=yes
fi

if test -z "${DEBUGGER_START_FILE}"; then
	DEBUGGER_START_FILE='${datadir}/bashdb/bashdb-main.inc'
fi

dnl optional shell features in config.h.in
opt_minimal_config=no

opt_job_control=yes
opt_alias=yes
opt_readline=yes
opt_history=yes
opt_bang_history=yes
opt_dirstack=yes
opt_restricted=yes
opt_process_subst=yes
opt_prompt_decoding=yes
opt_select=yes
opt_help=yes
opt_array_variables=yes
opt_dparen_arith=yes
opt_extended_glob=yes
opt_brace_expansion=yes
opt_disabled_builtins=no
opt_command_timing=yes
opt_xpg_echo=no
opt_strict_posix=no
opt_cond_command=yes
opt_cond_regexp=yes
opt_coproc=yes
opt_arith_for_command=yes
opt_net_redirs=yes
opt_progcomp=yes
opt_separate_help=no
opt_multibyte=yes
opt_debugger=yes
opt_single_longdoc_strings=yes
opt_casemod_attrs=yes
opt_casemod_expansions=yes
opt_extglob_default=no
opt_dircomplete_expand_default=no
opt_globascii_default=yes
opt_function_import=yes
opt_dev_fd_stat_broken=no
opt_prefer_posix_spawn=yes

dnl options that affect how bash is compiled and linked
opt_static_link=no
opt_profiling=no

dnl argument parsing for optional features
AC_ARG_ENABLE(minimal-config, AS_HELP_STRING([--enable-minimal-config],[a minimal sh-like configuration]), opt_minimal_config=$enableval)

dnl a minimal configuration turns everything off, but features can be
dnl added individually
if test $opt_minimal_config = yes; then
	opt_job_control=no opt_alias=no opt_readline=no
	opt_history=no opt_bang_history=no opt_dirstack=no
	opt_restricted=no opt_process_subst=no opt_prompt_decoding=no
	opt_select=no opt_help=no opt_array_variables=no opt_dparen_arith=no
	opt_brace_expansion=no opt_disabled_builtins=no opt_command_timing=no
	opt_extended_glob=no opt_cond_command=no opt_arith_for_command=no
	opt_net_redirs=no opt_progcomp=no opt_separate_help=no
	opt_multibyte=yes opt_cond_regexp=no opt_coproc=no
	opt_casemod_attrs=no opt_casemod_expansions=no opt_extglob_default=no
	opt_globascii_default=yes
fi

AC_ARG_ENABLE(alias, AS_HELP_STRING([--enable-alias],[enable shell aliases]), opt_alias=$enableval)
AC_ARG_ENABLE(arith-for-command, AS_HELP_STRING([--enable-arith-for-command],[enable arithmetic for command]), opt_arith_for_command=$enableval)
AC_ARG_ENABLE(array-variables, AS_HELP_STRING([--enable-array-variables],[include shell array variables]), opt_array_variables=$enableval)
AC_ARG_ENABLE(bang-history, AS_HELP_STRING([--enable-bang-history],[turn on csh-style history substitution]), opt_bang_history=$enableval)
AC_ARG_ENABLE(brace-expansion, AS_HELP_STRING([--enable-brace-expansion],[include brace expansion]), opt_brace_expansion=$enableval)
AC_ARG_ENABLE(casemod-attributes, AS_HELP_STRING([--enable-casemod-attributes],[include case-modifying variable attributes]), opt_casemod_attrs=$enableval)
AC_ARG_ENABLE(casemod-expansions, AS_HELP_STRING([--enable-casemod-expansions],[include case-modifying word expansions]), opt_casemod_expansions=$enableval)
AC_ARG_ENABLE(command-timing, AS_HELP_STRING([--enable-command-timing],[enable the time reserved word and command timing]), opt_command_timing=$enableval)
AC_ARG_ENABLE(cond-command, AS_HELP_STRING([--enable-cond-command],[enable the conditional command]), opt_cond_command=$enableval)
AC_ARG_ENABLE(cond-regexp, AS_HELP_STRING([--enable-cond-regexp],[enable extended regular expression matching in conditional commands]), opt_cond_regexp=$enableval)
AC_ARG_ENABLE(coprocesses, AS_HELP_STRING([--enable-coprocesses],[enable coprocess support and the coproc reserved word]), opt_coproc=$enableval)
AC_ARG_ENABLE(debugger, AS_HELP_STRING([--enable-debugger],[enable support for bash debugger]), opt_debugger=$enableval)
AC_ARG_ENABLE(dev-fd-stat-broken, AS_HELP_STRING([--enable-dev-fd-stat-broken],[enable this option if stat on /dev/fd/N and fstat on file descriptor N don't return the same results]), opt_dev_fd_stat_broken=$enableval)
AC_ARG_ENABLE(direxpand-default, AS_HELP_STRING([--enable-direxpand-default],[enable the direxpand shell option by default]), opt_dircomplete_expand_default=$enableval)
AC_ARG_ENABLE(directory-stack, AS_HELP_STRING([--enable-directory-stack],[enable builtins pushd/popd/dirs]), opt_dirstack=$enableval)
AC_ARG_ENABLE(disabled-builtins, AS_HELP_STRING([--enable-disabled-builtins],[allow disabled builtins to still be invoked]), opt_disabled_builtins=$enableval)
AC_ARG_ENABLE(dparen-arithmetic, AS_HELP_STRING([--enable-dparen-arithmetic],[include ((...)) command]), opt_dparen_arith=$enableval)
AC_ARG_ENABLE(extended-glob, AS_HELP_STRING([--enable-extended-glob],[include ksh-style extended pattern matching]), opt_extended_glob=$enableval)
AC_ARG_ENABLE(extended-glob-default, AS_HELP_STRING([--enable-extended-glob-default],[force extended pattern matching to be enabled by default]), opt_extglob_default=$enableval)
AC_ARG_ENABLE(function-import, AS_HELP_STRING([--enable-function-import],[allow bash to import exported function definitions by default]), opt_function_import=$enableval)
AC_ARG_ENABLE(glob-asciiranges-default, AS_HELP_STRING([--enable-glob-asciiranges-default],[force bracket range expressions in pattern matching to use the C locale by default]), opt_globascii_default=$enableval)
AC_ARG_ENABLE(help-builtin, AS_HELP_STRING([--enable-help-builtin],[include the help builtin]), opt_help=$enableval)
AC_ARG_ENABLE(history, AS_HELP_STRING([--enable-history],[turn on command history]), opt_history=$enableval)
AC_ARG_ENABLE(job-control, AS_HELP_STRING([--enable-job-control],[enable job control features]), opt_job_control=$enableval)
AC_ARG_ENABLE(multibyte, AS_HELP_STRING([--enable-multibyte],[enable multibyte characters if OS supports them]), opt_multibyte=$enableval)
AC_ARG_ENABLE(net-redirections, AS_HELP_STRING([--enable-net-redirections],[enable /dev/tcp/host/port redirection]), opt_net_redirs=$enableval)
AC_ARG_ENABLE(process-substitution, AS_HELP_STRING([--enable-process-substitution],[enable process substitution]), opt_process_subst=$enableval)
AC_ARG_ENABLE(progcomp, AS_HELP_STRING([--enable-progcomp],[enable programmable completion and the complete builtin]), opt_progcomp=$enableval)
AC_ARG_ENABLE(prompt-string-decoding, AS_HELP_STRING([--enable-prompt-string-decoding],[turn on escape character decoding in prompts]), opt_prompt_decoding=$enableval)
AC_ARG_ENABLE(readline, AS_HELP_STRING([--enable-readline],[turn on command line editing]), opt_readline=$enableval)
AC_ARG_ENABLE(restricted, AS_HELP_STRING([--enable-restricted],[enable a restricted shell]), opt_restricted=$enableval)
AC_ARG_ENABLE(select, AS_HELP_STRING([--enable-select],[include select command]), opt_select=$enableval)
AC_ARG_ENABLE(separate-helpfiles, AS_HELP_STRING([--enable-separate-helpfiles],[use external files for help builtin documentation]), opt_separate_help=$enableval)
AC_ARG_ENABLE(single-help-strings, AS_HELP_STRING([--enable-single-help-strings],[store help documentation as a single string to ease translation]), opt_single_longdoc_strings=$enableval)
AC_ARG_ENABLE(strict-posix-default, AS_HELP_STRING([--enable-strict-posix-default],[configure bash to be posix-conformant by default]), opt_strict_posix=$enableval)
AC_ARG_ENABLE(usg-echo-default, AS_HELP_STRING([--enable-usg-echo-default],[a synonym for --enable-xpg-echo-default]), opt_xpg_echo=$enableval)
AC_ARG_ENABLE(xpg-echo-default, AS_HELP_STRING([--enable-xpg-echo-default],[make the echo builtin expand escape sequences by default]), opt_xpg_echo=$enableval)

dnl options that alter how bash is compiled and linked
AC_ARG_ENABLE(profiling, AS_HELP_STRING([--enable-profiling],[allow profiling with gprof]), opt_profiling=$enableval)
AC_ARG_ENABLE(static-link, AS_HELP_STRING([--enable-static-link],[link bash statically, for use as a root shell]), opt_static_link=$enableval)

dnl this is for my future posix_spawn () refactoring work.
AC_ARG_ENABLE(prefer-posix-spawn, AS_HELP_STRING([--enable-prefer-posix-spawn],[prefer posix_spawn or vfork to fork when both are available]), opt_prefer_posix_spawn=$enableval)

dnl So-called `precious' variables
AC_ARG_VAR([CXX_FOR_BUILD], [C++ compiler used when compiling binaries used only at build time])
AC_ARG_VAR([CXXFLAGS_FOR_BUILD], [Compilation options (CXXFLAGS) used when compiling binaries used only at build time])
AC_ARG_VAR([LDFLAGS_FOR_BUILD], [Linker options (LDFLAGS) used when compiling binaries used only at build time])
AC_ARG_VAR([CPPFLAGS_FOR_BUILD], [C preprocessor options (CPPFLAGS) used when compiling binaries used only at build time])

dnl opt_job_control is handled later, after BASH_JOB_CONTROL_MISSING runs

dnl opt_readline and opt_history are handled later, because AC_PROG_CC needs
dnl to be run before we can check the version of an already-installed readline
dnl library

if test $opt_alias = yes; then
AC_DEFINE([ALIAS], [1], [Define if you want the alias feature.])
fi
if test $opt_dirstack = yes; then
AC_DEFINE([PUSHD_AND_POPD], [1], [Define if you want pushd, popd, and dirs commands.])
fi
if test $opt_restricted = yes; then
AC_DEFINE([RESTRICTED_SHELL], [1], [Define if you want restricted shell support ("rbash" or the -r flag).])
fi
if test $opt_process_subst = yes; then
AC_DEFINE([PROCESS_SUBSTITUTION], [1], [Define if you want ksh-like process substitution features with "<(file)". Requires FIFO support.])
fi
if test $opt_prompt_decoding = yes; then
AC_DEFINE([PROMPT_STRING_DECODE], [1], [Define if you want to expand backslash-escaped special characters in PS1 and PS2. Variable expansion will still be performed.])
fi
if test $opt_select = yes; then
AC_DEFINE([SELECT_COMMAND], [1], [Define if you want the ksh-style `select' command: select word in word_list; do command_list; done.])
fi
if test $opt_help = yes; then
AC_DEFINE([HELP_BUILTIN], [1], [Define if you want the `help' shell builtin and long documentation strings compiled into the shell.])
fi
if test $opt_array_variables = yes; then
AC_DEFINE([ARRAY_VARS], [1], [Define if you want ksh-style one-dimensional array variables.])
fi
if test $opt_dparen_arith = yes; then
AC_DEFINE([DPAREN_ARITHMETIC], [1], [Define if you want the ksh-style ((...)) arithmetic evaluation command.])
fi
if test $opt_brace_expansion = yes; then
AC_DEFINE([BRACE_EXPANSION], [1], [Define if you want csh-style curly brace expansion: foo{a,b} -> fooa foob. This feature can be turned off at shell startup with `-nobraceexpansion', or during execution with `set +o braceexpand'.])
fi
if test $opt_disabled_builtins = yes; then
AC_DEFINE([DISABLED_BUILTINS], [1], [Define if you want "builtin foo" to always run the shell builtin "foo", even if it has been disabled with "enable -n foo".])
fi
if test $opt_command_timing = yes; then
AC_DEFINE([COMMAND_TIMING], [1], [Define if you want the ksh-style `time' reserved word and the ability to time pipelines, functions, and builtins.])
fi
if test $opt_xpg_echo = yes ; then
AC_DEFINE([DEFAULT_ECHO_TO_XPG], [1], [Define if you want the echo builtin to interpret the backslash-escape characters by default, like the XPG Single Unix Spec V2 for echo. This requires V9_ECHO to be defined.])
fi
if test $opt_strict_posix = yes; then
AC_DEFINE([STRICT_POSIX], [1], [Define if you want bash to be strictly POSIX.2 conformant by default (except for echo, which is controlled separately).])
fi
if test $opt_extended_glob = yes ; then
AC_DEFINE([EXTENDED_GLOB], [1], [Define if you want ksh-style [*+@?!](patlist) extended pattern matching.])
fi
if test $opt_extglob_default = yes; then
AC_DEFINE([EXTGLOB_DEFAULT], [1], [Define to the value you'd like the extglob shell option to have by default.])
else
AC_DEFINE([EXTGLOB_DEFAULT], [0], [Define to the value you'd like the extglob shell option to have by default.])
fi
if test $opt_cond_command = yes ; then
AC_DEFINE([COND_COMMAND], [1], [Define if you want the ksh-style [[...]] conditional command.])
fi
if test $opt_cond_regexp = yes ; then
AC_DEFINE([COND_REGEXP], [1], [Define if you want extended regular expression matching and the =~ binary operator in the [[..]] conditional command.])
fi
if test $opt_coproc = yes; then
AC_DEFINE([COPROCESS_SUPPORT], [1], [Define if you want support for ksh-like coprocesses and the `coproc' reserved word.])
fi
if test $opt_arith_for_command = yes; then
AC_DEFINE([ARITH_FOR_COMMAND], [1], [Define if you want the ksh93-style for (( init; test; step )) do list; done arithmetic for command.])
fi
if test $opt_net_redirs = yes; then
AC_DEFINE([NETWORK_REDIRECTIONS], [1], [Define if you want /dev/(tcp|udp)/host/port to open socket connections when used in redirections.])
fi
if test $opt_progcomp = yes; then
AC_DEFINE([PROGRAMMABLE_COMPLETION], [1], [Define for the programmable completion features and the complete builtin.])
fi
if test $opt_multibyte = yes; then
AC_DEFINE([HANDLE_MULTIBYTE], [1], [Define to compile in support for multibyte characters, or undefine to remove this support.])
fi
if test $opt_debugger = yes; then
AC_DEFINE([DEBUGGER], [1], [Define if you want to compile in some features used only by the bash debugger.])
fi
if test $opt_casemod_attrs = yes; then
AC_DEFINE([CASEMOD_ATTRS], [1], [Define for case-modifying variable attributes; variables modified on assignment.])
fi
if test $opt_casemod_expansions = yes; then
AC_DEFINE([CASEMOD_EXPANSIONS], [1], [Define for case-modifying word expansions.])
fi
if test $opt_dircomplete_expand_default = yes; then
AC_DEFINE([DIRCOMPLETE_EXPAND_DEFAULT], [1], [Define to make the `direxpand' shopt option enabled by default.])
fi
if test $opt_globascii_default = yes; then
AC_DEFINE([GLOBASCII_DEFAULT], [1], [Define to 1 to make the `globasciiranges' shopt option enabled by default.])
else
AC_DEFINE([GLOBASCII_DEFAULT], [0], [Define to 1 to make the `globasciiranges' shopt option enabled by default.])
fi
if test $opt_function_import = yes; then
AC_DEFINE([FUNCTION_IMPORT], [1], [Define to allow functions to be imported from the environment.])
fi
if test $opt_dev_fd_stat_broken = yes; then
AC_DEFINE([DEV_FD_STAT_BROKEN], [1], [Define if stat on /dev/fd/N and fstat on file descriptor N don't return the same results.])
fi

if test $opt_prefer_posix_spawn = yes; then
AC_DEFINE([PREFER_POSIX_SPAWN], [1], [Define to prefer posix_spawn or vfork to fork when both are available.])
fi

if test $opt_debug = yes; then
AC_DEFINE([DEBUG], [1], [Define for extra debugging checks (enabled for non-release builds).])
fi

if test "$opt_minimal_config" = yes; then
	TESTSCRIPT=run-minimal
else
	TESTSCRIPT=run-all
fi

HELPDIR= HELPDIRDEFINE= HELPINSTALL= HELPFILES_TARGET=
if test "$opt_separate_help" != no; then
	if test "$opt_separate_help" = "yes" ; then
		HELPDIR='${datadir}/bash'
	else
		HELPDIR=$opt_separate_help
	fi
	HELPDIRDEFINE='-H ${HELPDIR}'
	HELPINSTALL='install-help'
	HELPFILES_TARGET='helpdoc'
fi
HELPSTRINGS=
if test "$opt_single_longdoc_strings" != "yes"; then
	HELPSTRINGS='-S'
fi

dnl now substitute in the values generated by arguments
AC_SUBST(TESTSCRIPT)

AC_SUBST(HELPDIR)
AC_SUBST(HELPDIRDEFINE)
AC_SUBST(HELPINSTALL)
AC_SUBST(HELPFILES_TARGET)
AC_SUBST(HELPSTRINGS)

AC_USE_SYSTEM_EXTENSIONS

dnl compilation checks
AC_LANG(C++)

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_YACC

dnl initialize libtool
LT_INIT

echo ""
echo "Beginning configuration for bash-$BASHVERS-$RELSTATUS for ${build_cpu}-${build_vendor}-${build_os}"
echo ""

AC_SYS_LARGEFILE

if test $opt_readline = yes; then
	AC_DEFINE([READLINE], [1], [Define to get the nifty/glitzy editing features. This is on by default. You can turn it off interactively with the -nolineediting flag.])
fi

if test $opt_history = yes || test $opt_bang_history = yes; then
	if test $opt_history = yes; then
		AC_DEFINE([HISTORY], [1], [Define if you want to have access to previously typed commands. If both HISTORY and READLINE are defined, you can manipulate the history with line editing commands. If only HISTORY is defined, the `fc' and `history' builtins are available.])
	fi

	if test $opt_bang_history = yes; then
		AC_DEFINE([BANG_HISTORY], [1], [Define if you want csh-style "!" history expansion. This is unrelated to READLINE.])
	fi
fi

case "$ac_cv_prog_YACC" in
*bison*)	;;
*)	AC_MSG_WARN([bison not available; needed to process parse.yy]) ;;
esac

dnl include custom bash rules, moved from aclocal.m4.
m4_include([m4/bash-rules.m4])

m4_include([m4/stat-time.m4])
m4_include([m4/timespec.m4])

dnl include files for gettext

m4_include([m4/codeset.m4])
m4_include([m4/extern-inline.m4])
m4_include([m4/fcntl-o.m4])
m4_include([m4/glibc2.m4])
m4_include([m4/glibc21.m4])
m4_include([m4/host-cpu-c-abi.m4])
m4_include([m4/intdiv0.m4])
m4_include([m4/intl.m4])
m4_include([m4/intlmacosx.m4])
m4_include([m4/intldir.m4])
m4_include([m4/intmax.m4])
m4_include([m4/inttypes-pri.m4])
m4_include([m4/inttypes.m4])
m4_include([m4/inttypes_h.m4])
m4_include([m4/lcmessage.m4])
m4_include([m4/lib-ld.m4])
m4_include([m4/lib-link.m4])
m4_include([m4/lib-prefix.m4])
m4_include([m4/lock.m4])
m4_include([m4/nls.m4])
m4_include([m4/po.m4])
m4_include([m4/printf-posix.m4])
m4_include([m4/progtest.m4])
m4_include([m4/pthread_rwlock_rdlock.m4])
m4_include([m4/size_max.m4])
m4_include([m4/stdint_h.m4])
m4_include([m4/threadlib.m4])
m4_include([m4/uintmax_t.m4])
m4_include([m4/ulonglong.m4])
m4_include([m4/visibility.m4])
m4_include([m4/wchar_t.m4])
m4_include([m4/wint_t.m4])
m4_include([m4/xsize.m4])

dnl Turn on any extensions available in the GNU C library.
AC_DEFINE([_GNU_SOURCE], [1], [Enable any available GNU C library extensions.])

dnl initialize GNU gettext
AM_ICONV
AM_GNU_GETTEXT([external], [need-ngettext], [intl])
AM_GNU_GETTEXT_VERSION([0.19])

dnl additional iconv checks
AC_SEARCH_LIBS([locale_charset], [iconv])
AC_CACHE_CHECK(
  [for nl_langinfo (CODESET)], ac_cv_langinfo_codeset,
  [AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
     [[#include <langinfo.h>]],
     [[char *codeset = nl_langinfo (CODESET);]]
   )],
  [ac_cv_langinfo_codeset=yes],
  [ac_cv_langinfo_codeset=no])]
)
if test x$ac_cv_langinfo_codeset = xyes; then
  AC_DEFINE([HAVE_LANGINFO_CODESET], [1], [have nl_langinfo (CODESET)])
fi

# Checks for header files.
AC_FUNC_ALLOCA
AC_HEADER_MAJOR
AC_CHECK_HEADERS([arpa/inet.h dirent.h fcntl.h grp.h inttypes.h langinfo.h libintl.h \
		  netdb.h netinet/in.h pwd.h regex.h sgtty.h spawn.h stdint.h stdio_ext.h \
		  sys/dir.h sys/file.h sys/ioctl.h sys/ndir.h sys/param.h \
		  sys/random.h sys/resource.h sys/socket.h sys/time.h sys/wait.h syslog.h \
		  termio.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_DECLS([sys_siglist])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_FORK
AC_FUNC_GETGROUPS
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MBRTOWC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRCOLL
AC_FUNC_STRNLEN
AC_CHECK_FUNCS([arc4random asprintf bcopy confstr dprintf eaccess getaddrinfo \
		getcwd getdtablesize fnmatch getentropy getgroups gethostbyname \
		gethostname getpeername getpwent getpwnam getpwuid \
		getrandom getrlimit getrusage getservbyname getservent \
		gettimeofday group_member imaxdiv inet_aton isascii kill killpg \
		localeconv localtime_r lstat mbschr mbsnrtowcs mkdir mkdtemp \
		mkfifo mkstemp mmap nl_langinfo pathconf posix_spawn \
		posix_spawn_file_actions_init posix_spawnattr_init pselect putenv random \
		readlink regcomp regexec rename select setdtablesize setenv setitimer \
		setlocale setresgid setresuid setvbuf socket stpcpy strcasecmp \
		strcasestr strchrnul strerror strncasecmp strnlen strpbrk \
		strsignal strtoimax strtoumax sysconf tcgetattr \
		times ttyname tzset ulimit uname vprintf waitpid \
		wcswidth wcwidth])

# checks for obsolete functions (flagged in red by VS Code).
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MKTIME

BASH_CHECK_MULTIBYTE

dnl checks for the dynamic loading library functions in libc and libdl
if test "$opt_static_link" != yes; then
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_FUNCS(dlopen dlclose dlsym)
fi

dnl check for getpeername in the socket library only if it's not in libc
if test "$ac_cv_func_getpeername" = no; then
	BASH_CHECK_LIB_SOCKET
fi
dnl check for gethostbyname in socket libraries if it's not in libc
if test "$ac_cv_func_gethostbyname" = no; then
	BASH_FUNC_GETHOSTBYNAME
fi

dnl system types
AC_TYPE_GETGROUPS
AC_TYPE_OFF_T
AC_TYPE_MODE_T
AC_TYPE_UID_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINTPTR_T

AC_CHECK_TYPE(ssize_t, int)
AC_CHECK_TYPE(time_t, long)

AC_CHECK_SIZEOF(char, 1)
AC_CHECK_SIZEOF(short, 2)
AC_CHECK_SIZEOF(int, 4)
AC_CHECK_SIZEOF(long, 4)
AC_CHECK_SIZEOF(char *, 4)
AC_CHECK_SIZEOF(double, 8)
AC_CHECK_SIZEOF([long long], 8)

dnl structures
AC_HEADER_STAT

dnl system services
AC_SYS_INTERPRETER
if test $ac_cv_sys_interpreter = yes; then
AC_DEFINE([HAVE_HASH_BANG_EXEC], [1], [Define if the kernel can exec files beginning with #!.])
fi

dnl Miscellaneous Bash tests
if test "$ac_cv_func_lstat" = "no"; then
BASH_FUNC_LSTAT
fi

dnl behavior of system calls and library functions
BASH_FUNC_CTYPE_NONASCII
BASH_FUNC_DUP2_CLOEXEC_CHECK
BASH_SYS_PGRP_SYNC
BASH_SYS_SIGNAL_VINTAGE

dnl checking for the presence of certain library symbols
BASH_SYS_ERRLIST
BASH_SYS_SIGLIST
BASH_UNDER_SYS_SIGLIST

dnl various system types
AC_CHECK_TYPES([clock_t], [], [], [[#include <sys/times.h>]])
AC_CHECK_TYPES([sigset_t], [], [], [[#include <signal.h>]])
AC_CHECK_TYPES([sig_atomic_t], [], [], [[#include <signal.h>]])
AC_CHECK_TYPES([quad_t])
AC_CHECK_TYPES([intmax_t])
AC_CHECK_TYPES([uintmax_t])
if test "$ac_cv_header_sys_socket_h" = "yes"; then
AC_CHECK_TYPES([socklen_t], [], [], [[#include <sys/socket.h>]])
fi
BASH_TYPE_RLIMIT

AC_CHECK_SIZEOF(intmax_t, 8)

dnl presence and contents of structures used by system calls
BASH_STRUCT_TERMIOS_LDISC
BASH_STRUCT_TERMIO_LDISC
BASH_STRUCT_DIRENT_D_INO
BASH_STRUCT_DIRENT_D_FILENO
BASH_STRUCT_DIRENT_D_NAMLEN
BASH_STRUCT_WINSIZE
BASH_STRUCT_TIMEVAL
BASH_STRUCT_TIMEZONE

BASH_STRUCT_WEXITSTATUS_OFFSET

BASH_CHECK_TYPE_STRUCT_TIMESPEC
BASH_STAT_TIME

dnl presence and behavior of C library functions
BASH_FUNC_STRSIGNAL
BASH_FUNC_OPENDIR_CHECK
BASH_FUNC_ULIMIT_MAXFDS
BASH_FUNC_FPURGE
BASH_FUNC_GETENV
BASH_FUNC_STRCOLL

dnl If putenv or unsetenv is not present, set the right define so the
dnl prototype and declaration in lib/sh/getenv.c will be standard-conformant

if test "$ac_cv_func_putenv" = "yes"; then
BASH_FUNC_STD_PUTENV
else
AC_DEFINE([HAVE_STD_PUTENV])
fi
if test "$ac_cv_func_unsetenv" = "yes"; then
BASH_FUNC_STD_UNSETENV
else
AC_DEFINE([HAVE_STD_UNSETENV])
fi

BASH_FUNC_PRINTF_A_FORMAT

BASH_FUNC_FNMATCH_EQUIV_FALLBACK

dnl presence and behavior of OS functions
BASH_SYS_REINSTALL_SIGHANDLERS
BASH_SYS_JOB_CONTROL_MISSING
BASH_SYS_NAMED_PIPES

dnl presence of certain CPP defines
AC_HEADER_TIOCGWINSZ
BASH_HAVE_TIOCSTAT
BASH_HAVE_FIONREAD

BASH_CHECK_WCONTINUED

dnl miscellaneous
BASH_CHECK_SPEED_T
BASH_CHECK_GETPW_FUNCS
BASH_CHECK_RTSIGS
BASH_CHECK_SYS_SIGLIST

dnl special checks
case "$host_os" in
hpux*)	BASH_CHECK_KERNEL_RLIMIT ;;
esac

if test "$opt_readline" = yes; then
dnl yuck
case "$host_os" in
aix*)	prefer_curses=yes ;;
esac
BASH_CHECK_LIB_TERMCAP
fi
AC_SUBST(TERMCAP_LIB)
AC_SUBST(TERMCAP_DEP)

BASH_CHECK_DEV_FD
BASH_CHECK_DEV_STDIN
BASH_SYS_DEFAULT_MAIL_DIR

if test "$bash_cv_job_control_missing" = missing; then
	opt_job_control=no
fi

if test "$opt_job_control" = yes; then
AC_DEFINE([JOB_CONTROL], [1], [Define if your operating system supports BSD-like job control.])
fi

if test "$opt_job_control" = yes; then
AC_DEFINE([JOB_CONTROL], [1], [Define if your operating system supports BSD-like job control.])
fi

# compile either jobs.cc or nojobs.cc, depending on this setting.
AM_CONDITIONAL([JOB_CONTROL], [test x$opt_job_control = xyes])

AC_DEFINE_UNQUOTED([PROGRAM], ["bash"], [The hardcoded program name.])
AC_DEFINE_UNQUOTED([CONF_CPUTYPE], ["$build_cpu"], [The build target CPU name.])
AC_DEFINE_UNQUOTED([CONF_OSTYPE], ["$build_os"], [The build target OS name.])
AC_DEFINE_UNQUOTED([CONF_MACHTYPE], ["$build"], [The build target machine triplet.])
AC_DEFINE_UNQUOTED([CONF_VENDOR], ["$build_vendor"], [The build target vendor name.])

# directory where we install dynamically loadable builtins
if test -z "$loadablesdir"; then
	loadablesdir='${libdir}/bash'
fi
AC_SUBST(loadablesdir)

if test -z "$headersdir"; then
	headersdir='$(includedir)/$(PACKAGE_NAME)'
fi
AC_SUBST(headersdir)

AC_SUBST(BASHVERS)
AC_SUBST(RELSTATUS)

AC_SUBST(build_cpu)
AC_SUBST(build_vendor)
AC_SUBST(build_os)

AC_CONFIG_FILES([Makefile intl/Makefile doc/Makefile po/Makefile.in \
		 pathnames.hh support/bash.pc support/bashbug.sh])
AC_OUTPUT
