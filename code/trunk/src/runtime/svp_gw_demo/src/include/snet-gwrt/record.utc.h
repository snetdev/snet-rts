/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : record.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_RECORD_H
#define __SVPSNETGWRT_RECORD_H

#include "common.utc.h"
#include "snet.utc.h"
#include "domain.utc.h"
#include "typeencode.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef enum {
    REC_DESCR_DATA,
    REC_DESCR_CTRL,
    REC_DESCR_NET

} snet_record_descr_t;

/*---*/

typedef enum {
    REC_DATA_MODE_TXT,
    REC_DATA_MODE_BIN

} snet_record_data_mode_t;

typedef enum {
    REC_CTRL_MODE_RT,
    REC_CTRL_MODE_APP

} snet_record_ctrl_mode_t;

/*---*/

typedef enum {
    REC_DATA_DISPOSE_MODE_CONSUMED,
    REC_DATA_DISPOSE_MODE_NONE,
    REC_DATA_DISPOSE_MODE_ALL

} snet_record_data_dispose_mode_t;

/*---*/
/**
 * Datatype for a record.
 */
typedef struct record snet_record_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_record_t*
SNetRecCreate(snet_record_descr_t descr, ...);

/*---*/

extern snet_record_t*
SNetDataRecCreate(snet_variantencoding_t *venc, ...);

extern snet_record_t*
SNetDataRecVCreate(snet_variantencoding_t *venc, va_list vargs);

extern snet_record_t*
SNetDataRecCreateFromArrays(
    snet_variantencoding_t *venc, void **fields, int *tags, int *btags);

/*---*/

extern snet_record_t*
SNetCtrlRecCreate(
    snet_record_ctrl_mode_t mode,
    unsigned int opcode, const void *data, unsigned int data_sz);

extern snet_record_t* SNetNetRecCreate(const snet_gnode_t *net_groot);

/*---*/

extern snet_record_t*
SNetRecCreateCopy(const snet_record_t *src);

extern void SNetRecDestroy(snet_record_t *rec);

/*----------------------------------------------------------------------------*/

extern snet_record_descr_t
SNetRecGetDescription(const snet_record_t *rec);

/*---*/

extern snet_record_data_mode_t
SNetRecGetDataMode(const snet_record_t *rec);

extern snet_record_data_dispose_mode_t
SNetRecGetDataDisposeMode(const snet_record_t *rec);

extern void
SNetRecSetDataMode(
    snet_record_t *rec, snet_record_data_mode_t mode);

extern void
SNetRecSetDataDisposeMode(
    snet_record_t *rec, snet_record_data_dispose_mode_t mode);

/*---*/

extern snet_bli_id_t 
SNetRecGetInterfaceId(const snet_record_t *rec);

extern void 
SNetRecSetInterfaceId(snet_record_t *rec, snet_bli_id_t id);

/*----------------------------------------------------------------------------*/

extern snet_variantencoding_t*
SNetRecGetVariantEncoding(const snet_record_t *rec);

extern void
SNetRecSetVariantEncoding(
    snet_record_t *rec, const snet_variantencoding_t *venc);
 
/*----------------------------------------------------------------------------*/

extern bool 
SNetRecHasTag(const snet_record_t *rec, int name);

extern bool
SNetRecHasBTag(const snet_record_t *rec, int name);

extern bool
SNetRecHasField(const snet_record_t *rec, int name);

/*---*/

extern bool
SNetRecIsTagConsumed(const snet_record_t *rec, int name);

extern bool
SNetRecIsBTagConsumed(const snet_record_t *rec, int name);

extern bool
SNetRecIsFieldConsumed(const snet_record_t *rec, int name);

/*---*/

extern void
SNetRecResetConsumed(snet_record_t *rec);

extern void
SNetRecResetConsumedTag(snet_record_t *rec, int name);

extern void
SNetRecResetConsumedBTag(snet_record_t *rec, int name);

extern void
SNetRecResetConsumedField(snet_record_t *rec, int name);

/*----------------------------------------------------------------------------*/

extern int
SNetRecGetTag(const snet_record_t *rec, int name);

extern int
SNetRecTakeTag(snet_record_t *rec, int name);

extern void
SNetRecSetTag(snet_record_t *rec, int name, int val);

/*---*/

extern int
SNetRecGetBTag(const snet_record_t *rec, int name);

extern int
SNetRecTakeBTag(snet_record_t *rec, int name);

extern void
SNetRecSetBTag(snet_record_t *rec, int name, int val);

/*---*/

extern void*
SNetRecGetField(const snet_record_t *rec, int name);

extern void*
SNetRecTakeField(snet_record_t *rec, int name);

extern void
SNetRecSetField(snet_record_t *rec, int name, void *val);

/*----------------------------------------------------------------------------*/

extern snet_record_ctrl_mode_t
SNetRecGetCtrlMode(const snet_record_t *rec);

extern unsigned int
SNetRecGetOpCode(const snet_record_t *rec);

extern const void*
SNetRecGetCtrlData(const snet_record_t *rec);

/*----------------------------------------------------------------------------*/

extern const snet_gnode_t*
SNetRecGetNetRoot(const snet_record_t *rec);

#endif // __SVPSNETGWRT_RECORD_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

