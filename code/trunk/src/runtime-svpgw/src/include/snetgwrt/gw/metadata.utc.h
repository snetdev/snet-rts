/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : metadata.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_METADATA_H
#define __SVPSNETGWRT_GW_METADATA_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Datatypes for the "name" of a box and service
 * identifiers (currently only as simple strings).
 */
typedef const char* snet_box_name_t;
typedef const char* snet_box_serviceid_t;

/*---*/

/**
 * Datatype for "bool/flag" like metadata
 * values.
 */
typedef bool snet_metadata_bool_t;

/**
 * Type for "generic"  metadata 
 * representation.
 */
typedef struct metadata snet_metadata_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Specialized metadata types */

typedef enum {
    SNET_METADATA_TYPE_NET,
    SNET_METADATA_TYPE_BOX,
    SNET_METADATA_TYPE_GLOBAL

} snet_metadata_type_t;

/*----------------------------------------------------------------------------*/

typedef struct {
    /**
     * Not implemented yet.
     */
} snet_net_metadata_t;

/*---*/

typedef struct {
    snet_box_serviceid_t service_id;

    snet_metadata_bool_t exec_local_always;
    snet_metadata_bool_t exec_local_ifnoresource;

} snet_box_metadata_t;

/*---*/

typedef struct {
    /**
     * Not implemented yet.
     */
} snet_global_metadata_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Loading and destruction
 *
 * This functions can be "overriden" by "user" code. That is their
 * definitions have been specified as "weak" within the library thus
 * if a module is linked with the library that contains definitions
 * of these the linker will use the ones within that module.
 * 
 * This has been done to allow "user" code to specify metadata to the
 * runtime (since the S-Net compiler does not do it; hopefully not yet).
 *
 * This is the only reason that these functions and the types above
 * are declared within this "public" header and why several "helper"
 * functions are also provided (see cast and wrap functions below).
 */

extern snet_metadata_t*
SNetMetaDataLoad(snet_metadata_type_t, ...);
 
extern void
SNetMetaDataDestroySpecialized(snet_metadata_t *meta);

/*----------------------------------------------------------------------------*/
/* Properties */

extern snet_metadata_type_t
SNetMetaDataGetType(const snet_metadata_t *meta);

/*----------------------------------------------------------------------------*/
/**
 * This function wraps specialized metadata data-structures into
 * a generic "snet_metadata_t" that the runtime is using (it should
 * be used in cases like within the "SNetMetaDataLoad()" function to
 * wrap the return value; that is when that function is "overrided"
 * by "user" code).
 */
extern snet_metadata_t*
SNetMetaDataWrap(
    snet_metadata_type_t type, 
    const void *meta, bool explicit_destroy);

/*----------------------------------------------------------------------------*/
/**
 * Cast function that can be used to convert a pointer
 * from a generic "snet_metadata_t" type to a specialized one.
 *
 * The function returns "void *" so that we have only one function
 * for all specialized metadata types. However casting with normal
 * cast the returned value from this function guarrantees a valid 
 * pointer to a specialized type.
 * 
 * e.g.
 *      void foo(snet_metadata_t *meta)
 *      {
 *          snet_box_metadata_t *bm =
 *               (snet_box_metadata_t *)(SNetMetaDataCastToSpecialized(meta));
 *          .
 *          .
 *          .
 *      }
 */
extern void*
SNetMetaDataCastToSpecialized(const snet_metadata_t *meta);

#endif // __SVPSNETGWRT_GW_METADATA_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

