/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : common.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header file with symbols macros and declarations
                     common to all modules.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_COMMON_INT_H
#define __SVPSNETGWRT_COMMON_INT_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * Include the default configuration header file
 * if no other has been included yet.
 */
#ifndef SVPSNETGWRT_INTERN_CFG_HEADER
#include "config.int.utc.h"
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Common includes
 */

#if SVPSNETGWRT_DEBUG > 0
$include <stdio.h>
$include <stdlib.h>
$include <string.h>
$include <assert.h>
#endif

#endif // __SVPSNETGWRT_COMMON_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

