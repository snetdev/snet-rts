#include "node.h"

/* Collect output records and forward */
void SNetNodeCollector(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const collector_arg_t *carg = DESC_NODE_SPEC(desc, collector);
  landing_collector_t   *land = DESC_LAND_SPEC(desc, collector);

  trace(__func__);
  if (land->outdesc == NULL) {
    land->outdesc = SNetStreamOpen(carg->output, desc);
  }
  switch (REC_DESCR(rec)) {
    case REC_data:
      /* Test if this collector is involved in preserving determinism. */
      if (carg->is_det | carg->is_detsup) {
        if (DATA_REC(rec, detref)) {
          detref_t *detref = SNetStackTop(DATA_REC(rec, detref));
          if (detref->leave == desc->landing &&
              detref->location == SNetDistribGetNodeId())
          {
            SNetDetLeaveRec(rec, desc->landing);
            rec = NULL;
          }
        }
        /* Test for a stray record. Not sure if this is still needed... */
        if (rec && SNetFifoNonEmpty(&land->detfifo)) {
          /* Obtain the sequence number of the most recent detref. */
          detref_t *tail = SNetFifoPeekLast(&land->detfifo);
          detref_t *detref = SNetRecDetrefCreate(rec, tail->seqnr + 1, desc->landing);
          /* Queue record to process it later. */
          SNetFifoPut(&detref->recfifo, rec);
          /* Mark record processing as done for now. */
          rec = NULL;
          /* Append detref to list of detrefs. */
          SNetFifoPut(&land->detfifo, detref);
          /* Check for ready detrefs. */
          SNetDetLeaveDequeue(desc->landing);
        }
      }
      if (rec) {
        bool last = (desc->landing->refs > 1 || SNetFifoNonEmpty(&land->detfifo));
        SNetWrite(&land->outdesc, rec, last);
        if (!last) {
          /* Dissolve when there is only one incoming connection left. */
          SNetBecomeIdentity(desc->landing, land->outdesc);
        }
      }
      break;

    case REC_detref:
      if (DETREF_REC( rec, leave) == desc->landing &&
          DETREF_REC( rec, location) == SNetDistribGetNodeId())
      {
        SNetDetLeaveCheckDetref(rec, &land->detfifo);
        SNetRecDestroy(rec);
        SNetDetLeaveDequeue(desc->landing);
      } else {
        SNetWrite(&land->outdesc, rec, true);
      }
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, carg->entity);
  }
}

/* Terminate a collector landing. */
void SNetTermCollector(landing_t *land, fifo_t *fifo)
{
  landing_collector_t   *lcoll = LAND_SPEC(land, collector);
  trace(__func__);
  if (lcoll->outdesc) {
    SNetFifoPut(fifo, lcoll->outdesc);
  }
  SNetFreeLanding(land);
}

/* Destroy a collector node. */
void SNetStopCollector(node_t *node, fifo_t *fifo)
{
  collector_arg_t *carg = NODE_SPEC(node, collector);
  trace(__func__);
  if (++carg->stopped == carg->num) {
    SNetStopStream(carg->output, fifo);
    SNetEntityDestroy(carg->entity);
    SNetDelete(node);
  }
}

/* Create a dynamic collector */
snet_stream_t *SNetCollectorDynamic(
    snet_stream_t *instream,
    int location,
    snet_info_t *info,
    bool is_det,
    node_t *peer)
{
  collector_arg_t *carg;
  node_t *node;
  snet_stream_t *output;

  trace(__func__);
  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_collector, location, &instream, 1, &output, 1,
                     SNetNodeCollector, SNetStopCollector, SNetTermCollector);
  carg = NODE_SPEC(node, collector);
  carg->output = output;
  carg->num = 1;
  carg->is_det = is_det;
  carg->is_detsup = (SNetDetGetLevel() > 0);
  carg->peer = peer;
  carg->stopped = 0;
  carg->entity = SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
                                   "<collector>", NULL, (void *) carg);
  return output;
}

/* Register a new stream with the dynamic collector */
void SNetCollectorAddStream(
    node_t *node,
    snet_stream_t *stream)
{
  collector_arg_t        *carg;

  trace(__func__);
  assert(node);
  assert(NODE_TYPE(node) == NODE_collector);
  carg = NODE_SPEC(node, collector);
  assert(!STREAM_DEST(stream));

  STREAM_DEST(stream) = node;
  ++carg->num;
  SNetNodeTableAdd(stream);
  if (SNetDebugDF()) {
    printf("[%s.%d]: table_index=%d\n", __func__, SNetDistribGetNodeId(),
           stream->table_index);
  }
}

snet_stream_t *SNetCollectorStatic(
    int num,
    snet_stream_t **instreams,
    int location,
    snet_info_t *info,
    bool is_det,
    node_t *peer)
{
  collector_arg_t *carg;
  node_t *node;
  snet_stream_t *output;

  trace(__func__);
  assert(num >= 2);
  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_collector, location, instreams, num, &output, 1,
                     SNetNodeCollector, SNetStopCollector, SNetTermCollector);
  carg = NODE_SPEC(node, collector);
  carg->output = output;
  carg->num = num;
  carg->is_det = is_det;
  carg->is_detsup = (SNetDetGetLevel() > 0);
  carg->peer = peer;
  carg->stopped = 0;
  carg->entity = SNetEntityCreate( ENTITY_collect, location, SNetLocvecGet(info),
                                   "<collector>", NULL, (void *) carg);
  return output;
}

