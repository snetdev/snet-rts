/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : metadata.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GW_METADATA_INT_H
#define __SVPSNETGWRT_GW_METADATA_INT_H

#include "gw/metadata.utc.h"

/*---*/

#include "common.int.utc.h"

/*---*/

#include "core/plcmng.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Cleanup function
 */
extern void
SNetMetaDataDestroy(snet_metadata_t *meta);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Box "name" manipulation
 * functions.
 */

extern snet_box_name_t
SNetMetaDataBoxNameCopy(snet_box_name_t bn);

extern snet_box_name_t
SNetMetaDataBoxNameXCreate(snet_box_name_t bn);

extern void
SNetMetaDataBoxNameDestroy(snet_box_name_t bn);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Helper function(s) used/needed by other inline functions in this file */

static inline bool
SNetMetaDataBoolToStdBool(snet_metadata_bool_t val)
{
    return (bool)(val);
}

/*----------------------------------------------------------------------------*/
/**
 * Helper functions to be used internally to get the "type" of 
 * metadata and do down-casts. These are needed because the runtime
 * always holds unmasked pointers to objects (where the "SNetMetaDataGetType()"
 * and "SNetMetaDataCastToSpecialized()" functions being "public" use masked
 * pointers).
 */

static inline 
snet_metadata_type_t 
SNetMetaDataGetTypeIntern(const snet_metadata_t *meta)
{
    return SNetMetaDataGetType(
        (const snet_metadata_t *)(SNetMaskPointer(meta)));
}

static inline void*
SNetMetaDataCastToSpecializedIntern(const snet_metadata_t *meta)
{
    return SNetMetaDataCastToSpecialized(
        (const snet_metadata_t *)(SNetMaskPointer(meta)));
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Cast functions to specialized types */

static inline
const snet_net_metadata_t*
SNetMetaDataToNetMetaData(const snet_metadata_t *meta)
{
    return (const snet_net_metadata_t *)(
        SNetMetaDataCastToSpecializedIntern(meta));
}

static inline
const snet_box_metadata_t*
SNetMetaDataToBoxMetaData(const snet_metadata_t *meta)
{
    return (const snet_box_metadata_t *)(
        SNetMetaDataCastToSpecializedIntern(meta));
}

static inline
const snet_global_metadata_t*
SNetMetaDataToGlobalMetaData(const snet_metadata_t *meta)
{
    return (const snet_global_metadata_t *)(
        SNetMetaDataCastToSpecializedIntern(meta));
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Accessor functions for the specialized metadata types */

static inline 
snet_box_serviceid_t
SNetMetaDataBoxGetServiceId(const snet_metadata_t *meta)
{
    assert(meta != NULL);
    assert(SNetMetaDataGetTypeIntern(meta) == SNET_METADATA_TYPE_BOX);

    return SNetMetaDataToBoxMetaData(meta)->service_id;
}

static inline bool
SNetMetaDataBoxGetExecLocalAlwaysFlag(const snet_metadata_t *meta)
{
    assert(meta != NULL);
    assert(SNetMetaDataGetTypeIntern(meta) == SNET_METADATA_TYPE_BOX);

    return SNetMetaDataBoolToStdBool(
        SNetMetaDataToBoxMetaData(meta)->exec_local_always);
}

static inline bool
SNetMetaDataBoxGetExecLocalIfNoResourceFlag(const snet_metadata_t *meta)
{
    assert(meta != NULL);
    assert(SNetMetaDataGetTypeIntern(meta) == SNET_METADATA_TYPE_BOX);

    return SNetMetaDataBoolToStdBool(
        SNetMetaDataToBoxMetaData(meta)->exec_local_ifnoresource);
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Cast function from box names and service ids to string. It is required 
 * because currently the resource manager (and the SEP) uses strings as 
 * service ids.
 */

static inline const char*
SNetMetaDataBoxNameToString(snet_box_name_t bn)
{
    // A simple cast is 
    // only what is needed!
    return (const char *)(bn);
}

static inline const char*
SNetMetaDataBoxServiceIdToString(snet_box_serviceid_t id)
{
    // Again a simple cast is 
    // only what is needed!
    return (const char *)(id);
}

#endif // __SVPSNETGWRT_GW_METADATA_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

