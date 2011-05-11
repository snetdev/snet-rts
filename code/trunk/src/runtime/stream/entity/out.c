#include "out.h"
#include "debug.h"
#include "handle_p.h"
#include "memfun.h"
#include "interface_functions.h"
#ifdef DBG_RT_TRACE_OUT_TIMINGS
#include <time.h>
#endif

#include "threading.h"

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
  int i, name;
  snet_record_t *out_rec, *old_rec;
  snet_variant_t *variant;
#ifdef DBG_RT_TRACE_OUT_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;

  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrace], SNetOut called from %p at"
                      " %lf\n", hnd, 
                        tv_in.tv_sec + tv_in.tv_usec/1000000.0);
#endif

  // set values from box
  if (variant_num > 0) {
    variant = SNetVariantListGet( SNetHndGetVariants( hnd), variant_num - 1);

    out_rec = SNetRecCreate( REC_data);

    SNetRecSetInterfaceId( out_rec, if_id);

    i = 0;
    VARIANT_FOR_EACH_FIELD(variant, name)
      SNetRecSetField( out_rec, name, fields[i]);
      i++;
    END_FOR

    i = 0;
    VARIANT_FOR_EACH_TAG(variant, name)
      SNetRecSetTag( out_rec, name, tags[i]);
      i++;
    END_FOR

    i = 0;
    VARIANT_FOR_EACH_BTAG(variant, name)
      SNetRecSetBTag( out_rec, name, btags[i]);
      i++;
    END_FOR

    SNetMemFree( fields);
    SNetMemFree( tags);
    SNetMemFree( btags);
  } else {
    SNetUtilDebugFatal("[SNetOut] variant_num <= 0\n\n");
  }


  // flow inherit
  old_rec = hnd->rec;
  if (SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    int name, val;
    snet_ref_t *ref;

    FOR_EACH_FIELD(old_rec, name, ref)
      if (!SNetRecHasField( out_rec, name)) {
        SNetRecSetField(out_rec, name, ref);
      }
    END_FOR

    FOR_EACH_TAG(old_rec, name, val)
      if (!SNetRecHasTag( out_rec, name)) {
        SNetRecSetTag( out_rec, name, val);
      }
    END_FOR

    SNetRecSetDataMode( out_rec,  SNetRecGetDataMode( old_rec));
  } else {
    SNetRecSetDataMode( out_rec, DEFAULT_MODE);
  }

  /* write to stream */
  SNetStreamWrite( hnd->out_sd, out_rec);

#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrace] SNetOut finished for %p at %lf\n",  hnd,
      (tv_out.tv_sec - tv_in.tv_sec) +(tv_out.tv_usec-tv_in.tv_usec) / 1000000.0
      );
#endif
  return hnd;
}



/**
 * Output Raw V
 */
snet_handle_t *SNetOutRawV( snet_handle_t *hnd, int id, int variant_num,
                            va_list args)
 {
  snet_record_t *out_rec, *old_rec;
  snet_variant_t *variant;
  int name;
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

    variant = SNetVariantListGet( SNetHndGetVariants( hnd), variant_num - 1);

    out_rec = SNetRecCreate( REC_data);

    SNetRecSetInterfaceId( out_rec, id);

    VARIANT_FOR_EACH_FIELD(variant, name)
      SNetRecSetField( out_rec, name, va_arg( args, snet_ref_t*));
    END_FOR

    VARIANT_FOR_EACH_TAG(variant, name)
      SNetRecSetTag( out_rec, name, va_arg( args, int));
    END_FOR

    VARIANT_FOR_EACH_BTAG(variant, name)
      SNetRecSetBTag( out_rec, name, va_arg( args, int));
    END_FOR
  } else {
    SNetUtilDebugFatal("[SNetOut] variant_num <= 0\n\n");
  }


  // flow inherit

  old_rec = hnd->rec;
  if (SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    int name, val;
    snet_ref_t *ref;

    FOR_EACH_FIELD(old_rec, name, ref)
      if (!SNetRecHasField( out_rec, name)) {
        SNetRecSetField( out_rec, name, ref);
      }
    END_FOR

    FOR_EACH_TAG(old_rec, name, val)
      if (!SNetRecHasTag( out_rec, name)) {
        SNetRecSetTag( out_rec, name, val);
      }
    END_FOR

    SNetRecSetDataMode( out_rec,  SNetRecGetDataMode( old_rec));
  } else {
    SNetRecSetDataMode( out_rec, DEFAULT_MODE);
  }

  /* write to stream */
  SNetStreamWrite( hnd->out_sd, out_rec);



#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrance] SnetOut finished for %p at %lf\n", hnd,
      (tv_out.tv_sec - tv_in.tv_sec) + (tv_out.tv_usec-tv_in.tv_usec) / 1000000.0
      );
#endif
  return hnd;
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

