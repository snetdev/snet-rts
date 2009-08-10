/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : common.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header file with symbols macros and declarations
                     common to all modules.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_CORE_COMMON_H
#define __SVPSNETGWRT_CORE_COMMON_H

/**
 * Version
 */
#define SVPSNETGWRT_VER 100

/*----------------------------------------------------------------------------*/
/**
 * Include the default configuration header file
 * if no other has been included yet.
 */
#ifndef SVPSNETGWRT_CFG_HEADER
#include "config.utc.h"
#endif

/*----------------------------------------------------------------------------*/
/**
 * Compiler
 */

#ifdef __GNUC__
#define SVPSNETGWRT_COMP_GCC
#ifdef  SVPSNETGWRT_SVP_PLATFORM_UTCPTL
#define ___GNUC___            __GNUC__
#define ___GNUC_MINOR___      __GNUC_MINOR__
#define ___GNUC_PATCHLEVEL___ __GNUC_PATCHLEVEL__

#undef  __GNUC__
#undef  __GNUC_MINOR__
#undef  __GNUC_PATCHLEVEL__

$define SVPSNETGWRT_COMP_VER \
    (__GNUC__ * 10000 +      \
     __GNUC_MINOR__ * 100 +  __GNUC_PATCHLEVEL__)

#define __GNUC__            ___GNUC___
#define __GNUC_MINOR__      ___GNUC_MINOR___
#define __GNUC_PATCHLEVEL__ ___GNUC_PATCHLEVEL___

#else
#define SVPSNETGWRT_COMP_VER \
    (__GNUC__ * 10000     +  \
     __GNUC_MINOR__ * 100 +  __GNUC_PATCHLEVEL__)
#endif // SVPSNETGWRT_SVP_PLATFORM_UTCPTL

#else  // not GCC

#ifdef __EUTCC__
#define SVPSNETGWRT_COMP_EUTCC
#define SVPSNETGWRT_COMP_VER 100

#else
#define SVPSNETGWRT_COMP_UNKNOWN
#define SVPSNETGWRT_COMP_VER 0

#endif // __EUTCC__
#endif // __GNUC__

/*----------------------------------------------------------------------------*/
/**
 * Configure SVP platform flags
 */

#ifndef SVPSNETGWRT_SVP_PLATFORM_UTCPTL
#ifndef SVPSNETGWRT_SVP_PLATFORM_DUTCPTL
#ifndef SVPSNETGWRT_SVP_PLATFORM_MGRID
#define SVPSNETGWRT_SVP_PLATFORM_UTCPTL
#define SVPSNETGWRT_SVP_PLATFORM_DUTCPTL
#endif
#else
#define SVPSNETGWRT_SVP_PLATFORM_UTCPTL
#endif
#endif

/*---*/

#ifndef SVPSNETGWRT_SVP_PLATFORM_HAS_SEP
#ifdef  SVPSNETGWRT_SVP_PLATFORM_DUTCPTL
#define SVPSNETGWRT_SVP_PLATFORM_HAS_SEP
#endif
#endif

/*---*/

#ifndef SVPSNETGWRT_ASSUME_DISTRIBUTED_MEMORY
#ifdef  SVPSNETGWRT_SVP_PLATFORM_DUTCPTL
#define SVPSNETGWRT_ASSUME_DISTRIBUTED_MEMORY
#endif
#endif

/*----------------------------------------------------------------------------*/
/**
 * Configure library type flags
 */

#ifndef SVPSNETGWRT_LIB_TYPE_HARD
#ifndef SVPSNETGWRT_LIB_TYPE_SOFT_STATIC
#ifndef SVPSNETGWRT_LIB_TYPE_SOFT_DYNAMIC
#define SVPSNETGWRT_LIB_TYPE_SOFT_STATIC
#endif
#endif
#endif

/*----------------------------------------------------------------------------*/
/**
 * Configure debug flags
 */

#ifndef SVPSNETGWRT_DEBUG_LEVEL
#define SVPSNETGWRT_DEBUG_LEVEL 5
#endif

#ifndef SVPSNETGWRT_DEBUG
#ifdef NDEBUG
#define SVPSNETGWRT_DEBUG 0
#else
#define SVPSNETGWRT_DEBUG SVPSNETGWRT_DEBUG_LEVEL
#endif
#endif

#endif // __SVPSNETGWRT_CORE_COMMON_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

