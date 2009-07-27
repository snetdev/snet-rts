/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : conslist.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_CONSLIST_INT_H
#define __SVPSNETGWRT_CONSLIST_INT_H

#include "common.int.utc.h"
#include "basetype.int.utc.h"

#include "graph.int.utc.h"
#include "idxvec.int.utc.h"
#include "record.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef enum {
    CONS_NODE_STATE_INIT,
    CONS_NODE_STATE_WALKING,

    CONS_NODE_STATE_WAIT_OUT,
    CONS_NODE_STATE_WAIT_SYNC,
    CONS_NODE_STATE_WAIT_EXTERN_CONN

} snet_conslst_node_state_t;


/*---*/

typedef struct cons_list      snet_conslst_t;
typedef struct cons_list_node snet_conslst_node_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void 
SNetConsLstNodeInit(
    snet_conslst_node_t *n,
    const snet_domain_t *domain);

extern snet_conslst_node_t*
SNetConsLstNodeCreate(const snet_domain_t *domain);

/*---*/

extern void SNetConsLstNodeDestroy(snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern void
SNetConsLstNodeSetState(
    snet_conslst_node_t *n,
    snet_conslst_node_state_t state);

/*---*/

extern void
SNetConsLstNodeSetFlags(
    snet_conslst_node_t *n, unsigned int value);

extern void
SNetConsLstNodeSetRecord(
    snet_conslst_node_t *n, snet_record_t *rec);

extern void 
SNetConsLstNodeSetGraphNode(
    snet_conslst_node_t *n, snet_gnode_t *gnode);

/*---*/

extern snet_conslst_t*
SNetConsLstNodeGetList(const snet_conslst_node_t *n);

extern snet_conslst_node_t*
SNetConsLstNodeGetNext(const snet_conslst_node_t *n);

extern snet_conslst_node_t*
SNetConsLstNodeGetPrevious(const snet_conslst_node_t *n);

/*---*/

extern snet_conslst_node_state_t
SNetConsLstNodeGetState(const snet_conslst_node_t *n);

extern unsigned int
SNetConsLstNodeGetFlags(const snet_conslst_node_t *n);

/*---*/

extern snet_record_t*
SNetConsLstNodeGetRecord(const snet_conslst_node_t *n);

extern snet_gnode_t*
SNetConsLstNodeGetGraphNode(const snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern snet_idxvec_t*
SNetConsLstNodeGetOrdGIdx(snet_conslst_node_t *n);

extern snet_idxvec_t*
SNetConsLstNodeGetDynGIdx(snet_conslst_node_t *n);

extern snet_idxvec_t*
SNetConsLstNodeGetMinGIdx(snet_conslst_node_t *n);

/*---*/

extern void 
SNetConsLstNodeAddDynGIdx(
    snet_conslst_node_t *n, const snet_idxvec_t *idx);

extern void
SNetConsLstNodeAddSameDynGIdxAs(
    snet_conslst_node_t *n1, const snet_conslst_node_t *n2);

extern void SNetConsLstNodeRemoveDynGIdx(snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern bool
SNetConsLstNodeIsNull(const snet_conslst_node_t *n);

extern bool
SNetConsLstNodeIsHead(const snet_conslst_node_t *n);

extern bool
SNetConsLstNodeIsTail(const snet_conslst_node_t *n);

extern bool
SNetConsLstNodeIsAttached(const snet_conslst_node_t *n);

/*---*/

extern bool
SNetConsLstNodeIsDynGIdxNew(const snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern snet_base_t* 
SNetConsLstNodeToBase(snet_conslst_node_t *n);

extern const snet_base_t* 
SNetConsLstNodeToBaseConst(const snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetConsLstInit(
    snet_conslst_t *lst,
    const snet_domain_t *domain);

extern snet_conslst_t*
SNetConsLstCreate(const snet_domain_t *domain);

/*---*/

extern void
SNetConsLstDestroy(snet_conslst_t *lst);

/*----------------------------------------------------------------------------*/

extern snet_conslst_node_t*
SNetConsLstGetHead(const snet_conslst_t *lst);

extern snet_conslst_node_t*
SNetConsLstGetTail(const snet_conslst_t *lst);

/*----------------------------------------------------------------------------*/

extern void
SNetConsLstPush(
    snet_conslst_t *lst, snet_conslst_node_t *n);

extern void
SNetConsLstInsertBefore(snet_conslst_node_t *n1, snet_conslst_node_t *n2);

/*---*/

extern snet_conslst_node_t* SNetConsLstPop(snet_conslst_t *lst);

/*---*/

extern snet_base_t*
SNetConsLstToBase(snet_conslst_t *lst);

extern const snet_base_t*
SNetConsLstToBaseConst(const snet_conslst_t *lst);

#endif // __SVPSNETGWRT_CONSLIST_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

