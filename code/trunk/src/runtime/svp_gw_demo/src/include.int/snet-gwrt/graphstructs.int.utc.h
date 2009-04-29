/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : graphstructs.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GRAPHSTRUCTS_INT_H
#define __SVPSNETGWRT_GRAPHSTRUCTS_INT_H

#include "graph.int.utc.h"
#include "domain.int.utc.h"
#include "conslist.int.utc.h"
#include "list.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef struct {
    unsigned int running_gw_cnt;
    unsigned int processed_recs_cnt;

} snet_gnode_stats_t;

/*---*/

typedef struct {
    unsigned int    id;
    snet_ginx_t    *inx;
    unsigned int    mpcnt;
    snet_handle_t **mhnds;

} snet_synccell_state_t;

/*---*/

typedef struct {
    unsigned int           snetd_id;
    snet_domain_type_t     snetd_type;
    snet_place_contract_t *cntr;

} snet_box_execplc_t;

/*---*/

typedef struct {
    unsigned int         taken_cnt;
    snet_gnode_t        *groot;        
    snet_typeencoding_t *type;

} snet_par_comb_branch_t;

/*----------------------------------------------------------------------------*/
/* Box nodes */

typedef struct {
    snet_box_fptr_t     func;
    snet_box_sign_t    *sign;

    snet_list_t        *exec_plc_cache;
    snet_box_execplc_t  exec_plc_default;

} snet_box_gnode_t;

/*---*/

typedef struct {
    snet_typeencoding_t *out_type;
    snet_typeencoding_t *patterns;
    snet_expr_list_t    *guards;

    snet_list_t         *states;
    snet_list_t         *ident_states_inxs;

} snet_synccell_gnode_t;

/*---*/

typedef struct {

} snet_filter_gnode_t;

/*----------------------------------------------------------------------------*/
/* Combinator nodes */

typedef struct {
    bool                    is_det;   
    unsigned int            branches_cnt;
    snet_par_comb_branch_t *branches;

} snet_parallel_gnode_t;

/*---*/

typedef struct {
    snet_typeencoding_t *type;
    snet_expr_list_t    *guards;
    snet_gnode_t        *groot;

} snet_star_gnode_t;

/*---*/

typedef struct {
    int           tag;
    snet_gnode_t *groot;

} snet_split_gnode_t;

/*---*/

typedef struct {
    snet_domain_t *snetd;
    
} snet_extern_conn_gnode_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef struct {
    bool merge;

    snet_gnode_t *node;
    snet_gnode_t *cached_node;

} snet_gnode_contlink_t;

/*---*/

typedef bool (*snet_gnode_hnd_fptr_t)(
    snet_gnode_t *gnode, snet_handle_t *hnd);

typedef snet_gnode_t* 
    (*snet_gnode_walk_hnd_fptr_t)(snet_gnode_t *gnode, snet_handle_t *hnd);


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef struct nop_graph_node {
    snet_gnode_t          *parent;
    snet_gnode_contlink_t  lnk;

} snet_nop_gnode_t;

/*----------------------------------------------------------------------------*/

typedef struct normal_graph_node {
    snet_gnode_t *parent;
    snet_ginx_t  *inx;

    /*---*/

    snet_gnode_contlink_t  lnk;
    snet_gnode_stats_t     stats;

    /*---*/

    snet_gnode_hnd_fptr_t      hnd_func;
    snet_gnode_walk_hnd_fptr_t walk_hnd_func;
    snet_gnode_walk_hnd_fptr_t walk_hnd_func_merge;

    /*---*/

    union {
        snet_box_gnode_t         *box;
        snet_synccell_gnode_t    *synccell;
        snet_filter_gnode_t      *filter;

        snet_parallel_gnode_t    *parallel;
        snet_star_gnode_t        *star;
        snet_split_gnode_t       *split;
        snet_extern_conn_gnode_t *extern_conn;

    } entity;

} snet_normal_gnode_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Generic node that combines all the above */

struct graph_node {
    snet_base_t       base;
    snet_gnode_type_t type;
    bool              synccells_ahead;

    union {
        snet_nop_gnode_t    *nop;
        snet_normal_gnode_t *normal;

    } data;

}; // struct graph_node

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Functions related to state management of synchro-cell nodes
 * that are used only by the GW.
 */

extern snet_synccell_state_t*
SNetSyncCellGNodeAddState(
    snet_gnode_t *gnode, const snet_ginx_t *ginx);

extern void 
SNetSyncCellGNodeRemoveState(
    snet_gnode_t *gnode, unsigned int state_id, bool set_as_ident);

#endif // __SVPSNETGWRT_GRAPHSTRUCTS_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

