#include <stdlib.h>
#include "node.h"

/* Allocate a new stream */
snet_stream_t *SNetStreamCreate(int capacity)
{
  snet_stream_t *stream;

  trace(__func__);
  stream = SNetNewAlign(snet_stream_t);
  STREAM_FROM(stream) = NULL;
  STREAM_DEST(stream) = NULL;

  return stream;
}

/* Free a stream */
void SNetStreamDestroy(snet_stream_t *stream)
{
  trace(__func__);

  stream->from = NULL;
  stream->dest = NULL;
  SNetDelete(stream);
}

/* Enqueue a record to a stream and add a note to the todo list. */
void SNetStreamWrite(snet_stream_desc_t *desc, snet_record_t *rec)
{
  DESC_INCR(desc);
  SNetFifoPut(&desc->fifo, rec);
  assert(desc->source->worker);
  SNetWorkerTodo(desc->source->worker, desc);
}

/* Merge a stream to an identity landing with the subsequent stream. */
static snet_stream_desc_t *SNetMergeStreams(snet_stream_desc_t **desc_ptr)
{
  snet_stream_desc_t    *desc = *desc_ptr;
  snet_stream_desc_t    *next = DESC_LAND_SPEC(desc, identity)->outdesc;
  fifo_node_t           *fifo_tail_start;
  fifo_node_t           *fifo_tail_end;
  fifo_node_t           *node;
  int                    count = 0;

  /* Remove all data from the queue towards the garbage landing. */
  fifo_tail_start = SNetFifoGetTail(&desc->fifo, &fifo_tail_end);

  /* Count the number of data items in the captured list. */
  for (node = fifo_tail_start; node; node = node->next) {
    ++count;
  }

  /* Append the captured list onto the subsequent stream. */
  SNetFifoPutTail(&next->fifo, fifo_tail_start, fifo_tail_end);

  /* Reconnect the source landing of the next landing. */
  next->source = desc->source;

  /* Increase the reference count by the number of added records. */
  AAF(&(next->refs), count);

  /* Report statistics. */
  if (SNetDebug()) {
    printf("%s: collecting %d recs, %d drefs, %d nrefs\n",
            __func__, count, desc->refs, next->refs);
  }

  /* Convert the identity landing into garbage. */
  SNetBecomeGarbage(desc->landing);

  /* Make sure no one ever attempts to write to the dissolved stream. */
  *desc_ptr = next;

  /* Unlock the garbage landing: some worker todo items may still need it. */
  unlock_landing(desc->landing);

  /* Decrease reference count to the garbage collected stream. */
  SNetDescDone(desc);

  /* Return the subsequent stream. */
  return next;
}

/* Enqueue a record to a stream and add a note to the todo list. */
void SNetWrite(snet_stream_desc_t **desc_ptr, snet_record_t *rec, bool last)
{
  snet_stream_desc_t    *desc = *desc_ptr;
  landing_t             *land = desc->landing;
  worker_t              *worker = desc->source->worker;

  /* Test if we can garbage collect this stream together with its landing. */
  if (land->type == LAND_identity && trylock_landing(land, worker)) {
    desc = SNetMergeStreams(desc_ptr);
    land = desc->landing;
  }

  /* Increase the reference count to the destination stream. */
  DESC_INCR(desc);

  /* If this write was the last statement in the caller function and we can
   * lock the destination landing then process the record right away. */
  if (last && land->id == 0 && trylock_landing(land, worker)) {
    /* Make sure we process records in stream FIFO order. */
    worker->continue_rec = (snet_record_t *) SNetFifoPutGet(&desc->fifo, rec);
    assert(worker->continue_rec);
    worker->continue_desc = desc;
  }
  else {
    /* Store the record into the destination stream. */
    SNetFifoPut(&desc->fifo, rec);
    /* Add a todo item to this worker's todo queue. */
    SNetWorkerTodo(worker, desc);
  }
}

/* Dequeue a record and process it. */
void SNetStreamWork(snet_stream_desc_t *desc, worker_t *worker)
{
  snet_record_t *rec    = (snet_record_t *) SNetFifoGet(&desc->fifo);
  landing_t     *land   = desc->landing;

  assert(rec);
  worker->continue_desc = NULL;
  (*land->node->work)(desc, rec);
  if (land->type != LAND_box && land->id == worker->id) {
    unlock_landing(land);
  }
  SNetDescDone(desc);

  while (worker->continue_desc) {
    rec = worker->continue_rec;
    desc = worker->continue_desc;
    land = desc->landing;
    worker->continue_desc = NULL;
    (*land->node->work)(desc, rec);
    if (land->type != LAND_box && land->id == worker->id) {
      unlock_landing(land);
    }
    SNetDescDone(desc);
  }
}

/* Destroy a stream descriptor. */
void SNetStreamClose(snet_stream_desc_t *desc)
{
  trace(__func__);
  assert(desc->refs == 0);
  SNetFifoDone(&desc->fifo);
  SNetDelete(desc);
}

/* Deallocate a stream, but remember it's destination node. */
void SNetStopStream(snet_stream_t *stream, fifo_t *fifo)
{
  SNetFifoPut(fifo, STREAM_DEST(stream));
  SNetStreamDestroy(stream);
}

/* Decrease the reference count of a stream descriptor by one. */
void SNetDescDone(snet_stream_desc_t *desc)
{
  trace(__func__);
  assert(desc->refs > 0);
  if (DESC_DECR(desc) == 0) {
    landing_t *land = desc->landing;
    SNetStreamClose(desc);
    SNetLandingDone(land);
  }
}

/* Let go of multiple references to a stream descriptor. */
void SNetDescRelease(snet_stream_desc_t *desc, int count)
{
  trace(__func__);
  assert(desc->refs >= count);
  if (DESC_DECR_MULTI(desc, count) == 0) {
    landing_t *land = desc->landing;
    SNetStreamClose(desc);
    SNetLandingDone(land);
  }
}

/* Open a descriptor to an output stream.
 *
 * The source landing which opens the stream is determined by parameter 'prev'.
 * The new descriptor holds a pointer to a landing which instantiates
 * the destination node as determined by the stream parameter 'stream'.
 *
 * Special cases:
 * (1) Dispatchers should create an additional landing for a future collector
 * and push this landing onto a stack of future landings.
 * (2) Collectors should retrieve their designated landing from this stack
 * and verify that it is theirs.
 */
snet_stream_desc_t *SNetStreamOpen(
    snet_stream_t *stream,
    snet_stream_desc_t *prev)
{
  snet_stream_desc_t    *desc;

  trace(__func__);

  desc = SNetNewAlign(snet_stream_desc_t);
  DESC_STREAM(desc) = stream;
  desc->source = prev->landing;
  desc->refs = 1;
  SNetFifoInit(&desc->fifo);

  switch (NODE_TYPE(stream->dest)) {

    case NODE_filter:
    case NODE_nameshift:
    case NODE_output:
      desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_siso);
      DESC_LAND_SPEC(desc, siso)->outdesc = NULL;
      break;

    case NODE_box:
      SNetNewBoxLanding(desc, prev);
      break;

    case NODE_parallel:
      SNetNewParallelLanding(desc, prev);
      break;

    case NODE_star:
      SNetNewStarLanding(desc, prev);
      break;

    case NODE_split:
      SNetNewSplitLanding(desc, prev);
      break;

    case NODE_feedback:
      SNetNewFeedbackLanding(desc, prev);
      break;

    case NODE_dripback:
      SNetNewDripBackLanding(desc, prev);
      break;

    case NODE_sync:
      desc->landing = SNetNewLanding(STREAM_DEST(stream), prev, LAND_sync);
      SNetSyncInitDesc(desc, stream);
      break;

    case NODE_zipper:
      /* Init the landing state of a fused sync-star node */
      desc->landing = SNetNewLanding(STREAM_DEST(stream), prev, LAND_zipper);
      DESC_LAND_SPEC(desc, zipper)->outdesc = NULL;
      DESC_LAND_SPEC(desc, zipper)->head = NULL;
      break;

    case NODE_collector:
      /* Collectors receive the previously created landing from the stack */
      desc->landing = SNetPopLanding(prev);
      assert(desc->landing);
      assert(desc->landing->type == LAND_collector);
      assert(DESC_NODE(desc) == STREAM_DEST(stream));
      assert(desc->landing->refs > 0);
      break;

    case NODE_observer:
      desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_observer);
      DESC_LAND_SPEC(desc, observer)->outdesc = NULL;
      DESC_LAND_SPEC(desc, observer)->oid = 0;
      break;

    case NODE_observer2:
      desc->landing = SNetPopLanding(prev);
      assert(desc->landing);
      assert(desc->landing->type == LAND_empty);
      break;

    case NODE_input:
    case NODE_identity:
    case NODE_garbage:
    default:
      desc->landing = NULL;
      assert(0);
      break;
  }

  /* if (SNetVerbose() && SNetNodeGetWorkerCount() <= 1) {
    printf("%s: %s, %s\n", __func__,
           SNetNodeName(desc->landing->node), SNetLandingName(desc->landing));
  } */

  return desc;
}

void *SNetStreamRead(snet_stream_desc_t *sd)
{
  /* impossible */
  assert(0);
  abort();
}

