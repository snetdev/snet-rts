#ifndef _XSTACK_H
#define _XSTACK_H

typedef struct snet_stack_node snet_stack_node_t;
typedef struct snet_stack snet_stack_t;

/* A snet_stack */
struct snet_stack {
  snet_stack_node_t *head;
};

/* A node for snet_stacks */
struct snet_stack_node {
  snet_stack_node_t *next;
  void *item;
};

#define STACK_FOR_EACH(stk, nod, itm)   \
        for (nod = (stk)->head; nod && (itm = nod->item, true); nod = nod->next)

#define STACK_POP_ALL(stk, itm)         \
        while ((itm = SNetStackPop(stk)) != NULL)

#endif
