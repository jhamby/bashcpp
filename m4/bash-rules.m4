dnl
dnl Bash specific tests
dnl
dnl Some derived from PDKSH 5.1.3 autoconf tests
dnl

dnl
dnl Type of struct rlimit fields: some systems (OSF/1, NetBSD, RISC/os 5.0)
dnl have a rlim_t, others (4.4BSD based systems) use quad_t, others use
dnl long and still others use int (HP-UX 9.01, SunOS 4.1.3).  To simplify
dnl matters, this just checks for rlim_t, quad_t, or long.
dnl
AC_DEFUN([BASH_TYPE_RLIMIT],
[AC_CACHE_CHECK([for size and type of struct rlimit fields], bash_cv_type_rlimit,
[bash_cv_type_rlimit=no
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/resource.h>]], [[rlim_t xxx;]])], [bash_cv_type_rlimit=rlim_t])
if test $bash_cv_type_rlimit = no; then
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
int
main()
{
#ifdef HAVE_QUAD_T
  struct rlimit rl;
  if (sizeof(rl.rlim_cur) == sizeof(quad_t))
    exit(0);
#endif
  exit(1);
}]])], [bash_cv_type_rlimit=quad_t], [bash_cv_type_rlimit=long],
        [AC_MSG_WARN(cannot check quad_t if cross compiling -- defaulting to long)
         bash_cv_type_rlimit=long])
fi
])
AC_DEFINE_UNQUOTED([RLIMTYPE], $bash_cv_type_rlimit, [Define the type of struct rlimit fields.])
])

AC_DEFUN([BASH_TYPE_SIG_ATOMIC_T],
[AC_CACHE_CHECK([for sig_atomic_t in signal.h], ac_cv_have_sig_atomic_t,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <signal.h>
]], [[ sig_atomic_t x; ]])],[ac_cv_have_sig_atomic_t=yes],[ac_cv_have_sig_atomic_t=no])])
if test "$ac_cv_have_sig_atomic_t" = "no"
then
    AC_CHECK_TYPE(sig_atomic_t,int)
fi
])

AC_DEFUN([BASH_FUNC_INET_ATON],
[
AC_CACHE_CHECK([for inet_aton], bash_cv_func_inet_aton,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
struct in_addr ap;]], [[ inet_aton("127.0.0.1", &ap); ]])],[bash_cv_func_inet_aton=yes],[bash_cv_func_inet_aton=no])])
if test $bash_cv_func_inet_aton = yes; then
  AC_DEFINE([HAVE_INET_ATON], 1, [Define if you have the inet_aton function.])
else
  AC_LIBOBJ(inet_aton)
fi
])

AC_DEFUN([BASH_FUNC_GETENV],
[AC_MSG_CHECKING(to see if getenv can be redefined)
AC_CACHE_VAL(bash_cv_getenv_redef,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <stdlib.h>
char *
getenv (const char *name)
{
return "42";
}
int
main()
{
char *s;
/* The next allows this program to run, but does not allow bash to link
   when it redefines getenv.  I'm not really interested in figuring out
   why not. */
#if defined (NeXT)
exit(1);
#endif
s = getenv("ABCDE");
exit(s == 0);  /* force optimizer to leave getenv in */
}
]])],[bash_cv_getenv_redef=yes],[bash_cv_getenv_redef=no],[AC_MSG_WARN(cannot check getenv redefinition if cross compil
ng -- defaulting to yes)
    bash_cv_getenv_redef=yes
])])
AC_MSG_RESULT($bash_cv_getenv_redef)
if test $bash_cv_getenv_redef = yes; then
AC_DEFINE([CAN_REDEFINE_GETENV], 1, [Define if we can redefine the getenv function for library calls.])
fi
])

AC_DEFUN([BASH_FUNC_ULIMIT_MAXFDS],
[AC_MSG_CHECKING(whether ulimit can substitute for getdtablesize)
AC_CACHE_VAL(bash_cv_ulimit_maxfds,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#ifdef HAVE_ULIMIT_H
#include <ulimit.h>
#endif
int
main()
{
long maxfds = ulimit(4, 0L);
exit (maxfds == -1L);
}
]])],[bash_cv_ulimit_maxfds=yes],[bash_cv_ulimit_maxfds=no],[AC_MSG_WARN(cannot check ulimit if cross compiling -- defaulting to no)
    bash_cv_ulimit_maxfds=no
])])
AC_MSG_RESULT($bash_cv_ulimit_maxfds)
if test $bash_cv_ulimit_maxfds = yes; then
AC_DEFINE([ULIMIT_MAXFDS], 1, [Define if ulimit can substitute for getdtablesize to get the max fds.])
fi
])

AC_DEFUN([BASH_FUNC_STRCOLL],
[AC_MSG_CHECKING(whether or not strcoll and strcmp differ)
AC_CACHE_VAL(bash_cv_func_strcoll_broken,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#if defined (HAVE_LOCALE_H)
#include <locale.h>
#endif
#include <string.h>
#include <stdlib.h>

int
main(int c, char *v[])
{
        int     r1, r2;
        char    *deflocale, *defcoll;

#ifdef HAVE_SETLOCALE
        deflocale = setlocale(LC_ALL, "");
	defcoll = setlocale(LC_COLLATE, "");
#endif

#ifdef HAVE_STRCOLL
	/* These two values are taken from tests/glob-test. */
        r1 = strcoll("abd", "aXd");
#else
	r1 = 0;
#endif
        r2 = strcmp("abd", "aXd");

	/* These two should both be greater than 0.  It is permissible for
	   a system to return different values, as long as the sign is the
	   same. */

        /* Exit with 1 (failure) if these two values are both > 0, since
	   this tests whether strcoll(3) is broken with respect to strcmp(3)
	   in the default locale. */
	exit (r1 > 0 && r2 > 0);
}
]])],[bash_cv_func_strcoll_broken=yes],[bash_cv_func_strcoll_broken=no],[AC_MSG_WARN(cannot check strcoll if cross compiling -- defaulting to no)
    bash_cv_func_strcoll_broken=no
])])
AC_MSG_RESULT($bash_cv_func_strcoll_broken)
if test $bash_cv_func_strcoll_broken = yes; then
AC_DEFINE([STRCOLL_BROKEN], 1, [Define if strcoll returns the same value as strcmp in the default locale.])
fi
])

AC_DEFUN([BASH_STRUCT_TERMIOS_LDISC],
[
AC_CHECK_MEMBER(struct termios.c_line, AC_DEFINE([TERMIOS_LDISC], 1, [Define if your struct termios has the c_line field.]), ,[
#include <sys/types.h>
#include <termios.h>
])
])

AC_DEFUN([BASH_STRUCT_TERMIO_LDISC],
[
AC_CHECK_MEMBER(struct termio.c_line, AC_DEFINE([TERMIO_LDISC], 1, [Define if your struct termio has the c_line field.]), ,[
#include <sys/types.h>
#include <termio.h>
])
])

dnl
dnl Like AC_STRUCT_ST_BLOCKS, but doesn't muck with LIBOBJS
dnl
dnl sets bash_cv_struct_stat_st_blocks
dnl
dnl unused for now; we'll see how AC_CHECK_MEMBERS works
dnl
AC_DEFUN([BASH_STRUCT_ST_BLOCKS],
[
AC_MSG_CHECKING([for struct stat.st_blocks])
AC_CACHE_VAL(bash_cv_struct_stat_st_blocks,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/stat.h>
]], [[
int
main()
{
static struct stat a;
if (a.st_blocks) return 0;
return 0;
}
]])],[bash_cv_struct_stat_st_blocks=yes],[bash_cv_struct_stat_st_blocks=no])
])
AC_MSG_RESULT($bash_cv_struct_stat_st_blocks)
if test "$bash_cv_struct_stat_st_blocks" = "yes"; then
AC_DEFINE([HAVE_STRUCT_STAT_ST_BLOCKS], 1, [Define if your struct stat has the st_blocks member.])
fi
])

AC_DEFUN([BASH_CHECK_LIB_TERMCAP],
[
if test "X$bash_cv_termcap_lib" = "X"; then
_bash_needmsg=yes
else
AC_MSG_CHECKING(which library has the termcap functions)
_bash_needmsg=
fi
AC_CACHE_VAL(bash_cv_termcap_lib,
[AC_CHECK_FUNC(tgetent, bash_cv_termcap_lib=libc,
  [AC_CHECK_LIB(termcap, tgetent, bash_cv_termcap_lib=libtermcap,
    [AC_CHECK_LIB(tinfo, tgetent, bash_cv_termcap_lib=libtinfo,
        [AC_CHECK_LIB(curses, tgetent, bash_cv_termcap_lib=libcurses,
	    [AC_CHECK_LIB(ncurses, tgetent, bash_cv_termcap_lib=libncurses,
                [AC_CHECK_LIB(ncursesw, tgetent, bash_cv_termcap_lib=libncursesw,
	            bash_cv_termcap_lib=gnutermcap)])])])])])])
if test "X$_bash_needmsg" = "Xyes"; then
AC_MSG_CHECKING(which library has the termcap functions)
fi
AC_MSG_RESULT(using $bash_cv_termcap_lib)
if test $bash_cv_termcap_lib = gnutermcap && test -z "$prefer_curses"; then
LDFLAGS="$LDFLAGS -L./lib/termcap"
TERMCAP_LIB="./lib/termcap/libtermcap.a"
TERMCAP_DEP="./lib/termcap/libtermcap.a"
elif test $bash_cv_termcap_lib = libtermcap && test -z "$prefer_curses"; then
TERMCAP_LIB=-ltermcap
TERMCAP_DEP=
elif test $bash_cv_termcap_lib = libtinfo; then
TERMCAP_LIB=-ltinfo
TERMCAP_DEP=
elif test $bash_cv_termcap_lib = libncurses; then
TERMCAP_LIB=-lncurses
TERMCAP_DEP=
elif test $bash_cv_termcap_lib = libc; then
TERMCAP_LIB=
TERMCAP_DEP=
else
TERMCAP_LIB=-lcurses
TERMCAP_DEP=
fi
])

AC_DEFUN([BASH_STRUCT_DIRENT_D_INO],
[AC_REQUIRE([AC_HEADER_DIRENT])
AC_MSG_CHECKING(for struct dirent.d_ino)
AC_CACHE_VAL(bash_cv_dirent_has_dino,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if defined(HAVE_DIRENT_H)
# include <dirent.h>
#else
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* SYSNDIR */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* SYSDIR */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif /* HAVE_DIRENT_H */
]], [[
struct dirent d; int z; z = d.d_ino;
]])],[bash_cv_dirent_has_dino=yes],[bash_cv_dirent_has_dino=no])])
AC_MSG_RESULT($bash_cv_dirent_has_dino)
if test $bash_cv_dirent_has_dino = yes; then
AC_DEFINE([HAVE_STRUCT_DIRENT_D_INO], 1, [Define if your struct dirent has the d_ino member.])
fi
])

AC_DEFUN([BASH_STRUCT_DIRENT_D_FILENO],
[AC_REQUIRE([AC_HEADER_DIRENT])
AC_MSG_CHECKING(for struct dirent.d_fileno)
AC_CACHE_VAL(bash_cv_dirent_has_d_fileno,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if defined(HAVE_DIRENT_H)
# include <dirent.h>
#else
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* SYSNDIR */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* SYSDIR */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif /* HAVE_DIRENT_H */
]], [[
struct dirent d; int z; z = d.d_fileno;
]])],[bash_cv_dirent_has_d_fileno=yes],[bash_cv_dirent_has_d_fileno=no])])
AC_MSG_RESULT($bash_cv_dirent_has_d_fileno)
if test $bash_cv_dirent_has_d_fileno = yes; then
AC_DEFINE([HAVE_STRUCT_DIRENT_D_FILENO], 1, [Define if your struct dirent has the d_fileno member.])
fi
])

AC_DEFUN([BASH_STRUCT_DIRENT_D_NAMLEN],
[AC_REQUIRE([AC_HEADER_DIRENT])
AC_MSG_CHECKING(for struct dirent.d_namlen)
AC_CACHE_VAL(bash_cv_dirent_has_d_namlen,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if defined(HAVE_DIRENT_H)
# include <dirent.h>
#else
# define dirent direct
# ifdef HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif /* SYSNDIR */
# ifdef HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif /* SYSDIR */
# ifdef HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif /* HAVE_DIRENT_H */
]], [[
struct dirent d; int z; z = d.d_namlen;
]])],[bash_cv_dirent_has_d_namlen=yes],[bash_cv_dirent_has_d_namlen=no])])
AC_MSG_RESULT($bash_cv_dirent_has_d_namlen)
if test $bash_cv_dirent_has_d_namlen = yes; then
AC_DEFINE(HAVE_STRUCT_DIRENT_D_NAMLEN, 1, [Define if your struct dirent has the d_namlen member.])
fi
])

AC_DEFUN([BASH_STRUCT_TIMEVAL],
[AC_MSG_CHECKING(for struct timeval in sys/time.h and time.h)
AC_CACHE_VAL(bash_cv_struct_timeval,
[AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[#if HAVE_SYS_TIME_H
		  #include <sys/time.h>
		  #endif
		  #include <time.h>
		]],
		[[static struct timeval x; x.tv_sec = x.tv_usec;]]
	)],
	bash_cv_struct_timeval=yes,
	bash_cv_struct_timeval=no)
])
AC_MSG_RESULT($bash_cv_struct_timeval)
if test $bash_cv_struct_timeval = yes; then
  AC_DEFINE([HAVE_TIMEVAL], 1, [Define if you have struct timeval in sys/time.h or time.h.])
fi
])

AC_DEFUN([BASH_STRUCT_TIMEZONE],
[AC_MSG_CHECKING(for struct timezone in sys/time.h and time.h)
AC_CACHE_VAL(bash_cv_struct_timezone,
[
AC_EGREP_HEADER(struct timezone, sys/time.h,
		bash_cv_struct_timezone=yes,
		AC_EGREP_HEADER(struct timezone, time.h,
			bash_cv_struct_timezone=yes,
			bash_cv_struct_timezone=no))
])
AC_MSG_RESULT($bash_cv_struct_timezone)
if test $bash_cv_struct_timezone = yes; then
  AC_DEFINE([HAVE_STRUCT_TIMEZONE], 1, [Define if you have struct timezone in sys/time.h or time.h.])
fi
])

AC_DEFUN([BASH_STRUCT_WINSIZE],
[AC_CACHE_CHECK([for struct winsize in sys/ioctl.h and termios.h], bash_cv_struct_winsize_header,
bash_cv_struct_winsize_header=no
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/ioctl.h>]], [[struct winsize x;]])],[bash_cv_struct_winsize_header=ioctl_h])
if test bash_cv_struct_winsize_header = no; then
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <sys/types.h>
#include <termios.h>], [struct winsize x;])],
  [bash_cv_struct_winsize_header=termios_h], [bash_cv_struct_winsize_header=other])
fi
if test $bash_cv_struct_winsize_header = ioctl_h; then
  AC_DEFINE([STRUCT_WINSIZE_IN_SYS_IOCTL], 1, [Define if you have struct winsize in sys/ioctl.h.])
elif test $bash_cv_struct_winsize_header = termios_h; then
  AC_DEFINE([STRUCT_WINSIZE_IN_TERMIOS], 1, [Define if you have struct winsize in termios.h.])
fi)])

dnl Check type of signal routines (posix, 4.2bsd, 4.1bsd or v7)
AC_DEFUN([BASH_SYS_SIGNAL_VINTAGE],
[AC_CACHE_CHECK([type of signal routines], bash_cv_signal_vintage,
bash_cv_signal_vintage=no
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <signal.h>]], [[
    sigset_t ss;
    struct sigaction sa;
    sigemptyset(&ss); sigsuspend(&ss);
    sigaction(SIGINT, &sa, (struct sigaction *) 0);
    sigprocmask(SIG_BLOCK, &ss, (sigset_t *) 0);
  ]])],[bash_cv_signal_vintage=posix])]
if test bash_cv_signal_vintage = no; then
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <signal.h>]], [[
	int mask = sigmask(SIGINT);
	sigsetmask(mask); sigblock(mask); sigpause(mask);
    ]])], [bash_cv_signal_vintage=4.2bsd])]
fi
if test bash_cv_signal_vintage = no; then
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <signal.h>]], [[
	void foo() { }
	int mask = sigmask(SIGINT);
	sigset(SIGINT, foo); sigrelse(SIGINT);
	sighold(SIGINT); sigpause(SIGINT);
        ]], [bash_cv_signal_vintage=svr3], [bash_cv_signal_vintage=v7])])]
fi
if test "$bash_cv_signal_vintage" = posix; then
AC_DEFINE([HAVE_POSIX_SIGNALS], 1, [Define if you have POSIX signal handling (struct sigaction and friends).])
elif test "$bash_cv_signal_vintage" = "4.2bsd"; then
AC_DEFINE([HAVE_BSD_SIGNALS], 1, [Define if you have 4.2BSD-style signal handling (sigsetmask, sigblock, sigpause).])
elif test "$bash_cv_signal_vintage" = svr3; then
AC_DEFINE([HAVE_USG_SIGHOLD], 1, [Define if you have SVR3-style signal handling (sigset, sigrelse, sigpause).])
fi
)])

dnl Check if the pgrp of setpgrp() can't be the pid of a zombie process.
AC_DEFUN([BASH_SYS_PGRP_SYNC],
[AC_REQUIRE([AC_FUNC_GETPGRP])
AC_MSG_CHECKING(whether pgrps need synchronization)
AC_CACHE_VAL(bash_cv_pgrp_pipe,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>
#endif
#include <stdlib.h>
int
main()
{
# ifdef GETPGRP_VOID
#  define getpgID()	getpgrp()
# else
#  define getpgID()	getpgrp(0)
#  define setpgid(x,y)	setpgrp(x,y)
# endif
	int pid1, pid2, fds[2];
	int status;
	char ok;

	switch (pid1 = fork()) {
	  case -1:
	    exit(1);
	  case 0:
	    setpgid(0, getpid());
	    exit(0);
	}
	setpgid(pid1, pid1);

	sleep(2);	/* let first child die */

	if (pipe(fds) < 0)
	  exit(2);

	switch (pid2 = fork()) {
	  case -1:
	    exit(3);
	  case 0:
	    setpgid(0, pid1);
	    ok = getpgID() == pid1;
	    write(fds[1], &ok, 1);
	    exit(0);
	}
	setpgid(pid2, pid1);

	close(fds[1]);
	if (read(fds[0], &ok, 1) != 1)
	  exit(4);
	wait(&status);
	wait(&status);
	exit(ok ? 0 : 5);
}
]])],[bash_cv_pgrp_pipe=no],[bash_cv_pgrp_pipe=yes],[AC_MSG_WARN(cannot check pgrp synchronization if cross compiling -- defaulting to no)
    bash_cv_pgrp_pipe=no])
])
AC_MSG_RESULT($bash_cv_pgrp_pipe)
if test $bash_cv_pgrp_pipe = yes; then
AC_DEFINE([PGRP_PIPE], 1, [Define if the pgrps from setpgrp can't be the pid of a zombie process.])
fi
])

AC_DEFUN([BASH_SYS_REINSTALL_SIGHANDLERS],
[
AC_MSG_CHECKING([if signal handlers must be reinstalled when invoked])
AC_CACHE_VAL(bash_cv_must_reinstall_sighandlers,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

typedef void sigfunc(int s);

volatile int nsigint;

#ifdef HAVE_POSIX_SIGNALS
sigfunc *
set_signal_handler(int sig, sigfunc *handler)
{
  struct sigaction act, oact;
  act.sa_handler = handler;
  act.sa_flags = 0;
  sigemptyset (&act.sa_mask);
  sigemptyset (&oact.sa_mask);
  sigaction (sig, &act, &oact);
  return (oact.sa_handler);
}
#else
#define set_signal_handler(s, h) signal(s, h)
#endif

void
sigint(int s)
{
  nsigint++;
}

int
main()
{
	nsigint = 0;
	set_signal_handler(SIGINT, sigint);
	kill((int)getpid(), SIGINT);
	kill((int)getpid(), SIGINT);
	exit(nsigint != 2);
}
]])],[bash_cv_must_reinstall_sighandlers=no],[bash_cv_must_reinstall_sighandlers=yes],[AC_MSG_WARN(cannot check signal handling if cross compiling -- defaulting to no)
    bash_cv_must_reinstall_sighandlers=no
])])
AC_MSG_RESULT($bash_cv_must_reinstall_sighandlers)
if test $bash_cv_must_reinstall_sighandlers = yes; then
AC_DEFINE([MUST_REINSTALL_SIGHANDLERS], 1, [Define if signal handlers must be reinstalled after invoking.])
fi
])

dnl check that some necessary job control definitions are present
AC_DEFUN([BASH_SYS_JOB_CONTROL_MISSING],
[AC_REQUIRE([BASH_SYS_SIGNAL_VINTAGE])
AC_MSG_CHECKING(for presence of necessary job control definitions)
AC_CACHE_VAL(bash_cv_job_control_missing,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <signal.h>

/* add more tests in here as appropriate */

/* signal type */
#if !defined (HAVE_POSIX_SIGNALS) && !defined (HAVE_BSD_SIGNALS)
#error
#endif

/* signals and tty control. */
#if !defined (SIGTSTP) || !defined (SIGSTOP) || !defined (SIGCONT)
#error
#endif

/* process control */
#if !defined (WNOHANG) || !defined (WUNTRACED)
#error
#endif

]], [[]])],[bash_cv_job_control_missing=present],[bash_cv_job_control_missing=missing
])])
AC_MSG_RESULT($bash_cv_job_control_missing)
if test $bash_cv_job_control_missing = missing; then
AC_DEFINE([JOB_CONTROL_MISSING], 1, [Define if this system is missing definitions required for job control.])
fi
])

dnl check whether named pipes are present
dnl this requires a previous check for mkfifo, but that is awkward to specify
AC_DEFUN([BASH_SYS_NAMED_PIPES],
[AC_MSG_CHECKING(for presence of named pipes)
AC_CACHE_VAL(bash_cv_sys_named_pipes,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>

/* Add more tests in here as appropriate. */
int
main()
{
int fd, err;

#if defined (HAVE_MKFIFO)
exit (0);
#endif

#if !defined (S_IFIFO) && (defined (_POSIX_VERSION) && !defined (S_ISFIFO))
exit (1);
#endif

#if defined (NeXT)
exit (1);
#endif
err = mkdir("bash-aclocal", 0700);
if (err < 0) {
  perror ("mkdir");
  exit(1);
}
fd = mknod ("bash-aclocal/sh-np-autoconf", 0666 | S_IFIFO, 0);
if (fd == -1) {
  rmdir ("bash-aclocal");
  exit (1);
}
close(fd);
unlink ("bash-aclocal/sh-np-autoconf");
rmdir ("bash-aclocal");
exit(0);
}]])],[bash_cv_sys_named_pipes=present],[bash_cv_sys_named_pipes=missing],[AC_MSG_WARN(cannot check for named pipes if cross-compiling -- defaulting to missing)
     bash_cv_sys_named_pipes=missing
])])
AC_MSG_RESULT($bash_cv_sys_named_pipes)
if test $bash_cv_sys_named_pipes = missing; then
AC_DEFINE([NAMED_PIPES_MISSING], 1, [Define if this system is missing support for named pipes.])
fi
])

AC_DEFUN([BASH_SYS_DEFAULT_MAIL_DIR],
[AC_MSG_CHECKING(for default mail directory)
AC_CACHE_VAL(bash_cv_mail_dir,
[if test -d /var/mail; then
   bash_cv_mail_dir=/var/mail
 elif test -d /var/spool/mail; then
   bash_cv_mail_dir=/var/spool/mail
 elif test -d /usr/mail; then
   bash_cv_mail_dir=/usr/mail
 elif test -d /usr/spool/mail; then
   bash_cv_mail_dir=/usr/spool/mail
 else
   bash_cv_mail_dir=unknown
 fi
])
AC_MSG_RESULT($bash_cv_mail_dir)
AC_DEFINE_UNQUOTED([DEFAULT_MAIL_DIRECTORY], "$bash_cv_mail_dir", [Location of the system default mail directory.])
])

AC_DEFUN([BASH_HAVE_TIOCGWINSZ],
[AC_MSG_CHECKING(for TIOCGWINSZ in sys/ioctl.h)
AC_CACHE_VAL(bash_cv_tiocgwinsz_in_ioctl,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/ioctl.h>]], [[int x = TIOCGWINSZ;]])],[bash_cv_tiocgwinsz_in_ioctl=yes],[bash_cv_tiocgwinsz_in_ioctl=no])])
AC_MSG_RESULT($bash_cv_tiocgwinsz_in_ioctl)
if test $bash_cv_tiocgwinsz_in_ioctl = yes; then
AC_DEFINE([GWINSZ_IN_SYS_IOCTL], 1, [Define if your system defines TIOCGWINSZ.])
fi
])

AC_DEFUN([BASH_HAVE_TIOCSTAT],
[AC_MSG_CHECKING(for TIOCSTAT in sys/ioctl.h)
AC_CACHE_VAL(bash_cv_tiocstat_in_ioctl,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/ioctl.h>]], [[int x = TIOCSTAT;]])],[bash_cv_tiocstat_in_ioctl=yes],[bash_cv_tiocstat_in_ioctl=no])])
AC_MSG_RESULT($bash_cv_tiocstat_in_ioctl)
if test $bash_cv_tiocstat_in_ioctl = yes; then
AC_DEFINE([TIOCSTAT_IN_SYS_IOCTL], 1, [Define if TIOCSTAT is defined in your sys/ioctl.h.])
fi
])

AC_DEFUN([BASH_HAVE_FIONREAD],
[AC_MSG_CHECKING(for FIONREAD in sys/ioctl.h)
AC_CACHE_VAL(bash_cv_fionread_in_ioctl,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>
#include <sys/ioctl.h>]], [[int x = FIONREAD;]])],[bash_cv_fionread_in_ioctl=yes],[bash_cv_fionread_in_ioctl=no])])
AC_MSG_RESULT($bash_cv_fionread_in_ioctl)
if test $bash_cv_fionread_in_ioctl = yes; then
AC_DEFINE([FIONREAD_IN_SYS_IOCTL], 1, [Define if FIONREAD is defined in your sys/ioctl.h.])
fi
])

dnl
dnl See if speed_t is declared in <sys/types.h>.  Some versions of linux
dnl require a definition of speed_t each time <termcap.h> is included,
dnl but you can only get speed_t if you include <termios.h> (on some
dnl versions) or <sys/types.h> (on others).
dnl
AC_DEFUN([BASH_CHECK_SPEED_T],
[AC_MSG_CHECKING(for speed_t in sys/types.h)
AC_CACHE_VAL(bash_cv_speed_t_in_sys_types,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/types.h>]], [[speed_t x;]])],[bash_cv_speed_t_in_sys_types=yes],[bash_cv_speed_t_in_sys_types=no])])
AC_MSG_RESULT($bash_cv_speed_t_in_sys_types)
if test $bash_cv_speed_t_in_sys_types = yes; then
AC_DEFINE([SPEED_T_IN_SYS_TYPES], 1, [Define if speed_t is defined in your sys/types.h.])
fi
])

AC_DEFUN([BASH_CHECK_GETPW_FUNCS],
[AC_MSG_CHECKING(whether getpw functions are declared in pwd.h)
AC_CACHE_VAL(bash_cv_getpw_declared,
[AC_EGREP_CPP(getpwuid,
[
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <pwd.h>
],
bash_cv_getpw_declared=yes,bash_cv_getpw_declared=no)])
AC_MSG_RESULT($bash_cv_getpw_declared)
if test $bash_cv_getpw_declared = yes; then
AC_DEFINE([HAVE_GETPW_DECLS], 1, [Define if getpw functions are declared in your pwd.h.])
fi
])

AC_DEFUN([BASH_CHECK_DEV_FD],
[AC_MSG_CHECKING(whether /dev/fd is available)
AC_CACHE_VAL(bash_cv_dev_fd,
[bash_cv_dev_fd=""
if test -d /dev/fd  && (exec test -r /dev/fd/0 < /dev/null) ; then
# check for systems like FreeBSD 5 that only provide /dev/fd/[012]
   if (exec test -r /dev/fd/3 3</dev/null) ; then
     bash_cv_dev_fd=standard
   else
     bash_cv_dev_fd=absent
   fi
fi
if test -z "$bash_cv_dev_fd" ; then
  if test -d /proc/self/fd && (exec test -r /proc/self/fd/0 < /dev/null) ; then
    bash_cv_dev_fd=whacky
  else
    bash_cv_dev_fd=absent
  fi
fi
])
AC_MSG_RESULT($bash_cv_dev_fd)
if test $bash_cv_dev_fd = "standard"; then
  AC_DEFINE([HAVE_DEV_FD], 1, [Define if /dev/fd is available.])
  AC_DEFINE([DEV_FD_PREFIX], "/dev/fd/", [Define the prefix for your /dev/fd.])
elif test $bash_cv_dev_fd = "whacky"; then
  AC_DEFINE([HAVE_DEV_FD], 1, [Define if /dev/fd is available.])
  AC_DEFINE([DEV_FD_PREFIX], "/proc/self/fd/", [Define the prefix for your /dev/fd.])
fi
])

AC_DEFUN([BASH_CHECK_DEV_STDIN],
[AC_MSG_CHECKING(whether /dev/stdin stdout stderr are available)
AC_CACHE_VAL(bash_cv_dev_stdin,
[if (exec test -r /dev/stdin < /dev/null) ; then
   bash_cv_dev_stdin=present
 else
   bash_cv_dev_stdin=absent
 fi
])
AC_MSG_RESULT($bash_cv_dev_stdin)
if test $bash_cv_dev_stdin = "present"; then
  AC_DEFINE([HAVE_DEV_STDIN], 1, [Define if /dev/stdin, stdout, and stderr are available.])
fi
])

dnl
dnl Check if HPUX needs _KERNEL defined for RLIMIT_* definitions
dnl
AC_DEFUN([BASH_CHECK_KERNEL_RLIMIT],
[AC_CACHE_CHECK([whether $host_os needs _KERNEL for RLIMIT defines], bash_cv_kernel_rlimit,
[bash_cv_kernel_rlimit=unknown;
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#include <sys/resource.h>
]], [[
  int f;
  f = RLIMIT_DATA;
]])],[bash_cv_kernel_rlimit=no])
if bash_cv_kernel_rlimit = unknown; then
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <sys/types.h>
#define _KERNEL
#include <sys/resource.h>
#undef _KERNEL
]],
[
	int f;
        f = RLIMIT_DATA;
], bash_cv_kernel_rlimit=yes, bash_cv_kernel_rlimit=no)])
fi])
AC_MSG_RESULT($bash_cv_kernel_rlimit)
if test $bash_cv_kernel_rlimit = yes; then
AC_DEFINE([RLIMIT_NEEDS_KERNEL], 1, [Define if you need _KERNEL defined for the RLIMIT_* definitions.])
fi
])

AC_DEFUN([BASH_CHECK_RTSIGS],
[AC_MSG_CHECKING(for unusable real-time signals due to large values)
AC_CACHE_VAL(bash_cv_unusable_rtsigs,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>

#ifndef NSIG
#  define NSIG 64
#endif

int
main ()
{
  int n_sigs = 2 * NSIG;
#ifdef SIGRTMIN
  int rtmin = SIGRTMIN;
#else
  int rtmin = 0;
#endif

  exit(rtmin < n_sigs);
}]])],[bash_cv_unusable_rtsigs=yes],[bash_cv_unusable_rtsigs=no],[AC_MSG_WARN(cannot check real-time signals if cross compiling -- defaulting to yes)
     bash_cv_unusable_rtsigs=yes
])])
AC_MSG_RESULT($bash_cv_unusable_rtsigs)
if test $bash_cv_unusable_rtsigs = yes; then
AC_DEFINE([UNUSABLE_RT_SIGNALS], 1, [Define if your real-time signals are unusuable due to large values.])
fi
])

dnl
dnl check for availability of multibyte characters and functions
dnl
dnl geez, I wish I didn't have to check for all of this stuff separately
dnl
AC_DEFUN([BASH_CHECK_MULTIBYTE],
[
AC_CHECK_HEADERS(wctype.h)
AC_CHECK_HEADERS(wchar.h)
AC_CHECK_HEADERS(langinfo.h)

AC_CHECK_HEADERS(mbstr.h)

AC_CHECK_FUNC(mbrlen)
AC_CHECK_FUNC(mbsnrtowcs)
AC_CHECK_FUNC(mbsrtowcs)

AC_CHECK_FUNC(wcrtomb)
AC_CHECK_FUNC(wcscoll)
AC_CHECK_FUNC(wcsdup)
AC_CHECK_FUNC(wcwidth)
AC_CHECK_FUNC(wctype)

AC_REPLACE_FUNCS(wcswidth)

dnl checks for both mbrtowc and mbstate_t
AC_FUNC_MBRTOWC
if test $ac_cv_func_mbrtowc = yes; then
	AC_DEFINE([HAVE_MBSTATE_T], 1, [Define if you have the mbstate_t type.])
fi

AC_CHECK_FUNCS(iswlower iswupper towlower towupper iswctype)

AC_CACHE_CHECK([for nl_langinfo and CODESET], bash_cv_langinfo_codeset,
[AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <langinfo.h>]], [[char* cs = nl_langinfo(CODESET);]])],[bash_cv_langinfo_codeset=yes],[bash_cv_langinfo_codeset=no])])
if test $bash_cv_langinfo_codeset = yes; then
  AC_DEFINE([HAVE_LANGINFO_CODESET], 1, [Define if you have <langinfo.h> and nl_langinfo(CODESET).])
fi

dnl check for broken wcwidth
AC_CACHE_CHECK([for wcwidth broken with unicode combining characters],
bash_cv_wcwidth_broken,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <locale.h>
#include <wchar.h>

int
main(int c, char **v)
{
        int     w;

        setlocale(LC_ALL, "en_US.UTF-8");
        w = wcwidth (0x0301);
        exit (w == 0);  /* exit 0 if wcwidth broken */
}
]])],[bash_cv_wcwidth_broken=yes],[bash_cv_wcwidth_broken=no],[bash_cv_wcwidth_broken=no])])
if test "$bash_cv_wcwidth_broken" = yes; then
        AC_DEFINE([WCWIDTH_BROKEN], 1, [Define for broken wcwidth with unicode combining chars, e.g. MINIX 3.4.])
fi

if test "$am_cv_func_iconv" = yes; then
	OLDLIBS="$LIBS"
	LIBS="$LIBS $LIBINTL $LIBICONV"
	AC_CHECK_FUNCS(locale_charset)
	LIBS="$OLDLIBS"
fi

AC_CHECK_SIZEOF(wchar_t, 4)

])

dnl need: prefix exec_prefix libdir includedir CC TERMCAP_LIB
dnl require:
dnl	AC_PROG_CC
dnl	BASH_CHECK_LIB_TERMCAP

AC_DEFUN([RL_LIB_READLINE_VERSION],
[
AC_REQUIRE([BASH_CHECK_LIB_TERMCAP])

AC_MSG_CHECKING([version of installed readline library])

# What a pain in the ass this is.

# save cpp and ld options
_save_CFLAGS="$CFLAGS"
_save_LDFLAGS="$LDFLAGS"
_save_LIBS="$LIBS"

# Don't set ac_cv_rl_prefix if the caller has already assigned a value.  This
# allows the caller to do something like $_rl_prefix=$withval if the user
# specifies --with-installed-readline=PREFIX as an argument to configure

if test -z "$ac_cv_rl_prefix"; then
test "x$prefix" = xNONE && ac_cv_rl_prefix=$ac_default_prefix || ac_cv_rl_prefix=${prefix}
fi

eval ac_cv_rl_includedir=${ac_cv_rl_prefix}/include
eval ac_cv_rl_libdir=${ac_cv_rl_prefix}/lib

LIBS="$LIBS -lreadlinecpp ${TERMCAP_LIB}"
CFLAGS="$CFLAGS -I${ac_cv_rl_includedir}"
LDFLAGS="$LDFLAGS -L${ac_cv_rl_libdir}"

AC_CACHE_VAL(ac_cv_rl_version,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdio.h>
#include <readline/readline.h>
#include <stdlib.h>

extern int rl_gnu_readline_p;

int
main()
{
	FILE *fp;
	fp = fopen("conftest.rlv", "w");
	if (fp == 0)
		exit(1);
	if (rl_gnu_readline_p != 1)
		fprintf(fp, "0.0\n");
	else
		fprintf(fp, "%s\n", rl_library_version ? rl_library_version : "0.0");
	fclose(fp);
	exit(0);
}
]])],[ac_cv_rl_version=`cat conftest.rlv`],[ac_cv_rl_version='0.0'],[ac_cv_rl_version='8.0'])])

CFLAGS="$_save_CFLAGS"
LDFLAGS="$_save_LDFLAGS"
LIBS="$_save_LIBS"

RL_MAJOR=0
RL_MINOR=0

# (
case "$ac_cv_rl_version" in
2*|3*|4*|5*|6*|7*|8*|9*)
	RL_MAJOR=`echo $ac_cv_rl_version | sed 's:\..*$::'`
	RL_MINOR=`echo $ac_cv_rl_version | sed -e 's:^.*\.::' -e 's:[[a-zA-Z]]*$::'`
	;;
esac

# (((
case $RL_MAJOR in
[[0-9][0-9]])	_RL_MAJOR=$RL_MAJOR ;;
[[0-9]])	_RL_MAJOR=0$RL_MAJOR ;;
*)		_RL_MAJOR=00 ;;
esac

# (((
case $RL_MINOR in
[[0-9][0-9]])	_RL_MINOR=$RL_MINOR ;;
[[0-9]])	_RL_MINOR=0$RL_MINOR ;;
*)		_RL_MINOR=00 ;;
esac

RL_VERSION="0x${_RL_MAJOR}${_RL_MINOR}"

# Readline versions greater than 4.2 have these defines in readline.h

if test $ac_cv_rl_version = '0.0' ; then
	AC_MSG_WARN([Could not test version of installed readline library.])
elif test $RL_MAJOR -gt 4 || { test $RL_MAJOR = 4 && test $RL_MINOR -gt 2 ; } ; then
	# set these for use by the caller
	RL_PREFIX=$ac_cv_rl_prefix
	RL_LIBDIR=$ac_cv_rl_libdir
	RL_INCLUDEDIR=$ac_cv_rl_includedir
	AC_MSG_RESULT($ac_cv_rl_version)
else

AC_DEFINE_UNQUOTED(RL_READLINE_VERSION, $RL_VERSION, [encoded version of the installed readline library])
AC_DEFINE_UNQUOTED(RL_VERSION_MAJOR, $RL_MAJOR, [major version of installed readline library])
AC_DEFINE_UNQUOTED(RL_VERSION_MINOR, $RL_MINOR, [minor version of installed readline library])

AC_SUBST(RL_VERSION)
AC_SUBST(RL_MAJOR)
AC_SUBST(RL_MINOR)

# set these for use by the caller
RL_PREFIX=$ac_cv_rl_prefix
RL_LIBDIR=$ac_cv_rl_libdir
RL_INCLUDEDIR=$ac_cv_rl_includedir

AC_MSG_RESULT($ac_cv_rl_version)

fi
])

AC_DEFUN([BASH_FUNC_CTYPE_NONASCII],
[
AC_MSG_CHECKING(whether the ctype macros accept non-ascii characters)
AC_CACHE_VAL(bash_cv_func_ctype_nonascii,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

int
main(int c, char *v[])
{
	char	*deflocale;
	unsigned char x;
	int	r1, r2;

#ifdef HAVE_SETLOCALE
	/* We take a shot here.  If that locale is not known, try the
	   system default.  We try this one because '\342' (226) is
	   known to be a printable character in that locale. */
	deflocale = setlocale(LC_ALL, "en_US.ISO8859-1");
	if (deflocale == 0)
		deflocale = setlocale(LC_ALL, "");
#endif

	x = '\342';
	r1 = isprint(x);
	x -= 128;
	r2 = isprint(x);
	exit (r1 == 0 || r2 == 0);
}
]])],[bash_cv_func_ctype_nonascii=yes],[bash_cv_func_ctype_nonascii=no],[AC_MSG_WARN(cannot check ctype macros if cross compiling -- defaulting to no)
    bash_cv_func_ctype_nonascii=no
])])
AC_MSG_RESULT($bash_cv_func_ctype_nonascii)
if test $bash_cv_func_ctype_nonascii = yes; then
AC_DEFINE([CTYPE_NON_ASCII], 1, [Define if the ctype macros accept non-ASCII characters.])
fi
])

AC_DEFUN([BASH_CHECK_WCONTINUED],
[
AC_MSG_CHECKING(whether WCONTINUED flag to waitpid is unavailable or available but broken)
AC_CACHE_VAL(bash_cv_wcontinued_broken,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#ifndef errno
extern int errno;
#endif
int
main()
{
	int	x;

	x = waitpid(-1, (int *)0, WNOHANG|WCONTINUED);
	if (x == -1 && errno == EINVAL)
		exit (1);
	else
		exit (0);
}
]])],[bash_cv_wcontinued_broken=no],[bash_cv_wcontinued_broken=yes],[AC_MSG_WARN(cannot check WCONTINUED if cross compiling -- defaulting to no)
    bash_cv_wcontinued_broken=no
])])
AC_MSG_RESULT($bash_cv_wcontinued_broken)
if test $bash_cv_wcontinued_broken = yes; then
AC_DEFINE([WCONTINUED_BROKEN], 1, [Define if your WCONTINUED flag for waitpid is unavailable or available but broken.])
fi
])

dnl
dnl tests added for bashdb
dnl


AC_DEFUN([AM_PATH_LISPDIR],
 [AC_ARG_WITH(lispdir, AS_HELP_STRING([--with-lispdir],[override the default lisp directory]),
  [ lispdir="$withval"
    AC_MSG_CHECKING([where .elc files should go])
    AC_MSG_RESULT([$lispdir])],
  [
  # If set to t, that means we are running in a shell under Emacs.
  # If you have an Emacs named "t", then use the full path.
  test x"$EMACS" = xt && EMACS=
  AC_CHECK_PROGS(EMACS, emacs xemacs, no)
  if test $EMACS != "no"; then
    if test x${lispdir+set} != xset; then
      AC_CACHE_CHECK([where .elc files should go], [am_cv_lispdir], [dnl
	am_cv_lispdir=`$EMACS -batch -q -eval '(while load-path (princ (concat (car load-path) "\n")) (setq load-path (cdr load-path)))' | sed -n -e 's,/$,,' -e '/.*\/lib\/\(x\?emacs\/site-lisp\)$/{s,,${libdir}/\1,;p;q;}' -e '/.*\/share\/\(x\?emacs\/site-lisp\)$/{s,,${datadir}/\1,;p;q;}'`
	if test -z "$am_cv_lispdir"; then
	  am_cv_lispdir='${datadir}/emacs/site-lisp'
	fi
      ])
      lispdir="$am_cv_lispdir"
    fi
  fi
 ])
 AC_SUBST(lispdir)
])

AC_DEFUN([BASH_STRUCT_WEXITSTATUS_OFFSET],
[AC_MSG_CHECKING(for offset of exit status in return status from wait)
AC_CACHE_VAL(bash_cv_wexitstatus_offset,
[AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <stdlib.h>
#include <unistd.h>

#include <sys/wait.h>

int
main(int c, char **v)
{
  pid_t pid, p;
  int s, i, n;

  s = 0;
  pid = fork();
  if (pid == 0)
    exit (42);

  /* wait for the process */
  p = wait(&s);
  if (p != pid)
    exit (255);

  /* crack s */
  for (i = 0; i < (sizeof(s) * 8); i++)
    {
      n = (s >> i) & 0xff;
      if (n == 42)
	exit (i);
    }

  exit (254);
}
]])],[bash_cv_wexitstatus_offset=0],[bash_cv_wexitstatus_offset=$?],[AC_MSG_WARN(cannot check WEXITSTATUS offset if cross compiling -- defaulting to 0)
    bash_cv_wexitstatus_offset=0
])])
if test "$bash_cv_wexitstatus_offset" -gt 32 ; then
  AC_MSG_WARN(bad exit status from test program -- defaulting to 0)
  bash_cv_wexitstatus_offset=0
fi
AC_MSG_RESULT($bash_cv_wexitstatus_offset)
AC_DEFINE_UNQUOTED([WEXITSTATUS_OFFSET], [$bash_cv_wexitstatus_offset], [Offset of exit status in wait status word])
])

dnl Useful macros to check libraries which are not implicit
dnl in Solaris, for instance. Originally by Thomas E. Dickey.
AC_DEFUN([BASH_LIB_NSL],
[
AC_CHECK_LIB(nsl, gethostbyname,
[
AC_MSG_CHECKING(if libnsl is mandatory)
AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([
#include <sys/types.h>
#include <netdb.h>
char *domain; ],
        [gethostbyname(domain)])], dnl
 [AC_MSG_RESULT(no)], dnl
 [AC_MSG_RESULT(yes); LIBS="${LIBS} -lnsl"])
])
])

AC_DEFUN([BASH_LIB_SOCKET],
[
AC_CHECK_LIB(socket, socket,
[
AC_MSG_CHECKING(if libsocket is mandatory)
AC_LINK_IFELSE(
        [AC_LANG_PROGRAM([
#include <sys/types.h>
#include <sys/socket.h>],
        [socket (AF_INET, SOCK_STREAM, 0) ])], dnl
 [AC_MSG_RESULT(no)], dnl
 [AC_MSG_RESULT(yes); LIBS="${LIBS} -lsocket"])
])
])
