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

#ifndef __SVPSNETGWRT_DOMAIN_H
#define __SVPSNETGWRT_DOMAIN_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/**
 * An S-Net domain represents an S-Net network within the runtime and it
 * acts as a holder for all the structures required by the runtime to run
 * a network (e.g cons-list, graph representation, etc...).
 */
typedef struct domain snet_domain_t;

/**
 * Required declaration because including the file
 * "graph.utc.h" would lead to a cyclic dependency.
 */
typedef struct graph_node snet_gnode_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_domain_t* SNetDomainCreate();

/*---*/

extern void SNetDomainDestroy(snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/

extern void 
SNetDomainSetGraph(snet_domain_t *snetd, snet_gnode_t *groot);

/*---*/

// extern void
// SNetDomainReplaceGraphNode(
//     snet_domain_t *snetd, const snet_idxvec_t *idx, const snet_gnode_t *n);

#endif // __SVPSNETGWRT_DOMAIN_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/
