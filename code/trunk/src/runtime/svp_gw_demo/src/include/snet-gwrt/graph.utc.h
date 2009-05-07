/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : graph.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GRAPH_H
#define __SVPSNETGWRT_GRAPH_H

#include "common.utc.h"
#include "handle.utc.h"
#include "domain.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/**
 * Datatype for a node of
 * the graph.
 */
typedef struct graph_node snet_gnode_t;

/*---*/
/**
 * Function pointer type for box
 * proxy functions.
 */
typedef int (*snet_box_fptr_t)(snet_handle_t *);

/*----------------------------------------------------------------------------*/

typedef enum {
    GRAPH_NODE_TYPE_NOP,
    
    GRAPH_NODE_TYPE_BOX,
    GRAPH_NODE_TYPE_SYNC,
    GRAPH_NODE_TYPE_FILTER,

    GRAPH_NODE_TYPE_COMB_STAR,
    GRAPH_NODE_TYPE_COMB_SPLIT,
    GRAPH_NODE_TYPE_COMB_PARALLEL,

    GRAPH_NODE_TYPE_EXTERN_CONNECTION

} snet_gnode_type_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_gnode_t*
SNetGraphCreate(snet_domain_t *snetd);

/*---*/

extern void SNetGraphDestroy(snet_gnode_t *groot);

#endif // __SVPSNETGWRT_GRAPH_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

