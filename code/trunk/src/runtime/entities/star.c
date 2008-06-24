#include "star.h"

#include "record.h"
#include "typeencode.h"
#include "threading.h"
#include "buffer.h"
#include "expression.h"
#include "handle.h"
#include "serial.h"
#include "debug.h"
#include "collectors.h"
/* ------------------------------------------------------------------------- */
/*  SNetStar                                                                 */
/* ------------------------------------------------------------------------- */

static bool 
MatchesExitPattern( snet_record_t *rec, 
                    snet_typeencoding_t *patterns, 
                    snet_expr_list_t *guards) 
{

  int i;
  bool is_match;
  snet_variantencoding_t *pat;

  for( i=0; i<SNetTencGetNumVariants( patterns); i++) {

    is_match = true;

    if( SNetEevaluateBool( SNetEgetExpr( guards, i), rec)) {
      pat = SNetTencGetVariant( patterns, i+1);
      is_match = SNetRecPatternMatches(pat, rec);
    }
    else {
      is_match = false;
    }

    if( is_match) {
      break;
    }
  }
  return( is_match);
}


static void *StarBoxThread( void *hndl)
{
  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t* (*box)( snet_buffer_t*);
  snet_buffer_t* (*self)( snet_buffer_t*);
  snet_buffer_t *real_outbuf, *our_outbuf, *starbuf=NULL;
  bool terminate = false;
  snet_typeencoding_t *exit_tags;
  snet_record_t *rec, *current_sort_rec = NULL;
  snet_expr_list_t *guards;

  real_outbuf = SNetHndGetOutbuffer( hnd);
  exit_tags = SNetHndGetType( hnd);
  box = SNetHndGetBoxfunA( hnd);
  self = SNetHndGetBoxfunB( hnd);
  guards = SNetHndGetGuardList( hnd);
        
  our_outbuf = SNetBufCreate( BUFFER_SIZE);

  while( !( terminate)) {
  
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        if( MatchesExitPattern( rec, exit_tags, guards)) {
          SNetBufPut( real_outbuf, rec);
        }
        else {
          if( starbuf == NULL) {
            // register new buffer with dispatcher,
            // starbuf is returned by self, which is SNetStarIncarnate
            starbuf = SNetSerial( our_outbuf, box, self);        
            SNetBufPut( real_outbuf, SNetRecCreate( REC_collect, starbuf));
/*            if( current_sort_rec != NULL) {
              fprintf( stderr, "put\n");
              SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_begin, 
                              SNetRecGetLevel( current_sort_rec), 
                              SNetRecGetNum( current_sort_rec)));
            }*/
         }
         SNetBufPut( our_outbuf, rec);
       }
       break;

      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
        SNetUtilDebugNotice("[STAR] unhandled control record,"
                            " destroying it\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_begin:
        SNetRecDestroy( current_sort_rec);
        current_sort_rec = SNetRecCreate( REC_sort_begin, SNetRecGetLevel( rec),
                                          SNetRecGetNum( rec));
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_begin,
                                    SNetRecGetLevel( rec),
                                    SNetRecGetNum( rec)));
        }
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_sort_end:
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_end,
                                    SNetRecGetLevel( rec),
                                    SNetRecGetNum( rec)));
          
        }
        SNetBufPut( SNetHndGetOutbuffer( hnd), rec);
        break;
      case REC_terminate:
        terminate = true;
        SNetHndDestroy( hnd);
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCopy( rec));
        }
        SNetBufPut( real_outbuf, rec);
        SNetBufBlockUntilEmpty( our_outbuf);
        SNetBufDestroy( our_outbuf);
        //real_outbuf will be destroyed by the dispatcher
        break;
      case REC_probe:
        if(starbuf != NULL) {
          SNetBufPut(our_outbuf, SNetRecCopy(rec));
        }
        SNetBufPut(our_outbuf, rec);
      break;
    }
  }

  return( NULL);
}

extern snet_buffer_t *SNetStar( snet_buffer_t *inbuf,
                                snet_typeencoding_t *type,
                                snet_expr_list_t *guards,
                                snet_buffer_t* (*box_a)(snet_buffer_t*),
                                snet_buffer_t* (*box_b)(snet_buffer_t*)) {

    
  snet_buffer_t *star_outbuf, *dispatch_outbuf;
  snet_handle_t *hnd;

  star_outbuf = SNetBufCreate( BUFFER_SIZE);
  
  hnd = SNetHndCreate( HND_star, inbuf, star_outbuf, box_a, box_b, type, guards, false);

  SNetThreadCreate( StarBoxThread, hnd, ENTITY_star_nondet);

  dispatch_outbuf = CreateCollector( star_outbuf);
  
  return( dispatch_outbuf);
}

extern
snet_buffer_t *SNetStarIncarnate( snet_buffer_t *inbuf,
                            snet_typeencoding_t *type,
                               snet_expr_list_t *guards,
                                  snet_buffer_t *(*box_a)(snet_buffer_t*),
                                  snet_buffer_t *(*box_b)(snet_buffer_t*)) {

  snet_buffer_t *outbuf;
  snet_handle_t *hnd;

  outbuf = SNetBufCreate( BUFFER_SIZE);


  hnd = SNetHndCreate( HND_star, inbuf, outbuf, box_a, box_b, type, guards, true);
  
  SNetThreadCreate( StarBoxThread, hnd, ENTITY_star_nondet);

  return( outbuf);
}





/* ------------------------------------------------------------------------- */
/*  SNetStarDet                                                              */
/* ------------------------------------------------------------------------- */

static void *DetStarBoxThread( void *hndl) {

  snet_handle_t *hnd = (snet_handle_t*)hndl;
  snet_buffer_t* (*box)( snet_buffer_t*);
  snet_buffer_t* (*self)( snet_buffer_t*);
  snet_buffer_t *real_outbuf, *our_outbuf, *starbuf=NULL;
  bool terminate = false;
  snet_typeencoding_t *exit_tags;
  snet_record_t *rec;
  snet_expr_list_t *guards;
 
  snet_record_t *sort_begin = NULL, *sort_end = NULL;
  int counter = 0;


  real_outbuf = SNetHndGetOutbuffer( hnd);
  exit_tags = SNetHndGetType( hnd);
  box = SNetHndGetBoxfunA( hnd);
  self = SNetHndGetBoxfunB( hnd);
  guards = SNetHndGetGuardList( hnd);
   
  our_outbuf = SNetBufCreate( BUFFER_SIZE);

  while( !( terminate)) {
  
    rec = SNetBufGet( SNetHndGetInbuffer( hnd));

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        if( !( SNetHndIsIncarnate( hnd))) { 
          SNetRecDestroy( sort_begin); SNetRecDestroy( sort_end);
          sort_begin = SNetRecCreate( REC_sort_begin, 0, counter);
          sort_end = SNetRecCreate( REC_sort_end, 0, counter);
        
          SNetBufPut( real_outbuf, SNetRecCopy( sort_begin));
          if( starbuf != NULL) {
            SNetBufPut( our_outbuf, SNetRecCopy( sort_begin));
          }
          
          if( MatchesExitPattern( rec, exit_tags, guards)) {
            SNetBufPut( real_outbuf, rec);
          }
          else {
            if( starbuf == NULL) {
              starbuf = SNetSerial( our_outbuf, box, self);        
              SNetBufPut( real_outbuf, SNetRecCreate( REC_collect, starbuf));
//              SNetBufPut( our_outbuf, sort_begin); 
            }
            SNetBufPut( our_outbuf, rec);
          }

          SNetBufPut( real_outbuf, SNetRecCopy( sort_end));
          if( starbuf != NULL) {
            SNetBufPut( our_outbuf, SNetRecCopy( sort_end));
          }
          counter += 1;
        }
        else { /* incarnation */
          if( MatchesExitPattern( rec, exit_tags, guards)) {
           SNetBufPut( real_outbuf, rec);
         }
         else {
           if( starbuf == NULL) {
             // register new buffer with dispatcher,
             // starbuf is returned by self, which is SNetStarIncarnate
             starbuf = SNetSerial( our_outbuf, box, self);        
             SNetBufPut( real_outbuf, SNetRecCreate( REC_collect, starbuf));
//             SNetBufPut( our_outbuf, sort_begin); /* sort_begin is set in "case REC_sort_xxx" */
          }
          SNetBufPut( our_outbuf, rec);
         }
        }
       break;

      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        SNetRecDestroy( rec);
        break;

      case REC_collect:
        SNetUtilDebugNotice("[STAR] Unhandled control record,"
                      " destroying it.\n\n");
        SNetRecDestroy( rec);
        break;
      case REC_sort_end:
      case REC_sort_begin:
        if( !( SNetHndIsIncarnate( hnd))) {
          SNetRecSetLevel( rec, SNetRecGetLevel( rec) + 1);
        }
        SNetBufPut( SNetHndGetOutbuffer( hnd), SNetRecCopy( rec));
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCopy( rec));
        }
        else {
          if( SNetRecGetDescriptor( rec) == REC_sort_begin) {
            SNetRecDestroy( sort_begin);
            sort_begin = SNetRecCopy( rec);
          }
          else {
            SNetRecDestroy( sort_end);
            sort_end = SNetRecCopy( rec);
          }
        }
        SNetRecDestroy( rec);
        break;
      case REC_terminate:

        terminate = true;
        
        SNetHndDestroy( hnd);
        if( starbuf != NULL) {
          SNetBufPut( our_outbuf, SNetRecCreate( REC_sort_begin, 0, counter));
          SNetBufPut( our_outbuf, SNetRecCopy( rec));
        }
        SNetBufPut( real_outbuf, SNetRecCreate( REC_sort_begin, 0, counter));
        SNetBufPut( real_outbuf, rec);
        SNetBufBlockUntilEmpty( our_outbuf);
        SNetBufDestroy( our_outbuf);
        //real_outbuf will be destroyed by the dispatcher
        break;
      case REC_probe:
        if(starbuf != NULL) {
          SNetBufPut(our_outbuf, SNetRecCopy(rec));
        }
        SNetBufPut(our_outbuf, rec);
      break;
    }
  }

  return( NULL);
}

extern snet_buffer_t *SNetStarDet( snet_buffer_t *inbuf,
                                   snet_typeencoding_t *type,
                                   snet_expr_list_t *guards,
                                   snet_buffer_t* (*box_a)(snet_buffer_t*),
                                   snet_buffer_t* (*box_b)(snet_buffer_t*)) {

    
  snet_buffer_t *star_outbuf, *dispatch_outbuf;
  snet_handle_t *hnd;

  star_outbuf = SNetBufCreate( BUFFER_SIZE);
  
  hnd = SNetHndCreate( HND_star, inbuf, star_outbuf, box_a, box_b, type, guards, false);

  SNetThreadCreate( DetStarBoxThread, hnd, ENTITY_star_det);

  dispatch_outbuf = CreateDetCollector( star_outbuf);
  
  return( dispatch_outbuf);
}

extern
snet_buffer_t *SNetStarDetIncarnate( snet_buffer_t *inbuf,
                               snet_typeencoding_t *type,  
                                  snet_expr_list_t *guards,
                                     snet_buffer_t *(*box_a)(snet_buffer_t*),
                                     snet_buffer_t *(*box_b)(snet_buffer_t*)) {
  snet_buffer_t *outbuf;
  snet_handle_t *hnd;

  outbuf = SNetBufCreate( BUFFER_SIZE);

  hnd = SNetHndCreate( HND_star, inbuf, outbuf, box_a, box_b, type, guards, true);
  
  SNetThreadCreate( DetStarBoxThread, hnd, ENTITY_star_det);

  return( outbuf);
}







