/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : handle.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_HANDLE_INT_H
#define __SVPSNETGWRT_HANDLE_INT_H

#include "handle.utc.h"

/*---*/

#include "common.int.utc.h"
#include "basetype.int.utc.h"
#include "conslist.int.utc.h"
#include "record.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetHandleInit(
    snet_handle_t *hnd,
    const snet_conslst_node_t *cons_node);

/*---*/

extern snet_handle_t*
SNetHandleCreate(
    const snet_conslst_node_t *cons_node);

/*---*/

extern void SNetHandleDestroy(snet_handle_t *hnd);

/*---*/

extern void SNetHandleSetupMutex(snet_handle_t *hnd, const place *p);

/*----------------------------------------------------------------------------*/

extern snet_conslst_node_t*
SNetHandleGetConsNode(const snet_handle_t *hnd);

extern snet_record_t*
SNetHandleGetOutRecord(const snet_handle_t *hnd);

/*---*/

extern void 
SNetHandleSetOutRecord(snet_handle_t *hnd, const snet_record_t *rec);

/*----------------------------------------------------------------------------*/

extern const snet_base_t* SNetHandleToBase(const snet_handle_t *hnd); 

#endif // __SVPSNETGWRT_HANDLE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

