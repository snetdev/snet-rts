#ifndef _OUTPORT_H_
#define _OUTPORT_H_

#include "task.h"
#include "stream.h"

typedef struct outport outport_t;

struct outport {
  stream_t *stream;
  //task_t *task;
};

extern outport_t *OutportCreate(stream_t *s);
extern void *OutportRead(outport_t *op);
extern void OutportDestroy(outport_t *op);


#endif /* _OUTPORT_H_ */
