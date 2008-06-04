#ifndef STACK_HEADER
#define STACK_HEADER

#include "bool.h"

typedef struct stack snet_util_stack_t;

snet_util_stack_t *SNetUtilStackCreate();
void SNetUtilStackDestroy(snet_util_stack_t *target);

bool SNetUtilStackIsEmpty(snet_util_stack_t *target);
void SNetUtilStackPush(snet_util_stack_t *target, void *content);
void SNetUtilStackPop(snet_util_stack_t *target);
void *SNetUtilStackPeek(snet_util_stack_t *target);


void SNetUtilStackGotoTop(snet_util_stack_t *target);
void SNetUtilStackGotoBottom(snet_util_stack_t *target); 
void SNetUtilStackUp(snet_util_stack_t *target);
void SNetUtilStackDown(snet_util_stack_t *target);
void *SNetUtilStackGet(snet_util_stack_t *target);
bool SNetUtilStackCurrentDefined(snet_util_stack_t *target);

#endif
