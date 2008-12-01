/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : snetentities.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header included from the S-Net compiler 
                     generated code as a master header for the
                     runtime.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_SNET_GWRT_SNETC_SNETENTITIES_H
#define __SVPSNETGWRT_SNET_GWRT_SNETC_SNETENTITIES_H

#include "snet-gwrt.utc.h"

/*----------------------------------------------------------------------------*/

/**
 * Datatype representing a stream that is
 * used by the S-Net compiler as input for the
 * "entities" functions of the CI. For us this
 * is also "snet_gnode_t". 
 *
 * It is required so that no changes are needed in 
 * the compiler generated code.
 */
typedef snet_gnode_t snet_tl_stream_t;

#endif // __SVPSNETGWRT_SNET_GWRT_SNETC_SNETENTITIES_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

