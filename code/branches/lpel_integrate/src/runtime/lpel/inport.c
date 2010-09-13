#include <stdlib.h>
#include <sched.h>

#include "inport.h"


inport_t *InportCreate(buffer_t *buf)
{
  inport_t *ip;

  ip = (inport_t *) malloc(sizeof(inport_t));
  ip->buffer = buf;

  return ip;
}

void InportWrite(inport_t *ip, void *item)
{
  while( !BufferIsSpace( ip->buffer) ) {
    (void) sched_yield();
  }
  BufferPut( ip->buffer, item);
}

void InportDestroy(inport_t *ip)
{
  /* stream has to be freed by client */
  free(ip);
}
