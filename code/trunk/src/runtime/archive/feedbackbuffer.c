/*
 * $Id$
 */

/*
 * This implements a buffer with unbounded capacity.
 */


#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include "feedbackbuffer.h"
#include "bool.h"
#include "memfun.h"
#include "record_p.h"
#include "list.h"
#include "bool.h"
#include "extended_semaphore.h"

#ifdef DEBUG
  #include <debug.h>
  extern debug_t *GLOBAL_OUTBUF_DBG; 
  #define GET_TRACE( BUFFER)\
            DBGtraceGet( GLOBAL_OUTBUF_DBG, BUFFER);
#else
  #define GET_TRACE( BUFFER)
#endif

struct fbckbuffer  {
  /* this list is sorted descending on the iterations of the records */
  snet_util_list_t *content;
  bool dispatcher_registered;
  sem_t *sem;

  
  pthread_mutex_t *mxCounter;
  bool is_locked;

  pthread_cond_t *elemRemoved;
  pthread_cond_t *condEmpty;
};

#ifdef DEBUG
static void printRec( snet_fbckbuffer_t *b, snet_record_t *rec)
{
  int k;
  
  printf("\n<DEBUG_INF::FeedbackBuffer> ---------------- >");
  printf("\n buffer [%p]", b);
  printf("\n record [%p]\n", rec);
  if( rec != NULL) {
    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
       printf("\n  - %2d fields: ", SNetRecGetNumFields( rec));
       for( k=0; k<SNetRecGetNumFields( rec); k++) {
         printf(" %d ", SNetRecGetFieldNames( rec)[k]);
       }
       printf("\n  - %2d tags  : ", SNetRecGetNumTags( rec));
       for( k=0; k<SNetRecGetNumTags( rec); k++) {
        printf(" %d ", SNetRecGetTagNames( rec)[k]);
       }
       printf("\n  - %2d btags : ", SNetRecGetNumBTags( rec));
       for( k=0; k<SNetRecGetNumBTags( rec); k++) {
        printf(" %d ", SNetRecGetBTagNames( rec)[k]);
       }  
      break;
      default:
      printf("\n Control record: type %d", SNetRecGetDescriptor( rec));
    }
  }
  printf("\n<----------------------------------- <\n\n");
}
#endif
bool SNetFbckBufIsEmpty(snet_fbckbuffer_t *buf) {
  bool is_empty;
  
  
  return SNetUtilListCount(buf->content);
}

extern void SNetFbckBufBlockUntilDataReady(snet_fbckbuffer_t *buf) {
  /* lock the mutex */
  pthread_mutex_lock( buf->mxCounter );
 
  /* wait for signal if there are no elements in the buffer */
  /* the mutex is unlocked in this case */
  while(SNetUtilListIsEmpty(buf->content)) {
    pthread_cond_wait( buf->condEmpty, buf->mxCounter);
  }
  pthread_mutex_unlock( buf->mxCounter);
}

extern snet_fbckbuffer_t *SNetFbckBufCreate() {
  snet_fbckbuffer_t *theBuffer;

  theBuffer = SNetMemAlloc( sizeof( snet_fbckbuffer_t));
  theBuffer->content = SNetUtilListCreate();

  theBuffer->dispatcher_registered = false;
  theBuffer->sem = NULL;

  theBuffer->mxCounter = SNetMemAlloc( sizeof( pthread_mutex_t));
  theBuffer->elemRemoved = SNetMemAlloc(sizeof(pthread_cond_t));
  theBuffer->condEmpty = SNetMemAlloc( sizeof( pthread_cond_t));

  pthread_mutex_init(theBuffer->mxCounter, NULL);
  pthread_cond_init(theBuffer->condEmpty, NULL);
  pthread_cond_init(theBuffer->elemRemoved, NULL);

  return( theBuffer);
}

/* @fn snet_util_list_iterator_t *
 *                    FindPosition(snet_util_list_iterator_t *position,
 *                                           snet_util_record_t *index)
 * @brief moves the given iterator to the element after the correct
 *        insertion point for a sort by insert. The iterator will be
 *        undefined if you have to add at the end of the list.
 * @param position the iterator to modify, must not be null
 * @param index the index to insert
 * @return a pointer to the modified iterator
 */
static snet_util_list_iter_t *
FindPosition(snet_util_list_iter_t *position, snet_record_t *new_element)
{
  snet_record_t *current_element;
  bool sorted;

  while(SNetUtilListIterCurrentDefined(position)) {
    current_element = SNetUtilListIterGet(position);
    sorted =
        SNetRecGetIteration(current_element) > SNetRecGetIteration(new_element);
    if(!sorted) {
      break;
    } else {
      position = SNetUtilListIterNext(position);
    }
  }
  return position;
}

extern snet_fbckbuffer_t *SNetFbckBufPut( snet_fbckbuffer_t *bf, void* elem) {
  snet_record_t *rec;
  snet_util_list_iter_t *target;

  rec = (snet_record_t*) elem;

  pthread_mutex_lock( bf->mxCounter);

  /* sort by insert */
  target = SNetUtilListFirst(bf->content);
  target = FindPosition(target, rec);
  if(SNetUtilListIterCurrentDefined(target)) {
    SNetUtilListIterAddBefore(target, elem);
  } else {
    SNetUtilListAddEnd(bf->content, elem);
  }
  SNetUtilListIterDestroy(elem);

  /* send signal to waiting threads that an item is now available */
  pthread_cond_signal( bf->condEmpty);

  pthread_mutex_unlock( bf->mxCounter);

  return( bf);
}

extern void *SNetFbckBufGet( snet_fbckbuffer_t *bf) {
  void *ptr;

  /* lock the mutex */
  pthread_mutex_lock( bf->mxCounter );
 
  /* wait for signal if there are no elements in the buffer */
  /* the mutex is unlocked in this case */
  while(SNetUtilListIsEmpty(bf->content)) {
    pthread_cond_wait( bf->condEmpty, bf->mxCounter);
  }
    
  ptr = SNetUtilListGetFirst(bf->content);
  SNetUtilListDeleteFirst(bf->content);

  pthread_cond_signal(bf->elemRemoved);
  /* unlock the mutex */
  pthread_mutex_unlock( bf->mxCounter);
    
  GET_TRACE( bf); /* this is nothing unless DEBUG is defined */
  return ( ptr);
}

extern void *SNetFbckBufTryGet(snet_fbckbuffer_t *bf, snet_fbckbuffer_msg_t *msg) { 
  void *ptr=NULL;
  /* return if the buffer is locked */
  if( pthread_mutex_trylock( bf->mxCounter) == EBUSY ) {
    *msg = FBCKBUF_blocked;
  }
  else {   /* return if there are no elements in the buffer */
    if(SNetUtilListIsEmpty(bf->content)) {
      *msg = FBCKBUF_empty;
      pthread_mutex_unlock( bf->mxCounter );
    } else {
      /* get an element from the buffer*/
      ptr = SNetUtilListGetFirst(bf->content);
      SNetUtilListDeleteFirst(bf->content);

      pthread_cond_signal(bf->elemRemoved);
      pthread_mutex_unlock( bf->mxCounter );
      *msg = FBCKBUF_success;
    }
  }
  return( ptr);
}

extern void *SNetFbckBufShow(snet_fbckbuffer_t *buf) {
  void *ptr;

  pthread_mutex_lock( buf->mxCounter );

  if(SNetUtilListIsEmpty(buf->content)) {
    ptr = NULL;
  } else {
    ptr = SNetUtilListGetFirst(buf->content);
  }

  pthread_mutex_unlock( buf->mxCounter);

  return( ptr);
}

extern void SNetFbckBufBlockUntilEmpty(snet_fbckbuffer_t *bf) {
  pthread_mutex_lock( bf->mxCounter);

  while(!SNetUtilListIsEmpty(bf->content)) {
    /* wake up, if an item is removed from the buffer */
    pthread_cond_wait( bf->elemRemoved, bf->mxCounter);
  }

  pthread_mutex_unlock( bf->mxCounter);
}



extern void SNetFbckBufRegisterDispatcher( snet_fbckbuffer_t *bf, snet_ex_sem_t *sem) {
  int i;
  pthread_mutex_lock( bf->mxCounter);
  bf->dispatcher_registered = true;
  //bf->sem = sem;

  for(i = 0; i < SNetUtilListCount(bf->content); i++) {
    //sem_post(sem);
  }
  pthread_mutex_unlock( bf->mxCounter);
}


extern void SNetFbckBufDestroyByDispatcher(snet_fbckbuffer_t *bf) {
  bf->dispatcher_registered = false;
  SNetFbckBufDestroy(bf);
}

extern void SNetFbckBufDestroy(snet_fbckbuffer_t *bf) {
  if(!(bf->dispatcher_registered)) {

    pthread_mutex_destroy( bf->mxCounter);
    pthread_cond_destroy( bf->condEmpty);
    pthread_cond_destroy( bf->elemRemoved);

    SNetMemFree( bf->condEmpty);
    SNetMemFree( bf->mxCounter);
    SNetMemFree( bf->elemRemoved);
    SNetUtilListDestroy(bf->content);
    SNetMemFree( bf);
  }
}




