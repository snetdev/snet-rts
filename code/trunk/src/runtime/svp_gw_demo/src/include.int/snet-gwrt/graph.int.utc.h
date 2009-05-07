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

#ifndef __SVPSNETGWRT_GRAPH_INT_H
#define __SVPSNETGWRT_GRAPH_INT_H

#include "graph.utc.h"

/*---*/

#include "common.int.utc.h"
#include "basetype.int.utc.h"
#include "graphindex.int.utc.h"
#include "typeencode.int.utc.h"
#include "expression.int.utc.h"
#include "filteriset.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void 
SNetGNodeInit(
    snet_gnode_t *n,
    snet_gnode_type_t type,
    const snet_domain_t *snetd);

/*---*/

extern snet_gnode_t*
SNetGNodeCreate(
    snet_gnode_type_t type,
    const snet_domain_t *snetd);

/*---*/

extern void SNetGNodeDestroy(snet_gnode_t *n, bool deep);

/*----------------------------------------------------------------------------*/

extern snet_gnode_type_t SNetGNodeGetType(const snet_gnode_t *n);

/*----------------------------------------------------------------------------*/

extern void 
SNetGNodeConnect(snet_gnode_t *n1, snet_gnode_t* n2, bool merge_lnk);

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupNormal(snet_gnode_t *n, const snet_ginx_t *inx); 

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupBox(
    snet_gnode_t *n,
    snet_box_fptr_t func, snet_box_sign_t *sign);

/*---*/

extern void
SNetGNodeSetupSync(
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
    snet_gnode_t *n, bool is_det, snet_gnode_t *groot, int tag);

/*----------------------------------------------------------------------------*/

extern void
SNetGNodeSetupParallel(
    snet_gnode_t *n,
    bool is_det, unsigned int branches_cnt);

/*---*/

extern void
SNetGNodeParallelSetupBranch(
    snet_gnode_t *n,
    unsigned int inx,
    snet_gnode_t *groot, snet_typeencoding_t *type);

/*----------------------------------------------------------------------------*/

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

extern void
SNetGraphEnumerate(snet_gnode_t *start_node);

#endif // __SVPSNETGWRT_GRAPH_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

