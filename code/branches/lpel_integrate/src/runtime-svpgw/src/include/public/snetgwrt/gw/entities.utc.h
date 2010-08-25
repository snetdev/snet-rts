/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : entities.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_ENTITIES_H
#define __SVPSNETGWRT_GW_ENTITIES_H

#include "common.utc.h"
#include "graph.utc.h"
#include "domain.utc.h"
#include "metadata.utc.h"
#include "expression.utc.h"
#include "filteriset.utc.h"

/*---*/

#include "snetgwrt/core/typeencode.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Function pointer type for functions
 * that are given as operands to the combinators
 */
typedef snet_gnode_t* (*snet_comb_op_fptr_t)(snet_gnode_t *);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetBox(
    snet_gnode_t *in,
    snet_box_name_t name,
    snet_box_fptr_t func,
    snet_box_sign_t *sign);

/*---*/

extern snet_gnode_t*
SNetSync(
    snet_gnode_t *in, 
    snet_typeencoding_t *outtype,
    snet_typeencoding_t *patterns, snet_expr_list_t *guards);

/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetSerial(
    snet_gnode_t *in,
    snet_comb_op_fptr_t left, snet_comb_op_fptr_t right);

/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetParallel(
    snet_gnode_t *in,
	snet_typeencoding_list_t *types, ...);

/*---*/

extern snet_gnode_t*
SNetParallelDet(
    snet_gnode_t *in,
	snet_typeencoding_list_t *types, ...);

/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetStar(
    snet_gnode_t *in, 
    snet_typeencoding_t *type,
	snet_expr_list_t *guards,
    snet_comb_op_fptr_t op_a, snet_comb_op_fptr_t op_b);

extern snet_gnode_t*
SNetStarIncarnate(
    snet_gnode_t *in, 
    snet_typeencoding_t *type,
	snet_expr_list_t *guards,
    snet_comb_op_fptr_t op_a, snet_comb_op_fptr_t op_b);

/*---*/

extern snet_gnode_t*
SNetStarDet(
    snet_gnode_t *in, 
    snet_typeencoding_t *type,
	snet_expr_list_t *guards,
    snet_comb_op_fptr_t op_a, snet_comb_op_fptr_t op_b);

extern snet_gnode_t*
SNetStarDetIncarnate(
    snet_gnode_t *in, 
    snet_typeencoding_t *type,
	snet_expr_list_t *guards,
    snet_comb_op_fptr_t op_a, snet_comb_op_fptr_t op_b);

/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetSplit(
    snet_gnode_t *in,
    snet_comb_op_fptr_t op, int ltag, int utag);

extern snet_gnode_t*
SNetSplitDet(
    snet_gnode_t *in,
    snet_comb_op_fptr_t op, int ltag, int utag);

/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetFilter(
    snet_gnode_t *in,
    snet_typeencoding_t *in_type, snet_expr_list_t *guards, ...);

extern snet_gnode_t*
SNetNameShift(
    snet_gnode_t *in, int offset, snet_variantencoding_t *untouched);

/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetExternConnection(snet_gnode_t *in, snet_domain_t *snetd);

#endif // __SVPSNETGWRT_GW_ENTITIES_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

