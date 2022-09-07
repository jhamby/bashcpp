# Makefile.am for bashcpp.
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

RLDIRS = lib/readline
SHDIR = lib/sh
TLDIR = lib/tilde
GLOBDIR = lib/glob

bin_PROGRAMS = bash

bash_SOURCES = shell.cc eval.cc parse.y general.cc y.tab.cc \
	dispose_cmd.cc execute_cmd.cc variables.cc version.cc \
	expr.cc copy_cmd.cc flags.cc subst.cc hashcmd.cc hashlib.cc mailcheck.cc \
	test.cc trap.cc alias.cc jobs.cc nojobs.cc braces.cc \
	input.cc bashhist.cc array.cc arrayfunc.cc assoc.cc sig.cc pathexp.cc \
	siglist.cc bashline.cc bracecomp.cc error.cc \
	stringlib.cc locale.cc findcmd.cc redir.cc \
	pcomplete.cc pcomplib.cc syntax.cc \
	alias.hh arrayfunc.hh array.hh assoc.hh bashintl.hh \
	bashtypes.hh builtins.hh command.hh config-bot.hh config-top.hh config.hh \
	conftypes.hh error.hh execute_cmd.hh externs.hh findcmd.hh flags.hh \
	general.hh hashcmd.hh hashlib.hh input.hh jobs.hh parser.hh \
	patchlevel.hh pathexp.hh pcomplete.hh quit.hh shell.hh sig.hh \
	siglist.hh subst.hh syntax.hh test.hh trap.hh variables.hh y.tab.hh \
	include/chartypes.hh include/filecntl.hh include/gettext.hh \
	include/maxpath.hh include/posixdir.hh include/posixselect.hh \
	include/posixstat.hh include/posixtime.hh include/posixwait.hh \
	include/shmbchar.hh include/shmbutil.hh include/shtty.hh \
	include/stat-time.hh include/systimes.hh include/typemax.hh \
	include/unionwait.hh pathnames.hh

bash_LDADD = lib/readline/libreadlinecpp.a lib/sh/libsh.a \
	lib/tilde/libtilde.a lib/glob/libglob.a

SUBDIRS = $(RLDIRS) $(SHDIR) $(TLDIR) $(GLOBDIR) doc po tests

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I$(srcdir)/include -I$(srcdir)/lib/readline -I$(srcdir)/lib/glob \
	-I$(srcdir)/lib/tilde
