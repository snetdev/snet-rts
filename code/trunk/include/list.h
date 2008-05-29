/*
 * This provides a doubly linked list with void* context.
 */
#ifndef LIST_H

#include "bool.h"

typedef struct list snet_util_list_t;
extern snet_util_list_t *SNetUtilListCreate();
extern void SNetUtilListDestroy(snet_util_list_t *target);
extern void SNetUtilListAddAfter(snet_util_list_t *target, void* content);
extern void SNetUtilListAddBefore(snet_util_list_t *target, void* content);
extern void SNetUtilListAddBeginning(snet_util_list_t *target, void* content);
extern void SNetUtilListAddEnd(snet_util_list_t *target, void* content);
extern void SNetUtilListGotoBeginning(snet_util_list_t *target);
extern void SNetUtilListGotoEnd(snet_util_list_t *target);
extern void SNetUtilListNext(snet_util_list_t *target);
extern void SNetUtilListPrev(snet_util_list_t *target);
extern void* SNetUtilListGet(snet_util_list_t *target); 
extern bool SNetUtilListHasNext(snet_util_list_t *target); 
extern bool SNetUtilListHasPrev(snet_util_list_t *target);
extern void SNetUtilListDeleteCurrent(snet_util_list_t *target);

#endif
