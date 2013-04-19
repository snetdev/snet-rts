/*
 * This FIFO queue allows for concurrent reading and writing,
 * provided that at most one reader and at most one writer
 * are active concurrently. Note that this condition holds 
 * for S-Net streams. 
 *
 * The FIFO is implemented as a linked list with an extra
 * tail pointer. There is always at least one node at the head,
 * which has an empty data item pointer.
 */

#include "node.h"

/* Initialize FIFO with a node with an empty item pointer. */
void SNetFifoInit(fifo_t *fifo)
{
  fifo->head = fifo->tail = SNetNew(fifo_node_t);
  fifo->head->next = NULL;
  fifo->head->item = NULL;
}

/* Create a new FIFO */
fifo_t *SNetFifoCreate(void)
{
  fifo_t *fifo = SNetNewAlign(fifo_t);
  SNetFifoInit(fifo);
  return fifo;
}

/* Cleanup a FIFO. */
void SNetFifoDone(fifo_t *fifo)
{
  assert(fifo->head == fifo->tail);
  assert(fifo->head->next == NULL);
  assert(fifo->head->item == NULL);
  SNetDelete(fifo->head);
  fifo->head = fifo->tail = NULL;
}

/* Delete a FIFO. */
void SNetFifoDestroy(fifo_t *fifo)
{
  SNetFifoDone(fifo);
  SNetDelete(fifo);
}

/* Append a new item at the tail of a FIFO. */
void SNetFifoPut(fifo_t *fifo, void *item)
{
  fifo_node_t   *node = SNetNew(fifo_node_t);
  fifo_node_t   *tail = fifo->tail;

  assert(item);
  node->next = NULL;
  node->item = item;
  tail->next = node;
  fifo->tail = node;
}

/* Retrieve an item from the head of a FIFO, or NULL if empty. */
void *SNetFifoGet(fifo_t *fifo)
{
  void          *item = NULL;
  fifo_node_t   *head = fifo->head;

  if (head->next) {
    item = head->next->item;
    head->next->item = NULL;
    fifo->head = head->next;
    SNetDelete(head);
    assert(item);
  }
  return item;
}

/* A fused send/recv if caller owns both sides. */
void *SNetFifoPutGet(fifo_t *fifo, void *item)
{
  fifo_node_t   *node = fifo->head;

  /* Preserve sequential ordering when FIFO is non-empty. */
  if (node->next) {
    /* Store new input item. */
    node->item = item;
    /* Remove node from head. */
    fifo->head = node->next;
    /* Retrieve new output item. */
    item = fifo->head->item;
    fifo->head->item = NULL;
    /* Append node at the tail. */
    node->next = NULL;
    fifo->tail->next = node;
    fifo->tail = node;
  }
  return item;
}

/* Peek non-destructively at the first FIFO item. */
void *SNetFifoPeekFirst(fifo_t *fifo)
{
  void          *item = NULL;
  fifo_node_t   *next = fifo->head->next;

  if (next) {
    item = next->item;
    assert(item);
  }
  return item;
}

/* Peek non-destructively at the last FIFO item. */
void *SNetFifoPeekLast(fifo_t *fifo)
{
  return fifo->tail->item;
}

/* Return true iff FIFO is empty. */
bool SNetFifoTestEmpty(fifo_t *fifo)
{
  fifo_node_t *next = fifo->head->next;
  return !next || !next->item;
}

/* Return true iff FIFO is non-empty. */
bool SNetFifoNonEmpty(fifo_t *fifo)
{
  fifo_node_t *next = fifo->head->next;
  return next && next->item;
}

/* Detach the tail part of a FIFO queue from a fifo structure. */
fifo_node_t *SNetFifoGetTail(fifo_t *fifo, fifo_node_t **tail_ptr)
{
  fifo_node_t *node = fifo->head->next;
  if (fifo->tail == fifo->head) {
    assert(node == NULL);
    *tail_ptr = NULL;
  } else {
    assert(node != NULL);
    fifo->head->next = NULL;
    *tail_ptr = fifo->tail;
    fifo->tail = fifo->head;
  }
  assert(fifo->tail->item == NULL);
  return node;
}

/* Attach a tail of a FIFO queue to an existing fifo structure. */
void SNetFifoPutTail(fifo_t *fifo, fifo_node_t *node, fifo_node_t *tail)
{
  if (node) {
    fifo->tail->next = node;
    fifo->tail = tail;
  } else {
    assert(!tail);
  }
}

