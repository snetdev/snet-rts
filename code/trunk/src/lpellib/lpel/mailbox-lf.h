#ifndef _MAILBOX_LF_H_
#define _MAILBOX_LF_H_

#include <pthread.h>
#include <semaphore.h>

#include "lpel.h"

#include "bool.h"

#ifdef __linux__
#define MAILBOX_USE_SPINLOCK
#endif

/*
 * worker msg body
 */
typedef enum {
  WORKER_MSG_TERMINATE = 1,
  WORKER_MSG_WAKEUP,
  WORKER_MSG_ASSIGN,
  WORKER_MSG_REQUEST,
  WORKER_MSG_TASKMIG,
} workermsg_type_t;

/* mailbox structures */

typedef struct {
  workermsg_type_t  type;
  union {
    lpel_taskreq_t *treq;
    lpel_task_t    *task;
    int             from;
  } body;
} workermsg_t;

typedef struct mailbox_node_t {
  struct mailbox_node_t *volatile next;
  workermsg_t msg;
} mailbox_node_t;


typedef struct {
  struct {
    mailbox_node_t  *volatile top;
    unsigned long    volatile out_cnt;
  } stack_free;
#ifdef MAILBOX_USE_SPINLOCK
  pthread_spinlock_t
                   lock_inbox;
#else
  pthread_mutex_t  lock_inbox;
#endif
  sem_t            counter;
  mailbox_node_t  *in_head;
  mailbox_node_t  *in_tail;
} mailbox_t;



void MailboxInit( mailbox_t *mbox);
void MailboxCleanup( mailbox_t *mbox);
void MailboxSend( mailbox_t *mbox, workermsg_t *msg);
void MailboxRecv( mailbox_t *mbox, workermsg_t *msg);
bool MailboxHasIncoming( mailbox_t *mbox);



#endif /* _MAILBOX_LF_H_ */
