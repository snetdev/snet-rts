dnl
dnl $Id$
dnl

dnl configure script for building snet compiler 
dnl on different hosts and os

dnl tell configure who we are
dnl parameters: package name, version, where to send bugreports
AC_INIT(snetc, svn, c.grelck@herts.ac.uk)

dnl set source directory and scripting directory
AC_CONFIG_SRCDIR(.)
AC_CONFIG_AUX_DIR(./config)

dnl we use C as our compiler language
AC_LANG(C)

dnl and which files to create
AC_CONFIG_FILES(src/makefiles/config.mkf)
AC_CONFIG_HEADER(src/compiler/global/config.h)

dnl check for host type
AC_CANONICAL_BUILD()
AC_CANONICAL_HOST()
AC_CANONICAL_TARGET()

dnl set OS and ARCH flags
AC_DEFINE_UNQUOTED(OS,"$target_os",set to the os type)
AC_SUBST(OS,$target_os)
AC_DEFINE_UNQUOTED(ARCH,"$target_cpu",set to the architecture)
AC_SUBST(ARCH,$target_cpu)

dnl search for a C compiler
AC_PROG_CC(gcc cc)

dnl check whether its suns cc
AC_MSG_CHECKING(whether cc is a Sun Pro C compiler)
AC_TRY_COMPILE(,
[#ifndef __SUNPRO_C
  choke me
#endif],
AC_MSG_RESULT(yes)
SUNC=yes,
AC_MSG_RESULT(no)
suNC=no
)

dnl check whether its compaqs (or dec) cc
AC_MSG_CHECKING(whether cc is a Compaq/DEC C compiler)
AC_TRY_COMPILE(,
[#ifndef __DECC
  choke me
#endif],
AC_MSG_RESULT(yes)
DECC=yes,
AC_MSG_RESULT(no)
DECC=no
)

dnl check whether its apples cc
AC_MSG_CHECKING(whether cc is a Apple compiler)
AC_TRY_COMPILE(,
[#ifndef __APPLE__
  choke me
  #endif],
  AC_MSG_RESULT(yes)
  MACC=yes
  GCC=no,
  AC_MSG_RESULT(no)
  MACC=no
)
  
dnl check whether popen is an extension
AC_MSG_CHECKING([need of _EXTENSIONS_ flag])
AC_TRY_COMPILE([#include <stdio.h>],[popen("rw","testfile")],
AC_MSG_RESULT(no)
need_ext_flags=no,
AC_MSG_RESULT(yes)
need_ext_flags=yes)

dnl check for C preprocessor
AC_PROG_CPP()

dnl check for ranlib
AC_PROG_RANLIB()
if test "$RANLIB"; then
  AC_MSG_CHECKING([whether ranlibs needs extra argument])
  case "$target_os" in
    *darwin*) RANLIB="$RANLIB -c"
    AC_MSG_RESULT([adding -c flag for OS X])
    ;;
    *)
    AC_MSG_RESULT([no])
    ;;
  esac
fi

dnl check, whether we need -traditional flag
AC_PROG_GCC_TRADITIONAL

dnl check whether its gnu indent
AC_MSG_CHECKING([whether GNU indent works])
GNUINDENT=$(indent --version 2>/dev/null)
case "$GNUINDENT" in
  *GNU*) 
    CB="indent"
    AC_MSG_RESULT(yes)
    ;;
  *) 
    GNUINDENT=$(gindent --version 2>/dev/null)
    case "$GNUINDENT" in
    *GNU*)
      CB="gindent"
      AC_MSG_RESULT(yes)
      ;;
    *)
      CB="\$(PROJECT_ROOT)/src/bin/cb"
      AC_MSG_RESULT(no)
      ;;
    esac
esac
AC_SUBST(CB, $CB)

dnl search for bison, byacc or yacc
AC_PROG_YACC()

dnl search for flex or lex
AC_PROG_LEX()

dnl search for xlst engine
AC_CHECK_PROGS(XSLT, [xsltproc sabcmd], [!fail])

dnl search for m4 macro processor
AC_CHECK_PROGS(M4, [gm4 m4], [!fail])

dnl search for pk gtk configuration script 
AC_CHECK_PROGS(PKGCONF, [pkg-config], [!fail])
AC_SUBST(GTKLIBS, `pkg-config --libs gtk+-2.0`)
AC_SUBST(GTKFLAGS, `pkg-config --cflags gtk+-2.0`)

dnl search for malloc.h
AC_CHECK_HEADERS(malloc.h)

dnl check for mkdtemp
AC_CHECK_FUNCS(mkdtemp)

dnl check whether to link dlopen library
AC_SEARCH_LIBS(dlopen, dl)

dnl checking for presence of strtok and strrchr
AC_CHECK_FUNCS([strtok strrchr])


dnl check if printf generates leading 0x for pointers
AC_MSG_CHECKING(for 0x prefix when printing pointers)
AC_EGREP_CPP(0x0*,
[#include <stdio.h>
int main()
{
printf("%p", (void*) 0);
}
], has_ptr_prefix=yes, has_ptr_prefix=no)
if [ test $has_ptr_prefix = yes ]; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_DEFINE(NEED_PTR_PREFIX, 1, set to 1 if printf does not print a leading
0x in front of pointers)
fi

dnl check if stderr is a constant (needed for dbug.c)
AC_MSG_CHECKING(whether stderr is a constant)
AC_TRY_COMPILE([
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

FILE *_db_fp_ = stderr;
],[],stderr_is_constant=yes,stderr_is_constant=no)
if [ test $stderr_is_constant = yes ]; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(STDERR_IS_CONSTANT, 1, set to 1 if stderr is a constant)
else
  AC_MSG_RESULT(no)
fi

dnl check for pthread flag on gcc
if test "$GCC" = yes; then
  AC_MSG_CHECKING([existence of pthread flag])
  echo "int main(){return 0;}" > .sac_configure_temp.c
  if test "`gcc -o .snet_configure_temp -pthread .snet_configure_temp.c 2>&1 | grep "unrecognized option"`" = ""; then
    AC_MSG_RESULT([supported])
    GCC_PTHREADS="-pthread"
  else
    AC_MSG_RESULT([unsupported])
    GCC_PTHREADS="-lpthread"
  fi
  rm -f .sac_configure_temp.c .sac_configure_temp
fi

dnl check whether we need to link libm
AC_CHECK_LIB(,sqrt,
    AC_SUBST( LIB_M, ""),
    AC_SUBST( LIB_M, "-lm"))

dnl check for socket libs
AC_CHECK_LIB(,bind,
    AC_SUBST( LIB_SOCKET, ""),
    AC_CHECK_LIB(socket,bind,
      AC_SUBST( LIB_SOCKET, "-lsocket -lnsl -lresolv"),
      [!fail]))

dnl test for compiler flags
dnl for sac2crc
AC_MSG_CHECKING([compiler flags])
if [ test "$GCC" = yes ]; then
  AC_SUBST(OPT_O0, [""])
  AC_SUBST(OPT_O1, [-O1])
  AC_SUBST(OPT_O2, [-O2])
  AC_SUBST(OPT_O3, [-O3])
  AC_SUBST(OPT_g, [-g])
  AC_SUBST(RCLDFLAGS, [""])
  AC_SUBST(RCCCFLAGS, ["-Wall -Wno-unused -fno-builtin -fPIC -DPIC"])
  AC_SUBST(MKCCFLAGS, ["-Wall -g -std=c99"])
  AC_SUBST(PDCCFLAGS, ["-Wall -g -O3 -std=c99"])
  AC_SUBST(DEPSFLAG, ["-M"])
  AC_SUBST(CPPFILE, ["gcc -E -C -x c"])
  AC_SUBST(CCMTLINK, [$GCC_PTHREADS])
  AC_MSG_RESULT([gcc settings])
elif [ test "$SUNC" = yes ]; then
  AC_SUBST(OPT_O0, [""])
  AC_SUBST(OPT_O1, [-xO2])
  AC_SUBST(OPT_O2, [-xO4])
  AC_SUBST(OPT_O3, [-xO5])
  AC_SUBST(OPT_g, [-g])
  AC_SUBST(RCLDFLAGS, [""])
  AC_SUBST(RCCCFLAGS, ["-dalign -fsimple -xsafe=mem"])
  AC_SUBST(MKCCFLAGS, ["-erroff=E_CAST_DOESNT_YIELD_LVALUE -g"])
  AC_SUBST(PDCCFLAGS, ["-erroff=E_CAST_DOESNT_YIELD_LVALUE -g -xO4"])
  AC_SUBST(DEPSFLAG, ["-xM"])
  AC_SUBST(CPPFILE, ["cc -E -C -x c"])
  AC_SUBST(CCMTLINK, ["-lpthread"])
  AC_MSG_RESULT([Sun cc settings])
elif [ test "$DECC" = yes ]; then
  AC_SUBST(OPT_O0, [""])
  AC_SUBST(OPT_O1, [-O1])
  AC_SUBST(OPT_O2, [-O2])
  AC_SUBST(OPT_O3, [-O3])
  AC_SUBST(OPT_g, [-g])
  AC_SUBST(RCLDFLAGS, [""])
  AC_SUBST(RCCCFLAGS, [""])
  AC_SUBST(MKCCFLAGS, ["-g"])
  AC_SUBST(PDCCFLAGS, ["-g3"])
  AC_SUBST(DEPSFLAG, ["-M"])
  AC_SUBST(CPPFILE, ["cc -E -C -x c"])
  AC_SUBST(CCMTLINK, ["-pthread"])
  AC_MSG_RESULT([Compaq/DEC cc settings])
elif [ test "$MACC" = yes ]; then
  AC_SUBST(OPT_O0, [""])
  AC_SUBST(OPT_O1, [-O1])
  AC_SUBST(OPT_O2, [-O2])
  AC_SUBST(OPT_O3, [-O3])
  AC_SUBST(OPT_g, [-g])
  AC_SUBST(RCLDFLAGS, [""])
  AC_SUBST(RCCCFLAGS, ["-Wall -no-cpp-precomp -Wno-unused -fno-builtin"])
  AC_SUBST(MKCCFLAGS, ["-g -Wall -std=c99"])
  AC_SUBST(PDCCFLAGS, ["-g -std=c99"])
  AC_SUBST(DEPSFLAG, ["-M"])
  AC_SUBST(CPPFILE, ["cpp3 -C -traditional-cpp"])
  AC_SUBST(CCMTLINK, [""])
  AC_MSG_RESULT([Apple cc settings])
else
  AC_SUBST(OPT_O0, [""])
  AC_SUBST(OPT_O1, [""])
  AC_SUBST(OPT_O2, [""])
  AC_SUBST(OPT_O3, [""])
  AC_SUBST(OPT_g, [""])
  AC_SUBST(RCLDFLAGS, [""])
  AC_SUBST(RCCCFLAGS, [""])
  AC_SUBST(MKCCFLAGS, [""])
  AC_SUBST(PDCCFLAGS, [""])
  AC_SUBST(DEPSFLAG, ["-M"])
  AC_SUBST(CPPFILE, ["cpp -C"])
  AC_SUBST(CCMTLINK, [""])
  AC_MSG_RESULT([none found])
fi   

dnl generate specific flags for known os
AC_MSG_CHECKING([sac2crc system flags])
OSFLAGS=""
case "$target_os" in
  solaris*)     
    OSFLAGS=["-D__EXTENSIONS__ -DMUST_INIT_YY -fPIC -DPIC"]
    LD_DYNAMIC=["ld -dy -G"]
    LD_PATH=["-R"]
    AC_MSG_RESULT([solaris settings])
    ;;
  *linux*)      
    OSFLAGS=["-fPIC -DPIC -D_POSIX_SOURCE -D_SVID_SOURCE -D_BSD_SOURCE -Dlint"]
    LD_DYNAMIC=["ld -G -dy -shared -allow-shlib-undefined"]
    LD_PATH=["-Wl,-rpath,"]
    AC_MSG_RESULT([linux settings])
    ;;
  *osf*)	
    OSFLAGS=["-D_OSF_SOURCE"]
    LD_DYNAMIC=[""]
    LD_PATH=[""]
    AC_MSG_RESULT([OSF settings])
    ;;
  *darwin*)      
    OSFLAGS=["-no-cpp-precomp -Wno-long-double"]
    LD_DYNAMIC=["ld -undefined suppress -flat_namespace -bundle /usr/lib/bundle1.o"]
    LD_PATH=["-install_name"]
    AC_MSG_RESULT([OS X settings])
    ;;
  *bsd*)
    OSFLAGS=["-fpic"]
    LD_DYNAMIC=["ld -dy -shared -symbolic"]
    LD_PATH=["-Wl,-rpath,"]
    AC_MSG_RESULT([BSD settings])
    ;;
  *)
    AC_MSG_RESULT([unknown])
    ;;
esac
AC_SUBST(OSFLAGS)
AC_SUBST(LD_DYNAMIC)
AC_SUBST(LD_PATH)

dnl 
dnl static sac2c settings for config.h
dnl


dnl generate configure dependent files

AC_OUTPUT()
