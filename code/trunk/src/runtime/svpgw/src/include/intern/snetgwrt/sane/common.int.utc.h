/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

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

#ifndef __SVPSNETGWRT_SANE_COMMON_INT_H
#define __SVPSNETGWRT_SANE_COMMON_INT_H

#include "sane/common.utc.h"

// Include the "common" from the "core"
// library first.
#include "core/common.int.utc.h"

/**
 * Include the default configuration header file
 * if no other has been included yet.
 */
#ifndef SVPSNETGWRT_INTERN_SANE_CFG_HEADER
#include "config.int.utc.h"
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Error and Warning codes.
 */

// Errors!!
#define SNET_SANEERR_NONE                0x0000

// Warnings!!
#define SNET_SANEWRN_NONE                0x0000

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Error and Warning management functions */

extern void
SNetReportSaneError(unsigned int code, ...);

extern void
SNetReportSaneWarning(unsigned int code, ...);

#endif // __SVPSNETGWRT_SANE_COMMON_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

