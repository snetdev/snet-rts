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

#ifndef __SVPSNETGWRT_GW_GRAPH_H
#define __SVPSNETGWRT_GW_GRAPH_H

#include "common.utc.h"

/*---*/

#include "snetgwrt/core/handle.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Datatype for a node of
 * the graph.
 */
typedef struct graph_node snet_gnode_t;

/**
 * Function pointer type for box
 * proxy functions.
 */
typedef void (*snet_box_fptr_t)(snet_handle_t *);

#endif // __SVPSNETGWRT_GW_GRAPH_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

