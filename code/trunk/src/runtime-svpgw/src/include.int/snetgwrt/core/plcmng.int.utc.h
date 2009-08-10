/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : plcmng.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_PLCMNG_INT_H
#define __SVPSNETGWRT_PLCMNG_INT_H

#include "common.int.utc.h"
#include "memmng.int.utc.h"

/*----------------------------------------------------------------------------*/

#ifdef SVPSNETGWRT_SVP_PLATFORM_UTCPTL
typedef snet_ref_t snet_place_t;

#else
#error \
Selected SVP platform not fully supported yet.
#endif

/*----------------------------------------------------------------------------*/

typedef enum place_type {
    PLACE_TYPE_GENERIC,
    PLACE_TYPE_FUNC_SPEC,
    PLACE_TYPE_SNET_RT_SPEC,

} snet_place_type_t;

typedef uint32_t snet_place_flags_t;

typedef void  (*snet_place_specsdata_free_fptr_t)(void *);
typedef void* (*snet_place_specsdata_copy_fptr_t)(void *);

/*---*/

typedef struct place_specs {
    bool               expires;
    bool               mutex;
    snet_place_type_t  type;
    snet_place_flags_t flags;
    
    void                             *type_spec_data;
    size_t                            type_spec_data_sz;
    snet_place_specsdata_free_fptr_t  type_spec_data_freefun;
    snet_place_specsdata_copy_fptr_t  type_spec_data_copyfun;

} snet_place_specs_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * The following set of specs is used quiet a lot 
 * throught the runtime so we'll make a macro to 
 * make things easier (and minimize the risk for bugs).
 */
#define SNET_DEFAULT_MUTEX_RTPLACE_SPECS_INITIALIZER { \
    false,                                             \
    true,                                              \
    PLACE_TYPE_SNET_RT_SPEC,                           \
    0,                                                 \
    NULL, 0, NULL, NULL                                \
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetPlaceMngSubSystemInit();

extern void
SNetPlaceMngSubSystemDestroy();

/*----------------------------------------------------------------------------*/

extern snet_place_t SNetPlaceGetMine();
extern snet_place_t SNetPlaceGetNull();

/*----------------------------------------------------------------------------*/

extern snet_place_t
SNetPlaceTrackUTCPlace(
    const place plc,
    const snet_place_specs_t *specs);

extern snet_place_t
SNetPlaceAlloc(const snet_place_specs_t *specs);

extern snet_place_t
SNetPlaceTryAlloc(const snet_place_specs_t *specs);

/*---*/

extern void 
SNetPlaceFree(snet_place_t plc);

extern snet_place_t
SNetPlaceCopy(const snet_place_t plc);

extern snet_place_t
SNetPlaceCopyAndFree(const snet_place_t plc);

/*----------------------------------------------------------------------------*/

extern 
const snet_place_specs_t*
SNetPlaceGetSpecs(const snet_place_t plc);

/*---*/

extern bool
SNetPlaceIsNull(const snet_place_t plc);

extern bool 
SNetPlaceIsOnSharedMemory(const snet_place_t plc);

extern bool 
SNetPlaceCompare(const snet_place_t plc1, const snet_place_t plc2);

/*----------------------------------------------------------------------------*/
/* Cast function to uTC place (to be used for "creates") */

extern place
SNetPlaceToUTCPlace(const snet_place_t plc);

#endif // __SVPSNETGWRT_PLCMNG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

