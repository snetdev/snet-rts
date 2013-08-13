#include "node.h"
#define _THREADING_H_
#include "handle_p.h"

#ifdef SNET_DEBUG_COUNTERS
#include "debugcounters.h"
#endif

//#define DBG_RT_TRACE_BOX_TIMINGS

#ifdef DBG_RT_TRACE_BOX_TIMINGS
#include <sys/time.h>
#endif

static void SNetNodeBoxData(box_context_t *box)
{
  box_arg_t     *barg = LAND_NODE_SPEC(box->land, box);

#ifdef DBG_RT_TRACE_BOX_TIMINGS
  static struct timeval tv_in;
  static struct timeval tv_out;
#endif
#ifdef SNET_DEBUG_COUNTERS
  snet_time_t time_in;
  snet_time_t time_out;
  long mseconds;
#endif
  snet_handle_t   hnd;

  /* set out descriptor */
  hnd.out_sd = box->outdesc;
  /* set out signs */
  hnd.sign = barg->output_variants;
  /* mapping */
  hnd.mapping = NULL;
  /* set variants */
  hnd.vars = barg->vars;
  /* box entity */
  hnd.ent = barg->entity;
  /* data record */
  hnd.rec = box->rec;

#ifdef DBG_RT_TRACE_BOX_TIMINGS
  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNoticeEnt( barg->entity,
      "[BOX] Firing box function at %lf.",
      tv_in.tv_sec + tv_in.tv_usec / 1000000.0
      );
#endif
#ifdef SNET_DEBUG_COUNTERS
  SNetDebugTimeGetTime(&time_in);
#endif

  (*barg->boxfun)( &hnd);

#ifdef DBG_RT_TRACE_BOX_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNoticeEnt( barg->entity,
      "[BOX] Return from box function after %lf sec.",
      (tv_out.tv_sec - tv_in.tv_sec) + (tv_out.tv_usec - tv_in.tv_usec)*1e-6);
#endif
#ifdef SNET_DEBUG_COUNTERS
  SNetDebugTimeGetTime(&time_out);
  mseconds = SNetDebugTimeDifferenceInMilliseconds(&time_in, &time_out);
  SNetDebugCountersIncreaseCounter(mseconds, SNET_COUNTER_TIME_BOX);
#endif

  if (REC_DESCR( box->rec) == REC_data) {
    SNetRecDetrefDestroy(box->rec, &box->outdesc);
  }
  SNetRecDestroy( box->rec);
}


/* Process one record. */
void SNetNodeBox(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const box_arg_t       *barg = DESC_NODE_SPEC(desc, box);
  landing_box_t         *land = DESC_LAND_SPEC(desc, box);
  box_context_t         *box = NULL;
  int                    i, conc;

  trace(__func__);

  if (barg->concurrency == 1) {
    box = &land->contexts[0].context;
    box->busy = true;
    box->rec = rec;
    if (!box->outdesc) {
      box->outdesc = SNetStreamOpen(barg->outputs[box->index], desc);
    }
  }
  else {

    /* Find an unused box context. */
    i = 0;
    while (land->contexts[i].context.busy) {
      ++i;
      assert(i < barg->concurrency);
    }
    box = &land->contexts[i].context;
    box->busy = true;
    /* if (!CAS(&box->busy, false, true)) {
      assert(0);
    } */
    box->rec = rec;

    /* Open output first. */
    if (!box->outdesc) {
      /* Push collector landing. */
      SNetPushLanding(desc, land->collland);

      box->outdesc = SNetStreamOpen(barg->outputs[box->index], desc);

      /* To allow concurrent unlocking we need our own source landing. */
      box->outdesc->source = SNetNewAlign(landing_t);
      *(box->outdesc->source) = *(desc->landing);
    }

    /* Make concurrent unlocking work. */
    box->outdesc->source->worker = desc->landing->worker;

    /* Data records may need determinism. */
    if (barg->is_det) {
      if (REC_DESCR(rec) == REC_data ||
          REC_DESCR(rec) == REC_trigger_initialiser)
      {
        SNetDetEnter(rec, &land->detenter, true, barg->entity);
      }
    }

    /* Increase concurrency counter. */
    conc = AAF(&(land->concurrency), 1);

    /* Allow more workers if below concurrency limit. */
    if (conc < barg->concurrency) {
      unlock_landing(desc->landing);
    }
  }

  switch (REC_DESCR(rec)) {
    case REC_data:
    case REC_trigger_initialiser:
      SNetNodeBoxData(box);
      break;

    case REC_detref:
      /* forward record */
      SNetWrite(&box->outdesc, rec, true);
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, barg->entity);
  }

  if (barg->concurrency == 1) {
    box->rec = NULL;
    box->busy = false;
    unlock_landing(desc->landing);
  }
  else {
    box->rec = NULL;
    box->outdesc->source->worker = NULL;
    box->outdesc->source->id = 0;
    box->busy = false;
    /* if (!CAS(&box->busy, true, false)) {
      assert(0);
    } */

    /* Decrease concurrency counter. */
    conc = FAS(&(land->concurrency), 1);

    /* Only unlock when leaving a full house. */
    if (conc == barg->concurrency) {
      unlock_landing(desc->landing);
    }
  }
}

/* Terminate a box landing. */
void SNetTermBox(landing_t *land, fifo_t *fifo)
{
  box_arg_t     *barg = LAND_NODE_SPEC(land, box);
  landing_box_t *lbox = LAND_SPEC(land, box);
  box_context_t *box;
  int            i;

  trace(__func__);

  for (i = 0; i < barg->concurrency; ++i) {
    box = &lbox->contexts[i].context;
    assert(box->index == i);
    assert(box->busy == false);
    if (box->outdesc) {
      if (barg->concurrency >= 2) {
        SNetDelete(box->outdesc->source);
        box->outdesc->source = NULL;
      }
      SNetFifoPut(fifo, box->outdesc);
    }
  }
  if (lbox->collland) {
    SNetLandingDone(lbox->collland);
  }
  SNetFreeLanding(land);
}

/* Destroy a box node. */
void SNetStopBox(node_t *node, fifo_t *fifo)
{
  box_arg_t    *barg = NODE_SPEC(node, box);
  int           i;

  trace(__func__);
  for (i = 0; i < barg->concurrency; ++i) {
    SNetStopStream(barg->outputs[i], fifo);
  }
  SNetDeleteN(barg->concurrency, barg->outputs);
  SNetIntListListDestroy(barg->output_variants);
  SNetVariantListDestroy(barg->vars);
  SNetEntityDestroy(barg->entity);
  SNetDelete(node);
}

/* Create box variant list. */
static snet_variant_list_t *CreateVarList(snet_int_list_list_t *output_variants)
{
  snet_variant_list_t   *vlist = SNetVariantListCreate(0);
  int                    i, j;

  for (i = 0; i < SNetIntListListLength( output_variants); i++) {
    snet_int_list_t *l = SNetIntListListGet( output_variants, i);
    snet_variant_t *v = SNetVariantCreateEmpty();
    for (j = 0; j < SNetIntListLength(l); j += 2) {
      switch (SNetIntListGet( l, j)) {
        case field:
          SNetVariantAddField( v, SNetIntListGet( l, j+1));
          break;
        case tag:
          SNetVariantAddTag( v, SNetIntListGet( l, j+1));
          break;
        case btag:
          SNetVariantAddBTag( v, SNetIntListGet( l, j+1));
          break;
        default:
          assert(0);
      }
    }
    SNetVariantListAppendEnd( vlist, v);
  }
  return vlist;
}

/* Create a box node. */
snet_stream_t *SNetBox( snet_stream_t *input,
                        snet_info_t *info,
                        int location,
                        const char *boxname,
                        snet_box_fun_t boxfun,
                        snet_exerealm_create_fun_t exerealm_create,
                        snet_exerealm_update_fun_t exerealm_update,
                        snet_exerealm_destroy_fun_t exerealm_destroy,
                        snet_int_list_list_t *output_variants)
{
  int                    i;
  snet_stream_t         *output = NULL;
  snet_stream_t        **outstreams = NULL;
  node_t                *node;
  box_arg_t             *barg;
  int                    concurrency;
  bool                   is_det = false;

  trace(__func__);

  concurrency = SNetGetBoxConcurrency(boxname, &is_det);
  if (SNetVerbose()) {
    printf("Creating box \"%s\" with concurrency %d%c.\n",
            boxname, concurrency, is_det ? 'D' : 'N');
  }

  outstreams = SNetNewN(concurrency, snet_stream_t *);
  for (i = 0; i < concurrency; ++i) {
    outstreams[i] = SNetStreamCreate(0);
  }
  node = SNetNodeNew(NODE_box, location, &input, 1, outstreams, concurrency,
                     SNetNodeBox, SNetStopBox, SNetTermBox);

  barg = NODE_SPEC(node, box);
  barg->outputs = outstreams;
  barg->boxfun = boxfun;
  barg->output_variants = output_variants;
  barg->vars = CreateVarList(output_variants);
  barg->boxname = boxname;
  barg->concurrency = concurrency;
  barg->is_det = is_det;
  barg->entity = SNetEntityCreate( ENTITY_box, location, SNetLocvecGet(info),
                                   barg->boxname, NULL, (void *) barg);

  if (concurrency >= 2) {
    output = SNetCollectorStatic(concurrency, outstreams, location, info,
                                 is_det, node);
  } else {
    output = outstreams[0];
  }

  return output;
}

