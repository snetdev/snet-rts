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

#ifndef __SVPSNETGWRT_NETIF_COMMON_INT_H
#define __SVPSNETGWRT_NETIF_COMMON_INT_H

#include "netif/common.utc.h"

// Include the "common" from the "core"
// and "GW" libraries.
#include "gw/common.int.utc.h"
#include "core/common.int.utc.h"

/**
 * Include the default configuration header file
 * if no other has been included yet.
 */
#ifndef SVPSNETGWRT_INTERN_NETIF_CFG_HEADER
#include "config.int.utc.h"
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Error and Warning codes.
 */

// Errors!!
#define SNET_NETIFERR_NONE   0x0000
#define SNET_NETIFERR_FIO    0x0001
#define SNET_NETIFERR_REGX   0x0002
#define SNET_NETIFERR_SYNTAX 0x0003

// Warnings!!
#define SNET_NETIFWRN_NONE   0x0000

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Error and Warning management functions */

extern void
SNetReportNetIfError(unsigned int code, ...);

extern void
SNetReportNetIfWarning(unsigned int code, ...);

#endif // __SVPSNETGWRT_NETIF_COMMON_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

