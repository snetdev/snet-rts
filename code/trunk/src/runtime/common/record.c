/* 
 * record.c
 * Implementation of the record and its functions.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "memfun.h"
#include "record.h"

#include "snetentities.h"
#include "stream_layer.h"

#include "stack.h"
#include "debug.h"

#ifdef DISTRIBUTED_SNET
#include "reference.h"
#endif /* DISTRIBUTED_SNET*/

#define GETNUM( N, M)   int i;\
                         i = N( GetVEnc( rec));\
                        return( i - GetConsumedCount( M( GetVEnc( rec)), i));


#define REMOVE_FROM_REC( RECNUMS, RECNAMES, RECGET, TENCREMOVE, NAME, TYPE)\
  int i, *names, skip;\
  TYPE *new;\
  names = RECNAMES( rec);\
  new = SNetMemAlloc( ( RECNUMS( rec) - 1) * sizeof( TYPE));\
  skip = 0;\
  for( i=0; i<RECNUMS( rec); i++) {\
    if( names[i] != name) {\
      new[i - skip] = RECGET( rec, names[i]);\
    }                                 \
    else {\
      skip += 1;\
    }\
  }\
  SNetMemFree( names);\
  TENCREMOVE( GetVEnc( rec), name);\
  SNetMemFree( rec->rec->data_rec->NAME);\
  rec->rec->data_rec->NAME = new;



/* macros for record datastructure */
#define REC_DESCR( name) name->rec_descr
#define RECPTR( name) name->rec
#define RECORD( name, type) RECPTR( name)->type

#define DATA_REC( name, component) RECORD( name, data_rec)->component
#define SYNC_REC( name, component) RECORD( name, sync_rec)->component
#define SORT_B_REC( name, component) RECORD( name, sort_begin_rec)->component
#define SORT_E_REC( name, component) RECORD( name, sort_end_rec)->component
#define TERMINATE_REC( name, component) RECORD( name, terminate_hnd)->component
#define STAR_REC( name, component) RECORD( name, star_rec)->component
#define PROBE_REC( name, component) RECORD( name, probe_rec)->component

#ifdef DISTRIBUTED_SNET
#define UPDATE_REC( name, component) RECORD( name, update_rec)->component
#define REDIRECT_REC( name, component) RECORD( name, redirect_rec)->component
#define CONCAT_REC( name, component) RECORD( name, concat_rec)->component
#endif

#ifdef DISTRIBUTED_SNET

typedef struct {
  snet_variantencoding_t *v_enc;
  snet_ref_t **fields;
  int *tags;
  int *btags;
  int interface_id;
  snet_record_mode_t mode;
} data_rec_t;
#else
typedef struct {
  snet_variantencoding_t *v_enc;
  void **fields;
  int *tags;
  int *btags;
  int interface_id;
  snet_record_mode_t mode;
} data_rec_t;
#endif

typedef struct {
  snet_tl_stream_t *input;
} sync_rec_t;

typedef struct {
  int num;
  int level;
} sort_begin_t;

typedef struct {
  int num;
  int level;
} sort_end_t;


typedef struct {
  /* empty */
} terminate_rec_t;

typedef struct {
  snet_tl_stream_t *output;
} star_rec_t;

typedef struct {
  /* empty */
} probe_rec_t;

#ifdef DISTRIBUTED_SNET
typedef struct {
  int node;
  int index;
  snet_tl_stream_t *stream;
} route_update_t;

typedef struct {
  int node;
  int index;
} route_redirect_t;

typedef struct {
  int index;
  snet_tl_stream_t *stream;
} route_concatenate_t;
#endif

union record_types {
  data_rec_t *data_rec;
  sync_rec_t *sync_rec;
  star_rec_t *star_rec;
  sort_begin_t *sort_begin_rec;
  sort_end_t *sort_end_rec;
  terminate_rec_t *terminate_rec;
  probe_rec_t *probe_rec;
#ifdef DISTRIBUTED_SNET
  route_update_t *update_rec;
  route_redirect_t *redirect_rec;
  route_concatenate_t *concat_rec;
#endif
};

struct record {
  snet_record_descr_t rec_descr;
  snet_record_types_t *rec;
  snet_util_stack_t *iteration_counters;
};


/* *********************************************************** */

static bool ContainsName( int name, int *names, int num) {
  
  int i;
  bool found;

  found = false;

  for( i=0; i<num; i++) {
    if( names[i] == name) {
      found = true;
      break;
    }
  }

  return( found);
}

#define FIND_NAME_IN_RECORD( TENCNUM, TENCNAMES, RECNAMES, RECNUM)\
    for( j=0; j<TENCNUM( pat); j++) {\
      if( !( ContainsName( TENCNAMES( pat)[j],\
                           RECNAMES( rec),\
                           RECNUM( rec)))) {\
        is_match = false;\
        break;\
      }\
    }

extern bool SNetRecPatternMatches(snet_variantencoding_t *pat,
				  snet_record_t *rec) {
  int j;
  bool is_match = true;
  FIND_NAME_IN_RECORD( SNetTencGetNumFields, SNetTencGetFieldNames,
                           SNetRecGetFieldNames, SNetRecGetNumFields);
  if( is_match) {
    FIND_NAME_IN_RECORD( SNetTencGetNumTags, SNetTencGetTagNames,
                             SNetRecGetTagNames, SNetRecGetNumTags);
    if( is_match) {
      FIND_NAME_IN_RECORD( SNetTencGetNumBTags, SNetTencGetBTagNames,
                               SNetRecGetBTagNames, SNetRecGetNumBTags);
    }
  }
  return is_match;
}

static int FindName( int *names, int count, int val)
{

  int i=0;
  if( ( names == NULL)) {
    SNetUtilDebugFatal("Record contains no name encoding. This is a runtime"
                    " system bug. Findname()");
  }
  while( (i < count) && (names[i] != val)) {
    i += 1;
  }

  if( i == count) {
    i = NOT_FOUND;
  }
  
   return( i);
}

static snet_variantencoding_t *GetVEnc( snet_record_t *rec)
{
  return( DATA_REC( rec, v_enc));
}


static int GetConsumedCount( int *names, int num)
{
  int counter,i;

  counter=0;
  for( i=0; i<num; i++) {
    if( (names[i] == CONSUMED)) {
      counter = counter + 1;
    }
  }

  return( counter);
}

static void NotFoundError( int name, char *action, char *type)
{
  SNetUtilDebugFatal("Attempted '%s' on non-existent %s [%d]",
                      action, type, name);
}
/* *********************************************************** */

extern snet_record_t *SNetRecCreate( snet_record_descr_t descr, ...)
{
  snet_record_t *rec;
  va_list args;
  snet_variantencoding_t *v_enc;

  rec = SNetMemAlloc( sizeof( snet_record_t));
  REC_DESCR( rec) = descr;
  rec->iteration_counters = SNetUtilStackCreate();

  va_start( args, descr);

  switch( descr) {
    case REC_data:
      v_enc = va_arg( args, snet_variantencoding_t*);

      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, data_rec) = SNetMemAlloc( sizeof( data_rec_t));
#ifdef DISTRIBUTED_SNET
      DATA_REC( rec, fields) = SNetMemAlloc( SNetTencGetNumFields( v_enc)
					     * sizeof( snet_ref_t *));
#else
      DATA_REC( rec, fields) = SNetMemAlloc( SNetTencGetNumFields( v_enc)
                                              * sizeof( void*));
#endif /* DISTRIBUTED_SNET */
      DATA_REC( rec, tags) = SNetMemAlloc( SNetTencGetNumTags( v_enc)
                                              * sizeof( int));
      DATA_REC( rec, btags) = SNetMemAlloc( SNetTencGetNumBTags( v_enc)
                                              * sizeof( int));
      DATA_REC( rec, v_enc) = v_enc;
      DATA_REC( rec, mode) = MODE_binary;
#ifdef DISTRIBUTED_SNET
      DATA_REC( rec, interface_id) = 0;
#endif /* DISTRIBUTED_SNET */
      break;
    case REC_sync:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sync_rec) = SNetMemAlloc( sizeof( sync_rec_t));
      SYNC_REC( rec, input) = va_arg( args, snet_tl_stream_t*);
      break;
    case REC_collect:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, star_rec) = SNetMemAlloc( sizeof( star_rec_t));
      STAR_REC( rec, output) = va_arg( args, snet_tl_stream_t*);
      break;
    case REC_terminate:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      break;
    case REC_sort_begin:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sort_begin_rec) = SNetMemAlloc( sizeof( sort_begin_t));
      SORT_B_REC( rec, level) = va_arg( args, int);
      SORT_B_REC( rec, num) =   va_arg( args, int);
      break;
    case REC_sort_end:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sort_end_rec) = SNetMemAlloc( sizeof( sort_end_t));
      SORT_E_REC( rec, level) = va_arg( args, int);
      SORT_E_REC( rec, num) = va_arg( args, int);
      break;
    case REC_probe:
      RECPTR(rec) = SNetMemAlloc(sizeof(snet_record_types_t));
      RECORD(rec, probe_rec) = SNetMemAlloc(sizeof(probe_rec_t));
    break;
#ifdef DISTRIBUTED_SNET
    case REC_route_update:
      RECPTR(rec) = SNetMemAlloc(sizeof(snet_record_types_t));
      RECORD(rec, update_rec)  = SNetMemAlloc(sizeof(route_update_t));
      UPDATE_REC( rec, node)   = va_arg( args, int);
      UPDATE_REC( rec, index)  = va_arg( args, int);
      UPDATE_REC( rec, stream) = va_arg( args, snet_tl_stream_t *);
    break;
    case REC_route_redirect:
      RECPTR(rec) = SNetMemAlloc(sizeof(snet_record_types_t));
      RECORD(rec, redirect_rec) = SNetMemAlloc(sizeof(route_redirect_t));
      REDIRECT_REC( rec, node)  = va_arg( args, int);
      REDIRECT_REC( rec, index) = va_arg( args, int);
    break;
    case REC_route_concatenate:
      RECPTR(rec) = SNetMemAlloc(sizeof(snet_record_types_t));
      RECORD(rec, redirect_rec) = SNetMemAlloc(sizeof(route_redirect_t));
      CONCAT_REC( rec, index)   = va_arg( args, int);
      CONCAT_REC( rec, stream)  = va_arg( args, snet_tl_stream_t *);
    break;      
#endif

    default:
      SNetUtilDebugFatal("Unknown control record destription. [%d]",
                          descr);
  }

  va_end( args);

  return( rec);
}


extern snet_util_stack_t *SNetRecGetIterationStack(snet_record_t *rec)
{
  return rec->iteration_counters;
}

extern void SNetRecDestroy( snet_record_t *rec)
{
  int i;
  int num, *names;
#ifdef DISTRIBUTED_SNET
  int offset;
#else
  snet_free_fun_t freefun;
#endif /* DISTRIBUTED_SNET */
  
  if( rec != NULL) {
    SNetUtilStackDestroy(rec->iteration_counters);
    switch( REC_DESCR( rec)) {
    case REC_data: 
      num = SNetRecGetNumFields( rec);
      names = SNetRecGetUnconsumedFieldNames( rec);
#ifdef DISTRIBUTED_SNET
      for( i=0; i<num; i++) {
	offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
			   SNetTencGetNumFields( GetVEnc( rec)), names[i]);

	if( offset == NOT_FOUND) {
	  NotFoundError( names[i], "destroy","field");
	}

	SNetRefDestroy(DATA_REC( rec, fields)[offset]);
      }
#else
      freefun = SNetGetFreeFunFromRec( rec);
      for( i=0; i<num; i++) {
	freefun( SNetRecTakeField( rec, names[i]));
      }
#endif /* DISTRIBUTED_SNET */
      SNetTencDestroyVariantEncoding( DATA_REC( rec, v_enc));
      SNetMemFree( names);
      SNetMemFree( DATA_REC( rec, fields));
      SNetMemFree( DATA_REC( rec, tags));
      SNetMemFree( DATA_REC( rec, btags));
      SNetMemFree( RECORD( rec, data_rec));
      break;
    
    case REC_sync:
      SNetMemFree( RECORD( rec, sync_rec));
      break;
    case REC_collect:
      SNetMemFree( RECORD( rec, star_rec));
      break;
      
    case REC_sort_begin:
      SNetMemFree( RECORD( rec, sort_begin_rec));
      break;
    
    case REC_sort_end:
      SNetMemFree( RECORD( rec, sort_end_rec));
      break;

    case REC_probe:
      SNetMemFree(RECORD(rec, probe_rec));
      break;
    
    case REC_terminate:
      break;
    
#ifdef DISTRIBUTED_SNET

  case REC_route_update:
    SNetMemFree(RECORD(rec, update_rec));
    break;

  case REC_route_redirect:
    SNetMemFree(RECORD(rec, redirect_rec));
    break;

  case REC_route_concatenate:
    SNetMemFree(RECORD(rec, redirect_rec));
    break;

#endif

    default:
      SNetUtilDebugFatal("Unknown control record description, in RECdestroy");
      break;
  }
  SNetMemFree( RECPTR( rec));
  SNetMemFree( rec);
 }
}

extern int SNetRecHasIteration(snet_record_t *rec)
{
  return !SNetUtilStackIsEmpty(rec->iteration_counters);
}

extern int SNetRecGetIteration(snet_record_t *rec)
{
  return SNetUtilStackPeek(rec->iteration_counters);
}

extern void SNetRecIncIteration(snet_record_t *rec)
{
  int counter;

  if(SNetUtilStackIsEmpty(rec->iteration_counters)) {
    SNetUtilDebugFatal("IncIteration requested for record without iteration"
                       " counter!");
  }
  counter = SNetUtilStackPeek(rec->iteration_counters);
  rec->iteration_counters = SNetUtilStackSet(rec->iteration_counters,
                                             counter+1);
}

extern void SNetRecAddIteration(snet_record_t *rec, int initial_value)
{
  snet_util_stack_t *current_stack = rec->iteration_counters;
  SNetUtilDebugNotice("adding an iteration to %p", rec);
  current_stack = SNetUtilStackPush(current_stack, initial_value);
}

extern void SNetRecRemoveIteration(snet_record_t *rec)
{
  snet_util_stack_t *current_stack = rec->iteration_counters;
  if(SNetUtilStackIsEmpty(rec->iteration_counters)) {
    SNetUtilDebugFatal("Removal of iteration counter requested without any "
                       "iterationcounters present");
  }

  current_stack = SNetUtilStackPop(current_stack);
}

extern void SNetRecCopyIterations(snet_record_t *source, snet_record_t *target)
{
  snet_util_stack_iterator_t *current_iteration;
  snet_util_stack_t *temp_stack;
  int current_counter;

  if(source == NULL) {
    SNetUtilDebugFatal("RecCopyIterations: source == NULL");
  }
  if(target == NULL) {
    SNetUtilDebugFatal("RecCopyIterations: target == NULL");
  }
  if(target == source) {
    SNetUtilDebugFatal("RecCopyIterations: target == source!");
  }
  temp_stack = SNetRecGetIterationStack(source);
  current_iteration = SNetUtilStackBottom(temp_stack);
  while(SNetUtilStackIterCurrDefined(current_iteration)) {
    current_counter = SNetUtilStackIterGet(current_iteration);
    SNetRecAddIteration(target, current_counter);

    current_iteration = SNetUtilStackIterNext(current_iteration);
  }
  SNetUtilStackIterDestroy(current_iteration);
}

extern snet_record_descr_t SNetRecGetDescriptor( snet_record_t *rec)
{
  return( REC_DESCR( rec));
}

extern snet_record_t *SNetRecSetInterfaceId( snet_record_t *rec, int id)
{

  switch( REC_DESCR( rec)) {
    case REC_data:
      DATA_REC( rec, interface_id) = id;
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECSetId() (%d)", REC_DESCR(rec));
      break;
  }

  return( rec);
}

extern int SNetRecGetInterfaceId( snet_record_t *rec)
{

  int result;

  switch( REC_DESCR( rec)) {
    case REC_data:
      result = DATA_REC( rec, interface_id);
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECGetID() (%d)", REC_DESCR(rec));
      break;
  }

  return( result);
}

extern snet_tl_stream_t *SNetRecGetStream( snet_record_t *rec)
{
  snet_tl_stream_t *result;

  switch( REC_DESCR( rec)) {
  case REC_sync:
    result = SYNC_REC( rec, input);
    break;
  case REC_collect:
    result = STAR_REC( rec, output);
    break;
#ifdef DISTRIBUTED_SNET
  case REC_route_update:
    result = UPDATE_REC( rec, stream);
    break;
  case REC_route_concatenate:
    result = CONCAT_REC( rec, stream);
    break;
#endif
  default:
    SNetUtilDebugFatal("Wrong type in RECgetBuffer() (%d)", REC_DESCR(rec));
    break;
  }
  
  return result;
}


extern void SNetRecSetTag( snet_record_t *rec, int name, int val)
{
  int offset=0;
  
  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)),
                SNetTencGetNumTags( GetVEnc( rec)), name);
  DATA_REC( rec, tags[offset]) = val;
}

extern void SNetRecSetBTag( snet_record_t *rec, int name, int val)
{
  int offset;
  
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)),
                            SNetTencGetNumBTags( GetVEnc( rec)), name);
  DATA_REC( rec, btags[offset]) = val;
}


extern void SNetRecSetField( snet_record_t *rec, int name, void *val) 
{
  int offset;
  
  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
                        SNetTencGetNumFields( GetVEnc( rec)), name);
#ifdef DISTRIBUTED_SNET

  DATA_REC( rec, fields[offset]) = SNetRefCreate(SNetIDServiceGetID(), 
						 SNetIDServiceGetNodeID(),
						 DATA_REC( rec, interface_id), val);
#else
  DATA_REC( rec, fields[offset]) = val;
#endif /* DISTRIBUTED_SNET */
}

extern int SNetRecGetTag( snet_record_t *rec, int name)
{
  int offset;
  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)),
                          SNetTencGetNumTags( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "get","tag");
  }
  return( DATA_REC( rec, tags[offset]));
}


extern int SNetRecGetBTag( snet_record_t *rec, int name)
{
  int offset;
  
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)),
                          SNetTencGetNumBTags( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "get","binding tag");
  }
  return( DATA_REC( rec, btags[offset]));
}

extern void *SNetRecGetField( snet_record_t *rec, int name) 
{
  int offset;
  
  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
                        SNetTencGetNumFields( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "get", "field");
  }
#ifdef DISTRIBUTED_SNET
  return SNetRefGetData(DATA_REC( rec, fields[offset]));
#else
  return( DATA_REC( rec, fields[offset]));
#endif /* DISTRIBUTED_SNET */
}

extern int SNetRecTakeTag( snet_record_t *rec, int name)
{
  int offset;
 
  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)),
                        SNetTencGetNumTags( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "take", "tag");
  }
  SNetTencRenameTag( GetVEnc( rec), name, CONSUMED);
  return( DATA_REC( rec, tags[offset]));
}


extern int SNetRecTakeBTag( snet_record_t *rec, int name)
{
  int offset;
 
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)),
                        SNetTencGetNumBTags( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "take","binding tag");
  }
  SNetTencRenameBTag( GetVEnc( rec), name, CONSUMED);
  return( DATA_REC( rec, btags[offset]));
}


extern void *SNetRecTakeField( snet_record_t *rec, int name)
{
  int offset;

  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
                      SNetTencGetNumFields( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "take","field");
  }
  SNetTencRenameField( GetVEnc( rec), name, CONSUMED);

#ifdef DISTRIBUTED_SNET
  return SNetRefTakeData(DATA_REC( rec, fields[offset]));
#else
  return( DATA_REC( rec, fields[offset]));
#endif /* DISTRIBUTED_SNET */
}

extern int SNetRecGetNumTags( snet_record_t *rec)
{
  GETNUM( SNetTencGetNumTags, SNetTencGetTagNames)
}

extern int SNetRecGetNumBTags( snet_record_t *rec)
{
  GETNUM( SNetTencGetNumBTags, SNetTencGetBTagNames)
}

extern int SNetRecGetNumFields( snet_record_t *rec)
{
  GETNUM( SNetTencGetNumFields, SNetTencGetFieldNames)
}

extern int *SNetRecGetTagNames( snet_record_t *rec)
{
  return( SNetTencGetTagNames( GetVEnc( rec)));
}


extern int *SNetRecGetBTagNames( snet_record_t *rec)
{
  return( SNetTencGetBTagNames( GetVEnc( rec)));
}


extern int *SNetRecGetFieldNames( snet_record_t *rec)
{
  return( SNetTencGetFieldNames( GetVEnc( rec)));
}


extern int SNetRecGetNum( snet_record_t *rec)
{
  int counter;
  
  switch( REC_DESCR( rec)) {
    case REC_sort_begin:
      counter = SORT_B_REC( rec, num);
      break;
    case REC_sort_end:
      counter = SORT_E_REC( rec, num);
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetCounter() (%d)", REC_DESCR( rec));
      break;
  }
  
  return( counter);
}

extern void SNetRecSetNum( snet_record_t *rec, int value)
{
    switch( REC_DESCR( rec)) {
    case REC_sort_begin:
      SORT_B_REC( rec, num) = value;
      break;
    case REC_sort_end:
      SORT_E_REC( rec, num) = value;
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetCounter() (%d)", REC_DESCR( rec));
      break;
  }
}


extern int SNetRecGetLevel( snet_record_t *rec)
{
  int counter;
  
  switch( REC_DESCR( rec)) {
    case REC_sort_begin:
      counter = SORT_B_REC( rec, level);
      break;
    case REC_sort_end:
      counter = SORT_E_REC( rec, level);
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetCounter() (%d)", REC_DESCR( rec));
      break;
  }
  return( counter);
}


extern void SNetRecSetLevel( snet_record_t *rec, int value)
{
  switch( REC_DESCR( rec)) {
    case REC_sort_begin:
      SORT_B_REC( rec, level) = value;
      break;
    case REC_sort_end:
      SORT_E_REC( rec, level) = value;
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetCounter() (%d)", REC_DESCR( rec));
      break;
  }
}
#define GET_UNCONSUMED_NAMES( RECNUM, RECNAMES)\
  int *names, i,j,k;\
  if( RECNUM( rec) == 0) {\
    names = NULL;\
  }\
  else {\
    names = SNetMemAlloc( RECNUM( rec) * sizeof( int));\
   j = 0; k = 0;\
   for( i=0; i<RECNUM( rec); i++) {\
       while( RECNAMES( rec)[k] == CONSUMED) {\
         k += 1;\
       }\
       names[j] = RECNAMES( rec)[k];\
       j += 1; k += 1;\
    }\
  }\
  return( names);


extern int *SNetRecGetUnconsumedFieldNames( snet_record_t *rec)
{
  GET_UNCONSUMED_NAMES( SNetRecGetNumFields, SNetRecGetFieldNames);
}

extern int *SNetRecGetUnconsumedTagNames( snet_record_t *rec)
{
  GET_UNCONSUMED_NAMES( SNetRecGetNumTags, SNetRecGetTagNames);
}

extern int *SNetRecGetUnconsumedBTagNames( snet_record_t *rec)
{
  GET_UNCONSUMED_NAMES( SNetRecGetNumBTags, SNetRecGetBTagNames);
}


#define ADD_TO_RECORD( TENCADD, TENCNUM, FIELD, TYPE)\
  int i;\
  TYPE *data;\
  if( TENCADD( GetVEnc( rec), name)) {\
    data = SNetMemAlloc( TENCNUM( GetVEnc( rec)) * sizeof( TYPE));\
    for( i=0; i<( TENCNUM( GetVEnc( rec)) - 1); i++) {\
      data[i] = rec->rec->data_rec->FIELD[i];\
    }\
    SNetMemFree( rec->rec->data_rec->FIELD);\
    rec->rec->data_rec->FIELD = data;\
    return( true);\
  }\
  else {\
    return( false);\
  }

extern bool SNetRecAddTag( snet_record_t *rec, int name)
{
  ADD_TO_RECORD( SNetTencAddTag, SNetTencGetNumTags, tags, int);
}
extern bool SNetRecAddBTag( snet_record_t *rec, int name)
{
  ADD_TO_RECORD( SNetTencAddBTag, SNetTencGetNumBTags, btags, int);
}
extern bool SNetRecAddField( snet_record_t *rec, int name)
{
#ifdef DISTRIBUTED_SNET
  ADD_TO_RECORD( SNetTencAddField, SNetTencGetNumFields, fields, snet_ref_t *);
#else
  ADD_TO_RECORD( SNetTencAddField, SNetTencGetNumFields, fields, void*);
#endif /* DISTRIBUTED_SNET */
}


extern void SNetRecRenameTag( snet_record_t *rec, int name, int new_name)
{
  SNetTencRenameTag( GetVEnc( rec), name, new_name);
}
extern void SNetRecRenameBTag( snet_record_t *rec, int name, int new_name)
{
  SNetTencRenameBTag( GetVEnc( rec), name, new_name);
}
extern void SNetRecRenameField( snet_record_t *rec, int name, int new_name)
{
  SNetTencRenameField( GetVEnc( rec), name, new_name);
}


extern void SNetRecRemoveTag( snet_record_t *rec, int name)
{
  REMOVE_FROM_REC( SNetRecGetNumTags, SNetRecGetUnconsumedTagNames,
                      SNetRecGetTag, SNetTencRemoveTag, tags, int);
}
extern void SNetRecRemoveBTag( snet_record_t *rec, int name)
{
  REMOVE_FROM_REC( SNetRecGetNumBTags, SNetRecGetUnconsumedBTagNames,
            SNetRecGetBTag, SNetTencRemoveBTag, btags, int);
}
extern void SNetRecRemoveField( snet_record_t *rec, int name)
{
#ifdef DISTRIBUTED_SNET
  REMOVE_FROM_REC( SNetRecGetNumFields, SNetRecGetUnconsumedFieldNames,
                        SNetRecGetField, SNetTencRemoveField, fields, snet_ref_t *);
#else
  REMOVE_FROM_REC( SNetRecGetNumFields, SNetRecGetUnconsumedFieldNames,
                        SNetRecGetField, SNetTencRemoveField, fields, void*);
#endif /* DISTRIBUTED_SNET */
}


extern snet_record_t *SNetRecCopy( snet_record_t *rec)
{
  int i;
  snet_record_t *new_rec;
  snet_util_stack_iterator_t *current_iteration;
  int temp;
#ifdef DISTRIBUTED_SNET
  int num;
  int *names;
#else
  snet_copy_fun_t copyfun;
#endif /* DISTRIBUTED_SNET */

  switch( REC_DESCR( rec)) {
  case REC_data:
    new_rec = SNetRecCreate( REC_data,
			     SNetTencCopyVariantEncoding( GetVEnc( rec)));
    for( i=0; i<SNetRecGetNumTags( rec); i++) {
      DATA_REC( new_rec, tags[i]) = DATA_REC( rec, tags[i]);
    }
    for( i=0; i<SNetRecGetNumBTags( rec); i++) {
      DATA_REC( new_rec, btags[i]) = DATA_REC( rec, btags[i]);
    }
#ifdef DISTRIBUTED_SNET
    num = SNetRecGetNumFields( rec);
    names = SNetRecGetUnconsumedFieldNames( rec);

    for( i=0; i<num; i++) {
      SNetRecCopyFieldToRec( rec,  names[i],
			     new_rec, names[i]);
    }

    SNetMemFree(names);
#else
    copyfun = SNetGetCopyFunFromRec( rec);
    for( i=0; i<SNetRecGetNumFields( rec); i++) {
      DATA_REC( new_rec, fields[i]) = copyfun( DATA_REC( rec, fields[i]));
    }
#endif /* DISTRIBUTED_SNET */
    SNetRecSetInterfaceId( new_rec, SNetRecGetInterfaceId( rec));
    
    SNetRecSetDataMode( new_rec, SNetRecGetDataMode( rec));
      
    break;
  case REC_sort_begin:
    new_rec = SNetRecCreate( REC_DESCR( rec),  SORT_B_REC( rec, level),
			     SORT_B_REC( rec, num));
    break;
    case REC_sort_end:
      new_rec = SNetRecCreate( REC_DESCR( rec),  SORT_E_REC( rec, level),
                               SORT_E_REC( rec, num));
      break;
  case REC_terminate:
    new_rec = SNetRecCreate( REC_terminate);
    break;
  case REC_probe:
    new_rec = SNetRecCreate(REC_probe);
    break;
#ifdef DISTRIUBTED_SNET
  case REC_route_update:
    new_rec = SNetRecCreate(REC_DESCR( rec), UPDATE_REC(rec, node),
			    UPDATE_REC(rec, index), UPDATE_REC(rec, stream));
    break;
  case REC_route_redirect:
    new_rec = SNetRecCreate(REC_DESCR( rec), REDIRECT_REC(rec, node),
			    REDIRECT_REC(rec, index));
    break;
  case REC_route_concatenate:
    new_rec = SNetRecCreate(REC_DESCR( rec), CONCAT_REC(rec, index),
			    CONCAT_REC(rec, stream), SNetRecCopy(CONCAT_REC(rec, rec)));
    break;
#endif
    default:
      SNetUtilDebugFatal("Can't copy record of that type (%d)",
                                              REC_DESCR( rec));
  }
  
  current_iteration = SNetUtilStackBottom(rec->iteration_counters);
  while(SNetUtilStackIterCurrDefined(current_iteration)) {
    temp = SNetUtilStackIterGet(current_iteration);
    SNetUtilStackPush(new_rec->iteration_counters, temp);
    current_iteration = SNetUtilStackIterNext(current_iteration);
  }

  SNetUtilStackIterDestroy(current_iteration);
  
  return( new_rec);
}

extern snet_variantencoding_t *SNetRecGetVariantEncoding( snet_record_t *rec)
{
  return( GetVEnc( rec));
}



extern bool SNetRecHasTag( snet_record_t *rec, int name)
{
  bool result;

  result = ( FindName( SNetRecGetTagNames( rec),
             SNetRecGetNumTags( rec), name) == NOT_FOUND) ? false : true;

  return( result);
}

extern bool SNetRecHasBTag( snet_record_t *rec, int name)
{
  bool result;

  result = ( FindName( SNetRecGetBTagNames( rec),
             SNetRecGetNumBTags( rec), name) == NOT_FOUND) ? false : true;

  return( result);
}

extern bool SNetRecHasField( snet_record_t *rec, int name)
{
  bool result;

  result = ( FindName( SNetRecGetFieldNames( rec),
             SNetRecGetNumTags( rec), name) == NOT_FOUND) ? false : true;

  return( result);
}

extern snet_record_t *SNetRecSetDataMode( snet_record_t *rec, snet_record_mode_t mod)
{

  switch( REC_DESCR( rec)) {
    case REC_data:
      DATA_REC( rec, mode) = mod;
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECSetMode() (%d)", REC_DESCR(rec));
      break;
  }

  return( rec);
}

extern snet_record_mode_t SNetRecGetDataMode( snet_record_t *rec)
{

  snet_record_mode_t result;

  switch( REC_DESCR( rec)) {
    case REC_data:
      result = DATA_REC( rec, mode);
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECGetMode() (%d)", REC_DESCR(rec));
      break;
  }

  return( result);
}

/* Copy */
extern void SNetRecCopyFieldToRec( snet_record_t *from, int old_name ,
				   snet_record_t *to, int new_name)
{
  int offset_old;
  int offset_new;
#ifndef DISTRIBUTED_SNET
  snet_copy_fun_t copyfun;
#endif /* DISTRIBUTED_SNET */

  offset_old = FindName( SNetTencGetFieldNames( GetVEnc( from)),
			 SNetTencGetNumFields( GetVEnc( from)), old_name);

  offset_new = FindName( SNetTencGetFieldNames( GetVEnc( to)),
			SNetTencGetNumFields( GetVEnc( to)), new_name);

  if( offset_new == NOT_FOUND || offset_old == NOT_FOUND) {
    NotFoundError( new_name, "get", "field");
  }

#ifdef DISTRIBUTED_SNET
  DATA_REC( to, fields[offset_new]) = SNetRefCopy(DATA_REC( from, fields[offset_old]));
#else
  copyfun = SNetGlobalGetCopyFun( SNetGlobalGetInterface( 
				   SNetRecGetInterfaceId( from)));

  DATA_REC( to, fields[offset_new]) = copyfun(DATA_REC( from, fields[offset_old]));
#endif /* DISTRIBUTED_SNET */
  return; 
}

/* Take */
extern void SNetRecMoveFieldToRec( snet_record_t *from, int old_name ,
				   snet_record_t *to, int new_name)
{
  int offset_old;
  int offset_new;

  offset_old = FindName( SNetTencGetFieldNames( GetVEnc( from)),
			 SNetTencGetNumFields( GetVEnc( from)), old_name);

  offset_new = FindName( SNetTencGetFieldNames( GetVEnc( to)),
			SNetTencGetNumFields( GetVEnc( to)), new_name);

  if( offset_new == NOT_FOUND || offset_old == NOT_FOUND) {
    NotFoundError( new_name, "get", "field");
  }

  SNetTencRenameField( GetVEnc( from), old_name, CONSUMED);

#ifdef DISTRIBUTED_SNET
  DATA_REC( to, fields[offset_new]) = DATA_REC( from, fields[offset_old]);
#else
  DATA_REC( to, fields[offset_new]) = DATA_REC( from, fields[offset_old]);
#endif /* DISTRIBUTED_SNET */
  DATA_REC( from, fields[offset_old]) = NULL;
}

#ifdef DISTRIBUTED_SNET
extern int SNetRecGetNode( snet_record_t *rec)
{
  int result;

  switch( REC_DESCR( rec)) {
  case REC_route_update:
    result = UPDATE_REC( rec, node);
    break;
  case REC_route_redirect:
    result = REDIRECT_REC( rec, node);
    break;
  default:
    SNetUtilDebugFatal("Wrong type in SNetRecGetNode() (%d)", REC_DESCR(rec));
    break;
  }
  
  return result;
}

extern void SNetRecSetNode( snet_record_t *rec, int node)
{

  switch( REC_DESCR( rec)) {
  case REC_route_update:
    UPDATE_REC( rec, node) = node;
    break;
  case REC_route_redirect:
    REDIRECT_REC( rec, node) = node;
    break;
  default:
    SNetUtilDebugFatal("Wrong type in SNetRecGetNode() (%d)", REC_DESCR(rec));
    break;
  }
 
}

extern int SNetRecGetIndex( snet_record_t *rec)
{
  int result;

  switch( REC_DESCR( rec)) {
  case REC_route_update:
    result = UPDATE_REC( rec, index);
    break;
  case REC_route_redirect:
    result = REDIRECT_REC( rec, index);
    break;
  case REC_route_concatenate:
    result = CONCAT_REC( rec, index);
    break;
  default:
    SNetUtilDebugFatal("Wrong type in SNetRecGetIndex() (%d)", REC_DESCR(rec));
    break;
  }
  
  return result;
}

extern int SNetRecPack(snet_record_t *rec, MPI_Comm comm, int *pos, void *buf, int buf_size)
{
  int result;
  int num;
  int *names;
  int i;
  int offset;

  /* Pack record type */
  if((result = MPI_Pack(&rec->rec_descr, 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
    return result; 
  }

  switch(REC_DESCR(rec)) {
  case REC_data:
    /* Pack interface id and mode */
    if((result = MPI_Pack(&DATA_REC(rec, interface_id), 2, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
      return result; 
    }

    num = SNetRecGetNumFields(rec);
    names = SNetRecGetUnconsumedFieldNames(rec);

    /* First pack number of fields and then all the fields. */
    if((result = MPI_Pack(&num, 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
      return result; 
    }

    for(i = 0; i < num; i++) {
      offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
			 SNetTencGetNumFields( GetVEnc( rec)), names[i]);

      if((result = MPI_Pack(&names[i], 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
	return result; 
      }

      if((result = SNetRefPack(DATA_REC( rec, fields[offset]), comm, pos, buf, buf_size)) != MPI_SUCCESS) {
	return result; 
      }

      //DATA_REC( rec, fields[offset]) = NULL;

      SNetTencRenameField( GetVEnc( rec), names[i], CONSUMED);
    }
  
    SNetMemFree(names);

    /* First pack number of tags and then all the tags. */ 

    num = SNetRecGetNumTags(rec);
    names = SNetRecGetTagNames(rec);

    if((result = MPI_Pack(&num, 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
      return result; 
    }

    for(i = 0; i < num; i++) {
      offset = FindName( SNetTencGetTagNames( GetVEnc( rec)),
			 SNetTencGetNumTags( GetVEnc( rec)), names[i]);

      if((result = MPI_Pack(&names[i], 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
	return result; 
      }

      if((result = MPI_Pack(&DATA_REC( rec, tags[offset]), 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
	return result; 
      }


      SNetTencRenameTag( GetVEnc( rec), names[i], CONSUMED);
    }

    /* First pack number of btags and then all the btags. */ 

    num = SNetRecGetNumBTags(rec);
    names = SNetRecGetBTagNames(rec);

    if((result = MPI_Pack(&num, 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
      return result; 
    }

    for(i = 0; i < num; i++) {
      offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)),
			 SNetTencGetNumBTags( GetVEnc( rec)), names[i]);

      if((result = MPI_Pack(&names[i], 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
	return result; 
      }

      if((result = MPI_Pack(&DATA_REC( rec, btags[offset]), 1, MPI_INT, buf, buf_size, pos, comm)) != MPI_SUCCESS) {
	return result; 
      }

      SNetTencRenameBTag( GetVEnc( rec), names[i], CONSUMED);
    }

    break;
  case REC_terminate:
    break;
  case REC_sort_begin:
    /* Pack num and level. */ 
    return MPI_Pack(&SORT_B_REC( rec, num), 2, MPI_INT, buf, buf_size, pos, comm);
    break;
  case REC_sort_end:
    /* Pack num and level. */ 
    return MPI_Pack(&SORT_E_REC( rec, num), 2, MPI_INT, buf, buf_size, pos, comm);
    break;
  case REC_probe:
    break;
  case REC_route_update:
   /* TODO: */
    SNetUtilDebugFatal("Tried to deserialize REC_route_update.");
    break;
  case REC_route_redirect:
   /* Pack node and index */
    return MPI_Pack(&REDIRECT_REC( rec, node), 2, MPI_INT, buf, buf_size, pos, comm);
    break;
  case REC_sync:
    SNetUtilDebugFatal("Tried to serialize REC_sync.");
    break;
  case REC_collect:
    SNetUtilDebugFatal("Tried to serialize REC_collect.");
    break;
  default:
    SNetUtilDebugFatal("Tried to serialize Unknown record.");
    break;
  }

  return MPI_SUCCESS;

}

extern snet_record_t *SNetRecUnpack(MPI_Comm comm, int *pos, void *buf, int buf_size)
{
  snet_record_t *rec = NULL;
  int i;
  int num;
  int offset;
  int temp_buf[4];

  /* Unpack record type. */ 
  if(MPI_Unpack(buf, buf_size, pos, &temp_buf, 1, MPI_INT, comm) == MPI_SUCCESS) {

    switch(temp_buf[0]) {
    case REC_data:
      rec =  SNetRecCreate(REC_data,
			   SNetTencVariantEncode( 
				SNetTencCreateVector( 0), 
				SNetTencCreateVector( 0), 
				SNetTencCreateVector( 0)));

      /* Unpack interface and mode. */ 
      if(MPI_Unpack(buf, buf_size, pos, &DATA_REC(rec, interface_id), 2, MPI_INT, comm) != MPI_SUCCESS) {
	SNetRecDestroy(rec);
	return NULL;
      }
      
      /* Unpack number of fields and then each field. */ 
      if(MPI_Unpack(buf, buf_size, pos, &num, 1, MPI_INT, comm) != MPI_SUCCESS) {
	SNetRecDestroy(rec);
	return NULL;
      }
      
      for(i = 0; i < num; i++) {
	
	if(MPI_Unpack(buf, buf_size, pos, &temp_buf, 1, MPI_INT, comm) != MPI_SUCCESS) {
	  SNetRecDestroy(rec);
	  return NULL;
	}
	
	if(SNetRecAddField(rec, temp_buf[0])) {
	  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
			   SNetTencGetNumFields( GetVEnc( rec)), temp_buf[0]);
	  
	  if((DATA_REC( rec, fields[offset]) = SNetRefUnpack(comm, pos, buf, buf_size)) == NULL) {
	    SNetRecDestroy(rec);
	    return NULL;
	  }
	  
	} else {
	  /* TODO: Error: could not add field! */
	}
	
      }
      
      /* Unpack number of tags and then each tag. */ 
      if(MPI_Unpack(buf, buf_size, pos, &num, 1, MPI_INT, comm) != MPI_SUCCESS) {
	SNetRecDestroy(rec);
	return NULL;
      }
      
      for(i = 0; i < num; i++) {
	if(MPI_Unpack(buf, buf_size, pos, &temp_buf, 2, MPI_INT, comm) != MPI_SUCCESS) {
	  SNetRecDestroy(rec);
	  return NULL;
	}
	
	SNetRecAddTag(rec, temp_buf[0]);
	
	SNetRecSetTag(rec, temp_buf[0], temp_buf[1]);
      }
      
      /* Unpack number of btags and then each btag. */ 
      if(MPI_Unpack(buf, buf_size, pos, &num, 1, MPI_INT, comm) != MPI_SUCCESS) {
	SNetRecDestroy(rec);
	return NULL;
      }
      
      for(i = 0; i < num; i++) {
	if(MPI_Unpack(buf, buf_size, pos, &temp_buf, 2, MPI_INT, comm) != MPI_SUCCESS) {
	  SNetRecDestroy(rec);
	  return NULL;
	}
	
	SNetRecAddBTag(rec, temp_buf[0]);
	
	SNetRecSetBTag(rec, temp_buf[0], temp_buf[1]);
      }
      
      return rec;
      break;
    case REC_terminate:
      return SNetRecCreate(REC_terminate);
      break;
    case REC_sort_begin:
      /* Unpack num and level. */ 
      if(MPI_Unpack(buf, buf_size, pos, &temp_buf, 2, MPI_INT, comm) == MPI_SUCCESS) {
	rec =  SNetRecCreate(REC_sort_begin, temp_buf[0], temp_buf[1]);
      }
      break;
    case REC_sort_end:
      /* Unpack num and level. */ 
      if(MPI_Unpack(buf, buf_size, pos, &temp_buf, 2, MPI_INT, comm) == MPI_SUCCESS) {
	return SNetRecCreate(REC_sort_end, temp_buf[0], temp_buf[1]);
      }
      break;
    case REC_probe:
      return SNetRecCreate(REC_probe);
      break;
    case REC_route_update:
      SNetUtilDebugFatal("Tried to deserialize REC_route_update.");
      /* TODO: */
      break;
    case REC_route_redirect:
      /* Unpack node and index. */ 
      if(MPI_Unpack(buf, buf_size, pos, &temp_buf, 2, MPI_INT, comm) == MPI_SUCCESS) {
	return SNetRecCreate(REC_route_redirect, temp_buf[0], temp_buf[1]);
      }
      SNetUtilDebugFatal("Tried to deserialize REC_route_redirect.");
      break;
    case REC_sync:
      SNetUtilDebugFatal("Tried to deserialize REC_sync.");
      break;
    case REC_collect:
      SNetUtilDebugFatal("Tried to deserialize REC_collect.");
      break;
    default:
      SNetUtilDebugFatal("Tried to deserialize unknown record.");
      break;
    }
  }

  return NULL;
}
  

#endif /* DISTRIBUTED_SNET */

