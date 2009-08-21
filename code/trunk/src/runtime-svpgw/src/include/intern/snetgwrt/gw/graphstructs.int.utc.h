/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

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

#ifndef __SVPSNETGWRT_GW_GRAPHSTRUCTS_INT_H
#define __SVPSNETGWRT_GW_GRAPHSTRUCTS_INT_H

#include "graph.int.utc.h"
#include "domain.int.utc.h"
#include "gwhandle.int.utc.h"

/*---*/

#include "core/list.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef uint16_t snet_gnode_flags_t;

/*---*/

typedef struct {
    /**
     * Not implemented yet.
     */
} snet_gnode_stats_t;

/*----------------------------------------------------------------------------*/

typedef struct {
    unsigned int      id;
    snet_idxvec_t    *idx;
    unsigned int      mpcnt;
    snet_gwhandle_t **mhnds;

} snet_synccell_state_t;

/*---*/

typedef struct {
    unsigned int used;
    snet_place_t plc;

} snet_box_execplcinfo_t;

/*---*/

typedef struct {
    unsigned int         taken_cnt;
    snet_gnode_t        *groot;        
    snet_typeencoding_t *type;

} snet_par_comb_branch_t;

/*----------------------------------------------------------------------------*/
/* Box nodes */

typedef struct {
    snet_box_name_t  name;
    snet_box_name_t  namex;
    snet_box_fptr_t  func;
    snet_box_sign_t *sign;

    snet_metadata_t *meta;
    snet_list_t     *exec_plc_cache;

} snet_box_gnode_t;

/*---*/

typedef struct {
    snet_typeencoding_t *out_type;
    snet_typeencoding_t *patterns;
    snet_expr_list_t    *guards;

    snet_list_t         *states;
    snet_list_t         *ident_states_idxs;

} snet_synccell_gnode_t;

/*---*/

typedef struct {
    /**
     * Not implemented yet.
     */
} snet_filter_gnode_t;

/*---*/

typedef struct {
    /**
     * Not implemented yet.
     */
} snet_nameshift_gnode_t;

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
    int           idx_tag;
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
    snet_gnode_t *gnode, snet_gwhandle_t *hnd);

typedef snet_gnode_t* 
    (*snet_gnode_walk_hnd_fptr_t)(snet_gnode_t *gnode, snet_gwhandle_t *hnd);


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef struct nop_graph_node {
    snet_gnode_t          *parent;
    snet_gnode_contlink_t  lnk;

} snet_nop_gnode_t;

/*----------------------------------------------------------------------------*/

typedef struct normal_graph_node {
    snet_gnode_t          *parent;
    snet_idxvec_t         *idx;
    snet_gnode_contlink_t  lnk;

    snet_gnode_hnd_fptr_t      hnd_func;
    snet_gnode_walk_hnd_fptr_t walk_hnd_func;
    snet_gnode_walk_hnd_fptr_t walk_hnd_func_merge;

    union {
        snet_box_gnode_t         *box;
        snet_synccell_gnode_t    *synccell;
        snet_filter_gnode_t      *filter;
        snet_nameshift_gnode_t   *nameshift;

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
    snet_base_t         base;
    snet_gnode_type_t   type;
    snet_gnode_flags_t  flags;
    snet_gnode_stats_t  stats;

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
    snet_gnode_t *gnode, const snet_idxvec_t *idx);

extern void 
SNetSyncCellGNodeRemoveState(
    snet_gnode_t *gnode, unsigned int state_id, bool set_as_ident);

#endif // __SVPSNETGWRT_GW_GRAPHSTRUCTS_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

