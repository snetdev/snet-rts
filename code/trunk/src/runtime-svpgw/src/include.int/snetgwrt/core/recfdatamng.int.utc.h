/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : recfdatamng.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_RECFDATAMNG_INT_H
#define __SVPSNETGWRT_RECFDATAMNG_INT_H

#include "common.int.utc.h"

#include "snet.int.utc.h"
#include "memmng.int.utc.h"

/*---*/

#define REC_FDATA_COPY_DEFAULT_FLAGS           0x00
#define REC_FDATA_COPY_SERIALIZED_FLAG         0x01
#define REC_FDATA_COPY_NO_REFCNT_DECR_FLAG     0x02
#define REC_FDATA_COPY_NO_NEW_DESCRIPTOR_FLAG  0x04

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/**
 * Type that holds info about
 * a record's field data.
 */
typedef struct rec_fdata_descriptor snet_rec_fdata_descriptor_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Descriptor management functions */

extern void
SNetRecFieldDataDescriptorInit(
    snet_rec_fdata_descriptor_t *rfdd, void *data, snet_bli_id_t blid);

/*---*/

extern snet_ref_t
SNetRecFieldDataDescriptorCreate(void *data, snet_bli_id_t blid);

/*----------------------------------------------------------------------------*/

extern void*
SNetRecFieldDataDescriptorGetDPtr(snet_ref_t rfdd);

extern snet_bli_id_t
SNetRecFieldDataDescriptorGetBlid(snet_ref_t rfdd);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Actual record field data managenent functions.
 *
 * These need to be threads because they should be executed
 * on the place where the descriptor (hence the data) resides.
 */

extern thread void
SNetRecFieldDataCopy(
    snet_ref_t rfdd, 
    uint8_t    flags,
    shared snet_ref_t ref, shared uint32_t sz);

/*----------------------------------------------------------------------------*/
/* Reference counting. */

extern thread void
SNetRecFieldDataIncreaseRefCount(snet_ref_t *rfddv, uint32_t count);

extern thread void
SNetRecFieldDataDecreaseRefCount(snet_ref_t *rfddv, uint32_t count);

#endif // __SVPSNETGWRT_RECFDATAMNG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

