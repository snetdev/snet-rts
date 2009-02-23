/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : buffer.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_BUFFER_H
#define __SVPSNETGWRT_BUFFER_H

#include "common.utc.h"
#include "domain.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Handle to a buffer object
 */
typedef struct buffer snet_buffer_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void SNetBufInit(
    snet_buffer_t *buf, 
    bool dynamic,
    unsigned int sz, 
    const snet_domain_t *snetd); 

extern void SNetBufInitCopy(
    snet_buffer_t *buf,
    const snet_buffer_t *src_buf, bool shalow); 

/*---*/

extern snet_buffer_t* 
SNetBufCreate(
    bool dynamic, 
    unsigned int sz, 
    const snet_domain_t *snetd); 

extern snet_buffer_t*
SNetBufCreateCopy(const snet_buffer_t *src_buf, bool shalow); 

/*---*/

extern void SNetBufDestroy(snet_buffer_t *buf);

/*----------------------------------------------------------------------------*/

extern bool SNetBufIsEmpty(const snet_buffer_t *buf);
extern bool SNetBufIsDynamic(const snet_buffer_t *buf);

/*---*/

extern unsigned int SNetBufGetSize(const snet_buffer_t *buf);
extern unsigned int SNetBufGetCapacity(const snet_buffer_t *buf);

/*----------------------------------------------------------------------------*/

extern void* SNetBufPop(snet_buffer_t *buf);
extern void* SNetBufGet(const snet_buffer_t *buf);

/*---*/

extern bool  SNetBufPush(snet_buffer_t *buf, const void *elem);

#endif // __SVPSNETGWRT_BUFFER_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

