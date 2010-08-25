/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : sane.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : This file contains runtime initialization and cleanup
                     functions as well as "primary" runtime services.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_SANE_SANE_H
#define __SVPSNETGWRT_SANE_SANE_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/

extern void
SNetGlobalSaneInit();

/*---*/

extern void
SNetGlobalSaneDestroy();

extern void
SNetGlobalSaneDestroyEx(bool force);

#endif // __SVPSNETGWRT_SANE_SANE_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

