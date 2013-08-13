#include <assert.h>
#include <stdarg.h>
#include "node.h"
#include "debug.h"

/* An array with pointers to all workers. */
static worker_t      **snet_workers;
static int             snet_worker_count;

/* Keep track of all created streams in a static table.
 * This is used in distributed S-Net when communicating
 * destination streams for inter-location traffic. */
static struct streams_table {
  snet_stream_t  **streams;
  int              size;
  int              last;
} snet_streams_table;

worker_t **SNetNodeGetWorkers(void)
{
  return snet_workers;
}

int SNetNodeGetWorkerCount(void)
{
  return snet_worker_count;
}

/* Lookup a stream in the streams table. Return index. */
static int SNetNodeTableLookup(snet_stream_t *stream)
{
  int i, found = 0;

  assert(stream);
  for (i = 1; i <= snet_streams_table.last; ++i) {
    if (snet_streams_table.streams[i]->from == stream->from &&
        snet_streams_table.streams[i]->dest == stream->dest) {
      found = i;
      break;
    }
  }
  return found;
}

/* Assign a new index to a stream and add it to the stream table. */
void SNetNodeTableAdd(snet_stream_t *stream)
{
  int found = SNetNodeTableLookup(stream);
  if (found == 0) {
    const int table_index = ++snet_streams_table.last;
    if (table_index >= snet_streams_table.size) {
      const int min_size = 16;
      if (snet_streams_table.size < min_size) {
        snet_streams_table.size = min_size;
      } else {
        /* Increase table size by 50 percent. */
        snet_streams_table.size += snet_streams_table.size / 2;
      }
      snet_streams_table.streams = SNetMemResize( snet_streams_table.streams,
                         snet_streams_table.size * sizeof(snet_stream_t *));
      /* Table starts index one: index zero detects invalid indices. */
      snet_streams_table.streams[0] = NULL;
    }
    snet_streams_table.streams[table_index] = stream;
    /* Also copy the table index to the stream structure. */
    stream->table_index = table_index;

    if (SNetDebug()) {
      printf("[%s.%d]: adding stream %2d from %9s to %9s for location %2d\n",
             __func__, SNetDistribGetNodeId(), table_index,
             stream->from ? SNetNodeName(stream->from) : " ",
             SNetNodeName(stream->dest), stream->dest->location);
    }
  }
}

/* Retrieve the node table index from a node structure. */
snet_stream_t* SNetNodeTableIndex(int table_index)
{
  assert(table_index >= 1 && table_index <= snet_streams_table.last);
  return snet_streams_table.streams[table_index];
}

/* Cleanup the node table */
void SNetNodeTableCleanup(void)
{
  snet_streams_table.last = 0;
  snet_streams_table.size = 0;
  SNetDelete(snet_streams_table.streams);
  snet_streams_table.streams = NULL;
}

/* Create a new node and connect its in/output streams. */
node_t *SNetNodeNew(
  node_type_t type,
  int location,
  snet_stream_t **ins,
  int num_ins,
  snet_stream_t **outs,
  int num_outs,
  node_work_fun_t work,
  node_stop_fun_t stop,
  node_term_fun_t term)
{
  int i;
  node_t *node = SNetNew(node_t);
  node->type = type;
  node->work = work;
  node->stop = stop;
  node->term = term;
  node->location = location;
  node->loc_split_level = SNetLocSplitGetLevel();
  node->subnet_level = SNetSubnetGetLevel();

  /* For all incoming streams: set destination to this node. */
  for (i = 0; i < num_ins; ++i) {
    STREAM_DEST(ins[i]) = node;
  }
  /* For all outgoing streams: set source pointer to this node. */
  for (i = 0; i < num_outs; ++i) {
    STREAM_FROM(outs[i]) = node;
  }

  /* For the common entities: add all incoming streams to the table. */
  switch (type) {
    case NODE_box:
    case NODE_parallel:
    case NODE_star:
    case NODE_split:
    case NODE_feedback:
    case NODE_sync:
    case NODE_filter:
    case NODE_nameshift:
    case NODE_zipper:
    case NODE_dripback:
    case NODE_transfer:
    case NODE_collector:
      for (i = 0; i < num_ins; ++i) {
        SNetNodeTableAdd(ins[i]);
      }
      break;
    default:
      break;
  }

  return node;
}

void SNetNodeStop(worker_t *worker)
{
  fifo_t         fifo;
  node_t        *initial;
  node_t        *node;

  if (worker->input_desc) {
    initial = DESC_NODE(worker->input_desc);
  } else {
    initial = SNetNodeTableIndex(1)->dest;
  }

  SNetFifoInit(&fifo);
  SNetFifoPut(&fifo, initial);
  while ((node = SNetFifoGet(&fifo)) != NULL) {
    (*node->stop)(node, &fifo);
  }
  SNetFifoDone(&fifo);
}

void *SNetNodeThreadStart(void *arg)
{
  worker_t      *worker = (worker_t *) arg;

  trace(__func__);

  SNetThreadSetSelf(worker);
  if (worker->role == InputManager) {
    SNetInputManagerRun(worker);
  } else {
    SNetWorkerRun(worker);
  }

  return arg;
}

/* Create workers and start them. */
void SNetNodeRun(snet_stream_t *input, snet_info_t *info, snet_stream_t *output)
{
  int            id, num_workers, mg, num_managers;

  trace(__func__);
  assert(input->dest);
  assert(output->from);

  /* Allocate array of worker structures. */
  num_workers = SNetThreadingWorkers();
  num_managers = SNetThreadedManagers();
  snet_worker_count = num_workers + num_managers;
  snet_workers = SNetNewN(snet_worker_count + 1, worker_t*);

  /* Set all workers to NULL to prevent premature stealing. */
  for (id = 0; id <= snet_worker_count; ++id) {
    snet_workers[id] = NULL;
  }

  /* Init global worker data. */
  SNetWorkerInit();

  /* Create managers with new threads. */
  for (mg = 1; mg <= num_managers; ++mg) {
    id = mg + num_workers;
    snet_workers[id] = SNetWorkerCreate(input->from, id, output->dest, InputManager);
    SNetThreadCreate(SNetNodeThreadStart, snet_workers[id]);
  }
  /* Create data workers with new threads. */
  for (id = 2; id <= num_workers; ++id) {
    snet_workers[id] = SNetWorkerCreate(input->from, id, output->dest, DataWorker);
    SNetThreadCreate(SNetNodeThreadStart, snet_workers[id]);
  }
  /* Reuse main thread for worker with ID 1. */
  snet_workers[1] = SNetWorkerCreate(input->from, 1, output->dest, DataWorker);
  SNetNodeThreadStart(snet_workers[1]);
}

void SNetNodeCleanup(void)
{
  int i;

  SNetNodeTableCleanup();
  SNetWorkerCleanup();
  for (i = snet_worker_count; i > 0; --i) {
    SNetWorkerDestroy(snet_workers[i]);
  }
  SNetDeleteN(snet_worker_count + 1, snet_workers);
  snet_worker_count = 0;
  snet_workers = NULL;
}

/* Create a new stream emmanating from this node. */
snet_stream_t *SNetNodeStreamCreate(node_t *node)
{
  snet_stream_t *stream = SNetStreamCreate(0);
  STREAM_FROM(stream) = node;
  return stream;
}

const char* SNetNodeTypeName(node_type_t type)
{
#define NAME(x) #x
#define NODE(l) type == l ? NAME(l)+5 :
  return
  NODE(NODE_box)
  NODE(NODE_parallel)
  NODE(NODE_star)
  NODE(NODE_split)
  NODE(NODE_feedback)
  NODE(NODE_sync)
  NODE(NODE_filter)
  NODE(NODE_nameshift)
  NODE(NODE_collector)
  NODE(NODE_input)
  NODE(NODE_output)
  NODE(NODE_detenter)
  NODE(NODE_detleave)
  NODE(NODE_zipper)
  NODE(NODE_identity)
  NODE(NODE_input)
  NODE(NODE_garbage)
  NODE(NODE_observer)
  NODE(NODE_observer2)
  NODE(NODE_dripback)
  NODE(NODE_transfer)
  NULL;
}

const char* SNetNodeName(node_t *node)
{
  return SNetNodeTypeName(node->type);
}

