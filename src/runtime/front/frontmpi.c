#include <stdio.h>
#include <assert.h>
#include "debug.h"
#include "distribcommon.h"
#include "distribfront.h"
#include "pack.h"
#include "reference.h"
#include "snetentities.h"

extern const char* SNetCommName(int i);
extern bool SNetDebugDF(void);

/* Initiate a new connection. */
void SNetDistribTransmitConnect(connect_t *connect)
{
  const int     num_ints = sizeof(connect_t) / sizeof(int);
  mpi_buf_t     buf = {0, 100, SNetMemAlloc(100)};

  SNetPackInt(&buf, num_ints, (int *) connect);
  if (SNetDebugDF()) {
    printf("[%s.%d]: sending %d connect bytes\n",
           __func__, SNetDistribGetNodeId(), buf.offset);
  }
  SNetMPISend(buf.data, buf.offset, connect->dest_loc, SNET_COMM_connect);
  SNetMemFree(buf.data);
}

/* Accept an incoming connection. */
static connect_t* SNetDistribReceiveConnect(mpi_buf_t *buf)
{
  const int     num_ints = sizeof(connect_t) / sizeof(int);
  connect_t    *connect = SNetNew(connect_t);

  SNetUnpackInt(buf, num_ints, (int *) connect);

  return connect;
}

void SNetDistribTransmitRecord(snet_record_t *rec, connect_t *connect)
{
  mpi_buf_t buf = {0, 1000, SNetMemAlloc(1000)};
  SNetPackInt(&buf, 2, (int *) connect);
  if (SNetDebugDF()) {
    printf("[%s.%d]: sending %d record bytes for type %s\n",
           __func__, SNetDistribGetNodeId(), buf.offset, SNetRecTypeName(rec));
  }
  SNetRecSerialise(rec, &buf, &SNetPackInt, &SNetPackRef);
  SNetMPISend(buf.data, buf.offset, connect->dest_loc, SNET_COMM_record);
  SNetMemFree(buf.data);
}

void SNetDistribReceiveRecord(snet_mesg_t *mesg, mpi_buf_t *buf)
{
  int           from[2];

  SNetUnpackInt(buf, 2, from);
  assert(from[0] == mesg->source);
  mesg->conn = from[1];
  mesg->rec = SNetRecDeserialise(buf, &SNetUnpackInt, &SNetUnpackRef);
}

/* Accept an incoming message via MPI. */
void SNetDistribReceiveMessage(snet_mesg_t *mesg)
{
  MPI_Status    status;
  int           count;
  int           interface;
  mpi_buf_t     buf = { 0, 1000, SNetMemAlloc(1000) };

  /* Wait for a new message to arrive. */
  MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
  /* Get number of top-level elements in 'count'. */
  MPI_Get_count(&status, MPI_PACKED, &count);
  /* Find out how much space is needed for the message. */
  MPI_Pack_size(count, MPI_PACKED, MPI_COMM_WORLD, &buf.offset);
  /* Assure that buf is big enough. */
  if (buf.offset > buf.size) {
    buf.data = SNetMemResize(buf.data, buf.offset);
    buf.size = buf.offset;
  }
  /* Load the message into buf. */
  MPI_Recv(buf.data, count, MPI_PACKED, status.MPI_SOURCE, status.MPI_TAG,
           MPI_COMM_WORLD, &status);
  /* Reset to beginning of message buffer. */
  buf.offset = 0;

  memset(mesg, 0, sizeof(*mesg));
  mesg->type = status.MPI_TAG;
  mesg->source = status.MPI_SOURCE;
  if (SNetDebugDF()) {
    printf("[%s.%d]: received tag %s and %d bytes from %d\n", __func__,
           SNetDistribGetNodeId(), SNetCommName(mesg->type), count, mesg->source);
  }

  switch (mesg->type) {

    case SNET_COMM_connect:
      mesg->connect = SNetDistribReceiveConnect(&buf);
      break;

    case SNET_COMM_record:
      SNetDistribReceiveRecord(mesg, &buf);
      break;

    case snet_ref_set:
      mesg->ref = SNetRefDeserialise(&buf, &SNetUnpackInt, &SNetUnpackByte);
      interface = SNetRefInterface(mesg->ref);
      mesg->data = (uintptr_t) SNetInterfaceGet(interface)->unpackfun(&buf);
      break;

    case snet_ref_fetch:
      mesg->ref = SNetRefDeserialise(&buf, &SNetUnpackInt, &SNetUnpackByte);
      mesg->data = (uintptr_t) status.MPI_SOURCE;
      break;

    case snet_ref_update:
      mesg->ref = SNetRefDeserialise(&buf, &SNetUnpackInt, &SNetUnpackByte);
      SNetUnpackInt(&buf, 1, &mesg->val);
      break;

    case snet_stop:
      mesg->type = SNET_COMM_stop;
      break;

    default:
      SNetUtilDebugFatal("[%s.%d]: unrecognized message type %d\n",
                         __func__, SNetDistribGetNodeId(), status.MPI_TAG);
  }

  SNetMemFree(buf.data);
}

