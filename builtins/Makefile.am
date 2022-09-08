# Makefile.am for bashcpp: builtins subdirectory.
#
# Copyright 2022, Jake Hamby.
# Copyright (C) 1996-2018 Free Software Foundation, Inc.

#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

noinst_LIBRARIES = libbuiltins.a

DEFSRC = $(srcdir)/alias_def.cc $(srcdir)/bind_def.cc $(srcdir)/break_def.cc \
	$(srcdir)/builtin_def.cc $(srcdir)/caller_def.cc $(srcdir)/cd_def.cc \
	$(srcdir)/colon_def.cc $(srcdir)/command_def.cc \
	$(srcdir)/declare_def.cc $(srcdir)/echo_def.cc \
	$(srcdir)/enable_def.cc $(srcdir)/eval_def.cc $(srcdir)/getopts_def.cc \
	$(srcdir)/exec_def.cc $(srcdir)/exit_def.cc $(srcdir)/fc_def.cc \
	$(srcdir)/fg_bg_def.cc $(srcdir)/hash_def.cc $(srcdir)/help_def.cc \
	$(srcdir)/history_def.cc $(srcdir)/jobs_def.cc $(srcdir)/kill_def.cc \
	$(srcdir)/let_def.cc $(srcdir)/read_def.cc $(srcdir)/return_def.cc \
	$(srcdir)/set_def.cc $(srcdir)/setattr_def.cc $(srcdir)/shift_def.cc \
	$(srcdir)/source_def.cc $(srcdir)/suspend_def.cc $(srcdir)/test_def.cc \
	$(srcdir)/times_def.cc $(srcdir)/trap_def.cc $(srcdir)/type_def.cc \
	$(srcdir)/ulimit_def.cc $(srcdir)/umask_def.cc $(srcdir)/wait_def.cc \
	$(srcdir)/reserved_def.cc $(srcdir)/pushd_def.cc $(srcdir)/shopt_def.cc \
	$(srcdir)/printf_def.cc $(srcdir)/complete_def.cc $(srcdir)/mapfile_def.cc

libbuiltins_a_SOURCES = alias_def.cc bind_def.cc break_def.cc \
	builtin_def.cc caller_def.cc cd_def.cc colon_def.cc \
	command_def.cc declare_def.cc echo_def.cc \
	enable_def.cc eval_def.cc getopts_def.cc \
	exec_def.cc exit_def.cc fc_def.cc \
	fg_bg_def.cc hash_def.cc help_def.cc \
	history_def.cc jobs_def.cc kill_def.cc \
	let_def.cc read_def.cc return_def.cc \
	set_def.cc setattr_def.cc shift_def.cc \
	source_def.cc suspend_def.cc test_def.cc \
	times_def.cc trap_def.cc type_def.cc \
	ulimit_def.cc umask_def.cc wait_def.cc \
	reserved_def.cc pushd_def.cc shopt_def.cc \
	printf_def.cc complete_def.cc mapfile_def.cc \
	common.cc evalstring.cc evalfile.cc \
	getopt.cc bashgetopt.cc getopt.hh \
	builtins.cc

AM_CPPFLAGS = -I$(srcdir)/.. -I$(srcdir)/../include -I$(srcdir)/../lib/readline

BUILT_SOURCES = builtext.hh builtins.cc pipesize.hh

noinst_PROGRAMS = mkbuiltins psize

mkbuiltins_SOURCES = mkbuiltins.cc

psize_SOURCES = psize.cc

PSIZE = psize$(EXEEXT)
MKBUILTINS = mkbuiltins$(EXEEXT)

DIRECTDEFINE = -D $(srcdir)
HELPDIRDEFINE = @HELPDIRDEFINE@
HELPSTRINGS = @HELPSTRINGS@

# standard extension for shell script to expect
psize.aux: $(PSIZE)
	@$(LN_S) $(PSIZE) psize.aux

pipesize.hh:	psize.aux $(srcdir)/psize.sh
	$(SHELL) $(srcdir)/psize.sh > $@

builtext.hh builtins.cc: $(MKBUILTINS) $(DEFSRC)
	@-if test -f builtins.cc; then mv -f builtins.cc old-builtins.cc; fi
	@-if test -f builtext.hh; then mv -f builtext.hh old-builtext.hh; fi
	./$(MKBUILTINS) -externfile builtext.hh -structfile builtins.cc \
	    -noproduction $(DIRECTDEFINE) $(HELPDIRDEFINE) $(HELPSTRINGS) $(DEFSRC)
	@-if cmp -s old-builtext.hh builtext.hh 2>/dev/null; then \
		mv old-builtext.hh builtext.hh; \
	 else \
		$(RM) old-builtext.hh; \
	 fi
	@-if cmp -s old-builtins.cc builtins.cc 2>/dev/null; then \
		mv old-builtins.cc builtins.cc; \
	 else \
		$(RM) old-builtins.cc; \
	 fi
