/**
 * Nondeterministic Single-Entity Feedback Combinator
 *
 * This feedback combinator implements a real feedback loop,
 * with a non-deterministic merging of records in the
 * input stream with those from the feedback stream.
 *
 * For reasons of efficiency and simplicity, this
 * combinator is implemented with only one entity.
 * This is realized by closely keeping track of the number of records which
 * have already been written to the operand stream. When the stream buffer
 * is nearly full then a dummy sorting record is written. Together with an
 * internal queue this allows the entity to avoid having to block waiting
 * for room in a full output buffer when writing to the operand network.
 *
 * This combinator uses the following four streams:
 * (1) The input stream to the combinator.
 * (2) The operand stream which goes to the internal operand network or box.
 * (3) The result stream which comes from the operand network.
 * (3) The output stream which connects the combinator to the outside world.
 * Records which match the feedback pattern are appended to an internal queue.
 */

#include <assert.h>

#include "snetentities.h"

#include "expression.h"
#include "memfun.h"
#include "locvec.h"
#include "queue.h"
#include "entities.h"

#include "threading.h"
#include "distribution.h"

typedef struct feed_state {
  snet_stream_t        *input;
  snet_stream_t        *operand;
  snet_stream_t        *result;
  snet_stream_t        *output;
  snet_variant_list_t  *back_patterns;
  snet_expr_list_t     *guards;
  snet_entity_t        *entity;
  snet_record_t        *terminate;
  snet_stream_desc_t   *indesc;
  snet_stream_desc_t   *opdesc;
  snet_stream_desc_t   *redesc;
  snet_stream_desc_t   *outdesc;
  snet_queue_t         *queue;
  snet_streamset_t      set;
  int                   capacity;
  int                   section;
  int                   written;
  int                   acked;
  bool                  sorting;
} feed_state_t;

/* Check if a record matches the feedback pattern. */
static bool MatchesBackPattern(
  snet_record_t *rec,
  snet_variant_list_t *back_patterns,
  snet_expr_list_t *guards)
{
  snet_expr_t *expr;
  snet_variant_t *pattern;

  LIST_ZIP_EACH(guards, back_patterns, expr, pattern) {
    if (SNetEevaluateBool(expr, rec) &&
        SNetRecPatternMatches(pattern, rec)) {
      return true;
    }
  }

  return false;
}

/* Check for enough buffer space for two records. */
static bool FeedbackOperandWritable(feed_state_t *feed)
{
  int delta = feed->written - feed->acked;
  assert(delta >= 0 && delta <= 2 * feed->section);
  return delta + 1 < 2 * feed->section;
}

/* Write a record to the operand stream.
 * Write a sorting record twice per buffer capacity.
 */
static void FeedbackWriteOperand(feed_state_t *feed, snet_record_t *rec)
{
  /* Write must be non-blocking. */
  if (SNetStreamTryWrite(feed->opdesc, rec)) {
    assert(false);
  }
  /* Count records written. */
  feed->written += 1;
  /* Always use less than the buffer capacity: keep one spare. */
  assert(feed->written - feed->acked < 2 * feed->section);
  /* Every section'th record must be a sorting record. */
  if (((feed->written + 1) % feed->section) == 0 ||
      (feed->terminate && SNetQueueSize(feed->queue) == 0))
  {
    /* Count records written. */
    feed->written += 1;
    /* Send a sorting record. */
    rec = SNetRecCreate(REC_sort_end, 0, feed->written);
    if (SNetStreamTryWrite(feed->opdesc, rec)) {
      /* Write should have been non-blocking. */
      assert(false);
    }
    /* Last record was a sort. */
    feed->sorting = true;
  } else {
    /* Last record not a sort. */
    feed->sorting = false;
  }
}

/* Read one record from the operand network output stream. */
static void FeedbackReadResult(feed_state_t *feed)
{
  int level;
  snet_record_t *rec = SNetStreamRead(feed->redesc);
  switch (REC_DESCR(rec)) {
    case REC_data:
      if (MatchesBackPattern(rec, feed->back_patterns, feed->guards)) {
        SNetQueuePut(feed->queue, rec);
      } else {
        SNetStreamWrite(feed->outdesc, rec);
      }
      break;
    case REC_sort_end:
      level = SNetRecGetLevel(rec);
      if (level == 0) {
        int num = SNetRecGetNum(rec);
        int delta = feed->written - num;
        assert(delta >= 0 && delta <= 2 * feed->section);
        feed->acked = num;
        SNetRecDestroy(rec);
      } else {
        assert(level > 0);
        SNetRecSetLevel(rec, level - 1);
        SNetStreamWrite(feed->outdesc, rec);
      }
      break;
    case REC_terminate:
      assert(feed->terminate == NULL);
      assert(feed->indesc == NULL);
      assert(SNetQueueSize(feed->queue) == 0);
      assert(feed->written - feed->acked == 1);
      SNetStreamWrite(feed->outdesc, rec);
      SNetStreamsetRemove(&feed->set, feed->redesc);
      SNetStreamClose(feed->redesc, true);
      feed->redesc = NULL;
      break;
    case REC_sync:
      SNetStreamReplace(feed->redesc, SNetRecGetStream(rec));
      SNetRecDestroy(rec);
      break;
    default:
      SNetRecUnknownEnt(__func__, rec, feed->entity);
  }
}

/* Read one record from the combinator input stream.
 * Only do this when the operand stream is non-full. */
static void FeedbackReadInput(feed_state_t *feed)
{
  snet_record_t *rec = SNetStreamRead(feed->indesc);
  switch (REC_DESCR(rec)) {
    case REC_data:
      FeedbackWriteOperand(feed, rec);
      break;
    case REC_sort_end:
      /* increase the level and forward */
      SNetRecSetLevel(rec, SNetRecGetLevel(rec)+1);
      FeedbackWriteOperand(feed, rec);
      break;
    case REC_terminate:
      assert(feed->terminate == NULL);
      feed->terminate = rec;
      SNetStreamsetRemove(&feed->set, feed->indesc);
      SNetStreamClose(feed->indesc, true);
      feed->indesc = NULL;
      break;
    case REC_sync:
      SNetStreamReplace(feed->indesc, SNetRecGetStream(rec));
      SNetRecDestroy(rec);
      break;
    default:
      SNetRecUnknownEnt(__func__, rec, feed->entity);
  }
}

/* Poll all input streams for readability and read one record. */
static void FeedbackReadAny(feed_state_t *feed)
{
  /* Input may be closed. */
  if (feed->indesc == NULL) {
    /* Blocking read from operand. */
    FeedbackReadResult(feed);
  } else {
    snet_stream_desc_t *poll = SNetStreamPoll(&feed->set);
    assert(poll);
    /* Always prefer result stream. */
    if (SNetStreamPeek(feed->redesc)) {
      FeedbackReadResult(feed);
    } else {
      assert(poll == feed->indesc);
      FeedbackReadInput(feed);
    }
  }
}

/* Take records from the queue and write them to the operand stream. */
static void FeedbackFeedback(feed_state_t *feed)
{
  do {
    snet_record_t *rec = (snet_record_t *) SNetQueueGet(feed->queue);
    assert(rec);
    FeedbackWriteOperand(feed, rec);
  } while (FeedbackOperandWritable(feed) && SNetQueueSize(feed->queue) > 0);
}

/* Feedback thread main loop. */
static void FeedbackTask(snet_entity_t *ent, void *arg)
{
  feed_state_t *feed = (feed_state_t *) arg;
  (void) ent; /* NOT USED */

  feed->indesc = SNetStreamOpen(feed->input, 'r');
  feed->opdesc = SNetStreamOpen(feed->operand, 'w');
  feed->redesc = SNetStreamOpen(feed->result, 'r');
  feed->outdesc = SNetStreamOpen(feed->output, 'w');
  feed->queue = SNetQueueCreate();

  /* Construct a streamset from the two input streams. */
  feed->set = NULL;
  SNetStreamsetPut(&feed->set, feed->indesc);
  SNetStreamsetPut(&feed->set, feed->redesc);

  /* Loop while the operand is alive. */
  while (feed->redesc) {

    /* Always prefer to feed back records from the queue. */
    if (SNetQueueSize(feed->queue) > 0) {
      /* Check for write buffer non-full. */
      if (FeedbackOperandWritable(feed)) {
        /* Loop back records. */
        FeedbackFeedback(feed);
      } else {
        /* If buffer full then do a blocking read on result. */
        FeedbackReadResult(feed);
      }
    }
    else {
      /* If no records in transit, check for termination. */
      if (feed->terminate) {
        if (feed->written == feed->acked) {
          SNetStreamWrite(feed->opdesc, feed->terminate);
          feed->terminate = NULL;
          feed->written += 1;
        }
        else if (feed->sorting == false) {
          snet_record_t *rec;
          feed->written += 1;
          rec = SNetRecCreate(REC_sort_end, 0, feed->written);
          if (SNetStreamTryWrite(feed->opdesc, rec)) {
            assert(false);
          }
          feed->sorting = true;
        } else {
          /* A blocking read on result. */
          FeedbackReadResult(feed);
        }
      }
      else if (FeedbackOperandWritable(feed)) {
        /* Wait for any input. */
        FeedbackReadAny(feed);
      } else {
        /* A blocking read on result. */
        FeedbackReadResult(feed);
      }
    }
  }

  SNetVariantListDestroy(feed->back_patterns);
  SNetExprListDestroy(feed->guards);

  SNetStreamClose(feed->opdesc, false);
  SNetStreamClose(feed->outdesc, false);
  SNetQueueDestroy(feed->queue);
  SNetDelete(feed);
}


/****************************************************************************/
/* CREATION FUNCTION OF THE NONDETERMINISTIC SINGLE-THREAD FEEDBACK ENTITY  */
/****************************************************************************/

snet_stream_t *SNetFeedback(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *back_patterns,
    snet_expr_list_t *guards,
    snet_startup_fun_t box_a)
{
  snet_stream_t *output;
  snet_locvec_t *locvec;

  locvec = SNetLocvecGet(info);
  SNetLocvecFeedbackEnter(locvec);

  input = SNetRouteUpdate(info, input, location);
  if (SNetDistribIsNodeLocation(location)) {
    feed_state_t *feed = SNetNew(feed_state_t);

    feed->capacity = SNET_STREAM_DEFAULT_CAPACITY;
    assert(feed->capacity >= 6 /*min.stream.cap.*/);
    feed->section = (feed->capacity - 1) / 2;
    feed->written = 0;
    feed->acked   = 0;
    feed->sorting = 0;

    feed->back_patterns = back_patterns;
    feed->guards = guards;

    feed->input = input;
    feed->operand = SNetStreamCreate(feed->capacity);
    feed->output = SNetStreamCreate(0);
    output = feed->output;

    /* create the instance network */
    feed->result = box_a(feed->operand, info, location);
    feed->result = SNetRouteUpdate(info, feed->result, location);

    feed->terminate = NULL;
    feed->entity = SNetEntityCreate(ENTITY_fbnond, location, locvec,
                                     "<fbnond>", FeedbackTask, (void*) feed);
    SNetThreadingSpawn(feed->entity);

  } else {
    SNetVariantListDestroy(back_patterns);
    SNetExprListDestroy(guards);
    output = box_a(input, info, location);
    output = SNetRouteUpdate(info, output, location);
  }

  SNetLocvecFeedbackLeave(locvec);

  return output;
}
