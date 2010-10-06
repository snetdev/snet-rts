#include <stdlib.h>
#include <sched.h>

#include "outport.h"
#include "buffer.h"


outport_t *OutportCreate(stream_t *s)
{
  outport_t *op;

  op = (outport_t *) malloc(sizeof(outport_t));
  op->stream = s;

  return op;
}

void *OutportRead(outport_t *op)
{
  void *item = BufferTop( &op->stream->buffer);
  while( item == NULL ) {
    (void) sched_yield();
    item = BufferTop( &op->stream->buffer);
  }
  BufferPop( &op->stream->buffer);
  return item;
}

void OutportDestroy(outport_t *op)
{
  StreamDestroy( op->stream);
  free(op);
}
