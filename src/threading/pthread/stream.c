

#include <string.h>
#include <pthread.h>
#include <assert.h>

#include "threading.h"

#include "memfun.h"

/* 'local' includes */
#include "stream.h"
#include "entity.h"



void SNetStreamSetSource(snet_stream_t *s, snet_locvec_t *lv)
{
  s->source = SNetLocvecCopy(lv);
}

snet_locvec_t *SNetStreamGetSource(snet_stream_t *s)
{
  return s->source;
}


void SNetStreamRegisterReadCallback(snet_stream_t *s,
    void (*callback)(void *), void *cbarg)
{
  s->callback_read.func = callback;
  s->callback_read.arg = cbarg;
}

void *SNetStreamGetCallbackArg(snet_stream_desc_t *sd)
{
  snet_stream_t *s = sd->stream;
  assert(s != NULL);
  return s->callback_read.arg;
}


snet_stream_t *SNetStreamCreate(int capacity)
{
  snet_stream_t *s = SNetMemAlloc(sizeof(snet_stream_t));
  pthread_mutex_init(&s->lock, NULL);
  pthread_cond_init(&s->notempty, NULL);
  pthread_cond_init(&s->notfull, NULL);


  s->size = (capacity > 0) ? capacity : SNET_STREAM_DEFAULT_CAPACITY;
  s->head = 0;
  s->tail = 0;
  s->count = 0;
  s->buffer = SNetMemAlloc( s->size*sizeof(void*) );

  s->producer = NULL;
  s->consumer = NULL;
  s->is_poll = 0;

  memset( s->buffer, 0, s->size*sizeof(void*) );

  s->source = NULL;

  s->callback_read.func = NULL;
  s->callback_read.arg = NULL;
  return s;
}



/* convenience */
void SNetStreamDestroy(snet_stream_t *s)
{
  pthread_mutex_destroy(&s->lock);
  pthread_cond_destroy(&s->notempty);
  pthread_cond_destroy(&s->notfull);

  if (s->source != NULL) {
    SNetLocvecDestroy(s->source);
  }

  SNetMemFree(s->buffer);
  SNetMemFree(s);
}


snet_stream_desc_t *SNetStreamOpen(snet_stream_t *stream, char mode)
{
  assert( mode=='w' || mode=='r' );
  snet_stream_desc_t *sd = SNetMemAlloc(sizeof(snet_stream_t));
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
  SNetMemFree(sd);
}


void SNetStreamReplace(snet_stream_desc_t *sd, snet_stream_t *new_stream)
{
  assert( sd->mode == 'r' );
  SNetStreamDestroy(sd->stream);
  pthread_mutex_lock( &new_stream->lock);
  new_stream->consumer = sd;
  new_stream->is_poll = 0;
  pthread_mutex_unlock( &new_stream->lock);
  sd->stream = new_stream;
}

snet_stream_t *SNetStreamGet(snet_stream_desc_t *sd)
{
  return sd->stream;
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

  /* call the read callback function */
  if (s->callback_read.func) {
    s->callback_read.func(s->callback_read.arg);
  }

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
  sd->thr = SNetThreadingSelf();

  pthread_mutex_lock( &s->lock);
  while(s->count == s->size){
    pthread_cond_wait(&s->notfull, &s->lock);
  }

  s->buffer[s->tail] = item;
  s->tail = (s->tail+1) % s->size;
  s->count++;

  if (s->count==1) pthread_cond_signal(&s->notempty);

  /* stream was registered to poll on */
  assert(s->is_poll == 0 || s->is_poll == 1);
  if (s->is_poll) {
    s->is_poll = 0;
    snet_thread_t *cons = s->consumer->thr;

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
    return -1;
  }
  pthread_mutex_unlock( &s->lock);

  SNetStreamWrite(sd, item);
  return 0;
}



snet_stream_desc_t *SNetStreamPoll(snet_streamset_t *set)
{
  assert( *set != NULL);

  snet_stream_desc_t *result = NULL;
  /* get 'self', i.e. the task calling SNetStreamPoll()
   * the set is a simple pointer to the last element
   */
  snet_thread_t *self = SNetThreadingSelf();
  snet_stream_iter_t *iter;
  int cnt = 0;

  //self = (*set)->thr;

  iter = SNetStreamIterCreate(set);

  /* fast path: there is something anywhere */
  {
    while( SNetStreamIterHasNext(iter)) {
      snet_stream_desc_t *sd = SNetStreamIterNext( iter);
      snet_stream_t *s = sd->stream;
      pthread_mutex_lock(&s->lock);
      if (s->count > 0) result = sd;
      pthread_mutex_unlock(&s->lock);
    }
    if (result != NULL) {
      goto poll_fastpath;
    }
  }




  /* reset wakeup_sd */
  pthread_mutex_lock( &self->lock );
  self->wakeup_sd = NULL;
  pthread_mutex_unlock( &self->lock );

  /* for each stream in the set */
  SNetStreamIterReset(iter, set);
  while( SNetStreamIterHasNext(iter)) {
    snet_stream_desc_t *sd = SNetStreamIterNext( iter);
    sd->thr = SNetThreadingSelf();
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
        cnt++;
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

  SNetStreamIterReset(iter, set);
  while( SNetStreamIterHasNext(iter)) {
    snet_stream_t *s = (SNetStreamIterNext(iter))->stream;
    pthread_mutex_lock(&s->lock);
    s->is_poll = 0;
    pthread_mutex_unlock(&s->lock);
    if (--cnt == 0) break;
  }

poll_fastpath:
  SNetStreamIterDestroy(iter);
  /* 'rotate' list to stream descriptor for non-empty buffer */
  *set = result;

  return result;
}
