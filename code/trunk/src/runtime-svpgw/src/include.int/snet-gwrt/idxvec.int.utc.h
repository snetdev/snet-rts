/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : idxvec.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_IDXVEC_INT_H
#define __SVPSNETGWRT_IDXVEC_INT_H

$ifndef __STDC_FORMAT_MACROS
$define __STDC_FORMAT_MACROS
$endif

/*---*/

#include "common.int.utc.h"
#include "basetype.int.utc.h"

/*---*/

$include <inttypes.h>

/*---*/

// Format specifier for
// elements of an index vector for
// "printf()" and similar functions.
#define PRISNETIDXVECITEM PRId64

/*----------------------------------------------------------------------------*/
/**
 * Datatype for an index vector
 */
typedef struct index_vector snet_idxvec_t;

/**
 * Type of each individual
 * item in an index vector
 */
typedef uint64_t snet_idxvec_item_t;

/**
 * Result of a comparison between 2
 * index vectors.
 */
typedef enum {
    IDXVEC_CMP_RES_EQUAL,
    IDXVEC_CMP_RES_LESS,
    IDXVEC_CMP_RES_GREATER,
    IDXVEC_CMP_RES_UNDEFINED

} snet_idxvec_cmp_result_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Initialization, Creation and Destruction */

extern void
SNetIdxVecInit(snet_idxvec_t *idxv);

extern void
SNetIdxVecInitCopy(
    snet_idxvec_t *idxv, bool clone);

extern void
SNetIdxVecInitFromStr(
    snet_idxvec_t *idxv, const char *str);

extern void
SNetIdxVecInitFromArray(
    snet_idxvec_t *idxv,
    const snet_idxvec_item_t *a, unsigned int aSize);

/*---*/

extern snet_idxvec_t* SNetIdxVecCreate();

extern snet_idxvec_t*
SNetIdxVecCreateCopy(const snet_idxvec_t *val, bool clone);

extern snet_idxvec_t*
SNetIdxVecCreateFromStr(const char *str);

extern snet_idxvec_t*
SNetIdxVecCreateFromArray(const snet_idxvec_item_t *arr, unsigned int arr_sz);

/*---*/

extern void SNetIdxVecDestroy(snet_idxvec_t *idxv);

/*----------------------------------------------------------------------------*/
/* Value changing */

extern void SNetIdxVecSetToInfinite(snet_idxvec_t *idxv);

/*---*/

extern void 
SNetIdxVecSetValue(
    snet_idxvec_t *idxv1, 
    const snet_idxvec_t *idxv2);

extern void 
SNetIdxVecSetValueFromStr(
    snet_idxvec_t *idxv, const char *str);

extern void 
SNetIdxVecSetValueFromArray(
    snet_idxvec_t *idxv,
    const snet_idxvec_item_t *arr, unsigned int arr_sz);

/*----------------------------------------------------------------------------*/
/* Item manipulation */

extern snet_idxvec_item_t 
SNetIdxVecGetItemValue(const snet_idxvec_t *idxv, unsigned int pos);

/*---*/

extern void 
SNetIdxVecSetItemValue(
    snet_idxvec_t *idxv,
    unsigned int pos, snet_idxvec_item_t val);

extern void 
SNetIdxVecIncrItemValue(
    snet_idxvec_t *idxv,
    unsigned int pos, snet_idxvec_item_t val);

extern void 
SNetIdxVecDecrItemValue(
    snet_idxvec_t *idxv,
    unsigned int pos, snet_idxvec_item_t val);

/*----------------------------------------------------------------------------*/
/* Properties retrieval */

extern bool SNetIdxVecIsInfinite(const snet_idxvec_t *idxv);

/*---*/

extern unsigned int SNetIdxVecGetLen(const snet_idxvec_t *idxv);
extern unsigned int SNetIdxVecGetBufSize(const snet_idxvec_t *idxv);

/*----------------------------------------------------------------------------*/
/* Comparison */

extern snet_idxvec_cmp_result_t
SNetIdxVecCompare(const snet_idxvec_t *idxv1, const snet_idxvec_t *idxv2);

/*---*/

extern snet_idxvec_cmp_result_t
SNetIdxVecCompareEx(const snet_idxvec_t *idxv1, const snet_idxvec_t *idxv2);

/*----------------------------------------------------------------------------*/
/* Infimum calculation */

extern snet_idxvec_t*
SNetIdxVecCalcInfimum(
    const snet_idxvec_t *idxv1, const snet_idxvec_t *idxv2);

/*---*/

extern snet_idxvec_t*
SNetIdxVecCalcInfimumEx(
    const snet_idxvec_t *idxv1, const snet_idxvec_t *idxv2);

/*----------------------------------------------------------------------------*/
/* Adding (pushing) and removing (popping) elements */

extern void 
SNetIdxVecPush(
    snet_idxvec_t *idxv1,
    const snet_idxvec_t *idxv2, 
    unsigned int start_item_idx);

extern void 
SNetIdxVecPushStr(snet_idxvec_t *idxv, const char *str);

extern void 
SNetIdxVecPushArray(
    snet_idxvec_t *idxv,
    const snet_idxvec_item_t *arr, unsigned int arr_sz);

extern void
SNetIdxVecPopLeft(snet_idxvec_t *idxv, unsigned int count);

extern void
SNetIdxVecPopRight(snet_idxvec_t *idxv, unsigned int count);

/*----------------------------------------------------------------------------*/
/* Convertion */

extern snet_idxvec_item_t* 
SNetIdxVecToArray(
    const snet_idxvec_t *idxv, snet_idxvec_item_t *arr);

/*---*/

extern const char* 
SNetIdxVecToString(const snet_idxvec_t *idxv, char *str);

#endif // __SVPSNETGWRT_IDXVEC_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

