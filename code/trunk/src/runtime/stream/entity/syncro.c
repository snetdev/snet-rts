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
#include "debug.h"


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
  void *field;
  snet_variant_t *pattern;
  snet_record_t *result = storage[0];
  snet_copy_fun_t copyfun;

  LIST_ENUMERATE(patterns, pattern, i)
    if (i > 0 && storage[i] != NULL) {
      copyfun = SNetInterfaceGet(SNetRecGetInterfaceId(storage[i]))->copyfun;

      RECORD_FOR_EACH_FIELD(storage[i], name, field)
        if (SNetVariantHasField(pattern, name)) {
            SNetRecSetField(result, name, copyfun(field));
        }
      END_FOR

      RECORD_FOR_EACH_TAG(storage[i], name, value)
        if (SNetVariantHasTag(pattern, name)) {
            SNetRecSetTag(result, name, value);
        }
      END_FOR

      if (storage[i] != NULL) {
        SNetRecDestroy(storage[i]);
      }
    }
  END_ENUMERATE

  return result;
}

/*****************************************************************************/
/* SYNCHRO TASK                                                              */
/*****************************************************************************/

typedef struct {
  snet_stream_t *input, *output;
  snet_variant_list_t *patterns;
  snet_expr_list_t *guard_exprs;
} sync_arg_t;

/**
 * Sync box task
 */
static void SyncBoxTask(void *arg)
{
  snet_expr_t *expr;
  snet_record_t *rec;
  snet_record_t dummy; /* used to indicate unmatched pattern */
  snet_variant_t *pattern;
  snet_stream_desc_t *outstream, *instream;

  bool terminate = false;
  int i, new_matches, match_cnt = 0;
  sync_arg_t *sarg = (sync_arg_t *) arg;
  int num_patterns = SNetVariantListLength( sarg->patterns);
  snet_record_t *storage[num_patterns];
  snet_locvec_t *instream_source = NULL;

  instream  = SNetStreamOpen(sarg->input,  'r');
  outstream = SNetStreamOpen(sarg->output, 'w');

  /* !! CAUTION !!
   * Set all storage slots to &dummy, indicating unmatched pattern. This frees
   * up NULL to indicate that a pattern was matched by a record which also
   * matched an earlier pattern. This lets us avoid the storage container
   * holding the same pointer multiple times, this means the merge function can
   * safely free them all without freeing the same pointer multiple times.
   */
  for (i = 0; i < num_patterns; i++) {
    storage[i] = &dummy;
  }

  /* MAIN LOOP START */
  while( !terminate) {
    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch (SNetRecGetDescriptor( rec)) {
      case REC_data:
        new_matches = 0;
        LIST_ZIP_ENUMERATE(sarg->patterns, pattern, sarg->guard_exprs, expr, i)
          /* storage empty and guard accepts => store record*/
          if (storage[i] == &dummy && SNetRecPatternMatches(pattern, rec) &&
              SNetEevaluateBool(expr, rec)) {
            if (new_matches == 0) {
              storage[i] = rec;
            } else {
              /* Record already stored as match for another pattern. */
              storage[i] = NULL;
            }
            new_matches += 1;
          }
        END_ZIP

        match_cnt += new_matches;
        if (new_matches == 0) {
          SNetStreamWrite( outstream, rec);
        } else if (match_cnt == num_patterns) {
          SNetStreamWrite( outstream, MergeFromStorage( storage, sarg->patterns));

          if (instream_source != NULL) {
            /* predecessor made known its location,
             * presumably for garbage collection, so we will forward it
             * to let our successor know
             */
            SNetStreamWrite( outstream,
            SNetRecCreate(REC_source, instream_source));
          }
          /* follow by a sync record */
          SNetStreamWrite( outstream, SNetRecCreate(REC_sync, sarg->input));

          /* the receiver of REC_sync will destroy the outstream */
          SNetStreamClose( outstream, false);
          /* instream has been sent to next entity, do not destroy  */
          SNetStreamClose( instream, false);

          terminate = true;
        }
        break;

      case REC_sync:
        {
          sarg->input = SNetRecGetStream( rec);
          SNetStreamReplace( instream, sarg->input);
          SNetRecDestroy( rec);
        }
        break;

      case REC_sort_end:
        /* forward sort record */
        SNetStreamWrite( outstream, rec);
        break;

      case REC_terminate:
        terminate = true;
        SNetStreamWrite( outstream, rec);
        SNetStreamClose( outstream, false);
        SNetStreamClose( instream, true);
        break;

      case REC_source:
        /* Get (a copy of) the location */
        if (instream_source != NULL) {
          SNetLocvecDestroy(instream_source);
        }
        instream_source = SNetLocvecCopy( SNetRecGetLocvec( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
        /* invalid record */
      default:
        assert(0);
        /* if ignore, destroy at least ... */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  /* free any stored location vector */
  if (instream_source != NULL) {
    SNetLocvecDestroy(instream_source);
  }

  SNetVariantListDestroy( sarg->patterns);
  SNetExprListDestroy( sarg->guard_exprs);
  SNetMemFree( sarg);
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

  input = SNetRouteUpdate(info, input, location);
  if(location == SNetNodeLocation) {
    output = SNetStreamCreate(0);
    sarg = (sync_arg_t *) SNetMemAlloc( sizeof( sync_arg_t));
    sarg->input  = input;
    sarg->output = output;
    sarg->patterns = patterns;
    sarg->guard_exprs = guard_exprs;

    SNetEntitySpawn( ENTITY_SYNC, SyncBoxTask, (void*)sarg);
  } else {
    SNetVariantListDestroy( patterns);
    SNetExprListDestroy( guard_exprs);
    output = input;
  }

  return output;
}
