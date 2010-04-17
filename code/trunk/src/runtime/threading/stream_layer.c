/*
 * $Id$
 */

#include <pthread.h>

#include "semaphore.h"
#include "list.h"
#include "feedbackbuffer.h"
#include "buffer.h"
#include "debug.h"
#include "memfun.h"
#include "ubuffer.h"

/* NOTICE:
 *
 * Locking:
 * 
 * - Stream is locked first and then the stream set (if needed)
 *   -> Always in this order or a deadlock may occur!
 *
 * Stream sets:
 *
 * - Streams that belong to a set are not destroyed before they are 
 *   removed from the set.
 * - Stream belonging to a set must be removed from the set before
 *   it can be added into another set
 *
 * Read/write
 *
 * - Any number of threads may write into any streams
 * - Only a single thread (at the time) may use Read/ReadMany/Peek/PeekMany
 *   on a stream that belongs to a stream set, otherwise a deadlock is very likely.
 *   - This should not be a problem in any case so far, but
 *     makes the synchronization simpler.
 * - Only the thread reading a stream set may use stream set operations.
 *   - i.e. no stream set operations during Read/ReadMany/Peek/PeekMany
 *     for streams belonging in a set.
 *
 */

#ifdef DISTRIBUTED_SNET
#define INVALID_LOCATION -1
#endif /* DISTRIBUTED_SNET*/

#define INVALID_INDEX -1
#define SET_INCREASE_SIZE 1

struct streamset {
  pthread_mutex_t access;        /* Mutex to guard access to the set */

  unsigned int stream_count;     /* Number of streams in the set */
  unsigned int size;             /* Max streams in the set */

  unsigned int *rec_counts;      /* Count of records in each stream, used to
				  * determine if a set contains records */
  snet_tl_stream_t **streams;    /* Streams in the set */

  snet_ex_sem_t *record_counter; /* This semaphore is used to count the records */
};

struct stream {
  pthread_mutex_t access;      /* Mutex to guard access to the stream */

  union buffers {
    snet_buffer_t *buffer;
    snet_ubuffer_t *ubuffer;
  } buffers;                   /* The actual record buffer */
 
  bool is_unbounded;           /* The type of the buffer */
  bool is_obsolete;            /* Is this stream obsolete */

  snet_tl_streamset_t *set;    /* The set the stream belong to, or NULL */
  int set_index;               /* Index of the stream in the set */

#ifdef DISTRIBUTED_SNET
  int location;                /* The location from where the stream originates */
#endif /* DISTRIBUTED_SNET*/
};


static snet_record_t *UnifiedPeek(snet_tl_stream_t *stream)
{
  snet_record_t *result = NULL;

  if(stream->is_unbounded) {
    result = SNetUBufShow(stream->buffers.ubuffer);
  } else {
    result = SNetBufShow(stream->buffers.buffer);
  }

  return result;
}

static snet_record_t *UnifiedRead(snet_tl_stream_t *stream)
{
  snet_record_t *result;

  if(stream->is_unbounded) {
    result = SNetUBufGet(stream->buffers.ubuffer);
  } else {
    /* SNetBufGet returns void* */
    result = (snet_record_t *)SNetBufGet(stream->buffers.buffer);
  }

  return result;
}

static bool UnifiedIsEmpty(snet_tl_stream_t *stream)
{
  bool result;

  if(stream->is_unbounded) {
    result = SNetUBufIsEmpty(stream->buffers.ubuffer);
  } else {
    result = SNetBufIsEmpty(stream->buffers.buffer);
  }
  return result;
}

static unsigned int UnifiedSize(snet_tl_stream_t *stream)
{
  unsigned int result;

  if(stream->is_unbounded) {
    result = SNetUBufSize(stream->buffers.ubuffer);
  } else {
    result = SNetBufSize(stream->buffers.buffer);
  }
  return result;
}

static bool UnifiedGetFlag(snet_tl_stream_t *stream)
{
  bool result = false;

  if(stream->is_unbounded) {
    result = SNetUBufGetFlag(stream->buffers.ubuffer);
  } else {
    result = SNetBufGetFlag(stream->buffers.buffer);
  } 

  return result;
}

static bool UnifiedSetFlag(snet_tl_stream_t *stream, bool flag)
{
  bool result = false;

  if(stream->is_unbounded) {
    SNetUBufSetFlag(stream->buffers.ubuffer, flag);
    result = true;
  } else {
    SNetBufSetFlag(stream->buffers.buffer, flag);
    result = true;
  } 

  return result;
}

static bool UnifiedWrite(snet_tl_stream_t *stream,
			 snet_record_t *rec)
{
  bool record_written = true;

  if(stream->is_unbounded) {
    record_written = SNetUBufPut(stream->buffers.ubuffer, rec);
  } else {    
    SNetBufPut(stream->buffers.buffer, rec);    
  }

  return record_written;
}

static void UnifiedDestroy(snet_tl_stream_t *stream)
{
  if(stream->is_unbounded) {
    SNetUBufDestroy(stream->buffers.ubuffer);
  } else {
    SNetBufDestroy(stream->buffers.buffer);
  }
  
  pthread_mutex_destroy(&stream->access);
  SNetMemFree(stream);
}

static int UnifiedRegisterDispatcher(snet_tl_stream_t *stream,
				     snet_ex_sem_t *dispatcher)
{
  int ret;

  if(stream->is_unbounded) {
    ret = SNetUBufRegisterDispatcher(stream->buffers.ubuffer, dispatcher);
  } else {
    ret = SNetBufRegisterDispatcher(stream->buffers.buffer, dispatcher);
  }

  return ret;
}

static int UnifiedUnregisterDispatcher(snet_tl_stream_t *stream,
				       snet_ex_sem_t *dispatcher)
{
  int ret;

  if(stream->is_unbounded) {
    ret = SNetUBufUnregisterDispatcher(stream->buffers.ubuffer, dispatcher);
  } else {
    ret = SNetBufUnregisterDispatcher(stream->buffers.buffer, dispatcher);
  }
  return ret;
}

void
SNetTlCreateComponent(void* (*func)(void*),
                      void *args,
                      snet_entity_id_t type)
{
  SNetThreadCreate( (void*)func, (void*)args, type);
}

snet_tl_stream_t *SNetTlCreateStream(int size)
{
  snet_tl_stream_t *stream;

  stream = (snet_tl_stream_t *)SNetMemAlloc(sizeof(snet_tl_stream_t));
  
  pthread_mutex_init(&stream->access, NULL);

  stream->buffers.buffer = SNetBufCreate(size, &stream->access);

  stream->is_unbounded = false;

  stream->is_obsolete = false;

  stream->set = NULL;
  stream->set_index = INVALID_INDEX;

#ifdef DISTRIBUTED_SNET
  stream->location = INVALID_LOCATION;
#endif /* DISTRIBUTED_SNET*/

  return stream;
}

snet_tl_stream_t *SNetTlCreateUnboundedStream()
{
  snet_tl_stream_t *stream;

  stream = (snet_tl_stream_t *)SNetMemAlloc(sizeof(snet_tl_stream_t));
  
  pthread_mutex_init(&stream->access, NULL);

  stream->buffers.ubuffer = SNetUBufCreate(&stream->access);

  stream->is_unbounded = true;

  stream->is_obsolete = false;

  stream->set = NULL;
  stream->set_index = INVALID_INDEX;

#ifdef DISTRIBUTED_SNET
  stream->location = INVALID_LOCATION;
#endif /* DISTRIBUTED_SNET*/

  return stream;
}

snet_tl_stream_t *SNetTlMarkObsolete(snet_tl_stream_t *stream)
{

  pthread_mutex_lock(&stream->access);

  stream->is_obsolete = true;

  if(UnifiedIsEmpty(stream) && stream->set == NULL) {
    pthread_mutex_unlock(&stream->access);

    UnifiedDestroy(stream);
  } else {
    pthread_mutex_unlock(&stream->access);
  }

  return stream;
}

snet_tl_streamset_t *SNetTlCreateStreamset()
{
  snet_tl_streamset_t *set;

  set = (snet_tl_streamset_t *)SNetMemAlloc(sizeof(snet_tl_streamset_t));

  pthread_mutex_init(&set->access, NULL);

  set->size = 0;
  set->stream_count = 0;

  set->rec_counts = NULL;
  set->streams = NULL;

  set->record_counter = SNetExSemCreate(&set->access,
					true, 0,
					false, -1,
					0);

  return set;
}

void SNetTlDestroyStreamset(snet_tl_streamset_t *set)
{
  pthread_mutex_destroy(&set->access);

  if(set->size > 0) {
    SNetMemFree(set->rec_counts);
    SNetMemFree(set->streams);
  }

  SNetExSemDestroy(set->record_counter);
  SNetMemFree(set);
}

snet_tl_streamset_t*
SNetTlAddToStreamset(snet_tl_streamset_t *set, snet_tl_stream_t *stream)
{
  unsigned int *new_counts;
  snet_tl_stream_t **new_streams;
  int new_size;
  int i;

  pthread_mutex_lock(&stream->access);

  pthread_mutex_lock(&set->access);

  if(stream->set != NULL) {

    SNetUtilDebugFatal("SNetTlAddToStreamset: Stream already belongs to another set!");
  }

  if(set->stream_count == set->size) {
    new_size = set->size + SET_INCREASE_SIZE;

    new_counts = SNetMemAlloc(sizeof(unsigned int) * new_size);
    new_streams = SNetMemAlloc(sizeof(snet_tl_stream_t *) * new_size);

    for(i = 0; i < set->size; i++) {
      new_counts[i] = set->rec_counts[i];
      new_streams[i] = set->streams[i];
    }
    
    for(i = set->size; i < new_size; i++) {
      new_counts[i] = 0;
      new_streams[i] = NULL;
    }

    i = set->size;

    SNetMemFree(set->rec_counts);
    SNetMemFree(set->streams);
    
    set->size = new_size;
    set->rec_counts = new_counts;
    set->streams = new_streams;
 
  } else {

    for(i = 0; i < set->size; i++) {
      if(set->streams[i] == NULL) {
	break;
      }
    }
  
  }

  stream->set_index = i;

  set->stream_count++;
  set->streams[stream->set_index] = stream;
  set->rec_counts[stream->set_index] = UnifiedSize(stream);

  UnifiedRegisterDispatcher(stream, set->record_counter);
  
  stream->set = set;

  pthread_mutex_unlock(&set->access);

  pthread_mutex_unlock(&stream->access);

  return set;
}

snet_tl_streamset_t*
SNetTlDeleteFromStreamset(snet_tl_streamset_t *set,
                          snet_tl_stream_t *stream)
{

  pthread_mutex_lock(&stream->access);

  set = stream->set;

  pthread_mutex_lock(&set->access);

  set->rec_counts[stream->set_index] = 0;
  set->streams[stream->set_index] = NULL;
  set->stream_count--;

  UnifiedUnregisterDispatcher(stream, set->record_counter);

  pthread_mutex_unlock(&set->access);

  stream->set = 0;
  stream->set_index = INVALID_INDEX;

  if(stream->is_obsolete && UnifiedIsEmpty(stream)) {
    pthread_mutex_unlock(&stream->access);

    UnifiedDestroy(stream);
  } else {
    pthread_mutex_unlock(&stream->access);
  }

  return set;
}

snet_tl_streamset_t* 
SNetTlReplaceInStreamset(snet_tl_streamset_t *set,
                         snet_tl_stream_t *old_stream,
                         snet_tl_stream_t *new_stream) 
{

  pthread_mutex_lock(&old_stream->access);
  pthread_mutex_lock(&new_stream->access);

    pthread_mutex_lock(&old_stream->set->access);

  if(new_stream->set != NULL) {

    SNetUtilDebugFatal("SNetTlReplaceInStreamset: Stream already belongs to another set!");
  }

  new_stream->set = old_stream->set;
  old_stream->set = NULL;

  new_stream->set_index = old_stream->set_index;
  old_stream->set_index = INVALID_INDEX;

  new_stream->set->streams[new_stream->set_index] = new_stream;
  new_stream->set->rec_counts[new_stream->set_index] = UnifiedSize(new_stream);

  UnifiedRegisterDispatcher(new_stream, set->record_counter);
  UnifiedUnregisterDispatcher(old_stream, set->record_counter);
  
  pthread_mutex_unlock(&new_stream->set->access);
 
  pthread_mutex_unlock(&new_stream->access);

  if(old_stream->is_obsolete && UnifiedIsEmpty(old_stream)) {
    pthread_mutex_unlock(&old_stream->access);

    UnifiedDestroy(old_stream);
  } else {
    pthread_mutex_unlock(&old_stream->access);
  }

  return set;
}

bool SNetTlGetFlag(snet_tl_stream_t *stream)
{
  bool result = false;

  if(stream != NULL) {

    pthread_mutex_lock(&stream->access);
  
    result = UnifiedGetFlag(stream);
    
    pthread_mutex_unlock(&stream->access);
  }

  return result;
}

bool SNetTlSetFlag(snet_tl_stream_t *stream, bool flag)
{
  bool result = false;

  if(stream != NULL) {

    pthread_mutex_lock(&stream->access);
    
    result = UnifiedSetFlag(stream, flag);
    
    pthread_mutex_unlock(&stream->access);
  }

  return result;
}

snet_record_t *SNetTlRead(snet_tl_stream_t *stream)
{
  snet_record_t *rec;

  pthread_mutex_lock(&stream->access);

  rec = UnifiedRead(stream);

  if(stream->set != NULL) {
    pthread_mutex_lock(&stream->set->access);

    SNetExSemDecrement(stream->set->record_counter);
    
    stream->set->rec_counts[stream->set_index]--;

    pthread_mutex_unlock(&stream->set->access);
  }

  if(stream->is_obsolete && UnifiedIsEmpty(stream) && stream->set == NULL) {
    pthread_mutex_unlock(&stream->access);

    UnifiedDestroy(stream);
  } else {
    pthread_mutex_unlock(&stream->access);
  }

  return rec;
}


snet_tl_stream_record_pair_t
SNetTlReadMany(snet_tl_streamset_t *set)
{
  snet_tl_stream_record_pair_t pair;
  int i;

  pthread_mutex_lock(&set->access);

  SNetExSemDecrement(set->record_counter);

  for(i = 0; i < set->size; i++) {
    if(set->streams[i] != NULL && set->rec_counts[i] > 0) {
      break;
    }
  }

  if(i == set->size) {
    SNetUtilDebugFatal("ReadMany: Invalid stream set index %d (set size = %d)", i, set->size);
  } 

  set->rec_counts[i]--;

  pair.stream = set->streams[i];
  
  pthread_mutex_unlock(&set->access);

  pthread_mutex_lock(&pair.stream->access);

  pair.record = UnifiedRead(pair.stream);

  if(pair.stream->is_obsolete && UnifiedIsEmpty(pair.stream) && pair.stream->set == NULL) {
    pthread_mutex_unlock(&pair.stream->access);

    UnifiedDestroy(pair.stream);
  } else {
    pthread_mutex_unlock(&pair.stream->access);
  }

  return pair;
}

snet_record_t *SNetTlPeek(snet_tl_stream_t *stream)
{
  snet_record_t *rec;

  pthread_mutex_lock(&stream->access);

  rec = UnifiedPeek(stream);

  pthread_mutex_unlock(&stream->access);

  return rec;
}

snet_tl_stream_record_pair_t
SNetTlPeekMany(snet_tl_streamset_t *set)
{
  snet_tl_stream_record_pair_t pair;
  int i;

  pthread_mutex_lock(&set->access);

  SNetExSemWaitWhileMinValue(set->record_counter);

  for(i = 0; i < set->size; i++) {
    if(set->streams[i] != NULL && set->rec_counts[i] > 0) {
      break;
    }
  }

  if(i == set->size) {
    SNetUtilDebugFatal("PeekMany: Invalid stream set index %d (set size = %d)", i, set->size);
  } 

  pair.stream = set->streams[i];

  pthread_mutex_unlock(&set->access);

  pthread_mutex_lock(&pair.stream->access);

  pair.record = UnifiedPeek(pair.stream);

  pthread_mutex_unlock(&pair.stream->access);

  return pair;
}

snet_tl_stream_t*
SNetTlWrite(snet_tl_stream_t *stream, snet_record_t *rec)
{
  pthread_mutex_lock(&stream->access);

  UnifiedWrite(stream, rec);

  if(stream->set != NULL) {
    pthread_mutex_lock(&stream->set->access);

    stream->set->rec_counts[stream->set_index]++;

    SNetExSemIncrement(stream->set->record_counter);

    pthread_mutex_unlock(&stream->set->access);
  }

  pthread_mutex_unlock(&stream->access);

  return stream; 
}

bool SNetTlTryWrite(snet_tl_stream_t *stream, 
		    snet_record_t *data)
{
  SNetUtilDebugFatal("SNetTlTryWrite not yet implemented");

  return false;
}
