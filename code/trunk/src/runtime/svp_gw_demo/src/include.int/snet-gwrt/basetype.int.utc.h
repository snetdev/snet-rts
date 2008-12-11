/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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

/**
 * Forward declaration to avoid
 * inclusion of the "domain.utc.h" file"
 */
typedef struct domain snet_domain_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Base type "inherited" by all "primary" datatypes
 * within the runtime system.
 */
typedef struct {
    snet_domain_t *domain;

    unsigned int   plc_flags;

    struct {
        place owner;
        place mutex;
        place spec_mod;

    } places;

} snet_base_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetBaseTypeInit(snet_base_t *var, const snet_domain_t *domain);

/*---*/

extern void 
SNetBaseTypeSetPlaces(
    snet_base_t *var,
    const place *owner, 
    const place *mutex,
    const place *spec_mod);

/*---*/

extern bool
SNetBaseTypeSameDomain(const snet_base_t *var1, const snet_base_t *var2);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

static inline const snet_domain_t*
SNetBaseTypeGetDomain(const snet_base_t *var)
{
    assert(var != NULL); return var->domain;
}

static inline unsigned int
SNetBaseTypeGetPlcFlags(const snet_base_t *var)
{
    assert(var != NULL); return var->plc_flags;
}

/*----------------------------------------------------------------------------*/

static inline place
SNetBaseTypeGetOwnerPlace(const snet_base_t *var)
{
    assert(var != NULL); return var->places.owner;
}

static inline place
SNetBaseTypeGetSpecModPlace(const snet_base_t *var)
{
    assert(var != NULL); return var->places.spec_mod;
}

static inline place
SNetBaseTypeGetMutexPlace(const snet_base_t *var)
{
    assert(var != NULL); return var->places.mutex;
}

#endif // __SVPSNETGWRT_BASETYPE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

