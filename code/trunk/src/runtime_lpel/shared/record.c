/* 
 * record.c
 * Implementation of the record and its functions.
 */


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "memfun.h"
#include "record_p.h"

#include "snettypes.h"
#include "snetentities.h"

#include "debug.h"
#include "distribution.h"

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
#define SORT_E_REC( name, component) RECORD( name, sort_end_rec)->component
#define TERMINATE_REC( name, component) RECORD( name, terminate_hnd)->component
#define COLL_REC( name, component) RECORD( name, coll_rec)->component

typedef struct {
  snet_variantencoding_t *v_enc;
  snet_ref_t **fields;
  int *tags;
  int *btags;
  int interface_id;
  snet_record_mode_t mode;
} data_rec_t;

typedef struct {
  snet_stream_t *input;
} sync_rec_t;


typedef struct {
  int num;
  int level;
} sort_end_t;


typedef struct {
  /* empty */
} terminate_rec_t;

typedef struct {
  snet_stream_t *output;
} coll_rec_t;


union record_types {
  data_rec_t *data_rec;
  sync_rec_t *sync_rec;
  coll_rec_t *coll_rec;
  sort_end_t *sort_end_rec;
  terminate_rec_t *terminate_rec;
};

struct record {
  snet_record_descr_t rec_descr;
  snet_record_types_t *rec;
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

bool SNetRecPatternMatches(snet_variantencoding_t *pat,
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

static void FailedError( int name, char *action, char *type)
{
  SNetUtilDebugFatal("Failed to'%s' %s [%d]",
                      action, type, name);
}
/* *********************************************************** */

snet_record_t *SNetRecCreate( snet_record_descr_t descr, ...)
{
  snet_record_t *rec;
  va_list args;
  snet_variantencoding_t *v_enc;

  rec = SNetMemAlloc( sizeof( snet_record_t));
  REC_DESCR( rec) = descr;

  va_start( args, descr);

  switch( descr) {
    case REC_data:
      v_enc = va_arg( args, snet_variantencoding_t*);

      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, data_rec) = SNetMemAlloc( sizeof( data_rec_t));
      DATA_REC( rec, fields) = SNetMemAlloc( SNetTencGetNumFields( v_enc)
                                             * sizeof( snet_ref_t *));
      DATA_REC( rec, tags) = SNetMemAlloc( SNetTencGetNumTags( v_enc)
                                              * sizeof( int));
      DATA_REC( rec, btags) = SNetMemAlloc( SNetTencGetNumBTags( v_enc)
                                              * sizeof( int));
      DATA_REC( rec, v_enc) = v_enc;
      DATA_REC( rec, mode) = MODE_binary;
      DATA_REC( rec, interface_id) = 0;
      break;
    case REC_trigger_initialiser:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      break;
    case REC_sync:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sync_rec) = SNetMemAlloc( sizeof( sync_rec_t));
      SYNC_REC( rec, input) = va_arg( args, snet_stream_t *);
      break;
    case REC_collect:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, coll_rec) = SNetMemAlloc( sizeof( coll_rec_t));
      COLL_REC( rec, output) = va_arg( args, snet_stream_t*);
      break;
    case REC_terminate:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      break;
    case REC_sort_end:
      RECPTR( rec) = SNetMemAlloc( sizeof( snet_record_types_t));
      RECORD( rec, sort_end_rec) = SNetMemAlloc( sizeof( sort_end_t));
      SORT_E_REC( rec, level) = va_arg( args, int);
      SORT_E_REC( rec, num) = va_arg( args, int);
      break;

    default:
      SNetUtilDebugFatal("Unknown control record destription. [%d]",
                          descr);
  }

  va_end( args);

  return( rec);
}



void SNetRecDestroy( snet_record_t *rec)
{
  int i;
  int num, *names;
  int offset;
  snet_free_fun_t freefun;

  if( rec != NULL) {
    switch( REC_DESCR( rec)) {
      case REC_data:
        num = SNetRecGetNumFields( rec);
        names = SNetRecGetUnconsumedFieldNames( rec);
        /*
        for( i=0; i<num; i++) {
          offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
              SNetTencGetNumFields( GetVEnc( rec)), names[i]);

          if( offset == NOT_FOUND) {
            NotFoundError( names[i], "destroy","field");
          }

          SNetRefDestroy(DATA_REC( rec, fields)[offset]);
        }*/
        //#ifndef DISTRIBUTED
        freefun = SNetGetFreeFunFromRec( rec);
        for (i=0; i<num; i++) {
            freefun(SNetRecTakeField( rec, names[i]));
        }
        //#endif
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
        SNetMemFree( RECORD( rec, coll_rec));
        break;

      case REC_sort_end:
        SNetMemFree( RECORD( rec, sort_end_rec));
        break;

      case REC_terminate:
        break;

      case REC_trigger_initialiser:
        break;

      default:
        SNetUtilDebugFatal("Unknown control record description, in RECdestroy");
        break;
    }
    SNetMemFree( RECPTR( rec));
    SNetMemFree( rec);
  }
}

snet_record_descr_t SNetRecGetDescriptor( snet_record_t *rec)
{
  return( REC_DESCR( rec));
}

snet_record_t *SNetRecSetInterfaceId( snet_record_t *rec, int id)
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

int SNetRecGetInterfaceId( snet_record_t *rec)
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

snet_stream_t *SNetRecGetStream( snet_record_t *rec)
{
  snet_stream_t *result;

  switch( REC_DESCR( rec)) {
  case REC_sync:
    result = SYNC_REC( rec, input);
    break;
  case REC_collect:
    result = COLL_REC( rec, output);
    break;
  default:
    SNetUtilDebugFatal("Wrong type in RECgetBuffer() (%d)", REC_DESCR(rec));
    break;
  }
  return result;
}


void SNetRecSetTag( snet_record_t *rec, int name, int val)
{
  int offset=0;
  
  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)),
                SNetTencGetNumTags( GetVEnc( rec)), name);
  DATA_REC( rec, tags[offset]) = val;
}

void SNetRecSetBTag( snet_record_t *rec, int name, int val)
{
  int offset;
  
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)),
                            SNetTencGetNumBTags( GetVEnc( rec)), name);
  DATA_REC( rec, btags[offset]) = val;
}


void SNetRecSetField( snet_record_t *rec, int name, void *val)
{
  int offset;

  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
                     SNetTencGetNumFields( GetVEnc( rec)), name);

  /*
  DATA_REC( rec, fields[offset]) = SNetRefCreate(SNetIDServiceGetID(),
                                                 SNetNodeLocation,
                                                 DATA_REC( rec, interface_id), val);
                                                 */
  DATA_REC( rec, fields[offset]) = val;
}

int SNetRecGetTag( snet_record_t *rec, int name)
{
  int offset;

  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)),
                          SNetTencGetNumTags( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "get","tag");
  }
  return( DATA_REC( rec, tags[offset]));
}

int SNetRecReadTag( snet_record_t *rec, int name) 
{
  return( SNetRecGetTag( rec, name));
}

int SNetRecGetBTag( snet_record_t *rec, int name)
{
  int offset;
  
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)),
                          SNetTencGetNumBTags( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "get","binding tag");
  }
  return( DATA_REC( rec, btags[offset]));
}

void *SNetRecGetField( snet_record_t *rec, int name)
{
  int offset;

  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
                     SNetTencGetNumFields( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "get", "field");
  }
  /*
  return SNetRefGetData(DATA_REC( rec, fields[offset]));
  */
  return (DATA_REC( rec, fields[offset]));
}

void *SNetRecReadField( snet_record_t *rec, int name)
{
  int offset;
  snet_copy_fun_t copyfun;

  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
                     SNetTencGetNumFields( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "get", "field");
  }
  /*
  return SNetRefCopy( SNetRefGetData(DATA_REC( rec, fields[offset])));
  */
  copyfun = SNetGetCopyFunFromRec(rec);
  return (copyfun(DATA_REC(rec, fields[offset])));
}


int SNetRecTakeTag( snet_record_t *rec, int name)
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


int SNetRecTakeBTag( snet_record_t *rec, int name)
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


void *SNetRecTakeField( snet_record_t *rec, int name)
{
  int offset;

  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)),
                     SNetTencGetNumFields( GetVEnc( rec)), name);
  if( offset == NOT_FOUND) {
    NotFoundError( name, "take","field");
  }
  SNetTencRenameField( GetVEnc( rec), name, CONSUMED);

  /*
  return SNetRefTakeData(DATA_REC( rec, fields[offset]));
  */
  return (DATA_REC( rec, fields[offset]));
}

int SNetRecGetNumTags( snet_record_t *rec)
{
  GETNUM( SNetTencGetNumTags, SNetTencGetTagNames)
}

int SNetRecGetNumBTags( snet_record_t *rec)
{
  GETNUM( SNetTencGetNumBTags, SNetTencGetBTagNames)
}

int SNetRecGetNumFields( snet_record_t *rec)
{
  GETNUM( SNetTencGetNumFields, SNetTencGetFieldNames)
}

int *SNetRecGetTagNames( snet_record_t *rec)
{
  return( SNetTencGetTagNames( GetVEnc( rec)));
}


int *SNetRecGetBTagNames( snet_record_t *rec)
{
  return( SNetTencGetBTagNames( GetVEnc( rec)));
}


int *SNetRecGetFieldNames( snet_record_t *rec)
{
  return( SNetTencGetFieldNames( GetVEnc( rec)));
}


int SNetRecGetNum( snet_record_t *rec)
{
  int counter;
  
  switch( REC_DESCR( rec)) {
    case REC_sort_end:
      counter = SORT_E_REC( rec, num);
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetCounter() (%d)", REC_DESCR( rec));
      break;
  }
  
  return( counter);
}

void SNetRecSetNum( snet_record_t *rec, int value)
{
    switch( REC_DESCR( rec)) {
    case REC_sort_end:
      SORT_E_REC( rec, num) = value;
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetCounter() (%d)", REC_DESCR( rec));
      break;
  }
}


int SNetRecGetLevel( snet_record_t *rec)
{
  int counter;
  
  switch( REC_DESCR( rec)) {
    case REC_sort_end:
      counter = SORT_E_REC( rec, level);
      break;
    default:
      SNetUtilDebugFatal("Wrong type in RECgetCounter() (%d)", REC_DESCR( rec));
      break;
  }
  return( counter);
}


void SNetRecSetLevel( snet_record_t *rec, int value)
{
  switch( REC_DESCR( rec)) {
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


int *SNetRecGetUnconsumedFieldNames( snet_record_t *rec)
{
  GET_UNCONSUMED_NAMES( SNetRecGetNumFields, SNetRecGetFieldNames);
}

int *SNetRecGetUnconsumedTagNames( snet_record_t *rec)
{
  GET_UNCONSUMED_NAMES( SNetRecGetNumTags, SNetRecGetTagNames);
}

int *SNetRecGetUnconsumedBTagNames( snet_record_t *rec)
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

bool SNetRecAddTag( snet_record_t *rec, int name)
{
  ADD_TO_RECORD( SNetTencAddTag, SNetTencGetNumTags, tags, int);
}
bool SNetRecAddBTag( snet_record_t *rec, int name)
{
  ADD_TO_RECORD( SNetTencAddBTag, SNetTencGetNumBTags, btags, int);
}
bool SNetRecAddField( snet_record_t *rec, int name)
{
  ADD_TO_RECORD( SNetTencAddField, SNetTencGetNumFields, fields, snet_ref_t *);
}


void SNetRecRenameTag( snet_record_t *rec, int name, int new_name)
{
  SNetTencRenameTag( GetVEnc( rec), name, new_name);
}
void SNetRecRenameBTag( snet_record_t *rec, int name, int new_name)
{
  SNetTencRenameBTag( GetVEnc( rec), name, new_name);
}
void SNetRecRenameField( snet_record_t *rec, int name, int new_name)
{
  SNetTencRenameField( GetVEnc( rec), name, new_name);
}


void SNetRecRemoveTag( snet_record_t *rec, int name)
{
  REMOVE_FROM_REC( SNetRecGetNumTags, SNetRecGetUnconsumedTagNames,
                      SNetRecGetTag, SNetTencRemoveTag, tags, int);
}
void SNetRecRemoveBTag( snet_record_t *rec, int name)
{
  REMOVE_FROM_REC( SNetRecGetNumBTags, SNetRecGetUnconsumedBTagNames,
            SNetRecGetBTag, SNetTencRemoveBTag, btags, int);
}
void SNetRecRemoveField( snet_record_t *rec, int name)
{
  REMOVE_FROM_REC( SNetRecGetNumFields, SNetRecGetUnconsumedFieldNames,
                   SNetRecGetField, SNetTencRemoveField, fields, snet_ref_t *);
}


snet_record_t *SNetRecCopy( snet_record_t *rec)
{
  int i;
  snet_record_t *new_rec;
  int num;
  int *names;
  snet_copy_fun_t copyfun;

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
    /*
    num = SNetRecGetNumFields( rec);
    names = SNetRecGetUnconsumedFieldNames( rec);

    for( i=0; i<num; i++) {
      SNetRecCopyFieldToRec( rec,  names[i],
                             new_rec, names[i]);
    }

    SNetMemFree(names);
    */
    copyfun = SNetGetCopyFunFromRec(rec);
    for (i=0; i <SNetRecGetNumFields( rec); i++) {
        DATA_REC( new_rec, fields[i]) = copyfun(DATA_REC(rec, fields[i]));
    }
    //endif
    SNetRecSetInterfaceId( new_rec, SNetRecGetInterfaceId( rec));

    SNetRecSetDataMode( new_rec, SNetRecGetDataMode( rec));

    break;
  case REC_sort_end:
    new_rec = SNetRecCreate( REC_DESCR( rec),  SORT_E_REC( rec, level),
        SORT_E_REC( rec, num));
    break;
  case REC_terminate:
    new_rec = SNetRecCreate( REC_terminate);
    break;
  default:
    SNetUtilDebugFatal("Can't copy record of that type (%d)",
        REC_DESCR( rec));
  }

  return( new_rec);
}

snet_variantencoding_t *SNetRecGetVariantEncoding( snet_record_t *rec)
{
  return( GetVEnc( rec));
}



bool SNetRecHasTag( snet_record_t *rec, int name)
{
  bool result;

  result = ( FindName( SNetRecGetTagNames( rec),
             SNetRecGetNumTags( rec), name) == NOT_FOUND) ? false : true;

  return( result);
}

bool SNetRecHasBTag( snet_record_t *rec, int name)
{
  bool result;

  result = ( FindName( SNetRecGetBTagNames( rec),
             SNetRecGetNumBTags( rec), name) == NOT_FOUND) ? false : true;

  return( result);
}

bool SNetRecHasField( snet_record_t *rec, int name)
{
  bool result;

  result = ( FindName( SNetRecGetFieldNames( rec),
             SNetRecGetNumTags( rec), name) == NOT_FOUND) ? false : true;

  return( result);
}

snet_record_t *SNetRecSetDataMode( snet_record_t *rec, snet_record_mode_t mod)
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

snet_record_mode_t SNetRecGetDataMode( snet_record_t *rec)
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
void SNetRecCopyFieldToRec( snet_record_t *from, int old_name ,
                                   snet_record_t *to, int new_name)
{
  int offset_old;
  int offset_new;
  snet_copy_fun_t copyfun;

  offset_old = FindName( SNetTencGetFieldNames( GetVEnc( from)),
                         SNetTencGetNumFields( GetVEnc( from)), old_name);

  offset_new = FindName( SNetTencGetFieldNames( GetVEnc( to)),
                        SNetTencGetNumFields( GetVEnc( to)), new_name);

  if(offset_old == NOT_FOUND) {
    NotFoundError( new_name, "copy", "field");
  }

  if(offset_new == NOT_FOUND) {
    if(SNetRecAddField(to, new_name)) {
      FailedError( new_name, "add", "field");
    }

    offset_new = FindName( SNetTencGetFieldNames( GetVEnc( to)),
			   SNetTencGetNumFields( GetVEnc( to)), new_name);
  }

  /*
  DATA_REC( to, fields[offset_new]) = SNetRefCopy(DATA_REC( from, fields[offset_old]));
  */
  copyfun = SNetGlobalGetCopyFun(SNetGlobalGetInterface(SNetRecGetInterfaceId(from)));
  DATA_REC( to, fields[offset_new]) = copyfun(DATA_REC( from, fields[offset_old]));
  return;
}

/* Take */
void SNetRecMoveFieldToRec( snet_record_t *from, int old_name,
                            snet_record_t *to, int new_name)
{
  int offset_old;
  int offset_new;

  offset_old = FindName( SNetTencGetFieldNames( GetVEnc( from)),
                         SNetTencGetNumFields( GetVEnc( from)), old_name);

  offset_new = FindName( SNetTencGetFieldNames( GetVEnc( to)),
                         SNetTencGetNumFields( GetVEnc( to)), new_name);

  if(offset_old == NOT_FOUND) {
    NotFoundError( new_name, "move", "field");
  }

  if(offset_new == NOT_FOUND) {
    if(SNetRecAddField(to, new_name)) {
      FailedError( new_name, "add", "field");
    }

    offset_new = FindName( SNetTencGetFieldNames( GetVEnc( to)),
			   SNetTencGetNumFields( GetVEnc( to)), new_name);
  }

  SNetTencRenameField( GetVEnc( from), old_name, CONSUMED);

  DATA_REC( to, fields[offset_new]) = DATA_REC( from, fields[offset_old]);
  DATA_REC( from, fields[offset_old]) = NULL;
}
