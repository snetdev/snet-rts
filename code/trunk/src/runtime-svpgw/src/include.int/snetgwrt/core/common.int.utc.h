/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

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

#ifndef __SVPSNETGWRT_CORE_COMMON_INT_H
#define __SVPSNETGWRT_CORE_COMMON_INT_H

#include "core/common.utc.h"

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
$include <stdarg.h>

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

#define SNET_ERR_NONE                   0x0000
#define SNET_ERR_UNEXPECTED             0x0001
#define SNET_ERR_INIT                   0x0002
#define SNET_ERR_MEMORY                 0x0003
#define SNET_ERR_IO                     0x0004
#define SNET_ERR_TYPE_ERROR             0x0005
#define SNET_ERR_PLACE                  0x0006

// Warnings!!

#define SNET_WRN_NONE                   0x0000
#define SNET_WRN_UNSUPPORTED            0x0001
#define SNET_WRN_IGNORED                0x0002
#define SNET_WRN_REC_FDATA_TOO_LARGE    0x0003
#define SNET_WRN_REC_FDATA_COPY_FAILED  0x0004
#define SNET_WRN_NULL_BLI_FUNCTION      0x0005
#define SNET_WRN_REC_ITEM_NOT_FOUND     0x0006

/*----------------------------------------------------------------------------*/
/* Helper macros for commonly used error messages. */

// Sub-system initialization
#define SNET_ERR_SUBSYS_INIT_MSG(subsys, cause) \
    "an error during the "                      \
    "initialization of the " subsys "sub-system (" cause ")"


/*----------------------------------------------------------------------------*/
/* Helper macros for setting, resetting and checking flags. */

#define __chk_flag(flags, f)   (((flags) & (f)) != 0)
#define __set_flag(flags, f)   do { (flags) |= (f); } while (0)
#define __reset_flag(flags, f) do { (flags) &= (~(f)); } while (0)

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Error and Warning management functions */

extern void SNetReportError(unsigned int code, ...);
extern void SNetReportWarning(unsigned int code, ...);

extern void SNetReportErrorCustom(const char *fmt, ...);
extern void SNetReportWarningCustom(const char *fmt, ...);

/*----------------------------------------------------------------------------*/

extern int
SNetRegisterErrorSource(const char **msgv);

extern int
SNetRegisterWarningSource(const char **msgv);

/*---*/

extern void 
SNetReportNonCoreError(
    unsigned int srcid, unsigned int code, ...);

extern void 
SNetReportNonCoreWarning(
    unsigned int srcid, unsigned int code, ...);

extern void 
SNetReportNonCoreErrorV(
    unsigned int srcid, unsigned int code, va_list vargs);

extern void 
SNetReportNonCoreWarningV(
    unsigned int srcid, unsigned int code, va_list vargs);

/*----------------------------------------------------------------------------*/

extern void SNetOnError();
extern void SNetOnWarning();

#endif // __SVPSNETGWRT_CORE_COMMON_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

