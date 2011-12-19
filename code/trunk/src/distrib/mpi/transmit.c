#include <mpi.h>

#include "debug.h"
#include "memfun.h"
#include "imanager.h"
#include "distribcommon.h"
#include "interface_functions.h"
#include "omanager.h"
#include "pack.h"
#include "record.h"
#include "reference.h"

static mpi_buf_t recvBuf = {0, 0, NULL};
static mpi_buf_t sendBuf = {0, 0, NULL};

static void PackInt(void *buf, int count, int *src)
{ MPIPack(buf, src, MPI_INT, count); }

static void UnpackInt(void *buf, int count, int *dst)
{ MPIUnpack(buf, dst, MPI_INT, count); }

static void PackByte(void *buf, int count, char *src)
{ MPIPack(buf, src, MPI_BYTE, count); }

static void UnpackByte(void *buf, int count, char *dst)
{ MPIUnpack(buf, dst, MPI_BYTE, count); }

static void MPIPackInt(int count, int *src)
{ PackInt(&sendBuf, count, src); }

static void MPIUnpackInt(int count, int *src)
{ UnpackInt(&recvBuf, count, src); }

static void MPIPackRef(int count, snet_ref_t **src)
{
  int i;
  for (i = 0; i < count; i++) {
    SNetRefSerialise(src[i], (void*) &sendBuf, &PackInt, &PackByte);
    SNetRefOutgoing(src[i]);
  }
}

static void MPIUnpackRef(int count, snet_ref_t **dst)
{
  int i;
  for (i = 0; i < count; i++) {
    dst[i] = SNetRefDeserialise((void*) &recvBuf, &UnpackInt, &UnpackByte);
    SNetRefIncoming(dst[i]);
  }
}

static void MPIPackDest(mpi_buf_t *buf, snet_dest_t *dest)
{
  MPIPack(buf, &dest->dest, MPI_INT, 1);
  MPIPack(buf, &dest->parent, MPI_INT, 1);
  MPIPack(buf, &dest->dynamicIndex, MPI_INT, 1);
  MPIPack(buf, &dest->parentNode, MPI_INT, 1);
  MPIPack(buf, &dest->dynamicLoc, MPI_INT, 1);
}

static void MPIUnpackDest(mpi_buf_t *buf, snet_dest_t *dest)
{
  MPIUnpack(buf, &dest->dest, MPI_INT, 1);
  MPIUnpack(buf, &dest->parent, MPI_INT, 1);
  MPIUnpack(buf, &dest->dynamicIndex, MPI_INT, 1);
  MPIUnpack(buf, &dest->parentNode, MPI_INT, 1);
  MPIUnpack(buf, &dest->dynamicLoc, MPI_INT, 1);
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
        result.rec = SNetRecDeserialise(&MPIUnpackInt, &MPIUnpackRef);
    case snet_block:
    case snet_unblock:
      result.dest.node = status.MPI_SOURCE;
      MPIUnpackDest(&recvBuf, &result.dest);
      break;
    case snet_ref_set:
      result.ref = SNetRefDeserialise((void*) &recvBuf, &UnpackInt, &UnpackByte);
      result.data = SNetInterfaceGet(SNetRefInterface(result.ref))->unpackfun(&recvBuf);
      break;
    case snet_ref_fetch:
      result.ref = SNetRefDeserialise((void*) &recvBuf, &UnpackInt, &UnpackByte);
      result.val = status.MPI_SOURCE;
      result.data = &result.val;
      break;
    case snet_ref_update:
      result.ref = SNetRefDeserialise((void*) &recvBuf, &UnpackInt, &UnpackByte);
      MPIUnpackInt(1, &result.val);
      break;
    default:
      break;
  }

  return result;
}

void SNetDistribSendRecord(snet_dest_t dest, snet_record_t *rec)
{
  sendBuf.offset = 0;
  SNetRecSerialise(rec, &MPIPackInt, &MPIPackRef);
  MPIPackDest(&sendBuf, &dest);
  MPI_Send(sendBuf.data, sendBuf.offset, MPI_PACKED, dest.node, snet_rec,
           MPI_COMM_WORLD);
}

void SNetDistribFetchRef(snet_ref_t *ref)
{
  mpi_buf_t buf = {0, 0, NULL};
  SNetRefSerialise(ref, (void*) &buf, &PackInt, &PackByte);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, SNetRefNode(ref), snet_ref_fetch, MPI_COMM_WORLD);
  SNetMemFree(buf.data);
}

void SNetDistribUpdateRef(snet_ref_t *ref, int count)
{
  mpi_buf_t buf = {0, 0, NULL};
  SNetRefSerialise(ref, (void*) &buf, &PackInt, &PackByte);
  MPIPack(&buf, &count, MPI_INT, 1);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, SNetRefNode(ref), snet_ref_update, MPI_COMM_WORLD);
  SNetMemFree(buf.data);
}

extern int node_location;
void SNetDistribUpdateBlocked(void)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPI_Send(buf.data, buf.offset, MPI_PACKED, node_location, snet_update, MPI_COMM_WORLD);
}

void SNetDistribUnblockDest(snet_dest_t dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPIPackDest(&buf, &dest);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, dest.node, snet_unblock, MPI_COMM_WORLD);
  SNetMemFree(buf.data);
}

void SNetDistribBlockDest(snet_dest_t dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPIPackDest(&buf, &dest);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, dest.node, snet_block, MPI_COMM_WORLD);
  SNetMemFree(buf.data);
}

void SNetDistribSendData(snet_ref_t *ref, void *data, void *dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  SNetRefSerialise(ref, (void*) &buf, &PackInt, &PackByte);
  SNetInterfaceGet(SNetRefInterface(ref))->packfun(data, &buf);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, *(int*)dest, snet_ref_set, MPI_COMM_WORLD);
  SNetMemFree(buf.data);
}
