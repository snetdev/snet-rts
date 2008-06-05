
/* 
 * This implements a buffer.
 */


#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include "feedbackbuffer.h"
#include "bool.h"
#include "memfun.h"
#include "record.h"
#include "list.h"

#ifdef DEBUG
  #include <debug.h>
  extern debug_t *GLOBAL_OUTBUF_DBG; 
  #define GET_TRACE( BUFFER)\
            DBGtraceGet( GLOBAL_OUTBUF_DBG, BUFFER);
#else
  #define GET_TRACE( BUFFER)
#endif

struct fbckbuffer  {
  snet_util_list_t *content;
  bool dispatcher_registered;
  sem_t *sem;

  
  pthread_mutex_t *mxCounter;
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

extern snet_fbckbuffer_t *SNetFbckBufPut( snet_fbckbuffer_t *bf, void* elem) {
  snet_record_t *current;

  snet_record_t *rec = (snet_record_t*) elem;
 
   /* lock the mutex */	
  pthread_mutex_lock( bf->mxCounter);
    
  /* put element into list */
  if(SNetUtilListIsEmpty(bf->content)) {
    SNetUtilListAddBeginning(bf->content, elem);
  } else {
    /* sort by insert */
    SNetUtilListGotoBeginning(bf->content);
    current = (snet_record_t*)SNetUtilListGet(bf->content);
    while(SNetRecGetIteration(current) < SNetRecGetIteration(rec)) {
      SNetUtilListNext(bf->content);
      current = SNetUtilListGet(bf->content);
    }
    SNetUtilListAddBefore(bf->content, rec);
  }
	
  /* send signal to waiting threads that an item is now available */
  pthread_cond_signal( bf->condEmpty);

  /* unlock the mutex */
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
    // wake up, if an item is removed from the buffer    
    pthread_cond_wait( bf->elemRemoved, bf->mxCounter);
  }

  pthread_mutex_unlock( bf->mxCounter);
}



extern void SNetFbckBufRegisterDispatcher( snet_fbckbuffer_t *bf, sem_t *sem) {
  int i;
  pthread_mutex_lock( bf->mxCounter);
  bf->dispatcher_registered = true;
  bf->sem = sem;
  
  for(i = 0; i < SNetUtilListCount(bf->content); i++) {
    sem_post(sem);
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




