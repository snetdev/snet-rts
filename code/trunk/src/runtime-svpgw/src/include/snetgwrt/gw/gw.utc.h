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
SNetGlobalInitialize();

/*---*/

extern void
SNetGlobalDestroy();

extern void
SNetGlobalDestroyEx(bool force);

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

/*----------------------------------------------------------------------------*/
/**
 * SNetGetBoxSelectedResource() function used
 * to retrieve the resource/ SVP place that
 * the runtime has has selected as the most
 * appropriate to execute a given box.
 * 
 * Note that the returned place is only valid
 * within the context of the "box proxy" function
 * that has been given to the runtime from user
 * code and which it calls to "execute" the box!!
 *
 * In any other case whether the returned place is
 * valid cannot be guaranteed (because after that
 * function returns the runtime is free to release
 * the place)!
 */
extern place
SNetGetBoxSelectedResource(const snet_handle_t *hnd);

#endif // __SVPSNETGWRT_GW_GW_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

