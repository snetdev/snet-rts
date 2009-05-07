/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : graphindex.int.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_GRAPHINDEX_INT_H
#define __SVPSNETGWRT_GRAPHINDEX_INT_H

#include "common.int.utc.h"
#include "basetype.int.utc.h"

/*----------------------------------------------------------------------------*/
/**
 * Datatype for an index
 */
typedef struct graph_index snet_ginx_t;

/**
 * Type of each individual
 * item in a graph index
 */
typedef long long snet_ginx_item_t;

/**
 * Result of a comparison between 2
 * graph indexes.
 */
typedef enum {
    GRAPH_INX_CMP_RES_EQUAL,
    GRAPH_INX_CMP_RES_LESS,
    GRAPH_INX_CMP_RES_GREATER,
    GRAPH_INX_CMP_RES_UNDEFINED

} snet_ginx_cmp_result_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/* Initialization, Creation and Destruction */

extern void
SNetGInxInit(snet_ginx_t *inx);

extern void
SNetGInxInitCopy(snet_ginx_t *inx, bool clone);

extern void
SNetGInxInitFromStr(snet_ginx_t *inx, const char *val);

extern void
SNetGInxInitFromArray(
    snet_ginx_t *inx, const snet_ginx_item_t *a, unsigned int aSize);

/*---*/

extern snet_ginx_t* SNetGInxCreate();

extern snet_ginx_t*
SNetGInxCreateCopy(const snet_ginx_t *val, bool clone);

extern snet_ginx_t*
SNetGInxCreateFromStr(const char *val);

extern snet_ginx_t*
SNetGInxCreateFromArray(const snet_ginx_item_t *arr, unsigned int arr_sz);

/*---*/

extern void SNetGInxDestroy(snet_ginx_t *inx);

/*----------------------------------------------------------------------------*/
/* Value changing */

extern void SNetGInxSetToInfinite(snet_ginx_t *inx);

/*---*/

extern void 
SNetGInxSetValue(
    snet_ginx_t *inx1, 
    const snet_ginx_t *inx2, bool clone);

extern void 
SNetGInxSetValueFromStr(
    snet_ginx_t *inx, const char *val);

extern void 
SNetGInxSetValueFromArray(
    snet_ginx_t *inx,
    const snet_ginx_item_t *arr, unsigned int arr_sz);

/*----------------------------------------------------------------------------*/
/* Item manipulation */

extern snet_ginx_item_t 
SNetGInxGetItemValue(const snet_ginx_t *inx, unsigned int pos);

/*---*/

extern void 
SNetGInxSetItemValue(
    snet_ginx_t *inx,
    unsigned int pos, snet_ginx_item_t val);

extern void 
SNetGInxIncrItemValue(
    snet_ginx_t *inx,
    unsigned int pos, snet_ginx_item_t val);

extern void 
SNetGInxDecrItemValue(
    snet_ginx_t *inx,
    unsigned int pos, snet_ginx_item_t val);

/*----------------------------------------------------------------------------*/
/* Properties retrieval */

extern bool SNetGInxIsInfinite(const snet_ginx_t *inx);

/*---*/

extern unsigned int SNetGInxGetLen(const snet_ginx_t *inx);
extern unsigned int SNetGInxGetBufSize(const snet_ginx_t *inx);

/*----------------------------------------------------------------------------*/
/* Comparison */

extern snet_ginx_cmp_result_t
SNetGInxCompare(const snet_ginx_t *inx1, const snet_ginx_t *inx2);

/*---*/

extern snet_ginx_cmp_result_t
SNetGInxCompareEx(const snet_ginx_t *inx1, const snet_ginx_t *inx2);

/*----------------------------------------------------------------------------*/
/* Infimum calculation */

extern snet_ginx_t*
SNetGInxCalcInfimum(const snet_ginx_t *inx1, const snet_ginx_t *inx2);

/*---*/

extern snet_ginx_t*
SNetGInxCalcInfimumEx(const snet_ginx_t *inx1, const snet_ginx_t *inx2);

/*----------------------------------------------------------------------------*/
/* Concatenation */

extern void 
SNetGInxConcat(
    snet_ginx_t *inx1,
    const snet_ginx_t *inx2, 
    unsigned int start_item_inx);

extern void 
SNetGInxConcatStr(snet_ginx_t *inx, const char *str);

extern void 
SNetGInxConcatArray(
    snet_ginx_t *inx,
    const snet_ginx_item_t *arr, unsigned int arr_sz);

/*----------------------------------------------------------------------------*/
/* Editting */

extern void
SNetGInxChopLeft(snet_ginx_t *inx, unsigned int count);

extern void
SNetGInxChopRight(snet_ginx_t *inx, unsigned int count);

/*----------------------------------------------------------------------------*/
/* Convertion */

extern snet_ginx_item_t* 
SNetGInxToArray(
    const snet_ginx_t *inx, snet_ginx_item_t *arr);

/*---*/

extern const char* 
SNetGInxToString(const snet_ginx_t *inx, char *str);

#endif // __SVPSNETGWRT_GRAPHINDEX_INT_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

