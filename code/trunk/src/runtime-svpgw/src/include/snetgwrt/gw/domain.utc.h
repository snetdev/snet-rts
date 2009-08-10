/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : domain.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_DOMAIN_H
#define __SVPSNETGWRT_GW_DOMAIN_H

#include "common.utc.h"

/*---*/

#include "snetgwrt/core/record.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/**
 * An S-Net domain represents an S-Net network within the runtime and it
 * acts as a holder for all the structures required by the runtime to run
 * a network (e.g cons-list, graph representation, etc...).
 */
typedef struct domain snet_domain_t;

/**
 * Datatype for a node of the graph (the 
 * file "graph.utc.h" is not included
 * to avoid cyclic references).
 */
typedef struct graph_node snet_gnode_t;

/*---*/

/**
 * Function pointer for "graph constructor"
 * type of functions.
 */
typedef snet_gnode_t*
    (*snet_graphctor_fptr_t)(snet_gnode_t *);

/**
 * Thread function pointer for "network output handling"
 * functions. The given thread function will be created
 * whenever there is something in the output buffer.
 */
typedef thread void
    (*snet_netout_tfptr_t)(snet_domain_t *, unsigned int count);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern 
snet_domain_t*
SNetDomainCreate(
    snet_graphctor_fptr_t gctor,
    snet_netout_tfptr_t netout_tfun);

/*---*/

extern void
SNetDomainDestroy(snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

// extern void
// SNetDomainReplaceGraphNode(
//    snet_domain_t *snetd,
//    const snet_idxvec_t *idx,
//    const snet_graphctor_fptr_t gctor);

#endif // __SVPSNETGWRT_GW_DOMAIN_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

