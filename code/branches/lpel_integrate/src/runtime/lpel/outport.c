#include <stdlib.h>
#include <sched.h>

#include "outport.h"


outport_t *OutportCreate(buffer_t *buf)
{
  outport_t *op;

  op = (outport_t *) malloc(sizeof(outport_t));
  op->buffer = buf;

  return op;
}

void *OutportRead(outport_t *op)
{
  void *item = BufferTop( op->buffer);
  while( item == NULL ) {
    (void) sched_yield();
    item = BufferTop( op->buffer);
  }
  BufferPop( op->buffer);
  return item;
}

void OutportDestroy(outport_t *op)
{
  BufferDestroy( op->buffer);
  free(op);
}
