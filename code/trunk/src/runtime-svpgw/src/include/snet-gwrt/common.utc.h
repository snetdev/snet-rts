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

#ifndef __SVPSNETGWRT_COMMON_H
#define __SVPSNETGWRT_COMMON_H

/**
 * Version
 */
#define SVPSNETGWRT_VER 100

/*----------------------------------------------------------------------------*/
/**
 * Compiler
 */

#ifdef __GNUC__
#define SVPSNETGWRT_COMP_GCC
#else
#ifdef __EUTCC__
#define SVPSNETGWRT_COMP_EUTCC
#else
#define SVPSNETGWRT_COMP_UNKNOWN
#endif
#endif

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
 * Configure platform flags
 */

#ifndef SVPSNETGWRT_SVP_PLATFORM_UTCPTL
#ifndef SVPSNETGWRT_SVP_PLATFORM_DUTCPTL
#ifndef SVPSNETGWRT_SVP_PLATFORM_MGRID
#ifndef SVPSNETGWRT_SVP_PLATFORM_UNKNOWN
#define SVPSNETGWRT_SVP_PLATFORM_UNKNOWN
#endif
#endif
#else
#define SVPSNETGWRT_SVP_PLATFORM_UTCPTL
#endif
#endif

/*---*/

#ifndef SVPSNETGWRT_ASSUME_DISTRIBUTED_MEMORY
#ifdef SVPSNETGWRT_SVP_PLATFORM_DUTCPTL
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

#endif // __SVPSNETGWRT_COMMON_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

