

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "threading.h"

#include "stream.h"
#include "entity.h"



snet_stream_t *SNetStreamCreate(int capacity)
{
  snet_stream_t *s = malloc(sizeof(snet_stream_t));
  pthread_mutex_init(&s->lock, NULL);
  pthread_cond_init(&s->notempty, NULL);
  pthread_cond_init(&s->notfull, NULL);
  

  s->size = (capacity > 0) ? capacity : SNET_STREAM_DEFAULT_CAPACITY;
  s->head = 0;
  s->tail = 0;
  s->count = 0;
  s->buffer = malloc( s->size*sizeof(void*) );

  s->producer = NULL;
  s->consumer = NULL;
  s->is_poll = 0;

  memset( s->buffer, 0, s->size*sizeof(void*) );
  return s;
}



/* convenience */
void SNetStreamDestroy(snet_stream_t *s)
{
  pthread_mutex_destroy(&s->lock);
  pthread_cond_destroy(&s->notempty);
  pthread_cond_destroy(&s->notfull);

  free(s->buffer);
  free(s);
}


snet_stream_desc_t *SNetStreamOpen(snet_entity_t *entity, snet_stream_t *stream, char mode)
{
  assert( mode=='w' || mode=='r' );
  snet_stream_desc_t *sd = malloc(sizeof(snet_stream_t));
  sd->entity = entity;
  sd->stream = stream;
  sd->next = NULL;
  sd->mode = mode;
  /* assign consumer or producer */
  switch(mode) {
    case 'w': stream->producer = sd; break;
    case 'r': stream->consumer = sd; break;
    default: assert(0);/*nop*/
  }
  return sd;
}


void SNetStreamClose(snet_stream_desc_t *sd, int destroy_s)
{
  if (destroy_s) {
    SNetStreamDestroy(sd->stream);
  }
  free(sd);
}


void SNetStreamReplace(snet_stream_desc_t *sd, snet_stream_t *new_stream)
{
  assert( sd->mode == 'r' );
  SNetStreamDestroy(sd->stream);
  new_stream->consumer = sd;
  sd->stream = new_stream;
}




void *SNetStreamRead(snet_stream_desc_t *sd)
{
  assert( sd->mode == 'r' );
  snet_stream_t *s = sd->stream;
  void *item;

  pthread_mutex_lock( &s->lock);
  while(s->count == 0){
    pthread_cond_wait(&s->notempty, &s->lock);
  }
 
  item = s->buffer[s->head];
  s->buffer[s->head] = NULL;
  s->head = (s->head+1) % s->size;
  s->count--;

  if (s->count == s->size-1) pthread_cond_signal(&s->notfull);

  pthread_mutex_unlock( &s->lock);

  return item;
}



void *SNetStreamPeek(snet_stream_desc_t *sd)
{
  assert( sd->mode == 'r' );
  void *top = NULL;
  snet_stream_t *s = sd->stream;

  pthread_mutex_lock( &s->lock);
  if (s->count > 0) {
    top = s->buffer[s->head];
  }
  pthread_mutex_unlock( &s->lock);

  return top;
}

void SNetStreamWrite(snet_stream_desc_t *sd, void *item)
{
  assert( sd->mode == 'w' );
  snet_stream_t *s = sd->stream;

  pthread_mutex_lock( &s->lock);
  while(s->count == s->size){
    pthread_cond_wait(&s->notfull, &s->lock);
  }
 
  s->buffer[s->tail] = item;
  s->tail = (s->tail+1) % s->size;
  s->count++;

  if (s->count==1) pthread_cond_signal(&s->notempty);

  /* stream was registered to poll on */
  if (s->is_poll) {
    s->is_poll = 0;
    snet_entity_t *cons = s->consumer->entity;
    
    pthread_mutex_lock( &cons->lock );
    if (cons->wakeup_sd == NULL) {
      cons->wakeup_sd = s->consumer;
      pthread_cond_signal( &cons->pollcond );
    }
    pthread_mutex_unlock( &cons->lock );
  }
  pthread_mutex_unlock( &s->lock);
}

int SNetStreamTryWrite(snet_stream_desc_t *sd, void *item)
{
  assert( sd->mode == 'w' );

  snet_stream_t *s = sd->stream;

  pthread_mutex_lock( &s->lock);
  if (s->count == s->size) {
    pthread_mutex_unlock( &s->lock);
    return 1;
  }
  pthread_mutex_unlock( &s->lock);
 
  SNetStreamWrite(sd, item);
  return 0;
}



snet_stream_desc_t *SNetStreamPoll(snet_streamset_t *set)
{
  assert( *set != NULL);

  snet_stream_desc_t *result = NULL;
  snet_entity_t *self;
  snet_stream_iter_t *iter;

  /* get 'self', i.e. the task calling SNetStreamPoll()
   * the set is a simple pointer to the last element
   */
  self = (*set)->entity;

  /* reset wakeup_sd */
  pthread_mutex_lock( &self->lock );
  self->wakeup_sd = NULL;
  pthread_mutex_unlock( &self->lock );

  /* for each stream in the set */
  iter = SNetStreamIterCreate(set);
  while( SNetStreamIterHasNext(iter)) {
    snet_stream_desc_t *sd = SNetStreamIterNext( iter);
    snet_stream_t *s = sd->stream;

    /* lock stream (prod-side) */
    pthread_mutex_lock( &s->lock);
    { /* CS BEGIN */
      /* check if there is something in the buffer */
      if ( s->count > 0 ) {
        /* yes, we can stop iterating through streams. */
        pthread_mutex_lock( &self->lock );
        if (self->wakeup_sd == NULL) {
          self->wakeup_sd = sd;
        }
        /* unlock self */
        pthread_mutex_unlock( &self->lock );
        /* unlock stream */
        pthread_mutex_unlock( &s->lock);
        /* exit loop */
        break;

      } else {
        /* nothing in the buffer, register stream as activator */
        s->is_poll = 1;
      }
    } /* CS END */
    /* unlock stream */
    pthread_mutex_unlock( &s->lock);
  } /* end for each stream */

  /* wait until wakeup_sd is set */
  pthread_mutex_lock( &self->lock );
  while( self->wakeup_sd == NULL ) {
    pthread_cond_wait(&self->pollcond, &self->lock);
  }
  result = self->wakeup_sd;
  self->wakeup_sd = NULL;
  pthread_mutex_unlock( &self->lock );

  
  /* 'rotate' list to stream descriptor for non-empty buffer */
  *set = result;

  return result;
}
