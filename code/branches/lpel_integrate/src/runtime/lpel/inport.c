#include <stdlib.h>
#include <sched.h>

#include "inport.h"
#include "atomic.h"


inport_t *InportCreate(stream_t *s)
{
  inport_t *ip;

  atomic_inc(&s->refcnt);
  ip = (inport_t *) malloc(sizeof(inport_t));
  ip->stream = s;

  return ip;
}

void InportWrite(inport_t *ip, void *item)
{
  while( !StreamIsSpace(NULL, ip->stream) ) {
    (void) sched_yield();
  }
  StreamWrite(NULL, ip->stream, item);
}

void InportDestroy(inport_t *ip)
{
  
  /* "close request" */
  StreamDestroy(ip->stream);

  /* stream has to be freed by client */

  free(ip);
}
