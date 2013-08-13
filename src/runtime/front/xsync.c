/**************************************************************************
 *
 * This implements the synchrocell component [...] TODO: write this...
 *
 * Flow inheritance at a synchrocell is (currently) defined in such a way
 * that only the record that matches the first pattern flow-inherits all
 * its constituents. The order of arrival is irrelevant. Previously, the
 * record that arrived last, i.e. the record that matched the last remaining
 * unmatched record, was to keep all its fields and tags.
 *
 * The necessary changes to implement the 'new' behaviour is minimal, as
 * construction of the resulting outbound record is implemented in a separate
 * merge function (see above ;)).
 * Where previously the last record was passed as argument to the merge
 * function, now the record which is stored at first position in the record
 * storage is passed to the merge function. As excess fields and tags, i.e.
 * all those labels that are not present in the synchro pattern, are not
 * stripped from records before they go into storage, the above is sufficient
 * to implement the desired behaviour.
 *
 *****************************************************************************/

#include <string.h>
#include "node.h"

static snet_record_t DUMMY;         /* used to indicate unmatched pattern */

/*****************************************************************************/
/* HELPER FUNCTIONS                                                          */
/*****************************************************************************/

void SNetSyncInitDesc(snet_stream_desc_t *desc, snet_stream_t *stream)
{
  sync_arg_t     *nsync = NODE_SPEC(STREAM_DEST(stream), sync);
  landing_sync_t *lsync = DESC_LAND_SPEC(desc, sync);
  int             i;

  lsync->outdesc = NULL;
  lsync->storage = SNetNewN(nsync->num_patterns, snet_record_t *);
  lsync->match_count = 0;
  lsync->state = SYNC_initial;
  for (i = 0; i < nsync->num_patterns; ++i) {
    lsync->storage[i] = &DUMMY;
  }
}

/*
 * Merges the other records from the storage into the first pattern record.
 * After that, all (but the first) records are destroyed from the storage.
 */
static snet_record_t *MergeFromStorage(
    const sync_arg_t *sarg,
    landing_sync_t *land)
{
  int i, name, value;
  snet_ref_t *field;
  snet_variant_t *pattern;
  snet_record_t *result = land->storage[0];

  land->storage[0] = NULL;
  LIST_ENUMERATE(sarg->patterns, i, pattern) {
    if (land->storage[i] != NULL) {

      RECORD_FOR_EACH_FIELD(land->storage[i], name, field) {
        if (SNetVariantHasField(pattern, name)) {
            SNetRecSetField(result, name, SNetRefCopy(field));
        }
      }

      RECORD_FOR_EACH_TAG(land->storage[i], name, value) {
        if (SNetVariantHasTag(pattern, name)) {
            SNetRecSetTag(result, name, value);
        }
      }

      /* free storage */
      SNetRecDestroy(land->storage[i]);
      land->storage[i] = &DUMMY;
    }
  }
  land->storage[0] = &DUMMY;

  return result;
}

static void DestroyStorage( sync_arg_t *sarg, landing_sync_t *land)
{
  int i;
  for (i = 0; i < sarg->num_patterns; ++i) {
    /* free storage */
    if (land->storage[i] != NULL && land->storage[i] != &DUMMY) {
      SNetRecDestroy(land->storage[i]);
    }
    land->storage[i] = &DUMMY;
  }
  land->state = SYNC_complete;
}

/* Check the record type against the synchro-cell pattern types. */
static int MatchPatterns(
    const sync_arg_t *sarg,
    snet_record_t *rec,
    landing_sync_t *land)
{
  int new_matches = 0;
  int i;
  snet_expr_t *expr;
  snet_variant_t *pattern;

  LIST_ZIP_ENUMERATE(sarg->patterns, sarg->guard_exprs, i, pattern, expr) {
    /* storage empty and guard accepts => store record*/
    if (land->storage[i] == &DUMMY &&
        SNetRecPatternMatches(pattern, rec) &&
        SNetEevaluateBool(expr, rec))
    {
      if (++new_matches == 1) {
        land->storage[i] = rec;
      } else {
        land->storage[i] = NULL;
      }
      land->state = SYNC_partial;
    }
  }

  /* Update the total number of matched patterns for this synchro-cell. */
  land->match_count += new_matches;

  return new_matches;
}

/* After synchronization has completed create the final output record. */
static snet_record_t *SNetComposeSynchronizedRecord(
    snet_record_t       *rec_in,
    const sync_arg_t    *sarg,
    landing_sync_t      *land)
{
  snet_record_t *rec_out;
  void          *detref = DATA_REC(rec_in, detref);

  DATA_REC(rec_in, detref) = NULL;
  rec_out = MergeFromStorage( sarg, land);
  DATA_REC(rec_out, detref) = detref;

  return rec_out;
}

/* Process an incoming data record. */
static bool SNetNodeSyncData(
    snet_record_t       *rec,
    const sync_arg_t    *sarg,
    landing_sync_t      *land)
{
  if (MatchPatterns(sarg, rec, land) == 0) {
    /* Forward records which do not match any of our patterns. */
    SNetWrite(&land->outdesc, rec, true);
  } else {
    /* Check if all required patterns have been matched. */
    if (land->match_count == sarg->num_patterns) {
      /* Update synchro-cell status to: complete. */
      land->state = SYNC_complete;
      /* Construct a new output record from the stored records. */
      rec = SNetComposeSynchronizedRecord(rec, sarg, land);
      SNetWrite(&land->outdesc, rec, false);
    } else {
      /* Remove determinism from this record. */
      SNetRecDetrefDestroy(rec, &land->outdesc);
    }
  }

  return (land->state == SYNC_complete);
}

/**
 * Create a new variant that contains the union of fields/tags/btags of
 * a list of variants.
 *
 * The result has to be destroyed by the client.
 */
static snet_variant_t *GetMergedTypeVariant( snet_variant_list_t *patterns )
{
  snet_variant_t *res, *var;

  res = SNetVariantCreateEmpty();
  LIST_FOR_EACH(patterns, var) {
    SNetVariantAddAll(res, var, false);
  }

  return res;
}


/*****************************************************************************/
/* SYNCCELL TASK                                                             */
/*****************************************************************************/

/* Sync box task */
void SNetNodeSync(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const sync_arg_t      *sarg = DESC_NODE_SPEC(desc, sync);
  landing_sync_t        *land = DESC_LAND_SPEC(desc, sync);

  trace(__func__);

  if (land->outdesc == NULL) {
    land->outdesc = SNetStreamOpen(sarg->output, desc);
  }

  switch (REC_DESCR(rec)) {
    case REC_data:
      if (land->state == SYNC_complete) {
        SNetWrite(&land->outdesc, rec, true);
      }
      else if (SNetNodeSyncData(rec, sarg, land) && sarg->garbage_collect) {
        if (sarg->merged_type_sync) {
          /* Examine the type of the subsequent landing. */
          switch ((int) land->outdesc->landing->type) {
            case LAND_parallel:
            case LAND_identity:
              /* Only parallel dispatchers can benefit from this. */
              rec = SNetRecCreate(REC_sync, (snet_stream_t *) desc);
              SNetRecSetVariant(rec, sarg->merged_pattern);
              SNetWrite(&land->outdesc, rec, false);
              break;
          }
        }
        SNetBecomeIdentity(desc->landing, land->outdesc);
      }
      break;

    case REC_detref:
      /* forward record */
      SNetWrite(&land->outdesc, rec, true);
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, sarg->entity);
  }
}

/* Terminate a sync landing. */
void SNetTermSync(landing_t *land, fifo_t *fifo)
{
  landing_sync_t        *lsync = LAND_SPEC(land, sync);

  trace(__func__);

  if (lsync->state == SYNC_partial) {
    sync_arg_t *sarg = NODE_SPEC(land->node, sync);
    DestroyStorage(sarg, lsync);
    SNetUtilDebugNoticeEnt(LAND_NODE_SPEC(land, sync)->entity,
      "[SYNC] Warning: Destroying partially synchronized sync-cell!");
  }

  if (lsync->outdesc) {
    SNetFifoPut(fifo, lsync->outdesc);
  }
  SNetFreeLanding(land);
}

/* Destroy a sync node. */
void SNetStopSync(node_t *node, fifo_t *fifo)
{
  sync_arg_t *sarg = NODE_SPEC(node, sync);
  trace(__func__);
  SNetStopStream(sarg->output, fifo);
  SNetVariantListDestroy(sarg->patterns);
  SNetExprListDestroy(sarg->guard_exprs);
  SNetVariantDestroy(sarg->merged_pattern);
  SNetEntityDestroy(sarg->entity);
  SNetMemFree(node);
}

/* Create a new Synchro-Cell */
snet_stream_t *SNetSync(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *patterns,
    snet_expr_list_t *guard_exprs )
{
  snet_stream_t *output;
  node_t        *node;
  sync_arg_t    *sarg;

  trace(__func__);
  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_sync, location, &input, 1, &output, 1,
                     SNetNodeSync, SNetStopSync, SNetTermSync);
  sarg = NODE_SPEC(node, sync);
  sarg->output = output;
  sarg->patterns = patterns;
  sarg->guard_exprs = guard_exprs;
  sarg->num_patterns = SNetVariantListLength( sarg->patterns);
  sarg->merged_pattern = GetMergedTypeVariant(sarg->patterns);
  sarg->garbage_collect = SNetGarbageCollection();
  sarg->merged_type_sync = (STREAM_FROM(input) &&
                            STREAM_FROM(input)->type == NODE_star);
  sarg->entity = SNetEntityCreate( ENTITY_sync, location, SNetLocvecGet(info),
                                   "<sync>", NULL, (void *) sarg);

  return output;
}

