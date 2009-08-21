/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : graph.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_GRAPH_INT_H
#define __SVPSNETGWRT_GW_GRAPH_INT_H

#include "gw/graph.utc.h"

/*---*/

#include "common.int.utc.h"
#include "domain.int.utc.h"
#include "idxvec.int.utc.h"
#include "metadata.int.utc.h"
#include "expression.int.utc.h"
#include "filteriset.int.utc.h"

/*---*/

#include "core/basetype.int.utc.h"
#include "core/typeencode.int.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * Graph node flags
 */
#define GNODE_REORDER_POINT_FLAG 0x01

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef enum {
    GRAPH_NODE_TYPE_NOP,
    
    GRAPH_NODE_TYPE_BOX,
    GRAPH_NODE_TYPE_SYNCCELL,
    GRAPH_NODE_TYPE_FILTER,
    GRAPH_NODE_TYPE_NAMESHIFT,

    GRAPH_NODE_TYPE_COMB_STAR,
    GRAPH_NODE_TYPE_COMB_SPLIT,
    GRAPH_NODE_TYPE_COMB_PARALLEL,

    GRAPH_NODE_TYPE_EXTERN_CONNECTION

} snet_gnode_type_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void 
SNetGNodeInit(
    snet_gnode_t *n,
    snet_gnode_type_t type);

/*---*/

extern snet_gnode_t*
SNetGNodeCreate(snet_gnode_type_t type);

/*---*/

extern void SNetGNodeDestroy(snet_gnode_t *n, bool deep);

/*----------------------------------------------------------------------------*/

extern snet_gnode_type_t SNetGNodeGetType(const snet_gnode_t *n);

/*----------------------------------------------------------------------------*/

extern void 
SNetGNodeConnect(snet_gnode_t *n1, snet_gnode_t* n2, bool merge_lnk);

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupNormal(snet_gnode_t *n, const snet_idxvec_t *idx); 

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupBox(
    snet_gnode_t *n,
    snet_box_name_t name,
    snet_box_fptr_t func,
    snet_box_sign_t *sign, snet_metadata_t *meta);

/*---*/

extern void
SNetGNodeSetupSyncCell(
    snet_gnode_t *n,
    snet_typeencoding_t *out_type,
    snet_typeencoding_t *patterns, snet_expr_list_t *guards);

/*---*/

extern void SNetGNodeSetupFilter(snet_gnode_t *n);

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupStar(
    snet_gnode_t *n,
    bool is_det,
    snet_typeencoding_t *type, 
    snet_expr_list_t *guards, snet_gnode_t *groot);

/*---*/

extern void
SNetGNodeSetupSplit(
    snet_gnode_t *n, bool is_det, snet_gnode_t *groot, int idx_tag);

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupParallel(
    snet_gnode_t *n,
    bool is_det, unsigned int branches_cnt);

/*---*/

extern void
SNetGNodeParallelSetupBranch(
    snet_gnode_t *n,
    unsigned int idx,
    snet_gnode_t *groot, snet_typeencoding_t *type);

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupFilter(snet_gnode_t *n);

extern void 
SNetGNodeSetupNameShift(snet_gnode_t *n);

extern void
SNetGNodeSetupExternConn(snet_gnode_t *n, snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/

extern const snet_box_sign_t*
SNetGNodeGetBoxTypeSignature(const snet_gnode_t *n);

/*----------------------------------------------------------------------------*/

extern snet_base_t* 
SNetGNodeToBase(snet_gnode_t *n);

extern const snet_base_t* 
SNetGNodeToBaseConst(const snet_gnode_t *n);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Functions that operate on a whole (or a whole section) of a graph. */

extern void
SNetGraphDestroy(snet_gnode_t *groot);

extern void
SNetGraphEnumerate(snet_gnode_t *groot);

#endif // __SVPSNETGWRT_GW_GRAPH_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

