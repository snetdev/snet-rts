
#include <stdlib.h>
#include <assert.h>

#include "arch/atomic.h"
#include "mailbox-lf.h"




/******************************************************************************/
/* Free node pool management functions                                        */
/******************************************************************************/

mailbox_node_t *MailboxGetFree( mailbox_t *mbox)
{
  mailbox_node_t * volatile top;
  volatile unsigned long ocnt;
  do {
    ocnt = mbox->out_cnt;
    top = mbox->list_free;
    if (!top) return NULL;
  } while( !CAS2( (void**) &mbox->list_free, top, ocnt, top->next, ocnt+1));
  
  top->next = NULL;
  return top;
}

mailbox_node_t *MailboxAllocateNode( void)
{
  /* allocate new node */
  mailbox_node_t *n = (mailbox_node_t *)malloc( sizeof( mailbox_node_t));
  n->next = NULL;
  return n;
}

void MailboxPutFree( mailbox_t *mbox, mailbox_node_t *node)
{
  mailbox_node_t * volatile top;
  do {
    top = mbox->list_free;
    node->next = top;
  } while( !compare_and_swap( (void**) &mbox->list_free, top, node));
}



/******************************************************************************/
/* Public functions                                                           */
/******************************************************************************/


void MailboxInit( mailbox_t *mbox)
{
  mailbox_node_t *n;
  int i;

#ifdef MAILBOX_USE_SPINLOCK
  (void) pthread_spin_init( &mbox->lock_inbox, PTHREAD_PROCESS_PRIVATE);
#else
  (void) pthread_mutex_init( &mbox->lock_inbox, NULL);
#endif
  (void) sem_init( &mbox->counter, 0, 0);

  mbox->list_free  = NULL;
  mbox->out_cnt = 0;

  /* pre-create free nodes */
  for (i=0; i<100; i++) {
    n = (mailbox_node_t *)malloc( sizeof( mailbox_node_t));
    n->next = mbox->list_free;
    mbox->list_free = n;
  }

  /* dummy node */
  n = MailboxAllocateNode();
    
  mbox->in_head = n;
  mbox->in_tail = n;
}



void MailboxCleanup( mailbox_t *mbox)
{
  mailbox_node_t * volatile top;

  while( MailboxHasIncoming( mbox)) {
    workermsg_t msg;
    MailboxRecv( mbox, &msg);
  }
  /* inbox  empty */
  assert( mbox->in_head->next == NULL );

  /* free dummy */
  MailboxPutFree( mbox, mbox->in_head);
  
  /* free list_free */
  do {
    do {
      top = mbox->list_free;
      if (!top) break;
    } while( !compare_and_swap( (void**) &mbox->list_free, top, top->next));

    if (top) free(top);
  } while(top);

  /* destroy sync primitives */
#ifdef MAILBOX_USE_SPINLOCK
  (void) pthread_spin_destroy( &mbox->lock_inbox);
#else
  (void) pthread_mutex_destroy( &mbox->lock_inbox);
#endif
  (void) sem_destroy( &mbox->counter);
}





void MailboxSend( mailbox_t *mbox, mailbox_node_t *node)
{
  assert( node != NULL);

  /* aquire tail lock */
#ifdef MAILBOX_USE_SPINLOCK
  pthread_spin_lock( &mbox->lock_inbox);
#else
  pthread_mutex_lock( &mbox->lock_inbox);
#endif
  /* link node at the end of the linked list */
  mbox->in_tail->next = node;
  /* swing tail to node */
  mbox->in_tail = node;
  /* release tail lock */
#ifdef MAILBOX_USE_SPINLOCK
  pthread_spin_unlock( &mbox->lock_inbox);
#else
  pthread_mutex_unlock( &mbox->lock_inbox);
#endif

  /* signal semaphore */
  (void) sem_post( &mbox->counter);
}


void MailboxRecv( mailbox_t *mbox, workermsg_t *msg)
{
  mailbox_node_t *node, *new_head;

  /* wait semaphore */
  (void) sem_wait( &mbox->counter);

  //do {

  /* read head (dummy) */
  node = mbox->in_head;
  /* read next pointer */
  new_head = node->next;

  //} while (new_head == NULL);

  /* is queue empty? */
  //if (new_head == NULL) return NULL;
  assert( new_head != NULL);

  /* queue is not empty, copy message */
  *msg = new_head->msg;

  /* swing head to next node (becomes new dummy) */
  mbox->in_head = new_head;

  /* put node into free pool */
  MailboxPutFree( mbox, node);
}

/**
 * @note: does not need to be locked as a 'missed' msg 
 *        will be eventually fetched in the next worker loop
 */
bool MailboxHasIncoming( mailbox_t *mbox)
{
  return ( mbox->in_head->next != NULL );
}
