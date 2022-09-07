# Makefile.am for bashcpp: lib/glob subdirectory.
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

noinst_LIBRARIES = libglob.a

libglob_a_SOURCES = glob.cc smatch.cc gmisc.cc xmbsrtowcs.cc \
	collsyms.hh glob.hh glob_loop.hh gm_loop.hh ndir.hh \
	sm_loop.hh strmatch.hh

AM_CPPFLAGS = -I$(srcdir)/../.. -I$(srcdir)/../../include
