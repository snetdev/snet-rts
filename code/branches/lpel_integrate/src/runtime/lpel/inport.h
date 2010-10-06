#ifndef _INPORT_H_
#define _INPORT_H_

#include "stream.h"

typedef struct inport inport_t;

struct inport {
  stream_t *stream;
};

extern inport_t *InportCreate( stream_t *buf);
extern void InportWrite( inport_t *ip, void *item);
extern void InportDestroy( inport_t *ip);


#endif /* _INPORT_H_ */
