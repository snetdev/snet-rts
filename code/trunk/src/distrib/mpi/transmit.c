#include <mpi.h>

#include "debug.h"
#include "dest.h"
#include "memfun.h"
#include "imanager.h"
#include "iomanagers.h"
#include "interface_functions.h"
#include "omanager.h"
#include "pack.h"
#include "record.h"
#include "reference.h"

static mpi_buf_t recvBuf = {0, 0, NULL};
static mpi_buf_t sendBuf = {0, 0, NULL};

static void MPIPackInt(int count, int *src)
{
  MPIPack(&sendBuf, src, MPI_INT, count);
}

static void MPIUnpackInt(int count, int *dst)
{
  MPIUnpack(&recvBuf, dst, MPI_INT, count);
}

static void PackRef(mpi_buf_t *buf, snet_ref_t ref)
{
  MPIPack(buf, &ref.node, MPI_INT, 1);
  MPIPack(buf, &ref.interface, MPI_INT, 1);
  MPIPack(buf, &ref.data, MPI_BYTE, sizeof(uintptr_t));
}

static snet_ref_t UnpackRef(mpi_buf_t *buf)
{
  snet_ref_t ref;
  MPIUnpack(buf, &ref.node, MPI_INT, 1);
  MPIUnpack(buf, &ref.interface, MPI_INT, 1);
  MPIUnpack(buf, &ref.data, MPI_BYTE, sizeof(uintptr_t));
  return ref;
}

static void MPIPackRef(int count, snet_ref_t **src)
{
  int i;
  for (i = 0; i < count; i++) {
    PackRef(&sendBuf, *src[i]);
    SNetRefOutgoing(src[i]);
  }
}

static void MPIUnpackRef(int count, snet_ref_t **dst)
{
  int i;
  for (i = 0; i < count; i++) {
    snet_ref_t ref = UnpackRef(&recvBuf);
    dst[i] = SNetRefIncoming(&ref);
  }
}

snet_msg_t SNetDistribRecvMsg(void)
{
  int count;
  snet_msg_t result;
  MPI_Status status;

  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  MPI_Get_count(&status, MPI_PACKED, &count);

  MPI_Pack_size(count, MPI_PACKED, MPI_COMM_WORLD, &recvBuf.offset);
  if ((unsigned) recvBuf.offset > recvBuf.size) {
    SNetMemFree(recvBuf.data);
    recvBuf.data = SNetMemAlloc(recvBuf.offset);
    recvBuf.size = recvBuf.offset;
  }

  MPI_Recv(recvBuf.data, count, MPI_PACKED, MPI_ANY_SOURCE, status.MPI_TAG,
            MPI_COMM_WORLD, &status);

  recvBuf.offset = 0;
  result.type = status.MPI_TAG;

  switch (result.type) {
    case snet_rec:
    case snet_block:
    case snet_unblock:
      result.dest = SNetMemAlloc(sizeof(snet_dest_t));
      result.dest->node = status.MPI_SOURCE;
      MPIUnpackInt(1, &result.dest->dest);
      MPIUnpackInt(1, &result.dest->parent);
      MPIUnpackInt(1, &result.dest->dynamicIndex);
      MPIUnpackInt(1, &result.dest->parentNode);
      MPIUnpackInt(1, &result.dest->dynamicLoc);
      if (result.type == snet_rec) {
        result.rec = SNetRecDeserialise(&MPIUnpackInt, &MPIUnpackRef);
      }
      break;
    case snet_ref_set:
      result.ref = UnpackRef(&recvBuf);
      result.data = SNetInterfaceGet(result.ref.interface)->unpackfun(&recvBuf);
      break;
    case snet_ref_fetch:
      result.ref = UnpackRef(&recvBuf);
      result.val = status.MPI_SOURCE;
      break;
    case snet_ref_update:
      result.ref = UnpackRef(&recvBuf);
      MPIUnpackInt(1, &result.val);
      break;
    default:
      break;
  }

  return result;
}

void SNetDistribSendRecord(snet_dest_t *dest, snet_record_t *rec)
{
  sendBuf.offset = 0;
  MPIPackInt(1, &dest->dest);
  MPIPackInt(1, &dest->parent);
  MPIPackInt(1, &dest->dynamicIndex);
  MPIPackInt(1, &dest->parentNode);
  MPIPackInt(1, &dest->dynamicLoc);
  SNetRecSerialise(rec, &MPIPackInt, &MPIPackRef);
  MPI_Send(sendBuf.data, sendBuf.offset, MPI_PACKED, dest->node, snet_rec,
           MPI_COMM_WORLD);
}

void SNetDistribFetchRef(snet_ref_t *ref)
{
  mpi_buf_t buf = {0, 0, NULL};
  PackRef(&buf, *ref);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, ref->node, snet_ref_fetch, MPI_COMM_WORLD);
}

void SNetDistribUpdateRef(snet_ref_t *ref, int count)
{
  mpi_buf_t buf = {0, 0, NULL};
  PackRef(&buf, *ref);
  MPIPack(&buf, &count, MPI_INT, 1);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, ref->node, snet_ref_update, MPI_COMM_WORLD);
}

extern int node_location;
void SNetDistribUpdateBlocked(void)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPI_Send(buf.data, buf.offset, MPI_PACKED, node_location, snet_update, MPI_COMM_WORLD);
}

void SNetDistribUnblockDest(snet_dest_t *dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPIPack(&buf, &dest->dest, MPI_INT, 1);
  MPIPack(&buf, &dest->parent, MPI_INT, 1);
  MPIPack(&buf, &dest->parentNode, MPI_INT, 1);
  MPIPack(&buf, &dest->dynamicIndex, MPI_INT, 1);
  MPIPack(&buf, &dest->dynamicLoc, MPI_INT, 1);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, dest->node, snet_unblock, MPI_COMM_WORLD);
}

void SNetDistribBlockDest(snet_dest_t *dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPIPack(&buf, &dest->dest, MPI_INT, 1);
  MPIPack(&buf, &dest->parent, MPI_INT, 1);
  MPIPack(&buf, &dest->parentNode, MPI_INT, 1);
  MPIPack(&buf, &dest->dynamicIndex, MPI_INT, 1);
  MPIPack(&buf, &dest->dynamicLoc, MPI_INT, 1);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, dest->node, snet_block, MPI_COMM_WORLD);
}

void SNetDistribSendData(snet_ref_t ref, void *data, int node)
{
  mpi_buf_t buf = {0, 0, NULL};
  PackRef(&buf, ref);
  SNetInterfaceGet(ref.interface)->packfun(data, &buf);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, node, snet_ref_set, MPI_COMM_WORLD);
}
