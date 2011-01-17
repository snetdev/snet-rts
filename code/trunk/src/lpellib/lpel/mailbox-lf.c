
#include <stdlib.h>
#include <assert.h>

#include "mailbox-lf.h"




/******************************************************************************/
/* Free node pool management functions                                        */
/******************************************************************************/

static mailbox_node_t *GetFree( mailbox_t *mbox)
{
  mailbox_node_t * volatile top;
  do {
    top = mbox->list_free;
    if (!top) break;
  } while( !compare_and_swap( (void**) &mbox->list_free, top, top->next));

  if (!top) {
    /* allocate new node */
    top = (mailbox_node_t *)malloc( sizeof( mailbox_node_t));
  }
  top->next = NULL;
  return top;
}

static void PutFree( mailbox_t *mbox, mailbox_node_t *node)
{
  mailbox_node_t * volatile  top;
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
  mailbox_node_t * volatile n;
  int i;

  (void) pthread_mutex_init( &mbox->lock_inbox, NULL);
  (void) sem_init( &mbox->counter, 0, 0);
  mbox->list_free  = NULL;

  /* pre-create free nodes */
  for (i=0; i<100; i++) {
    n = (mailbox_node_t *)malloc( sizeof( mailbox_node_t));
    n->next = mbox->list_free;
    mbox->list_free = n;
  }
  
  /* dummy node */
  n = GetFree( mbox);
  n->next = NULL;
    
  mbox->in_head = n;
  mbox->in_tail = n;
  atomic_init( &mbox->in_count, 0);

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
  //assert( atomic_read( &mbox->in_count) == 0);
  /* free dummy */
  PutFree( mbox, mbox->in_head);
  

  /* free list_free */
  do {
    do {
      top = mbox->list_free;
      if (!top) break;
    } while( !compare_and_swap( (void**) &mbox->list_free, top, top->next));

    if (top) free(top);
  } while(top);

  /* destroy sync primitives */
  (void) pthread_mutex_destroy( &mbox->lock_inbox);
  (void) sem_destroy( &mbox->counter);
}






void MailboxSend( mailbox_t *mbox, workermsg_t *msg)
{
  /* get a free node from recepient */
  mailbox_node_t *node = GetFree( mbox);
  /* copy the message */
  node->msg = *msg;

  /* aquire tail lock */
  pthread_mutex_lock( &mbox->lock_inbox);
  /* link node at the end of the linked list */
  mbox->in_tail->next = node;
  /* swing tail to node */
  mbox->in_tail = node;
  /* release tail lock */
  pthread_mutex_unlock( &mbox->lock_inbox);

  /* signal semaphore */
  (void) sem_post( &mbox->counter);

  /* update counter */
  atomic_inc( &mbox->in_count);
}


void MailboxRecv( mailbox_t *mbox, workermsg_t *msg)
{
  //mailbox_node_t *volatile node, *volatile new_head;
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
  atomic_dec( &mbox->in_count);

  /* swing head to next node (becomes new dummy) */
  mbox->in_head = new_head;

  /* put node into free pool */
  PutFree( mbox, node);
}

/**
 * @note: does not need to be locked as a 'missed' msg 
 *        will be eventually fetched in the next worker loop
 */
bool MailboxHasIncoming( mailbox_t *mbox)
{
  return ( atomic_read( &mbox->in_count) > 0 );
}
