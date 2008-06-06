
/* 
 * This implements a buffer.
 */


#include <stdlib.h>
#include <stdio.h>

#include <errno.h>

#include <buffer.h>
#include <bool.h>
#include <memfun.h>
#include <record.h>

#ifdef DEBUG
  #include <debug.h>
  extern debug_t *GLOBAL_OUTBUF_DBG; 
  #define GET_TRACE( BUFFER)\
            DBGtraceGet( GLOBAL_OUTBUF_DBG, BUFFER);
#else
  #define GET_TRACE( BUFFER)
#endif


struct buffer  {
  void **ringBuffer;
  unsigned int bufferCapacity;
  unsigned int bufferSpaceLeft;
  unsigned int bufferHead;
  unsigned int bufferEnd;

  bool dispatcher_registered;
  sem_t *sem;

  pthread_mutex_t *mxCounter;
  pthread_cond_t  *condFull;
  pthread_cond_t  *condEmpty;
};

#ifdef DEBUG
static void printRec( snet_buffer_t *b, snet_record_t *rec)
{
  int k;
  
  printf("\n<DEBUG_INF::Buffer> ---------------- >");
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

snet_buffer_t *SNetBufCreate( unsigned int size) {
  snet_buffer_t *theBuffer;
       
  theBuffer = SNetMemAlloc( sizeof( snet_buffer_t));
	theBuffer->ringBuffer = SNetMemAlloc( size * sizeof( void**));
  theBuffer->bufferCapacity=size;
  theBuffer->bufferHead=0;
  theBuffer->bufferEnd=0;
  theBuffer->bufferSpaceLeft=size;

  theBuffer->dispatcher_registered = false;
  theBuffer->sem = NULL;

  theBuffer->mxCounter = SNetMemAlloc( sizeof( pthread_mutex_t));

  theBuffer->condFull = SNetMemAlloc( sizeof( pthread_cond_t));
  theBuffer->condEmpty = SNetMemAlloc( sizeof( pthread_cond_t));

  pthread_mutex_init(theBuffer->mxCounter, NULL);
  pthread_cond_init(theBuffer->condFull, NULL);
  pthread_cond_init(theBuffer->condEmpty, NULL);

  return( theBuffer);
}

snet_buffer_t *SNetBufPut( snet_buffer_t *bf, void* elem) {

   /* lock the mutex */	
  pthread_mutex_lock( bf->mxCounter);
    
  /* wait for signal if the buffer has no space left */
  /* the mutex is unlocked in this case */
  while( bf->bufferSpaceLeft == 0) {
    pthread_cond_wait( bf->condFull, bf->mxCounter);
  }
    
  /* put pointer into the stack */
  bf->ringBuffer[ bf->bufferHead ] = elem;  
#ifdef DEBUG
  printRec( bf, (snet_record_t*)elem);
#endif
  bf->bufferHead += 1;
  bf->bufferHead %= bf->bufferCapacity;
  bf->bufferSpaceLeft -= 1;

  if( bf->dispatcher_registered) {
    sem_post( bf->sem);
  }
	
  /* send signal to waiting threads that an item is now available */
  pthread_cond_signal( bf->condEmpty);

  /* unlock the mutex */
  pthread_mutex_unlock( bf->mxCounter);
  
  return( bf);
}

extern void *SNetBufGet( snet_buffer_t *bf) {

  void *ptr;

  /* lock the mutex */
  pthread_mutex_lock( bf->mxCounter );
 
  /* wait for signal if there are no elements in the buffer */
  /* the mutex is unlocked in this case */
  while( bf->bufferSpaceLeft == bf->bufferCapacity ) {
    pthread_cond_wait( bf->condEmpty, bf->mxCounter);
  }
    
  /* get the element from the buffer*/
  ptr = (bf->ringBuffer[ bf->bufferEnd++ ]);   
  bf->bufferEnd %= bf->bufferCapacity;
  bf->bufferSpaceLeft += 1;
  

  /* signal that there is space available in the buffer now */
  pthread_cond_signal( bf->condFull);

  /* unlock the mutex */
  pthread_mutex_unlock( bf->mxCounter);
    
  GET_TRACE( bf);
  return ( ptr);
}


extern snet_buffer_t *SNetBufTryPut( snet_buffer_t *bf, void* elem, snet_buffer_msg_t *msg) {

  /* return if the buffer is locked */	
  if( pthread_mutex_trylock( bf->mxCounter) == EBUSY) {  
    *msg = BUF_blocked;	    
  }
  else {    /* return if the buffer is full */
    if( bf->bufferSpaceLeft == 0 ) {
  		*msg = BUF_full;
      pthread_mutex_unlock( bf->mxCounter );
    }
    else /* put the element into the buffer */
    {
      bf->ringBuffer[ bf->bufferHead++ ] = elem;   
    
      bf->bufferHead %= bf->bufferCapacity;
      bf->bufferSpaceLeft -= 1;

      if( bf->dispatcher_registered) {
        sem_post( bf->sem);
      }

      pthread_cond_signal( bf->condEmpty);
      pthread_mutex_unlock( bf->mxCounter);
      *msg = BUF_success;
    }
  } 
  return( bf);
}


extern void *SNetBufTryGet( snet_buffer_t *bf, snet_buffer_msg_t *msg) { 

  void *ptr=NULL;

  /* return if the buffer is locked */	
  if( pthread_mutex_trylock( bf->mxCounter) == EBUSY ) {
    *msg = BUF_blocked;	    
  }
  else {   /* return if there are no elements in the buffer */
    if( bf->bufferSpaceLeft == bf->bufferCapacity ) {
   		*msg = BUF_empty;
      pthread_mutex_unlock( bf->mxCounter );
    } else { /* get an element from the buffer */
      ptr = (bf->ringBuffer[ bf->bufferEnd++ ]);   

      bf->bufferEnd %= bf->bufferCapacity;
      bf->bufferSpaceLeft++;

      pthread_cond_signal( bf->condFull );
      pthread_mutex_unlock( bf->mxCounter );
      *msg = BUF_success;
    }
  }
  return( ptr);
}

extern void *SNetBufShow( snet_buffer_t *buf) {
 
  void *ptr;
  
  pthread_mutex_lock( buf->mxCounter );
  
  if( buf->bufferCapacity != buf->bufferSpaceLeft) {
    ptr = (buf->ringBuffer[ buf->bufferEnd]);
  }
  else {
   ptr = NULL; 
  }
  
  pthread_mutex_unlock( buf->mxCounter);
  
  return( ptr);
}


extern unsigned int SNetBufGetCapacity( snet_buffer_t *bf) {
  return( bf->bufferCapacity);
}

extern unsigned int SNetGetSpaceLeft( snet_buffer_t *bf) {
  return( bf->bufferSpaceLeft);
}



extern void SNetBufBlockUntilEmpty( snet_buffer_t *bf) {
  
  pthread_mutex_lock( bf->mxCounter);

  while( bf->bufferCapacity != bf->bufferSpaceLeft) {
    // wake up, if an item is removed from the buffer    
    pthread_cond_wait( bf->condFull, bf->mxCounter);
  }

  pthread_mutex_unlock( bf->mxCounter);
}



extern void SNetBufRegisterDispatcher( snet_buffer_t *bf, sem_t *sem) {

  int i;
  pthread_mutex_lock( bf->mxCounter);
  bf->dispatcher_registered = true;
  bf->sem = sem;
  for( i=0; i<( SNetBufGetCapacity( bf) - SNetGetSpaceLeft( bf)); i++) {
    sem_post( sem);
  }
  pthread_mutex_unlock( bf->mxCounter);
}


extern void SNetBufDestroyByDispatcher( snet_buffer_t *bf) {
  bf->dispatcher_registered = false;
  SNetBufDestroy( bf);
}

extern void SNetBufDestroy( snet_buffer_t *bf) {
  if( !( bf->dispatcher_registered)) {

   pthread_mutex_destroy( bf->mxCounter);
   pthread_cond_destroy( bf->condEmpty);
   pthread_cond_destroy( bf->condFull);
   
   SNetMemFree( bf->condEmpty);
   SNetMemFree( bf->mxCounter);
   SNetMemFree( bf->condFull);
   SNetMemFree( bf->ringBuffer);
   SNetMemFree( bf);
  }
}




