# Makefile.am for bashcpp: lib/sh subdirectory.
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

noinst_LIBRARIES = libsh.a

libsh_a_SOURCES = clktck.cc clock.cc getcwd.cc \
	getenv.cc oslib.cc setlinebuf.cc \
	strcasecmp.cc strerror.cc \
	vprint.cc rename.cc zread.cc \
	inet_aton.cc netconn.cc netopen.cc \
	strpbrk.cc timeval.cc makepath.cc \
	pathcanon.cc pathphys.cc tmpfile.cc spell.cc \
	shquote.cc strtrans.cc strcasestr.cc \
	snprintf.cc mailstat.cc \
	fmtulong.cc fmtullong.cc fmtumax.cc \
	shmatch.cc strnlen.cc \
	strtoimax.cc strtoumax.cc \
	mktime.cc mbschr.cc zcatfd.cc \
	zmapfd.cc winsize.cc eaccess.cc \
	fpurge.cc zgetline.cc uconvert.cc ufuncs.cc \
	casemod.cc dprintf.cc input_avail.cc fnxform.cc \
	strchrnul.cc unicode.cc wcswidth.cc \
	wcsnwidth.cc shmbchar.cc \
	random.cc gettimeofday.cc

AM_CPPFLAGS = -I$(srcdir)/../.. -I$(srcdir)/../../include -I$(srcdir)/../../lib/readline
