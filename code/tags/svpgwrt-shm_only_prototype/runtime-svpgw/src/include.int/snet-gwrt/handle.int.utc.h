/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

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
SNetHndInit(
    snet_handle_t *hnd,
    snet_conslst_node_t *cons_node);

/*---*/

extern snet_handle_t*
SNetHndCreate(snet_conslst_node_t *cons_node);

/*---*/

extern void SNetHndDestroy(snet_handle_t *hnd);

/*----------------------------------------------------------------------------*/

extern snet_conslst_node_t*
SNetHndGetConsNode(const snet_handle_t *hnd);

extern snet_record_t*
SNetHndGetOutRecord(const snet_handle_t *hnd);

extern snet_record_t* 
SNetHndGetConsNodeRecord(const snet_handle_t *hnd);

/*---*/

extern snet_variantencoding_t*
SNetHndCreateVEncForOutRecord(
    const snet_handle_t *hnd, unsigned int variant_idx);

/*---*/

extern void 
SNetHndSetOutRecord(snet_handle_t *hnd, snet_record_t *rec);

extern void
SNetHndSetConsNode(snet_handle_t *hnd, snet_conslst_node_t *cnode);

/*----------------------------------------------------------------------------*/

extern snet_base_t*
SNetHndToBase(snet_handle_t *hnd); 

extern const snet_base_t*
SNetHndToBaseConst(const snet_handle_t *hnd); 

#endif // __SVPSNETGWRT_HANDLE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

