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

/* --------------------------------------------------------
 * Syncro Cell: Flow Inheritance, uncomment desired variant
 * --------------------------------------------------------
 */

/* ------------------------------------------------------------------------- */
/*  SNetSync                                                                 */
/* ------------------------------------------------------------------------- */

struct sync_state {
  bool terminated;
  int match_count;
  snet_record_t **storage;
};

// Destroys storage including all records
static snet_record_t *Merge( snet_record_t **storage,
    snet_variant_list_t *patterns)
{
  int i = 0, name;
  snet_variant_t *pattern;
  snet_record_t *result = storage[0];

  LIST_FOR_EACH(patterns, pattern)
    if (i != 0) {
      VARIANT_FOR_EACH_FIELD(pattern, name)
        if (!SNetRecHasField( result, name)) {
          SNetRecSetField(result, name, SNetRecTakeField(storage[i], name));
        }
      END_FOR

      VARIANT_FOR_EACH_TAG(pattern, name)
        if (!SNetRecHasTag( result, name)) {
          SNetRecSetTag( result, name, SNetRecGetTag( storage[i], name));
        }
      END_FOR
    }
    i++;
  END_FOR

  for( i=1; i<SNetvariantListSize( patterns); i++) {
    SNetRecDestroy( storage[i]);
  }

  return result;
}

typedef struct {
  snet_stream_t *input, *output;
  snet_variant_list_t *patterns;
  snet_expr_list_t *guards;
} sync_arg_t;

/**
 * Sync box task
 */
static void SyncBoxTask(void *arg)
{
  int i;
  int match_cnt=0, new_matches=0;
  int num_patterns;
  bool terminate = false;
  sync_arg_t *sarg = (sync_arg_t *) arg;
  snet_stream_desc_t *outstream, *instream;
  snet_record_t **storage;
  snet_variant_t *variant;
  snet_record_t *rec;
  snet_record_t *temp_record;

  instream  = SNetStreamOpen(sarg->input,  'r');
  outstream = SNetStreamOpen(sarg->output, 'w');

  num_patterns = SNetvariantListSize( sarg->patterns);
  storage = SNetMemAlloc(num_patterns * sizeof(snet_record_t*));
  for(i = 0; i < num_patterns; i++) {
    storage[i] = NULL;
  }
  match_cnt = 0;

  /* MAIN LOOP START */
  while( !terminate) {
    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        new_matches = 0;
        for( i=0; i<num_patterns; i++) {
          /* storage empty and guard accepts => store record*/
          if( (storage[i] == NULL) &&
              (SNetRecPatternMatches(SNetvariantListGet(sarg->patterns, i), rec)) &&
              (SNetEevaluateBool(SNetEgetExpr(sarg->guards, i), rec))) {
            storage[i] = rec;
            new_matches += 1;
          }
        }

        if( new_matches == 0) {
          SNetStreamWrite( outstream, rec);
        } else {
          match_cnt += new_matches;
          if(match_cnt == num_patterns) {
            temp_record =  Merge( storage, sarg->patterns);
            SNetStreamWrite( outstream, temp_record);
            /* current_state->terminated = true; */
            SNetStreamWrite( outstream,
                SNetRecCreate(REC_sync, sarg->input)
                );

            /* the receiver of REC_sync will destroy the outstream */
            SNetStreamClose( outstream, false);
            /* instream has been sent to next entity, do not destroy  */
            SNetStreamClose( instream, false);

            terminate = true;
          }
        }
        break;

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream( rec);
          SNetStreamReplace( instream, newstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
        /* invalid record */
        assert(0);
        SNetRecDestroy( rec);
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

      default:
        assert(0);
        /* if ignore, destroy at least ... */
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  SNetMemFree(storage);
  LIST_FOR_EACH(sarg->patterns, variant)
    SNetVariantDestroy(variant);
  END_FOR

  SNetvariantListDestroy(sarg->patterns);
  SNetEdestroyList( sarg->guards);
  SNetMemFree( sarg);
}



/**
 * Synchro-Box creation function
 */
snet_stream_t *SNetSync( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_list_t *patterns,
    snet_expr_list_t *guards )
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
    sarg->guards = guards;

    SNetEntitySpawn( ENTITY_SYNC, SyncBoxTask, (void*)sarg);

  } else {
    snet_variant_t *variant;
    LIST_FOR_EACH(patterns, variant)
      SNetVariantDestroy(variant);
    END_FOR
    SNetvariantListDestroy( patterns);
    SNetEdestroyList(guards);
    output = input;
  }
  return( output);
}

