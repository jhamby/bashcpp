# Makefile.am for bashcpp: lib/readline subdirectory.
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

noinst_LIBRARIES = libreadlinecpp.a

libreadlinecpp_a_SOURCES = readline.cc funmap.cc keymaps.cc \
	vi_mode.cc parens.cc rltty.cc \
	complete.cc bind.cc isearch.cc \
	display.cc signals.cc emacs_keymap.cc \
	vi_keymap.cc util.cc kill.cc \
	undo.cc macro.cc input.cc \
	callback.cc terminal.cc \
	history.cc histsearch.cc histexpand.cc \
	histfile.cc nls.cc search.cc \
	shell.cc tilde.cc text.cc misc.cc \
	colors.cc parse-colors.cc mbutil.cc \
	chardefs.hh colors.hh history.hh \
	parse-colors.hh posixdir.hh posixselect.hh \
	posixstat.hh readline.hh rlconf.hh \
	rldefs.hh rlmbutil.hh rlprivate.hh \
	rlshell.hh rltty.hh rlwinsize.hh \
	tcap.hh tilde.hh

AM_CPPFLAGS = -I$(srcdir)/../..
