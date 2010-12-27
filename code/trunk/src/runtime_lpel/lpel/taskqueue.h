#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "lpel.h"
#include "bool.h"


typedef struct {
  lpel_task_t *head, *tail;
  unsigned int count;
} taskqueue_t;

void TaskqueueInit( taskqueue_t *tq);

void TaskqueuePushBack(  taskqueue_t *tq, lpel_task_t *t);
void TaskqueuePushFront( taskqueue_t *tq, lpel_task_t *t);

lpel_task_t *TaskqueuePopBack(  taskqueue_t *tq);
lpel_task_t *TaskqueuePopFront( taskqueue_t *tq);

#define TaskqueueEnqueue    TaskqueuePushBack
#define TaskqueueDequeue    TaskqueuePopFront

int TaskqueueIterateRemove( taskqueue_t *tq, 
    bool (*cond)( lpel_task_t*,void*),
    void (*action)(lpel_task_t*,void*),
    void *arg );

#endif /* _TASKQUEUE_H_ */
