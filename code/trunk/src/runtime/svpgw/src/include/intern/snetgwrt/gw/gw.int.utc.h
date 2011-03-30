/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : gw.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_GW_INT_H
#define __SVPSNETGWRT_GW_GW_INT_H

#include "gw/gw.utc.h"

/*---*/

#include "common.int.utc.h"
#include "graph.int.utc.h"

/*---*/

#include "core/record.int.utc.h"
#include "core/handle.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetGwSetupNormalGNodeHndFuncs(snet_gnode_t *n);

/*---*/

/**
 * The library-level "public" interface thread functions
 * for the Graph Walker.
 */
  
extern thread void 
SNetGwInit(
    snet_ref_t snetd, 
    snet_handle_t *hnd, snet_record_t *rec);

extern thread void 
SNetGwFork(snet_handle_t *hnd, snet_record_t *rec);

#endif // __SVPSNETGWRT_GW_GW_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

