/*
 * This provides a doubly linked list with void* context.
 */
#ifndef LIST_H

#include "bool.h"

typedef struct list snet_util_list_t;
snet_util_list_t *SNetUtilListCreate();
void SNetUtilListDestroy(snet_util_list_t *target);

void SNetUtilListAddAfter(snet_util_list_t *target, void* content);
void SNetUtilListAddBefore(snet_util_list_t *target, void* content);
void SNetUtilListAddBeginning(snet_util_list_t *target, void* content);
void SNetUtilListAddEnd(snet_util_list_t *target, void* content);

void SNetUtilListSet(snet_util_list_t *target, void *new_content);
void SNetUtilListDelete(snet_util_list_t *target);

void SNetUtilListGotoBeginning(snet_util_list_t *target);
void SNetUtilListGotoEnd(snet_util_list_t *target);

void SNetUtilListNext(snet_util_list_t *target);
void SNetUtilListPrev(snet_util_list_t *target);

void* SNetUtilListGet(snet_util_list_t *target); 

bool SNetUtilListHasNext(snet_util_list_t *target); 
bool SNetUtilListHasPrev(snet_util_list_t *target);

void SNetUtilListRotateForward(snet_util_list_t *target);

bool SNetUtilListIsEmpty(snet_util_list_t *target);
bool SNetUtilListCurrentDefined(snet_util_list_t *target);
int SNetUtilListCount(snet_util_list_t *target);
#endif
