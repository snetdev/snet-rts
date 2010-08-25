/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : dutcmacros.int.utc.h

    File Type      : Header File.

    ---------------------------------------

    File 
    Description    : This file contains some helper macros to make code
                     within I/O functions of "distributable" thread
                     functions more clear.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_SANE_DUTCMACROS_INT_H
#define __SVPSNETGWRT_SANE_DUTCMACROS_INT_H

#include "common.int.utc.h"

/*---*/

#define dutc_db           uTC::DataBuffer*
#define dutc_global(type) type&
#define dutc_shared(type) type*&
#define dutc_io_phase(db) db->phase()

#endif // __SVPSNETGWRT_SANE_DUTCMACROS_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

