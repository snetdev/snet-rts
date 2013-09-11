#include "node.h"

/* Test if a record matches one of several exit conditions */
static bool MatchesExitPattern(
    snet_record_t       *rec,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t    *guards)
{
  int i;
  snet_variant_t *pattern;

  LIST_ENUMERATE(exit_patterns, i, pattern) {
    if (SNetRecPatternMatches(pattern, rec) &&
        SNetEevaluateBool( SNetExprListGet( guards, i), rec)) {
      return true;
    }
  }

  return false;
}

/* A star writes to either a collector or an instance. */
typedef enum {
  WriteCollector,
  WriteInstance,
} star_write_t;

/* Send a record to a stream which may need to be opened */
static void StarWrite(star_write_t kind, snet_record_t *rec, 
                      snet_stream_desc_t *desc)
{
  const star_arg_t      *sarg = DESC_NODE_SPEC(desc, star);
  landing_star_t        *land = DESC_LAND_SPEC(desc, star);

  switch (kind) {

    case WriteCollector:
      /* check if open */
      if (!land->colldesc) {
        /* first landing must be collector */
        if (SNetTopLanding(desc) != land->collland) {
          SNetPushLanding(desc, land->collland);
        }
        land->colldesc = SNetStreamOpen(sarg->collector, desc);
      }
      SNetWrite(&land->colldesc, rec, true);
      break;

    case WriteInstance:
      /* check if open */
      if (!land->instdesc) {
        /* first landing must be instance */
        if (SNetTopLanding(desc) != land->instland) {
          SNetPushLanding(desc, land->instland);
        }
        land->instdesc = SNetStreamOpen(sarg->instance, desc);
        /* remove instance landing */
        if (SNetTopLanding(desc) == land->instland) {
          SNetPopLanding(desc);
          SNetLandingDone(land->instland);
        }
        /* prevent double deallocation */
        land->instland = NULL;
        /* emit output record to instance */
        SNetWrite(&land->instdesc, rec, true);
      } else {
        /* A star is superfluous when the next landing is its incarnation. */
        bool superfluous = (desc->landing->node == land->instdesc->landing->node);

        /* emit output record to instance */
        SNetWrite(&land->instdesc, rec, (superfluous == false));

        /* If instance has been garbage collected then dissolve. */
        if (superfluous) {
          /* Possibly forward leader status to next incarnation. */
          rec = (land->star_leader && (sarg->is_det | sarg->is_detsup)) ?
                SNetRecCreate(REC_star_leader, land->detenter.counter,
                                               land->detenter.seqnr) : NULL;
          land->star_leader = false;

          /* Discard stream towards collector. */
          if (land->colldesc) {
            SNetDescDone(land->colldesc);
          }

          /* Discard collector landing. */
          SNetLandingDone(land->collland);

          /* Change landing into identity, which can be garbage collected. */
          SNetBecomeIdentity(desc->landing, land->instdesc);

          /* Finally, notify incarnation of new leader status. */
          if (rec) {
            SNetWrite(&(DESC_LAND_SPEC(desc, identity)->outdesc), rec, true);
          }
        }
      }
      break;

    default:
      assert(0);
  }
}

/* Star component: forward record to instance or to collector. */
void SNetNodeStar(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const star_arg_t      *sarg = DESC_NODE_SPEC(desc, star);
  landing_star_t        *land = DESC_LAND_SPEC(desc, star);

  trace(__func__);

  switch (REC_DESCR(rec)) {
    case REC_data:
      if (land->star_leader && (sarg->is_det | sarg->is_detsup)) {
        SNetDetEnter(rec, &land->detenter, sarg->is_det, sarg->entity);
      }
      if (MatchesExitPattern( rec, sarg->exit_patterns, sarg->guards)) {
        StarWrite(WriteCollector, rec, desc);
      } else {
        StarWrite(WriteInstance, rec, desc);
      }
      break;

    case REC_detref:
      /* Forward record */
      StarWrite(WriteCollector, rec, desc);
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    case REC_star_leader:
      land->star_leader = true;
      land->detenter.counter = LEADER_REC(rec, counter);
      land->detenter.seqnr   = LEADER_REC(rec, seqnr  );
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, sarg->entity);
  }
}

/* Terminate a star landing. */
void SNetTermStar(landing_t *land, fifo_t *fifo)
{
  landing_star_t        *lstar = LAND_SPEC(land, star);

  trace(__func__);

  if (lstar->instdesc) {
    SNetFifoPut(fifo, lstar->instdesc);
  }
  if (lstar->colldesc) {
    SNetFifoPut(fifo, lstar->colldesc);
  }
  if (lstar->instland) {
    SNetLandingDone(lstar->instland);
  }

  SNetLandingDone(lstar->collland);
  SNetFreeLanding(land);
}

/* Destroy a star node. */
void SNetStopStar(node_t *node, fifo_t *fifo)
{
  star_arg_t *sarg = NODE_SPEC(node, star);
  trace(__func__);
  if (!sarg->stopping) {
    sarg->stopping = 1;
    SNetStopStream(sarg->instance, fifo);
  }
  else if (sarg->stopping == 1) {
    sarg->stopping = 2;
    SNetStopStream(sarg->collector, fifo);
    SNetExprListDestroy(sarg->guards);
    SNetVariantListDestroy(sarg->exit_patterns);
    SNetEntityDestroy(sarg->entity);
    SNetDelete(node);
  }
}

/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/

/**
 * Convenience function for creating
 * Star, DetStar, StarIncarnate or DetStarIncarnate,
 * dependent on parameters is_incarnate and is_det.
 */
static snet_stream_t *CreateStar(
    snet_stream_t       *input,
    snet_info_t         *info,
    int                  location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t    *guards,
    snet_startup_fun_t   box_a,
    snet_startup_fun_t   box_b,
    bool                 is_incarnate,
    bool                 is_det)
{
  snet_stream_t *output;
  node_t        *node;
  star_arg_t    *sarg;
  snet_locvec_t *locvec;

  locvec = SNetLocvecGet(info);
  if (!is_incarnate) {
    SNetLocvecStarEnter(locvec);
  } else {
    assert(false);
  }

  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_star, location, &input, 1, &output, 1,
                     SNetNodeStar, SNetStopStar, SNetTermStar);
  sarg = NODE_SPEC(node, star);
  sarg->input = input;
  sarg->collector = output;
  sarg->exit_patterns = exit_patterns;
  sarg->guards = guards;
  sarg->is_incarnate = is_incarnate;
  sarg->is_det = is_det;
  sarg->is_detsup = (SNetDetGetLevel() > 0);
  sarg->stopping = 0;

  /* create operand A */
  sarg->instance = SNetNodeStreamCreate(node);
  (void) SNetLocvecStarSpawn(locvec);
  SNetSubnetIncrLevel();
  sarg->internal = (*box_a)(sarg->instance, info, location);
  SNetSubnetDecrLevel();
  (void) SNetLocvecStarSpawnRet(locvec);
  /* direct destination of operand back to this node */
  STREAM_DEST(sarg->internal) = node;

  /* Is this a Star followed by only a Sync? */
  if (SNetZipperEnabled() &&
      NODE_TYPE(STREAM_DEST(sarg->instance)) == NODE_sync &&
      STREAM_FROM(sarg->internal) == STREAM_DEST(sarg->instance))
  {
    /* Replace the combination of star + sync with a fused sync-star. */
    sync_arg_t *sync = NODE_SPEC(STREAM_DEST(sarg->instance), sync);
    /* if (SNetVerbose()) {
      printf("Replacing a star + sync with a fused sync-star.\n");
    } */
    output = SNetZipper(input, info, location, exit_patterns, guards,
                        sync->patterns, sync->guard_exprs);
    SNetEntityDestroy(sync->entity);
    SNetVariantDestroy(sync->merged_pattern);
    SNetDelete(STREAM_DEST(sarg->instance));
    SNetStreamDestroy(sarg->instance);
    SNetStreamDestroy(sarg->internal);
    SNetStreamDestroy(sarg->collector);
    SNetMemFree(node);
  } else {
    sarg->entity = SNetEntityCreate( ENTITY_star, location, locvec,
                                     "<star>", NULL, (void *) sarg);
    if (!is_incarnate) {
      /* the "top-level" star also creates a collector */
      output = SNetCollectorDynamic(sarg->collector, location, info,
                                    is_det, node);
      SNetNodeTableAdd(sarg->internal);
    }
  }

  if (!is_incarnate) {
    SNetLocvecStarLeave(locvec);
  }

  return output;
}

/* Star creation function */
snet_stream_t *SNetStar( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  trace(__func__);
  return CreateStar( input, info, location, exit_patterns, guards, box_a, box_b,
                     false, /* not incarnate */
                     false); /* not det */
}

/* Star incarnate creation function */
snet_stream_t *SNetStarIncarnate( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  assert(false);
  return NULL;
}

/* Det Star creation function */
snet_stream_t *SNetStarDet(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  snet_stream_t *outstream;
  trace(__func__);
  SNetDetIncrLevel();
  outstream = CreateStar( input, info, location, exit_patterns, guards, box_a, box_b,
                          false, /* not incarnate */
                          true); /* is det */
  SNetDetDecrLevel();
  return outstream;
}

/* Det star incarnate creation function */
snet_stream_t *SNetStarDetIncarnate(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *exit_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  assert(false);
  return NULL;
}

