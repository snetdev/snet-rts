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
    SNetRefOutgoing(src[i]);
    PackRef(&sendBuf, *src[i]);
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

snet_record_t *SNetDistribRecvRecord(snet_dest_t **recordDest)
{
  MPI_Status status;

  while (true) {
    int count;
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

    if (status.MPI_TAG == 0) {
      snet_dest_t dest;
      dest.node = status.MPI_SOURCE;
      MPIUnpackInt(1, &dest.dest);
      MPIUnpackInt(1, &dest.parent);
      MPIUnpackInt(1, &dest.dynamicIndex);
      MPIUnpackInt(1, &dest.parentNode);
      MPIUnpackInt(1, &dest.dynamicLoc);
      snet_record_t *rec = SNetRecDeserialise(&MPIUnpackInt, &MPIUnpackRef);
      *recordDest = SNetDestCopy(&dest);
      return rec;
    } else if (status.MPI_TAG == 1) {
      return NULL;
    } else if (status.MPI_TAG == 6) {
      SNetInputManagerUpdate();
    } else if (status.MPI_TAG == 7) {
      snet_dest_t dest;
      dest.node = status.MPI_SOURCE;
      MPIUnpackInt(1, &dest.dest);
      MPIUnpackInt(1, &dest.parent);
      MPIUnpackInt(1, &dest.dynamicIndex);
      MPIUnpackInt(1, &dest.parentNode);
      MPIUnpackInt(1, &dest.dynamicLoc);
      SNetOutputManagerUnblockDest(&dest);
    } else if (status.MPI_TAG == 8) {
      snet_dest_t dest;
      dest.node = status.MPI_SOURCE;
      MPIUnpackInt(1, &dest.dest);
      MPIUnpackInt(1, &dest.parent);
      MPIUnpackInt(1, &dest.dynamicIndex);
      MPIUnpackInt(1, &dest.parentNode);
      MPIUnpackInt(1, &dest.dynamicLoc);
      SNetOutputManagerBlockDest(&dest);
    } else {
      void *tmp;
      snet_ref_t ref = UnpackRef(&recvBuf);

      if (status.MPI_TAG == 2) {
        tmp = SNetRefFetch(&ref);
        recvBuf.offset = 0;
        PackRef(&recvBuf, ref);
        SNetInterfaceGet(ref.interface)->packfun(tmp, &recvBuf);
        MPI_Send(recvBuf.data, recvBuf.offset, MPI_PACKED, status.MPI_SOURCE, 4, MPI_COMM_WORLD);
        SNetInterfaceGet(ref.interface)->freefun(tmp);
      } else if (status.MPI_TAG == 3) {
        int blaat;
        MPIUnpackInt(1, &blaat);
        SNetRefUpdate(&ref, blaat);
      } else if (status.MPI_TAG == 4) {
        snet_unpack_fun_t unpackfun = SNetInterfaceGet(ref.interface)->unpackfun;
        SNetRefSet(&ref, unpackfun(&recvBuf));
      }
    }
  }
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
  MPI_Send(sendBuf.data, sendBuf.offset, MPI_PACKED, dest->node, 0,
           MPI_COMM_WORLD);
}

void SNetDistribRemoteRefFetch(snet_ref_t *ref)
{
  mpi_buf_t buf = {0, 0, NULL};
  PackRef(&buf, *ref);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, ref->node, 2, MPI_COMM_WORLD);
}

void SNetDistribRemoteRefUpdate(snet_ref_t *ref, int count)
{
  mpi_buf_t buf = {0, 0, NULL};
  PackRef(&buf, *ref);
  MPIPack(&buf, &count, MPI_INT, 1);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, ref->node, 3, MPI_COMM_WORLD);
}

extern int node_location;
void SNetDistribRemoteDestUnblock(snet_dest_t *dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  if (dest == NULL) {
    MPI_Send(buf.data, buf.offset, MPI_PACKED, node_location, 6, MPI_COMM_WORLD);
  } else {
    MPIPack(&buf, &dest->dest, MPI_INT, 1);
    MPIPack(&buf, &dest->parent, MPI_INT, 1);
    MPIPack(&buf, &dest->parentNode, MPI_INT, 1);
    MPIPack(&buf, &dest->dynamicIndex, MPI_INT, 1);
    MPIPack(&buf, &dest->dynamicLoc, MPI_INT, 1);
    MPI_Send(buf.data, buf.offset, MPI_PACKED, dest->node, 7, MPI_COMM_WORLD);
  }
}

void SNetDistribRemoteDestBlock(snet_dest_t *dest)
{
  mpi_buf_t buf = {0, 0, NULL};
  MPIPack(&buf, &dest->dest, MPI_INT, 1);
  MPIPack(&buf, &dest->parent, MPI_INT, 1);
  MPIPack(&buf, &dest->parentNode, MPI_INT, 1);
  MPIPack(&buf, &dest->dynamicIndex, MPI_INT, 1);
  MPIPack(&buf, &dest->dynamicLoc, MPI_INT, 1);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, dest->node, 8, MPI_COMM_WORLD);
}
