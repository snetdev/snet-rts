#include "out.h"
#include "debug.h"
#include "handle_p.h"
#include "record_p.h"
#include "typeencode.h"
#include "memfun.h"
#include "interface_functions.h"
#ifdef DBG_RT_TRACE_OUT_TIMINGS
#include <time.h>
#endif

#include "task.h"
#include "stream.h"

//#define BOX_DEBUG


/* This is used if an initialiser outputs a record */
#define DEFAULT_MODE MODE_textual


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


snet_handle_t *SNetOut( snet_handle_t *hnd, snet_record_t *rec)
{
  SNetUtilDebugFatal("[SNetOut] Not implemented\n\n");
  return( hnd);
}

snet_handle_t *SNetOutRawArray( snet_handle_t *hnd, 
    int if_id, int variant_num, void **fields, int *tags, int *btags) 
{
  int i, *names;
  snet_record_t *out_rec, *old_rec;
  snet_variantencoding_t *venc;
#ifdef DBG_RT_TRACE_OUT_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;
  
  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrace], SNetOut called from %p at"
                      " %lf\n", hnd, 
                        tv_in.tv_sec + tv_in.tv_usec/1000000.0);
#endif

  // set values from box
  if( variant_num > 0) { 

   venc = SNetTencGetVariant( 
            SNetTencBoxSignGetType( SNetHndGetBoxSign( hnd)), 
            variant_num);

   out_rec = SNetRecCreate( REC_data, SNetTencCopyVariantEncoding( venc));

   SNetRecSetInterfaceId( out_rec, if_id);

   names = SNetTencGetFieldNames( venc);
   for( i=0; i<SNetTencGetNumFields( venc); i++) {
     SNetRecSetField( out_rec, names[i], fields[i]);
   }
   names = SNetTencGetTagNames( venc);
   for( i=0; i<SNetTencGetNumTags( venc); i++) {
     SNetRecSetTag( out_rec, names[i], tags[i]);
   }
   names = SNetTencGetBTagNames( venc);
   for( i=0; i<SNetTencGetNumBTags( venc); i++) {
     SNetRecSetBTag( out_rec, names[i], btags[i]);
   }

   SNetMemFree( fields);
   SNetMemFree( tags);
   SNetMemFree( btags);
  }
  else {
    SNetUtilDebugFatal("[SNetOut] variant_num <= 0\n\n");
  }


  // flow inherit

  old_rec = hnd->rec;
  if( SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    names = SNetRecGetUnconsumedFieldNames( old_rec);
    for( i=0; i<SNetRecGetNumFields( old_rec); i++) {
      if( SNetRecAddField( out_rec, names[i])) {
  
        SNetRecCopyFieldToRec(old_rec, names[i],
  			    out_rec, names[i]);
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
  }

  if( SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    SNetRecSetDataMode( out_rec,  SNetRecGetDataMode( old_rec));
  }
  else {
    SNetRecSetDataMode( out_rec, DEFAULT_MODE);
  }
  
  /* write to stream */
  StreamWrite( hnd->out_sd, out_rec);

#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrace] SNetOut finished for %p at %lf\n",  hnd,
      (tv_out.tv_sec - tv_in.tv_sec) +(tv_out.tv_usec-tv_in.tv_usec) / 1000000.0
      );
#endif
  return( hnd);
}



/**
 * Output Raw V
 */
snet_handle_t *SNetOutRawV( snet_handle_t *hnd, 
    int id, int variant_num, va_list args)
 {
  int i, *names;
  snet_record_t *out_rec, *old_rec;
  snet_variantencoding_t *venc;
  snet_vector_t *mapping;
  int num_entries, f_count=0, t_count=0, b_count=0;
  int *f_names, *t_names, *b_names;
#ifdef DBG_RT_TRACE_OUT_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;
  
  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrace], SNetOut called from %p at"
                      " %lf\n", hnd, 
                        tv_in.tv_sec + tv_in.tv_usec/1000000.0);
#endif

  // set values from box
  if( variant_num > 0) { 

   venc = SNetTencGetVariant( 
            SNetTencBoxSignGetType( SNetHndGetBoxSign( hnd)), 
            variant_num);

   out_rec = SNetRecCreate( REC_data, SNetTencCopyVariantEncoding( venc));

   SNetRecSetInterfaceId( out_rec, id);

   num_entries = SNetTencGetNumFields( venc) +
                 SNetTencGetNumTags( venc) +
                 SNetTencGetNumBTags( venc);
   mapping = 
     SNetTencBoxSignGetMapping( SNetHndGetBoxSign( hnd), variant_num-1); 
   f_names = SNetTencGetFieldNames( venc);
   t_names = SNetTencGetTagNames( venc);
   b_names = SNetTencGetBTagNames( venc);

   for( i=0; i<num_entries; i++) {
     switch( SNetTencVectorGetEntry( mapping, i)) {
       case field:
         SNetRecSetField( out_rec, f_names[f_count++], va_arg( args, void*));
         break;
       case tag:
         SNetRecSetTag( out_rec, t_names[t_count++], va_arg( args, int));
         break;
       case btag:
         SNetRecSetBTag( out_rec, b_names[b_count++], va_arg( args, int));
         break;
     }
   }
  }
  else {
    SNetUtilDebugFatal("[SNetOut] variant_num <= 0\n\n");
  }


  // flow inherit

  old_rec = hnd->rec;
  
  if( SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) { 
    names = SNetRecGetUnconsumedFieldNames( old_rec);
    for( i=0; i<SNetRecGetNumFields( old_rec); i++) {
      if( SNetRecAddField( out_rec, names[i])) {
  
        SNetRecCopyFieldToRec(old_rec, names[i],
  			    out_rec, names[i]);
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
  }
  // output record
  if( SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    SNetRecSetDataMode( out_rec,  SNetRecGetDataMode( old_rec));
  }
  else {
    SNetRecSetDataMode( out_rec, DEFAULT_MODE);
  }

  /* write to stream */
  StreamWrite( hnd->out_sd, out_rec);



#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrance] SnetOut finished for %p at %lf\n", hnd,
      (tv_out.tv_sec - tv_in.tv_sec) + (tv_out.tv_usec-tv_in.tv_usec) / 1000000.0
      );
#endif
  return( hnd);
}

/**
 * Output Raw
 */
snet_handle_t *SNetOutRaw( snet_handle_t *hnd, 
                           int id, 
			   int variant_num, 
			   ...)
{
  snet_handle_t *h;
  va_list args;

  va_start( args, variant_num);
  h = SNetOutRawV( hnd, id, variant_num, args);
  va_end( args);

  return( h);
}

