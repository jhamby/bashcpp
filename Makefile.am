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

ACLOCAL_AMFLAGS = -I m4

RLDIR = lib/readline
SHDIR = lib/sh
TLDIR = lib/tilde
GLBDIR = lib/glob
BLTDIR = builtins

DEFS += -DLOCALEDIR=\"$(localedir)\"

# TODO: add docs and tests subdirs
SUBDIRS = po

bin_PROGRAMS = bash

bash_SOURCES = shell.cc eval.cc parse.yy general.cc parser.cc \
	dispose_cmd.cc execute_cmd.cc variables.cc version.cc \
	expr.cc copy_cmd.cc flags.cc subst.cc hashcmd.cc hashlib.cc mailcheck.cc \
	test.cc trap.cc alias.cc braces.cc make_cmd.cc syntax.cc \
	input.cc bashhist.cc array.cc arrayfunc.cc assoc.cc sig.cc pathexp.cc \
	siglist.cc bashline.cc bracecomp.cc error.cc \
	stringlib.cc locale.cc findcmd.cc redir.cc \
	pcomplete.cc pcomplib.cc alias.hh array.hh assoc.hh bashintl.hh \
	bashtypes.hh builtins.hh command.hh config-bot.hh config-top.hh \
	conftypes.hh externs.hh findcmd.hh flags.hh \
	general.hh hashcmd.hh hashlib.hh input.hh jobs.hh parser.hh \
	patchlevel.hh pathexp.hh pcomplete.hh quit.hh shell.hh sig.hh \
	siglist.hh subst.hh syntax.hh test.hh trap.hh variables.hh \
	include/chartypes.hh include/filecntl.hh include/gettext.hh \
	include/maxpath.hh include/posixdir.hh include/posixselect.hh \
	include/posixstat.hh include/posixtime.hh include/posixwait.hh \
	include/shmbchar.hh include/shmbutil.hh include/shtty.hh \
	include/stat-time.hh include/systimes.hh include/typemax.hh \
	include/unionwait.hh include/timer.hh pathnames.hh

if JOB_CONTROL
bash_SOURCES += jobs.cc
else
bash_SOURCES += nojobs.cc
endif

AM_YFLAGS = -d

AM_CPPFLAGS = -I$(srcdir)/include -I$(srcdir)/lib/readline -I$(srcdir)/lib/glob \
	-I$(srcdir)/lib/tilde -I$(srcdir)/builtins

DEFSRC = $(BLTDIR)/alias_def.cc $(BLTDIR)/bind_def.cc $(BLTDIR)/break_def.cc \
	$(BLTDIR)/builtin_def.cc $(BLTDIR)/caller_def.cc $(BLTDIR)/cd_def.cc \
	$(BLTDIR)/colon_def.cc $(BLTDIR)/command_def.cc \
	$(BLTDIR)/declare_def.cc $(BLTDIR)/echo_def.cc \
	$(BLTDIR)/enable_def.cc $(BLTDIR)/eval_def.cc $(BLTDIR)/getopts_def.cc \
	$(BLTDIR)/exec_def.cc $(BLTDIR)/exit_def.cc $(BLTDIR)/fc_def.cc \
	$(BLTDIR)/fg_bg_def.cc $(BLTDIR)/hash_def.cc $(BLTDIR)/help_def.cc \
	$(BLTDIR)/history_def.cc $(BLTDIR)/jobs_def.cc $(BLTDIR)/kill_def.cc \
	$(BLTDIR)/let_def.cc $(BLTDIR)/read_def.cc $(BLTDIR)/return_def.cc \
	$(BLTDIR)/set_def.cc $(BLTDIR)/setattr_def.cc $(BLTDIR)/shift_def.cc \
	$(BLTDIR)/source_def.cc $(BLTDIR)/suspend_def.cc $(BLTDIR)/test_def.cc \
	$(BLTDIR)/times_def.cc $(BLTDIR)/trap_def.cc $(BLTDIR)/type_def.cc \
	$(BLTDIR)/ulimit_def.cc $(BLTDIR)/umask_def.cc $(BLTDIR)/wait_def.cc \
	$(BLTDIR)/reserved_def.cc $(BLTDIR)/pushd_def.cc $(BLTDIR)/shopt_def.cc \
	$(BLTDIR)/printf_def.cc $(BLTDIR)/complete_def.cc $(BLTDIR)/mapfile_def.cc

bash_SOURCES += $(DEFSRC) $(BLTDIR)/builtins.cc \
	$(BLTDIR)/common.cc $(BLTDIR)/evalstring.cc $(BLTDIR)/evalfile.cc \
	$(BLTDIR)/getopt.cc $(BLTDIR)/bashgetopt.cc $(BLTDIR)/getopt.hh

# programs for generating sources and used by the test cases
noinst_PROGRAMS = mkbuiltins psize mksignames mksyntax

mkbuiltins_SOURCES = $(BLTDIR)/mkbuiltins.cc
psize_SOURCES = $(BLTDIR)/psize.cc
mksignames_SOURCES = support/signames.cc support/mksignames.cc
mksyntax_SOURCES = mksyntax.cc

PSIZE = psize$(EXEEXT)
MKBUILTINS = mkbuiltins$(EXEEXT)
MKSIGNAMES = mksignames$(EXEEXT)
MKSYNTAX = mksyntax$(EXEEXT)

DIRECTDEFINE = -D $(srcdir)
HELPDIRDEFINE = @HELPDIRDEFINE@
HELPSTRINGS = @HELPSTRINGS@

# link to a known filename for shell script to expect
psize.aux: $(PSIZE)
	-$(LN_S) $(PSIZE) psize.aux

pipesize.hh: psize.aux $(srcdir)/builtins/psize.sh
	$(SHELL) $(srcdir)/builtins/psize.sh > $@

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

signames.hh: $(MKSIGNAMES)
	$(RM) $@
	./$(MKSIGNAMES) $@

syntax.cc: $(MKSYNTAX)
	$(RM) $@
	./$(MKSYNTAX) -o $@

CLEANFILES = builtins.cc builtext.hh pipesize.hh psize.aux version.hh signames.hh

# manual dependencies on generated files
builtins/common.$(OBJEXT): builtext.hh
builtins/declare_def.$(OBJEXT): builtext.hh
builtins/evalstring.$(OBJEXT): builtext.hh
builtins/exit_def.$(OBJEXT): builtext.hh
builtins/mkbuiltins.$(OBJEXT): config.hh
builtins/pushd_def.$(OBJEXT): builtext.hh
builtins/shopt_def.$(OBJEXT): version.hh
builtins/ulimit_def.$(OBJEXT): pipesize.hh
builtins/psize.$(OBJEXT): config.hh
bashline.$(OBJEXT): builtext.hh
execute_cmd.$(OBJEXT): builtext.hh
jobs.$(OBJEXT): builtext.hh
nojobs.$(OBJEXT): builtext.hh
parse.$(OBJEXT): builtext.hh
pcomplete.$(OBJEXT): builtext.hh
parser.$(OBJEXT): builtext.hh
redir.$(OBJEXT): pipesize.hh
$(srcdir)/shell.hh: parse.hh
sig.$(OBJEXT): builtext.hh
signames.hh: support/signames.cc support/mksignames.cc $(MKSIGNAMES)
subst.$(OBJEXT): builtext.hh
support/mksignames.$(OBJEXT): config.hh
support/signames.$(OBJEXT): config.hh
trap.$(OBJEXT): builtext.hh signames.hh
variables.$(OBJEXT): builtext.hh version.hh
version.$(OBJEXT): version.hh

Program = bash$(EXEEXT)
Version = @BASHVERS@
RELSTATUS = @RELSTATUS@

version.hh: $(CONFIG_HEADER) patchlevel.hh
	$(SHELL) $(srcdir)/support/mkversion.sh -b -S $(srcdir) -s $(RELSTATUS) -d $(Version) -o newversion.hh \
		&& mv newversion.hh version.hh

bash_SOURCES += $(GLBDIR)/glob.cc $(GLBDIR)/smatch.cc $(GLBDIR)/gmisc.cc \
	$(GLBDIR)/xmbsrtowcs.cc $(GLBDIR)/collsyms.hh $(GLBDIR)/glob.hh \
	$(GLBDIR)/glob_loop.hh $(GLBDIR)/gm_loop.hh $(GLBDIR)/ndir.hh \
	$(GLBDIR)/sm_loop.hh $(GLBDIR)/strmatch.hh

# TODO: include these source files conditionally based on READLINE and HISTORY
bash_SOURCES += $(RLDIR)/readline.cc $(RLDIR)/funmap.cc $(RLDIR)/keymaps.cc \
	$(RLDIR)/vi_mode.cc $(RLDIR)/parens.cc $(RLDIR)/rltty.cc \
	$(RLDIR)/complete.cc $(RLDIR)/bind.cc $(RLDIR)/isearch.cc \
	$(RLDIR)/display.cc $(RLDIR)/signals.cc $(RLDIR)/emacs_keymap.hh \
	$(RLDIR)/vi_keymap.hh $(RLDIR)/util.cc $(RLDIR)/kill.cc \
	$(RLDIR)/undo.cc $(RLDIR)/macro.cc $(RLDIR)/input.cc \
	$(RLDIR)/callback.cc $(RLDIR)/terminal.cc \
	$(RLDIR)/history.cc $(RLDIR)/histsearch.cc $(RLDIR)/histexpand.cc \
	$(RLDIR)/histfile.cc $(RLDIR)/nls.cc $(RLDIR)/search.cc \
	$(RLDIR)/shell.cc $(RLDIR)/tilde.cc $(RLDIR)/text.cc $(RLDIR)/misc.cc \
	$(RLDIR)/colors.cc $(RLDIR)/parse-colors.cc $(RLDIR)/mbutil.cc \
	$(RLDIR)/chardefs.hh $(RLDIR)/colors.hh $(RLDIR)/history.hh \
	$(RLDIR)/parse-colors.hh $(RLDIR)/posixdir.hh $(RLDIR)/posixselect.hh \
	$(RLDIR)/posixstat.hh $(RLDIR)/readline.hh $(RLDIR)/rlconf.hh \
	$(RLDIR)/rldefs.hh $(RLDIR)/rlmbutil.hh $(RLDIR)/rlprivate.hh \
	$(RLDIR)/rlshell.hh $(RLDIR)/rltty.hh $(RLDIR)/rlwinsize.hh \
	$(RLDIR)/tcap.hh $(RLDIR)/tilde.hh

bash_SOURCES += $(SHDIR)/clktck.cc $(SHDIR)/clock.cc $(SHDIR)/getcwd.cc \
	$(SHDIR)/getenv.cc $(SHDIR)/oslib.cc $(SHDIR)/setlinebuf.cc \
	$(SHDIR)/strcasecmp.cc $(SHDIR)/strerror.cc \
	$(SHDIR)/vprint.cc $(SHDIR)/rename.cc $(SHDIR)/zread.cc \
	$(SHDIR)/inet_aton.cc $(SHDIR)/netconn.cc $(SHDIR)/netopen.cc \
	$(SHDIR)/strpbrk.cc $(SHDIR)/timeval.cc $(SHDIR)/makepath.cc \
	$(SHDIR)/pathcanon.cc $(SHDIR)/pathphys.cc $(SHDIR)/tmpfile.cc \
	$(SHDIR)/spell.cc $(SHDIR)/shquote.cc $(SHDIR)/strtrans.cc \
	$(SHDIR)/strcasestr.cc $(SHDIR)/mailstat.cc \
	$(SHDIR)/fmtulong.cc $(SHDIR)/fmtullong.cc $(SHDIR)/fmtumax.cc \
	$(SHDIR)/shmatch.cc $(SHDIR)/strnlen.cc \
	$(SHDIR)/strtoimax.cc $(SHDIR)/strtoumax.cc \
	$(SHDIR)/stringlist.cc $(SHDIR)/stringvec.cc $(SHDIR)/strvis.cc \
	$(SHDIR)/mbschr.cc $(SHDIR)/timers.cc $(SHDIR)/zcatfd.cc \
	$(SHDIR)/zmapfd.cc $(SHDIR)/winsize.cc $(SHDIR)/eaccess.cc \
	$(SHDIR)/fpurge.cc $(SHDIR)/zgetline.cc $(SHDIR)/uconvert.cc \
	$(SHDIR)/ufuncs.cc $(SHDIR)/casemod.cc $(SHDIR)/dprintf.cc \
	$(SHDIR)/input_avail.cc $(SHDIR)/fnxform.cc \
	$(SHDIR)/strchrnul.cc $(SHDIR)/unicode.cc $(SHDIR)/wcswidth.cc \
	$(SHDIR)/wcsnwidth.cc $(SHDIR)/shmbchar.cc \
	$(SHDIR)/random.cc $(SHDIR)/gettimeofday.cc

# TODO: conditionally add mktime.cc if the system mktime tests fail.

bash_SOURCES += $(TLDIR)/tilde.cc
