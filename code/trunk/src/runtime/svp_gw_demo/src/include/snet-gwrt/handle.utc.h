/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : handle.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_HANDLE_H
#define __SVPSNETGWRT_HANDLE_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

/**
 * Generic S-Net handle.
 */
typedef struct handle snet_handle_t;

/**
 * Datatype for a record (forward declaration
 * to avoid including the file "record.utc.h").
 */
typedef struct record snet_record_t;

/*----------------------------------------------------------------------------*/
/**
 * Returns the input record with which
 * a box was called.
 */

extern snet_record_t*
SNetHndGetRecord(const snet_handle_t *hnd);

#endif // __SVPSNETGWRT_HANDLE_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

