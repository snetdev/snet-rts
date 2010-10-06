#ifndef _OUTPORT_H_
#define _OUTPORT_H_

#include "stream.h"

typedef struct outport outport_t;

struct outport {
  stream_t *stream;
};

extern outport_t *OutportCreate( stream_t *buf);
extern void *OutportRead( outport_t *op);
extern void OutportDestroy( outport_t *op);


#endif /* _OUTPORT_H_ */
