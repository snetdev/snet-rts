/*
 * $Id$
 */
#ifndef STREAM_HEADER
#define STREAM_HEADER

typedef struct stream snet_tl_stream_t;
typedef struct streamset snet_tl_streamset_t;
typedef struct component snet_tl_component_t;
#include "record.h"
#include "threading.h" /* entity type */
typedef struct stream_record_pair {
  snet_tl_stream_t *stream;
  snet_record_t *record;
} snet_tl_stream_record_pair_t;

void SNetTlCreateComponent(void* (*func)(void*), void* args, snet_entity_id_t type);
void SMetTlTerminate(snet_tl_component_t *terminated_component);

snet_tl_stream_t *SNetTlCreateStream(int size);
snet_tl_stream_t *SNetTlCreateUnboundedStream();

snet_tl_stream_t *SNetTlMarkObsolete(snet_tl_stream_t *obsoletE_stream);

snet_tl_streamset_t *SNetTlCreateStreamset();
void SNetTlDestroyStreamset(snet_tl_streamset_t *set_to_destroy);

snet_tl_streamset_t*
SNetTlAddToStreamset(snet_tl_streamset_t *stream_destination,
                     snet_tl_stream_t *new_stream);
snet_tl_streamset_t*
SNetTlDeleteFromStreamset(snet_tl_streamset_t *stream_container,
                          snet_tl_stream_t *new_stream);

snet_record_t *SNetTlRead(snet_tl_stream_t *record_source);

snet_tl_stream_record_pair_t
SNetTlReadMany(snet_tl_streamset_t *possible_sources);

snet_record_t *SNetTlPeek(snet_tl_stream_t *record_source);

snet_tl_stream_record_pair_t
SNetTlPeekMany(snet_tl_streamset_t *possible_sources);

snet_tl_stream_t*
SNetTlWrite(snet_tl_stream_t *record_destination, snet_record_t *data);


snet_tl_stream_t*
SNetTlTryWrite(snet_tl_stream_t *record_destination, snet_record_t *data);

snet_tl_streamset_t*
SNetTlReplaceInStreamset(snet_tl_streamset_t *set_to_modify,
                         snet_tl_stream_t *old_stream,
                         snet_tl_stream_t *replacement_stream);

#endif
