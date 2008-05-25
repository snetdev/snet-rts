#include "out.h"
#include "debug.h"
#include "handle.h"
#include "record.h"
#include "typeencode.h"
#include "memfun.h"
#include "stdarg.h"
#include "interface_functions.h"
#ifdef DBG_RT_TRACE_OUT_TIMINGS
#include "time.h"
#endif

// used in SNetOut (depends on local variables!)
#define ENTRYSUM( RECNUM, TENCNUM)\
                        RECNUM( HNDgetRecord( hnd)) +\
                        TENCNUM( TENCgetVariant( HNDgetType( hnd), variant_num))

// used in SNetOut (depends on local variables!)
#define FILL_LOOP( RECNUM, RECNAME, TENCNUM, SET1, SET2 )\
  i = 0; cnt = 0;\
  while( cnt < RECNUM( HNDgetRecord( hnd))) {\
    int name;\
    name = RECNAME( HNDgetRecord( hnd))[i];\
    if( name != CONSUMED ) {\
      SET1 ;\
      cnt = cnt + 1;\
    }\
    i = i + 1;\
  }\
  for ( i=0; i < TENCNUM( TENCgetVariant( HNDgetType( hnd), variant_num)); i++) {\
    SET2 ;\
    cnt = cnt + 1;\
  }\

// used in SNetOut (depends on local variables!)
#define FILLVECTOR( RECNUM, RECNAME, NAMES, TENCNUM, TENCNAME) \
          FILL_LOOP( RECNUM, RECNAME, TENCNUM, \
            TENCsetVectorEntry( NAMES, cnt, RECNAME( HNDgetRecord( hnd))[i]),\
            TENCsetVectorEntry( NAMES, cnt, TENCNAME( TENCgetVariant( HNDgetType( hnd), variant_num))[i]))

// used in SNetOut (depends on local variables!)
#define FILLRECORD( RECNUM, RECNAME, RECSET, RECGET, TENCNUM, TENCNAME, TYPE) \
          FILL_LOOP( RECNUM, RECNAME, TENCNUM, \
            RECSET( outrec, name, RECGET( HNDgetRecord( hnd), name)),\
            RECSET( outrec, TENCNAME( TENCgetVariant( HNDgetType( hnd), variant_num))[i], va_arg( args, TYPE)))

/* ------------------------------------------------------------------------- */
/*  SNetOut                                                                  */
/* ------------------------------------------------------------------------- */


extern snet_handle_t *SNetOut( snet_handle_t *hnd, snet_record_t *rec) {
  SNetUtilDebugFatal("[SNetOut] Not implemented\n\n");
  return( hnd);
}

/* MERGE THE SNET_OUTS! */
extern snet_handle_t 
*SNetOutRawArray( snet_handle_t *hnd, 
                  int if_id,
                  int variant_num,
                  void **fields,
                  int *tags,
                  int *btags) 
{
  
#ifdef DBG_RT_TRACE_OUT_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;
#endif

  int i, *names;
  snet_record_t *out_rec, *old_rec;
  snet_variantencoding_t *venc;
  void* (*copyfun)(void*);


#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrace], SNetOut called from %p at"
                      " %lf\n", SNetHndGetBoxfun(hnd), 
                        tv_in.tv_sec + tv_in.tv_usex/1000000.0);
#endif
  venc = SNetTencGetVariant( SNetHndGetType( hnd), variant_num);
  
  if( variant_num > 0) {
    out_rec = SNetRecCreate( REC_data, SNetTencCopyVariantEncoding( venc));

    names = SNetTencGetFieldNames( venc);
    for( i=0; i<SNetTencGetNumFields( venc); i++) {
      SNetRecSetField( out_rec, names[i], fields[i]);
    }
    SNetMemFree( fields); 

    names = SNetTencGetTagNames( venc);
    for( i=0; i<SNetTencGetNumTags( venc); i++) {
      SNetRecSetTag( out_rec, names[i], tags[i]);
    }
    SNetMemFree( tags);

    names = SNetTencGetBTagNames( venc);
    for( i=0; i<SNetTencGetNumBTags( venc); i++) {
      SNetRecSetBTag( out_rec, names[i], btags[i]);
    }
    SNetMemFree( btags);
  }
  else {
    SNetUtilDebugFatal("[SNetOut] Variant <= 0");
  }
  
  SNetRecSetInterfaceId( out_rec, if_id);
  
  old_rec = SNetHndGetRecord( hnd);
  
  copyfun = SNetGlobalGetCopyFun( SNetGlobalGetInterface( 
                SNetRecGetInterfaceId( old_rec)));

  names = SNetRecGetUnconsumedFieldNames( old_rec);
  for( i=0; i<SNetRecGetNumFields( old_rec); i++) {
    if( SNetRecAddField( out_rec, names[i])) {
      SNetRecSetField( out_rec, names[i], copyfun( SNetRecGetField( old_rec, names[i])));
    }
  }
  SNetMemFree( names);

  names = SNetRecGetUnconsumedTagNames( old_rec);
  for( i=0; i<SNetRecGetNumTags( old_rec); i++) {
    if( SNetRecAddTag( out_rec, names[i])) {
      SNetRecSetTag( out_rec, names[i], SNetRecGetTag( old_rec, names[i]));
    }
  }
  SNetMemFree( names);


  // output record
  SNetBufPut( SNetHndGetOutbuffer( hnd), out_rec);

#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrance] SnetOut finished for %p at %lf\n",
                SNetHandGetBoxfun(hnd),
                (tv_out.tv_sec - tv_in.tv_sec) +
                    (tv_out.tv_usec-tv_in.tv_usec) / 1000000.0);
#endif

  return( hnd);
}


extern snet_handle_t *SNetOutRaw( snet_handle_t *hnd, int variant_num, ...) {

  int i, *names;
  va_list args;
  snet_record_t *out_rec, *old_rec;
  snet_variantencoding_t *venc;

  // set values from box
  if( variant_num > 0) { 

   venc = SNetTencGetVariant( SNetHndGetType( hnd), variant_num);
   out_rec = SNetRecCreate( REC_data, SNetTencCopyVariantEncoding( venc));

   va_start( args, variant_num);
   names = SNetTencGetFieldNames( venc);
   for( i=0; i<SNetTencGetNumFields( venc); i++) {
     SNetRecSetField( out_rec, names[i], va_arg( args, void*));
   }
   names = SNetTencGetTagNames( venc);
   for( i=0; i<SNetTencGetNumTags( venc); i++) {
     SNetRecSetTag( out_rec, names[i], va_arg( args, int));
   }
   names = SNetTencGetBTagNames( venc);
   for( i=0; i<SNetTencGetNumBTags( venc); i++) {
     SNetRecSetBTag( out_rec, names[i], va_arg( args, int));
   }
   va_end( args);
  }
  else {
    SNetUtilDebugFatal("[SNetOut] Varian <= 0\n\n");
  }


  // flow inherit

  old_rec = SNetHndGetRecord( hnd);

  names = SNetRecGetUnconsumedFieldNames( old_rec);
  for( i=0; i<SNetRecGetNumFields( old_rec); i++) {
    if( SNetRecAddField( out_rec, names[i])) {
      SNetRecSetField( out_rec, names[i], SNetRecGetField( old_rec, names[i]));
    }
  }
  SNetMemFree( names);

  names = SNetRecGetUnconsumedTagNames( old_rec);
  for( i=0; i<SNetRecGetNumTags( old_rec); i++) {
    if( SNetRecAddTag( out_rec, names[i])) {
      SNetRecSetTag( out_rec, names[i], SNetRecGetTag( old_rec, names[i]));
    }
  }
  SNetMemFree( names);


  // output record
  SNetBufPut( SNetHndGetOutbuffer( hnd), out_rec);

  return( hnd);
}
