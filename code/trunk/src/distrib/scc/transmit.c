#include <assert.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "debug.h"
#include "dest.h"
#include "memfun.h"
#include "imanager.h"
#include "iomanagers.h"
#include "interface_functions.h"
#include "omanager.h"
#include "record.h"
#include "reference.h"
#include "scc.h"

#include "SCC_API.h"

static t_vcharp sendMPB;
static t_vcharp recvMPB;

static void write_pid(void)
{
    FILE *file = fopen("/sys/module/async_scc/parameters/pid", "w");
    if (file == NULL) {
        perror("Could not open module parameter");
        exit(EXIT_FAILURE);
    }
    fprintf(file, "%d", getpid());
    fclose(file);
}

void SCCPackInt(int count, int *src)
{
  cpy_mem_to_mpb(sendMPB, src, count * sizeof(int));
}

void SCCUnpackInt(int count, int *dst)
{
  cpy_mpb_to_mem(recvMPB, dst, count * sizeof(int));
}

void SCCPackRef(int count, snet_ref_t **src)
{
  int i;
  for (i = 0; i < count; i++) {
    cpy_mem_to_mpb(sendMPB, src[i], sizeof(snet_ref_t));
    SNetRefOutgoing(src[i]);
  }
}

void SCCUnpackRef(int count, snet_ref_t **dst)
{
  int i;
  snet_ref_t ref;
  for (i = 0; i < count; i++) {
    cpy_mpb_to_mem(recvMPB, &ref, sizeof(snet_ref_t));
    dst[i] = SNetRefIncoming(&ref);
  }
}

snet_msg_t SNetDistribRecvMsg(void)
{
  snet_msg_t result;
  int sig, start, end;
  static int pid = 0;
  static sigset_t sigmask;
  static bool handling = true;

  if (!pid) {
    pid = 1;
    write_pid();
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGUSR1);
  }

  recvMPB = mpbs[node_location];

start:
  if (!handling) sigwait(&sigmask, &sig);
  lock(node_location);

  flush();
  start = START(recvMPB);
  end = END(recvMPB);

  if (start == end) {
    handling = false;
    HANDLING(recvMPB) = 0;
    FOOL_WRITE_COMBINE;
    unlock(node_location);
    goto start;
  } else {
    handling = true;
    HANDLING(recvMPB) = 1;
    FOOL_WRITE_COMBINE;
  }

  cpy_mpb_to_mem(recvMPB, &result.type, sizeof(snet_comm_type_t));
  switch (result.type) {
    case snet_rec:
    case snet_block:
    case snet_unblock:
      result.dest = SNetMemAlloc(sizeof(snet_dest_t));
      cpy_mpb_to_mem(recvMPB, result.dest, sizeof(snet_dest_t));
      if (result.type == snet_rec) {
        result.rec = SNetRecDeserialise(&SCCUnpackInt, &SCCUnpackRef);
      }
      break;
    case snet_ref_set:
      cpy_mpb_to_mem(recvMPB, &result.ref, sizeof(snet_ref_t));
      result.data = SNetInterfaceGet(result.ref.interface)->unpackfun(localSpace);
      break;
    case snet_ref_fetch:
      cpy_mpb_to_mem(recvMPB, &result.ref, sizeof(snet_ref_t));
      cpy_mpb_to_mem(recvMPB, &result.val, sizeof(int));
      break;
    case snet_ref_update:
      cpy_mpb_to_mem(recvMPB, &result.ref, sizeof(snet_ref_t));
      cpy_mpb_to_mem(recvMPB, &result.val, sizeof(int));
      break;
    default:
      break;
  }

  unlock(node_location);
  return result;
}

void SNetDistribSendRecord(snet_dest_t *dest, snet_record_t *rec)
{
  snet_comm_type_t type = snet_rec;
  int node = dest->node;
  sendMPB = mpbs[node];

  start_write_node(node);
  cpy_mem_to_mpb(sendMPB, &type, sizeof(snet_comm_type_t));

  dest->node = node_location;
  cpy_mem_to_mpb(sendMPB, dest, sizeof(snet_dest_t));
  dest->node = node;

  SNetRecSerialise(rec, &SCCPackInt, &SCCPackRef);
  stop_write_node(node);
}

void SNetDistribFetchRef(snet_ref_t *ref)
{
  snet_comm_type_t type = snet_ref_fetch;
  start_write_node(ref->node);
  cpy_mem_to_mpb(mpbs[ref->node], &type, sizeof(snet_comm_type_t));
  cpy_mem_to_mpb(mpbs[ref->node], ref, sizeof(snet_ref_t));
  cpy_mem_to_mpb(mpbs[ref->node], &node_location, sizeof(int));
  stop_write_node(ref->node);
}

void SNetDistribUpdateRef(snet_ref_t *ref, int count)
{
  snet_comm_type_t type = snet_ref_update;
  start_write_node(ref->node);
  cpy_mem_to_mpb(mpbs[ref->node], &type, sizeof(snet_comm_type_t));
  cpy_mem_to_mpb(mpbs[ref->node], ref, sizeof(snet_ref_t));
  cpy_mem_to_mpb(mpbs[ref->node], &count, sizeof(int));
  stop_write_node(ref->node);
}

void SNetDistribUpdateBlocked(void)
{
  snet_comm_type_t type = snet_update;
  start_write_node(node_location);
  cpy_mem_to_mpb(mpbs[node_location], &type, sizeof(int));
  stop_write_node(node_location);
}

void SNetDistribUnblockDest(snet_dest_t *dest)
{
  snet_comm_type_t type = snet_unblock;
  start_write_node(dest->node);
  cpy_mem_to_mpb(mpbs[dest->node], &type, sizeof(snet_comm_type_t));
  cpy_mem_to_mpb(mpbs[dest->node], dest, sizeof(snet_dest_t));
  stop_write_node(dest->node);
}

void SNetDistribBlockDest(snet_dest_t *dest)
{
  snet_comm_type_t type = snet_block;
  start_write_node(dest->node);
  cpy_mem_to_mpb(mpbs[dest->node], &type, sizeof(snet_comm_type_t));
  cpy_mem_to_mpb(mpbs[dest->node], dest, sizeof(snet_dest_t));
  stop_write_node(dest->node);
}

void SNetDistribSendData(snet_ref_t ref, void *data, int node)
{
  snet_comm_type_t type = snet_ref_set;
  start_write_node(node);
  cpy_mem_to_mpb(mpbs[node], &type, sizeof(snet_comm_type_t));
  cpy_mem_to_mpb(mpbs[node], &ref, sizeof(snet_ref_t));
  SNetInterfaceGet(ref.interface)->packfun(data, &node);
  stop_write_node(node);
}
