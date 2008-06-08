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
            PTR = SNetBufCreate( SIZE);

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
static snet_record_t *Merge( snet_record_t **storage, snet_typeencoding_t *patterns, 
						            snet_typeencoding_t *outtype, snet_record_t *rec) {

  int i,j, name, *names;
  snet_variantencoding_t *variant;
  
  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {
    variant = SNetTencGetVariant( patterns, i+1);
    names = SNetTencGetFieldNames( variant);
    for( j=0; j<SNetTencGetNumFields( variant); j++) {
      name = names[j];
      if( SNetRecAddField( rec, name)) {
        SNetRecSetField( rec, name, SNetRecTakeField( storage[i], name));  
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
static snet_record_t *Merge( snet_record_t **storage, snet_typeencoding_t *patterns, 
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
        void *ptr;
        ptr = SNetRecTakeField( storage[i], SNetTencGetFieldNames( pat)[j]);
        SNetRecSetField( outrec, SNetTencGetFieldNames( pat)[j], ptr);
      }
      for( j=0; j<SNetTencGetNumTags( pat); j++) {
        int tag;
        tag = SNetRecTakeTag( storage[i], SNetTencGetTagNames( pat)[j]);
        SNetRecSetTag( outrec, SNetTencGetTagNames( pat)[j], tag);
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
          SNetRecSetField( outrec, names[j], SNetRecTakeField( storage[i], names[j]));
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
              snet_variantencoding_t *pat,
              snet_expr_t *guard) {

  int i,j, *names;
  bool is_match, found_name;
  is_match = true;

  if( SNetEevaluateBool( guard, rec)) {
    found_name = false;
    FIND_NAME_IN_PATTERN( SNetTencGetNumTags, SNetRecGetNumTags, 
                          SNetTencGetTagNames, SNetRecGetUnconsumedTagNames);

    if( is_match) {
      FIND_NAME_IN_PATTERN( SNetTencGetNumFields, SNetRecGetNumFields, 
                            SNetTencGetFieldNames, SNetRecGetUnconsumedFieldNames);
    }
  }
  else {
    is_match = false;
  }

  return( is_match);
}



static void *SyncBoxThread( void *hndl) {
  
  int i;
  int match_cnt=0, new_matches=0;
  int num_patterns;
  bool was_match;
  bool terminate = false;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_buffer_t *outbuf;
  snet_record_t **storage;
  snet_record_t *rec;
  snet_typeencoding_t *outtype;
  snet_typeencoding_t *patterns;
  snet_expr_list_t *guards;
  
  struct sync_state *current_state;
  snet_util_tree_t *states;
  snet_util_list_t *to_free;

  outbuf = SNetHndGetOutbuffer( hnd);
  outtype = SNetHndGetType( hnd);
  patterns = SNetHndGetPatterns( hnd);
  num_patterns = SNetTencGetNumVariants( patterns);
  guards = SNetHndGetGuardList( hnd);
  

  /* TODO: maybe make finding of dead synccells cleverer? */
  states = SNetUtilTreeCreate();
  to_free = SNetUtilListCreate();

  
  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        if(!SNetUtilTreeContains(states, SNetRecGetIterationStack(rec))) {
          /* insert fresh state for that stack. */
          current_state = (struct sync_state *)SNetMemAlloc(
                                          sizeof(struct sync_state));
          current_state->storage = SNetMemAlloc(num_patterns *
                                                    sizeof(struct sync_state));
          for(i = 0; i < num_patterns; i++) {
            current_state->storage[i] = NULL;
          }
          current_state->match_count = 0;
          current_state->terminated = false;


          SNetUtilTreeSet(states, SNetRecGetIterationStack(rec), current_state);
          SNetUtilListAddEnd(to_free, current_state);
        } else {
          current_state = SNetUtilTreeGet(states,SNetRecGetIterationStack(rec));
        }

        if(current_state->terminated) {
          SNetBufPut(outbuf, rec);
          continue;
        }
        storage = current_state->storage;
        match_cnt = current_state->match_count;

        new_matches = 0;
        for( i=0; i<num_patterns; i++) {
          /* storage empty and guard accepts => store record*/
          if( (storage[i] == NULL) &&
              (MatchPattern(rec, SNetTencGetVariant(patterns, i+1),
                        SNetEgetExpr( guards, i)))) {
            storage[i] = rec;
            was_match = true;
            new_matches += 1;
          }
        }

        if( new_matches == 0) {
          SNetBufPut( outbuf, rec);
        }
        else {
          current_state->match_count += new_matches;
          if(current_state->match_count == num_patterns) {
            #ifdef SYNC_FI_VARIANT_1
            SNetBufPut( outbuf, Merge( storage, patterns, outtype));
            #endif
            #ifdef SYNC_FI_VARIANT_2
            SNetBufPut( outbuf, Merge( storage, patterns, outtype, rec));
            #endif
            SNetBufPut(outbuf,
                      SNetRecCreate(REC_sync, SNetHndGetInbuffer( hnd)));
            current_state->terminated = true;
         }
        }
      break;
    case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);

      break;
      case REC_collect:
        SNetUtilDebugNotice("[Sync] Unhandled control record, destroying"
                            " it\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
    case REC_terminate:
        SNetUtilTreeDestroy(states);
        SNetUtilListGotoBeginning(to_free);
        while(SNetUtilListCurrentDefined(to_free)) {
          current_state = SNetUtilListGet(to_free);
          SNetUtilListNext(to_free);
          SNetMemFree(current_state->storage);
          SNetMemFree(current_state);
        }
        SNetUtilListDestroy(to_free);
        SNetUtilDebugNotice("[Sync] received termination record\n"
                                "stored records are discarded");
        terminate = true;
        SNetBufPut( outbuf, rec);
      break;
      case REC_probe:
        SNetBufPut(SNetHndGetOutbuffer(hnd), rec);
      break;
    }
  }

  SNetBufBlockUntilEmpty( outbuf);
  SNetBufDestroy( outbuf);
  SNetDestroyTypeEncoding( outtype);
  SNetDestroyTypeEncoding( patterns);
  SNetHndDestroy( hnd);
  SNetMemFree( storage);

  return( NULL);
}


extern snet_buffer_t *SNetSync( snet_buffer_t *inbuf,
                                snet_typeencoding_t *outtype,
                                snet_typeencoding_t *patterns,
                                snet_expr_list_t *guards ) {

  snet_buffer_t *outbuf;
  snet_handle_t *hnd;

//  outbuf = SNetBufCreate( BUFFER_SIZE);
  BUF_CREATE( outbuf, BUFFER_SIZE);
  hnd = SNetHndCreate( HND_sync, inbuf, outbuf, outtype, patterns, guards);


  SNetThreadCreate( SyncBoxThread, (void*)hnd, ENTITY_sync);

  return( outbuf);
}

