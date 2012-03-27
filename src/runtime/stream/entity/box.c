#include <assert.h>

#include "snetentities.h"
#include "debugtime.h"
#include "debug.h"
#include "memfun.h"
#include "locvec.h"
#include "moninfo.h"
#include "type.h"
#include "threading.h"
#include "handle_p.h"
#include "list.h"
#include "distribution.h"
#include "entities.h"

#ifdef SNET_DEBUG_COUNTERS
#include "debugcounters.h"
#endif /* SNET_DEBUG_COUNTERS  */


//#define DBG_RT_TRACE_BOX_TIMINGS

#ifdef DBG_RT_TRACE_BOX_TIMINGS
#include <sys/time.h>
#endif



typedef struct {
#ifdef SNET_DEBUG_COUNTERS
  snet_time_t time_in;
  snet_time_t time_out;
  long mseconds;
#endif /* SNET_DEBUG_COUNTERS */
#ifdef DBG_RT_TRACE_BOX_TIMINGS
  static struct timeval tv_in;
  static struct timeval tv_out;
#endif
  snet_stream_t *input, *output;
  snet_info_t *info;
  void (*boxfun)( snet_handle_t*);
  snet_int_list_list_t *output_variants;
  const char *boxname;
  snet_variant_list_t *vars;
} box_arg_t;



/* ------------------------------------------------------------------------- */
/*  SNetBox                                                                  */
/* ------------------------------------------------------------------------- */


static void BoxTask(snet_entity_t *ent, void *arg)
{
  box_arg_t *barg = (box_arg_t *)arg;
  snet_record_t *rec;
  snet_stream_desc_t *instream;
  snet_stream_desc_t *outstream;
  snet_handle_t hnd;
  snet_entity_t *newent;

  newent = SNetEntityCopy(ent);
  SNetEntitySetFunction(newent, &BoxTask);

  instream = SNetStreamOpen(barg->input, 'r');
  outstream = SNetStreamOpen(barg->output, 'w');
  /* read from input stream */
  rec = SNetStreamRead( instream);

  /* set out descriptor */
  hnd.out_sd = outstream;
  /* set out signs */
  hnd.sign = barg->output_variants;
  /* mapping */
  hnd.mapping = NULL;
  /* set variants */
  hnd.vars = barg->vars;
  /* box entity */
  hnd.ent = ent;

  switch( SNetRecGetDescriptor(rec)) {
    case REC_trigger_initialiser:
    case REC_data:
      hnd.rec = rec;
#ifdef DBG_RT_TRACE_BOX_TIMINGS
      gettimeofday( &(barg->tv_in), NULL);
      SNetUtilDebugNoticeEnt( ent,
          "[BOX] Firing box function at %lf.",
          barg->tv_in.tv_sec + barg->tv_in.tv_usec / 1000000.0
          );
#endif
#ifdef SNET_DEBUG_COUNTERS
      SNetDebugTimeGetTime(&(barg->time_in));
#endif /* SNET_DEBUG_COUNTERS */

#ifdef USE_USER_EVENT_LOGGING
      /* Emit a monitoring message of a record read to be processed by a box */
      if (SNetRecGetDescriptor(rec) == REC_data) {
        SNetThreadingEventSignal( ent,
            SNetMonInfoCreate( EV_MESSAGE_IN, MON_RECORD, rec)
            );
      }
#endif

      (*barg->boxfun)( &hnd);

      /*
       * Emit an event here?
       * SNetMonInfoEvent( EV_BOX_???, MON_RECORD, rec);
       */

#ifdef DBG_RT_TRACE_BOX_TIMINGS
      gettimeofday( &(barg->tv_out), NULL);
      SNetUtilDebugNoticeEnt( ent,
          "[BOX] Return from box function after %lf sec.",
          (barg->tv_out.tv_sec - barg->tv_in.tv_sec) + (barg->tv_out.tv_usec - 
           barg->tv_in.tv_usec) / 1000000.0
          );
#endif


#ifdef SNET_DEBUG_COUNTERS
      SNetDebugTimeGetTime(&(barg->time_out));
      mseconds = SNetDebugTimeDifferenceInMilliseconds(&(barg->time_in),
                      &(barg->time_out));
      SNetDebugCountersIncreaseCounter(barg->mseconds, SNET_COUNTER_TIME_BOX);
#endif /* SNET_DEBUG_COUNTERS */

      SNetRecDestroy( rec);

      /* restrict to one data record per execution */
      //SNetThreadingYield();
      break;

    case REC_sync:
      {
        snet_stream_t *newstream = SNetRecGetStream(rec);
        SNetStreamReplace( instream, newstream);
        barg->input = newstream;
        SNetRecDestroy( rec);
      }
      break;

    case REC_sort_end:
      /* forward the sort record */
      SNetStreamWrite( outstream, rec);
      break;

    case REC_terminate:
      SNetStreamWrite( outstream, rec);
      SNetStreamClose( instream, true);
      SNetStreamClose( outstream, false);

      //SNetHndDestroy( &hnd);

      /* destroy box arg */
      SNetIntListListDestroy(barg->output_variants);
      SNetMemFree( barg);
      return;
    case REC_collect:
    default:
      assert(0);
      /* if ignore, destroy at least ...*/
      SNetRecDestroy( rec);
  }
  SNetStreamClose( instream, false);
  SNetStreamClose( outstream, false);

  /* schedule new task */
  SNetThreadingSpawn(newent);
}


static void InitBoxTask(snet_entity_t *ent, void *arg){
  snet_entity_t *newent;

  newent = SNetEntityCopy(ent);
  SNetEntitySetFunction(newent, &BoxTask);

  /* schedule new task */
  SNetThreadingSpawn(newent);
}


/**
 * Box creation function
 */
snet_stream_t *SNetBox( snet_stream_t *input,
                        snet_info_t *info,
                        int location,
                        const char *boxname,
                        snet_box_fun_t boxfun,
                        snet_int_list_list_t *output_variants)
{
  int i,j;
  snet_stream_t *output;
  box_arg_t *barg;
  snet_variant_list_t *vlist;

  input = SNetRouteUpdate(info, input, location);

  if(SNetDistribIsNodeLocation(location)) {
    output = SNetStreamCreate(0);
    vlist = SNetVariantListCreate(0);
    for( i=0; i<SNetIntListListLength( output_variants); i++) {
      snet_int_list_t *l = SNetIntListListGet( output_variants, i);
      snet_variant_t *v = SNetVariantCreateEmpty();
      for( j=0; j<SNetIntListLength( l); j+=2) {
        switch( SNetIntListGet( l, j)) {
          case field:
            SNetVariantAddField( v, SNetIntListGet( l, j+1));
            break;
          case tag:
            SNetVariantAddTag( v, SNetIntListGet( l, j+1));
            break;
          case btag:
            SNetVariantAddBTag( v, SNetIntListGet( l, j+1));
            break;
          default:
            assert(0);
        }
      }
      SNetVariantListAppendEnd( vlist, v);
    }

    barg = (box_arg_t *) SNetMemAlloc( sizeof( box_arg_t));
    barg->input  = input;
    barg->output = output;
    barg->boxfun = boxfun;
    barg->output_variants = output_variants;
    barg->vars = vlist;
    barg->boxname = boxname;
    barg->info = SNetInfoCopy(info);

    SNetThreadingSpawn(
        SNetEntityCreate( ENTITY_box, location, SNetLocvecGet(info),
          barg->boxname, InitBoxTask, (void*)barg)
        );


  } else {
    SNetIntListListDestroy(output_variants);
    output = input;
  }

  return output;
}
