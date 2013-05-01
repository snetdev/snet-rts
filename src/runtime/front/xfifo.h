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

/* Iterate over all FIFO elements. */
#define FIFO_FOR_EACH(fif, nod, itm)    \
        for (nod = (fif)->head->next; nod && (itm = nod->item) != NULL; nod = nod->next)

#endif
