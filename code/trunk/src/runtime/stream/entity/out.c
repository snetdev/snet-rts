#include "out.h"
#include "debug.h"
#include "handle_p.h"
#include "memfun.h"
#include "interface_functions.h"
#ifdef DBG_RT_TRACE_OUT_TIMINGS
#include <time.h>
#endif

#include "threading.h"

/* This is used if an initialiser outputs a record */
#define DEFAULT_MODE MODE_textual

snet_handle_t *SNetOutRawArray( snet_handle_t *hnd,
    int if_id, snet_variant_t *variant, void **fields, int *tags, int *btags) 
{
  int i, name;
  snet_record_t *out_rec, *old_rec;
  snet_copy_fun_t copyfun = SNetInterfaceGet(if_id)->copyfun;
#ifdef DBG_RT_TRACE_OUT_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;

  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNotice("[DBG::RT::TimeTrace], SNetOut called from %p at"
                      " %lf\n", hnd, 
                        tv_in.tv_sec + tv_in.tv_usec/1000000.0);
#endif

  // set values from box
  out_rec = SNetRecCreate( REC_data);
  SNetRecSetInterfaceId( out_rec, if_id);

  i = 0;
  VARIANT_FOR_EACH_FIELD(variant, name)
    SNetRecSetField( out_rec, name, copyfun(fields[i]));
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

  // flow inherit
  old_rec = hnd->rec;
  if (SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    SNetRecFlowInherit(variant, old_rec, out_rec);
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
      SNetRecSetField( out_rec, name, va_arg( args, void*));
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
    SNetRecFlowInherit(variant, old_rec, out_rec);
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
