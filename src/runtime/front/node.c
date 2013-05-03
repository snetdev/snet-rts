#include <assert.h>
#include <stdarg.h>
#include "node.h"
#include "debug.h"

worker_t **snet_workers;
int snet_worker_count;

worker_t **SNetNodeGetWorkers(void)
{
  return snet_workers;
}

int SNetNodeGetWorkerCount(void)
{
  return snet_worker_count;
}

/* Create a new node and connect its in/output streams. */
node_t *SNetNodeNew(
  node_type_t type,
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

  for (i = 0; i < num_ins; ++i) {
    STREAM_DEST(ins[i]) = node;
  }
  for (i = 0; i < num_outs; ++i) {
    STREAM_FROM(outs[i]) = node;
  }

  return node;
}

void SNetNodeStop(worker_t *worker)
{
  fifo_t         fifo;
  node_t        *initial;
  node_t        *node;

  initial = DESC_NODE(worker->input_desc);

  SNetFifoInit(&fifo);
  SNetFifoPut(&fifo, initial);
  while ((node = SNetFifoGet(&fifo)) != NULL) {
    (*node->stop)(node, &fifo);
  }
  SNetFifoDone(&fifo);
}

void *SNetNodeThreadStart(void *arg)
{
  trace(__func__);
  SNetThreadSetSelf(arg);
  SNetWorkerRun((worker_t *) arg);

  return arg;
}

/* Create workers and start them. */
void SNetNodeRun(snet_stream_t *input, snet_info_t *info, snet_stream_t *output)
{
  int            id, num_workers;

  trace(__func__);
  assert(input->dest);
  assert(input->from);
  assert(NODE_TYPE(input->from) == NODE_input);
  assert(output->dest);
  assert(output->from);
  assert(NODE_TYPE(output->dest) == NODE_output);

  /* Allocate array of worker structures. */
  num_workers = SNetThreadingWorkers();
  snet_worker_count = num_workers;
  snet_workers = SNetNewN(num_workers + 1, worker_t*);

  /* Set all workers to NULL to prevent premature stealing. */
  for (id = 0; id <= num_workers; ++id) {
    snet_workers[id] = NULL;
  }

  /* Init global worker data. */
  SNetWorkerInit();

  /* First create workers with new threads. */
  for (id = 2; id <= num_workers; ++id) {
    snet_workers[id] = SNetWorkerCreate(input->from, id, output->dest);
    SNetThreadCreate(SNetNodeThreadStart, snet_workers[id]);
  }
  /* Reuse main thread for worker with ID 1. */
  snet_workers[1] = SNetWorkerCreate(input->from, 1, output->dest);
  SNetNodeThreadStart(snet_workers[1]);
}

void SNetNodeCleanup(void)
{
  int i;

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

const char* SNetNodeName(node_t *node)
{
#define NAME(x) #x
#define NODE(l) node->type == l ? NAME(l)+5 :
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
  NULL;
}

