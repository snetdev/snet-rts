#ifndef _MAILBOX_H_
#define _MAILBOX_H_

#include <pthread.h>
#include <semaphore.h>

#include "lpel.h"

#include "bool.h"
#include "taskqueue.h"

#define MAILBOX_USE_SPINLOCK

/*
 * worker msg body
 */
typedef enum {
  WORKER_MSG_TERMINATE,
  WORKER_MSG_WAKEUP,
  WORKER_MSG_ASSIGN,
  WORKER_MSG_REQUEST,
  WORKER_MSG_TASKMIG,
} workermsg_type_t;

typedef struct {
  workermsg_type_t  type;
  union {
    lpel_taskreq_t *treq;
    lpel_task_t    *task;
    taskqueue_t     tqueue;
    int             from_worker;
  } body;
} workermsg_t;



/* mailbox structures */

typedef struct mailbox_node_t {
  struct mailbox_node_t * volatile next;
  workermsg_t msg;
} mailbox_node_t;

typedef struct {
#ifdef MAILBOX_USE_SPINLOCK
  pthread_spinlock_t
                   lock_inbox;
#else
  pthread_mutex_t  lock_inbox;
#endif
  sem_t            counter;
  mailbox_node_t  *in_head;
  mailbox_node_t  *in_tail;
  mailbox_node_t  *volatile list_free;
  unsigned long    volatile out_cnt;
} mailbox_t;


void MailboxInit( mailbox_t *mbox);
void MailboxCleanup( mailbox_t *mbox);

mailbox_node_t *MailboxGetFree( mailbox_t *mbox);
mailbox_node_t *MailboxAllocateNode( void);
void MailboxSend( mailbox_t *mbox, mailbox_node_t *node);

void MailboxRecv( mailbox_t *mbox, workermsg_t *msg);

bool MailboxHasIncoming( mailbox_t *mbox);



#endif /* _MAILBOX_H_ */
