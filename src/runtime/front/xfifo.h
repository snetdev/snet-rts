#ifndef _XFIFO_H
#define _XFIFO_H

typedef struct fifo_node fifo_node_t;
typedef struct fifo fifo_t;

/* A node for FIFO queues */
struct fifo_node {
  fifo_node_t  *next;
  void         *item;
};

/* A FIFO queue */
struct fifo {
  fifo_node_t  *head;
  fifo_node_t  *tail;
};

/* Get the first FIFO node. */
#define FIFO_FIRST_NODE(fif)    (fif)->head->next
#define FIFO_NODE_NEXT(node)    (node)->next
#define FIFO_NODE_ITEM(node)    (node)->item

#endif
