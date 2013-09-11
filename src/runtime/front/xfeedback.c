/*
 * One feedback node is instantiated by four different kinds
 * of feedback landing:
 *
 * feedback1 is the landing which is accessed first when a record
 * comes from outside the feedback network. At this point a record
 * must be tested against the back pattern to decide if it should
 * go either to the feedback2 or the feedback4 landing.
 *
 * feedback2 is entry point to the instantiated feedback loop network.
 * Records arrive here via either the feedback1 or feedback3 landing.
 *
 * feedback3 is the return point for records which have traversed
 * the feedback loop. At this point a record must be tested against
 * the back pattern to decide if it should go either to the feedback2
 * or the feedback4 landing.
 *
 * feedback4 is the exit point of the feedback node. This is the
 * last feedback landing a record traverses before continueing
 * outside of the feedback network.
 *
 * Termination of this quartet of landings is a challenge.
 * For one because termination by reference counting is
 * tricky for a self-referencing loop. Another reason is
 * that the loop cannot terminate until there are no more
 * records traversing through it.
 *
 * The solution for the latter problem is to attach a detref
 * reference counting data structure to every record entering
 * the feedback network. This allows to detect the number of
 * records which are still in the loop. When all records
 * have left the feedback network the feedback3 landing
 * terminates the loop by closing its descriptors to the
 * streams to the feedback2 and feedback4 landing.
 */

#include <stdlib.h>
#include "node.h"
#include "detref.h"

/* Test if a record matches at least one of the loopback conditions.
 * This is executed for records from the backloop and from outside. */
bool SNetFeedbackMatch(
    snet_record_t       *rec,
    snet_variant_list_t *back_patterns,
    snet_expr_list_t    *guards)
{
  snet_expr_t *expr;
  snet_variant_t *pattern;

  LIST_ZIP_EACH(guards, back_patterns, expr, pattern) {
    if (SNetEevaluateBool( expr, rec) &&
        SNetRecPatternMatches(pattern, rec)) {
      return true;
    }
  }

  return false;
}

/* A record arriving via the loopback now leaves the feedback network.
 * Remove the detref structure and check for termination conditions. */
void SNetFeedbackLeave(snet_record_t *rec, landing_t *landing, fifo_t *detfifo)
{
  snet_stack_t          *stack;
  detref_t              *detref, *first;

  trace(__func__);

  // record must have a stack of detrefs
  if ((stack = DATA_REC(rec, detref)) == NULL) {
    SNetUtilDebugFatal("[%s]: missing stack.", __func__);
  }

  // stack must have at least one detref
  if ((detref = SNetStackPop(stack)) == NULL) {
    SNetUtilDebugFatal("[%s]: empty stack.", __func__);
  }
  if (SNetStackIsEmpty(stack)) {
    SNetStackDestroy(stack);
    DATA_REC(rec, detref) = NULL;
  }

  // detref must refer to this DetLeave node
  if (detref->leave != landing || detref->location != SNetDistribGetNodeId()) {
    SNetUtilDebugFatal("[%s]: leave %p.%d != landing %p.%d.", __func__,
                       detref->leave, detref->location,
                       landing, SNetDistribGetNodeId());
  }

  // reference counter must be at least two
  if (detref->refcount < DETREF_MIN_REFCOUNT) {
    SNetUtilDebugFatal("[%s]: refcnt %d < 1.", __func__, detref->refcount);
  }

  // decrease reference count
  if (DETREF_DECR(detref) == 1) {
    DETREF_DECR(detref);
  }
  assert(detref->refcount >= 0);

  // pop detrefs in sequence which have no more records in the loopback left.
  while ((first = SNetFifoPeekFirst(detfifo)) != NULL) {
    // stop processing if more records to come for this sequence counter
    if (first->refcount > 0) {
      break;
    }

    SNetFifoGet(detfifo);
    SNetFifoDone(&first->recfifo);
    SNetDelete(first);
  }
}

/* Check if there are any records in the feedback loop left. */
static void FeedbackCheckBusy(landing_feedback3_t *fb3)
{
  detref_t              *first;

  /* check if feedback loop is still active */
  while ((first = SNetFifoPeekFirst(&fb3->detfifo)) != NULL) {
    if (first->refcount > 0) {
      break;
    }
    SNetFifoGet(&fb3->detfifo);
    SNetFifoDone(&first->recfifo);
    SNetDelete(first);
  }

  if (first == NULL && fb3->terminate == FeedbackDraining) {
    fb3->terminate = FeedbackTerminating;
  }
}

/* Feedback forward a record. */
void SNetNodeFeedback(snet_stream_desc_t *desc, snet_record_t *rec)
{
  landing_t             *land = desc->landing;
  feedback_arg_t        *farg = LAND_NODE_SPEC(land, feedback);

  trace(__func__);

  if (land->type == LAND_feedback1) {
    landing_feedback1_t *fb1 = LAND_SPEC(land, feedback1);

    switch (REC_DESCR( rec)) {
      case REC_data:
        /* Test if record should go into the feedback loop. */
        if (SNetFeedbackMatch( rec, farg->back_patterns, farg->guards)) {
          /* Because record came from outside add a detref counter. */
          SNetRecDetrefAdd(rec, ++(fb1->counter), fb1->feedback3, fb1->detfifo);
          if (fb1->instdesc == NULL) {
            SNetPushLanding(desc, fb1->feedback2);
            fb1->instdesc = SNetStreamOpen(farg->selfref2, desc);
          }
          SNetWrite(&fb1->instdesc, rec, true);
        } else {
          if (fb1->outdesc == NULL) {
            SNetPushLanding(desc, fb1->feedback4);
            fb1->outdesc = SNetStreamOpen(farg->selfref4, desc);
          }
          SNetWrite(&fb1->outdesc, rec, true);
        }
        break;

      case REC_detref:
        if (fb1->outdesc == NULL) {
          SNetPushLanding(desc, fb1->feedback4);
          fb1->outdesc = SNetStreamOpen(farg->selfref4, desc);
        }
        SNetWrite(&fb1->outdesc, rec, true);
        break;

      case REC_sync:
        SNetRecDestroy(rec);
        break;

      default:
        SNetRecUnknownEnt(__func__, rec, farg->entity);
    }
  }
  else if (land->type == LAND_feedback2) {
    landing_feedback2_t *fb2 = LAND_SPEC(land, feedback2);
    if (fb2->instdesc == NULL) {
      SNetPushLanding(desc, fb2->feedback3);
      fb2->instdesc = SNetStreamOpen(farg->instance, desc);
    }
    switch (REC_DESCR( rec)) {
      case REC_data:
      case REC_detref:
        SNetWrite(&fb2->instdesc, rec, true);
        break;

      default:
        SNetRecUnknownEnt(__func__, rec, farg->entity);
    }
  }
  else if (land->type == LAND_feedback4) {
    landing_feedback4_t *fb4 = LAND_SPEC(land, feedback4);
    if (fb4->outdesc == NULL) {
      fb4->outdesc = SNetStreamOpen(farg->output, desc);
    }
    switch (REC_DESCR( rec)) {
      case REC_data:
      case REC_detref:
        SNetWrite(&fb4->outdesc, rec, true);
        break;

      default:
        SNetRecUnknownEnt(__func__, rec, farg->entity);
    }
  }
  else if (land->type == LAND_feedback3) {
    landing_feedback3_t *fb3 = LAND_SPEC(land, feedback3);
    if (fb3->terminate == FeedbackTerminated) {
      assert(REC_DESCR( rec) != REC_data);
      SNetRecDestroy(rec);
    } else {
      switch (REC_DESCR( rec)) {
        case REC_data:
          /* Test if record should go into the feedback loop. */
          if (SNetFeedbackMatch( rec, farg->back_patterns, farg->guards)) {
            if (fb3->instdesc == NULL) {
              SNetPushLanding(desc, fb3->feedback2);
              fb3->instdesc = SNetStreamOpen(farg->selfref2, desc);
            }
            /* send the record to the instance */
            SNetWrite(&fb3->instdesc, rec, false);
          } else {
            /* record leaves the feedback loop. */
            SNetFeedbackLeave(rec, land, &fb3->detfifo);
            if (fb3->outdesc == NULL) {
              SNetPushLanding(desc, fb3->feedback4);
              fb3->outdesc = SNetStreamOpen(farg->selfref4, desc);
            }
            /* forward record outwards */
            SNetWrite(&fb3->outdesc, rec, false);
            if (fb3->terminate) {
              assert(fb3->terminate == FeedbackDraining);
              FeedbackCheckBusy(fb3);
            }
          }
          break;

        case REC_detref:
          if (DETREF_REC( rec, leave) == land &&
              DETREF_REC( rec, location) == SNetDistribGetNodeId())
          {
            if (DETREF_REC( rec, seqnr) == -1L) {
              assert(fb3->terminate == FeedbackInitial);
              fb3->terminate = FeedbackDraining;
            } else {
              SNetDetLeaveCheckDetref(rec, &fb3->detfifo);
            }
            SNetRecDestroy(rec);
            FeedbackCheckBusy(fb3);
          }
          break;

        case REC_sync:
          SNetRecDestroy(rec);
          break;

        default:
          SNetRecUnknownEnt(__func__, rec, farg->entity);
      }
      if (fb3->terminate == FeedbackTerminating) {
        if (fb3->outdesc) {
          SNetDescDone(fb3->outdesc);
        }
        if (fb3->instdesc) {
          SNetDescDone(fb3->instdesc);
        }
        SNetLandingDone(fb3->feedback2);
        fb3->terminate = FeedbackTerminated;
      }
    }
  }
  else {
    assert(0);
  }
}

/* Terminate any kind of feedback landing. */
void SNetTermFeedback(landing_t *land, fifo_t *fifo)
{
  trace(__func__);
  if (land->type == LAND_feedback1) {
    landing_feedback1_t   *fb1 = LAND_SPEC(land, feedback1);
    landing_feedback3_t   *fb3 = LAND_SPEC(fb1->feedback3, feedback3);

    assert(fb3->terminate == FeedbackInitial);
    if (fb1->outdesc) {
      SNetFifoPut(fifo, fb1->outdesc);
    }
    if (fb1->instdesc) {
      /* Send the instance a fake REC_detref to wake it up, if need be. */
      const long fake_sequence_number = -1L;
      const int dest_location = SNetDistribGetNodeId();
      snet_record_t *recdet = SNetRecCreate(REC_detref, fake_sequence_number,
                                            dest_location, fb1->feedback3, NULL);
      land->worker = SNetThreadGetSelf();
      SNetWrite(&fb1->instdesc, recdet, false);
      SNetFifoPut(fifo, fb1->instdesc);
    }
    SNetLandingDone(fb1->feedback2);
    SNetLandingDone(fb1->feedback4);
    SNetLandingDone(fb1->feedback3);
    SNetFreeLanding(land);
  }
  else if (land->type == LAND_feedback2) {
    landing_feedback2_t   *fb2 = LAND_SPEC(land, feedback2);

    if (fb2->instdesc) {
      SNetFifoPut(fifo, fb2->instdesc);
    }
    SNetLandingDone(fb2->feedback3);
    SNetFreeLanding(land);
  }
  else if (land->type == LAND_feedback3) {
    landing_feedback3_t   *fb3 = LAND_SPEC(land, feedback3);

    SNetLandingDone(fb3->feedback4);
    SNetFreeLanding(land);
  }
  else if (land->type == LAND_feedback4) {
    landing_feedback4_t   *fb4 = LAND_SPEC(land, feedback4);

    if (fb4->outdesc) {
      SNetFifoPut(fifo, fb4->outdesc);
    }
    SNetFreeLanding(land);
  }
  else {
    assert(0);
  }
}

/* Destroy a feedback node. */
void SNetStopFeedback(node_t *node, fifo_t *fifo)
{
  feedback_arg_t *farg = NODE_SPEC(node, feedback);
  trace(__func__);
  if (farg->stopping == 0) {
    farg->stopping = 1;
    SNetStopStream(farg->instance, fifo);
  }
  else if (farg->stopping == 1) {
    farg->stopping = 2;
    SNetStopStream(farg->output, fifo);
    SNetVariantListDestroy(farg->back_patterns);
    SNetExprListDestroy(farg->guards);
    SNetStreamDestroy(farg->selfref2);
    SNetStreamDestroy(farg->selfref4);
    SNetEntityDestroy(farg->entity);
    SNetDelete(node);
  }
}

/* Feedback creation function */
snet_stream_t *SNetFeedback(
    snet_stream_t       *input,
    snet_info_t         *info,
    int                  location,
    snet_variant_list_t *back_patterns,
    snet_expr_list_t    *guards,
    snet_startup_fun_t   box_a)
{
  snet_stream_t  *output;
  node_t         *node;
  feedback_arg_t *farg;
  snet_locvec_t  *locvec;
  int             detlevel;

  trace(__func__);
  if (SNetFeedbackDeterministic()) {
    return SNetDripBack(input, info, location, back_patterns, guards, box_a);
  }

  detlevel = SNetDetSwapLevel(0);
  locvec = SNetLocvecGet(info);
  SNetLocvecFeedbackEnter(locvec);

  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_feedback, location, &input, 1, &output, 1,
                     SNetNodeFeedback, SNetStopFeedback, SNetTermFeedback);
  farg = NODE_SPEC(node, feedback);

  /* fill in the node argument */
  farg->input = input;
  farg->output = output;
  farg->back_patterns = back_patterns;
  farg->guards = guards;
  farg->stopping = 0;

  /* Create the instance network */
  farg->instance = SNetNodeStreamCreate(node);
  SNetSubnetIncrLevel();
  farg->feedback = (*box_a)(farg->instance, info, location);
  SNetSubnetDecrLevel();
  STREAM_DEST(farg->feedback) = node;
  SNetNodeTableAdd(farg->feedback);

  /* Create two self-referencing streams. */
  farg->selfref2 = SNetNodeStreamCreate(node);
  STREAM_DEST(farg->selfref2) = node;
  SNetNodeTableAdd(farg->selfref2);
  farg->selfref4 = SNetNodeStreamCreate(node);
  STREAM_DEST(farg->selfref4) = node;
  SNetNodeTableAdd(farg->selfref4);

  farg->entity = SNetEntityCreate( ENTITY_fbdisp, location, locvec,
                                   "<feedback>", NULL, (void*)farg);

  SNetLocvecFeedbackLeave(locvec);
  SNetDetSwapLevel(detlevel);

  return output;
}

