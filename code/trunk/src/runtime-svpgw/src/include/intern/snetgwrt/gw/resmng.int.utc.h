/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : resmng.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_RESMNG_INT_H
#define __SVPSNETGWRT_GW_RESMNG_INT_H

#include "common.int.utc.h"
#include "metadata.int.utc.h"

/*---*/

#include "core/plcmng.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetResMngSubSystemInit();

extern void
SNetResMngSubSystemDestroy();

/*----------------------------------------------------------------------------*/

extern void
SNetResourceRelease(snet_place_t plc);

/*---*/

extern snet_place_t
SNetResourceRequestBox(
    snet_box_name_t bn, snet_metadata_t *bmeta);

extern snet_place_t
SNetResourceRequestDomain(snet_metadata_t *nmeta);

#endif // __SVPSNETGWRT_GW_RESMNG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

