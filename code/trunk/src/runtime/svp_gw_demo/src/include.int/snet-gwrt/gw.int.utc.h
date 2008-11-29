/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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

#ifndef __SVPSNETGWRT_GW_INT_H
#define __SVPSNETGWRT_GW_INT_H

#include "common.int.utc.h"
#include "graph.int.utc.h"
#include "record.int.utc.h"
#include "handle.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void SNetGWSetupNormalGNodeHndFuncs(snet_gnode_t *n);

/*----------------------------------------------------------------------------*/

extern thread void SNetGW(
    snet_handle_t *hnd, snet_record_t *rec);

extern thread void SNetGWRaw(
    snet_handle_t *hnd, int variant_num, void **args);

extern thread void SNetGWRawArray(
    snet_handle_t *hnd, 
    int if_id, int var_num, void **fields, int **tags, int **btags);

#endif // __SVPSNETGWRT_GW_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

