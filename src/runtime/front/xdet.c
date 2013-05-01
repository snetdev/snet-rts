#include <stdlib.h>
#include "node.h"
#include "detref.h"

/* Keep track of determinism during graph node construction.
 * Upon entering/leaving a deterministic network the level
 * is incremented/decremented. Feedback networks implement
 * special determinism level tricks not documented here.
 */
static int snet_determinism_level;

int SNetDetGetLevel(void)
{
  trace(__func__);
  return snet_determinism_level;
}

void SNetDetIncrLevel(void)
{
  trace(__func__);
  ++snet_determinism_level;
}

void SNetDetDecrLevel(void)
{
  trace(__func__);
  --snet_determinism_level;
  assert(snet_determinism_level >= 0);
}

/* Set determinism level and return the previous value. */
int SNetDetSwapLevel(int level)
{
  int old = snet_determinism_level;
  trace(__func__);
  assert(level >= 0);
  snet_determinism_level = level;
  return old;
}

/* Copy the stack of detref references from one record to another. */
void SNetRecDetrefCopy(snet_record_t *new_rec, snet_record_t *old_rec)
{
  snet_stack_t          *old_stack = DATA_REC(old_rec, detref);
  snet_stack_t          *new_stack;
  detref_t              *detref;
  snet_stack_node_t     *node;

  trace(__func__);
  assert(DATA_REC(new_rec, detref) == NULL);

  if (old_stack) {
    new_stack = SNetStackCreate();
    SNetStackCopy(new_stack, old_stack);
    STACK_FOR_EACH(new_stack, node, detref) {
      int n = detref->refcount;
      DETREF_INCR(detref);
      assert(n >= 2);
    }
    DATA_REC(new_rec, detref) = new_stack;
    BAR();
  }
}

/* Destroy the stack of detrefs for a record. */
void SNetRecDetrefDestroy(snet_record_t *rec, snet_stream_desc_t **desc_ptr)
{
  snet_stack_t  *stack = DATA_REC(rec, detref);
  detref_t      *detref;

  trace(__func__);
  if (stack) {
    DATA_REC(rec, detref) = NULL;
    STACK_POP_ALL(stack, detref) {
      assert(detref->refcount >= 2);
      if (DETREF_DECR(detref) == 1) {
        /* First create and send a REC_detref. */
        snet_record_t *tmp = SNetRecCreate(REC_detref,
                             detref->seqnr, detref->leave, detref);
        /* Then decrement refcount further down to zero. */
        assert(detref->refcount == 1);
        DETREF_DECR(detref);
        /* Now the collector may already have deallocated this detref. */
        SNetWrite(desc_ptr, tmp, false);
      }
    }
    SNetStackDestroy(stack);
  }
}

/* Add a sequence number to a record to support determinism. */
void SNetRecDetrefAdd(
    snet_record_t *rec,
    long seqnr,
    landing_t *leave,
    fifo_t *fifo)
{
  /* Allocate a new 'detref' structure. */
  detref_t *detref = SNetNewAlign(detref_t);

  trace(__func__);
  /* Reference count 2 has a special meaning to prevent a race condition:
   * the last single decrement ends at 1, then another decrement
   * must be done to get the final value of zero. */
  detref->refcount = 2;
  detref->seqnr = seqnr;
  detref->leave = leave;

  /* Initialize queue for pending output records. */
  SNetFifoInit(&detref->recfifo);

  /* Record needs a stack of detrefs. */
  if (DATA_REC(rec, detref) == NULL) {
    DATA_REC(rec, detref) = SNetStackCreate();
  }

  /* Push detref onto stack. */
  SNetStackPush(DATA_REC(rec, detref), detref);

  BAR();

  /* Notify collector of upcoming detref. */
  SNetFifoPut(fifo, detref);
}

/* Record needs support for determinism when entering a network. */
void SNetDetEnter(
    snet_record_t *rec,
    landing_detenter_t *land,
    bool is_det)
{
  trace(__func__);
  if (is_det) {
    /* In deterministic networks each record has its own counter value. */
    land->counter += 1;
    SNetRecDetrefAdd(rec, land->counter, land->collland, land->detfifo);
  }
  else {
    /* In non-deterministic networks, which are supporting determinism
     * in outer networks, sequence numbers can be reused if the record
     * sequence number for the outer network doesn't change.
     */
    snet_stack_t *stack = DATA_REC(rec, detref);
    if (stack) {
      detref_t *detref = SNetStackTop(stack);
      if (detref) {
        if (detref->seqnr != land->seqnr) {
          assert(detref->seqnr > land->seqnr);
          land->seqnr = detref->seqnr;
          land->counter += 1;
        }
        SNetRecDetrefAdd(rec, land->counter, land->collland, land->detfifo);
      } else {
        SNetUtilDebugFatal("[%s]: empty detref stack", __func__);
      }
    } else {
      SNetUtilDebugFatal("[%s]: no detref stack", __func__);
    }
  }
}

/* Output queued records if allowed, while preserving determinism. */
void SNetDetLeaveDequeue(landing_t *landing)
{
  landing_collector_t   *leave = LAND_SPEC(landing, collector);
  snet_record_t         *rec;
  detref_t              *detref;

  trace(__func__);
  /* Loop over detrefs until sequence numbers don't match. */
  while ((detref = SNetFifoPeekFirst(&leave->detfifo)) != NULL) {

    /* Verify sequence numbers are monotonically increasing. */
    if (detref->seqnr != leave->counter) {
      if (detref->seqnr < leave->counter) {
        SNetUtilDebugFatal("[%s]: detref seqnr %ld < leave counter %ld",
                               __func__, detref->seqnr, leave->counter);
      } else {
        /* Keep track of sequence number sequence. */
        leave->counter = detref->seqnr;
      }
    }

    /* Forward the queued records */
    while ((rec = SNetFifoGet(&detref->recfifo)) != NULL) {
      SNetWrite(&leave->outdesc, rec, false);
    }

    /* Deallocate empty detref */
    if (detref->refcount == 0) {
      /* Remove empty detref from FIFO. */
      SNetFifoGet(&leave->detfifo);
      /* Deallocate head node. */
      SNetFifoDone(&detref->recfifo);
      /* Deallocate detref. */
      SNetDelete(detref);
    }
    else {
      /* Abort the removal of detrefs as more records are coming. */
      break;
    }
  }

  /* Check for equal sequence numbers in subsequent detrefs. */
  if (detref) {
    fifo_node_t *next = FIFO_FIRST_NODE(&leave->detfifo);
    detref_t *item;

    while ((next = next->next) != NULL &&
           (item = (detref_t *) next->item) != NULL &&
           item->seqnr == detref->seqnr)
    {
      /* Forward the queued records */
      while ((rec = SNetFifoGet(&item->recfifo)) != NULL) {
        SNetWrite(&leave->outdesc, rec, false);
      }
    }
  }
}

/* Record leaves a deterministic network */
void SNetDetLeaveRec(snet_record_t *rec, landing_t *landing)
{
  snet_stack_t          *stack;
  detref_t              *detref;
  landing_collector_t   *leave = LAND_SPEC(landing, collector);

  trace(__func__);
  /* Record must have a stack of detrefs */
  if ((stack = DATA_REC(rec, detref)) == NULL) {
    SNetUtilDebugFatal("[%s]: missing stack", __func__);
  }

  /* stack must have at least one detref */
  if ((detref = SNetStackPop(stack)) == NULL) {
    SNetUtilDebugFatal("[%s]: empty stack", __func__);
  }

  /* destroy empty stacks */
  if (SNetStackIsEmpty(stack)) {
    SNetStackDestroy(stack);
    DATA_REC(rec, detref) = NULL;
  }

  /* detref must refer to this DetLeave node */
  if (detref->leave != landing) {
    SNetUtilDebugFatal("[%s]: leave %p != landing %p",
                          __func__, detref->leave, landing);
  }

  /* reference counter must be at least two */
  if (detref->refcount < 2) {
    SNetUtilDebugFatal("[%s]: refcnt %d < 1", __func__, detref->refcount);
  }

  /* Sequence number must be monotonically increasing. */
  assert(detref->seqnr >= leave->counter);

  /* Queue record. */
  SNetFifoPut(&detref->recfifo, rec);

  /* It no longer uses the detref: decrement reference count */
  if (DETREF_DECR(detref) == 1) {
    DETREF_DECR(detref);
  }

  /* Dequeue queued records */
  SNetDetLeaveDequeue(landing);
}

