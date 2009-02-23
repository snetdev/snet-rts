/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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
 * Check debug flag
 */
#ifndef SVPSNETGWRT_DEBUG
#ifdef NDEBUG
#define SVPSNETGWRT_DEBUG 0
#else
#define SVPSNETGWRT_DEBUG 1
#endif
#endif

#endif // __SVPSNETGWRT_COMMON_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

