/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : snet.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : 

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_CORE_SNET_INT_H
#define __SVPSNETGWRT_CORE_SNET_INT_H

#include "core/snet.utc.h"

/*---*/

#include "common.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Initialization and cleanup of the "core" of the runtime (these
 * is supposed to be called by parts of the runtime that work on
 * top of the core (e.g. the "gw" library).
 */
extern void SNetGlobalCoreInit();
extern void SNetGlobalCoreDestroy();
extern void SNetGlobalCoreDestroyEx(bool force);

#endif // __SVPSNETGWRT_CORE_SNET_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

