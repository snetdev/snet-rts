#ifndef _TASKQUEUE_H_
#define _TASKQUEUE_H_

#include "bool.h"
#include "task.h"

typedef struct {
  task_t *head;
  task_t *tail;
  unsigned int count;
} taskqueue_t;

extern void TaskqueueInit(taskqueue_t *tq);

extern void TaskqueuePushBack(taskqueue_t *tq, task_t *t);
extern void TaskqueuePushFront(taskqueue_t *tq, task_t *t);

extern task_t *TaskqueuePopBack(taskqueue_t *tq);
extern task_t *TaskqueuePopFront(taskqueue_t *tq);

/* = PushBack: */
extern void TaskqueueEnqueue(taskqueue_t *tq, task_t *t);

/* = PopFront: */
extern task_t *TaskqueueDequeue(taskqueue_t *tq);


extern int TaskqueueIterateRemove(taskqueue_t *tq, 
    bool (*cond)(task_t*,void*), void (*action)(task_t*,void*), void *arg );

#endif /* _TASKQUEUE_H_ */
