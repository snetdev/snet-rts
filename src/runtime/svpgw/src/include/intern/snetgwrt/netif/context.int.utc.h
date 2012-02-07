/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : context.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_NETIF_CONTEXT_INT_H
#define __SVPSNETGWRT_NETIF_CONTEXT_INT_H

#include "common.int.utc.h"

/*---*/

#include "gw/domain.int.utc.h"

/*---*/

$include <stdio.h>

/*----------------------------------------------------------------------------*/
/**
 * Type of a "Network interface context"
 */
typedef struct netifcontext snet_netifcontext_t;

/*----------------------------------------------------------------------------*/

extern void
SNetNetIfCntxInit(
    snet_netifcontext_t *cntx, 
    snet_domain_t       *snetd, 
    FILE                *ifhnd,
    FILE                *ofhnd,
    int lblc, const char **lblv, 
    int blic, const char **bliv);

/*---*/

extern 
snet_netifcontext_t*
SNetNetIfCntxCreate(
    snet_domain_t *snetd, 
    FILE          *ifhnd,
    FILE          *ofhnd,
    int lblc, const char **lblv, 
    int blic, const char **bliv);

extern void
SNetNetIfCntxDestroy(snet_netifcontext_t *cntx);

/*----------------------------------------------------------------------------*/

extern snet_domain_t*
SNetNetIfCntxGetDomain(const snet_netifcontext_t *cntx);

extern FILE*
SNetNetIfCntxGetInputFileHandle(const snet_netifcontext_t *cntx);

extern FILE*
SNetNetIfCntxGetOutputFileHandle(const snet_netifcontext_t *cntx);

/*----------------------------------------------------------------------------*/

extern int
SNetNetIfCntxGetLabelsCount(
    const snet_netifcontext_t *cntx);

extern const char*
SNetNetIfCntxGetLabel(
    const snet_netifcontext_t *cntx, int idx);

extern int
SNetNetIfCntxGetLabelIndex(
    const snet_netifcontext_t *cntx, const char *lbl);

/*---*/

extern int
SNetNetIfGetBliLabelsCount(
    const snet_netifcontext_t *cntx);

extern const char*
SNetNetIfGetBliLabel(
    const snet_netifcontext_t *cntx, int idx);

extern int
SNetNetIfCntxGetBliLabelIndex(
    const snet_netifcontext_t *cntx, const char *lbl);

/*----------------------------------------------------------------------------*/

extern bool
SNetNetIfCntxValidateLabel(
    const snet_netifcontext_t *cntx, const char *lbl);

extern bool
SNetNetIfCntxValidateBliLabel(
    const snet_netifcontext_t *cntx, const char *lbl);

#endif // __SVPSNETGWRT_NETIF_CONTEXT_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/


