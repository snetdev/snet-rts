/** <!--********************************************************************-->
 *
 * $Id$
 *
 * file syncro.c
 * 
 * This implemets the synchrocell component [...] TODO: write this...
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



#include "syncro.h"

#include "bool.h"
#include "record.h"
#include "buffer.h"
#include "memfun.h"
#include "handle.h"
#include "debug.h"
#include "threading.h"
#include "tree.h"
#include "list.h"
#include "stream_layer.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif /* DISTRIBUTED_SNET */

//#define SYNCRO_DEBUG

/* --------------------------------------------------------
 * Syncro Cell: Flow Inheritance, uncomment desired variant
 * --------------------------------------------------------
 */

// inherit all fields and tags from matched records
//#define SYNC_FI_VARIANT_1

// inherit no tags/fields from previous matched records
// merge tags/fields from pattern to last arriving record.
#define SYNC_FI_VARIANT_2
/* --------------------------------------------------------
 */


#define BUF_CREATE( PTR, SIZE)\
            PTR = SNetTlCreateStream( SIZE);

#define BUF_SET_NAME( PTR, NAME)


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
                             snet_record_t *rec) {

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
                             snet_typeencoding_t *outtype) {

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


static bool
MatchPattern( snet_record_t *rec,
              snet_ variantencoding_t *pat,
              snet_expr_t *guard) {

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



static void *SyncBoxThread( void *hndl) {
  
  int i; 
  int match_cnt=0, new_matches=0;
  int num_patterns;
  bool terminate = false;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_tl_stream_t *output;
  snet_record_t **storage;
  snet_record_t *rec;
  snet_record_t *temp_record;
  snet_typeencoding_t *outtype;
  snet_typeencoding_t *patterns;
  snet_expr_list_t *guards;
  //snet_util_list_iter_t *current_storage;
  //struct sync_state *current_state;
  //snet_util_tree_t *states;
  //snet_util_list_t *to_free;
  //snet_util_stack_t *temp_stack;

#ifdef SYNCRO_DEBUG
  SNetUtilDebugNotice("(CREATION SYNCRO)");
#endif
  output = SNetHndGetOutput( hnd);
  outtype = SNetHndGetType( hnd);
  patterns = SNetHndGetPatterns( hnd);
  num_patterns = SNetTencGetNumVariants( patterns);
  guards = SNetHndGetGuardList( hnd);
  
  storage = SNetMemAlloc(num_patterns * sizeof(snet_record_t*));
  for(i = 0; i < num_patterns; i++) {
    storage[i] = NULL;
  }
  match_cnt = 0;
  
  while( !( terminate)) {
    rec = SNetTlRead( SNetHndGetInput( hnd));
    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        new_matches = 0;
        for( i=0; i<num_patterns; i++) {
          /* storage empty and guard accepts => store record*/
          if( (storage[i] == NULL) &&
              (MatchPattern(rec, SNetTencGetVariant(patterns, i+1),
                        SNetEgetExpr( guards, i)))) {
            storage[i] = rec;
            new_matches += 1;
          }
        }
 
        if( new_matches == 0) {
          SNetTlWrite(output, rec);
        }
        else {
          match_cnt += new_matches;
          if(match_cnt == num_patterns) {
#ifdef SYNC_FI_VARIANT_1
            temp_record = Merge( storage, patterns, outtype);
            SNetRecSetInterfaceId( temp_record, SNetRecGetInterfaceId( rec));
            SNetRecSetDataMode( temp_record, SNetRecGetDataMode( rec));
#endif
#ifdef SYNC_FI_VARIANT_2
            temp_record =  Merge( storage, 
                                  patterns, 
                                  outtype, 
                                  storage[0]);
#endif

            SNetTlWrite(output, temp_record);
            /* current_state->terminated = true; */
            SNetTlWrite(output, SNetRecCreate(REC_sync, 
                                              SNetHndGetInput(hnd)));

	          SNetTlMarkObsolete(output);

            terminate = true;
          }
        }
      break;
    case REC_sync:
        SNetHndSetInput( hnd, SNetRecGetStream( rec));
        SNetRecDestroy( rec);

      break;
      case REC_collect:
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetTlWrite( SNetHndGetOutput( hnd), rec);
        break;
      case REC_sort_end:
        SNetTlWrite( SNetHndGetOutput( hnd), rec);
        break;
    case REC_terminate:
        /* SNetUtilTreeDestroy(states);*/
        /* check if all storages are empty */
        /*
        current_storage = SNetUtilListFirst(to_free);
        while(SNetUtilListIterCurrentDefined(current_storage)) {
          current_state = SNetUtilListIterGet(current_storage);
          current_storage = SNetUtilListIterNext(current_storage);
          if(!current_state->terminated) {
            SNetUtilDebugNotice("SYNCRO received termination record - "
                                 "stored records are discarded");
          }
          SNetMemFree(current_state->storage);
          SNetMemFree(current_state);
        }
        SNetUtilListDestroy(to_free);
        */

        terminate = true;
        SNetTlWrite( output, rec);
      	SNetTlMarkObsolete(output);
      break;

      case REC_probe:
        SNetTlWrite(SNetHndGetOutput(hnd), rec);
      break;
    default:
      SNetUtilDebugNotice("[Synchro] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( rec));
      SNetRecDestroy( rec);
      break;
    }
  }
  SNetMemFree(storage);
  SNetDestroyTypeEncoding( outtype);
  SNetDestroyTypeEncoding( patterns);
  SNetHndDestroy( hnd);

  return( NULL);
}


extern snet_tl_stream_t *SNetSync( snet_tl_stream_t *input,
#ifdef DISTRIBUTED_SNET
				   sne t_info_t *info, 
				   int location,
#endif /* DISTRIBUTED_SNET */
				   snet_typeencoding_t *outtype,
				   snet_typeencoding_t *patterns,
				   snet_expr_list_t *guards ) {

  snet_tl_stream_t *output;
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location); 

  if(location == SNetIDServiceGetNodeID()) {

#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Synchrocell created");
#endif /* DISTRIBUTED_DEBUG */

#endif /* DISTRIBUTED_SNET */
    
    //  output = SNetBufCreate( BUFFER_SIZE);
    BUF_CREATE( output, BUFFER_SIZE);
#ifdef SYNCRO_DEBUG
    SNetUtilDebugNotice("-");
    SNetUtilDebugNotice("| SYNCRO CREATED");
    SNetUtilDebugNotice("| input: %p", input);
    SNetUtilDebugNotice("| output: %p", output);
    SNetUtilDebugNotice("-");
#endif
    hnd = SNetHndCreate( HND_sync, input, output, outtype, patterns, guards);
    
    
    SNetTlCreateComponent( SyncBoxThread, (void*)hnd, ENTITY_sync);
#ifdef SYNCRO_DEBUG
    SNetUtilDebugNotice("SYNCRO CREATION DONE");
#endif
    
#ifdef DISTRIBUTED_SNET
  } else {
    SNetDestroyTypeEncoding( outtype);

    SNetDestroyTypeEncoding( patterns);

    SNetEdestroyList(guards);

    output = input;
}
#endif /* DISTRIBUTED_SNET */

  return( output);
}

