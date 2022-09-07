dnl
dnl Configure script for bashcpp-5.1
dnl
dnl report bugs to jake.hamby@gmail.com
dnl
dnl Process this file with autoconf to produce a configure script.

# Copyright (C) 1987-2020 Free Software Foundation, Inc.

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

AC_INIT([bash],[bashvers-relstatus],[jake.hamby@gmail.com])

dnl make sure we are using a recent autoconf version
AC_PREREQ([2.71])

AC_CONFIG_SRCDIR(shell.h)

dnl where to find install.sh, config.sub, and config.guess
AC_CONFIG_AUX_DIR(./support)
AC_CONFIG_HEADERS(config.h)
AC_CONFIG_MACRO_DIRS([m4])

dnl checks for version info
BASHVERS=bashvers
RELSTATUS=relstatus

dnl defaults for debug settings
case "$RELSTATUS" in
alp*|bet*|dev*|rc*|releng*|maint*)	DEBUG='-DDEBUG' ;;
*)	DEBUG= ;;
esac

dnl canonicalize the host and os so we can do some tricky things before
dnl parsing options
AC_CANONICAL_HOST
AC_CANONICAL_BUILD

dnl configure defaults
opt_curses=no
opt_with_installed_readline=no
opt_with_vfork_subshell=no

#htmldir=

dnl
dnl macros for the bash debugger
dnl
dnl AM_PATH_LISPDIR
AC_ARG_VAR(DEBUGGER_START_FILE, [location of bash debugger initialization file])

dnl autoheader support
AH_TOP([#include "config-top.h"])
AH_BOTTOM([#include "config-bot.h"])

dnl arguments to configure
dnl packages

AC_ARG_WITH(curses, AS_HELP_STRING([--with-curses],[use the curses library instead of the termcap library]), opt_curses=$withval)
AC_ARG_WITH(installed-readline, AS_HELP_STRING([--with-installed-readline],[use a version of the readline library that is already installed]), opt_with_installed_readline=$withval)
AC_ARG_WITH(vfork-subshell, AS_HELP_STRING([--with-vfork-subshell],[prefer vfork to fork when both are available]), opt_with_vfork_subshell=$withval)

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
if test $opt_multibyte = no; then
AC_DEFINE([NO_MULTIBYTE_SUPPORT], [1], [Define to not compile in support for multibyte characters, even if the OS supports them.])
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

if test $opt_with_vfork_subshell = yes; then
AC_DEFINE([VFORK_SUBSHELL], [1], [Define to prefer vfork to fork when both are supported by the OS.])
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

AC_SUBST(htmldir)

AC_SUBST(HELPDIR)
AC_SUBST(HELPDIRDEFINE)
AC_SUBST(HELPINSTALL)
AC_SUBST(HELPFILES_TARGET)
AC_SUBST(HELPSTRINGS)

dnl We want these before the checks, so the checks can modify their values.
if test -z "$CFLAGS"; then
	want_auto_cflags=1
fi

echo ""
echo "Beginning configuration for bash-$BASHVERS-$RELSTATUS for ${host_cpu}-${host_vendor}-${host_os}"
echo ""

dnl compilation checks
AC_LANG(C++)

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_MKDIR_P
AC_PROG_YACC

AC_USE_SYSTEM_EXTENSIONS

if test -n "$want_auto_cflags"; then
	AUTO_CXXFLAGS="-g ${GCC+-O2}"
	AUTO_LDFLAGS="-g ${GCC+-O2}"
	STYLE_CXXFLAGS="${GCC+-Wno-parentheses} ${GCC+-Wno-format-security}"
else
	AUTO_CXXFLAGS= AUTO_LDFLAGS= STYLE_CXXFLAGS=
fi

dnl test for Unix variants
AC_SEARCH_LIBS([strerror],[cposix])

AC_SYS_LARGEFILE

dnl BEGIN changes for cross-building (currently cygwin, minGW, and
dnl (obsolete) BeOS)

SIGNAMES_O=
SIGNAMES_H=lsignames.h

dnl load up the cross-building cache file -- add more cases and cache
dnl files as necessary

dnl Note that host and target machine are the same, and different than the
dnl build machine.
dnl Set SIGNAMES_H based on whether or not we're cross-compiling.

CROSS_COMPILE=
if test "x$cross_compiling" = "xyes"; then
    case "${host}" in
    *-cygwin*)
	cross_cache=${srcdir}/cross-build/cygwin32.cache
	;;
    *-mingw*)
	cross_cache=${srcdir}/cross-build/cygwin32.cache
	;;
    i[[3456]]86-*-beos*)
	cross_cache=${srcdir}/cross-build/x86-beos.cache
	;;
    *-qnx*)
	cross_cache=${srcdir}/cross-build/qnx.cache
	;;
    *)	echo "configure: cross-compiling for $host is not supported" >&2
	;;
    esac
    if test -n "${cross_cache}" && test -r "${cross_cache}"; then
	echo "loading cross-build cache file ${cross_cache}"
	. ${cross_cache}
    fi
    unset cross_cache
    SIGNAMES_O='signames.o'
    CROSS_COMPILE='-DCROSS_COMPILING'
    AC_SUBST(CROSS_COMPILE)
fi
AC_SUBST(SIGNAMES_H)
AC_SUBST(SIGNAMES_O)

dnl END changes for cross-building

dnl default values
CXXFLAGS=${CXXFLAGS-"$AUTO_CXXFLAGS"}
# LDFLAGS=${LDFLAGS="$AUTO_LDFLAGS"}	# XXX

dnl handle options that alter how bash is compiled and linked
dnl these must come after the test for cc/gcc
if test "$opt_profiling" = "yes"; then
	PROFILE_FLAGS=-pg
	case "$host_os" in
	solaris2*|linux*)	;;
	*)		opt_static_link=yes ;;
	esac
	DEBUG=
fi

prefer_shared=yes
prefer_static=no

if test "$opt_static_link" = yes; then
	prefer_static=yes
	prefer_shared=no
	# if we're using gcc, add `-static' to LDFLAGS, except on Solaris >= 2
	if test -n "$GCC" || test "$ac_cv_c_compiler_gnu" = "yes"; then
		STATIC_LD="-static"
		case "$host_os" in
		solaris2*|linux*)	;;
		*)		LDFLAGS="$LDFLAGS -static" ;;	# XXX experimental
		esac
	fi
fi

# set the appropriate make variables for building the "build tools"
# modify defaults based on whether or not we are cross compiling, since the
# options for the target host may not be appropriate for the build host
if test "X$cross_compiling" = "Xno"; then
	CXX_FOR_BUILD=${CXX_FOR_BUILD-'$(CXX)'}
	CPPFLAGS_FOR_BUILD=${CPPFLAGS_FOR_BUILD-"$CPPFLAGS"}	# XXX - should it be '$(CPPFLAGS)'
	if test X"$opt_profiling" = Xyes; then
		LDFLAGS_FOR_BUILD=${LDFLAGS_FOR_BUILD-'$(BASE_LDFLAGS)'}
	else
		LDFLAGS_FOR_BUILD=${LDFLAGS_FOR_BUILD-'$(LDFLAGS)'}
	fi
	# CXXFLAGS set above to default value if not passed in environment
	if test -n "$want_auto_cflags" ; then
		CXXFLAGS_FOR_BUILD="${CXXFLAGS} ${STYLE_CXXFLAGS}"
	else
		CXXFLAGS_FOR_BUILD=${CXXFLAGS-'$(CXXFLAGS)'}
	fi
	LIBS_FOR_BUILD=${LIBS_FOR_BUILD-'$(LIBS)'}
else
	CXX_FOR_BUILD=${CXX_FOR_BUILD-"gcc"}
	CPPFLAGS_FOR_BUILD=${CPPFLAGS_FOR_BUILD-""}
	LDFLAGS_FOR_BUILD=${LDFLAGS_FOR_BUILD-""}
	CXXFLAGS_FOR_BUILD=${CXXFLAGS_FOR_BUILD="-g"}
	LIBS_FOR_BUILD=${LIBS_FOR_BUILD-""}
fi

AC_SUBST(CXXFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(STATIC_LD)

AC_SUBST(CXX_FOR_BUILD)
AC_SUBST(CXXFLAGS_FOR_BUILD)
AC_SUBST(CPPFLAGS_FOR_BUILD)
AC_SUBST(LDFLAGS_FOR_BUILD)
AC_SUBST(LIBS_FOR_BUILD)

AC_PROG_GCC_TRADITIONAL

dnl BEGIN READLINE and HISTORY LIBRARY SECTION
dnl prepare to allow bash to be linked against an already-installed readline

dnl first test that the readline version is new enough to link bash against
if test "$opt_readline" = yes && test "$opt_with_installed_readline" != "no"
then
	# If the user specified --with-installed-readline=PREFIX and PREFIX
	# is not `yes', set ac_cv_rl_prefix to PREFIX
	test $opt_with_installed_readline != "yes" && ac_cv_rl_prefix=$opt_with_installed_readline

	RL_LIB_READLINE_VERSION

	case "$ac_cv_rl_version" in
	9*)	;;
	*)	opt_with_installed_readline=no
		AC_MSG_WARN([installed readline library is too old to be linked with bash])
		AC_MSG_WARN([using private bash version])
		;;
	esac
fi

TILDE_LIB=-ltilde
if test $opt_readline = yes; then
	AC_DEFINE([READLINE], [1], [Define to get the nifty/glitzy editing features. This is on by default. You can turn it off interactively with the -nolineediting flag.])
	if test "$opt_with_installed_readline" != "no" ; then
		case "$opt_with_installed_readline" in
		yes)	RL_INCLUDE= ;;
		*)	case "$RL_INCLUDEDIR" in
			/usr/include)	;;
			*)		RL_INCLUDE='-I${RL_INCLUDEDIR}' ;;
			esac
			;;
		esac
		READLINE_DEP=
		READLINE_LIB=-lreadlinecpp
		# section for OS versions that don't allow unresolved symbols
		# to be compiled into dynamic libraries.
		case "$host_os" in
		cygwin*)	TILDE_LIB= ;;
		esac
	else
		RL_LIBDIR='$(dot)/$(LIBSUBDIR)/readline'
		READLINE_DEP='$(READLINE_LIBRARY)'
		# section for OS versions that ship an older/broken version of
		# readline as a standard dynamic library and don't allow a
		# static version specified as -llibname to override the
		# dynamic version
		case "${host_os}" in
		darwin[[89]]*|darwin10*) READLINE_LIB='${READLINE_LIBRARY}' ;;
		*)		READLINE_LIB=-lreadlinecpp ;;
		esac
	fi
else
	RL_LIBDIR='$(dot)/$(LIBSUBDIR)/readline'
	READLINE_LIB= READLINE_DEP=
fi
if test $opt_history = yes || test $opt_bang_history = yes; then
	if test $opt_history = yes; then
		AC_DEFINE([HISTORY], [1], [Define if you want to have access to previously typed commands. If both HISTORY and READLINE are defined, you can manipulate the history with line editing commands. If only HISTORY is defined, the `fc' and `history' builtins are available.])
	fi
	if test $opt_bang_history = yes; then
		AC_DEFINE([BANG_HISTORY], [1], [Define if you want csh-style "!" history expansion. This is unrelated to READLINE.])
	fi
 	if test "$opt_with_installed_readline" != "no"; then
		HIST_LIBDIR=$RL_LIBDIR
		HISTORY_DEP=
		HISTORY_LIB=-lhistorycpp
		case "$opt_with_installed_readline" in
		yes)	RL_INCLUDE= ;;
		*)	case "$RL_INCLUDEDIR" in
			/usr/include)	;;
			*)		RL_INCLUDE='-I${RL_INCLUDEDIR}' ;;
			esac
			;;
		esac
	else
		HIST_LIBDIR='$(dot)/$(LIBSUBDIR)/readline'
		HISTORY_DEP='$(HISTORY_LIBRARY)'
		# section for OS versions that ship an older version of
		# readline as a standard dynamic library and don't allow a
		# static version specified as -llibname to override the
		# dynamic version
		case "${host_os}" in
		darwin[[89]]*|darwin10*) HISTORY_LIB='${HISTORY_LIBRARY}' ;;
		*)		HISTORY_LIB=-lhistorycpp ;;
		esac
	fi
else
	HIST_LIBDIR='$(dot)/$(LIBSUBDIR)/readline'
	HISTORY_LIB= HISTORY_DEP=
fi
AC_SUBST(READLINE_LIB)
AC_SUBST(READLINE_DEP)
AC_SUBST(RL_LIBDIR)
AC_SUBST(RL_INCLUDEDIR)
AC_SUBST(RL_INCLUDE)
AC_SUBST(HISTORY_LIB)
AC_SUBST(HISTORY_DEP)
AC_SUBST(HIST_LIBDIR)
AC_SUBST(TILDE_LIB)

dnl END READLINE and HISTORY LIBRARY SECTION

dnl programs needed by the build and install process
AC_PROG_INSTALL

AC_PROG_YACC
AC_PROG_MAKE_SET

case "$ac_cv_prog_YACC" in
*bison*)	;;
*)	AC_MSG_WARN([bison not available; needed to process parse.y]) ;;
esac

case "$host_os" in
opennt*|interix*)	MAKE_SHELL="$INTERIX_ROOT/bin/sh" ;;
*)			MAKE_SHELL=/bin/sh ;;
esac
AC_SUBST(MAKE_SHELL)

dnl this is similar to the expanded AC_PROG_RANLIB
if test x$SIZE = x; then
	if test x$ac_tool_prefix = x; then
		SIZE=size
	else
		SIZE=${ac_tool_prefix}size
		save_IFS=$IFS ; IFS=:
		size_found=0
		for dir in $PATH; do
			if test -x $dir/$SIZE ; then
				size_found=1
				break
			fi
		done
		if test $size_found -eq 0; then
			SIZE=:
		fi
		IFS=$save_IFS
	fi
fi
AC_SUBST(SIZE)

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

dnl C compiler characteristics
AC_C_CONST
AC_C_INLINE
AC_C_BIGENDIAN
AC_C_STRINGIZE
AC_TYPE_LONG_DOUBLE
AC_C_PROTOTYPES
AC_C_CHAR_UNSIGNED
AC_C_VOLATILE
AC_C_RESTRICT

dnl libtool for bundled lib/intl
LT_INIT

dnl initialize GNU gettext
AM_ICONV

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

AM_GNU_GETTEXT([external], [need-ngettext])
AM_GNU_GETTEXT_VERSION([0.19])
AM_GNU_GETTEXT_INTL_SUBDIR([lib/intl])
IT_PROG_INTLTOOL

dnl header files
AC_HEADER_DIRENT
AC_HEADER_MAJOR

BASH_HEADER_INTTYPES

AC_CHECK_HEADERS(unistd.h termcap.h termio.h termios.h dlfcn.h \
		 stdbool.h stdint.h netdb.h pwd.h grp.h \
		 regex.h syslog.h ulimit.h)
AC_CHECK_HEADERS(sys/pte.h sys/stream.h sys/select.h sys/file.h sys/ioctl.h \
		 sys/mman.h sys/param.h sys/random.h sys/socket.h sys/stat.h \
		 sys/time.h sys/times.h sys/types.h sys/wait.h sys/resource.h)
AC_CHECK_HEADERS(netinet/in.h arpa/inet.h)

dnl sys/ptem.h requires definitions from sys/stream.h on systems where it
dnl exists
AC_CHECK_HEADER(sys/ptem.h, , ,[[
#if HAVE_SYS_STREAM_H
#  include <sys/stream.h>
#endif
]])

dnl special checks for libc functions
AC_FUNC_CHOWN
AC_FUNC_GETPGRP
AC_FUNC_STRCOLL

dnl if vprintf is not in libc, see if it's defined in stdio.h
if test "$ac_cv_func_vprintf" = no; then
    AC_MSG_CHECKING(for declaration of vprintf in stdio.h)
    AC_EGREP_HEADER([[int[ 	]*vprintf[^a-zA-Z0-9]]],stdio.h,ac_cv_func_vprintf=yes)
    AC_MSG_RESULT($ac_cv_func_vprintf)
    if test $ac_cv_func_vprintf = yes; then
	AC_DEFINE([HAVE_VPRINTF], 1, [Define if you have the vprintf function in stdio.h.])
    fi
fi

if test "$ac_cv_func_vprintf" = no && test "$ac_cv_func__doprnt" = "yes"; then
  AC_LIBOBJ(vprint)
fi

dnl checks for certain version-specific system calls and libc functions
AC_CHECK_FUNC(__setostype, AC_DEFINE([HAVE_SETOSTYPE], 1, [Define if you have the __setostype function.]))
AC_CHECK_FUNC(wait3, AC_DEFINE([HAVE_WAIT3], 1, [Define if you have the wait3 function (used only if you don't have waitpid).]))

dnl checks for missing libc functions
AC_CHECK_FUNC(mkfifo, AC_DEFINE([HAVE_MKFIFO], 1, [Define if you have the mkfifo function.]), AC_DEFINE([MKFIFO_MISSING], 1, [Define if you are missing the mkfifo function.]))

dnl checks for system calls
AC_CHECK_FUNCS(dup2 eaccess fcntl fork getdtablesize getentropy getgroups \
		gethostname getpagesize getpeername getrandom getrlimit \
		getrusage gettimeofday kill killpg localeconv lstat pselect readlink \
		select setdtablesize setitimer tcgetpgrp uname ulimit vfork waitpid)
AC_REPLACE_FUNCS(rename)

dnl checks for c library functions
AC_CHECK_FUNCS(bcopy bzero confstr faccessat fnmatch getaddrinfo \
		gethostbyname getservbyname getservent group_member inet_aton \
		imaxdiv memmove pathconf putenv raise random regcomp regexec \
		setenv setlinebuf setlocale setvbuf siginterrupt strchr \
		sysconf syslog tcgetattr times ttyname tzset unsetenv)

AC_CHECK_FUNCS(vasprintf asprintf)
AC_CHECK_FUNCS(isascii isblank isgraph isprint isspace isxdigit)
AC_CHECK_FUNCS(getpwent getpwnam getpwuid)
AC_CHECK_FUNCS(mkstemp mkdtemp)
AC_CHECK_FUNCS(arc4random)

AC_REPLACE_FUNCS(getcwd)
AC_REPLACE_FUNCS(strcasecmp strcasestr strerror strftime strnlen strpbrk)
AC_REPLACE_FUNCS(strtoimax strtoumax)
AC_REPLACE_FUNCS(dprintf)
AC_REPLACE_FUNCS(strchrnul)

AC_CHECK_HEADERS(libaudit.h)
AC_CHECK_DECLS([AUDIT_USER_TTY],,, [[#include <linux/audit.h>]])

AC_CHECK_DECLS([confstr])
AC_CHECK_DECLS([setregid])
dnl AC_CHECK_DECLS[(setresuid])
dnl AC_CHECK_DECLS[(setresgid])
AC_CHECK_DECLS([strsignal])

AC_CHECK_FUNCS(setresuid setresgid)
AC_CHECK_DECLS(strtoimax)
AC_CHECK_DECLS(strtoumax)

AC_FUNC_MKTIME

AC_CHECK_HEADERS([argz.h fcntl.h stdio_ext.h])

AC_FUNC_MMAP
AC_CHECK_FUNCS([__argz_count __argz_next __argz_stringify dcgettext mempcpy \
		munmap mremap stpcpy strcspn])

INTL_DEP= INTL_INC= LIBINTL_H=
if test "x$USE_INCLUDED_LIBINTL" = "xyes"; then
	INTL_DEP='${INTL_LIBDIR}/libintl.a'
	INTL_INC='-I${INTL_LIBSRC} -I${INTL_BUILDDIR}'
	LIBINTL_H='${INTL_BUILDDIR}/libintl.h'
fi
AC_SUBST(INTL_DEP)
AC_SUBST(INTL_INC)
AC_SUBST(LIBINTL_H)
AC_SUBST(BUILD_INCLUDED_LIBINTL)
AC_SUBST(USE_INCLUDED_LIBINTL)

dnl
dnl End of checks needed by files in lib/intl
dnl

BASH_CHECK_MULTIBYTE

dnl checks for the dynamic loading library functions in libc and libdl
if test "$opt_static_link" != yes; then
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_FUNCS(dlopen dlclose dlsym)
fi

dnl network functions -- check for inet_aton again
if test "$ac_cv_func_inet_aton" != 'yes'; then
BASH_FUNC_INET_ATON
fi

dnl libraries
dnl this is reportedly no longer necessary for irix[56].?
case "$host_os" in
irix4*)	AC_CHECK_LIB(sun, getpwent) ;;
esac

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

AC_CHECK_TYPE(u_int, [unsigned int])
AC_CHECK_TYPE(u_long, [unsigned long])

BASH_TYPE_BITS16_T
BASH_TYPE_U_BITS16_T
BASH_TYPE_BITS32_T
BASH_TYPE_U_BITS32_T
BASH_TYPE_BITS64_T

BASH_TYPE_PTRDIFF_T

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

dnl additional checks recommended by autoscan

AC_CHECK_DECLS([sys_siglist])
AC_CHECK_FUNCS([alarm])
AC_CHECK_FUNCS([endgrent])
AC_CHECK_FUNCS([endpwent])
AC_CHECK_FUNCS([ftruncate])
dnl the inet_ntoa function is only wanted by examples/loadables/accept.c.
AC_CHECK_FUNCS([inet_ntoa])
dnl the iswprint function is only wanted by lib/sh/strtrans.c.
AC_CHECK_FUNCS([iswprint])
AC_CHECK_FUNCS([mblen])
AC_CHECK_FUNCS([mbrlen])
AC_CHECK_FUNCS([memchr])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([mkdir])
AC_CHECK_FUNCS([mkfifo])
AC_CHECK_FUNCS([nl_langinfo])
AC_CHECK_FUNCS([rmdir])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strncasecmp])
AC_CHECK_FUNCS([strpbrk])
AC_CHECK_FUNCS([strrchr])
dnl the strspn function is only wanted by examples/loadables/seq.c.
AC_CHECK_FUNCS([strspn])
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([strtol])
AC_CHECK_FUNCS([strtoull])
AC_CHECK_FUNCS([strtoumax])
AC_CHECK_FUNCS([wcwidth])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([malloc.h])
AC_CHECK_HEADERS([sgtty.h])
AC_CHECK_HEADERS([strings.h])
AC_CHECK_HEADER_STDBOOL
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_TYPES([ptrdiff_t])
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_GETGROUPS
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRNLEN
AC_FUNC_STRTOD
AC_STRUCT_ST_BLOCKS
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

dnl presence and contents of structures used by system calls
BASH_STRUCT_TERMIOS_LDISC
BASH_STRUCT_TERMIO_LDISC
BASH_STRUCT_DIRENT_D_INO
BASH_STRUCT_DIRENT_D_FILENO
BASH_STRUCT_DIRENT_D_NAMLEN
BASH_STRUCT_WINSIZE
BASH_STRUCT_TIMEVAL
AC_CHECK_MEMBERS([struct stat.st_blocks])
AC_STRUCT_TM
AC_STRUCT_TIMEZONE
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
if test "$ac_cv_func_getcwd" = "yes"; then
BASH_FUNC_GETCWD
fi
BASH_FUNC_STRCOLL
BASH_FUNC_SNPRINTF
BASH_FUNC_VSNPRINTF

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
JOBS_O=jobs.o
else
JOBS_O=nojobs.o
fi

AC_SUBST(JOBS_O)

dnl Defines that we want to propagate to the Makefiles in subdirectories,
dnl like glob and readline

LOCAL_DEFS=-DSHELL

dnl use this section to possibly define more cpp variables, specify local
dnl libraries, and specify any additional local cc or ld flags
dnl
dnl most of these are really old; this should really go away someday

case "${host_os}" in
sysv4.2*)	AC_DEFINE([SVR4_2], [1], [Are we running SVR4.2?])
		AC_DEFINE([SVR4], [1], [Are we running some verson of SVR4?]) ;;
sysv4*)		AC_DEFINE([SVR4], [1], [Are we running some verson of SVR4?]) ;;
sysv5*)		AC_DEFINE([SVR5], [1], [Are we running SVR5 (UnixWare 7)?]) ;;
hpux9*)		LOCAL_CFLAGS="-DHPUX9 -DHPUX -DTGETENT_BROKEN -DTGETFLAG_BROKEN" ;;
hpux*)		LOCAL_CFLAGS="-DHPUX -DTGETENT_BROKEN -DTGETFLAG_BROKEN" ;;
dgux*)		LOCAL_CFLAGS=-D_DGUX_SOURCE; LOCAL_LIBS=-ldgc ;;
isc*)		LOCAL_CFLAGS=-Disc386 ;;
rhapsody*)	LOCAL_CFLAGS=-DRHAPSODY ;;
darwin*)	LOCAL_CFLAGS=-DMACOSX ;;
sco3.2v5*)	LOCAL_CFLAGS="-b elf -DWAITPID_BROKEN -DPATH_MAX=1024" ;;
sco3.2v4*)	LOCAL_CFLAGS="-DMUST_UNBLOCK_CHLD -DPATH_MAX=1024" ;;
sco3.2*)	LOCAL_CFLAGS=-DMUST_UNBLOCK_CHLD ;;
sunos4*)	LOCAL_CFLAGS=-DSunOS4 ;;
solaris2.5*)	LOCAL_CFLAGS="-DSunOS5 -DSOLARIS" ;;
solaris2.8*)	LOCAL_CFLAGS=-DSOLARIS  ;;
solaris2.9*)	LOCAL_CFLAGS=-DSOLARIS  ;;
solaris2.10*)	LOCAL_CFLAGS=-DSOLARIS  ;;
solaris2*)	LOCAL_CFLAGS=-DSOLARIS ;;
lynxos*)	LOCAL_CFLAGS=-DRECYCLES_PIDS ;;
linux*)		LOCAL_LDFLAGS=-rdynamic		 # allow dynamic loading
		case "`uname -r`" in
		1.*|2.[[0123]]*)	: ;;
		*)	AC_DEFINE([PGRP_PIPE], [1], [Linux 2.4 and newer pgrp pipe support.]) ;;
		esac ;;
netbsd*|openbsd*)	LOCAL_CFLAGS="-DDEV_FD_STAT_BROKEN" ;;
freebsd*)	LOCAL_CFLAGS='-DHEREDOC_PIPESIZE=4096' ;;
*qnx[[67]]*)	LOCAL_LIBS="-lncurses" ;;
*qnx*)		LOCAL_CFLAGS="-Dqnx -F -3s" LOCAL_LDFLAGS="-3s" LOCAL_LIBS="-lunix -lncurses" ;;
powerux*)	LOCAL_LIBS="-lgen" ;;
cygwin*)	LOCAL_CFLAGS=-DRECYCLES_PIDS ;;
opennt*|interix*) LOCAL_CFLAGS="-DNO_MAIN_ENV_ARG -DBROKEN_DIRENT_D_INO -D_POSIX_SOURCE -D_ALL_SOURCE -DRECYCLES_PIDS" ;;
*openstep*)	LOCAL_CFLAGS="-D__APPLE_CC__" ;;
esac

dnl Stanza for OS/compiler pair-specific flags
case "${host_os}-${CC}" in
aix4.2*-*gcc*)	LOCAL_LDFLAGS="-Xlinker -bexpall -Xlinker -brtl" ;;
aix4.2*)	LOCAL_LDFLAGS="-bexpall -brtl" ;;
bsdi4*-*gcc*)	LOCAL_LDFLAGS="-rdynamic" ;;	# allow dynamic loading, like Linux
bsdi5*-*gcc*)	LOCAL_LDFLAGS="-rdynamic" ;;	# allow dynamic loading, like Linux
hpux11*-gcc*)	LOCAL_LDFLAGS="-Wl,-E" ;;	# allow dynamic loading
esac

dnl FreeBSD-3.x can have either a.out or ELF
case "${host_os}" in
freebsd[[3-9]]*)
		if test -x /usr/bin/objformat && test "`/usr/bin/objformat`" = "elf" ; then
			LOCAL_LDFLAGS=-rdynamic # allow dynamic loading
		fi ;;
freebsdelf*)	LOCAL_LDFLAGS=-rdynamic ;;	# allow dynamic loading
dragonfly*)	LOCAL_LDFLAGS=-rdynamic ;;	# allow dynamic loading
esac

case "$host_cpu" in
*cray*)		LOCAL_CFLAGS="-DCRAY" ;; # shell var so config.h can use it
esac

case "$host_cpu-$host_os" in
ibmrt-*bsd4*)	LOCAL_CFLAGS="-ma -U__STDC__" ;;
esac

case "$host_cpu-$host_vendor-$host_os" in
m88k-motorola-sysv3)	LOCAL_CFLAGS=-DWAITPID_BROKEN ;;
mips-pyramid-sysv4)	LOCAL_CFLAGS=-Xa ;;
esac

# turn off paren warnings in gcc
CFLAGS="$CFLAGS ${STYLE_CFLAGS}"

#
# Shared object configuration section.  These values are generated by
# ${srcdir}/support/shobj-conf
#
if test "$ac_cv_func_dlopen" = "yes" && test -f ${srcdir}/support/shobj-conf
then
	AC_MSG_CHECKING(shared object configuration for loadable builtins)
	eval `${CONFIG_SHELL-/bin/sh} ${srcdir}/support/shobj-conf -C "${CC}" -c "${host_cpu}" -o "${host_os}" -v "${host_vendor}"`
	AC_SUBST(SHOBJ_CC)
	AC_SUBST(SHOBJ_CFLAGS)
	AC_SUBST(SHOBJ_LD)
	AC_SUBST(SHOBJ_LDFLAGS)
	AC_SUBST(SHOBJ_XLDFLAGS)
	AC_SUBST(SHOBJ_LIBS)
	AC_SUBST(SHOBJ_STATUS)
	AC_MSG_RESULT($SHOBJ_STATUS)
else
	SHOBJ_STATUS=unsupported
	AC_SUBST(SHOBJ_STATUS)
fi

# try to create a directory tree if the source is elsewhere
# this should be packaged into a script accessible via ${srcdir}/support
case "$srcdir" in
.)	;;
*)	for d in doc tests support lib examples; do	# dirs
		test -d $d || mkdir $d
	done
	for ld in readline glob tilde sh termcap; do	# libdirs
		test -d lib/$ld || mkdir lib/$ld
	done
	test -d examples/loadables || mkdir examples/loadables	# loadable builtins
	test -d examples/loadables/perl || mkdir examples/loadables/perl
	;;
esac

BUILD_DIR=`pwd`
case "$BUILD_DIR" in
*\ *)	BUILD_DIR=`echo "$BUILD_DIR" | sed 's: :\\\\ :g'` ;;
*)	;;
esac

if test -z "$localedir"; then
	localedir='${datarootdir}/locale'
fi
if test -z "$datarootdir"; then
	datarootdir='${prefix}/share'
fi

AC_SUBST(PROFILE_FLAGS)

AC_SUBST(incdir)
AC_SUBST(BUILD_DIR)

# Some versions of autoconf don't substitute these automatically
AC_SUBST(datarootdir)
AC_SUBST(localedir)

# directory where we install dynamically loadable builtins
if test -z "$loadablesdir"; then
	loadablesdir='${libdir}/bash'
fi
AC_SUBST(loadablesdir)
if test -z "$headersdir"; then
	headersdir='$(includedir)/$(PACKAGE_NAME)'
fi
AC_SUBST(headersdir)

AC_SUBST(YACC)
AC_SUBST(AR)
AC_SUBST(ARFLAGS)

AC_SUBST(BASHVERS)
AC_SUBST(RELSTATUS)
AC_SUBST(DEBUG)

AC_SUBST(host_cpu)
AC_SUBST(host_vendor)
AC_SUBST(host_os)

AC_SUBST(LOCAL_LIBS)
AC_SUBST(LOCAL_CFLAGS)
AC_SUBST(LOCAL_LDFLAGS)
AC_SUBST(LOCAL_DEFS)

AC_CONFIG_FILES([Makefile builtins/Makefile lib/readline/Makefile lib/glob/Makefile \
	  lib/intl/Makefile \
	  lib/sh/Makefile lib/termcap/Makefile \
	  lib/tilde/Makefile doc/Makefile support/Makefile po/Makefile.in \
	  examples/loadables/Makefile examples/loadables/Makefile.inc \
	  examples/loadables/perl/Makefile \
	  support/bash.pc support/bashbug.sh])
AC_CONFIG_COMMANDS([default],[
# Makefile uses this timestamp file to record whether config.h is up to date.
echo timestamp > stamp-h
],[])
AC_OUTPUT
