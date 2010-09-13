#ifndef _INPORT_H_
#define _INPORT_H_

#include "buffer.h"

typedef struct inport inport_t;

struct inport {
  buffer_t *buffer;
};

extern inport_t *InportCreate( buffer_t *buf);
extern void InportWrite( inport_t *ip, void *item);
extern void InportDestroy( inport_t *ip);


#endif /* _INPORT_H_ */
