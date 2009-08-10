/*----------------------------------------------------------------------------*/
/*
      -------------------------------------------------------------------

              * * * * ! SVP S-Net Graph Walker Runtime ! * * * *

                  Computer Systems Architecture (CSA) Group
                            Informatics Institute
                        University Of Amsterdam  2008
                         
      -------------------------------------------------------------------

    File Name      : list.utc.h

    File Type      : Header File

    ---------------------------------------

    File 
    Description    :

    Updates 
    Description    : N/A

*/
/*----------------------------------------------------------------------------*/

#ifndef __SVPSNETGWRT_CORE_LIST_H
#define __SVPSNETGWRT_CORE_LIST_H

#include "common.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
/**
 * Datatype of a list object
 */
typedef struct list snet_list_t;
typedef void*       snet_list_item_t;

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern void
SNetListInit(
    snet_list_t *lst, 
    unsigned int item_sz, 
    unsigned int bucket_sz); 

extern void
SNetListInitCopy(
    snet_list_t *lst,
    const snet_list_t *src_lst);

/*---*/

extern snet_list_t*
SNetListCreate(
    unsigned int item_sz, 
    unsigned int bucket_sz);

extern snet_list_t*
SNetListCreateCopy(const snet_list_t *src_lst);

/*---*/

extern void
SNetListDestroy(snet_list_t *lst);

/*----------------------------------------------------------------------------*/

extern bool
SNetListIsEmpty(const snet_list_t *lst);

extern unsigned int
SNetListGetSize(const snet_list_t *lst);

extern unsigned int
SNetListGetCapacity(const snet_list_t *lst);

/*----------------------------------------------------------------------------*/

extern unsigned int
SNetListPushBack(
    snet_list_t *lst, const void *item_val);

extern unsigned int
SNetListPushFront(
    snet_list_t *lst, const void *item_val);

/*---*/

extern unsigned int
SNetListInsertAfter(
    snet_list_t *lst, 
    unsigned int item_id, const void *item_val);

extern unsigned int
SNetListInsertBefore(
    snet_list_t *lst, 
    unsigned int item_id, const void *item_val);

/*----------------------------------------------------------------------------*/

extern bool
SNetListPopBack(snet_list_t *lst, void *item_val);

extern bool
SNetListPopFront(snet_list_t *lst, void *item_val);

/*---*/

extern void
SNetListRemove(snet_list_t *lst, unsigned int item_id);

extern void
SNetListRemoveByVal(snet_list_t *lst, const void *item_val);

/*----------------------------------------------------------------------------*/

extern unsigned int SNetListBegin(const snet_list_t *lst);
extern unsigned int SNetListEnd(const snet_list_t *lst);

/*---*/

extern unsigned int
SNetListItemGetNext(const snet_list_t *lst, unsigned int item_id);

extern unsigned int 
SNetListItemGetPrev(const snet_list_t *lst, unsigned int item_id);

/*---*/

extern bool 
SNetListItemIsLast(const snet_list_t *lst, unsigned int item_id);

extern bool 
SNetListItemIsFirst(const snet_list_t *lst, unsigned int item_id);

/*----------------------------------------------------------------------------*/

extern bool
SNetListItemGetValue(
    const snet_list_t *lst,
    unsigned int item_id, void *item_val);

extern void*
SNetListItemGetValueByRef(const snet_list_t *lst, unsigned int item_id);

/*----------------------------------------------------------------------------*/

extern unsigned int
SNetListFindItem(const snet_list_t *lst, const void *item_val);

/*---*/

extern bool
SNetListContainsItem(const snet_list_t *lst, const void *item_val);

extern bool
SNetListContainsItemWithId(const snet_list_t *lst, unsigned int item_id);

#endif // __SVPSNETGWRT_CORE_LIST_H

/*------------------------------- END OF FILE --------------------------------*/
/*----------------------------------------------------------------------------*/

