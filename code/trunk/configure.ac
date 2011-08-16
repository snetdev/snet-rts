AC_INIT([SNet], [1.x], [info@snet-home.org])
AC_PREREQ([2.68])

AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux])

AM_INIT_AUTOMAKE([1.11 tar-ustar no-define foreign dist-bzip2 subdir-objects
                  parallel-tests color-tests silent-rules])
AM_SILENT_RULES([yes])

dnl If --enable-dist-mpi=auto is used, try to find MPI, but use standard C compiler if it is not found.
dnl If --enable-dist-mpi=yes is used, try to find MPI and fail if it isn't found.
dnl If --enable-dist-mpi=no is used, use a standard C compiler instead.
AC_ARG_ENABLE([dist-mpi], [AS_HELP_STRING([--enable-dist-mpi],
    [Enable the distribution layer using MPI. Set to "yes", "no" (or --disable), or "auto" (default). If "auto", enable if MPI is available.])],
    [], [enable_dist_mpi=auto])

dnl try to detect the MPI compiler before any other C tests are run. This 
dnl MUST appear before other C tests / configurations so that they can
dnl pick `mpicc' as compiler name.

use_mpi=no
AX_PROG_CC_MPI([test x"$enable_dist_mpi" != xno], [use_mpi=yes],
               [if test x"$with_mpi" = xyes; then
  AC_MSG_FAILURE([MPI compiler requested, but couldn't use MPI.])
else
  AC_MSG_WARN([No MPI compiler found, won't use MPI.])
fi])
AM_CONDITIONAL([ENABLE_DIST_MPI], [test $use_mpi = yes])

dnl ensure that either mpicc or cc detected above are configured in c99 mode
AC_PROG_CC_C99 
dnl and API extensions are available. 
AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CPP

AC_PROG_YACC
AC_PROG_LEX

AC_LANG_PUSH([C])

AX_PTHREAD
LIBS="$LIBS $PTHREAD_LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

AC_CHECK_FUNCS([pthread_setaffinity_np])
AC_CHECK_FUNCS([pthread_yield_np])
AC_CHECK_FUNCS([pthread_yield])

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
AC_SUBST([LIBTOOL_DEPS])



dnl check for building lpel threading backend

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
AM_CONDITIONAL([ENABLE_TH_LPEL], [test $have_lpel = yes])



dnl check for sac compiler in order to build sac4snet interface

AC_CHECK_PROGS([SAC2C], [sac2c])
AC_CHECK_PROGS([SAC4C], [sac4c])
have_sac=no
if test -n "$SAC2C" -a -n "$SAC4C"; then
  have_sac=yes
fi
AM_CONDITIONAL([ENABLE_SAC4SNET], [test $have_sac = yes])



dnl generate output

AC_OUTPUT([Makefile src/interfaces/sac4snet/Makefile])

