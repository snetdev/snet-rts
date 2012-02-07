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

#ifndef __SVPSNETGWRT_GW_COMMON_INT_H
#define __SVPSNETGWRT_GW_COMMON_INT_H

#include "gw/common.utc.h"

// Include the "common" from the "core"
// library first.
#include "core/common.int.utc.h"

/**
 * Include the default configuration header file
 * if no other has been included yet.
 */
#ifndef SVPSNETGWRT_INTERN_GW_CFG_HEADER
#include "config.int.utc.h"
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Error and Warning codes.
 */

// Errors!!
#define SNET_GWERR_NONE                 0x0000
#define SNET_GWERR_TYPE_ERROR           0x0001
#define SNET_GWERR_BOX                  0x0002
#define SNET_GWERR_GCTOR_INV_RETVAL     0x0003

// Warnings!!
#define SNET_GWWRN_NONE                 0x0000
#define SNET_GWWRN_IGNORED_ENTITY       0x0001
#define SNET_GWWRN_NULL_NETOUT_TFUN     0x0002
#define SNET_GWWRN_NULL_NETOUT_BUFF_POP 0x0003

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Error and Warning management functions */

extern void
SNetReportGwError(unsigned int code, ...);

extern void
SNetReportGwWarning(unsigned int code, ...);

#endif // __SVPSNETGWRT_GW_COMMON_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

