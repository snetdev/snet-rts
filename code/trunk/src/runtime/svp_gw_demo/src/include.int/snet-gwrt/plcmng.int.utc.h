/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

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

/*----------------------------------------------------------------------------*/

typedef enum place_type {
    PLACE_TYPE_GENERIC,
    PLACE_TYPE_CONDVAR,
    PLACE_TYPE_FUNC_SPEC,
    PLACE_TYPE_SNET_RT_SPEC

} snet_place_type_t;

typedef struct place_specs {
    bool               expires;
    bool               mutex;
    snet_place_type_t  type;
    
    void              *type_spec_data;
    unsigned int       type_spec_data_sz;

} snet_place_specs_t;

/*---*/

typedef struct place_contract {
    bool                expires;
    place               plc;
    snet_place_specs_t *plc_specs;
    
    void               *data;
    unsigned int        data_sz;
    
} snet_place_contract_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void SNetPlcMngSubSystemInit();
extern void SNetPlcMngSubSystemDestroy();

/*----------------------------------------------------------------------------*/

extern place SNetGetMyPlace();

/*----------------------------------------------------------------------------*/

extern snet_place_contract_t*
SNetPlaceAlloc(const snet_place_specs_t *specs);

extern snet_place_contract_t*
SNetTryPlaceAlloc(const snet_place_specs_t *specs);

/*---*/

extern void SNetPlaceFree(snet_place_contract_t *contract);

/*----------------------------------------------------------------------------*/

extern bool 
SNetPlaceRenewContract(snet_place_contract_t *contract);

/*---*/

extern bool
SNetPlaceValidateContract(const snet_place_contract_t *contract);

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Some helper inlines */

static inline place
SNetPlaceGetFromContract(const snet_place_contract_t *contract)
{
    assert(contract != NULL); return contract->plc;
}

static inline snet_place_specs_t*
SNetPlaceGetSpecsFromContract(const snet_place_contract_t *contract)
{
    assert(contract != NULL);
    return (snet_place_specs_t *) contract->plc_specs;
}

#endif // __SVPSNETGWRT_PLCMNG_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

