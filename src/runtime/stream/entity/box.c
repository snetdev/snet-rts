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
  snet_stream_t *input, *output;
  snet_handle_t* (*boxfun)(snet_handle_t*);
  snet_handle_t* (*exerealm_create)(snet_handle_t*);
  snet_handle_t* (*exerealm_update)(snet_handle_t*);
  snet_handle_t* (*exerealm_destroy)(snet_handle_t*);
  const char *boxname;
  snet_handle_t hnd;
} box_arg_t;



/* ------------------------------------------------------------------------- */
/*  SNetBox                                                                  */
/* ------------------------------------------------------------------------- */

static void BoxTask(snet_entity_t *ent, void *arg)
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

  instream = SNetStreamOpen(barg->input, 'r');
  outstream =  SNetStreamOpen(barg->output, 'w');
  /* set out descriptor */
  barg->hnd.out_sd = outstream;
  /* set entity */
  barg->hnd.ent = ent;

  /* MAIN LOOP */
  while(!terminate) {
    /* read from input stream */
    rec = SNetStreamRead(instream);

    switch(SNetRecGetDescriptor(rec)) {
      case REC_trigger_initialiser:
      case REC_data:
      	barg->hnd.rec = rec;

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday(&tv_in, NULL);
        SNetUtilDebugNoticeEnt(ent,
            "[BOX] Firing box function at %lf.",
            tv_in.tv_sec + tv_in.tv_usec / 1000000.0
            );
#endif
#ifdef SNET_DEBUG_COUNTERS
        SNetDebugTimeGetTime(&time_in);
#endif /* SNET_DEBUG_COUNTERS */

#ifdef USE_USER_EVENT_LOGGING
        /* Emit a monitoring message of a record read to be processed by a box */
        if (SNetRecGetDescriptor(rec) == REC_data) {
          SNetThreadingEventSignal(ent,
              SNetMonInfoCreate(EV_MESSAGE_IN, MON_RECORD, rec)
              );
        }
#endif

        /* execute box function and update execution realm */
        barg->hnd = *barg->boxfun(&barg->hnd);
        barg->hnd = *barg->exerealm_update(&barg->hnd);

        /*
         * Emit an event here?
         * SNetMonInfoEvent(EV_BOX_???, MON_RECORD, rec);
         */

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday(&tv_out, NULL);
        SNetUtilDebugNoticeEnt(ent,
            "[BOX] Return from box function after %lf sec.",
            (tv_out.tv_sec - tv_in.tv_sec) + (tv_out.tv_usec - tv_in.tv_usec) / 1000000.0
            );
#endif


#ifdef SNET_DEBUG_COUNTERS
        SNetDebugTimeGetTime(&time_out);
        mseconds = SNetDebugTimeDifferenceInMilliseconds(&time_in, &time_out);
        SNetDebugCountersIncreaseCounter(mseconds, SNET_COUNTER_TIME_BOX);
#endif /* SNET_DEBUG_COUNTERS */

        SNetRecDestroy(rec);

        /* restrict to one data record per execution */
        //SNetThreadingYield();

        /* check the box task should be migrated after one record execution */
        SNetThreadingCheckMigrate();
        break;

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream(rec);
          SNetStreamReplace(instream, newstream);
          SNetRecDestroy(rec);
        }
        break;

      case REC_sort_end:
        /* forward the sort record */
        SNetStreamWrite(outstream, rec);
        break;

      case REC_terminate:
      	barg->hnd = *barg->exerealm_destroy(&barg->hnd);
        SNetStreamWrite(outstream, rec);
        terminate = true;
        break;

      case REC_collect:
      default:
        assert(0);
    }
  } /* MAIN LOOP END */
  barg->hnd = *barg->exerealm_destroy(&barg->hnd);
  SNetStreamClose(instream, true);
  SNetStreamClose(outstream, false);
 
  /* destroy box arg */
  SNetVariantListDestroy(barg->hnd.vars);
  SNetIntListListDestroy(barg->hnd.sign);
  SNetMemFree( barg);
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
  int i,j;
  snet_stream_t *output;
  box_arg_t *barg;
  snet_variant_list_t *vlist;

  input = SNetRouteUpdate(info, input, location);

  if(SNetDistribIsNodeLocation(location)) {
    output = SNetStreamCreate(0);
    vlist = SNetVariantListCreate(0);
    for(i=0; i<SNetIntListListLength(output_variants); i++) {
      snet_int_list_t *l = SNetIntListListGet(output_variants, i);
      snet_variant_t *v = SNetVariantCreateEmpty();
      for(j=0; j<SNetIntListLength(l); j+=2) {
        switch(SNetIntListGet(l, j)) {
          case field:
            SNetVariantAddField(v, SNetIntListGet(l, j+1));
            break;
          case tag:
            SNetVariantAddTag(v, SNetIntListGet(l, j+1));
            break;
          case btag:
            SNetVariantAddBTag(v, SNetIntListGet(l, j+1));
            break;
          default:
            assert(0);
        }
      }
      SNetVariantListAppendEnd(vlist, v);
    }

    barg = (box_arg_t *) SNetMemAlloc(sizeof(box_arg_t));
    barg->input  = input;
    barg->output = output;
    barg->boxfun = boxfun;
    barg->exerealm_create = er_create;
    barg->exerealm_update = er_update;
    barg->exerealm_destroy = er_destroy;
    /* set out signs */
    barg->hnd.sign = output_variants;
    /* mapping */
    barg->hnd.mapping = NULL;
    /* set variants */
    barg->hnd.vars = vlist;
    barg->hnd = *barg->exerealm_create(&barg->hnd);

    SNetThreadingSpawn(
        SNetEntityCreate(ENTITY_box, location, SNetLocvecGet(info),
          boxname, BoxTask, (void*)barg)
        );


  } else {
    SNetIntListListDestroy(output_variants);
    output = input;
  }

  return output;
}
