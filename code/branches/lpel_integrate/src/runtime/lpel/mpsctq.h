#ifndef _MPSCTQ_H_
#define _MPSCTQ_H_

/**
 * Intrusive multiple producer single consumer taskqueue
 */
#include "task.h"

typedef struct {
  task_t *volatile head;
  task_t *tail;
  task_t  stub; /* a dummy node */
} mpsctq_t;

extern void MpscTqInit(mpsctq_t *q);
extern void MpscTqCleanup(mpsctq_t *q);
extern void MpscTqEnqueue(mpsctq_t *q, task_t *t);
extern task_t *MpscTqDequeue(mpsctq_t *q);

#endif /* _MPSCTQ_H_ */
