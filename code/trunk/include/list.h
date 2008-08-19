/*
 * This provides a doubly linked list with void* context.
 */
#ifndef LIST_H
#define LIST_H

#include "bool.h"

typedef struct list snet_util_list_t;

void SNetUtilListDump(snet_util_list_t *target);
snet_util_list_t *SNetUtilListCreate();
void SNetUtilListDestroy(snet_util_list_t *target);

snet_util_list_t *SNetUtilListDeleteFirst(snet_util_list_t *target);
void *SNetUtilListGetFirst(snet_util_list_t *target);

snet_util_list_t *
SNetUtilListAddBeginning(snet_util_list_t *target, void* content);
snet_util_list_t *SNetUtilListAddEnd(snet_util_list_t *target, void* content);

bool SNetUtilListIsEmpty(snet_util_list_t *target);
int SNetUtilListCount(snet_util_list_t *target);

snet_util_list_t *SNetUtilListClone(snet_util_list_t *target);

typedef struct list_iterator snet_util_list_iter_t;
snet_util_list_t *SNetUtilListIterSet(snet_util_list_iter_t *target, void *new_content);
snet_util_list_t *SNetUtilListIterDelete(snet_util_list_iter_t *target);
snet_util_list_iter_t *SNetUtilListFirst(snet_util_list_t *target);
snet_util_list_iter_t *SNetUtilListLast(snet_util_list_t *target);
snet_util_list_t *SNetUtilListIterAddAfter(snet_util_list_iter_t *target, void* content);
snet_util_list_t * SNetUtilListIterAddBefore(snet_util_list_iter_t *target, void* content);
snet_util_list_iter_t *SNetUtilListIterNext(snet_util_list_iter_t *target);
snet_util_list_iter_t *SNetUtilListIterPrev(snet_util_list_iter_t *target);
snet_util_list_t *SNetUtilListIterMoveToBack(snet_util_list_iter_t *target);
void *SNetUtilListIterGet(snet_util_list_iter_t *target);
bool SNetUtilListIterHasNext(snet_util_list_iter_t *target);
bool SNetUtilListIterHasPrev(snet_util_list_iter_t *target);
bool SNetUtilListIterCurrentDefined(snet_util_list_iter_t *target);
snet_util_list_iter_t *SNetUtilListIterRotateForward(snet_util_list_iter_t *target);
void SNetUtilListIterDestroy(snet_util_list_iter_t *target);
#endif
