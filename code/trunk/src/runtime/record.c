/* 
 * record.c
 * Implementation of the record and its functions.
 */

// TODO: replace Get/Set/Take function bodies by
//       macro calls (-> getNum)

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <memfun.h>
#include <record.h>



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


typedef struct {
  snet_variantencoding_t *v_enc;
  void **fields;
  int *tags;
  int *btags;
} data_rec_t;

typedef struct {
  snet_buffer_t *inbuf;
} sync_rec_t;

typedef struct {

} terminate_rec_t;

typedef struct {
  snet_buffer_t *outbuf;
} star_rec_t;

union record_types {
  data_rec_t *data_rec;
  sync_rec_t *sync_rec;
  star_rec_t *star_rec;
  terminate_rec_t *terminate_rec;
};

struct record {
  snet_record_descr_t rec_descr;
  snet_record_types_t *rec;
};


/* *********************************************************** */

static int FindName( int *names, int count, int val) {

  int i=0;

  while( (names[i] != val) && (i < count) ) {
    i += 1;
  }

  if( i == count) {
    // Handle this error here?
    i = NOT_FOUND;
  }
   return( i);
}

static snet_variantencoding_t *GetVEnc( snet_record_t *rec) {
  
  return( rec->rec->data_rec->v_enc);
}


static int GetConsumedCount( int *names, int num) {
  
  int counter,i;

  counter=0;
  for( i=0; i<num; i++) {
    if( (names[i] == CONSUMED)) {
      counter = counter + 1;
    }
  }

  return( counter);
}

/* *********************************************************** */


extern snet_record_t *SNetRecCreate( snet_record_descr_t descr, ...) {

  snet_record_t *rec;
  va_list args;
  snet_variantencoding_t *v_enc;

  rec = SNetMemAlloc( sizeof( snet_record_t));
  rec->rec_descr = descr;
  

  va_start( args, descr);

  switch( descr) {
    case REC_data:
      v_enc = va_arg( args, snet_variantencoding_t*);

      rec->rec = SNetMemAlloc( sizeof( snet_record_types_t));
      rec->rec->data_rec = SNetMemAlloc( sizeof( data_rec_t));
      rec->rec->data_rec->fields = SNetMemAlloc( SNetTencGetNumFields( v_enc) * sizeof( void*));
      rec->rec->data_rec->tags = SNetMemAlloc( SNetTencGetNumTags( v_enc) * sizeof( int));
      rec->rec->data_rec->btags = SNetMemAlloc( SNetTencGetNumBTags( v_enc) * sizeof( int));
      rec->rec->data_rec->v_enc = v_enc;
      break;

    case REC_sync:
      rec->rec = SNetMemAlloc( sizeof( snet_record_types_t));
      rec->rec->sync_rec = SNetMemAlloc( sizeof( sync_rec_t));
      rec->rec->sync_rec->inbuf = va_arg( args, snet_buffer_t*);
      break;
    case REC_star:
      rec->rec = SNetMemAlloc( sizeof( snet_record_types_t));
      rec->rec->star_rec = SNetMemAlloc( sizeof( star_rec_t));
      rec->rec->star_rec->outbuf = va_arg( args, snet_buffer_t*);
      break;
    case REC_terminate:
      break;

    default:
      printf("\n\n ** Fatal Error ** : Unknown control record description. \n\n");
      exit( 1);
  }

  va_end( args); 

  return( rec);
}


extern void SNetRecDestroy( snet_record_t *rec) {

  switch( rec->rec_descr) {
    case REC_data:
      SNetTencDestroyVariantEncoding( rec->rec->data_rec->v_enc);
      SNetMemFree( rec->rec->data_rec->fields);
      SNetMemFree( rec->rec->data_rec->tags);
      SNetMemFree( rec->rec->data_rec->btags);
      SNetMemFree( rec->rec->data_rec);
      break;

    case REC_sync:
      SNetMemFree( rec->rec->sync_rec);
      break;
    case REC_star:
      SNetMemFree( rec->rec->sync_rec);
    case REC_terminate:
      break;
      
    default:
      printf("\n\n ** Fatal Error ** : Unknown control record description in RECdestroy(). \n\n");
      exit( 1);
      break;
  }
  SNetMemFree( rec->rec);
  SNetMemFree( rec);
}


extern snet_record_descr_t SNetRecGetDescriptor( snet_record_t *rec) {
  return( rec->rec_descr);
}

extern snet_buffer_t *SNetRecGetBuffer( snet_record_t *rec) {
 
  snet_buffer_t *inbuf;

  switch( rec->rec_descr) {
    case REC_sync:
      inbuf = rec->rec->sync_rec->inbuf;
      break;
    case REC_star:
      inbuf = rec->rec->star_rec->outbuf;
      break;
    default:
      printf("\n\n ** Fatal Error ** : Wrong type in RECgetBuffer() (%d) \n\n", rec->rec_descr);
      exit( 1);
      break;
  }

  return( inbuf);
}


extern void SNetRecSetTag( snet_record_t *rec, int name, int val) {

  int offset=0;

  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)), SNetTencGetNumTags( GetVEnc( rec)), name);
  rec->rec->data_rec->tags[offset] = val;
}

extern void SNetRecSetBTag( snet_record_t *rec, int name, int val) {
 
  int offset;
  
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)), SNetTencGetNumBTags( GetVEnc( rec)), name);
  rec->rec->data_rec->btags[offset] = val;
}


extern void SNetRecSetField( snet_record_t *rec, int name, void *val) {
 
  int offset;
  
  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)), SNetTencGetNumFields( GetVEnc( rec)), name);
  rec->rec->data_rec->fields[offset] = val;
}



extern int SNetRecGetTag( snet_record_t *rec, int name) {
  
  int offset;

  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)), SNetTencGetNumTags( GetVEnc( rec)), name);
  return( rec->rec->data_rec->tags[offset]);
}


extern int SNetRecGetBTag( snet_record_t *rec, int name) {
  
  int offset;
  
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)), SNetTencGetNumBTags( GetVEnc( rec)), name);
  return( rec->rec->data_rec->btags[offset]);
}

extern void *SNetRecGetField( snet_record_t *rec, int name) {
  
  int offset;
  
  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)), SNetTencGetNumFields( GetVEnc( rec)), name);
  return( rec->rec->data_rec->fields[offset]);
}

extern int SNetRecTakeTag( snet_record_t *rec, int name) {
  
  int offset;
 
  offset = FindName( SNetTencGetTagNames( GetVEnc( rec)), SNetTencGetNumTags( GetVEnc( rec)), name);
  SNetTencRenameTag( GetVEnc( rec), name, CONSUMED);  
  return( rec->rec->data_rec->tags[offset]);
}


extern int SNetRecTakeBTag( snet_record_t *rec, int name) {
  
  int offset;
 
  offset = FindName( SNetTencGetBTagNames( GetVEnc( rec)), SNetTencGetNumBTags( GetVEnc( rec)), name);
  SNetTencRenameBTag( GetVEnc( rec), name, CONSUMED);  
  return( rec->rec->data_rec->btags[offset]);
}


extern void *SNetRecTakeField( snet_record_t *rec, int name) {
  
  int offset;
 
  offset = FindName( SNetTencGetFieldNames( GetVEnc( rec)), SNetTencGetNumFields( GetVEnc( rec)), name);
  SNetTencRenameField( GetVEnc( rec), name, CONSUMED);  
  return( rec->rec->data_rec->fields[offset]);
}


extern int SNetRecGetNumTags( snet_record_t *rec) {

  GETNUM( SNetTencGetNumTags, SNetTencGetTagNames)
}

extern int SNetRecGetNumBTags( snet_record_t *rec) {

  GETNUM( SNetTencGetNumBTags, SNetTencGetBTagNames)
}

extern int SNetRecGetNumFields( snet_record_t *rec) {
 
  GETNUM( SNetTencGetNumFields, SNetTencGetFieldNames)
}

extern int *SNetRecGetTagNames( snet_record_t *rec) {

  return( SNetTencGetTagNames( GetVEnc( rec)));
}


extern int *SNetRecGetBTagNames( snet_record_t *rec) {

  return( SNetTencGetBTagNames( GetVEnc( rec)));
}


extern int *SNetRecGetFieldNames( snet_record_t *rec) {

  return( SNetTencGetFieldNames( GetVEnc( rec)));
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


extern int *SNetRecGetUnconsumedFieldNames( snet_record_t *rec) {
  GET_UNCONSUMED_NAMES( SNetRecGetNumFields, SNetRecGetFieldNames);
}

extern int *SNetRecGetUnconsumedTagNames( snet_record_t *rec) {
  GET_UNCONSUMED_NAMES( SNetRecGetNumTags, SNetRecGetTagNames);
}

extern int *SNetRecGetUnconsumedBTagNames( snet_record_t *rec) {
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

extern bool SNetRecAddTag( snet_record_t *rec, int name) {
  ADD_TO_RECORD( SNetTencAddTag, SNetTencGetNumTags, tags, int);
}
extern bool SNetRecAddBTag( snet_record_t *rec, int name) {
  ADD_TO_RECORD( SNetTencAddBTag, SNetTencGetNumBTags, btags, int);
}
extern bool SNetRecAddField( snet_record_t *rec, int name) {
  ADD_TO_RECORD( SNetTencAddField, SNetTencGetNumFields, fields, void*);
}




extern void SNetRecRenameTag( snet_record_t *rec, int name, int new_name) {
  SNetTencRenameTag( GetVEnc( rec), name, new_name);
}
extern void SNetRecRenameBTag( snet_record_t *rec, int name, int new_name) {
  SNetTencRenameBTag( GetVEnc( rec), name, new_name);
}
extern void SNetRecRenameField( snet_record_t *rec, int name, int new_name) {
  SNetTencRenameField( GetVEnc( rec), name, new_name);
}


extern void SNetRecRemoveTag( snet_record_t *rec, int name) {
  REMOVE_FROM_REC( SNetRecGetNumTags, SNetRecGetUnconsumedTagNames, SNetRecGetTag, SNetTencRemoveTag, tags, int);
}
extern void SNetRecRemoveBTag( snet_record_t *rec, int name) {
  REMOVE_FROM_REC( SNetRecGetNumBTags, SNetRecGetUnconsumedBTagNames, SNetRecGetBTag, SNetTencRemoveBTag, btags, int);
}
extern void SNetRecRemoveField( snet_record_t *rec, int name) {
  REMOVE_FROM_REC( SNetRecGetNumFields, SNetRecGetUnconsumedFieldNames, SNetRecGetField, SNetTencRemoveField, fields, void*);
}


extern snet_record_t *RECcopy( snet_record_t *rec) {

  int i;
  snet_record_t *new_rec;

  new_rec = SNetRecCreate( REC_data, SNetTencCopyVariantEncoding( GetVEnc( rec)));

  for( i=0; i<SNetRecGetNumTags( rec); i++) {
    new_rec->rec->data_rec->tags[i] = rec->rec->data_rec->tags[i]; 
  }
  for( i=0; i<SNetRecGetNumBTags( rec); i++) {
    new_rec->rec->data_rec->btags[i] = rec->rec->data_rec->btags[i]; 
  }
  for( i=0; i<SNetRecGetNumFields( rec); i++) {
    new_rec->rec->data_rec->fields[i] = rec->rec->data_rec->fields[i]; 
  }

  return( new_rec);
}

extern snet_variantencoding_t *SNetRecGetVariantEncoding( snet_record_t *rec) {
  return( GetVEnc( rec));
}
