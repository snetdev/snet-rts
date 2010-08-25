/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : svp4snetc.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : This file contains APIs specific to the SVP4SNet
                     compiler plugin.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_SVP4SNETC_H
#define __SVPSNETGWRT_GW_SVP4SNETC_H

#include "common.utc.h"

/*---*/

#include "snetgwrt/core/handle.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * SNetGetBoxSelectedResource() function used
 * to retrieve the resource/ SVP place that
 * the runtime has has selected as the most
 * appropriate to execute a given box.
 * 
 * Note that the returned place is only valid
 * within the context of the "box proxy" function
 * that has been given to the runtime from user
 * code and which it calls to "execute" the box!!
 *
 * In any other case whether the returned place is
 * valid cannot be guaranteed (because after that
 * function returns the runtime is free to release
 * the place)!
 */
extern place
SNetGetBoxSelectedResource(const snet_handle_t *hnd);

/*----------------------------------------------------------------------------*/
/**
 * Function that can be used within "default" definitions
 * of a box for a place in order to warn the user that the 
 * box is not implemented on that place (see the "svp4snetc"
 * compiler plugin for an example use of this function).
 */
extern void
SNetWarnMissingBoxDefinition(const snet_handle_t *hnd);

#endif // __SVPSNETGWRT_GW_SVP4SNETC_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

