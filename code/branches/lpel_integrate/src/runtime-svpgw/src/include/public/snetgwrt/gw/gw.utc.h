/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : gw.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : This file contains runtime initialization and cleanup
                     functions as well as "primary" runtime services.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_GW_H
#define __SVPSNETGWRT_GW_GW_H

#include "common.utc.h"
#include "domain.utc.h"

/*---*/

#include "snetgwrt/core/record.utc.h"

/*----------------------------------------------------------------------------*/

extern void
SNetGlobalGwInit();

/*---*/

extern void
SNetGlobalGwDestroy();

extern void
SNetGlobalGwDestroyEx(bool force);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Network execution management functions
 */
extern void
SNetKill(snet_domain_t *snetd);

extern void
SNetSqueeze(snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * SNetPushIn() function used to insert
 * records to the network.
 */
extern void
SNetPushIn(snet_domain_t *snetd, snet_record_t *rec);

/*----------------------------------------------------------------------------*/
/**
 * SNetPopOut() function used to get output
 * records of a network.
 */
extern snet_record_t*
SNetPopOut(snet_domain_t *snetd);

/*---*/

unsigned int
SNetGetOutputBufferSize(const snet_domain_t *snetd);

#endif // __SVPSNETGWRT_GW_GW_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

