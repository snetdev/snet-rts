/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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
#include "record.int.utc.h"
#include "graph.int.utc.h"
#include "graphindex.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

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

/*---*/

extern void
SNetConsLstNodeSetupMutex(snet_conslst_node_t *n, const place *p);

/*----------------------------------------------------------------------------*/

extern void
SNetConsLstNodeSetRecord(
    snet_conslst_node_t *n, const snet_record_t *rec);

extern void 
SNetConsLstNodeSetGraphNode(
    snet_conslst_node_t *n, const snet_gnode_t *gnode);

/*---*/

extern const snet_record_t*
SNetConsLstNodeGetRecord(const snet_conslst_node_t *n);

extern const snet_gnode_t*
SNetConsLstNodeGetGraphNode(const snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern snet_ginx_t*
SNetConsLstNodeGetDynGInx(snet_conslst_node_t *n);

extern snet_ginx_t*
SNetConsLstNodeGetMinGInx(snet_conslst_node_t *n);

/*---*/

extern void 
SNetConsLstNodeAddDynInx(
    snet_conslst_node_t *n, const snet_ginx_t *ginx);

extern void SNetConsLstNodeRemoveDynInx(snet_conslst_node_t *n);

/*---*/

extern void SNetConsLstNodeSetMinInxModifiedFlag(snet_conslst_node_t *n);
extern void SNetConsLstNodeResetMinInxModifiedFlag(snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern snet_conslst_node_t*
SNetConsLstNodeGetNext(snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern bool
SNetConsLstNodeIsHead(const snet_conslst_node_t *n);

extern bool
SNetConsLstNodeIsTail(const snet_conslst_node_t *n);

extern bool
SNetConsLstNodeIsAttached(const snet_conslst_node_t *n);

/*----------------------------------------------------------------------------*/

extern const snet_base_t* SNetConsLstNodeToBase(const snet_conslst_node_t *n);

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

/*---*/

extern void
SNetConsLstSetupMutex(snet_conslst_t *lst, const place *p);

/*----------------------------------------------------------------------------*/

extern const snet_conslst_node_t*
SNetConsLstGetHead(const snet_conslst_t *lst);

extern const snet_conslst_node_t*
SNetConsLstGetTail(const snet_conslst_t *lst);

/*----------------------------------------------------------------------------*/

extern void
SNetConsLstPush(
    snet_conslst_t *lst, snet_conslst_node_t *n);

extern void
SNetConsLstInsertAfter(
    snet_conslst_node_t *n1, snet_conslst_node_t *n2);

/*---*/

extern snet_conslst_node_t* SNetConsLstPop(snet_conslst_t *lst);

/*---*/

extern const snet_base_t* SNetConsLstToBase(const snet_conslst_t *lst);

#endif // __SVPSNETGWRT_CONSLIST_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

