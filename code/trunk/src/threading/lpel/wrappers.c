#include "lpel.h"
#include "threading.h"

void SNetEntityYield(void) {  LpelTaskYield(); }

snet_stream_t* SNetStreamCreate(int cap) { return LpelStreamCreate(cap); }
snet_stream_desc_t *SNetStreamOpen(snet_stream_t *stream, char mode) { return LpelStreamOpen((lpel_stream_t*)stream, mode); }

void SNetStreamClose(snet_stream_desc_t *sd, int destroy_stream) { LpelStreamClose((lpel_stream_desc_t*)sd, destroy_stream); }
void SNetStreamReplace(snet_stream_desc_t *sd, snet_stream_t *new_stream) { LpelStreamReplace((lpel_stream_desc_t*)sd, (lpel_stream_t*)new_stream); }
snet_stream_t *SNetStreamGet(snet_stream_desc_t *sd) { return (snet_stream_t*)LpelStreamGet((lpel_stream_desc_t*)sd); }
void *SNetStreamRead(snet_stream_desc_t *sd) { return LpelStreamRead((lpel_stream_desc_t*)sd); }
void *SNetStreamPeek(snet_stream_desc_t *sd) { return LpelStreamPeek((lpel_stream_desc_t*)sd); }
void SNetStreamWrite(snet_stream_desc_t *sd, void *item) { LpelStreamWrite((lpel_stream_desc_t*)sd, item); }
int SNetStreamTryWrite(snet_stream_desc_t *sd, void *item) { LpelStreamTryWrite((lpel_stream_desc_t*)sd, item); }
snet_stream_desc_t *SNetStreamPoll(snet_streamset_t *set) { return (snet_stream_desc_t*) LpelStreamPoll((lpel_streamset_t*)set); }
void SNetStreamsetPut(snet_streamset_t *set, snet_stream_desc_t *sd) { LpelStreamsetPut((lpel_streamset_t*)set, (lpel_stream_desc_t*)sd); }
int SNetStreamsetRemove(snet_streamset_t *set, snet_stream_desc_t *sd) { LpelStreamsetRemove((lpel_streamset_t*)set, (lpel_stream_desc_t*)sd); }

snet_stream_iter_t *SNetStreamIterCreate(snet_streamset_t *set) { return (snet_stream_iter_t*)LpelStreamIterCreate((lpel_streamset_t*)set); }
void SNetStreamIterDestroy(snet_stream_iter_t *iter) { LpelStreamIterDestroy((lpel_stream_iter_t*)iter); }
void SNetStreamIterReset(snet_stream_iter_t *iter, snet_streamset_t *set) { LpelStreamIterReset((lpel_stream_iter_t*)iter, (lpel_streamset_t*)set); }
int SNetStreamIterHasNext(snet_stream_iter_t *iter) { return LpelStreamIterHasNext((lpel_stream_iter_t*)iter); }
snet_stream_desc_t *SNetStreamIterNext( snet_stream_iter_t *iter) { return (snet_stream_desc_t*)LpelStreamIterNext((lpel_stream_iter_t*)iter); }
void SNetStreamIterAppend( snet_stream_iter_t *iter, snet_stream_desc_t *node) { LpelStreamIterAppend((lpel_stream_iter_t*)iter, (lpel_stream_desc_t*)node); }
void SNetStreamIterRemove( snet_stream_iter_t *iter) { LpelStreamIterRemove((lpel_stream_iter_t*)iter); }
