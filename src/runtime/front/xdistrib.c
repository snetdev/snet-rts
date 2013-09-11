/* Implement Distributed S-Net for the Front runtime system. */

#include "node.h"
#include "reference.h"
#include "distribfront.h"

void SNetDistribLocalStop(void);
void SNetDistribImplementationInit(int argc, char **argv, snet_info_t *info);

void hello(const char *s)
{
  if (SNetDebugDF()) {
    printf("[%s.%d] hello\n", s, SNetDistribGetNodeId());
  }
}

/* Initialize distributed computing. Called by SNetInRun in networkinterface.c. */
void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  /* Tell MPI to init. */
  SNetDistribImplementationInit(argc, argv, info);

  /* Setup data structures. */
  SNetReferenceInit();
  SNetInputManagerInit();
  SNetTransferInit();
}

/* Start input+output managers. Called by SNetInRun in networkinterface.c. */
void SNetDistribStart(void)
{
  /* Not needed in Front. */
}

/* Cause dist layer to terminate. */
void SNetDistribStop(void)
{
  hello(__func__);

  /* Tell lower layers to cleanup. */
  SNetDistribLocalStop();
  SNetTransferStop();
  SNetInputManagerStop();
}

/* Dummy function needed for networkinterface.c */
snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *in, int loc)
{
  return in;
}

extern void SNetPackInt(void *buf, int count, int *data);
extern void SNetPackLong(void *buf, int count, long *data);
extern void SNetPackVoid(void *buf, int count, void **data);
extern void SNetUnpackInt(void *buf, int count, int *dst);
extern void SNetUnpackLong(void *buf, int count, long *src);
extern void SNetUnpackVoid(void *buf, int count, void **dst);

void SNetRecDetrefStackSerialise(snet_record_t *rec, void *buf)
{
  snet_stack_t *stack = DATA_REC( rec, detref);
  int size = stack ? SNetStackElementCount(stack) : 0;
  SNetPackInt(buf, 1, &size);
  if (stack) {
    snet_stack_node_t *node;
    detref_t *detref;
    STACK_FOR_EACH(stack, node, detref) {
      void *vptr;
      SNetPackLong(buf, 1, &detref->seqnr);
      vptr = detref->leave;
      SNetPackVoid(buf, 1, &vptr);
      vptr = (detref->nonlocal ? detref->nonlocal : detref);
      SNetPackVoid(buf, 1, &vptr);
      SNetPackInt(buf, 1, &detref->location);
    }
  }
}

void SNetRecDetrefRecSerialise(snet_record_t *rec, void *buf)
{
  void *vptr;
  SNetPackLong(buf, 1, &DETREF_REC(rec, seqnr));
  SNetPackInt(buf, 1, &DETREF_REC(rec, location));
  SNetPackInt(buf, 1, &DETREF_REC(rec, senderloc));
  vptr = DETREF_REC(rec, leave);
  SNetPackVoid(buf, 1, &vptr);
  vptr = DETREF_REC(rec, detref);
  SNetPackVoid(buf, 1, &vptr);
}

void SNetRecDetrefStackDeserialise(snet_record_t *rec, void *buf)
{
  int size = 0;
  SNetUnpackInt(buf, 1, &size);
  if (size > 0) {
    snet_stack_t *stack = SNetStackCreate();
    int i;
    for (i = 0; i < size; ++i) {
      detref_t *detref = SNetNewAlign(detref_t);
      void *vptr = NULL;
      SNetUnpackLong(buf, 1, &detref->seqnr);
      SNetUnpackVoid(buf, 1, &vptr);
      detref->leave = vptr;
      SNetUnpackVoid(buf, 1, &vptr);
      detref->nonlocal = vptr;
      SNetUnpackInt(buf, 1, &detref->location);
      detref->refcount = DETREF_MIN_REFCOUNT;
      if (detref->location == SNetDistribGetNodeId()) {
        detref_t *is_local = detref->nonlocal;
        assert(is_local->seqnr == detref->seqnr);
        assert(is_local->refcount >= DETREF_MIN_REFCOUNT);
        SNetMemFree(detref);
        detref = is_local;
      }
      SNetStackAppend(stack, detref);
    }
    DATA_REC(rec, detref) = stack;
  }
}

snet_record_t* SNetRecDetrefRecDeserialise(void *buf)
{
  long  seqnr;
  int   location, senderloc;
  void *leave, *detref;
  snet_record_t *rec;

  SNetUnpackLong(buf, 1, &seqnr);
  SNetUnpackInt (buf, 1, &location);
  SNetUnpackInt (buf, 1, &senderloc);
  SNetUnpackVoid(buf, 1, &leave);
  SNetUnpackVoid(buf, 1, &detref);
  rec = SNetRecCreate(REC_detref, seqnr, location, leave, detref);
  DETREF_REC(rec, senderloc) = senderloc;
  return rec;
}

