#ifndef _DETREF_H
#define _DETREF_H

#if defined(_XFIFO_H) && defined(_NODE_H_)

/*
 * A reference counter for deterministic ordering:
 *
 * Every record that enters a deterministic network
 * is associated with a new 'detref' structure
 * to keep track of its ordering with respect to
 * previous and subsequent records.
 * Records which are created within the deterministic network
 * are always created in the context of an existing record
 * which already carries a 'detref' structure.
 * The newly created records are then also associated with
 * this same 'detref' structure and for each new record the
 * reference counter in this 'detref' structure is incremented.
 * For each records which is deleted the reference counter
 * is decremented. Therefore, at each moment the detref structure
 * tells how many records correspond to that sorting position
 * in the deterministic output sorting.
 *
 * The order of records leaving the deterministic network
 * is restored according to the value of 'seqnr'.
 * This is a copy of a the sequence counter of the DetEnter
 * landing structure at the time of entrance of the record.
 *
 * If at the exit of the deterministic network records
 * need to be queued then they are added to 'recfifo'.
 *
 * Records within a nested deterministic network, which
 * is a deterministic network with a deterministic network,
 * have a stack of references to 'detref' structures.
 */
typedef struct detref {
  long                  seqnr;          /* sequence number for sorting */
  landing_t            *leave;          /* destination collector landing */
  struct detref        *nonlocal;       /* non-NULL if on a remote location */
  int                   location;       /* in distributed computing */
  int                   refcount;       /* number of associated records */
  fifo_t                recfifo;        /* record queue at collector */
} detref_t;

#define DETREF_INCR(detref)     AAF(&(detref)->refcount, 1)
#define DETREF_DECR(detref)     SAF(&(detref)->refcount, 1)
#define DETREF_MIN_REFCOUNT     2

#endif

void SNetRecDetrefCopy(snet_record_t *new_rec, snet_record_t *rec);

#endif
