
/* 
 * This implements a buffer.
 */


#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include <feedbackbuffer.h>
#include <bool.h>
#include <memfun.h>
#include <record.h>

#ifdef DEBUG
  /* debug.h is nowhere to be found? */
  #include <debug.h>
  extern debug_t *GLOBAL_OUTBUF_DBG; 
  #define GET_TRACE( BUFFER)\
            DBGtraceGet( GLOBAL_OUTBUF_DBG, BUFFER);
#else
  #define GET_TRACE( BUFFER)
#endif

struct fbckbuffer_element {
  void *content;
  fbckbuffer_element* next;
};

struct fbckbuffer  {
  struct fbckbuffer_element **first;

  bool dispatcher_registered;
  sem_t *sem;

  pthread_mutex_t *mxCounter;
  pthread_cond_t  *condEmpty;
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
  struct fbckbuffer_element *first = NULL;

  theBuffer = SNetMemAlloc( sizeof( snet_fbckbuffer_t));
  theBuffer->first = &first;

  theBuffer->dispatcher_registered = false;
  theBuffer->sem = NULL;

  theBuffer->mxCounter = SNetMemAlloc( sizeof( pthread_mutex_t));

  theBuffer->condFull = SNetMemAlloc( sizeof( pthread_cond_t));
  theBuffer->condEmpty = SNetMemAlloc( sizeof( pthread_cond_t));

  pthread_mutex_init(theBuffer->mxCounter, NULL);
  pthread_cond_init(theBuffer->condEmpty, NULL);

  return( theBuffer);
}

extern snet_fbckbuffer_t *SNetFbckBufPut( snet_fbckbuffer_t *bf, void* elem) {
  int current_star_index = -1;
  int current_record_iteration = -1;

  fbckbuffer_element *current = NULL;
  fbckbuffer_element *last = NULL;

  snet_record_t *rec = (snet_record_t*) elem;
 
   /* lock the mutex */	
  pthread_mutex_lock( bf->mxCounter);
    
  /* put element into list */
  struct fbckbuffer_element new_element = SNetMemAlloc(sizeof(struct fbckbuffer_element));
  new_element->content = elem;

  current = *(bf->first);
  last = NULL;
  while(current &&
        SNetRecGetIteration((snet_record_t*) current->content) > SNetGetIteration(rec)) {
    current = current->next;
    last = current;
  }
  if(last == NULL) {
    /* element goes to the beginning of the buffer */
    new_element->next = *(bf->first);
    bf->first = &new_element;
  } else {
    /* element goes somewhere in the list */
    new_element->next = current;
    last->next = new_element;
  }

  if( bf->dispatcher_registered) {
    sem_post( bf->sem);
  }
	
  /* send signal to waiting threads that an item is now available */
  pthread_cond_signal( bf->condEmpty);

  /* unlock the mutex */
  pthread_mutex_unlock( bf->mxCounter);
  
  return( bf);
}

extern void *SNetFbckBufGet( snet_fbckbuffer_t *bf) {
  fbckbuffer_element* element;  
  void *ptr;

  /* lock the mutex */
  pthread_mutex_lock( bf->mxCounter );
 
  /* wait for signal if there are no elements in the buffer */
  /* the mutex is unlocked in this case */
  while(!*(bf->first)) {
    pthread_cond_wait( bf->condEmpty, bf->mxCounter);
  }
    
  /* get the element from the buffer*/
  element = *(bf->first);
  bf->first = &element->next;
  ptr = element->content;
  SNetMemFree(element);

  /* unlock the mutex */
  pthread_mutex_unlock( bf->mxCounter);
    
  GET_TRACE( bf); /* this is nothing unless DEBUG is defined */
  return ( ptr);
}

extern void *SNetFbckBufTryGet(snet_fbckbuffer_t *bf, snet_fbckbuffer_msg_t *msg) { 
  void *ptr=NULL;

  /* return if the buffer is locked */	
  if( pthread_mutex_trylock( bf->mxCounter) == EBUSY ) {
    *msg = BUF_blocked;	    
  }
  else {   /* return if there are no elements in the buffer */
    if( *(bf->first) == NULL) {
      *msg = BUF_empty;
      pthread_mutex_unlock( bf->mxCounter );
    } else { 
      /* get an element from the buffer*/
      element = *(bf->first);
      bf->first = &element->next;
      ptr = element->content;
      SNetMemFree(element);

      pthread_cond_signal( bf->condFull );
      pthread_mutex_unlock( bf->mxCounter );
      *msg = BUF_success;
    }
  }
  return( ptr);
}

extern void *SNetFbckBufShow(snet_fbckbuffer_t *buf) {
  void *ptr;
  
  pthread_mutex_lock( buf->mxCounter );
 
  ptr = *(bf->first); 
  
  pthread_mutex_unlock( buf->mxCounter);
  
  return( ptr);
}

extern void SNetFbckBufBlockUntilEmpty(snet_fbckbuffer_t *bf) {
  
  pthread_mutex_lock( bf->mxCounter);

  while(*(bf->first)) {
    // wake up, if an item is removed from the buffer    
    pthread_cond_wait( bf->condFull, bf->mxCounter);
  }

  pthread_mutex_unlock( bf->mxCounter);
}



extern void SNetFbckBufRegisterDispatcher( snet_fbckbuffer_t *bf, sem_t *sem) {
  int i;
  fbckbuffer_element current;
  pthread_mutex_lock( bf->mxCounter);
  bf->dispatcher_registered = true;
  bf->sem = sem;
  
  current = *(bf->first);
  while(current) {
    sem_post(sem);
  }
  pthread_mutex_unlock( bf->mxCounter);
}


extern void SNetFbckBufDestroyByDispatcher(snet_fbckbuffer_t *bf) {
  bf->dispatcher_registered = false;
  SNetFbckBufDestroy(bf);
}

extern void SNetFbckBufDestroy(snet_fbckbuffer_t *bf) {
  fbckbuffer_element current;

  if(!(bf->dispatcher_registered)) {

    pthread_mutex_destroy( bf->mxCounter);
    pthread_cond_destroy( bf->condEmpty);
    pthread_cond_destroy( bf->condFull);
    
    SNetMemFree( bf->condEmpty);
    SNetMemFree( bf->mxCounter);
    SNetMemFree( bf->condFull);
    /* usually, this should be empty, but better be safe than leaky. */
    current = *(bf->first);
    while(current) {
      SNetMemFree(current);
    }
    SNetMemFree( bf);
  }
}




