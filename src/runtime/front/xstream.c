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
  stream->table_index = 0;

  return stream;
}

/* Free a stream */
void SNetStreamDestroy(snet_stream_t *stream)
{
  SNetDelete(stream);
}

/* Dummy function needed for linking with other libraries. */
void *SNetStreamRead(snet_stream_desc_t *sd)
{
  /* impossible */
  assert(0);
  abort();
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

  /* Report garbage collection statistics. */
  if (SNetDebugGC()) {
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

  if (SNetDebugSL()) {
    printf("work %s by %d@%d\n", SNetLandingName(land), worker->id,
                                 SNetDistribGetNodeId());
  }

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

    if (SNetDebugSL()) {
      printf("work %s by %d@%d\n", SNetLandingName(land), worker->id,
                                   SNetDistribGetNodeId());
    }

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
  if (STREAM_DEST(stream)) {
    SNetFifoPut(fifo, STREAM_DEST(stream));
  }
  SNetStreamDestroy(stream);
}

/* Decrease the reference count of a stream descriptor by one. */
void SNetDescDone(snet_stream_desc_t *desc)
{
  trace(__func__);
  assert(desc->refs > 0);
  if (DESC_DECR(desc) == 0) {
    landing_t *land = desc->landing;

    if (SNetDebugSL()) {
      printf("[%s.%d]: Closing stream (%s to %s) to %s %s\n", __func__,
             SNetDistribGetNodeId(), SNetNodeName(desc->stream->from),
             SNetNodeName(desc->stream->dest), SNetLandingName(land),
             (land->type == LAND_box) ? LAND_NODE_SPEC(land, box)->boxname : "");
    }

    SNetStreamClose(desc);
    SNetLandingDone(land);
  }
  else if (SNetDebugSL()) {
    landing_t *land = desc->landing;
    printf("[%s.%d]: Decrementing stream (%s to %s) to %s %s to %d refs\n", __func__,
           SNetDistribGetNodeId(), SNetNodeName(desc->stream->from),
           SNetNodeName(desc->stream->dest), SNetLandingName(land),
           (land->type == LAND_box) ? LAND_NODE_SPEC(land, box)->boxname : "",
           desc->refs);
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

/* Maybe fill in a newly created descriptor structure for Distributed S-Net. */
static bool SNetStreamDistributed(
    snet_stream_desc_t *desc,
    snet_stream_desc_t *prev)
{
  int   location;
  int   delta = DESC_DEST(prev)->subnet_level - DESC_DEST(desc)->subnet_level;

  /* Check for the end of a subnetwork. */
  switch (delta) {
    case 1: {
        /* Peek at the top of the stack of future landings. */
        landing_t *next = SNetStackTop(&prev->landing->stack);
        /* If the top-of-stack is a 'remote' landing then insert a transfer stream. */
        if (next->type == LAND_remote) {
          landing_remote_t *remote = LAND_SPEC(next, remote);
          if (remote->cont_loc == SNetDistribGetNodeId()) {
            /* Restore a previously created continuation on this same location. */
            (*remote->stackfunc)(next, prev);
            return false;
          } else {
            /* Insert a 'transfer' landing to redirect to a remote location. */
            landing_t *discard = SNetPopLanding(prev);
            assert(remote->cont_loc >= 0);
            desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_transfer);
            (*remote->retfunc)(next, desc, prev);
            SNetLandingDone(discard);
            return true;
          }
        }
      }
      break;

    case 0: /*OK*/
      break;

    case -1: /*OK*/
      break;

    default: SNetUtilDebugFatal("[%s.%d]: Invalid delta %d\n", __func__,
                                SNetDistribGetNodeId(), delta);
  }

  /* Copy destination locations for indexed placement from previous landing. */
  if ((location = DESC_DEST(desc)->location) == LOCATION_UNKNOWN) {
    const int split_level = DESC_NODE(prev)->loc_split_level;
    assert(split_level > 0);
    location = prev->landing->dyn_locs[split_level - 1];
    assert(location >= 0);
  }
  /* Redirect streams for remote locations to a transfer landing. */
  if (location != SNetDistribGetNodeId()) {
    desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_transfer);
    SNetTransferOpen(location, desc, prev);
    return true;
  }

  /* No landing has been created yet. */
  return false;
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
 * (3) Each star instance should create the subsequent star incarnation
 * and push this incarnation onto the stack of landings.
 * (4) When a destination landing is part of subnetwork of an
 * indexed placement combinator then the node location must be
 * retrieved from the 'dyn_locs' attribute of the previous landing.
 * (5) When the location is on a different machine then a 'transfer'
 * landing must be created instead which connects to a remote location.
 */
snet_stream_desc_t *SNetStreamOpen(
    snet_stream_t *stream,
    snet_stream_desc_t *prev)
{
  snet_stream_desc_t    *desc;

  trace(__func__);

  /* Create and initialize a new descriptor. */
  desc = SNetNewAlign(snet_stream_desc_t);
  DESC_STREAM(desc) = stream;
  desc->source = prev->landing;
  desc->refs = 1;
  SNetFifoInit(&desc->fifo);

  /* Distributed S-Net inserts extra streams for inter-node record transfer. */
  if (SNetDistribIsDistributed()) {
    if (SNetStreamDistributed(desc, prev)) {
      return desc;
    }
  }

  /* Construct a destination landing, which is node-specific. */
  switch (NODE_TYPE(stream->dest)) {

    case NODE_collector:
      /* Collectors receive the previously created landing from the stack */
      desc->landing = SNetPopLanding(prev);
      assert(desc->landing);
      assert(desc->landing->type == LAND_collector);
      assert(DESC_NODE(desc) == STREAM_DEST(stream));
      assert(desc->landing->refs > 0);
      break;

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

    /* Impossible cases. */
    case NODE_input:
    case NODE_identity:
    case NODE_garbage:
    case NODE_transfer:
    default:
      desc->landing = NULL;
      assert(0);
      break;
  }

  /* if (SNetVerbose() && SNetNodeGetWorkerCount() <= 1) {
    printf("[%s.%d]: %s -> %s\n", __func__, SNetDistribGetNodeId(),
           SNetNodeName(desc->landing->node), SNetLandingName(desc->landing));
  } */

  return desc;
}

