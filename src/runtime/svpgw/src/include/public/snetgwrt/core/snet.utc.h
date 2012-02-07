/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : snet.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    : This file contains functions that implement the primary 
                     services of the runtime.

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_CORE_SNET_H
#define __SVPSNETGWRT_CORE_SNET_H

#include "common.utc.h"
#include "handle.utc.h"

/*---*/

$include <stdio.h>
$include <stdarg.h>

/*----------------------------------------------------------------------------*/
/**
 * Serialization formats. This cannot be an enumration
 * because it should be possible for users to add more
 * formats. This is also why the format argument to the
 * serialization and deserialization functions is of type
 * "int" instead of an enumeration. Here we just reserve 
 * the first 3 values for the formats the runtime has (will
 * have) support for.
 *
 * These formats are the "compalsory" ones (i.e. any box 
 * language interface is expected to / should support at 
 * lesast these)!
 */
#define SNET_DATASER_FORMAT_BIN    0x000
#define SNET_DATASER_FORMAT_TXT    0x001
#define SNET_DATASER_FORMAT_XML    0x002

/**
 * The codes for any additional to the above serialization
 * formats, a box language interface supports should start
 * from the value specified below.
 *
 * e.g. #define SNET_DATASER_FORMAT_FOO SNET_DATASER_FORMAT_CUSTOM + 1
 */
#define SNET_DATASER_FORMAT_CUSTOM 0x400


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Datatype for a record (forward declaration
 * to avoid including the file "record.utc.h").
 */
typedef struct record snet_record_t;


/**
 * Types related to the box language
 * interface system.
 */

typedef unsigned int snet_bli_id_t;

/**
 * Functions a box language interface
 * should provide.
 */
typedef void  (*snet_bli_freefptr_t)(void *);
typedef void* (*snet_bli_copyfptr_t)(void *);

/*---*/

typedef size_t (*snet_bli_serfptr_t)(unsigned int, void *, void **);
typedef void*  (*snet_bli_deserfptr_t)(unsigned int, void *, size_t);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Sets the stream where the runtime will output its
 * error and warning messages (if any). If not set
 * the default will be "stderr".
 */
extern void SNetGlobalSetErrorStream(FILE *stream);

/*----------------------------------------------------------------------------*/
/**
 * Box lanugage interface related routines.
 */

extern bool
SNetGlobalBliRegister(
    snet_bli_id_t        id,
    snet_bli_freefptr_t  freefun,
    snet_bli_copyfptr_t  copyfun,
    snet_bli_serfptr_t   serfun,
    snet_bli_deserfptr_t deserfun);

/*---*/

extern snet_bli_freefptr_t
SNetBliGetFreeFun(snet_bli_id_t id);

extern snet_bli_copyfptr_t
SNetBliGetCopyFun(snet_bli_id_t id);

extern snet_bli_serfptr_t
SNetBliGetSerializationFun(snet_bli_id_t id);

extern snet_bli_deserfptr_t
SNetBliGetDeserializationFun(snet_bli_id_t id);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * SNetOutxxx() functions that are called from boxes to emmit
 * output records.
 */

extern void
SNetOut(snet_handle_t *hnd, snet_record_t *rec);

extern void 
SNetOutRaw(
    snet_handle_t *hnd, 
    snet_bli_id_t blid, int variant_idx, ...);

extern void
SNetOutRawV(
    snet_handle_t *hnd, 
    snet_bli_id_t blid, int variant_idx, va_list vargs);

extern void
SNetOutRawArray(
    snet_handle_t *hnd, 
    snet_bli_id_t blid,
    int variant_idx, void **fields, int *tags, int *btags);

#endif // __SVPSNETGWRT_CORE_SNET_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

