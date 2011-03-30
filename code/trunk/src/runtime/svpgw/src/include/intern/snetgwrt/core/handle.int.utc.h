/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : handle.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_HANDLE_INT_H
#define __SVPSNETGWRT_HANDLE_INT_H

#include "core/handle.utc.h"

/*---*/

#include "common.int.utc.h"
#include "basetype.int.utc.h"
#include "record.int.utc.h"
#include "typeencode.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Types of functions for the
 * handle's VTable.
 */

typedef void* (*snet_hnd_cast_fptr_t)(const snet_handle_t *);

/*---*/

typedef snet_record_t*
    (*snet_hnd_recget_fptr_t)(const void *);

typedef const snet_box_sign_t*
    (*snet_hnd_boxsignget_fptr_t)(const void *);

/*---*/

typedef snet_place_t (*snet_hnd_snetdhostplcget_fptr_t)(const void *);

/*----------------------------------------------------------------------------*/
/**
 * The "handle" data type which will be the "base" for specific
 * ones. For this reason its declaration must be here since in
 * order to be "inherited" the complete type is required.
 */
struct handle {
    snet_base_t    base;
    snet_record_t *out_rec;

    struct {
         snet_hnd_cast_fptr_t            cast_fun;
         snet_hnd_recget_fptr_t          recget_fun;
         snet_hnd_boxsignget_fptr_t      boxsignget_fun;
         snet_hnd_snetdhostplcget_fptr_t snetdhostplcget_fun;

    } vtbl;

}; // struct handle (snet_handle_t)

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetHndInit(
    snet_handle_t                   *hnd,
    snet_hnd_cast_fptr_t             cast_fun,
    snet_hnd_recget_fptr_t           recget_fun,
    snet_hnd_boxsignget_fptr_t       boxsignget_fun,
    snet_hnd_snetdhostplcget_fptr_t  snetdhostplcget_fun);


/*---*/

extern void SNetHndDispose(snet_handle_t *hnd);

/*----------------------------------------------------------------------------*/

extern snet_record_t*
SNetHndGetOutRecord(const snet_handle_t *hnd);

extern void 
SNetHndSetOutRecord(snet_handle_t *hnd, snet_record_t *rec);

/*----------------------------------------------------------------------------*/

extern void*
SNetHndCastToDerivedHandle(const snet_handle_t *hnd);

/*---*/

extern const snet_box_sign_t*
SNetHndGetBoxSignature(const snet_handle_t *hnd);

extern snet_place_t
SNetHndGetNetDomainHostPlace(const snet_handle_t *hnd);

/*----------------------------------------------------------------------------*/

extern 
snet_variantencoding_t*
SNetHndCreateVEncForOutRecord(
    const snet_handle_t *hnd, unsigned int variant_idx);

/*----------------------------------------------------------------------------*/

extern snet_base_t*
SNetHndToBase(snet_handle_t *hnd); 

extern const snet_base_t*
SNetHndToBaseConst(const snet_handle_t *hnd); 

#endif // __SVPSNETGWRT_HANDLE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

