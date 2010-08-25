
/***********************************************************************
 *                                                                     *
 *                     Copyright (c) 1984, Fred Fish                   *
 *                         All Rights Reserved                         *
 *                                                                     *
 *     This software and/or documentation is released for public       *
 *     distribution for personal, non-commercial use only.             *
 *     Limited rights to use, modify, and redistribute are hereby      *
 *     granted for non-commercial purposes, provided that all          *
 *     copyright notices remain intact and all changes are clearly     *
 *     documented.  The author makes no warranty of any kind with      *
 *     respect to this product and explicitly disclaims any implied    *
 *     warranties of merchantability or fitness for any particular     *
 *     purpose.                                                        *
 *                                                                     *
 ***********************************************************************
 */
 
/************************************************************************
 *                                                                      *
 * Note that this file has undergone substantial rewriting by the       *
 * SAC team!!                                                           *
 *                                                                      *
 ************************************************************************
 */
 

/*
 *  FILE 
 *
 *     dbug.h    user include file for programs using the dbug package
 *
 *  SYNOPSIS
 *
 *     #include <dbug.h>                    (modified, 20.8.87, Strobl)
 *
 *
 *  SCCS ID
 *
 *     @(#)dbug.h      1.9 10/29/86
 *
 *  DESCRIPTION
 *
 *     Programs which use the dbug package must include this file.
 *     It contains the appropriate macros to call support routines
 *     in the dbug runtime library.
 *
 *     To disable compilation of the macro expansions define the
 *     preprocessor symbol "DBUG_OFF".  This will result in null
 *     macros expansions so that the resulting code will be smaller
 *     and faster.  (The difference may be smaller than you think
 *     so this step is recommended only when absolutely necessary).
 *     In general, tradeoffs between space and efficiency are
 *     decided in favor of efficiency since space is seldom a
 *     problem on the new machines).
 *
 *     All externally visible symbol names follow the pattern
 *     "_db_xxx..xx_" to minimize the possibility of a dbug package
 *     symbol colliding with a user defined symbol.
 *
 *     The DBUG_<N> style macros are obsolete and should not be used
 *     in new code.  Macros to map them to instances of DBUG_PRINT
 *     are provided for compatibility with older code.  They may go
 *     away completely in subsequent releases.
 *
 *  AUTHOR
 *
 *     Fred Fish
 *     (Currently employed by Motorola Computer Division, Tempe, Az.)
 *     (602) 438-5976
 *
 */
 

/*
 *     Internally used dbug variables which must be global.
 */
 

#include <stdio.h>
#include <stdlib.h>

#ifndef DBUG_OFF
    extern int _db_on_;                    /* TRUE if debug currently enabled */
    extern int _db_dummy_;                 /* dummy for fooling macro preprocessor */
    extern FILE *_db_fp_;                  /* Current debug output stream */
    extern char *_db_process_;             /* Name of current process */
    extern int _db_keyword_ (char *);      /* Accept/reject keyword */
    extern void _db_push_ (char *);        /* Push state, set up new state */
    extern void _db_pop_ (void);           /* Pop previous debug state */
    extern void _db_enter_ (char *, char *, int, char **, char **, int *);
                                           /* New user function entered */
    extern void _db_return_ (int, char **, char **, int *);
                                           /* User function return */
    extern void _db_pargs_ (int, char*);   /* Remember args for line */
    extern void _db_doprnt_ (char *, ...);
                                           /* Print debug output */
    extern void _db_doprnt_assert_1_ (char *, int, char *);   
                                           /* Print debug output */
    extern void _db_doprnt_assert_2_ (char *, ...);
                                           /* Print debug output */
    extern void _db_setjmp_ (void);        /* Save debugger environment */
    extern void _db_longjmp_ (void);       /* Restore debugger environment */
# endif
 

/*
 *     These macros provide a user interface into functions in the
 *     dbug runtime support library.  They isolate users from changes
 *     in the MACROS and/or runtime support.
 *
 *     The symbols "__LINE__" and "__FILE__" are expanded by the
 *     preprocessor to the current source file line number and file
 *     name respectively.
 *
 *     WARNING ---  Because the DBUG_ENTER macro allocates space on
 *     the user function's stack, it must precede any executable
 *     statements in the user function.
 *
 */
 
#ifdef DBUG_OFF
#define DBUG_ENTER(a1)
#define DBUG_RETURN(a1) return(a1)
#define DBUG_VOID_RETURN return
#define DBUG_EXECUTE(keyword,a1)
#define DBUG_DO_NOT_EXECUTE(keyword,a1) a1
#define DBUG_PRINT(keyword,arglist)
#define DBUG_PUSH(a1)
#define DBUG_POP()
#define DBUG_PROCESS(a1)
#define DBUG_FILE (stderr)
#define DBUG_SETJMP setjmp
#define DBUG_LONGJMP longjmp
#define DBUG_ASSERT(p,q)
#define DBUG_ASSERTF(p,q)
#define DBUG_ASSERT_EXPR(p,q,r) (r)
#define DBUG_LPRINT(key1,key2,arglist)
#define DBUG_PRINTE( keyword, arglist ) \
    {printf arglist;printf("\n");fflush(stdout);}
#else
#define NOOP _db_dummy_=0
#define DBUG_ENTER(a) \
       auto char *_db_func_, *_db_file_; \
      int _db_level_; \
      _db_enter_ (a,__FILE__,__LINE__,&_db_func_,&_db_file_,&_db_level_)
#define DBUG_LEAVE \
      (_db_return_ (__LINE__, &_db_func_, &_db_file_, &_db_level_))
#if 0    /* to be set 0 whenever non constant returnvalues are to be used */
#define DBUG_RETURN(a1) if (1) { DBUG_PRINT("RETURN", ("return value %d=%08x", a1, a1)); return (DBUG_LEAVE, (a1)); } else NOOP
#else
#define DBUG_RETURN(a1) return (DBUG_LEAVE, (a1))
#endif

#define DBUG_VOID_RETURN {DBUG_LEAVE; return;}
#define DBUG_EXECUTE(keyword,a1) \
      if ((_db_on_ && _db_keyword_ (keyword))) { a1 } else NOOP
#define DBUG_DO_NOT_EXECUTE(keyword,a1) \
      if ((_db_on_ && !_db_keyword_ (keyword))) { a1 } else NOOP
#define DBUG_PRINT(keyword,arglist) \
      if (_db_on_) { _db_pargs_(__LINE__,keyword); _db_doprnt_ arglist;} else NOOP
#define DBUG_PUSH(a1) _db_push_ (a1)
#define DBUG_POP() _db_pop_ ()
#define DBUG_PROCESS(a1) (_db_process_ = a1)
#define DBUG_FILE (_db_fp_)
#define DBUG_SETJMP(a1) (_db_setjmp_ (), setjmp (a1))
#define DBUG_LONGJMP(a1,a2) (_db_longjmp_ (), longjmp (a1, a2))

#define DBUG_ASSERTF(expr,text)                                 \
        if (!(expr)) {                                          \
          _db_doprnt_assert_1_( __FILE__,__LINE__, NULL);       \
          _db_doprnt_assert_2_ text ;                           \
          abort();                                              \
        } else NOOP

#define DBUG_ASSERT(expr,text)                                  \
        if (!(expr)) {                                          \
          _db_doprnt_assert_1_( __FILE__,__LINE__, text);       \
          abort();                                              \
        } else NOOP

#define DBUG_ASSERT_EXPR(expr,text,val)                                              \
        (!(expr) ? ( fprintf(stderr,     	                                     \
                             "Assertion 'expr' failed: file '%s', line %d\n** %s\n", \
                             __FILE__,__LINE__,text ),                               \
                     exit(1), val)                                                   \
          : val)

#define DBUG_LPRINT(key1,key2,arglist) \
       {if (_db_on_) {_db_lpargs_(__LINE__, key1, key2); _db_ldoprnt_ arglist;}}
#define DBUG_PRINTE( keyword, arglist ) \
        ( printf arglist,printf("\n"),fflush(stdout) )
# endif



