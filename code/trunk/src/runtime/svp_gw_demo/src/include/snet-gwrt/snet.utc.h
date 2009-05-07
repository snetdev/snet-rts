/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

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

/*---*/

$include <stdio.h>

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Datatype for a record (forward declaration
 * to avoid including the file "record.utc.h").
 */
typedef struct record snet_record_t;


/**
 * Types related to the box language
 * interface system.
 */

typedef unsigned int snet_bli_id_t;

/**
 * Functions a box language interface
 * should provide.
 */
typedef void  (*snet_bli_freefptr_t)(void *);
typedef void* (*snet_bli_copyfptr_t)(void *);

/*---*/

typedef unsigned int (*snet_bli_serfptr_t)(void *, char **);
typedef void*        (*snet_bli_deserfptr_t)(char *, unsigned int);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Global initialization and termination
 */
extern void SNetGlobalInitialize();
extern void SNetGlobalDestroy();

/*----------------------------------------------------------------------------*/
/**
 * Sets the stream where the runtime will output its
 * error and warning messages (if any). If not set
 * the default will be "stderr".
 */
extern void SNetGlobalSetErrorStream(FILE *stream);

/*----------------------------------------------------------------------------*/
/**
 * Box lanugage interface related routines.
 */

extern bool
SNetGlobalRegisterInterface(
    snet_bli_id_t        id,
    snet_bli_freefptr_t  freefun,
    snet_bli_copyfptr_t  copyfun,
    snet_bli_serfptr_t   serfun,
    snet_bli_deserfptr_t deserfun);

/*---*/

extern snet_bli_freefptr_t
SNetGetFreeFun(snet_bli_id_t id);

extern snet_bli_copyfptr_t
SNetGetCopyFun(snet_bli_id_t id);

extern snet_bli_serfptr_t
SNetGetSerializationFun(snet_bli_id_t id);

extern snet_bli_deserfptr_t
SNetGetDeserializationFun(snet_bli_id_t id);

/*---*/

extern snet_bli_freefptr_t  
SNetGetFreeFunFromRec(snet_record_t *rec);

extern snet_bli_copyfptr_t
SNetGetCopyFunFromRec(snet_record_t *rec);

extern snet_bli_serfptr_t
SNetGetSerializationFunFromRec(snet_record_t *rec);

extern snet_bli_deserfptr_t
SNetGetDeserializationFunFromRec(snet_record_t *rec);

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
    snet_handle_t *hnd, 
    snet_bli_id_t blid, int variant_inx, ...);

extern void SNetOutRawArray(
    snet_handle_t *hnd, 
    snet_bli_id_t blid,
    int variant_inx, void **fields, int *tags, int *btags);

#endif // __SVPSNETGWRT_SNET_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

