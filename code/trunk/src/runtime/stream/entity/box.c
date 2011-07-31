#include <assert.h>

#include "snetentities.h"
#include "debugtime.h"
#include "debug.h"
#include "memfun.h"
#include "locvec.h"

#include "threading.h"

//extern void SNetThreadingEventBoxStart(void);
//extern void SNetThreadingEventBoxStop(void);



#include "handle_p.h"
#include "list.h"
#include "distribution.h"

#ifdef SNET_DEBUG_COUNTERS
#include "debugcounters.h"
#endif /* SNET_DEBUG_COUNTERS  */


//#define DBG_RT_TRACE_OUT_TIMINGS



typedef struct {
  snet_stream_t *input, *output;
  void (*boxfun)( snet_handle_t*);
  snet_variant_list_t *output_variants;
  snet_locvec_t *myloc;
  const char *boxname;
} box_arg_t;



/* ------------------------------------------------------------------------- */
/*  SNetBox                                                                  */
/* ------------------------------------------------------------------------- */

static void BoxTask(void *arg)
{
#ifdef DBG_RT_TRACE_BOX_TIMINGS
  static struct timeval tv_in;
  static struct timeval tv_out;
#endif

#ifdef SNET_DEBUG_COUNTERS
  snet_time_t time_in;
  snet_time_t time_out;
  long mseconds;
#endif /* SNET_DEBUG_COUNTERS */

  box_arg_t *barg = (box_arg_t *)arg;
  snet_record_t *rec;
  snet_stream_desc_t *instream, *outstream;
  bool terminate = false;

  /* storage for the handle is within the box task */
  snet_handle_t hnd;

  instream = SNetStreamOpen(barg->input, 'r');
  outstream =  SNetStreamOpen(barg->output, 'w');

  /* set out descriptor */
  hnd.out_sd = outstream;
  /* set out signs */
  hnd.sign = barg->output_variants;
  /* mapping */
  hnd.mapping = NULL;
  /* location of box */
  hnd.boxloc = barg->myloc;

  /* MAIN LOOP */
  while( !terminate) {
    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch( SNetRecGetDescriptor(rec)) {
      case REC_trigger_initialiser:
      case REC_data:
        hnd.rec = rec;

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday( &tv_in, NULL);
        SNetUtilDebugNoticeLoc( barg->myloc,
            "[BOX] Firing box function at %lf.",
            tv_in.tv_sec + tv_in.tv_usec / 1000000.0
            );
#endif
#ifdef SNET_DEBUG_COUNTERS
        SNetDebugTimeGetTime(&time_in);
#endif /* SNET_DEBUG_COUNTERS */

        /* Emit a monitoring message of a record written by a box */
        if (SNetRecGetDescriptor(rec) == REC_data) SNetMonInfoEvent( EV_BOX_START, MON_RECORD, rec);

        SNetThreadingEventSignal(EVT_BOXFIRE);
        (*barg->boxfun)( &hnd);
        SNetThreadingEventSignal(EVT_BOXRET);

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday( &tv_out, NULL);
        SNetUtilDebugNoticeLoc( barg->myloc,
            "[BOX] Return from box function after %lf sec.",
            (tv_out.tv_sec - tv_in.tv_sec) + (tv_out.tv_usec - tv_in.tv_usec) / 1000000.0
            );
#endif


#ifdef SNET_DEBUG_COUNTERS
        SNetDebugTimeGetTime(&time_out);
        mseconds = SNetDebugTimeDifferenceInMilliseconds(&time_in, &time_out);
        SNetDebugCountersIncreaseCounter(mseconds, SNET_COUNTER_TIME_BOX);
#endif /* SNET_DEBUG_COUNTERS */

        SNetRecDestroy( rec);

        /* restrict to one data record per execution */
        SNetEntityYield();
        break;

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream(rec);
          SNetStreamReplace( instream, newstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_sort_end:
        /* forward the sort record */
        SNetStreamWrite( outstream, rec);
        break;

      case REC_terminate:
        SNetStreamWrite( outstream, rec);
        terminate = true;
        break;

      case REC_collect:
      default:
        assert(0);
        /* if ignore, destroy at least ...*/
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  SNetStreamClose( instream, true);
  SNetStreamClose( outstream, false);

  SNetHndDestroy( &hnd);

  /* destroy box arg */
  SNetLocvecDestroy(barg->myloc);
  SNetVariantListDestroy(barg->output_variants);
  SNetMemFree( barg);
}



/**
 * Box creation function
 */
snet_stream_t *SNetBox( snet_stream_t *input,
                        snet_info_t *info,
                        int location,
                        const char *boxname,
                        snet_box_fun_t boxfun,
                        snet_variant_list_t *output_variants)
{
  snet_stream_t *output;
  box_arg_t *barg;
  snet_locvec_t *locvec;

  input = SNetRouteUpdate(info, input, location, NULL);

  locvec = SNetLocvecGet(info);

  if(SNetDistribIsNodeLocation(location)) {
    output = SNetStreamCreate(0);

    barg = (box_arg_t *) SNetMemAlloc( sizeof( box_arg_t));
    barg->input  = input;
    barg->output = output;
    barg->boxfun = boxfun;
    barg->output_variants = output_variants;
    barg->myloc = SNetLocvecCopy(locvec);
    barg->boxname = boxname;

    SNetEntitySpawn( ENTITY_box, locvec, location,
      barg->boxname, BoxTask, (void*)barg);


  } else {
    SNetVariantListDestroy(output_variants);
    output = input;
  }

  return output;
}
