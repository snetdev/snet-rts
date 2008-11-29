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
    REC_MODE_TXT,
    REC_MODE_BIN

} snet_record_mode_t;

/**
 * Datatype for a record.
 */
typedef struct record snet_record_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_record_t*
SNetRecCreate(snet_record_descr_t descr, const snet_domain_t *domain, ...);

/*---*/

extern void SNetRecDestroy(snet_record_t *rec);

/*----------------------------------------------------------------------------*/

extern snet_record_descr_t
SNetRecGetDescription(const snet_record_t *rec);

#endif // __SVPSNETGWRT_RECORD_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

