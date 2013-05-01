#include <assert.h>


#include "out.h"
#include "debug.h"
#include "handle_p.h"
#include "memfun.h"
#include "interface_functions.h"
#include "type.h"
#include "moninfo.h"

//#define DBG_RT_TRACE_OUT_TIMINGS



#ifdef DBG_RT_TRACE_OUT_TIMINGS
//#include <time.h>
#include <sys/time.h>
#endif

#include "threading.h"

/* This is used if an initialiser outputs a record */
#define DEFAULT_MODE MODE_textual

snet_handle_t *SNetOutRawArray( snet_handle_t *hnd,
    int if_id, snet_variant_t *variant, void **fields, int *tags, int *btags)
{
  int i, name;
  snet_record_t *out_rec, *old_rec;

#ifdef DBG_RT_TRACE_OUT_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;

  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNoticeEnt( hnd->ent,
      "[BOX] SNetOut called at %lf.",
      tv_in.tv_sec + tv_in.tv_usec / 1000000.0
      );
#endif

  // set values from box
  out_rec = SNetRecCreate( REC_data);
  SNetRecSetInterfaceId( out_rec, if_id);

  i = 0;
  VARIANT_FOR_EACH_FIELD(variant, name) {
    SNetRecSetField( out_rec, name, SNetRefCreate(fields[i], if_id));
    i++;
  }

  i = 0;
  VARIANT_FOR_EACH_TAG(variant, name) {
    SNetRecSetTag( out_rec, name, tags[i]);
    i++;
  }

  i = 0;
  VARIANT_FOR_EACH_BTAG(variant, name) {
    SNetRecSetBTag( out_rec, name, btags[i]);
    i++;
  }

  SNetMemFree( fields);
  SNetMemFree( tags);
  SNetMemFree( btags);

  // flow inherit
  old_rec = hnd->rec;
  if (SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    SNetRecFlowInherit(variant, old_rec, out_rec);
    SNetRecSetDataMode( out_rec,  SNetRecGetDataMode( old_rec));
    SNetRecDetrefCopy(out_rec, old_rec);
  } else {
    SNetRecSetDataMode( out_rec, DEFAULT_MODE);
  }


#ifdef USE_USER_EVENT_LOGGING
  /* Emit a monitoring message of a record write by a box takes place */
  SNetThreadingEventSignal( hnd->ent,
      SNetMonInfoCreate( EV_MESSAGE_OUT, MON_RECORD, out_rec)
      );
#endif

  /* write to stream */
  SNetStreamWrite( hnd->out_sd, out_rec);

#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNoticeEnt( hnd->ent,
      "[BOX] SNetOut finished after %lf sec.",
      (tv_out.tv_sec-tv_in.tv_sec)+(tv_out.tv_usec-tv_in.tv_usec)/1000000.0
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
  snet_int_list_t *variant;
  int i, name;

#ifdef DBG_RT_TRACE_OUT_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;

  gettimeofday( &tv_in, NULL);
  SNetUtilDebugNoticeEnt( hnd->ent,
      "[BOX] SNetOut called at %lf.",
      tv_in.tv_sec + tv_in.tv_usec / 1000000.0
      );
#endif

  // set values from box
  if( variant_num > 0) {

    variant = SNetIntListListGet( SNetHndGetVariants( hnd), variant_num - 1);

    out_rec = SNetRecCreate( REC_data);

    SNetRecSetInterfaceId( out_rec, id);

    for(i=0; i<SNetIntListLength( variant); i+=2) {
      name = SNetIntListGet( variant, i+1);
      switch(SNetIntListGet( variant, i)) {
        case field:
          SNetRecSetField( out_rec, name,
              SNetRefCreate(va_arg( args, void*), id));
          break;
        case tag:
          SNetRecSetTag( out_rec, name, va_arg( args, int));
          break;
        case btag:
          SNetRecSetBTag( out_rec, name, va_arg( args, int));
          break;
        default:
          assert(0);
      }
    }
  } else {
    out_rec = NULL;
    SNetUtilDebugFatalEnt( hnd->ent,
        "[BOX] SNetOut: variant_num <= 0"
        );
  }


  // flow inherit

  old_rec = hnd->rec;
  if (SNetRecGetDescriptor( old_rec) != REC_trigger_initialiser) {
    SNetRecFlowInherit(
        SNetVariantListGet( SNetHndGetVariantList( hnd), variant_num -1),
        old_rec,
        out_rec);
    SNetRecSetDataMode( out_rec,  SNetRecGetDataMode( old_rec));
    SNetRecDetrefCopy(out_rec, old_rec);
  } else {
    SNetRecSetDataMode( out_rec, DEFAULT_MODE);
  }


#ifdef USE_USER_EVENT_LOGGING
  /* Emit a monitoring message of a record write by a box takes place */
  SNetThreadingEventSignal( hnd->ent,
      SNetMonInfoCreate( EV_MESSAGE_OUT, MON_RECORD, out_rec)
      );
#endif

  /* write to stream */
  SNetStreamWrite( hnd->out_sd, out_rec);

#ifdef DBG_RT_TRACE_OUT_TIMINGS
  gettimeofday( &tv_out, NULL);
  SNetUtilDebugNoticeEnt( hnd->ent,
      "[BOX] SNetOut finished after %lf sec.",
      (tv_out.tv_sec-tv_in.tv_sec)+(tv_out.tv_usec-tv_in.tv_usec)/1000000.0
      );
#endif

  return hnd;
}
