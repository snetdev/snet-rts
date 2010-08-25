/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : basetype.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_BASETYPE_INT_H
#define __SVPSNETGWRT_BASETYPE_INT_H

#include "common.int.utc.h"
#include "plcmng.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Base type "inherited" by all "primary" datatypes
 * within the runtime system.
 */
typedef struct {
    struct {
        snet_place_t owner;
        snet_place_t mutex;
        snet_place_t ddefault;

    } places;
} snet_base_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetBaseTypeInit(snet_base_t *var);

extern void
SNetBaseTypeDispose(snet_base_t *var);

/*---*/

extern void 
SNetBaseTypeSetPlaces(
    snet_base_t *var,
    const snet_place_t owner, 
    const snet_place_t mutex,
    const snet_place_t ddefault);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static inline void
SNetBaseTypeSetPlacesDefault(snet_base_t *var)
{
    SNetBaseTypeSetPlaces(
        var,
        SNetPlaceGetMine(),
        SNetPlaceGetNull(), SNetPlaceGetNull());
}

/*----------------------------------------------------------------------------*/

extern inline bool 
SNetBaseTypeHasOwnerPlace(const snet_base_t *var)
{
    assert(var != NULL);
    return SNetPlaceIsNull(var->places.owner);
}

extern inline bool 
SNetBaseTypeHasMutexPlace(const snet_base_t *var)
{
    assert(var != NULL);
    return SNetPlaceIsNull(var->places.mutex);
}

extern inline bool 
SNetBaseTypeHasDefaultPlace(const snet_base_t *var)
{
    assert(var != NULL);
    return SNetPlaceIsNull(var->places.ddefault);
}

/*----------------------------------------------------------------------------*/

static inline snet_place_t
SNetBaseTypeGetOwnerPlace(const snet_base_t *var)
{
    assert(var != NULL); return var->places.owner;
}

static inline snet_place_t
SNetBaseTypeGetMutexPlace(const snet_base_t *var)
{
    assert(var != NULL); return var->places.mutex;
}

static inline snet_place_t
SNetBaseTypeGetDefaultPlace(const snet_base_t *var)
{
    assert(var != NULL); return var->places.ddefault;
}

#endif // __SVPSNETGWRT_BASETYPE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

