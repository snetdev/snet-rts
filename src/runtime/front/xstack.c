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

bool SNetStackNonEmpty(snet_stack_t *stack)
{
  return stack->head != NULL;
}

void SNetStackPush(snet_stack_t *stack, void *item)
{
  snet_stack_node_t *snet_stack_node = SNetNew(snet_stack_node_t);

  snet_stack_node->next = stack->head;
  snet_stack_node->item = item;
  stack->head = snet_stack_node;
}

void SNetStackAppend(snet_stack_t *stack, void *item)
{
  snet_stack_node_t *snet_stack_node = SNetNew(snet_stack_node_t);

  snet_stack_node->item = item;
  snet_stack_node->next = NULL;
  if (stack->head == NULL) {
    stack->head = snet_stack_node;
  } else {
    snet_stack_node_t *node = stack->head;
    while (node->next) {
      node = node->next;
    }
    node->next = snet_stack_node;
  }
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

/* Construct a literal copy of an existing stack. */
snet_stack_t* SNetStackClone(const snet_stack_t *source)
{
  snet_stack_t *clone = SNetStackCreate();
  SNetStackCopy(clone, source);
  return clone;
}

/* Swap two stacks. */
void SNetStackSwap(snet_stack_t *one, snet_stack_t *two)
{
  snet_stack_node_t *head = one->head;
  one->head = two->head;
  two->head = head;
}

/* Compute number of elements in the stack. */
int SNetStackElementCount(snet_stack_t *stack)
{
  int count = 0;
  snet_stack_node_t *node;
  for (node = stack->head; node; node = node->next) {
    ++count;
  }
  return count;
}

