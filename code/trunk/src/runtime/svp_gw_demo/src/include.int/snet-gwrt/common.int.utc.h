/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : common.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : Header file with symbols macros and declarations
                     common to all modules.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_COMMON_INT_H
#define __SVPSNETGWRT_COMMON_INT_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * Include the default configuration header file
 * if no other has been included yet.
 */
#ifndef SVPSNETGWRT_INTERN_CFG_HEADER
#include "config.int.utc.h"
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * __create_mutex() and __sync_mutex() macros.
 *
 * e.g.
 *      __create_mutex(p) {
 *          .
 *          .
 *          .
 *      } __sync_mutex(p);
 */

#define __create_mutex(plc) \
    pthread_mutex_lock(plc->exclussion_mutex); if (1)

#define __sync_mutex(plc) pthread_mutex_unlock(plc->exclussion_mutex)

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Common includes
 */
$include <assert.h>

#if SVPSNETGWRT_DEBUG > 0
$include <stdio.h>
$include <stdlib.h>
$include <string.h>
#endif

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Error and Warning codes.
 */

// Errors!!

#define SNET_ERR_NONE               0x0000
#define SNET_ERR_UNEXPECTED         0x0001
#define SNET_ERR_MEMORY             0x0002
#define SNET_ERR_IO                 0x0003
#define SNET_ERR_TYPE_ERROR         0x0004
#define SNET_ERR_PLACE              0x0005
#define SNET_ERR_BOX                0x0006

// Warnings!!

#define SNET_WRN_NONE               0x0000
#define SNET_WRN_UNSUPPORTED        0x0001
#define SNET_WRN_IGNORED            0x0002
#define SNET_WRN_IGNORED_ENTITY     0x0003

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Error and Warning functions */

extern void SNetReportError(unsigned int code, ...);
extern void SNetReportWarning(unsigned int code, ...);

extern void SNetReportErrorCustom(const char *fmt, ...);
extern void SNetReportWarningCustom(const char *fmt, ...);

/*---*/

extern void SNetOnError();
extern void SNetOnWarning();

#endif // __SVPSNETGWRT_COMMON_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

