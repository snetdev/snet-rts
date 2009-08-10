/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

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

$ifndef __STDC_FORMAT_MACROS
$define __STDC_FORMAT_MACROS
$endif

/*---*/

#include "common.int.utc.h"

/*---*/

$include <inttypes.h>

/*---*/

#define NULL_REF ((snet_ref_t)(0))

/*---*/

#define DATABLK_TYPE_EXTERN         -1
#define DATABLK_TYPE_UNSPECIFIED     0
#define DATABLK_TYPE_RT_INTERNAL     1
#define DATABLK_TYPE_REC_FDATA_DCPTR 2
#define DATABLK_TYPE_REC_FDATA_SER   3
#define DATABLK_TYPE_PLACE_STRUCT    4
#define DATABLK_TYPE_NET_DOMAIN      5

/*----------------------------------------------------------------------------*/
/**
 * Types and symbols related to "references" to data!!
 *
 * References are similar to pointers (they point to data) with the
 * difference that a refererence can be validated making it more "safe"
 * than a pointer (not completely though which means that references can
 * still be used to modify data in all kinds of wrong ways) and it can 
 * also refer to "remote" data as well (e.g. in distributed memory
 * enviroments).
 *
 * Note however that if a reference points to "remote" data that data has to
 * be fetched and a local copy to be created in order to access it; direct
 * access to "remote" data is not supported. What the reference provides is
 * a safe (from the perspective that the remote node will be able to validate
 * it) and portable "identifier" for the data to be fetched from a remote node!
 *
 * Since references have some extra overhead they are only used in places
 * where a pointer to some block of data is required but it cannot be 100%
 * certain that the data will always be "local" for the cases the runtime
 * is used in a distributed memory system enviroment (e.g. record fields).
 *
 * Note that if the runtime is built ONLY for shared memory systems (i.e
 * SVPSNETGWRT_ASSUME_DISTRIBUTED_MEMORY is not  defined) then references
 * are treated more or less like normal pointer in order to reduce the
 * extra overhead (see code below and in the file "src/core/memmng.utc").
 */
#ifdef SVPSNETGWRT_ASSUME_DISTRIBUTED_MEMORY

// For a distributed enviroment references will have to be
// of fixed size. They will be 32bits. References are just
// indexes to lookup tables thus we can have 2^32 blocks (where
// each block can be of any size) which is more than enough (at least
// for now).
typedef uint32_t snet_ref_t;

// "printf()" format specifiers
#define PRISNETREF  PRIx32
#define PRIiSNETREF PRIi32
#define PRIoSNETREF PRIo32
#define PRIxSNETREF PRIx32
#define PRIOSNETREF PRIO32
#define PRIXSNETREF PRIX32

#else // assume shared memory enviroment

// In a shared memory enviroment the reference will
// be just a pointer (to the header of the block).
typedef uintptr_t snet_ref_t;

// "printf()" format specifiers
#define PRISNETREF  PRIxPTR
#define PRIiSNETREF PRIiPTR
#define PRIoSNETREF PRIoPTR
#define PRIxSNETREF PRIxPTR
#define PRIOSNETREF PRIOPTR
#define PRIXSNETREF PRIXPTR

#endif // SVPSNETGWRT_ASSUME_DISTRIBUTED_MEMORY

/*----------------------------------------------------------------------------*/
/* Memory allocation monitoring configuration */

#ifndef SVPSNETGWRT_MONITOR_MALLOC
#if SVPSNETGWRT_DEBUG > 1
#define SVPSNETGWRT_MONITOR_MALLOC
#endif
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void SNetMemMngSubSystemInit();
extern void SNetMemMngSubSystemDestroy();

/*----------------------------------------------------------------------------*/

extern void* SNetMaskPointer(const void *ptr);
extern void* SNetUnmaskPointer(const void *ptr);

/*----------------------------------------------------------------------------*/

extern size_t
SNetMemGetBlockSize(const void *p);

/*---*/

#ifdef SVPSNETGWRT_MONITOR_MALLOC
extern size_t        SNetMemGetAllocSize();
extern unsigned long SNetMemGetAllocCount();
#endif

/*----------------------------------------------------------------------------*/

extern void*
SNetMemAlloc(size_t sz);

extern void*
SNetMemRealloc(void *p, size_t sz);

/*---*/

extern void*
SNetMemTryAlloc(size_t sz);

extern void*
SNetMemTryRealloc(void *p, size_t sz);

/*----------------------------------------------------------------------------*/

extern snet_ref_t
SNetMemCreateRef(void *p, int type);

/*---*/

extern void* 
SNetMemGetPtr(const snet_ref_t ref);

extern void* 
SNetMemGetPtrWithType(const snet_ref_t ref, int type);

/*---*/

extern snet_ref_t SNetMemGetRef(const void *p);
extern snet_ref_t SNetMemGetExternDataRef(const void *p);

/*----------------------------------------------------------------------------*/

extern void 
SNetMemFree(void *p);

extern void
SNetMemDestroyRef(const snet_ref_t ref);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern unsigned long 
SNetMemIncreaseRefCount(void *p);

extern unsigned long
SNetMemDecreaseRefCount(void *p);

/*----------------------------------------------------------------------------*/
extern void
SNetMemSet(void *p, int value, size_t sz);

extern void 
SNetMemCopy(const void *src, void *dest, size_t count);

#endif // __SVPSNETGWRT_MEMMNG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

