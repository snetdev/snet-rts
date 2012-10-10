#include <assert.h>
#include <string.h>

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

#ifdef SNET_DEBUG_COUNTERS
#include "debugcounters.h"
#endif /* SNET_DEBUG_COUNTERS  */

#ifdef DBG_RT_TRACE_BOX_TIMINGS
#include <sys/time.h>
#endif



typedef struct {
  snet_stream_desc_t *instream, *outstream;
  snet_handle_t* (*boxfun)(snet_handle_t*);
  snet_handle_t* (*exerealm_create)(snet_handle_t*);
  snet_handle_t* (*exerealm_update)(snet_handle_t*);
  snet_handle_t* (*exerealm_destroy)(snet_handle_t*);
  snet_handle_t hnd;
} box_arg_t;



/* ------------------------------------------------------------------------- */
/*  SNetBox                                                                  */
/* ------------------------------------------------------------------------- */


static void BoxTask(void *arg)
{
#if defined(DBG_RT_TRACE_BOX_TIMINGS) || defined(SNET_DEBUG_COUNTERS)
  snet_time_t time_in;
  snet_time_t time_out;
  long mseconds;
#endif

  box_arg_t *barg = arg;
  snet_record_t *rec;

  /* read from input stream */
  rec = SNetStreamRead( barg->instream);
  switch (SNetRecGetDescriptor(rec)) {
    case REC_trigger_initialiser:
    case REC_data:
      barg->hnd.rec = rec;
#if defined(DBG_RT_TRACE_BOX_TIMINGS) || defined(SNET_DEBUG_COUNTERS)
      SNetDebugTimeGetTime(&time_in);
      #ifdef DBG_RT_TRACE_BOX_TIMINGS
        SNetUtilDebugNoticeTask(
            "[BOX] Firing box function at %lf.",
            SNetDebugTimeGetMilliseconds(&time_in));
     #endif
#endif

#ifdef USE_USER_EVENT_LOGGING
      /* Emit a monitoring message of a record read to be processed by a box */
      if (SNetRecGetDescriptor(rec) == REC_data) {
        SNetThreadingEventSignal(
            SNetMonInfoCreate( EV_MESSAGE_IN, MON_RECORD, rec));
      }
#endif

      /* execute box function and update execution realm */
      barg->hnd = *barg->boxfun(&barg->hnd);
      barg->hnd = *barg->exerealm_update(&barg->hnd);

      /*
       * Emit an event here?
       * SNetMonInfoEvent( EV_BOX_???, MON_RECORD, rec);
       */

#if defined(DBG_RT_TRACE_BOX_TIMINGS) || defined(SNET_DEBUG_COUNTERS)
      SNetDebugTimeGetTime(&time_out);
      mseconds = SNetDebugTimeDifferenceInMilliseconds(time_in, time_out);

      #if defined(DBG_RT_TRACE_BOX_TIMINGS)
        SNetUtilDebugNoticeTask(
            "[BOX] Return from box function after %lf sec.", mseconds);
      #elif defined(SNET_DEBUG_COUNTERS)
        SNetDebugCountersIncreaseCounter(mseconds, SNET_COUNTER_TIME_BOX);
      #endif
#endif

      SNetRecDestroy( rec);
      break;

    case REC_sync:
        SNetStreamReplace( barg->instream, SNetRecGetStream(rec));
        SNetRecDestroy( rec);
      break;

    case REC_sort_end:
      /* forward the sort record */
      SNetStreamWrite( barg->outstream, rec);
      break;

    case REC_terminate:
      barg->hnd = *barg->exerealm_destroy(&barg->hnd);
      SNetStreamWrite( barg->outstream, rec);
      SNetStreamClose( barg->instream, true);
      SNetStreamClose( barg->outstream, false);

      /* destroy box arg */
      SNetVariantListDestroy(barg->hnd.vars);
      SNetIntListListDestroy(barg->hnd.sign);
      SNetMemFree( barg);
      return;
    case REC_collect:
    default:
      assert(0);
      /* if ignore, destroy at least ...*/
      SNetRecDestroy( rec);
  }

  /* schedule new task */
  SNetThreadingRespawn(NULL);
}

/**
 * Box creation function
 */
snet_stream_t *SNetBox(snet_stream_t *input,
                        snet_info_t *info,
                        int location,
                        const char *boxname,
                        snet_box_fun_t boxfun,
                        snet_exerealm_create_fun_t er_create,
                        snet_exerealm_update_fun_t er_update,
                        snet_exerealm_destroy_fun_t er_destroy,
                        snet_int_list_list_t *output_variants)
{
  snet_stream_t *output;
  box_arg_t *barg;
  snet_variant_list_t *vlist;

  input = SNetRouteUpdate(info, input, location);

  if (SNetDistribIsNodeLocation(location)) {
    snet_int_list_t *list;
    vlist = SNetVariantListCreate(0);

    LIST_FOR_EACH(output_variants, list) {
      snet_variant_t *v = SNetVariantCreateEmpty();

      for (int i = 0; i < SNetIntListLength(list); i += 2) {
        switch (SNetIntListGet(list, i)) {
          case field:
            SNetVariantAddField(v, SNetIntListGet(list, i + 1));
            break;
          case tag:
            SNetVariantAddTag(v, SNetIntListGet(list, i + 1));
            break;
          case btag:
            SNetVariantAddBTag(v, SNetIntListGet(list, i + 1));
            break;
          default:
            assert(0);
        }
      }

      SNetVariantListAppendEnd(vlist, v);
    }

    output = SNetStreamCreate(0);

    barg = (box_arg_t *) SNetMemAlloc( sizeof( box_arg_t));
    barg->instream = SNetStreamOpen(input, 'r');
    barg->outstream = SNetStreamOpen(output, 'w');
    barg->boxfun = boxfun;
    barg->exerealm_create = er_create;
    barg->exerealm_update = er_update;
    barg->exerealm_destroy = er_destroy;
    /* set out signs */
    barg->hnd.sign = output_variants;
    /* set out descriptor */
    barg->hnd.out_sd = barg->outstream;
    /* mapping */
    barg->hnd.mapping = NULL;
    /* set variants */
    barg->hnd.vars = vlist;
    barg->hnd = *barg->exerealm_create(&barg->hnd);

    SNetThreadingSpawn( ENTITY_box, location, SNetLocvecGet(info),
          boxname, &BoxTask, barg);


  } else {
    SNetIntListListDestroy(output_variants);
    output = input;
  }

  return output;
}
