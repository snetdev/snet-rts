/* Distributed S-Net: Transfer records and landing state to other locations. */

#include <string.h>
#include "node.h"
#include "distribfront.h"
#include "hashtable.h"

/* The frozen state of a landing, which is needed later
 * when an outgoing remote stream returns to the originating location. */
typedef struct continuation {
  int           cont_num;       /* Unique identification */
  snet_stack_t  stack;          /* A stack of landings */
} continuation_t;

/* A transfer node forwards records to a remote location. */
static node_t snet_transfer_node = {
  NODE_transfer,                /* type */
  SNetNodeTransfer,             /* forward incoming records */
  (node_stop_fun_t) NULL,       /* no stream stopping needed */
  SNetTermTransfer,             /* cleanup a transfer landing */
};

/* Store continuations hashed by continuation number. */
static snet_hashtable_t* snet_continuations;

/* A lock for mutual exclusion of the continuations hashtable. */
static lock_t snet_continuations_lock;

/* Combine two signed 32-bits integers into a 64-bit unsigned hash key. */
uint64_t snet_ints_to_key(int i1, int i2)
{
  const uint64_t mask = 0xFFFFFFFF;
  uint64_t key = (((uint64_t) i1 & mask) << 32) | ((uint64_t) i2 & mask);
  return key;
}

/* Initialize storage for continuations. */
void SNetTransferInit(void)
{
  /* Adjust node definition to current location. */
  snet_transfer_node.location = SNetDistribGetNodeId();
  /* Create database of continuations. */
  snet_continuations = SNetHashtableCreate(0);
  /* Initialize continuations lock. */
  LOCK_INIT(snet_continuations_lock);
}

/* Delete storage for continuations. */
void SNetTransferStop(void)
{
  uint64_t key;

  hello(__func__);

  LOCK(snet_continuations_lock);

  /* No more continuation should remain: complain about that do. */
  if (SNetHashtableFirstKey(snet_continuations, &key)) {
    do {
      continuation_t *cont = SNetHashtableGet(snet_continuations, key);
      printf("[%s.%d]: unfreed continuation %d\n",
             __func__, SNetDistribGetNodeId(), cont->cont_num);
    } while (SNetHashtableNextKey(snet_continuations, key, &key));
  }

  /* Free all resources. */
  SNetHashtableDestroy(snet_continuations);
  snet_continuations = NULL;
  UNLOCK(snet_continuations_lock);
  LOCK_DESTROY(snet_continuations_lock);
}

/* Return a unique number for identification of continuations and connections. */
static int SNetTransferUniqueNumber(void)
{
  static int counter;
  return AAF(&counter, 1);
}

/* Preserve the state of the current landing for a future incoming connection. */
static continuation_t* CreateContinuation(snet_stream_desc_t *desc)
{
  /* Create a new continuation. */
  continuation_t        *cont = SNetNew(continuation_t);
  snet_stack_node_t     *node;
  landing_t             *item;
  uint64_t               key;

  /* Assign a unique number to each new continuation. */
  cont->cont_num = SNetTransferUniqueNumber();

  /* Initialize landings stack. */
  SNetStackInit(&cont->stack);

  /* Copy the stack of landings from desc to cont. */
  SNetStackCopy(&cont->stack, &desc->landing->stack);

  /* Increment each landing reference count. */
  STACK_FOR_EACH(&cont->stack, node, item) {
    LAND_INCR(item);
  }

  /* Store the new continuation in the hash table for later referral. */
  key = snet_ints_to_key(SNetDistribGetNodeId(), cont->cont_num);
  LOCK(snet_continuations_lock);
  SNetHashtablePut(snet_continuations, key, cont);
  UNLOCK(snet_continuations_lock);

  return cont;
}

/* Restore landing state from a continuation. */
void SNetTransferRestore(landing_t *land, snet_stream_desc_t *desc)
{
  landing_remote_t      *remote = LAND_SPEC(land, remote);
  uint64_t               key = snet_ints_to_key(remote->cont_loc, remote->cont_num);
  continuation_t        *cont;
  landing_t             *temp;

  /* Take continuation out of the hashtable. */
  LOCK(snet_continuations_lock);
  cont = SNetHashtableRemove(snet_continuations, key);
  UNLOCK(snet_continuations_lock);

  /* Verify that continuation is still meaningful. */
  assert(cont && cont->cont_num == remote->cont_num);
  assert(SNetStackNonEmpty(&cont->stack));

  /* Empty existing descriptor landing stack. */
  STACK_POP_ALL(&desc->landing->stack, temp) {
    SNetLandingDone(temp);
  }

  /* Move the continuation stack to the descriptor landing. */
  SNetStackSwap(&cont->stack, &desc->landing->stack);

  /* Free continuation. */
  SNetDelete(cont);
}

/* Setup a new connection to a remote input manager. */
static void SNetTransferNewConnect(snet_stream_desc_t *desc)
{
  /* Extract the currently executing landing. */
  landing_transfer_t    *transfer = DESC_LAND_SPEC(desc, transfer);
  /* Allocate a new connection structure. */
  connect_t             *connect = SNetNew(connect_t);
  /* Look at the first collector or star incarnation, or get NULL. */
  landing_t             *next = SNetStackTop(&desc->landing->stack);

  connect->source_loc  = SNetDistribGetNodeId();        /* my location */
  connect->source_conn = SNetTransferUniqueNumber();    /* connection ID */
  connect->dest_loc    = transfer->dest_loc;            /* destination */
  connect->table_index = transfer->table_index;         /* stream ID */
  assert(connect->table_index >= 1);                    /* non-zero */

  if (next == NULL) {
    /* If the stack of landings is empty then
     * all future locations must be statically determined. */
    connect->cont_loc = LOCATION_UNKNOWN;
    connect->cont_num = 0;
    connect->split_level = 0;
    connect->dyn_loc  = LOCATION_UNKNOWN;
  }
  else {
    /* The next collector or star incarnation may be on a remote location. */
    if (next->type == LAND_remote) {
      /* Copy information from the 'remote' continuation to this connection. */
      connect->cont_loc = LAND_SPEC(next, remote)->cont_loc;
      connect->cont_num = LAND_SPEC(next, remote)->cont_num;
      connect->split_level = LAND_SPEC(next, remote)->split_level;
      connect->dyn_loc  = LAND_SPEC(next, remote)->dyn_loc;
    }
    else {
      /* Create a new continuation which directs back to the current location. */
      continuation_t *cont = CreateContinuation(desc);
      connect->cont_loc = SNetDistribGetNodeId();
      connect->cont_num = cont->cont_num;
      connect->split_level = transfer->split_level;
      connect->dyn_loc = (transfer->split_level >= 1) ?
                         desc->landing->dyn_locs[transfer->split_level - 1] :
                         LOCATION_UNKNOWN;
    }
  }
  transfer->connect = connect;
  SNetDistribTransmitConnect(connect);
}

/* Transmit a termination record. */
static void SNetTransferDisconnect(connect_t *connect)
{
  snet_record_t *rec = SNetRecCreate( REC_terminate);
  SNetDistribTransmitRecord(rec, connect);
  SNetMemFree(connect);
}

/* Forward a record to a different node. */
void SNetNodeTransfer(snet_stream_desc_t *desc, snet_record_t *rec)
{
  landing_transfer_t    *transfer = DESC_LAND_SPEC(desc, transfer);

  if (transfer->connect == NULL) {
    /* Setup the connection. */
    SNetTransferNewConnect(desc);
  }
  switch (REC_DESCR(rec)) {

    case REC_data:
    case REC_detref:
      SNetDistribTransmitRecord(rec, transfer->connect);
      break;

    default:
      SNetUtilDebugFatal("[%s.%d]: unrecognized record type %s\n", __func__,
                         SNetDistribGetNodeId(), SNetRecTypeName(rec));
  }
}

/* Terminate a landing of type 'transfer'. */
void SNetTermTransfer(landing_t *land, fifo_t *fifo)
{
  hello(__func__);
  assert(land->type == LAND_transfer);
  /* tell remote party to cleanup */
  SNetTransferDisconnect(LAND_SPEC(land, transfer)->connect);
  /* unusual: free stream */
  SNetStreamDestroy(LAND_SPEC(land, transfer)->stream);
  SNetFreeLanding(land);
}

/* Initialize a transfer landing, which connects to a remote location. */
snet_stream_desc_t *SNetTransferOpen(
    int destination_location,
    snet_stream_desc_t *desc,
    snet_stream_desc_t *prev)
{
  landing_transfer_t    *transfer = DESC_LAND_SPEC(desc, transfer);

  hello(__func__);

  /* Fill in the transfer landing specific details. */
  transfer->dest_loc = destination_location;
  transfer->table_index = desc->stream->table_index;
  transfer->split_level = DESC_DEST(desc)->loc_split_level;

  /* The network connection must still be setup. */
  transfer->connect = NULL;
  assert(transfer->table_index >= 1);

  /* Create a new outgoing stream. */
  transfer->stream = SNetStreamCreate(0);
  STREAM_FROM(transfer->stream) = DESC_NODE(prev);
  STREAM_DEST(transfer->stream) = &snet_transfer_node;

  /* Bind descriptor to new stream. */
  DESC_STREAM(desc) = transfer->stream;

  /* Correct node type. */
  DESC_NODE(desc) = &snet_transfer_node;

  return desc;
}

/* Setup a transfer landing to connect back to a remote continuation. */
void SNetTransferReturn(
    landing_t *next,
    snet_stream_desc_t *desc,
    snet_stream_desc_t *prev)
{
  landing_remote_t      *remote = LAND_SPEC(next, remote);
  landing_transfer_t    *transfer = DESC_LAND_SPEC(desc, transfer);

  hello(__func__);
  SNetTransferOpen(remote->cont_loc, desc, prev);

  transfer->connect = SNetNew(connect_t);
  transfer->connect->source_loc  = SNetDistribGetNodeId();
  transfer->connect->source_conn = SNetTransferUniqueNumber();
  transfer->connect->dest_loc    = remote->cont_loc;
  transfer->connect->table_index = transfer->table_index;
  transfer->connect->cont_loc    = remote->cont_loc;
  transfer->connect->cont_num    = remote->cont_num;
  transfer->connect->split_level = remote->split_level;
  transfer->connect->dyn_loc     = remote->dyn_loc;
  SNetDistribTransmitConnect(transfer->connect);
}


