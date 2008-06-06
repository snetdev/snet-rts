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
#include "stack.h"
#include "debug.h"

//#include <lbindings.h>

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
    }\
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

/* DATA_REC(x, y) -> x->rec->data_rec->y */
typedef struct {
  snet_variantencoding_t *v_enc;
  void **fields;
  int *tags;
  int *btags;
  int interface_id;
} data_rec_t;

typedef struct {
  snet_buffer_t *inbuf;
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
  snet_buffer_t *outbuf;
} star_rec_t;

typedef struct {
  /* empty */
} probe_rec_t;

union record_types {
  data_rec_t *data_rec;
  sync_rec_t *sync_rec;
  star_rec_t *star_rec;
  sort_begin_t *sort_begin_rec;
  sort_end_t *sort_end_rec;
  terminate_rec_t *terminate_rec;
  probe_rec_t *probe_rec;
};

struct record {
  snet_record_descr_t rec_descr;
  snet_record_types_t *rec;
  snet_util_stack_t *iteration_counters;
};


/* *********************************************************** */

static int FindName( int *names, int count, int val)
{

  int i=0;
 if( ( names == NULL)) {
  SNetUtilDebugFatal("Record contains no name encoding. This is a runtime"
                    " system bug. Findname()");
  }
  while( (names[i] != val) && (i < count) ) {
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
      DATA_REC( rec, fields) = SNetMemAlloc( SNetTencGetNumFields( v_enc)
                                              * sizeof( void*));
      DATA_REC( rec, tags) = SNetMemAlloc( SNetTencGetNumTags( v_enc)
                                              * sizeof( int));
      DATA_REC( rec, btags) = SNetMemAlloc( SNetTencGetNumBTags( v_enc)
                                              * sizeof( int));
      DATA_REC( rec, v_enc) = v_enc;
      break;
    case REC_sync:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sync_rec) = SNetMemAlloc( sizeof( sync_rec_t));
      SYNC_REC( rec, inbuf) = va_arg( args, snet_buffer_t*);
      break;
    case REC_collect:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, star_rec) = SNetMemAlloc( sizeof( star_rec_t));
      STAR_REC( rec, outbuf) = va_arg( args, snet_buffer_t*);
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
      RECORD(rec, probe_rec) = SNetMemAlloc(sizeof(sort_end_t));
    break;

    default:
      SNetUtilDebugFatal("Unknown control record destription. [%d]",
                          descr);
  }

  va_end( args);

  return( rec);
}


extern snet_util_stack_t *SNetRectGetIterationStack(snet_record_t *rec)
{
  return rec->iteration_counters;
}

extern void SNetRecDestroy( snet_record_t *rec)
{
  int i;
  int num, *names;
  
  if( rec != NULL) {
    SNetUtilStackDestroy(rec->iteration_counters);
  switch( REC_DESCR( rec)) {
    case REC_data: {
        void (*freefun)(void*);
        num = SNetRecGetNumFields( rec);
        names = SNetRecGetUnconsumedFieldNames( rec);
        freefun = SNetGetFreeFunFromRec( rec);
        for( i=0; i<num; i++) {
          freefun( SNetRecTakeField( rec, names[i]));
        }
        SNetMemFree( names);
        SNetTencDestroyVariantEncoding( DATA_REC( rec, v_enc));
        SNetMemFree( DATA_REC( rec, fields));
        SNetMemFree( DATA_REC( rec, tags));
        SNetMemFree( DATA_REC( rec, btags));
        SNetMemFree( RECORD( rec, data_rec));
      }
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
  int result;

  result = *((int*)SNetUtilStackPeek(rec->iteration_counters));

  return result;
}

extern void SNetRecIncIteration(snet_record_t *rec)
{
  int *counter;

  if(SNetUtilStackIsEmpty(rec->iteration_counters)) {
    SNetUtilDebugFatal("IncIteration requested for record without iteration"
                       " counter!");
  }
  counter = ((int*)SNetUtilStackPeek(rec->iteration_counters));

  *counter = *counter + 1;
}

extern void SNetRecAddIteration(snet_record_t *rec, int initial_value)
{
  int *new_iteration;

  new_iteration = (int*) malloc(sizeof(int));
  *new_iteration = initial_value;

  SNetUtilStackPush(rec->iteration_counters, new_iteration);
}

extern void SNetRecRemoveIteration(snet_record_t *rec)
{
  int *old_iteration;

  if(SNetUtilStackIsEmpty(rec->iteration_counters)) {
    SNetUtilDebugFatal("Removal of iteration counter requested without any "
                       "iterationcounters present");
  }

  old_iteration = SNetUtilStackPeek(rec->iteration_counters);
  SNetUtilStackPop(rec->iteration_counters);
  free(old_iteration);
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

extern snet_buffer_t *SNetRecGetBuffer( snet_record_t *rec)
{
  snet_buffer_t *inbuf;

  switch( REC_DESCR( rec)) {
    case REC_sync:
      inbuf = SYNC_REC( rec, inbuf);
      break;
    case REC_collect:
      inbuf = STAR_REC( rec, outbuf);
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetBuffer() (%d)", REC_DESCR(rec));
      break;
  }

  return( inbuf);
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
  DATA_REC( rec, fields[offset]) = val;
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
  return( DATA_REC( rec, fields[offset]));
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
  return( DATA_REC( rec, fields[offset]));
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
  ADD_TO_RECORD( SNetTencAddField, SNetTencGetNumFields, fields, void*);
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
  REMOVE_FROM_REC( SNetRecGetNumFields, SNetRecGetUnconsumedFieldNames,
                        SNetRecGetField, SNetTencRemoveField, fields, void*);
}


extern snet_record_t *SNetRecCopy( snet_record_t *rec)
{
  int i;
  snet_record_t *new_rec;
  int *temp;

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
      void* (*copyfun)(void*);
      copyfun = SNetGetCopyFunFromRec( rec);
      for( i=0; i<SNetRecGetNumFields( rec); i++) {
        DATA_REC( new_rec, fields[i]) = copyfun( DATA_REC( rec, fields[i]));
      }
      SNetRecSetInterfaceId( new_rec, SNetRecGetInterfaceId( rec));

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
    default:
      SNetUtilDebugFatal("Can't copy record of that type (%d)",
                                              REC_DESCR( rec));
  }

  new_rec->iteration_counters = SNetUtilStackCreate();
  SNetUtilStackGotoBottom(rec->iteration_counters);
  while(SNetUtilStackCurrentDefined(rec->iteration_counters)) {
    temp = malloc(sizeof(int));
    *temp = *((int*)SNetUtilStackGet(rec->iteration_counters));
    SNetUtilStackPush(new_rec->iteration_counters, temp);
    SNetUtilStackUp(rec->iteration_counters);
  }
  return( new_rec);
}
/*
extern snet_lang_descr_t SNetRecGetLanguage( snet_record_t *rec) {

  snet_lang_descr_t res;

  switch( REC_DESCR( rec)) {
    case REC_data:
      res = DATA_REC( rec, lang);
      break;
    default:
      printf("\n\n ** Fatal Error ** : Wrong type in SNetGetLanguage() (%d)"
             "\n\n", REC_DESCR( rec));
      exit( 1);
      break;
  }
  return( res);
}

extern void SNetRecSetLanguage( snet_record_t *rec, snet_lang_descr_t lang) {

  switch( REC_DESCR( rec)) {
    case REC_data:
      DATA_REC( rec, lang) = lang;
      break;
    default:
      printf("\n\n ** Fatal Error ** : Wrong type in SNetSetLanguage() (%d)"
             "\n\n", REC_DESCR( rec));
      exit( 1);
      break;
  }
}
*/

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

