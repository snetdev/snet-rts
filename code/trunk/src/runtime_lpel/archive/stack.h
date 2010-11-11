#ifndef STACK_HEADER
#define STACK_HEADER

#include "bool.h"

typedef struct stack snet_util_stack_t;
typedef struct stack_iterator snet_util_stack_iterator_t;

snet_util_stack_t *SNetUtilStackCreate();
void SNetUtilStackDestroy(snet_util_stack_t *target);

bool SNetUtilStackIsEmpty(snet_util_stack_t *target);
snet_util_stack_t *SNetUtilStackPush(snet_util_stack_t *target, int content);
snet_util_stack_t *SNetUtilStackPop(snet_util_stack_t *target);
int SNetUtilStackPeek(snet_util_stack_t *target);
snet_util_stack_t *SNetUtilStackSet(snet_util_stack_t *target, int new_value);


snet_util_stack_iterator_t *SNetUtilStackTop(snet_util_stack_t *target);
snet_util_stack_iterator_t *SNetUtilStackBottom(snet_util_stack_t *target);
void SNetUtilStackIterDestroy(snet_util_stack_iterator_t *target);
snet_util_stack_iterator_t *
SNetUtilStackIterNext(snet_util_stack_iterator_t *target);
bool SNetUtilStackIterCurrDefined(snet_util_stack_iterator_t *target);
int SNetUtilStackIterGet(snet_util_stack_iterator_t *target);
#endif
