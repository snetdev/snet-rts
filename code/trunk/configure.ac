AC_INIT([SNet], [1.x], [info@snet-home.org])
AC_PREREQ([2.68])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 tar-ustar no-define foreign dist-bzip2 subdir-objects
                  parallel-tests color-tests silent-rules])
AM_SILENT_RULES([yes])

AC_PROG_CC_C99
AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CPP

AC_PROG_YACC
AC_PROG_LEX

AC_LANG_PUSH([C])

AX_PTHREAD
LIBS="$LIBS $PTHREAD_LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

AC_CHECK_FUNCS([pthread_setaffinity_np])

AC_SEARCH_LIBS([sqrt], [m])

dnl check network functions
AC_SEARCH_LIBS([socket], [socket])
AC_SEARCH_LIBS([gethostbyname], [nsl])
AC_SEARCH_LIBS([inet_aton], [resolv])


dnl check for compiler builtins for
dnl atomic memory access (__sync_fetch_and_add/dec)
dnl !!! we need to try to link the program, on platforms on which
dnl !!! these functions are not available, they are undefined references
AC_MSG_CHECKING([atomic compiler builtins])
have_compiler_atomics=no
AC_TRY_LINK([], [
    int a = 0;
    (void) __sync_fetch_and_add(&a, 1);
    (void) __sync_fetch_and_sub(&a, 1);
    (void) __sync_lock_test_and_set(&a, 42);
  ],
  [have_compiler_atomics=yes])
AC_MSG_RESULT([$have_compiler_atomics])
if test $have_compiler_atomics = yes; then
  AC_DEFINE([HAVE_SYNC_ATOMIC_BUILTINS], [1], [Define to 1 if compiler builtins for atomic memory access are available.])
fi

AC_LANG_POP([C])

LT_PREREQ([2.2])
LT_INIT([dlopen])

AC_ARG_WITH([lpel-includes], [AC_HELP_STRING([--with-lpel-includes=DIR], [look up lpel.h in DIR])], [], [with_lpel_includes=])
AC_MSG_CHECKING([for lpel.h in $with_lpel_includes])
if test -r "$with_lpel_includes/lpel.h"; then
   with_lpel_includes=`(cd "$with_lpel_includes"; pwd)`
   AC_MSG_RESULT([yes])
else
   AC_MSG_RESULT([no])
   AC_MSG_WARN([unable to find $with_lpel_includes/lpel.h, cannot use LPEL])
   with_lpel_includes=
fi

AC_ARG_WITH([lpel-libs], [AC_HELP_STRING([--with-lpel-libs=DIR], [look up liblpel.la in DIR])], [], [with_lpel_libs=])
AC_MSG_CHECKING([for liblpel.la in $with_lpel_libs])
if test -r "$with_lpel_libs/liblpel.la"; then
   with_lpel_libs=`(cd "$with_lpel_libs"; pwd)`
   AC_MSG_RESULT([yes])
else
   AC_MSG_RESULT([no])
   AC_MSG_WARN([unable to find $with_lpel_libs/liblpel.la, cannot use LPEL])
   with_lpel_libs=
fi
have_lpel=no
if test -n "$with_lpel_includes" -a -n "$with_lpel_libs"; then
   LPEL_INCLUDES=-I$with_lpel_includes
   LIBLPEL_LA=$with_lpel_libs/liblpel.la
   AC_SUBST([LIBLPEL_LA])
   AC_SUBST([LPEL_INCLUDES])
   have_lpel=yes
fi
AM_CONDITIONAL([HAVE_LPEL], [test $have_lpel = yes])

AX_MPI([have_mpi=yes],[have_mpi=no])
AM_CONDITIONAL([HAVE_MPI], [test $have_mpi = yes])

AC_CHECK_PROGS([SAC2C], [sac2c])
AC_CHECK_PROGS([SAC4C], [sac4c])

AC_OUTPUT([Makefile])
