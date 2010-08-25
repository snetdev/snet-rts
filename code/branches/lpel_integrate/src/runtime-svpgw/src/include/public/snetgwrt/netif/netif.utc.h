/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : netif.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_NETIF_NETIF_H
#define __SVPSNETGWRT_NETIF_NETIF_H

#include "common.utc.h"

/*---*/

#include "snetgwrt/gw/domain.utc.h"

/*----------------------------------------------------------------------------*/
/* Initialization and termination functions */

extern void
SNetGlobalNetIfInit();

extern void
SNetGlobalNetIfDestroy();

extern void
SNetGlobalNetIfDestroyEx(bool force);

/*----------------------------------------------------------------------------*/
/**
 * Function to start the network interface component (and thus the execution
 * of the network).
 */

extern int
SNetNetIfRun(
    int          argc,
    const char **argv,
    int          lblc,
    const char **lblv,
    int          blic,
    const char **bliv,
    snet_graphctor_fptr_t gctor_fun);

#endif // __SVPSNETGWRT_NETIF_NETIF_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

