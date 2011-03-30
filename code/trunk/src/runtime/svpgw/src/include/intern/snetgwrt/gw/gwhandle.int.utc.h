/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : gwhandle.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GWHANDLE_INT_H
#define __SVPSNETGWRT_GWHANDLE_INT_H

#include "common.int.utc.h"
#include "domain.int.utc.h"
#include "conslist.int.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * Datatype for a GW handle. This
 * is a "derived" type of the type
 * "snet_handle_t" from the "core"
 * library.
 */
typedef struct gwhandle snet_gwhandle_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetGwHndInit(
    snet_gwhandle_t     *hnd,
    snet_domain_t       *snetd,
    snet_conslst_node_t *cons_node);

/*----------------------------------------------------------------------------*/

extern 
snet_gwhandle_t*
SNetGwHndCreate(
    snet_domain_t  *snetd,
    snet_conslst_node_t *cons_node);

extern void
SNetGwHndDestroy(snet_gwhandle_t *hnd);

/*----------------------------------------------------------------------------*/

extern snet_domain_t*
SNetGwHndGetDomain(const snet_gwhandle_t *hnd);

extern snet_conslst_node_t*
SNetGwHndGetConsNode(const snet_gwhandle_t *hnd);

extern snet_record_t*
SNetGwHndGetConsNodeRecord(const snet_gwhandle_t *hnd);

extern void*
SNetGwHndGetSyncCellState(const snet_gwhandle_t *hnd);

extern bool
SNetGwHndGetSyncCellDefiniteMatch(const snet_gwhandle_t *hnd);

extern snet_place_t
SNetGwHndGetBoxSelectedPlace(const snet_gwhandle_t *hnd);

/*---*/

extern void
SNetGwHndSetConsNode(
    snet_gwhandle_t *hnd, snet_conslst_node_t *cnode);

extern void
SNetGwHndSetSyncCellState(snet_gwhandle_t *hnd, void *state);

extern void
SNetGwHndSetSyncCellDefiniteMatch(snet_gwhandle_t *hnd, bool value);

extern void
SNetGwHndSetBoxSelectedPlace(snet_gwhandle_t *hnd, snet_place_t plc);

/*----------------------------------------------------------------------------*/

extern snet_handle_t*
SNetGwHndToBaseHandle(snet_gwhandle_t *hnd);

extern const snet_handle_t*
SNetGwHndToBaseHandleConst(const snet_gwhandle_t *hnd);

#endif // __SVPSNETGWRT_GWHANDLE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

