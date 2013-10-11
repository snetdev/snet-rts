#include "node.h"
#define _THREADING_H_
#include "handle_p.h"


static void SNetNodeBoxData(box_context_t *box)
{
  box_arg_t     *barg = LAND_NODE_SPEC(box->land, box);
  snet_handle_t   hnd;

  /* data record */
  hnd.rec = box->rec;
  /* set out signs */
  hnd.sign = barg->output_variants;
  /* mapping */
  hnd.mapping = NULL;
  /* set out descriptor */
  hnd.out_sd = box->outdesc;
  /* set variants */
  hnd.vars = barg->vars;
  /* box entity */
  hnd.ent = barg->entity;

  (*barg->boxfun)( &hnd);

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
  box_context_t         *box = NULL, *last = NULL;

  trace(__func__);

  /* Search for an unused context in the linked list. */
  for (box = land->context; box && box->busy; box = box->next) {
    /* Remember previous context. */
    last = box;
  }
  if (box == NULL) {
    box = SNetNewAlign(box_context_t);
    box->next = NULL;
    if (barg->concurrency >= 2) {
      /* Push collector landing. */
      SNetPushLanding(desc, land->collland);
      /* Open stream to collector. */
      box->outdesc = SNetStreamOpen(barg->output, desc);
      /* To allow concurrent unlocking we need our own source landing. */
      box->outdesc->source = SNetNewAlign(landing_t);
      *(box->outdesc->source) = *(desc->landing);
    } else {
      /* Open output stream. */
      box->outdesc = SNetStreamOpen(barg->output, desc);
    }
    box->rec = rec;
    box->land = desc->landing;
    box->busy = true;
    if (last) {
      last->next = box;
    } else {
      land->context = box;
    }
  } else {
    box->rec = rec;
    box->busy = true;
  }

  if (barg->concurrency >= 2) {
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

    /* Allow more workers. */
    unlock_landing(desc->landing);
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
    BAR();
    box->busy = false;
  }
}

/* Terminate a box landing. */
void SNetTermBox(landing_t *land, fifo_t *fifo)
{
  box_arg_t     *barg = LAND_NODE_SPEC(land, box);
  landing_box_t *lbox = LAND_SPEC(land, box);
  box_context_t *box;

  trace(__func__);

  while ((box = lbox->context) != NULL) {
    lbox->context = box->next;
    assert(box->busy == false);
    assert(box->outdesc != NULL);
    if (barg->concurrency >= 2) {
      SNetDelete(box->outdesc->source);
      box->outdesc->source = NULL;
    }
    SNetFifoPut(fifo, box->outdesc);
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

  trace(__func__);
  SNetStopStream(barg->output, fifo);
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
  snet_stream_t         *output = NULL;
  snet_stream_t         *outstream = NULL;
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

  outstream = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_box, location, &input, 1, &outstream, 1,
                     SNetNodeBox, SNetStopBox, SNetTermBox);

  barg = NODE_SPEC(node, box);
  barg->output = outstream;
  barg->boxfun = boxfun;
  barg->output_variants = output_variants;
  barg->vars = CreateVarList(output_variants);
  barg->boxname = boxname;
  barg->concurrency = concurrency;
  barg->is_det = is_det;
  barg->entity = SNetEntityCreate( ENTITY_box, location, SNetLocvecGet(info),
                                   barg->boxname, NULL, (void *) barg);

  if (concurrency >= 2) {
    output = SNetCollectorDynamic(outstream, location, info, is_det, node);
  } else {
    output = outstream;
  }

  return output;
}

