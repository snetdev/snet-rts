#include "node.h"
#include <string.h>
#include <sys/param.h>

/* Copy dynamic locations due to indexed placement, from prev to new landing. */
static void CopyDynamicLocations(landing_t *land, snet_stream_desc_t *prev)
{
  /* Keep track of dynamic locations due to indexed placement. */
  if (land->node && land->node->loc_split_level) {
    const int my_level = land->node->loc_split_level;
    land->dyn_locs = SNetNewN(my_level, int);
    if (prev) {
      const int prev_level = DESC_NODE(prev)->loc_split_level;
      const int copy_count = MIN(my_level, prev_level);
      memcpy(land->dyn_locs, prev->landing->dyn_locs, copy_count * sizeof(int));
      if (my_level > copy_count) {
        assert(my_level == 1 + copy_count);
        assert(land->node->type == NODE_split || land->node->type == NODE_collector);
        land->dyn_locs[my_level - 1] = SNetDistribGetNodeId();
      }
      else if (my_level < prev_level) {
        assert(my_level + 1 == prev_level);
        assert(DESC_NODE(prev)->type == NODE_collector);
      }
    } else {
      assert(my_level == 1);
      assert(land->node->type == NODE_split);
      land->dyn_locs[my_level - 1] = SNetDistribGetNodeId();
    }
  } else {
    land->dyn_locs = NULL;
  }
}

/* Create a new landing with reference count 1. */
landing_t *SNetNewLanding(
    node_t *node,
    snet_stream_desc_t *prev,
    landing_type_t type)
{
  landing_t             *land   = SNetNewAlign(landing_t);
  snet_stack_node_t     *elem;
  landing_t             *item;

  trace(__func__);
  land->type    = type;
  land->node    = node;
  land->worker  = NULL;
  land->id      = 0;
  land->refs    = 1;

  SNetStackInit(&land->stack);
  if (prev) {
    /* Copy the small stack of indexed placement evaluations. */
    CopyDynamicLocations(land, prev);

    /* Copy the stack of landings and increment each landing reference count. */
    SNetStackCopy(&land->stack, &prev->landing->stack);
    STACK_FOR_EACH(&land->stack, elem, item) {
      LAND_INCR(item);
    }
  } else {
    land->dyn_locs = NULL;
  }

  return land;
}

/* Add a landing to the stack of future destination landings. */
void SNetPushLanding(snet_stream_desc_t *desc, landing_t *land)
{
  trace(__func__);
  assert(land->refs > 0);
  LAND_INCR(land);
  SNetStackPush(&desc->landing->stack, land);
}

/* Take the first landing from the stack of future landings. */
landing_t *SNetPopLanding(snet_stream_desc_t *desc)
{
  landing_t *land;
  trace(__func__);
  land = (landing_t *) SNetStackPop(&desc->landing->stack);
  assert(land && land->refs > 0);
  return land;
}

/* Peek non-destructively at the top of the stack of future landings. */
landing_t *SNetTopLanding(const snet_stream_desc_t *desc)
{
  trace(__func__);
  return (landing_t *) SNetStackTop(&desc->landing->stack);
}

/* Free the type specific dynamic memory resources of a landing. */
void SNetCleanupLanding(landing_t *land)
{
  landing_parallel_t    *lpar;
  landing_collector_t   *lcoll;

  trace(__func__);
  switch (land->type) {
    case LAND_box:
      SNetDeleteN(LAND_NODE_SPEC(land, box)->concurrency,
                  LAND_SPEC(land, box)->contexts);
      break;

    case LAND_collector:
      lcoll = LAND_SPEC(land, collector);
      SNetFifoDone(&lcoll->detfifo);
      break;

    case LAND_parallel:
      lpar = LAND_SPEC(land, parallel);
      SNetDelete(lpar->matchcounter);
      SNetDelete(lpar->outdescs);
      if (lpar->terminated) {
        SNetDelete(lpar->terminated);
      }
      break;

    case LAND_split:
      HashtabDestroy(LAND_SPEC(land, split)->hashtab);
      LAND_SPEC(land, split)->hashtab = NULL;
      break;

    case LAND_sync:
      SNetDelete(LAND_SPEC(land, sync)->storage);
      break;

    case LAND_zipper:
      assert(LAND_SPEC(land, zipper)->head == NULL);
      break;

    case LAND_feedback3:
      SNetFifoDone(&LAND_SPEC(land, feedback3)->detfifo);
      break;

    case LAND_dripback2:
      fprintf(stderr, "Cleanup for dripback2.\n");
      SNetFifoDone(&LAND_SPEC(land, dripback2)->detfifo);
      SNetFifoDone(&LAND_SPEC(land, dripback2)->recfifo);
      break;

    default:
      break;
  }

  while (SNetStackTop(&land->stack)) {
    landing_t *popped = SNetStackPop(&land->stack);
    SNetLandingDone(popped);
  }
  SNetMemFree(land->dyn_locs);
  land->dyn_locs = NULL;
}

/* Destroy a landing. */
void SNetFreeLanding(landing_t *land)
{
  trace(__func__);
  assert(land->refs == 0);
  SNetCleanupLanding(land);
  SNetDelete(land);
}

/* Decrease the reference count of a landing and collect garbage. */
void SNetLandingDone(landing_t *land)
{
  trace(__func__);
  assert(land->refs > 0);
  if (LAND_DECR(land) == 0) {
    if (land->node) {
      snet_stream_desc_t *desc;
      fifo_t fifo;
      SNetFifoInit(&fifo);
      (*land->node->term)(land, &fifo);
      while ((desc = SNetFifoGet(&fifo)) != NULL) {
        assert(desc->refs > 0);
        if (DESC_DECR(desc) == 0) {
          land = desc->landing;
          assert(land->refs > 0);
          if (LAND_DECR(land) == 0) {
            if (land->node) {
              (*land->node->term)(land, &fifo);
            } else {
              assert(land->type == LAND_remote);
              SNetFreeLanding(land);
            }
          }
          SNetStreamClose(desc);
        }
      }
      SNetFifoDone(&fifo);
    } else /* land->node == NULL */ {
      assert(land->type == LAND_remote);
      SNetFreeLanding(land);
    }
  }
}

/* Initialize a deterministic enter structure. */
static void SNetInitDetEnter(landing_detenter_t *detenter, landing_t *collland)
{
  landing_collector_t *land = LAND_SPEC(collland, collector);

  trace(__func__);
  detenter->counter = 0;
  detenter->seqnr = 0;
  detenter->collland = collland;
  detenter->detfifo = &(land->detfifo);
}

/* Construct a new collector landing. */
static landing_t *NewCollectorLanding(
    node_t *coll,
    snet_stream_desc_t *prev,
    landing_t *peer)
{
  landing_t             *land = SNetNewLanding(coll, prev, LAND_collector);
  landing_collector_t   *lcoll = LAND_SPEC(land, collector);

  trace(__func__);
  lcoll->outdesc = NULL;
  lcoll->counter = 1;
  lcoll->peer = peer;
  SNetFifoInit(&lcoll->detfifo);
  return land;
}

/* Create a new landing for a box. */
void SNetNewBoxLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev)
{
  box_arg_t             *barg = NODE_SPEC(DESC_DEST(desc), box);
  landing_box_t         *lbox;
  int                    i;

  trace(__func__);
  desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_box);
  lbox = DESC_LAND_SPEC(desc, box);
  lbox->concurrency = 0;
  lbox->contexts = SNetNewAlignN(barg->concurrency, box_context_union_t);
  for (i = 0; i < barg->concurrency; ++i) {
    box_context_t *box = &lbox->contexts[i].context;
    box->index = i;
    box->outdesc = NULL;
    box->rec = NULL;
    box->land = desc->landing;
    box->busy = false;
  }

  /* Create landing for future collector node */
  if (barg->concurrency >= 2) {
    lbox->collland = NewCollectorLanding(STREAM_DEST(barg->outputs[0]),
                                         prev, desc->landing);
  } else {
    lbox->collland = NULL;
  }

  SNetInitDetEnter(&(lbox->detenter), lbox->collland);
}

/* Create a new landing for a dispatcher. */
void SNetNewParallelLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev)
{
  const parallel_arg_t  *npar = NODE_SPEC(DESC_DEST(desc), parallel);
  landing_parallel_t    *lpar;
  int                    i;

  trace(__func__);
  desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_parallel);
  lpar = DESC_LAND_SPEC(desc, parallel);
  lpar->outdescs = SNetNewN(npar->num, snet_stream_desc_t *);
  for (i = 0; i < npar->num; ++i) {
    lpar->outdescs[i] = NULL;
  }
  lpar->matchcounter = SNetNewN(npar->num, match_count_t);
  lpar->colldesc = NULL;
  lpar->terminated = NULL;
  lpar->initialized = false;

  /* Create landing for future collector node */
  lpar->collland = NewCollectorLanding(STREAM_DEST(npar->collector),
                                       prev, desc->landing);

  SNetInitDetEnter(&(lpar->detenter), lpar->collland);
}

/* Create a new landing for a star. */
void SNetNewStarLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev)
{
  const star_arg_t      *nstar = NODE_SPEC(DESC_DEST(desc), star);
  landing_t             *land;
  landing_star_t        *lstar;

  trace(__func__);
  if (DESC_STREAM(desc) == nstar->input) {
    /* Create landing for this star node */
    desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_star);
    lstar = DESC_LAND_SPEC(desc, star);
    lstar->colldesc = NULL;
    lstar->instdesc = NULL;
    /* Create landing for future collector node */
    lstar->collland = NewCollectorLanding(STREAM_DEST(nstar->collector),
                                          prev, desc->landing);
    lstar->incarnation_count = 1;
    lstar->star_leader = true;
    SNetInitDetEnter(&(lstar->detenter), lstar->collland);
  }
  else if (DESC_STREAM(desc) == nstar->internal) {
    /* Pop landing for this star incarnation */
    desc->landing = SNetPopLanding(prev);
    assert(desc->landing);
    assert(desc->landing->type == LAND_star);
    assert(DESC_NODE(desc) == DESC_DEST(desc));
    assert(desc->landing->refs > 1);
    SNetLandingDone(desc->landing);
  }
  else {
    assert(0);
  }

  /* Create landing for future incarnation */
  land = SNetNewLanding(DESC_DEST(desc), prev, LAND_star);
  lstar = LAND_SPEC(land, star);
  lstar->colldesc = NULL;
  lstar->instdesc = NULL;
  /* Copy the landing for future collector */
  lstar->collland = DESC_LAND_SPEC(desc, star)->collland;
  LAND_INCR(lstar->collland);
  lstar->instland = NULL;
  lstar->incarnation_count = 1 + DESC_LAND_SPEC(desc, star)->incarnation_count;
  lstar->star_leader = false;
  SNetInitDetEnter(&(lstar->detenter), lstar->collland);
  DESC_LAND_SPEC(desc, star)->instland = land;
}

/* Create a new landing for a split. */
void SNetNewSplitLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev)
{
  const split_arg_t     *nsplit = NODE_SPEC(DESC_DEST(desc), split);
  landing_split_t       *lsplit;

  trace(__func__);
  /* Create landing for split node */
  desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_split);
  lsplit = DESC_LAND_SPEC(desc, split);
  lsplit->colldesc = NULL;
  lsplit->hashtab = HashtabCreate(4);
  lsplit->split_count = 0;
  /* Create landing for future collector node */
  lsplit->collland = NewCollectorLanding(STREAM_DEST(nsplit->collector),
                                         prev, desc->landing);

  SNetInitDetEnter(&(lsplit->detenter), lsplit->collland);
}

/* Create a new landing for a dripback. */
void SNetNewDripBackLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev)
{
  dripback_arg_t        *ndrip = NODE_SPEC(DESC_DEST(desc), dripback);
  landing_dripback1_t   *db1;
  landing_dripback2_t   *db2;

  trace(__func__);
  /* Create the dripback when arriving via the input stream */
  if (DESC_STREAM(desc) == ndrip->input) {
    /* Create access landing. */
    desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_dripback1);
    db1 = DESC_LAND_SPEC(desc, dripback1);
    db1->controldesc = NULL;

    /* Create control landing. */
    db1->dripback2 = SNetNewLanding(DESC_DEST(desc), prev, LAND_dripback2);
    db2 = LAND_SPEC(db1->dripback2, dripback2);
    db2->outdesc = NULL;
    db2->instdesc = NULL;
    SNetFifoInit(&db2->detfifo);
    SNetFifoInit(&db2->recfifo);
    db2->queued = 0;
    db2->entered = 0;
    db2->state = DripBackIdle;
    db2->terminate = DripBackInitial;

    /* Access notifies control about input of new records via recfifo. */
    db1->recfifo = &db2->recfifo;
  }
  else if (DESC_STREAM(desc) == ndrip->dripback) {
    /* Retrieve dripback2 landing from the landing stack */
    desc->landing = SNetPopLanding(prev);
    assert(desc->landing);
    assert(DESC_NODE(desc) == DESC_DEST(desc));
    assert(desc->landing->type == LAND_dripback2);
  }
  else if (DESC_STREAM(desc) == ndrip->selfref) {
    /* Retrieve dripback2 landing from the landing stack */
    desc->landing = SNetPopLanding(prev);
    assert(desc->landing);
    assert(DESC_NODE(desc) == DESC_DEST(desc));
    assert(desc->landing->type == LAND_dripback2);
  }
  else {
    assert(0);
  }
}

/* Create a new landing for a feedback. */
void SNetNewFeedbackLanding(snet_stream_desc_t *desc, snet_stream_desc_t *prev)
{
  feedback_arg_t        *nfeed = NODE_SPEC(DESC_DEST(desc), feedback);
  landing_feedback1_t   *fb1;
  landing_feedback2_t   *fb2;
  landing_feedback3_t   *fb3;
  landing_feedback4_t   *fb4;

  trace(__func__);
  /* Create the feedback when arriving via the input stream */
  if (DESC_STREAM(desc) == nfeed->input) {
    /* Create access landing. */
    desc->landing = SNetNewLanding(DESC_DEST(desc), prev, LAND_feedback1);
    fb1 = DESC_LAND_SPEC(desc, feedback1);
    fb1->counter = 1;
    fb1->outdesc = NULL;
    fb1->instdesc = NULL;

    /* Create exit landing. */
    fb1->feedback4 = SNetNewLanding(DESC_DEST(desc), prev, LAND_feedback4);
    fb4 = LAND_SPEC(fb1->feedback4, feedback4);
    fb4->outdesc = NULL;

    /* Create entry landing. */
    fb1->feedback2 = SNetNewLanding(DESC_DEST(desc), prev, LAND_feedback2);
    fb2 = LAND_SPEC(fb1->feedback2, feedback2);
    fb2->instdesc = NULL;

    /* Create return landing. */
    fb1->feedback3 = SNetNewLanding(DESC_DEST(desc), prev, LAND_feedback3);
    fb3 = LAND_SPEC(fb1->feedback3, feedback3);
    /* Access links to entry. */
    fb3->feedback2 = fb1->feedback2;
    LAND_INCR(fb3->feedback2);
    /* Access links to exit. */
    fb3->feedback4 = fb1->feedback4;
    LAND_INCR(fb3->feedback4);
    fb3->outdesc = NULL;
    fb3->instdesc = NULL;
    fb3->terminate = FeedbackInitial;
    SNetFifoInit(&fb3->detfifo);

    /* Access notifies return about entry of new records. */
    fb1->detfifo = &fb3->detfifo;

    /* Entry links to return. */
    fb2->feedback3 = fb1->feedback3;
    LAND_INCR(fb2->feedback3);
  }
  else if (DESC_STREAM(desc) == nfeed->feedback) {
    /* Retrieve feedback3 landing from the landing stack */
    desc->landing = SNetPopLanding(prev);
    assert(desc->landing);
    assert(DESC_NODE(desc) == DESC_DEST(desc));
    assert(desc->landing->type == LAND_feedback3);
  }
  else if (DESC_STREAM(desc) == nfeed->selfref2) {
    /* Retrieve feedback2 landing from the landing stack */
    desc->landing = SNetPopLanding(prev);
    assert(desc->landing);
    assert(DESC_NODE(desc) == DESC_DEST(desc));
    assert(desc->landing->type == LAND_feedback2);
  }
  else if (DESC_STREAM(desc) == nfeed->selfref4) {
    /* Retrieve feedback4 landing from the landing stack */
    desc->landing = SNetPopLanding(prev);
    assert(desc->landing);
    assert(DESC_NODE(desc) == DESC_DEST(desc));
    assert(desc->landing->type == LAND_feedback4);
  }
  else {
    assert(0);
  }
}

/* Give a string representation of the landing type */
const char *SNetLandingTypeName(landing_type_t ltype)
{
  return
#define NAME(x) #x
#define LAND(l) ltype == l ? NAME(l)+5 :
  LAND(LAND_siso)
  LAND(LAND_box)
  LAND(LAND_collector)
  LAND(LAND_feedback1)
  LAND(LAND_feedback2)
  LAND(LAND_feedback3)
  LAND(LAND_feedback4)
  LAND(LAND_parallel)
  LAND(LAND_split)
  LAND(LAND_star)
  LAND(LAND_sync)
  LAND(LAND_zipper)
  LAND(LAND_identity)
  LAND(LAND_input)
  LAND(LAND_garbage)
  LAND(LAND_observer)
  LAND(LAND_dripback1)
  LAND(LAND_dripback2)
  LAND(LAND_transfer)
  LAND(LAND_remote)
  NULL;
}

/* Give a string representation of the landing type */
const char *SNetLandingName(landing_t *land)
{
  return SNetLandingTypeName(land->type);
}

