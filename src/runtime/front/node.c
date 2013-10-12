#include "node.h"

/* Keep track of all created streams in a static table.
 * This is used in distributed S-Net when communicating
 * destination streams for inter-location traffic. */
static struct streams_table {
  snet_stream_t  **streams;
  int              size;
  int              last;
} snet_streams_table;

/* Lookup a stream in the streams table. Return index. */
static int SNetNodeTableLookup(snet_stream_t *stream)
{
  int i, found = 0;

  assert(stream);
  for (i = 1; i <= snet_streams_table.last; ++i) {
    snet_stream_t *s = snet_streams_table.streams[i];
    if (s && s->from == stream->from && s->dest == stream->dest) {
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

/* Delete a stream from the stream table. */
void SNetNodeTableRemove(snet_stream_t *stream)
{
  int i;
  for (i = snet_streams_table.last; i >= 1; --i) {
    if (stream == snet_streams_table.streams[i]) {
      snet_streams_table.streams[i] = NULL;
      if (snet_streams_table.last == i) {
        snet_streams_table.last -= 1;
      }
      break;
    }
  }
  if (i < 1) {
    fprintf(stderr, "[%s]: Could not remove table entry (%d).\n", __func__, i);
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

void SNetNodeCleanup(void)
{
  SNetNodeTableCleanup();
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
    case NODE_input:
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

void SNetNodeStop(void)
{
  fifo_t         fifo;
  node_t        *initial;
  node_t        *node;
  snet_stream_t *stream;

  /* if (input_desc) {
    initial = DESC_NODE(input_desc);
  } else */
  stream = SNetNodeTableIndex(1);
  if (STREAM_FROM(stream)) {
    initial = STREAM_FROM(stream);
  } else {
    initial = STREAM_DEST(stream);
  }

  SNetFifoInit(&fifo);
  SNetFifoPut(&fifo, initial);
  while ((node = SNetFifoGet(&fifo)) != NULL) {
    (*node->stop)(node, &fifo);
  }
  SNetFifoDone(&fifo);
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

