#ifndef _DETREF_H
#define _DETREF_H

#if defined(_XFIFO_H) && defined(_NODE_H_)

/*
 * A reference counter for deterministic ordering:
 *
 * All records that enter a deterministic network
 * receive a 'detref' structure to keep track of
 * their ordering in the streams of records.
 * Records which are created within the network
 * are always created in the context of an
 * existing record which carries a 'detref' structure.
 * The newly created records are also associated with
 * this 'detref' structure and for each one the reference
 * counter in this 'detref' structure is incremented.
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
  long                  seqnr;
  landing_t            *leave;
  int                   refcount;
  fifo_t                recfifo;
} detref_t;

#define DETREF_INCR(detref)     AAF(&(detref)->refcount, 1)
#define DETREF_DECR(detref)     SAF(&(detref)->refcount, 1)

#endif

void SNetRecDetrefCopy(snet_record_t *new_rec, snet_record_t *rec);

#endif
