#include "box.h"
#include "handle.h"
#include "record.h"
#include "time.h"
#include "debug.h"
#include "stream_layer.h"

#include "stream.h"
#include "task.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif

#ifdef SNET_DEBUG_COUNTERS
#include "debugtime.h"
#include "debugcounters.h"
#endif /* SNET_DEBUG_COUNTERS  */

//#define BOX_DEBUG

/* ------------------------------------------------------------------------- */
/*  SNetBox                                                                  */
/* ------------------------------------------------------------------------- */

/**
 * Box task
 */
static void BoxTask(task_t *self, void *arg)
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

  snet_handle_t *hnd;
  snet_record_t *rec;
  void (*boxfun)( snet_handle_t*);
  bool terminate = false;

  stream_t *instream, *outstream;

  hnd = (snet_handle_t*) arg;
#ifdef BOX_DEBUG
  SNetUtilDebugNotice("(CREATION BOX)");
#endif
  boxfun = SNetHndGetBoxfun( hnd);

  instream = SNetHndGetInput(hnd);
  StreamOpen(self, instream, 'r');

  outstream = SNetHndGetOutput(hnd);
  StreamOpen(self, outstream, 'w');

  /* set boxtask */
  SNetHndSetBoxtask( hnd, self);

  /* MAIN LOOP */
  while( !terminate) {
    /* read from input stream */
    rec = StreamRead(self, instream);

    switch( SNetRecGetDescriptor(rec)) {
      case REC_trigger_initialiser:
      case REC_data:
        SNetHndSetRecord( hnd, rec);

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday( &tv_in, NULL);
        SNetUtilDebugNotice("[DBG::RT::TimeTrace SnetBox Calls %p at %lf\n",
                        boxfun, tv_in.tv_sec + tv_in.tv_usec / 1000000.0);
#endif
#ifdef SNET_DEBUG_COUNTERS 
	SNetDebugTimeGetTime(&time_in);
#endif /* SNET_DEBUG_COUNTERS */

        (*boxfun)( hnd);

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday( &tv_out, NULL);
        SNetUtilDebugNotice("[DBG::RT::TimeTrace] SnetBox resumes from %p after "
                    "%lf\n\n", boxfun, (tv_out.tv_sec - tv_in.tv_sec) +
                        (tv_out.tv_usec - tv_in.tv_usec) / 1000000.0);
#endif
#ifdef SNET_DEBUG_COUNTERS
	SNetDebugTimeGetTime(&time_out);

	mseconds = SNetDebugTimeDifferenceInMilliseconds(&time_in, &time_out);

	SNetDebugCountersIncreaseCounter(mseconds, SNET_COUNTER_TIME_BOX);
#endif /* SNET_DEBUG_COUNTERS */
        SNetRecDestroy( rec);
        break;

      case REC_sync:
        {
          stream_t *newinstream = SNetRecGetStream(rec);
          SNetHndSetInput( hnd, newinstream);
          StreamReplace( self, &instream, newinstream);
          SNetRecDestroy( rec);
        }
        break;

      case REC_collect:
        SNetUtilDebugNotice("Unhandled control record, destroying it");
        SNetRecDestroy( rec);
        break;

      case REC_sort_end:
        /* forward the sort record */
        StreamWrite( self, outstream, rec);
        break;

      case REC_terminate:
        terminate = true;
        StreamWrite( self, outstream, rec);
        break;

    default:
      SNetUtilDebugNotice("[Box] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( rec));
      SNetRecDestroy( rec);
      break;
    }
  } /* MAIN LOOP END */

  StreamClose( self, instream);
  StreamClose( self, outstream);
  StreamDestroy(outstream);

  SNetHndDestroy( hnd);
}


/**
 * Box creation function
 */
stream_t *SNetBox( stream_t *input,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
    int location,
#endif /* DISTRIBUTED_SNET */ 
    const char *boxname,
    snet_box_fun_t boxfun,
    snet_box_sign_t *out_signs) 
{
  stream_t *output;
  snet_handle_t *hndl;

#ifdef DISTRIBUTED_SNET
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Box \"%s\" created", boxname);
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */

    output = StreamCreate();
#ifdef BOX_DEBUG
    SNetUtilDebugNotice("-");
    SNetUtilDebugNotice("| BOX %s [%p]", boxname, boxfun);
    SNetUtilDebugNotice("| input: %x", input);
    SNetUtilDebugNotice("| output: %x", output);
    SNetUtilDebugNotice("-");
#endif
    
    hndl = SNetHndCreate( HND_box, input, output, NULL, boxfun, out_signs);
    SNetEntitySpawn( BoxTask, (void*)hndl, ENTITY_box);
    
#ifdef DISTRIBUTED_SNET
  } else {
    SNetTencBoxSignDestroy(out_signs);
    output = input;
  }
#endif /* DISTRIBUTED_SNET */
  return( output);
}


