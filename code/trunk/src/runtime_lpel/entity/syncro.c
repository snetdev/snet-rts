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
#include "record_p.h"
#include "memfun.h"

#include "threading.h"

#include "distribution.h"

/* --------------------------------------------------------
 * Syncro Cell: Flow Inheritance, uncomment desired variant
 * --------------------------------------------------------
 */

// inherit all fields and tags from matched records
//#define SYNC_FI_VARIANT_1

// inherit no tags/fields from previous matched records
// merge tags/fields from pattern to last arriving record.
#define SYNC_FI_VARIANT_2

#define MC_ISMATCH( name) name->is_match
#define MC_COUNT( name) name->match_count

// used in SNetSync->IsMatch (depends on local variables!)
#define FIND_NAME_IN_PATTERN( TENCNUM, RECNUM, TENCNAMES, RECNAMES)\
  names = RECNAMES( rec);\
  for( i=0; i<TENCNUM( pat); i++) {\
    for( j=0; j<RECNUM( rec); j++) {\
      if( names[j] == TENCNAMES( pat)[i]) {\
        found_name = true;\
        break;\
      }\
    }\
    if ( !( found_name)) {\
      is_match = false;\
      break;\
    }\
    found_name = false;\
  }\
  SNetMemFree( names);

/* ------------------------------------------------------------------------- */
/*  SNetSync                                                                 */
/* ------------------------------------------------------------------------- */

struct sync_state {
  bool terminated;
  int match_count;
  snet_record_t **storage;
};

#ifdef SYNC_FI_VARIANT_2
// Destroys storage including all records
static snet_record_t *Merge( snet_record_t **storage, 
    snet_typeencoding_t *patterns, 
    snet_typeencoding_t *outtype,
    snet_record_t *rec)
{
  int i,j, name, *names;
  snet_variantencoding_t *variant;
  
  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    variant = SNetTencGetVariant( patterns, i+1);
    names = SNetTencGetFieldNames( variant);
    for( j=0; j<SNetTencGetNumFields( variant); j++) {
      name = names[j];
      if( SNetRecAddField( rec, name)) {
      	SNetRecMoveFieldToRec(storage[i], name, rec, name);
      }
    }
    names = SNetTencGetTagNames( variant);
    for( j=0; j<SNetTencGetNumTags( variant); j++) {
      name = names[j];
      if( SNetRecAddTag( rec, name)) {
        SNetRecSetTag( rec, name, SNetRecGetTag( storage[i], name));  
      }
    }
  }

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    if( storage[i] != rec) {
      SNetRecDestroy( storage[i]);
    }
  }
  
  return( rec);
}
#endif
#ifdef SYNC_FI_VARIANT_1
static snet_record_t *Merge( snet_record_t **storage, 
    snet_typeencoding_t *patterns, 
    snet_typeencoding_t *outtype)
{
  int i,j;
  snet_record_t *outrec;
  snet_variantencoding_t *outvariant;
  snet_variantencoding_t *pat;
  outvariant = SNetTencCopyVariantEncoding( SNetTencGetVariant( outtype, 1));
  
  outrec = SNetRecCreate( REC_data, outvariant);

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    pat = SNetTencGetVariant( patterns, i+1);
    for( j=0; j<SNetTencGetNumFields( pat); j++) {
      // set value in outrec of field (referenced by its new name) to the
      // according value of the original record (old name)
      if( storage[i] != NULL) {
        SNetRecMoveFieldToRec( storage[i], 
                               SNetTencGetFieldNames( pat)[j],
                               outrec, 
                               SNetTencGetFieldNames( pat)[j]);
      }
      for( j=0; j<SNetTencGetNumTags( pat); j++) {
        SNetRecSetTag( outrec, 
                       SNetTencGetTagNames( pat)[j], 
                       SNetRecTakeTag( storage[i], 
                         SNetTencGetTagNames( pat)[j]));
      }
    }
  }
 
  
  // flow inherit all tags/fields that are present in storage
  // but not in pattern
  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    int *names, num;
    if( storage[i] != NULL) {

      names = SNetRecGetUnconsumedTagNames( storage[i]);
      num = SNetRecGetNumTags( storage[i]);
      for( j=0; j<num; j++) {
        int tag_value;
        if( SNetRecAddTag( outrec, names[j])) { // Don't overwrite existing Tags
          tag_value = SNetRecTakeTag( storage[i], names[j]);
          SNetRecSetTag( outrec, names[j], tag_value);
        }
      }
      SNetMemFree( names);

      names = SNetRecGetUnconsumedFieldNames( storage[i]);
      num = SNetRecGetNumFields( storage[i]);
      for( j=0; j<num; j++) {
        if( SNetRecAddField( outrec, names[j])) {
          SNetRecMoveFieldToRec(storage[i], name[j], outrec, names[j]);
        }
      }
      SNetMemFree( names);
    }
  }

  return( outrec);
}
#endif


static bool MatchPattern( snet_record_t *rec,
    snet_variantencoding_t *pat, snet_expr_t *guard)
{
  int i,j, *names;
  bool is_match, found_name;
  is_match = true;

  found_name = false;
  FIND_NAME_IN_PATTERN( SNetTencGetNumTags, SNetRecGetNumTags, 
			SNetTencGetTagNames, SNetRecGetUnconsumedTagNames);

  if( is_match) {
    FIND_NAME_IN_PATTERN( SNetTencGetNumFields, SNetRecGetNumFields, 
			  SNetTencGetFieldNames, SNetRecGetUnconsumedFieldNames);
  }

  if(is_match && !SNetEevaluateBool( guard, rec)) {
    is_match = false; 
  }

  return( is_match);
}


typedef struct {
  snet_stream_t *input, *output;
  snet_typeencoding_t *outtype, *patterns;
  snet_expr_list_t *guards;
} sync_arg_t;

/**
 * Sync box task
 */
static void SyncBoxTask( snet_entity_t *self, void *arg)
{  
  int i; 
  int match_cnt=0, new_matches=0;
  int num_patterns;
  bool terminate = false;
  sync_arg_t *sarg = (sync_arg_t *) arg;
  snet_stream_desc_t *outstream, *instream;
  snet_record_t **storage;
  snet_record_t *rec;
  snet_record_t *temp_record;

  instream  = SNetStreamOpen( self, sarg->input,  'r');
  outstream = SNetStreamOpen( self, sarg->output, 'w');

  num_patterns = SNetTencGetNumVariants( sarg->patterns);
  
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
              (MatchPattern(rec, SNetTencGetVariant(sarg->patterns, i+1),
                            SNetEgetExpr( sarg->guards, i)))) {
            storage[i] = rec;
            new_matches += 1;
          }
        }

        if( new_matches == 0) {
          SNetStreamWrite( outstream, rec);
        } else {
          match_cnt += new_matches;
          if(match_cnt == num_patterns) {
#ifdef SYNC_FI_VARIANT_1
            temp_record = Merge( storage, sarg->patterns, sarg->outtype);
            SNetRecSetInterfaceId( temp_record, SNetRecGetInterfaceId( rec));
            SNetRecSetDataMode( temp_record, SNetRecGetDataMode( rec));
#endif
#ifdef SYNC_FI_VARIANT_2
            temp_record =  Merge( storage, 
                                  sarg->patterns, 
                                  sarg->outtype, 
                                  storage[0]);
#endif
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
  SNetDestroyTypeEncoding( sarg->outtype);
  SNetDestroyTypeEncoding( sarg->patterns);
  SNetEdestroyList( sarg->guards);

  SNetMemFree( sarg);
}



/**
 * Synchro-Box creation function
 */
snet_stream_t *SNetSync( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_typeencoding_t *outtype,
    snet_typeencoding_t *patterns,
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
    sarg->outtype = outtype;
    sarg->patterns = patterns;
    sarg->guards = guards;

    SNetEntitySpawn( ENTITY_sync, SyncBoxTask, (void*)sarg);

  } else {
    SNetDestroyTypeEncoding( outtype);
    SNetDestroyTypeEncoding( patterns);
    SNetEdestroyList(guards);
    output = input;
  }
  return( output);
}

