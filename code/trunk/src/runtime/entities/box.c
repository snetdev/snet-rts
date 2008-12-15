#include "box.h"
#include "handle.h"
#include "record.h"
#include "time.h"
#include "debug.h"
#include "stream_layer.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif

//#define BOX_DEBUG
/* ------------------------------------------------------------------------- */
/*  SNetBox                                                                  */
/* ------------------------------------------------------------------------- */

static void *BoxThread( void *hndl) {

#ifdef DBG_RT_TRACE_BOX_TIMINGS
  struct timeval tv_in;
  struct timeval tv_out;
#endif 

  snet_handle_t *hnd;
  snet_record_t *rec;
  void (*boxfun)( snet_handle_t*);
  bool terminate = false;
  hnd = (snet_handle_t*) hndl;
#ifdef BOX_DEBUG
  SNetUtilDebugNotice("(CREATION BOX)");
#endif
  boxfun = SNetHndGetBoxfun( hnd);


  while( !( terminate)) {
    rec = SNetTlRead(SNetHndGetInput(hnd));

    switch( SNetRecGetDescriptor(rec)) {
      case REC_data:
        SNetHndSetRecord( hnd, rec);

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday( &tv_in, NULL);
        SNetUtilDebugNotice("[DBG::RT::TimeTrace SnetBox Calls %p at %lf\n",
                        boxfun, tv_in.tv_sec + tv_in.tv_usec / 1000000.0);
#endif
        (*boxfun)( hnd);

#ifdef DBG_RT_TRACE_BOX_TIMINGS
        gettimeofday( &tv_out, NULL);
        SNetUtilDebugNotice("[DBG::RT::TimeTrace] SnetBox resumes from %p after"
                    "$lf\n\n", boxfun, (tv_out.tv_sec - tv_in.tv_sec) +
                        (tv_out.tv_usec - tv_in.tv_usec) / 1000000.0);
#endif
        SNetRecDestroy( rec);
        break;
      case REC_sync:
        SNetHndSetInput( hnd, SNetRecGetStream( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        SNetUtilDebugNotice("Unhandled control record, destroying it");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetTlWrite( SNetHndGetOutput( hnd), rec);
        break;
      case REC_sort_end:
        SNetTlWrite( SNetHndGetOutput( hnd), rec);
        break;
      case REC_terminate:
        terminate = true;
        SNetTlWrite( SNetHndGetOutput( hnd), rec);
        SNetTlMarkObsolete(SNetHndGetOutput(hnd));
        SNetHndDestroy( hnd);
        break;
      case REC_probe:
        SNetTlWrite(SNetHndGetOutput(hnd), rec);
      break;
#ifdef DISTRIBUTED_SNET
      case REC_route_update:
	break;
      case REC_route_redirect:
	break;
#endif /* DISTRIBUTED_SNET */
    }
  }
   return( NULL);
 
}

extern snet_tl_stream_t *SNetBox( snet_tl_stream_t *input,
#ifdef DISTRIBUTED_SNET
				  snet_info_t *info, 
				  int location,
#endif /* DISTRIBUTED_SNET */ 
				  snet_box_fun_t boxfun,
				  snet_box_sign_t *out_signs) 
{
  snet_tl_stream_t *output;
  snet_handle_t *hndl;

#ifdef DISTRIBUTED_SNET
  DISTRIBUTED_STARTUP_ENTRY(info, location, input);
#endif /* DISTRIBUTED_SNET */

  output = SNetTlCreateStream(BUFFER_SIZE);
    
#ifdef BOX_DEBUG
  SNetUtilDebugNotice("-");
  SNetUtilDebugNotice("| BOX");
  SNetUtilDebugNotice("| input: %x", input);
  SNetUtilDebugNotice("| output: %x", output);
  SNetUtilDebugNotice("-");
#endif
  
  hndl = SNetHndCreate( HND_box, input, output, NULL, boxfun, out_signs);
    
  SNetTlCreateComponent((void*)BoxThread, (void*)hndl, ENTITY_box);
  
#ifdef DISTRIBUTED_SNET
  DISTRIBUTED_STARTUP_EXIT(input, output);
#endif /* DISTRIBUTED_SNET */

  return( output);
}
