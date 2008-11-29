/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : memmng.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_MEMMNG_INT_H
#define __SVPSNETGWRT_MEMMNG_INT_H

#include "common.int.utc.h"

/*----------------------------------------------------------------------------*/

typedef struct {
    place  plc;
    void  *ptr;

} snet_remote_mem_block_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void* SNetMaskPointer(void *ptr);
extern void* SNetUnmaskPointer(void *ptr);

/*----------------------------------------------------------------------------*/

extern void*
SNetMemAlloc(unsigned int sz);

extern void*
SNetMemRealloc(void *p, unsigned int sz);

/*---*/

extern void SNetMemFree(void *p);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern bool
SNetRemoteMemAlloc(
    place p,
    unsigned int sz,
    snet_remote_mem_block_t *blk);

extern bool
SNetRemoteMemRealloc(
    unsigned int sz,
    snet_remote_mem_block_t *blk);

/*---*/

extern void SNetRemoteMemFree(const snet_remote_mem_block_t *blk);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void 
SNetMemCopy(const void *src, void *dest, unsigned int count);

/*---*/

extern void
SNetRemoteMemCopy(
    const void *src,
    const snet_remote_mem_block_t *blk,  unsigned int count);

#endif // __SVPSNETGWRT_MEMMNG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

