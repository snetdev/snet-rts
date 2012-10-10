/** <!--********************************************************************-->
 *
 * $Id: syncro.c 2887 2010-10-17 19:37:56Z dlp $
 *
 * file syncro.c
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

#include <assert.h>

#include "snetentities.h"
#include "bool.h"
#include "memfun.h"
#include "threading.h"
#include "distribution.h"
#include "locvec.h"
#include "debug.h"
#include "moninfo.h"

/*
 * needs to be enabled to trigger the garbage collection mechanism
 * at the parallel dispatcher
 */
#define SYNC_SEND_OUTTYPES

/*****************************************************************************/
/* HELPER FUNCTIONS                                                          */
/*****************************************************************************/

/*
 * Merges the other records from the storage into the first pattern record.
 * After that, all (but the first) records are destroyed from the storage.
 */
static snet_record_t *MergeFromStorage( snet_record_t **storage,
                                        snet_variant_list_t *patterns)
{
  int i, name, value;
  snet_ref_t *field;
  snet_variant_t *pattern;
  snet_record_t *result = SNetRecCopy(storage[0]);

  SNetRecAddAsParent( result, storage[0]);

  LIST_ENUMERATE(patterns, i, pattern) {
    if (i > 0 && storage[i] != NULL) {
      SNetRecAddAsParent( result, storage[i]);

      RECORD_FOR_EACH_FIELD(storage[i], name, field) {
        if (SNetVariantHasField(pattern, name)) {
            SNetRecSetField(result, name, SNetRefCopy(field));
        }
      }

      RECORD_FOR_EACH_TAG(storage[i], name, value) {
        if (SNetVariantHasTag(pattern, name)) {
            SNetRecSetTag(result, name, value);
        }
      }
    }
    /* free storage */
    if (storage[i] != NULL) {
      SNetRecDestroy(storage[i]);
      storage[i] = NULL;
    }
  }

  return result;
}

static void DestroyStorage( snet_record_t **storage,
    snet_variant_list_t *patterns,
    snet_record_t *dummy)
{
  int i;
  snet_variant_t *pattern;
  (void) pattern;

  LIST_ENUMERATE(patterns, i, pattern) {
    /* free storage */
    if (storage[i] != NULL && storage[i] != dummy) {
      SNetRecDestroy(storage[i]);
    }
  }
}

#ifdef SYNC_SEND_OUTTYPES
/**
 * Create a new variant that contains the union of fields/tags/btags of
 * a list of variants.
 *
 * It has to be destroyed by the client.
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
#endif


/*****************************************************************************/
/* SYNCCELL TASK                                                             */
/*****************************************************************************/

typedef struct {
  snet_stream_desc_t *outstream, *instream;
  snet_variant_list_t *patterns;
  snet_expr_list_t *guard_exprs;
  snet_record_t **storage;
  snet_record_t dummy; /* used to indicate unmatched pattern */
  bool partial_sync;
  int match_cnt;
} sync_arg_t;

static void TerminateSyncBoxTask(sync_arg_t *sarg)
{
  SNetMemFree( sarg->storage);
  SNetVariantListDestroy( sarg->patterns);
  SNetExprListDestroy( sarg->guard_exprs);
  SNetMemFree( sarg);
}

/**
 * Sync box task
 */
static void SyncBoxTask(void *arg)
{
  sync_arg_t *sarg = arg;

  snet_expr_t *expr;
  snet_record_t *rec;
  snet_variant_t *pattern;

  int i, new_matches;
  int num_patterns = SNetVariantListLength( sarg->patterns);


  /* read from input stream */
  rec = SNetStreamRead( sarg->instream);

  switch (SNetRecGetDescriptor( rec)) {
    case REC_data:
      new_matches = 0;
      LIST_ZIP_ENUMERATE(sarg->patterns, sarg->guard_exprs, i, pattern, expr) {
        /* storage empty and guard accepts => store record*/
        if (sarg->storage[i] == &sarg->dummy && SNetRecPatternMatches(pattern, rec) &&
            SNetEevaluateBool(expr, rec)) {
          if (new_matches == 0) {
            sarg->storage[i] = rec;
          } else {
            /* Record already stored as match for another pattern. */
            sarg->storage[i] = NULL;
          }
          new_matches += 1;
          /* this is the first sync */
          if (!sarg->partial_sync) {
            sarg->partial_sync = true;
          }
#ifdef USE_USER_EVENT_LOGGING
          /* Emit a monitoring message of first record entering syncro cell */
          SNetThreadingEventSignal(
                SNetMonInfoCreate( EV_MESSAGE_IN, MON_RECORD, rec));
#endif

        }
      }

      sarg->match_cnt += new_matches;
      if (new_matches == 0) {
        SNetStreamWrite( sarg->outstream, rec);
      } else if (sarg->match_cnt == num_patterns) {
#ifdef SYNC_SEND_OUTTYPES
        snet_variant_t *outtype;
#endif
        snet_record_t *syncrec = MergeFromStorage( sarg->storage, sarg->patterns);

#ifdef USE_USER_EVENT_LOGGING
        /* Emit a monitoring message of firing syncro cell */
        SNetThreadingEventSignal(
            SNetMonInfoCreate( EV_MESSAGE_OUT, MON_RECORD, syncrec));
#endif
        /* this is the last sync */
        SNetStreamWrite( sarg->outstream, syncrec);

        /* follow by a sync record */
        syncrec = SNetRecCreate(REC_sync, SNetStreamGet(sarg->instream));
#ifdef SYNC_SEND_OUTTYPES
        /* if we read from a star entity, we store the outtype along
           with the sync record */
        if( SNetStreamGetSource( SNetStreamGet(sarg->instream)) != NULL ) {
          /*
           * To trigger garbage collection at a following parallel dispatcher
           * within a state-modeling network, the dispatcher needs knowledge about the
           * type of the merged record ('outtype' of the synchrocell).
           */
          outtype = GetMergedTypeVariant(sarg->patterns);
          /* is copied internally */
          SNetRecSetVariant(syncrec, outtype);
          SNetVariantDestroy(outtype);
        }
#endif

        SNetStreamWrite( sarg->outstream, syncrec);

        /* the receiver of REC_sync will destroy the outstream */
        SNetStreamClose( sarg->outstream, false);
        /* instream has been sent to next entity, do not destroy  */
        SNetStreamClose( sarg->instream, false);

        sarg->partial_sync = false;
        TerminateSyncBoxTask(sarg);
        return;
      }
      break;

    case REC_sync:
      SNetStreamReplace( sarg->instream, SNetRecGetStream(rec));
      SNetRecDestroy( rec);
      break;

    case REC_sort_end:
      /* forward sort record */
      SNetStreamWrite( sarg->outstream, rec);
      break;

    case REC_terminate:
      if (sarg->partial_sync) {
        DestroyStorage(sarg->storage, sarg->patterns, &sarg->dummy);
        SNetUtilDebugNoticeTask(
        "[SYNC] Warning: Destroying partially synchronized sync-cell!");
      }
      SNetStreamWrite( sarg->outstream, rec);
      SNetStreamClose( sarg->outstream, false);
      SNetStreamClose( sarg->instream, true);
      TerminateSyncBoxTask(sarg);
      return;
    case REC_collect:
      /* invalid record */
    default:
      assert(0);
      /* if ignore, destroy at least ... */
      SNetRecDestroy( rec);
  }
  SNetThreadingRespawn(NULL);
}

/*****************************************************************************/
/* CREATION FUNCTION                                                         */
/*****************************************************************************/


/**
 * Synchro-Box creation function
 */
snet_stream_t *SNetSync( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *patterns,
    snet_expr_list_t *guard_exprs )
{
  snet_stream_t *output;
  sync_arg_t *sarg;
  snet_locvec_t *locvec;

  locvec = SNetLocvecGet(info);

  input = SNetRouteUpdate(info, input, location);
  if(SNetDistribIsNodeLocation(location)) {
    int i, num_patterns;
    output = SNetStreamCreate(0);
    sarg = SNetMemAlloc( sizeof( sync_arg_t));
    sarg->instream = SNetStreamOpen(input, 'r');
    sarg->outstream = SNetStreamOpen(output, 'w');
    sarg->patterns = patterns;
    sarg->guard_exprs = guard_exprs;
    sarg->partial_sync = false;
    sarg->match_cnt = 0;
    num_patterns = SNetVariantListLength( sarg->patterns);
    sarg->storage = SNetMemAlloc(num_patterns*sizeof(snet_record_t *));

    /* !! CAUTION !!
     * Set all storage slots to &dummy, indicating unmatched pattern. This frees
     * up NULL to indicate that a pattern was matched by a record which also
     * matched an earlier pattern. This lets us avoid the storage container
     * holding the same pointer multiple times, this means the merge function can
     * safely free them all without freeing the same pointer multiple times.
     */
    for (i = 0; i < num_patterns; i++) {
      sarg->storage[i] = &sarg->dummy;
    }

    SNetThreadingSpawn( ENTITY_sync, location, locvec,
          "<sync>", &SyncBoxTask, sarg);
  } else {
    SNetVariantListDestroy( patterns);
    SNetExprListDestroy( guard_exprs);
    output = input;
  }

  return output;
}
