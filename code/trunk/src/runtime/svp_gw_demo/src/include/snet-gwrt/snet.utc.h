/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : snet.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : This file contains functions that implement the primary 
                     services of the runtime.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_SNET_H
#define __SVPSNETGWRT_SNET_H

#include "common.utc.h"
#include "domain.utc.h"
#include "handle.utc.h"
#include "record.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Global initialization and termination and box lanugage interface related
 * routines for the runtime.
 */

extern void SNetGlobalInitialize();

/*---*/

extern void SNetGlobalDestroy();

/*----------------------------------------------------------------------------*/

extern bool SNetGlobalRegisterInterface(
    int id,
    void  (*freefun) (void *),
    void* (*copyfun) (void *),
    int   (*serfun)  (void *, char **),
    void* (*deserfun)(char *, int));

/*---*/

extern void* SNetGetCopyFun(int id);
extern void* SNetGetFreeFun(int id);
extern void* SNetGetSerializationFun(int id);
extern void* SNetGetDeserializationFun(int id);

extern void* SNetGetCopyFunFromRec(snet_record_t *rec);
extern void* SNetGetFreeFunFromRec(snet_record_t *rec);
extern void* SNetGetSerializationFunFromRec(snet_record_t *rec);
extern void* SNetGetDeserializationFunFromRec(snet_record_t *rec);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Network execution management functions
 */

extern void SNetKill(snet_domain_t *snetd);
extern void SNetSqueeze(snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * SNetPushIn() function used to insert
 * records to the network.
 */
extern void SNetPushIn(snet_domain_t *snetd, snet_record_t *rec);

/*----------------------------------------------------------------------------*/
/**
 * SNetPopOut() function used to get output
 * records of a network.
 */
extern snet_record_t* SNetPopOut(snet_domain_t *snetd, bool block);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * SNetOutxxx() functions that are called from boxes to emmit
 * output records.
 */

extern void SNetOut(
    snet_handle_t *hnd, snet_record_t *rec);

extern void SNetOutRaw(
    snet_handle_t *hnd, int variant_num, ...);

extern void SNetOutRawArray(
    snet_handle_t *hnd, 
    int if_id, int var_num, void **fields, int **tags, int **btags);

#endif // __SVPSNETGWRT_SNET_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

