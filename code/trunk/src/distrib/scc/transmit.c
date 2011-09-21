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

snet_record_t *SNetDistribRecvRecord(snet_dest_t **recordDest)
{
  int sig, start, end, status;
  static int pid = 0;
  static sigset_t sigmask;
  static bool handling = true;

  recvMPB = mpbs[node_location];

  if (!pid) {
    pid = 1;
    write_pid();
    sigemptyset(&sigmask);
    sigaddset(&sigmask, SIGUSR1);
  }

  while (true) {
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
      continue;
    } else {
      handling = true;
      HANDLING(recvMPB) = 1;
      FOOL_WRITE_COMBINE;
    }

    cpy_mpb_to_mem(recvMPB, &status, sizeof(int));
    if (status == 0) {
      snet_dest_t dest;
      cpy_mpb_to_mem(recvMPB, &dest, sizeof(snet_dest_t));
      snet_record_t *rec = SNetRecDeserialise(&SCCUnpackInt, &SCCUnpackRef);
      *recordDest = SNetDestCopy(&dest);
      unlock(node_location);
      return rec;
    } else if (status == 1) {
      unlock(node_location);
      return NULL;
    } else if (status == 6) {
      SNetInputManagerUpdate();
    } else if (status == 7) {
      snet_dest_t dest;
      cpy_mpb_to_mem(recvMPB, &dest, sizeof(snet_dest_t));
      SNetOutputManagerUnblock(SNetDestCopy(&dest));
    } else if (status == 8) {
      snet_dest_t dest;
      cpy_mpb_to_mem(recvMPB, &dest, sizeof(snet_dest_t));
      SNetOutputManagerBlock(SNetDestCopy(&dest));
    }
    unlock(node_location);
  }
}

void SNetDistribSendRecord(snet_dest_t *dest, snet_record_t *rec)
{
  int handling = 0;
  int node = dest->node;
  sendMPB = mpbs[node];

  start_write_node(node);
  cpy_mem_to_mpb(sendMPB, &handling, sizeof(int));

  dest->node = node_location;
  cpy_mem_to_mpb(sendMPB, dest, sizeof(snet_dest_t));
  dest->node = node;

  SNetRecSerialise(rec, &SCCPackInt, &SCCPackRef);
  stop_write_node(node);
}

void SNetDistribRemoteRefFetch(snet_ref_t *ref)
{
  assert(0);
/*
  mpi_buf_t buf = {0, 0, NULL};
  PackRef(&buf, *ref);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, ref->node, 2, MPI_COMM_WORLD);
*/
}

void SNetDistribRemoteRefUpdate(snet_ref_t *ref, int count)
{
  assert(0);
/*
  mpi_buf_t buf = {0, 0, NULL};
  PackRef(&buf, *ref);
  MPIPack(&buf, &count, MPI_INT, 1);
  MPI_Send(buf.data, buf.offset, MPI_PACKED, ref->node, 3, MPI_COMM_WORLD);
*/
}

void SNetDistribRemoteDestUnblock(snet_dest_t *dest)
{
  int node, status;

  node = dest ? dest->node : node_location;

  start_write_node(node);
  if (dest == NULL) {
    status = 6;
    cpy_mem_to_mpb(mpbs[node], &status, sizeof(int));
  } else {
    status = 7;
    cpy_mem_to_mpb(mpbs[node], &status, sizeof(int));
    cpy_mem_to_mpb(mpbs[node], dest, sizeof(snet_dest_t));
  }
  stop_write_node(node);
}

void SNetDistribRemoteDestBlock(snet_dest_t *dest)
{
  int status = 8;

  start_write_node(dest->node);
  cpy_mem_to_mpb(mpbs[dest->node], &status, sizeof(int));
  cpy_mem_to_mpb(mpbs[dest->node], dest, sizeof(snet_dest_t));
  stop_write_node(dest->node);
}
