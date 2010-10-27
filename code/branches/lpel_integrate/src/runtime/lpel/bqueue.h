#ifndef _BQUEUE_H_
#define _BQUEUE_H_

#include <pthread.h>
#include "bool.h"
#include "taskqueue.h"

typedef struct {
  taskqueue_t     queue;
  pthread_mutex_t lock;
  pthread_cond_t  cond;
  bool            terminate;  
} bqueue_t;


void BQueueInit( bqueue_t *bq);
void BQueueCleanup( bqueue_t *bq);
void BQueuePut( bqueue_t *bq, task_t *t);
task_t *BQueueFetch( bqueue_t *bq);
void BQueueTerm( bqueue_t *bq);

#endif /* _BQUEUE_H_ */
