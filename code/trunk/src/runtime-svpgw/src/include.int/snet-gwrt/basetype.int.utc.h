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

/**
 * Forward declaration to avoid
 * inclusion of the "domain.utc.h" file"
 */
typedef struct domain snet_domain_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef union {
    place                  val;
    snet_place_contract_t *contr;

} snet_base_type_place_t;

/**
 * Base type "inherited" by all "primary" datatypes
 * within the runtime system.
 */
typedef struct {
    snet_domain_t *domain;

    unsigned int plc_flags;

    struct {
        snet_base_type_place_t owner;
        snet_base_type_place_t mutex;
        snet_base_type_place_t spec_mod;

    } places;

} snet_base_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetBaseTypeInit(
    snet_base_t *var,
    const snet_domain_t *domain);

extern void
SNetBaseTypeDestroy(snet_base_t *var);

/*---*/

extern void 
SNetBaseTypeSetPlaces(
    snet_base_t *var,
    const place *owner, 
    const place *mutex,
    const place *spec_mod);

extern void
SNetBaseTypeSetPlacesContracts(
    snet_base_t *var,
    snet_place_contract_t *owner,
    snet_place_contract_t *mutex,
    snet_place_contract_t *spec_mod);

/*----------------------------------------------------------------------------*/

extern bool
SNetBaseTypeSameDomain(const snet_base_t *var1, const snet_base_t *var2);

/*---*/

extern bool 
SNetBaseTypeHasOwnerPlace(const snet_base_t *var);

extern bool 
SNetBaseTypeHasMutexPlace(const snet_base_t *var);

extern bool 
SNetBaseTypeHasSpecModPlace(const snet_base_t *var);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static inline snet_domain_t*
SNetBaseTypeGetDomain(const snet_base_t *var)
{
    assert(var != NULL); return var->domain;
}

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static inline place
SNetBaseTypeGetOwnerPlace(const snet_base_t *var)
{
    assert(var != NULL);
    
    return ((var->plc_flags & 0x02) == 0 ?
        var->places.owner.val : 
        SNetPlaceGetFromContract(var->places.owner.contr));
}

static inline place
SNetBaseTypeGetMutexPlace(const snet_base_t *var)
{
    assert(var != NULL);

    return ((var->plc_flags & 0x08) == 0 ?
        var->places.mutex.val : 
        SNetPlaceGetFromContract(var->places.mutex.contr));
}

static inline place
SNetBaseTypeGetSpecModPlace(const snet_base_t *var)
{
    assert(var != NULL);

    return ((var->plc_flags & 0x20) == 0 ?
        var->places.spec_mod.val : 
        SNetPlaceGetFromContract(var->places.spec_mod.contr));
}

#endif // __SVPSNETGWRT_BASETYPE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

