#include "node.h"

void SNetStackInit(snet_stack_t *stack)
{
  stack->head = NULL;
}

snet_stack_t *SNetStackCreate(void)
{
  snet_stack_t *stack = SNetNewAlign(snet_stack_t);
  SNetStackInit(stack);
  return stack;
}

void SNetStackDone(snet_stack_t *stack)
{
  assert(stack->head == NULL);
}

void SNetStackDestroy(snet_stack_t *stack)
{
  SNetStackDone(stack);
  SNetDelete(stack);
}

bool SNetStackIsEmpty(snet_stack_t *stack)
{
  return stack->head == NULL;
}

void SNetStackPush(snet_stack_t *stack, void *item)
{
  snet_stack_node_t *snet_stack_node = SNetNew(snet_stack_node_t);

  snet_stack_node->next = stack->head;
  snet_stack_node->item = item;
  stack->head = snet_stack_node;
}

void *SNetStackTop(const snet_stack_t *stack)
{
  return stack->head ? stack->head->item : NULL;
}

void *SNetStackPop(snet_stack_t *stack)
{
  void *item = NULL;
  snet_stack_node_t *head = stack->head;
  if (head) {
    stack->head = head->next;
    item = head->item;
    head->item = NULL;
    SNetDelete(head);
  }
  return item;
}

void SNetStackPopAll(snet_stack_t *stack)
{
  snet_stack_node_t *head;
  while ((head = stack->head) != NULL) {
    stack->head = head->next;
    SNetDelete(head);
  }
}

/* Shallow duplicate a stack. Stack 'dest' must exist as empty. */
void SNetStackCopy(snet_stack_t *dest, const snet_stack_t *source)
{
  const snet_stack_node_t    *iter;
  snet_stack_node_t         **copy = &(dest->head);

  trace(__func__);
  assert(*copy == NULL);

  /* copy nodes into a new list */
  for (iter = source->head; iter != NULL; iter = iter->next) {
    *copy = SNetNew(snet_stack_node_t);
    (*copy)->item = iter->item;
    /* advance pointer to next node */
    copy = &((*copy)->next);
  }
  /* terminate new list */
  *copy = NULL;
}

