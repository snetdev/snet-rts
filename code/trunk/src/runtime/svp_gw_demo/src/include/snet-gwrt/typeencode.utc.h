/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

           * * * * ! SVP S-Net Graph Walker Runtime (demo) ! * * * *

                   Computer Systems Architecture (CSA) Group
                             Informatics Institute
                         University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : typeencode.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_TYPEENCODE_H
#define __SVPSNETGWRT_TYPEENCODE_H

#include "common.utc.h"
#include "list.utc.h"

/*---*/

#define TENC_VEC_UNSET_ITEM_VAL    -1
#define TENC_VEC_CONSUMED_ITEM_VAL -2

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*/

typedef struct vector           snet_vector_t;
typedef struct variant_encoding snet_variantencoding_t;
typedef struct type_encoding    snet_typeencoding_t;

typedef snet_list_t             snet_typeencoding_list_t;

/*---*/

typedef struct box_sign         snet_box_sign_t;

/*----------------------------------------------------------------------------*/

typedef enum {
    VARIANT_ITEM_TYPE_FIELD,
    VARIANT_ITEM_TYPE_TAG,
    VARIANT_ITEM_TYPE_BTAG

} snet_variant_item_type_t;  

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Vector functions */

extern snet_vector_t*
SNetTencVectorCreate(unsigned int sz, ...);

extern void SNetTencVectorDestroy(snet_vector_t *vec);

/*----------------------------------------------------------------------------*/
/* Variant encoding functions */

extern snet_variantencoding_t*
SNetTencVariantEncode(
    snet_vector_t *fields,
    snet_vector_t *tags,
    snet_vector_t *btags);

extern snet_variantencoding_t*
SNetTencVariantCreateCopy(
    const snet_variantencoding_t *src_venc);

extern void
SNetTencVariantDestroy(snet_variantencoding_t *venc);

/*---*/

extern unsigned int
SNetTencVariantGetFieldsCount(
    const snet_variantencoding_t *venc);

extern unsigned int
SNetTencVariantGetTagsCount(
    const snet_variantencoding_t *venc);

extern unsigned int
SNetTencVariantGetBTagsCount(
    const snet_variantencoding_t *venc);

/*---*/

extern int
SNetTencVariantGetField(
    const snet_variantencoding_t *venc, unsigned int inx);

extern int
SNetTencVariantGetTag(
    const snet_variantencoding_t *venc, unsigned int inx);

extern int
SNetTencVariantGetBTag(
    const snet_variantencoding_t *venc, unsigned int inx);

/*---*/

extern bool
SNetTencVariantContainsField(
    const snet_variantencoding_t *venc, int name, unsigned int *inx);

extern bool
SNetTencVariantContainsTag(
    const snet_variantencoding_t *venc, int name, unsigned int *inx);

extern bool
SNetTencVariantContainsBTag(
    const snet_variantencoding_t *venc, int name, unsigned int *inx);

/*---*/

extern void 
SNetTencVariantRenameField(
    snet_variantencoding_t *venc, int name, int new_name);

extern void
SNetTencVariantRenameTag(
    snet_variantencoding_t *venc, int name, int new_name);

extern void 
SNetTencVariantRenameBTag(
    snet_variantencoding_t *venc, int name, int new_name);

/*---*/

extern bool
SNetTencVariantAddField(
    snet_variantencoding_t *venc, int name);

extern bool
SNetTencVariantAddTag(
    snet_variantencoding_t *venc, int name);

extern bool
SNetTencVariantAddBTag(
    snet_variantencoding_t *venc, int name);

/*---*/

extern void 
SNetTencVariantRemoveField(
    snet_variantencoding_t *venc, int name);

extern void 
SNetTencVariantRemoveTag(
    snet_variantencoding_t *venc, int name);

extern void
SNetTencVariantRemoveBTag(
    snet_variantencoding_t *venc, int name);

/*---*/

extern bool 
SNetTencVariantRemoveFieldEx(
    snet_variantencoding_t *venc,
    int name, unsigned int *inx, bool mark_only);

extern bool 
SNetTencVariantRemoveTagEx(
    snet_variantencoding_t *venc,
    int name, unsigned *inx, bool mark_only);

extern bool
SNetTencVariantRemoveBTagEx(
    snet_variantencoding_t *venc,
    int name, unsigned int *inx, bool mark_only);

/*---*/

extern void 
SNetTencVariantRemoveMarked(snet_variantencoding_t *venc);

extern void 
SNetTencVariantRemoveMarkedFields(snet_variantencoding_t *venc);

extern void 
SNetTencVariantRemoveMarkedTags(snet_variantencoding_t *venc);

extern void 
SNetTencVariantRemoveMarkedBTags(snet_variantencoding_t *venc);

/*----------------------------------------------------------------------------*/
/* Type encoding functions */

extern snet_typeencoding_t*
SNetTencTypeEncode(
    unsigned int variants_cnt, ...);

extern snet_typeencoding_t*
SNetTencTypeCreateCopy(
    const snet_typeencoding_t *src_tenc);

extern void
SNetTencTypeDestroy(snet_typeencoding_t *tenc);

/*---*/

extern unsigned int
SNetTencTypeGetVariantsCount(const snet_typeencoding_t *tenc);

extern snet_variantencoding_t*
SNetTencTypeGetVariant(const snet_typeencoding_t *tenc, unsigned int inx);

/*---*/

extern snet_typeencoding_list_t*
SNetTencCreateTypeEncodingList(unsigned int sz, ...);

extern void
SNetTencTypeEncodingListDestroy(snet_typeencoding_list_t *tenc_lst);

/*----------------------------------------------------------------------------*/
/* Box signature encoding functions */

extern snet_box_sign_t*
SNetTencBoxSignEncode(snet_typeencoding_t *tenc, ...);

extern void SNetTencBoxSignDestroy(snet_box_sign_t *sign);

#endif // __SVPSNETGWRT_TYPEENCODE_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

