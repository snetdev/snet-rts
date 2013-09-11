/*
 * This is a deterministic version of the feedback combinator.
 * It is implemented by two landings. The first landing is
 * the access landing. It connects only to the second landing.
 * The second landing is the control landing. Here it is
 * decided whether to route records to the feedback loop
 * or outwards. See the non-deterministic feedback for details.
 */

#include <stdlib.h>
#include "node.h"
#include "detref.h"

/* Check if there are any records in the dripback loop left. */
static bool DripBackCheckBusy(landing_dripback2_t *db2)
{
  detref_t              *first;

  /* check if dripback loop is still active */
  while ((first = SNetFifoPeekFirst(&db2->detfifo)) != NULL) {
    BAR();
    if (first->refcount > 0) {
      break;
    }
    SNetFifoGet(&db2->detfifo);
    SNetFifoDone(&first->recfifo);
    SNetDelete(first);
  }

  return first ? true : false;
}

/* DripBack process a record. */
void SNetNodeDripBack(snet_stream_desc_t *desc, snet_record_t *rec)
{
  landing_t             *land = desc->landing;
  dripback_arg_t        *darg = LAND_NODE_SPEC(land, dripback);

  trace(__func__);

  if (land->type == LAND_dripback1) {
    landing_dripback1_t *db1 = LAND_SPEC(land, dripback1);
    landing_dripback2_t *db2 = LAND_SPEC(db1->dripback2, dripback2);
    SNetFifoPut(db1->recfifo, rec);
    if (FAA(&db2->queued, 1) == 0) {
      if (db1->controldesc == NULL) {
        SNetPushLanding(desc, db1->dripback2);
        db1->controldesc = SNetStreamOpen(darg->selfref, desc);
      }
      rec = SNetRecCreate(REC_wakeup);
      SNetWrite(&db1->controldesc, rec, true);
    }
  }
  else if (land->type == LAND_dripback2) {
    landing_dripback2_t *db2 = LAND_SPEC(land, dripback2);
    bool via_access = (DESC_STREAM(desc) == darg->selfref);

    assert(via_access || DESC_STREAM(desc) == darg->dripback);

    while (rec) {
      switch (REC_DESCR( rec)) {
        case REC_data:
          /* Test if record should go to the instance. */
          if (SNetFeedbackMatch( rec, darg->back_patterns, darg->guards)) {
            if (via_access) {
              /* Because record came from outside add a detref counter. */
              assert(db2->detfifo.head->next == NULL);
              SNetRecDetrefAdd(rec, ++(db2->entered), land, &db2->detfifo);
            } else {
              /* Record came from the instance. */
              assert(DATA_REC(rec, detref));
            }
            if (db2->instdesc == NULL) {
              /* Instance should come back to this landing. */
              if (SNetTopLanding(desc) != land) {
                SNetPushLanding(desc, land);
              }
              db2->instdesc = SNetStreamOpen(darg->instance, desc);
              if (SNetTopLanding(desc) == land) {
                SNetPopLanding(desc);
                SNetLandingDone(land);
              }
            }
            SNetWrite(&db2->instdesc, rec, false);
          } else {
            if (!via_access) {
              /* Record leaves the dripback loop. */
              assert(DATA_REC(rec, detref));
              SNetFeedbackLeave(rec, land, &db2->detfifo);
            }
            if (db2->outdesc == NULL) {
              db2->outdesc = SNetStreamOpen(darg->output, desc);
            }
            SNetWrite(&db2->outdesc, rec, false);
          }
          break;

        case REC_wakeup:
          assert(via_access);
          SNetRecDestroy(rec);
          break;

        case REC_terminate:
          assert(via_access);
          assert(db2->terminate == DripBackInitial);
          db2->terminate = DripBackDraining;
          SNetRecDestroy(rec);
          break;

        case REC_detref:
          if (DETREF_REC( rec, leave) == land &&
              DETREF_REC( rec, location) == SNetDistribGetNodeId())
          {
            assert(!via_access);
            SNetDetLeaveCheckDetref(rec, &db2->detfifo);
            if (DETREF_REC( rec, detref) == SNetFifoPeekFirst(&db2->detfifo)) {
              DripBackCheckBusy(db2);
            }
            SNetRecDestroy(rec);
          } else {
            assert(via_access);
            if (db2->outdesc == NULL) {
              db2->outdesc = SNetStreamOpen(darg->output, desc);
            }
            SNetWrite(&db2->outdesc, rec, false);
          }
          break;

        case REC_sync:
          SNetRecDestroy(rec);
          break;

        default:
          SNetRecUnknownEnt(__func__, rec, darg->entity);
      }
      rec = NULL;
      if (db2->state == DripBackBusy) {
        if (DripBackCheckBusy(db2) == false) {
          assert(db2->queued > 0);
          if (SAF(&db2->queued, 1) == 0) {
            db2->state = DripBackIdle;
          } else {
            rec = SNetFifoGet(&db2->recfifo);
            assert(rec);
            via_access = true;
          }
        }
      } else {
        assert(db2->state == DripBackIdle);
        assert(SNetFifoPeekFirst(&db2->detfifo) == NULL);
        if (db2->queued > 0) {
          rec = SNetFifoGet(&db2->recfifo);
          assert(rec);
          via_access = true;
          db2->state = DripBackBusy;
        }
      }
    }
    if (db2->terminate == DripBackDraining) {
      if (db2->state == DripBackIdle) {
        assert(db2->queued == 0 && DripBackCheckBusy(db2) == false);
        db2->terminate = DripBackTerminated;
      }
    }
    if (db2->terminate == DripBackTerminated) {
      if (db2->instdesc) {
        snet_stream_desc_t *desc = db2->instdesc;
        db2->instdesc = NULL;
        SNetDescDone(desc);
        SNetLandingDone(land);
      }
    }
  }
  else {
    assert(0);
  }
}

/* Terminate any kind of dripback landing. */
void SNetTermDripBack(landing_t *land, fifo_t *fifo)
{
  trace(__func__);

  if (land->type == LAND_dripback1) {
    landing_dripback1_t   *db1 = LAND_SPEC(land, dripback1);

    if (db1->controldesc) {
      snet_record_t *rec = SNetRecCreate(REC_terminate);
      land->worker = SNetThreadGetSelf();
      SNetWrite(&db1->controldesc, rec, false);
      SNetFifoPut(fifo, db1->controldesc);
    } else {
      SNetLandingDone(db1->dripback2);
    }
    SNetFreeLanding(land);
  }
  else if (land->type == LAND_dripback2) {
    landing_dripback2_t   *db2 = LAND_SPEC(land, dripback2);

    if (db2->instdesc) {
      fprintf(stderr, "%s: Premature termination of dripback2\n", __func__);
      SNetFifoPut(fifo, db2->instdesc);
      db2->instdesc = NULL;
    }
    if (db2->outdesc) {
      SNetFifoPut(fifo, db2->outdesc);
      db2->outdesc = NULL;
    }
    SNetFreeLanding(land);
  }
  else {
    assert(0);
  }
}

/* Destroy a dripback node. */
void SNetStopDripBack(node_t *node, fifo_t *fifo)
{
  dripback_arg_t *darg = NODE_SPEC(node, dripback);
  trace(__func__);
  if (darg->stopping == 0) {
    darg->stopping = 1;
    SNetStopStream(darg->instance, fifo);
  }
  else if (darg->stopping == 1) {
    darg->stopping = 2;
    SNetStopStream(darg->output, fifo);
    SNetVariantListDestroy(darg->back_patterns);
    SNetExprListDestroy(darg->guards);
    SNetStreamDestroy(darg->selfref);
    SNetEntityDestroy(darg->entity);
    SNetDelete(node);
  }
}

/* DripBack creation function */
snet_stream_t *SNetDripBack(
    snet_stream_t       *input,
    snet_info_t         *info,
    int                  location,
    snet_variant_list_t *back_patterns,
    snet_expr_list_t    *guards,
    snet_startup_fun_t   box_a)
{
  snet_stream_t  *output;
  node_t         *node;
  dripback_arg_t *darg;
  snet_locvec_t  *locvec;
  int             detlevel;

  trace(__func__);

  detlevel = SNetDetSwapLevel(0);
  locvec = SNetLocvecGet(info);
  SNetLocvecFeedbackEnter(locvec);

  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_dripback, location, &input, 1, &output, 1,
                     SNetNodeDripBack, SNetStopDripBack, SNetTermDripBack);
  darg = NODE_SPEC(node, dripback);

  /* fill in the node argument */
  darg->input = input;
  darg->output = output;
  darg->back_patterns = back_patterns;
  darg->guards = guards;
  darg->stopping = 0;

  /* Create the instance network */
  darg->instance = SNetNodeStreamCreate(node);
  SNetSubnetIncrLevel();
  darg->dripback = (*box_a)(darg->instance, info, location);
  SNetSubnetDecrLevel();
  STREAM_DEST(darg->dripback) = node;
  SNetNodeTableAdd(darg->dripback);

  /* Create one self-referencing stream. */
  darg->selfref = SNetNodeStreamCreate(node);
  STREAM_DEST(darg->selfref) = node;
  SNetNodeTableAdd(darg->selfref);

  darg->entity = SNetEntityCreate( ENTITY_fbdisp, location, locvec,
                                   "<feedback>", NULL, (void*)darg);

  SNetLocvecFeedbackLeave(locvec);
  SNetDetSwapLevel(detlevel);

  return output;
}

