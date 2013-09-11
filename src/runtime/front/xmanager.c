/* Accept incoming connections for Distributed S-Net for the Front runtime system. */

#include <string.h>
#include <unistd.h>
#include "node.h"
#include "reference.h"
#include "distribcommon.h"
#include "distribfront.h"
#include "hashtable.h"

/* We index incoming connections based on the sender identification. */
#define CONNECT_KEY(c)  snet_ints_to_key((c)->source_loc, (c)->source_conn)

/* All state for a single incoming connection. */
typedef struct connect_state {
  connect_t             *connect;
  snet_stream_desc_t    *desc;
} connect_state_t;

static node_t            input_manager_node;
static snet_hashtable_t *connections;

void hello(const char *s);

/* Setup data structures for input manager. */
void SNetInputManagerInit(void)
{
  imanager_arg_t        *iarg;

  /* Input manager node. */
  input_manager_node.type =  NODE_imanager;
  input_manager_node.location = SNetDistribGetNodeId();

  /* Create dummy incoming stream: needed for SNetStreamOpen. */
  iarg = NODE_SPEC(&input_manager_node, imanager);
  iarg->input = SNetStreamCreate(0);
  iarg->indesc = SNetNewAlign(snet_stream_desc_t);
  memset(iarg->indesc, 0, sizeof(snet_stream_desc_t));
  iarg->indesc->stream = iarg->input;
  iarg->indesc->stream->dest = &input_manager_node;

  /* Create landing: needed to store stack. */
  iarg->indesc->landing = SNetNewLanding(&input_manager_node, NULL, LAND_imanager);

  /* Create a hashtable to store state for incoming connections. */
  connections = SNetHashtableCreate(0);
}

/* Start input manager thread. */
void SNetInputManagerStart(void)
{
  imanager_arg_t        *iarg = NODE_SPEC(&input_manager_node, imanager);
  worker_t              *worker;

  /* Lock the landing by the worker. */
  worker = SNetWorkerGetInputManager();
  assert(worker != NULL);
  if (!trylock_landing(iarg->indesc->landing, worker)) {
    assert(false);
  }
}

/* Cleanup input manager. */
void SNetInputManagerStop(void)
{
  imanager_arg_t        *iarg = NODE_SPEC(&input_manager_node, imanager);
  uint64_t               key;

  hello(__func__);

  iarg->indesc->landing->refs = 0;
  SNetFreeLanding(iarg->indesc->landing);
  iarg->indesc->landing = NULL;
  SNetDelete(iarg->indesc);
  iarg->indesc = NULL;
  SNetStreamDestroy(iarg->input);
  iarg->input = NULL;

  /* Look for unterminated connections: none should exist. */
  if (SNetHashtableFirstKey(connections, &key)) {
    /* We describe each unfreed connection in detail. */
    do {
      connect_state_t *cs = SNetHashtableGet(connections, key);
      printf("[%s.%d]: unfreed connection: \n"
             "\tfrom %d.%d to %d.%d, cont %d.%d, split %d.%d,\n"
             "\tstream %s to %s, refs %d, landing %s %s, refs %d\n", 
             __func__, SNetDistribGetNodeId(),
             cs->connect->source_loc, cs->connect->source_conn,
             cs->connect->dest_loc, cs->connect->table_index,
             cs->connect->cont_loc, cs->connect->cont_num,
             cs->connect->split_level, cs->connect->dyn_loc,
             SNetNodeName(cs->desc->stream->from),
             SNetNodeName(cs->desc->stream->dest),
             cs->desc->refs,
             SNetLandingName(cs->desc->landing),
             (cs->desc->landing->type == LAND_box) ?
             LAND_NODE_SPEC(cs->desc->landing, box)->boxname :
             "", cs->desc->landing->refs);
    } while (SNetHashtableNextKey(connections, key, &key));
  }

  SNetHashtableDestroy(connections);
}

/* Open a new stream for an incoming connection. */
static snet_stream_desc_t* SNetInputManagerOpenStream(connect_state_t *cs)
{
  imanager_arg_t        *iarg = NODE_SPEC(&input_manager_node, imanager);
  landing_t             *land = iarg->indesc->landing;
  snet_stream_t         *stream = SNetNodeTableIndex(cs->connect->table_index);
  snet_stream_desc_t    *desc;
  landing_t             *remote;
  int                    i;

  assert(SNetStackIsEmpty(&land->stack));
  assert(land->dyn_locs == NULL);

  input_manager_node.subnet_level = STREAM_FROM(stream)->subnet_level;

  if (cs->connect->cont_loc >= 0 && cs->connect->cont_num > 0) {
    input_manager_node.loc_split_level = cs->connect->split_level;
    if (cs->connect->split_level > 0) {
      land->dyn_locs = SNetNewN(cs->connect->split_level, int);
      for (i = 0; i < cs->connect->split_level; ++i) {
        land->dyn_locs[i] = LOCATION_UNKNOWN;
      }
      land->dyn_locs[cs->connect->split_level - 1] = cs->connect->dyn_loc;
    }
    remote = SNetNewLanding(NULL, iarg->indesc, LAND_remote);
    LAND_SPEC(remote, remote)->cont_loc = cs->connect->cont_loc;
    LAND_SPEC(remote, remote)->cont_num = cs->connect->cont_num;
    LAND_SPEC(remote, remote)->split_level = cs->connect->split_level;
    LAND_SPEC(remote, remote)->dyn_loc = cs->connect->dyn_loc;
    LAND_SPEC(remote, remote)->stackfunc = SNetTransferRestore;
    LAND_SPEC(remote, remote)->retfunc = SNetTransferReturn;
    SNetStackPush(&land->stack, remote);
  }
  desc = SNetStreamOpen(stream, iarg->indesc);

  if (SNetDebugDF()) {
    printf("[%s.%d]: open stream %d (%s to %s), landing %s, cont %d,%d.\n",
           __func__, SNetDistribGetNodeId(), cs->connect->table_index,
           SNetNodeName(stream->from), SNetNodeName(stream->dest),
           SNetLandingName(cs->desc->landing),
           cs->connect->cont_loc, cs->connect->cont_num);
  }

  /* Empty stack. */
  while (SNetStackNonEmpty(&land->stack)) {
    landing_t *temp = SNetStackPop(&land->stack);
    SNetLandingDone(temp);
  }
  /* Free existing stack of dynamic locations. */
  if (land->dyn_locs) {
    SNetMemFree(land->dyn_locs);
    land->dyn_locs = NULL;
  }

  return desc;
}

/* Process a connection setup message. */
static void SNetInputManagerNewConnection(connect_t *connect)
{
  uint64_t              key = CONNECT_KEY(connect);
  connect_state_t      *cs;

  if (SNetDebugDF()) {
    printf("[%s.%d]: new con from %d.%d to %d.%d cont %d.%d split %d.%d\n",
           __func__, SNetDistribGetNodeId(),
           connect->source_loc, connect->source_conn,
           connect->dest_loc, connect->table_index,
           connect->cont_loc, connect->cont_num,
           connect->split_level, connect->dyn_loc);
  }

  /* Create a new connection state. */
  cs = SNetNew(connect_state_t);
  /* Copy parameters. */
  cs->connect = connect;
  /* Open descriptor. */
  cs->desc = SNetInputManagerOpenStream(cs);
  /* Store connection state. */
  SNetHashtablePut(connections, key, cs);
}

/* Inject a new record into a stream. */
static void SNetInputManagerForwardRecord(snet_mesg_t *mesg)
{
  uint64_t              key = snet_ints_to_key(mesg->source, mesg->conn);
  connect_state_t      *cs = SNetHashtableGet(connections, key);

  if (!cs) {
    SNetUtilDebugFatal("[%s.%d]: Unknown connection %d, %d\n", __func__,
                       SNetDistribGetNodeId(), mesg->source, mesg->conn);
  } else {
    bool forward = false;

    switch (REC_DESCR(mesg->rec)) {

      case REC_data: forward = true; break;
      case REC_detref: forward = true; break;

      case REC_terminate:
        usleep(100*1000);
        assert(cs->desc->refs >= 1);
        if (SNetDebugDF()) {
          printf("[%s.%d]: REC_terminate, refs = %d.\n", __func__,
                 SNetDistribGetNodeId(), cs->desc->refs);
        }
        SNetDescDone(cs->desc);
        SNetDelete(cs->connect);
        SNetDelete(cs);
        SNetHashtableRemove(connections, key);
        break;

      default:
        SNetUtilDebugFatal("[%s.%d]: Unknown record type %s from %d, %d\n", __func__,
                           SNetDistribGetNodeId(), SNetRecTypeName(mesg->rec),
                           mesg->source, mesg->conn);
    }

    if (forward) {
      if (SNetDebugDF()) {
        printf("[%s.%d]: writing record %s to landing %s\n", __func__,
               SNetDistribGetNodeId(), SNetRecTypeName(mesg->rec),
               SNetLandingName(cs->desc->landing));
      }

      SNetStreamWrite(cs->desc, mesg->rec);
    } else {
      SNetRecDestroy(mesg->rec);
      mesg->rec = NULL;
    }
  }
}

/* Process one input message. */
bool SNetInputManagerDoTask(worker_t *worker)
{
  bool          running = true;
  snet_mesg_t   mesg;

  SNetDistribReceiveMessage(&mesg);

  switch (mesg.type) {

    case SNET_COMM_connect:
      SNetInputManagerNewConnection(mesg.connect);
      break;

    case SNET_COMM_stop:
    case snet_stop:
      running = false;
      break;

    case SNET_COMM_record:
      SNetInputManagerForwardRecord(&mesg);
      break;

    case snet_ref_set:
      SNetRefSet(mesg.ref, (void*) mesg.data);
      SNetMemFree(mesg.ref);
      break;

    case snet_ref_update:
      SNetRefUpdate(mesg.ref, mesg.val);
      SNetMemFree(mesg.ref);
      break;

    case snet_ref_fetch:
      SNetDistribSendData(mesg.ref, SNetRefGetData(mesg.ref), (void*) mesg.data);
      SNetRefUpdate(mesg.ref, -1);
      SNetMemFree(mesg.ref);
      break;

    default:
      SNetUtilDebugFatal("[%s.%d]: Unexpected distribution message %d.",
                         __func__, SNetDistribGetNodeId(), mesg.type);
      break;
  }
  return running;
}

/* Loop forever processing network input. */
void SNetInputManagerRun(worker_t *worker)
{
  SNetInputManagerStart();
  while (SNetInputManagerDoTask(worker) == true) {
    SNetWorkerMaintenaince(worker);
  }
  SNetDistribStop();
  SNetWorkerWait(worker);
}

/* Convert a distributed communication protocol message type to a string. */
const char* SNetCommName(int i)
{
  char *p;
#define NAME(c) #c
#define COMM(c) i == c ? NAME(c) :
  return COMM(SNET_COMM_connect)
         COMM(SNET_COMM_stop)
         COMM(SNET_COMM_record)
         COMM(snet_ref_set)
         COMM(snet_ref_fetch)
         COMM(snet_ref_update)
         COMM(snet_stop)
         (p = SNetMemAlloc(100), snprintf(p, 100, "Unknown comm name %d", i), p);
}

