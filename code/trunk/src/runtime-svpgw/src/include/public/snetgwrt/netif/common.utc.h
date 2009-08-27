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
    Description    : Header file with symbols, macros, etc.. common 
                     to all modules.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_NETIF_COMMON_H
#define __SVPSNETGWRT_NETIF_COMMON_H

// The "common" from the "core"
// and "GW" libraries.
#include "snetgwrt/gw/common.utc.h"
#include "snetgwrt/core/common.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * Include the default configuration header file
 * if no other has been included yet.
 */
#ifndef SVPSNETGWRT_NETIF_CFG_HEADER
#include "config.utc.h"
#endif

#endif // __SVPSNETGWRT_NETIF_COMMON_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

