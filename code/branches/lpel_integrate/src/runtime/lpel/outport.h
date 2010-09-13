#ifndef _OUTPORT_H_
#define _OUTPORT_H_

#include "buffer.h"

typedef struct outport outport_t;

struct outport {
  buffer_t *buffer;
};

extern outport_t *OutportCreate( buffer_t *buf);
extern void *OutportRead( outport_t *op);
extern void OutportDestroy( outport_t *op);


#endif /* _OUTPORT_H_ */
