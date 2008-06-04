#ifndef TREE_HEADER
#define TREE_HEADER

#include "bool.h"
#include "stack.h"

typedef struct tree snet_util_tree_t;

snet_util_tree_t *SNetUtilTreeCreate();
void SNetUtilTreeDestroy(snet_util_tree_t *target);

bool SNetUtilTreeContains(snet_util_tree_t *tree, snet_util_stack_t *key);
void SNetUtilTreeSet(snet_util_tree_t *tree, snet_util_stack_t *key, 
                      void *value);
void *SNetUtilTreeGet(snet_util_tree_t *tree, snet_util_stack_t *key);

#endif
