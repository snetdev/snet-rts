#include "parallel.h"
#include "handle.h"
#include "record.h"
#include "typeencode.h"
#include "collectors.h"
#include "memfun.h"
#include "debug.h"
#include "threading.h"
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

/* ------------------------------------------------------------------------- */
/*  SNetParallel                                                             */
/* ------------------------------------------------------------------------- */

#define FIND_NAME_LOOP( RECNUM, TENCNUM, RECNAMES, TENCNAMES)\
for( i=0; i<TENCNUM( venc); i++) {\
       if( !( ContainsName( TENCNAMES( venc)[i], RECNAMES( rec), RECNUM( rec)))) {\
        MC_ISMATCH( mc) = false;\
      }\
      else {\
        MC_COUNT( mc) += 1;\
      }\
    }




static match_count_t 
*CheckMatch( snet_record_t *rec, 
             snet_typeencoding_t *tenc, 
             match_count_t *mc) {

  snet_variantencoding_t *venc;
  int i,j,max=-1;

  for( j=0; j<SNetTencGetNumVariants( tenc); j++) {
    venc = SNetTencGetVariant( tenc, j+1);
    MC_COUNT( mc) = 0;
    MC_ISMATCH( mc) = true;

    if( ( SNetRecGetNumFields( rec) < SNetTencGetNumFields( venc)) ||
        ( SNetRecGetNumTags( rec) < SNetTencGetNumTags( venc)) ||
        ( SNetRecGetNumBTags( rec) != SNetTencGetNumBTags( venc))) {
      MC_ISMATCH( mc) = false;
    }
    else { // is_match is set to value inside the macros
      FIND_NAME_LOOP( SNetRecGetNumFields, SNetTencGetNumFields,
          SNetRecGetFieldNames, SNetTencGetFieldNames);
      FIND_NAME_LOOP( SNetRecGetNumTags, SNetTencGetNumTags, 
          SNetRecGetTagNames, SNetTencGetTagNames);
  
      for( i=0; i<SNetRecGetNumBTags( rec); i++) {
         if(!SNetTencContainsBTagName(venc, SNetRecGetBTagNames(rec)[i])) {
          MC_ISMATCH( mc) = false;
        }
        else {
         MC_COUNT( mc) += 1;
        }
      }
    }
    if( MC_ISMATCH( mc)) {
      max = MC_COUNT( mc) > max ? MC_COUNT( mc) : max;
    }
  } // for all variants

  if( max >= 0) {
    MC_ISMATCH( mc) = true;
    MC_COUNT( mc) = max;
  }
  
  return( mc);
}

// Checks for "best match" and decides which buffer to dispatch to
// in case of a draw.
static int BestMatch( match_count_t **counter, int num) {

  int i;
  int res, max;
  
  res = -1;
  max = -1;
  for( i=0; i<num; i++) {
    if( MC_ISMATCH( counter[i])) {
      if( MC_COUNT( counter[i]) > max) {
        res = i;
        max = MC_COUNT( counter[i]);
      }
    }
  }

  return( res);
}


static void PutToBuffers( snet_buffer_t **buffers, int num, 
                          int idx, snet_record_t *rec, int counter, bool det) {

  int i;

  for( i=0; i<num; i++) {
    if( det) {
      SNetBufPut( buffers[i], SNetRecCreate( REC_sort_begin, 0, counter));
    }

    if( i == idx) {
      SNetBufPut( buffers[i], rec);
    }

    if( det) {
      SNetBufPut( buffers[i], SNetRecCreate( REC_sort_end, 0, counter));
    }
  }
}

static void *ParallelBoxThread( void *hndl) {

  int i, num, buf_index;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_record_t *rec;
  snet_buffer_t *go_buffer = NULL;
  match_count_t **matchcounter;
  snet_buffer_t **buffers;
  snet_typeencoding_list_t *types;

  bool is_det = SNetHndIsDet( hnd);
  
  bool terminate = false;
  int counter = 1;

  buffers = SNetHndGetOutbuffers( hnd);

  types = SNetHndGetTypeList( hnd);
  num = SNetTencGetNumTypes( types);

  matchcounter = SNetMemAlloc( num * sizeof( match_count_t*));

  for( i=0; i<num; i++) {
    matchcounter[i] = SNetMemAlloc( sizeof( match_count_t));
  }

  while( !( terminate)) {
    
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        for( i=0; i<num; i++) {
          CheckMatch( rec, SNetTencGetTypeEncoding( types, i), matchcounter[i]);
        }
 
        buf_index = BestMatch( matchcounter, num);
        go_buffer = buffers[ buf_index];
        PutToBuffers( buffers, num, buf_index, rec, counter, is_det); 
        counter += 1;
      break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        SNetUtilDebugNotice("[Parallel] Unhandled control record, destroying" 
                            " it\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
      case REC_sort_end:
        if( is_det) {
          SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        }
        for( i=0; i<(num-1); i++) {

          SNetBufPut( buffers[i], SNetRecCopy( rec));
        }
        SNetBufPut( buffers[num-1], rec);
        break;
      case REC_terminate:
        terminate = true;
        if( is_det) {
          for( i=0; i<num; i++) {
            SNetBufPut( buffers[i], 
                SNetRecCreate( REC_sort_begin, 0, counter));
          }
        }
        for( i=0; i<num; i++) {
          if( i == (num-1)) {
            SNetBufPut( buffers[i], rec);
          }
          else {
            SNetBufPut( buffers[i], SNetRecCopy( rec));
          }
        }
        for( i=0; i<num; i++) {
          SNetBufBlockUntilEmpty( buffers[i]);
          SNetBufDestroy( buffers[i]);
          SNetMemFree( matchcounter[i]);
        }
        SNetMemFree( buffers);
        SNetMemFree( matchcounter);
        SNetHndDestroy( hnd);
        break;
    }
  }  

  return( NULL);
}



static snet_buffer_t *SNetParallelStartup( snet_buffer_t *inbuf, 
                                           snet_typeencoding_list_t *types, 
                                           void **funs, bool is_det) {

  int i;
  int num;
  snet_handle_t *hnd;
  snet_buffer_t *outbuf;
  snet_buffer_t **transits;
  snet_buffer_t **outbufs;
  snet_buffer_t* (*fun)(snet_buffer_t*);
  snet_entity_id_t my_id;

  num = SNetTencGetNumTypes( types);
  outbufs = SNetMemAlloc( num * sizeof( snet_buffer_t*));
  transits = SNetMemAlloc( num * sizeof( snet_buffer_t*));


  transits[0] = SNetBufCreate( BUFFER_SIZE);
  fun = funs[0];
  outbufs[0] = (*fun)( transits[0]);

  if( is_det) {
    outbuf = CreateDetCollector( outbufs[0]);
    SNetBufPut( outbufs[0], SNetRecCreate( REC_sort_begin, 0, 0));
    my_id = ENTITY_parallel_det;
  }
  else {
    outbuf = CreateCollector( outbufs[0]);
    my_id = ENTITY_parallel_nondet;
  }
 
  for( i=1; i<num; i++) {

    transits[i] = SNetBufCreate( BUFFER_SIZE);

    fun = funs[i];
    outbufs[i] = (*fun)( transits[i]);
    SNetBufPut( outbufs[0], SNetRecCreate( REC_collect, outbufs[i]));
    if( is_det) {
      SNetBufPut( outbufs[i], SNetRecCreate( REC_sort_end, 0, 0));
    }
  }
  
  if( is_det) {
    SNetBufPut( outbufs[0], SNetRecCreate( REC_sort_end, 0, 0));
  }
  hnd = SNetHndCreate( HND_parallel, inbuf, transits, types, is_det);

  SNetThreadCreate( ParallelBoxThread, (void*)hnd, my_id);

  SNetMemFree( funs);

  return( outbuf);
}


extern snet_buffer_t *SNetParallel( snet_buffer_t *inbuf,
                                    snet_typeencoding_list_t *types,
                                    ...) {


  va_list args;
  int i, num;
  void **funs;

  num = SNetTencGetNumTypes( types);

  funs = SNetMemAlloc( num * sizeof( void*)); 

  va_start( args, types);
  
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

  return( SNetParallelStartup( inbuf, types, funs, false));
}

extern snet_buffer_t *SNetParallelDet( snet_buffer_t *inbuf,
                                       snet_typeencoding_list_t *types,
                                       ...) {


  va_list args;
  int i, num;
  void **funs;

  num = SNetTencGetNumTypes( types);

  funs = SNetMemAlloc( num * sizeof( void*)); 

  va_start( args, types);
  
  for( i=0; i<num; i++) {
    funs[i] = va_arg( args, void*);
  }
  va_end( args);

  return( SNetParallelStartup( inbuf, types, funs, true));
}


