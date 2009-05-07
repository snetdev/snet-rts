/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : filteriset.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_FILTERISET_H
#define __SVPSNETGWRT_FILTERISET_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef enum {
	FILTER_OP_TAG,
	FILTER_OP_BTAG,
	FILTER_OP_FIELD,
	FILTER_OP_CREATE_REC

} snet_filter_opcode_t;

#endif // __SVPSNETGWRT_FILTERISET_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

