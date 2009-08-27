/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : binld.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header file with symbols macros and declarations
                     common to all modules.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_NETIF_BINLD_INT_H
#define __SVPSNETGWRT_NETIF_BINLD_INT_H

#include "common.int.utc.h"
#include "context.int.utc.h"

/*----------------------------------------------------------------------------*/

extern
thread void 
SNetNetIfBinLoad(snet_netifcontext_t *cntx);

#endif // __SVPSNETGWRT_NETIF_BINLD_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

