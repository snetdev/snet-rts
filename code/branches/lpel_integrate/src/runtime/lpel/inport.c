#include <stdlib.h>
#include <sched.h>

#include "inport.h"
#include "buffer.h"


inport_t *InportCreate( stream_t *s)
{
  inport_t *ip;

  ip = (inport_t *) malloc(sizeof(inport_t));
  ip->stream = s;

  return ip;
}

void InportWrite( inport_t *ip, void *item)
{
  while( !BufferIsSpace( &ip->stream->buffer) ) {
    (void) sched_yield();
  }
  BufferPut( &ip->stream->buffer, item);
}

void InportDestroy(inport_t *ip)
{
  /* stream has to be freed by client */
  free(ip);
}
