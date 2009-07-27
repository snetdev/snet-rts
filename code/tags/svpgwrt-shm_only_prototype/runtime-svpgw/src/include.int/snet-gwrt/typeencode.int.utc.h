/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : typeencode.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_TYPEENCODE_INT_H
#define __SVPSNETGWRT_TYPEENCODE_INT_H

#include "typeencode.utc.h"

/*---*/

#include "common.int.utc.h"
#include "basetype.int.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Vector functions */

extern void
SNetTencVectorInit(
    snet_vector_t *vec, unsigned int sz);

extern void
SNetTencVectorInitCopy(
    snet_vector_t *vec, const snet_vector_t *src_vec);

/*---*/

extern snet_vector_t*
SNetTencVectorCreateEmpty(unsigned int sz);

extern snet_vector_t*
SNetTencVectorCreateCopy(const snet_vector_t *vec);

/*---*/

extern int 
SNetTencVectorGetEntry(const snet_vector_t *vec, unsigned int idx); 

extern void
SNetTencVectorSetEntry(snet_vector_t *vec, unsigned int idx, int val);

/*---*/

extern unsigned int
SNetTencVectorGetSize(const snet_vector_t *vec);

/*---*/

extern void
SNetTencVectorRemoveUnsetEntries(snet_vector_t *vec);

extern void
SNetTencVectorResize(snet_vector_t *vec, unsigned int sz);

/*---*/

extern bool 
SNetTencVectorContainsItem(
    const snet_vector_t *vec, int item_val, unsigned int *item_idx);

extern unsigned int
SNetTencVectorCompare(const snet_vector_t *vec1, const snet_vector_t *vec2);

/*----------------------------------------------------------------------------*/
/* Variant encoding functions */

extern void
SNetTencVariantInit(
    snet_variantencoding_t *venc,
    snet_vector_t *fields, snet_vector_t *tags, snet_vector_t *btags);

extern void
SNetTencVariantInitCopy(
    snet_variantencoding_t *venc, const snet_variantencoding_t *src_venc);

/*---*/

extern const snet_vector_t* 
SNetTencVariantGetFields(const snet_variantencoding_t *venc);

extern const snet_vector_t* 
SNetTencVariantGetTags(const snet_variantencoding_t *venc);

extern const snet_vector_t* 
SNetTencVariantGetBTags(const snet_variantencoding_t *venc);

/*---*/

extern int 
SNetTencVariantsMatch(
    const snet_variantencoding_t *venc1, 
    const snet_variantencoding_t *venc2, bool *matched_ident);

extern int
SNetTencVariantMatchesType(
    const snet_variantencoding_t *venc, 
    const snet_typeencoding_t    *tenc,
    unsigned int *matched_venc_idx, bool *matched_ident);

/*----------------------------------------------------------------------------*/
/* Type encoding functions */

extern void
SNetTencTypeInit(
    snet_typeencoding_t *tenc, unsigned int variants_cnt);

extern void
SNetTencTypeInitCopy(
    snet_typeencoding_t *tenc, snet_typeencoding_t *src_tenc);

/*----------------------------------------------------------------------------*/
/* Box signature encoding functions */

extern void
SNetTencBoxSignInit(snet_box_sign_t *sign, snet_typeencoding_t *tenc);

extern void
SNetTencBoxSignInitCopy(snet_box_sign_t *sign, snet_box_sign_t *src_sign);

/*---*/

extern snet_box_sign_t*
SNetTencBoxSignCreateCopy(snet_box_sign_t *src_sign);

/*---*/

extern snet_typeencoding_t*
SNetTencBoxSignGetType(const snet_box_sign_t *sign);

extern snet_vector_t*
SNetTencBoxSignGetMapping(const snet_box_sign_t *sign, unsigned int idx);

#endif // __SVPSNETGWRT_TYPEENCODE_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

