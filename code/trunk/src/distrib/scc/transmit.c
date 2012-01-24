#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "SCC_API.h"
#include "distribcommon.h"
#include "memfun.h"
#include "interface_functions.h"
#include "record.h"
#include "reference.h"
#include "scc.h"
#include "sccmalloc.h"

extern size_t SNetRefAllocSize(snet_ref_t *ref);

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

inline static void PackInt(void *buf, int count, int *src)
{ cpy_mem_to_mpb((uintptr_t) buf, src, count * sizeof(int)); }

inline static void UnpackInt(void *buf, int count, int *dst)
{ cpy_mpb_to_mem((uintptr_t) buf, dst, count * sizeof(int)); }

inline static void PackByte(void *buf, int count, char *src)
{ cpy_mem_to_mpb((uintptr_t) buf, src, count); }

inline static void UnpackByte(void *buf, int count, char *dst)
{ cpy_mpb_to_mem((uintptr_t) buf, dst, count); }

inline static void PackRef(void *buf, int count, snet_ref_t **src)
{
  for (int i = 0; i < count; i++) {
    SNetRefSerialise(src[i], buf, &PackInt, &PackByte);
    SNetRefOutgoing(src[i]);
  }
}

inline static void UnpackRef(void *buf, int count, snet_ref_t **dst)
{
  for (int i = 0; i < count; i++) {
    dst[i] = SNetRefDeserialise(buf, &UnpackInt, &UnpackByte);
    SNetRefIncoming(dst[i]);
  }
}

snet_msg_t SNetDistribRecvMsg(void)
{
  lut_addr_t addr;
  snet_msg_t result;
  static sigset_t sigmask;
  static bool handling = true, set = false;

  if (!set) {
    set = true;
    write_pid();
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGUSR1);
    sigaddset(&sigmask, SIGUSR2);
  }

start:
  if (!handling) {
    sigwait(&sigmask, &result.val);
    if (result.val == SIGUSR2) {
      result.type = snet_update;
      return result;
    }
  }

  lock(node_location);
  flush();
  if (START(node_location) == END(node_location)) {
    handling = false;
    HANDLING(node_location) = 0;
    FOOL_WRITE_COMBINE;
    unlock(node_location);
    goto start;
  } else if (!handling) {
    handling = true;
    HANDLING(node_location) = 1;
    FOOL_WRITE_COMBINE;
  }

  cpy_mpb_to_mem(node_location, &result.type, sizeof(snet_comm_type_t));
  switch (result.type) {
    case snet_rec:
      result.rec = SNetRecDeserialise((void*) node_location, &UnpackInt, &UnpackRef);
    case snet_block:
    case snet_unblock:
      cpy_mpb_to_mem(node_location, &result.dest, sizeof(snet_dest_t));
      break;
    case snet_ref_set:
      result.ref = SNetRefDeserialise((void*) node_location, &UnpackInt, &UnpackByte);
      if (!remap) cpy_mpb_to_mem(node_location, &addr, sizeof(lut_addr_t));
      result.data = (uintptr_t) SNetInterfaceGet(SNetRefInterface(result.ref))->unpackfun(&addr);
      break;
    case snet_ref_fetch:
      result.ref = SNetRefDeserialise((void*) node_location, &UnpackInt, &UnpackByte);

      if (remap) {
        cpy_mpb_to_mem(node_location, &result.val, sizeof(int));
        unlock(node_location);
        SNetDistribSendData(result.ref, SNetRefGetData(result.ref), &result.val);
        SNetMemFree(result.ref);
        goto start;
      } else {
        result.data = (uintptr_t) SNetMemAlloc(sizeof(lut_addr_t));
        cpy_mpb_to_mem(node_location, (void*) result.data, sizeof(lut_addr_t));
      }
      break;
    case snet_ref_update:
      result.ref = SNetRefDeserialise((void*) node_location, &UnpackInt, &UnpackByte);
      cpy_mpb_to_mem(node_location, &result.val, sizeof(int));
      break;
    default:
      break;
  }

  unlock(node_location);
  return result;
}

void SNetDistribSendRecord(snet_dest_t dest, snet_record_t *rec)
{
  int node = dest.node;
  snet_comm_type_t type = snet_rec;

  start_write_node(node);
  cpy_mem_to_mpb(node, &type, sizeof(snet_comm_type_t));
  SNetRecSerialise(rec, (void*) node, &PackInt, &PackRef);
  dest.node = node_location;
  cpy_mem_to_mpb(node, &dest, sizeof(snet_dest_t));
  dest.node = node;
  stop_write_node(node);
}

void SNetDistribFetchRef(snet_ref_t *ref)
{
  int node = SNetRefNode(ref);
  snet_comm_type_t type = snet_ref_fetch;

  start_write_node(node);
  cpy_mem_to_mpb(node, &type, sizeof(snet_comm_type_t));
  SNetRefSerialise(ref, (void*) node, &PackInt, &PackByte);
  if (remap) {
    cpy_mem_to_mpb(node, &node_location, sizeof(node_location));
  } else {
    lut_addr_t addr = SCCPtr2Addr(SCCMallocPtr(SNetRefAllocSize(ref)));
    cpy_mem_to_mpb(node, &addr, sizeof(lut_addr_t));
  }
  stop_write_node(node);
}

void SNetDistribUpdateRef(snet_ref_t *ref, int count)
{
  int node = SNetRefNode(ref);
  snet_comm_type_t type = snet_ref_update;

  start_write_node(node);
  cpy_mem_to_mpb(node, &type, sizeof(snet_comm_type_t));
  SNetRefSerialise(ref, (void*) node, &PackInt, &PackByte);
  cpy_mem_to_mpb(node, &count, sizeof(int));
  stop_write_node(node);
}

void SNetDistribUpdateBlocked(void)
{
  snet_comm_type_t type = snet_update;
  start_write_node(node_location);
  cpy_mem_to_mpb(node_location, &type, sizeof(snet_comm_type_t));
  stop_write_node(node_location);
}

void SNetDistribUnblockDest(snet_dest_t dest)
{
  snet_comm_type_t type = snet_unblock;
  start_write_node(dest.node);
  cpy_mem_to_mpb(dest.node, &type, sizeof(snet_comm_type_t));
  cpy_mem_to_mpb(dest.node, &dest, sizeof(snet_dest_t));
  stop_write_node(dest.node);
}

void SNetDistribBlockDest(snet_dest_t dest)
{
  snet_comm_type_t type = snet_block;
  start_write_node(dest.node);
  cpy_mem_to_mpb(dest.node, &type, sizeof(snet_comm_type_t));
  cpy_mem_to_mpb(dest.node, &dest, sizeof(snet_dest_t));
  stop_write_node(dest.node);
}

void SNetDistribSendData(snet_ref_t *ref, void *data, void *dest)
{
  int node;
  lut_addr_t addr;
  snet_comm_type_t type = snet_ref_set;

  if (remap) {
    addr.node = *(int*) dest;
  } else {
    addr = *(lut_addr_t*) dest;
    SNetMemFree(dest);
  }

  node = addr.node;

  start_write_node(node);
  cpy_mem_to_mpb(node, &type, sizeof(snet_comm_type_t));
  SNetRefSerialise(ref, (void*) node, &PackInt, &PackByte);
  if (!remap) cpy_mem_to_mpb(node, &addr, sizeof(lut_addr_t));
  SNetInterfaceGet(SNetRefInterface(ref))->packfun(data, &addr);
  stop_write_node(node);
}
