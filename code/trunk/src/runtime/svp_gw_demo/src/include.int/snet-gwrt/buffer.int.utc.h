/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : buffer.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_BUFFER_INT_H
#define __SVPSNETGWRT_BUFFER_INT_H

#include "buffer.utc.h"

/*---*/

#include "common.int.utc.h"
#include "basetype.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_base_t*
SNetBufToBase(snet_buffer_t *buf);

extern const snet_base_t*
SNetBufToBaseConst(const snet_buffer_t *buf);

#endif // __SVPSNETGWRT_BUFFER_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

