/*
 * $Id$
 */

#include "semaphore.h"
#include "list.h"
#include "feedbackbuffer.h"
#include "buffer.h"
#include "debug.h"
#include "memfun.h"


#include "pthread.h"

//#define STREAM_LAYER_DEBUG

struct streamset {
  pthread_mutex_t *access;

  snet_ex_sem_t *record_counter;

  snet_util_list_t *streams;
};

struct stream {
  pthread_mutex_t *access;

  union buffers {
    snet_buffer_t *buffer;
    snet_fbckbuffer_t *feedback_buffer;
  } buffers;
  bool is_unbounded;
  bool is_obsolete;

  bool is_in_streamset;
  snet_tl_streamset_t *containing_streamset;
};

static void Lock(snet_tl_stream_t *target) {
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(CALLINFO (STREAM %p) (Lock (stream  %p)))",
                      target, target);
  SNetUtilDebugNotice("Stream mutex: %p", target->access);
  #endif
  pthread_mutex_lock(target->access);
}

static void Unlock(snet_tl_stream_t *target) {
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(CALLINFO (STREAM %p) (Unlock (stream %p)))",
                      target, target);
  #endif
  pthread_mutex_unlock(target->access);
}

static void LockStreamset(snet_tl_streamset_t *target) {
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(CALLINFO (STREAMSET %p) (LockStreamset (stream %p)))",
                      target, target);
  #endif
  pthread_mutex_lock(target->access);
}
static void UnlockStreamset(snet_tl_streamset_t *target) {
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(CALLINFO (STREAMSET %p) (UnlockStreamset (stream %p)))",
                      target, target);
  #endif
  pthread_mutex_unlock(target->access);
}

static snet_record_t *UnifiedPeek(snet_tl_stream_t *record_source)
{
  //snet_buffer_msg_t buffer_message;
  //snet_fbckbuffer_msg_t feedback_message;

  snet_record_t *result = NULL;
  if(record_source->is_unbounded) {
    result = SNetFbckBufShow(record_source->buffers.feedback_buffer);
  } else {
    result = SNetBufShow(record_source->buffers.buffer);
  }
  return result;
}

static snet_record_t *UnifiedRead(snet_tl_stream_t *record_source)
{
  #ifdef STREAM_LAYER_DEBUG
  char record_message[100];
  #endif
  snet_record_t *result;
  if(record_source->is_unbounded) {
    result = SNetFbckBufGet(record_source->buffers.feedback_buffer);
  } else {
    result = SNetBufGet(record_source->buffers.buffer);
  }

  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugDumpRecord(result, record_message);
  SNetUtilDebugNotice("STREAMLAYER: UnifiedRead(%p) = %s",
                      record_source,
                      record_message);
  #endif
  return result;
}

static bool UnifiedIsEmpty(snet_tl_stream_t *record_source)
{
  bool result;
  if(record_source->is_unbounded) {
    result = SNetFbckBufIsEmpty(record_source->buffers.feedback_buffer);
  } else {
    result = SNetBufIsEmpty(record_source->buffers.buffer);
  }
  return result;
}

static bool UnifiedWrite(snet_tl_stream_t *record_destination,
                            snet_record_t *data)
{
  bool record_written = true;
  //snet_buffer_msg_t buffer_status;
  if(record_destination->is_obsolete) {
    SNetUtilDebugFatal("Writing to write record %p to obsolete stream %p!",
                       data,
                       record_destination);
  }
  if(record_destination->is_unbounded) {
    SNetFbckBufPut(record_destination->buffers.feedback_buffer, data);
    record_written = true;
  } else {
    
      SNetBufPut(record_destination->buffers.buffer,
                    data);
    
  }
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("UnifiedTryWrite(destination=%p, data=%p) = %s",
                      record_destination,
                      data,
                      record_written ? "true" : "false");
  #endif
  return record_written;
}

static void UnifiedDestroy(snet_tl_stream_t *stream_to_destroy)
{
  if(stream_to_destroy == NULL) {
    SNetUtilDebugFatal("Cannot destroy NULL-stream!");
  }
#ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("destroying %p", stream_to_destroy);
#endif
  if(!stream_to_destroy->is_obsolete) {
    SNetUtilDebugFatal("Stream to destroy must be marked obsolete!");
  }
  if(stream_to_destroy->is_unbounded) {
    SNetFbckBufDestroy(stream_to_destroy->buffers.feedback_buffer);
  } else {
    SNetBufDestroy(stream_to_destroy->buffers.buffer);
  }
  pthread_mutex_destroy(stream_to_destroy->access);
  SNetMemFree(stream_to_destroy->access);
  SNetMemFree(stream_to_destroy);
}

static snet_tl_stream_t *UnifiedRegisterDispatcher(snet_tl_stream_t *stream,
                                                  snet_ex_sem_t *dispatcher)
{
  if(stream->is_unbounded) {
    /*SNetFbckBufRegisterDispatcher(stream->buffers.feedbackbuffer, dispatcher);*/
  } else {
    SNetBufRegisterDispatcher(stream->buffers.buffer, dispatcher);
  }
  return stream;
}

void
SNetTlCreateComponent(void* (*func)(void*),
                      void *args,
                      snet_entity_id_t type)
{
  SNetThreadCreate( (void*)func, (void*)args, type);
#ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("CREATE COMPONENT DONE");
#endif
}

void SNetTlTerminate(snet_tl_component_t *terminated_component)
{
  /* empty right now */
}

snet_tl_stream_t *SNetTlCreateStream(int size)
{
  snet_tl_stream_t *result;

  if(size <= 0) {
    SNetUtilDebugFatal("CreateStream: size must be >= 0 and not %d", size);
  }
  result = SNetMemAlloc(sizeof(snet_tl_stream_t));
  result->access = SNetMemAlloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(result->access, NULL);

#ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(CREATION (STREAM %p) (mutex %p))",
                      result, result->access);
#endif

  result->is_obsolete = false;
  result->is_unbounded = false;
  result->buffers.buffer = SNetBufCreate(size, result->access);

  result->is_in_streamset = false;
  return result;
}

snet_tl_stream_t *SNetTlCreateUnboundedStream()
{
  snet_tl_stream_t *result;
  result = SNetMemAlloc(sizeof(snet_tl_stream_t));
  result->access = SNetMemAlloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(result->access, NULL);

  result->is_obsolete = false;
  result->is_unbounded = true;
  result->buffers.feedback_buffer = SNetFbckBufCreate();
  result->is_in_streamset = false;
  return result;
}


snet_tl_stream_t *SNetTlMarkObsolete(snet_tl_stream_t *obsolete_stream)
{
  snet_tl_streamset_t *set = NULL;

  if(obsolete_stream == NULL) {
    SNetUtilDebugFatal("cannot mark NULL obsolete!");
  }
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("%p  marked obsolete!", obsolete_stream);
  #endif
  Lock(obsolete_stream);

  obsolete_stream->is_obsolete = true;
  if(UnifiedIsEmpty(obsolete_stream)) {
    if(obsolete_stream->is_in_streamset) {
      set = obsolete_stream->containing_streamset;
      Unlock(obsolete_stream);
      SNetTlDeleteFromStreamset(set, obsolete_stream);
    } else {
      Unlock(obsolete_stream);
    }
    UnifiedDestroy(obsolete_stream);
    return NULL;
  } else {
    Unlock(obsolete_stream);
    return obsolete_stream;
  }
}

snet_tl_streamset_t *SNetTlCreateStreamset()
{
  snet_tl_streamset_t *result;

  result = SNetMemAlloc(sizeof(snet_tl_streamset_t));
  result->access = SNetMemAlloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(result->access, NULL);

   result->record_counter = SNetExSemCreate(result->access,
                                           true, 0,
                                           false, -1,
                                           0)
;
  result->streams = SNetUtilListCreate();
  return result;
}

snet_tl_streamset_t* 
SNetTlReplaceInStreamset(snet_tl_streamset_t *set_to_modify,
                         snet_tl_stream_t *old_stream,
                         snet_tl_stream_t *replacement_stream) {
  snet_util_list_iter_t *current_position;
  snet_tl_stream_t *current_stream;

  if(set_to_modify == NULL) {
    SNetUtilDebugFatal("ReplaceInStreamset: set_to_modify == NULL");
  }
  if(old_stream == NULL) {
    SNetUtilDebugFatal("ReplaceInStreamset: old_stream == NULL");
  }
  if(replacement_stream == NULL) {
    SNetUtilDebugFatal("ReplaceInStreamset: replacement_stream == NULL");
  }
  LockStreamset(set_to_modify);
  Lock(replacement_stream);

  /* name is not good: basically, this increments the record_counter 
   * semaphore by the amount of records currently in the stream
   */
  UnifiedRegisterDispatcher(replacement_stream, set_to_modify->record_counter);

  replacement_stream->is_in_streamset = true;
  replacement_stream->containing_streamset = set_to_modify;

  Unlock(replacement_stream);

  current_position = SNetUtilListFirst(set_to_modify->streams);
  while(SNetUtilListIterCurrentDefined(current_position)) {
    current_stream = SNetUtilListIterGet(current_position);
    if(current_stream == old_stream) {
      set_to_modify->streams = SNetUtilListIterSet(current_position, 
                                          replacement_stream);
      break;
    }
    current_position = SNetUtilListIterNext(current_position);
  }
  UnlockStreamset(set_to_modify);
  return set_to_modify;
}

void SNetTlDestroyStreamset(snet_tl_streamset_t *set_to_destroy)
{
  pthread_mutex_destroy(set_to_destroy->access);
  SNetMemFree(set_to_destroy->access);
  SNetUtilListDestroy(set_to_destroy->streams);
  SNetExSemDestroy(set_to_destroy->record_counter);
  SNetMemFree(set_to_destroy);
}

snet_tl_streamset_t*
SNetTlAddToStreamset(snet_tl_streamset_t *stream_destination,
                     snet_tl_stream_t *new_stream)
{
  if(stream_destination == NULL) {
    SNetUtilDebugFatal("Cannot add %p to Streamset NULL",
		       new_stream);
  }
  if(new_stream == NULL) {
    SNetUtilDebugFatal("Cannot add stream NULL to streamset %p",
		       stream_destination);
  }
  
  LockStreamset(stream_destination);
  Lock(new_stream);
  /* increment so many times, see above */
  UnifiedRegisterDispatcher(new_stream, stream_destination->record_counter);

  new_stream->is_in_streamset = true;
  new_stream->containing_streamset = stream_destination;

  SNetUtilListAddBeginning(stream_destination->streams, new_stream);
  Unlock(new_stream);
  UnlockStreamset(stream_destination);
  return stream_destination;
}

snet_tl_streamset_t*
SNetTlDeleteFromStreamset(snet_tl_streamset_t *stream_container,
                          snet_tl_stream_t *stream_to_remove)
{
  snet_util_list_iter_t *current_position;
  snet_tl_stream_t *current_stream;

  LockStreamset(stream_container);
  for(current_position = SNetUtilListFirst(stream_container->streams);
      SNetUtilListIterCurrentDefined(current_position);
      current_position = SNetUtilListIterNext(current_position)) {
      current_stream = (snet_tl_stream_t*)SNetUtilListIterGet(current_position);
      if(current_stream == stream_to_remove) {
        Lock(current_stream);
        current_stream->is_in_streamset = false;
        Unlock(current_stream);
        stream_container->streams = SNetUtilListIterDelete(current_position);
        break;
      }
  }
  SNetUtilListIterDestroy(current_position);
  UnlockStreamset(stream_container);
  return stream_container;
}

snet_record_t *SNetTlRead(snet_tl_stream_t *record_source)
{
  snet_record_t *result;
#ifdef STREAM_LAYER_DEBUG
  char message[100];
#endif
  if(record_source == NULL) {
    SNetUtilDebugFatal("SnetTLRead(NULL)!");
  }
#ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("Read(%p)", record_source);
#endif
  if(record_source->is_in_streamset) {
    LockStreamset(record_source->containing_streamset);
  }

  Lock(record_source);
  result = UnifiedRead(record_source);

  if(record_source->is_in_streamset) {
#ifdef STREAM_LAYER_DEBUG
    SNetUtilDebugNotice("(Streamset %p) At entry of Read many: semaphore has value %d",
                        record_source->containing_streamset,
                        SNetExSemGetValue(record_source->containing_streamset->record_counter));
#endif
    SNetExSemDecrement(record_source->containing_streamset->record_counter);
  }

  if(result == NULL) {
    SNetUtilDebugFatal("Scheduler invariant broken:"
                          " Could not read data after waiting");
  }
 
  if(record_source->is_obsolete && UnifiedIsEmpty(record_source)) {
    if(record_source->is_in_streamset) {
      Unlock(record_source);
      UnlockStreamset(record_source->containing_streamset);
      SNetTlDeleteFromStreamset(record_source->containing_streamset,
				record_source);
    }else {
      Unlock(record_source);
    }
    UnifiedDestroy(record_source);
  }else {
    Unlock(record_source);
    if(record_source->is_in_streamset) {
      UnlockStreamset(record_source->containing_streamset);
    }
  }

#ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugDumpRecord(result, message);
  SNetUtilDebugNotice("Read(%p) = %s",
                      record_source,
                      message);
#endif
  return result;
}

static snet_tl_stream_record_pair_t
ReadAny(snet_tl_streamset_t *possible_sources, bool remove_record)
{
  snet_tl_stream_record_pair_t result;
  snet_util_list_iter_t *current_position;
  snet_tl_stream_t *current_stream;
  snet_record_t *read_record;
  result.stream = NULL;
  result.record = NULL;

#ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(STATEINFO (STREAMSET %p)"
                      "(ReadAny (possible sources %p) (remove_record %s))"
                      "(at entry) (length of possible_sources->streams %d)",
                      possible_sources,
                      possible_sources, remove_record?"true":"false",
                      SNetUtilListCount(possible_sources->streams));
  SNetUtilListDump(possible_sources->streams);
#endif /* STREAM_LAYER_DEBUG */
  for(current_position = SNetUtilListFirst(possible_sources->streams);
      SNetUtilListIterCurrentDefined(current_position);
      current_position = SNetUtilListIterNext(current_position)) {
    current_stream = SNetUtilListIterGet(current_position);
    Lock(current_stream);
    read_record = UnifiedPeek(current_stream);
#ifdef STREAM_LAYER_DEBUG
    SNetUtilDebugNotice("(STATEINFO (STREAMSET %p) "
                        "(ReadAny (possible sources %p) (remove_record %s))"
                        "(in the loop in ReadAny)"
                        "((current_stream %p) (read_record %p)))",
                        possible_sources,
                        possible_sources,
                        remove_record ? "true" : "false",
                        current_stream, read_record);
#endif /* STREAM_LAYER_DEBUG */
    if(read_record != NULL) {
      if(remove_record) {
        UnifiedRead(current_stream);
      }
      possible_sources->streams = SNetUtilListIterMoveToBack(current_position);
      result.stream = current_stream;
      result.record = read_record;
      Unlock(current_stream);
      break;
    } else {
      Unlock(current_stream);
    }
  }
  SNetUtilListIterDestroy(current_position);
  return result;
}

snet_tl_stream_record_pair_t
SNetTlReadMany(snet_tl_streamset_t *possible_sources)
{
  snet_tl_stream_record_pair_t result;
  #ifdef STREAM_LAYER_DEBUG
  char record_representation[100];
  #endif

  if(possible_sources == NULL) {
    SNetUtilDebugFatal("Cant read from NULL!");
  }
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("ReadMany(%p)", possible_sources);
  #endif
  LockStreamset(possible_sources);
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(streamset %p) At entry of Read many: semaphore has value %d",
                      possible_sources,
                      SNetExSemGetValue(possible_sources->record_counter));
  #endif
  SNetExSemDecrement(possible_sources->record_counter);
  result = ReadAny(possible_sources, true);
  UnlockStreamset(possible_sources);

  if(result.record == NULL) {
    LockStreamset(possible_sources);
    SNetUtilDebugNotice("STREAMLAYER: Invariant broken: No data after waiting");
    SNetUtilDebugNotice("record count value in streamset: %d",
                         SNetExSemGetValue(possible_sources->record_counter));
    UnlockStreamset(possible_sources);
    SNetUtilDebugFatal("");
  }
  
  #ifdef  STREAM_LAYER_DEBUG
  SNetUtilDebugDumpRecord(result.record, record_representation);
  SNetUtilDebugNotice("ReadMany(%p) = (%p, %s)",
                      possible_sources,
                      result.stream, record_representation);
  #endif

  return result;
}

snet_record_t *SNetTlPeek(snet_tl_stream_t *record_source)
{
  snet_record_t *result;
  #ifdef STREAM_LAYER_DEBUG
  char record_representation[100];
  SNetUtilDebugNotice("Peek(%p)", record_source);
  #endif

  Lock(record_source);
  result = UnifiedPeek(record_source);
  Unlock(record_source);

  #ifdef  STREAM_LAYER_DEBUG
  SNetUtilDebugDumpRecord(result, record_representation);
  SNetUtilDebugNotice("Peek(%p) = %s",
                      record_source,
                      record_representation);
  #endif
  return result;
}

snet_tl_stream_record_pair_t
SNetTlPeekMany(snet_tl_streamset_t *possible_sources)
{
  snet_tl_stream_record_pair_t result;
  #ifdef STREAM_LAYER_DEBUG
  char record_string[100];
  #endif

  if(possible_sources == NULL) {
    SNetUtilDebugFatal("Cant read from NULL!");
  }

  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(CALLINFO (PeekMany (possible_sources %p)))", 
                      possible_sources);
  #endif
  LockStreamset(possible_sources);
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("(streamset %p) At entry of Peek Many: semaphore has value %d",
                      possible_sources,
                      SNetExSemGetValue(possible_sources->record_counter));
  #endif
  SNetExSemWaitWhileMinValue(possible_sources->record_counter);
  result = ReadAny(possible_sources, false);
  UnlockStreamset(possible_sources);

  if(result.record == NULL) {
    LockStreamset(possible_sources);
    SNetUtilDebugNotice("STREAMLAYER: Invariant broken: No data after waiting");
    SNetUtilDebugNotice("record count value in streamset: %d",
                         SNetExSemGetValue(possible_sources->record_counter));
    UnlockStreamset(possible_sources);
    SNetUtilDebugFatal("");
  }


  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugDumpRecord(result.record, record_string);
  SNetUtilDebugNotice("Peekmany(%p) = (%p, %s)",
                      possible_sources,
                      result.stream,
                      record_string);
  #endif
  return result;
}

snet_tl_stream_t*
SNetTlWrite(snet_tl_stream_t *record_destination, snet_record_t *data)
{
  bool record_written;

  #ifdef STREAM_LAYER_DEBUG
  char message_space[100];
  SNetUtilDebugDumpRecord(data, message_space);
  SNetUtilDebugNotice("Write(stream=%p,record=%s)", 
                      record_destination,
                      message_space);
  #endif
  if(record_destination->is_in_streamset) {
    LockStreamset(record_destination->containing_streamset);
  }

  Lock(record_destination);
  record_written = UnifiedWrite(record_destination, data);

  if(record_destination->is_in_streamset) {
    #ifdef STREAM_LAYER_DEBUG
    SNetUtilDebugNotice("(Streamset %p) At entry of Write: semaphore has value %d",
                        
                        record_destination->containing_streamset,
                        SNetExSemGetValue(record_destination->containing_streamset->
                                          record_counter));
    #endif
    SNetExSemIncrement(record_destination->containing_streamset->record_counter);
  }

  if(!record_written) {
    SNetUtilDebugFatal("Scheduler invariant broken:"
                       " Could not write record after waiting for space!");
  }
  #ifdef STREAM_LAYER_DEBUG
  SNetUtilDebugNotice("Write(%p, %p) exited",
                      record_destination,
                      data);
  #endif
  Unlock(record_destination);

  if(record_destination->is_in_streamset) {
    UnlockStreamset(record_destination->containing_streamset);
  }

  return record_destination;
}

/* TODO */
snet_tl_stream_t*
SNetTlTryWrite(snet_tl_stream_t *record_destination, snet_record_t *data)
{
  bool record_written;
 
  #ifdef STREAM_LAYER_DEBUG
  char message_space[100];

  SNetUtilDebugDumpRecord(data, message_space);
  SNetUtilDebugNotice("(CALLINFO (STREAM %p) (TryWrite (stream %p) (record %s)))",
                      record_destination, 
                      record_destination, message_space);
  #endif

  if(record_destination->is_in_streamset) {
    LockStreamset(record_destination->containing_streamset);
  }

  Lock(record_destination);

  record_written = UnifiedWrite(record_destination, data);

  if(record_destination->is_in_streamset) {
    SNetExSemIncrement(record_destination->containing_streamset->record_counter);
  }

  Unlock(record_destination);

  if(record_destination->is_in_streamset) {
    UnlockStreamset(record_destination->containing_streamset);
  }

  if(record_written) {
    return record_destination;
  } else {
    return NULL;
  }
}
