#include "box.h"
#include "handle.h"
#include "record.h"
#include "time.h"
#include "debug.h"
#include "buffer.h"
#include "threading.h"

#define BUF_CREATE( PTR, SIZE)\
            PTR = SNetBufCreate( SIZE);
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
  boxfun = SNetHndGetBoxfun( hnd);


  while( !( terminate)) {
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
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
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;
      case REC_collect:
        fprintf( stderr, "\nUnhandled control record, destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;        
      case REC_terminate: 
        terminate = true;
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        SNetBufBlockUntilEmpty( SNetHndGetOutbuffer( hnd));
        SNetBufDestroy( SNetHndGetOutbuffer( hnd));
        SNetHndDestroy( hnd);
        break;
    }
  }
   return( NULL);
 
}

extern snet_buffer_t *SNetBox( snet_buffer_t *inbuf, 
                                        void (*boxfun)( snet_handle_t*), 
                         snet_typeencoding_t *outspec) {

  snet_buffer_t *outbuf;
  snet_handle_t *hndl;
  

  BUF_CREATE( outbuf, BUFFER_SIZE);
  hndl = SNetHndCreate( HND_box, inbuf, outbuf, NULL, boxfun, outspec);

  SNetThreadCreate( (void*)BoxThread, (void*)hndl, ENTITY_box);
  
  return( outbuf);
}
