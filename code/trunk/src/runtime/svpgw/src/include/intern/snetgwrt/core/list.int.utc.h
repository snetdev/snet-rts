/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : list.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_LIST_INT_H
#define __SVPSNETGWRT_LIST_INT_H

#include "core/list.utc.h"

/*---*/

#include "common.int.utc.h"
#include "basetype.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern snet_base_t*
SNetListToBase(snet_list_t *lst);

extern const snet_base_t*
SNetListToBaseConst(const snet_list_t *lst);

#endif // __SVPSNETGWRT_LIST_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/


