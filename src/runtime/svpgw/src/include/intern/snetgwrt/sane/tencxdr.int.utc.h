/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : tencxdr.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_SANE_TENCXDR_INT_H
#define __SVPSNETGWRT_SANE_TENCXDR_INT_H

#include "common.int.utc.h"

/*---*/

#include "core/typeencode.int.utc.h"

/*---*/

// #include <rpc/rpc.h>
// #include <rpc/xdr.h>

/*----------------------------------------------------------------------------*/

extern void
SNetSaneXDRTencVector(XDR *xdrs, snet_vector_t *vec);

extern void
SNetSaneXDRTencVariant(XDR *xdrs, snet_variantencoding_t *venc);

/*----------------------------------------------------------------------------*/

extern void
SNetSaneXDRTencType(XDR *xdrs, snet_typeencoding_t *tenc);

/*---*/

extern void
SNetSaneXDRTencBoxSing(XDR *xdrs, snet_box_sign_t *sign);

#endif // __SVPSNETGWRT_SANE_TENCXDR_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

