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

extern int node_location;

inline static void MPISend(void *src, int size, int dest, int type)
{ MPI_Send(src, size, MPI_PACKED, dest, type, MPI_COMM_WORLD); }

void SNetMPISend(void *src, int size, int dest, int type)
{ MPI_Send(src, size, MPI_PACKED, dest, type, MPI_COMM_WORLD); }

void SNetPackInt(void *buf, int count, int *src)
{ MPIPack(buf, src, MPI_INT, count); }

void SNetUnpackInt(void *buf, int count, int *dst)
{ MPIUnpack(buf, dst, MPI_INT, count); }

void SNetPackByte(void *buf, int count, char *src)
{ MPIPack(buf, src, MPI_BYTE, count); }

void SNetUnpackByte(void *buf, int count, char *dst)
{ MPIUnpack(buf, dst, MPI_BYTE, count); }

void SNetPackLong(void *buf, int count, long *src)
{ MPIPack(buf, src, MPI_LONG, count); }

void SNetUnpackLong(void *buf, int count, long *dst)
{ MPIUnpack(buf, dst, MPI_LONG, count); }

void SNetPackVoid(void *buf, int count, void **src)
{ MPIPack(buf, src, MPI_VOID_POINTER, count); }

void SNetUnpackVoid(void *buf, int count, void **dst)
{ MPIUnpack(buf, dst, MPI_VOID_POINTER, count); }

void SNetPackRef(void *buf, int count, snet_ref_t **src)
{
  for (int i = 0; i < count; i++) {
    SNetRefSerialise(src[i], buf, &SNetPackInt, &SNetPackByte);
    SNetRefOutgoing(src[i]);
  }
}

void SNetUnpackRef(void *buf, int count, snet_ref_t **dst)
{
  for (int i = 0; i < count; i++) {
    dst[i] = SNetRefDeserialise(buf, &SNetUnpackInt, &SNetUnpackByte);
    SNetRefIncoming(dst[i]);
  }
}

inline static void PackDest(mpi_buf_t *buf, snet_dest_t *dest)
{
  SNetPackInt(buf, 1, &dest->dest);
  SNetPackInt(buf, 1, &dest->parent);
  SNetPackInt(buf, 1, &dest->dynamicIndex);
  SNetPackInt(buf, 1, &dest->parentNode);
  SNetPackInt(buf, 1, &dest->dynamicLoc);
}

inline static void UnpackDest(mpi_buf_t *buf, snet_dest_t *dest)
{
  SNetUnpackInt(buf, 1, &dest->dest);
  SNetUnpackInt(buf, 1, &dest->parent);
  SNetUnpackInt(buf, 1, &dest->dynamicIndex);
  SNetUnpackInt(buf, 1, &dest->parentNode);
  SNetUnpackInt(buf, 1, &dest->dynamicLoc);
}

snet_msg_t SNetDistribRecvMsg(void)
{
  int count;
  snet_msg_t result;
  MPI_Status status;
  static mpi_buf_t recvBuf = {0, 0, NULL};

  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  MPI_Get_count(&status, MPI_PACKED, &count);

  MPI_Pack_size(count, MPI_PACKED, MPI_COMM_WORLD, &recvBuf.offset);
  if (recvBuf.offset > recvBuf.size) {
    recvBuf.data = SNetMemResize(recvBuf.data, recvBuf.offset);
    recvBuf.size = recvBuf.offset;
  }

  MPI_Recv(recvBuf.data, count, MPI_PACKED, status.MPI_SOURCE, status.MPI_TAG,
            MPI_COMM_WORLD, &status);

  recvBuf.offset = 0;
  result.type = status.MPI_TAG;

  switch (status.MPI_TAG) {
    case snet_rec:
      result.rec = SNetRecDeserialise(&recvBuf, &SNetUnpackInt, &SNetUnpackRef);
    case snet_block:
    case snet_unblock:
      result.dest.node = status.MPI_SOURCE;
      UnpackDest(&recvBuf, &result.dest);
      break;
    case snet_ref_set:
      result.ref = SNetRefDeserialise(&recvBuf, &SNetUnpackInt, &SNetUnpackByte);
      result.data = (uintptr_t) SNetInterfaceGet(SNetRefInterface(result.ref))->unpackfun(&recvBuf);
      break;
    case snet_ref_fetch:
      result.ref = SNetRefDeserialise(&recvBuf, &SNetUnpackInt, &SNetUnpackByte);
      result.data = status.MPI_SOURCE;
      break;
    case snet_ref_update:
      result.ref = SNetRefDeserialise(&recvBuf, &SNetUnpackInt, &SNetUnpackByte);
      SNetUnpackInt(&recvBuf, 1, &result.val);
      break;
    case snet_update:
      break;
    case snet_stop:
      break;
    default:
      SNetUtilDebugFatal("[%s]: Unexpected MPI TAG %d\n", __func__, result.type);
      break;
  }

  return result;
}

void SNetDistribSendRecord(snet_dest_t dest, snet_record_t *rec)
{
  static mpi_buf_t sendBuf = {0, 0, NULL};

  sendBuf.offset = 0;
  SNetRecSerialise(rec, &sendBuf, &SNetPackInt, &SNetPackRef);
  PackDest(&sendBuf, &dest);
  MPISend(sendBuf.data, sendBuf.offset, dest.node, snet_rec);
}

void SNetDistribFetchRef(snet_ref_t *ref)
{
  mpi_buf_t buf = {0, 0, NULL};
  SNetRefSerialise(ref, &buf, &SNetPackInt, &SNetPackByte);
  MPISend(buf.data, buf.offset, SNetRefNode(ref), snet_ref_fetch);
  SNetMemFree(buf.data);
}

void SNetDistribUpdateRef(snet_ref_t *ref, int count)
{
  mpi_buf_t buf = {0, 0, NULL};
  SNetRefSerialise(ref, &buf, &SNetPackInt, &SNetPackByte);
  SNetPackInt(&buf, 1, &count);
  MPISend(buf.data, buf.offset, SNetRefNode(ref), snet_ref_update);
  SNetMemFree(buf.data);
}

void SNetDistribUpdateBlocked(void)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPISend(buf.data, buf.offset, node_location, snet_update);
}

void SNetDistribUnblockDest(snet_dest_t dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  PackDest(&buf, &dest);
  MPISend(buf.data, buf.offset, dest.node, snet_unblock);
  SNetMemFree(buf.data);
}

void SNetDistribBlockDest(snet_dest_t dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  PackDest(&buf, &dest);
  MPISend(buf.data, buf.offset, dest.node, snet_block);
  SNetMemFree(buf.data);
}

void SNetDistribSendData(snet_ref_t *ref, void *data, void *dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  SNetRefSerialise(ref, &buf, &SNetPackInt, &SNetPackByte);
  SNetInterfaceGet(SNetRefInterface(ref))->packfun(data, &buf);
  MPISend(buf.data, buf.offset, (uintptr_t) dest, snet_ref_set);
  SNetMemFree(buf.data);
}
