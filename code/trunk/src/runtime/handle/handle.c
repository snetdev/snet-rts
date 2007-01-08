/*
 * This implements the handle.
 */

#include <stdlib.h>
#include <stdarg.h>

#include <memfun.h>
#include <snetentities.h>
#include <handle.h>

#include <bool.h>


#include <stdio.h>
#define VAL_OR_UNSET( PREFIX, BOOL, NAME, VAL, UNSET)\
  if( BOOL) {\
    HANDLE->PREFIX->NAME = VAL;\
  } else {\
    HANDLE->PREFIX->NAME = UNSET;\
  }

#define VAL_OR_NULL( PREFIX, BOOL, NAME, VAL)\
  VAL_OR_UNSET( PREFIX, BOOL, NAME, VAL, NULL)

#define VAL_OR_ZERO( PREFIX, BOOL, NAME, VAL)\
  VAL_OR_UNSET( PREFIX, BOOL, NAME, VAL, 0)  

#define FILL_HANDLE( PREFIX, INBUF, OUTBUFA, OUTBUFB, REC, FUNA, FUNB, TYPE, PATTERNS, TAGA, TAGB, FILTER)\
  VAL_OR_NULL( PREFIX, INBUF, inbuf, va_arg( args, buffer_t*))\
  VAL_OR_NULL( PREFIX, OUTBUFA, outbuf_a, va_arg( args, buffer_t*))\
  VAL_OR_NULL( PREFIX, OUTBUFB, outbuf_b, va_arg( args, buffer_t*))\
  VAL_OR_NULL( PREFIX, REC, rec, va_arg( args, record_t*))\
  VAL_OR_NULL( PREFIX, FUNA, boxfun_a, va_arg( args, void*))\
  VAL_OR_NULL( PREFIX, FUNB, boxfun_b, va_arg( args, void*))\
  VAL_OR_NULL( PREFIX, TYPE, type, va_arg( args, typeencoding_t*))\
  VAL_OR_NULL( PREFIX, PATTERNS, patterns, va_arg( args, patternset_t*))\
  VAL_OR_ZERO( PREFIX, TAGA, tag_a, va_arg( args, int))\
  VAL_OR_ZERO( PREFIX, TAGB, tag_b, va_arg( args, int))\
  VAL_OR_NULL( PREFIX, FILTER, filter_instructions, va_arg( args, int*))







typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_record_t *rec;
  void (*boxfun_a)( snet_handle_t*);
  snet_typeencoding_t *type;
} box_handle_t;

typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_buffer_t *outbuf_b;
  snet_typeencoding_t *type;
} parallel_handle_t;

typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  void (*boxfun_a)( snet_handle_t*);
  void (*boxfun_b)( snet_handle_t*);
  snet_typeencoding_t *type;
  bool is_incarnate;
} star_handle_t;

typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_typeencoding_t *patterns;
  snet_typeencoding_t *type;
} sync_handle_t;

typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  void (*boxfun_a)( snet_handle_t*);
  int tag_a;
  int tag_b;
} split_handle_t;

typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_typeencoding_t *in_type;
  snet_typeencoding_t *out_type;
  snet_filter_instruction_set_t **instr_set;
} filter_handle_t;


typedef union {
  box_handle_t *box_hnd;
  parallel_handle_t *parallel_hnd;
  star_handle_t *star_hnd;
  sync_handle_t *sync_hnd;
  split_handle_t *split_hnd;
  filter_handle_t *filter_hnd;
} hnd_collection_t;

struct handle {
  snet_handledescriptor_t descr;
  hnd_collection_t *hnd;
};

/*
struct handle {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_buffer_t *outbuf_b;
  snet_record_t *rec;
  void (*boxfun_a)( snet_handle_t*);
  void (*boxfun_b)( snet_handle_t*);
  snet_typeencoding_t *type;
  snet_patternset_t *patterns;
  int tag_a;
  int tag_b;
  int *filter_instructions;
  snet_handledescriptor_t descr;
};
*/

/* ------------------------------------------------------------------------- */
  
static void WrongHandleType() {
  printf("\n\n ** Fatal Error ** : Wrong handle was passed.\n\n");
  exit( 1);
}

/* ------------------------------------------------------------------------- */

#define HANDLE hnd->hnd
extern snet_handle_t *SNetHndCreate( snet_handledescriptor_t desc, ...) {

  snet_handle_t *hnd;
  va_list args;


  hnd = SNetMemAlloc( sizeof( snet_handle_t));
  hnd->descr = desc;
  hnd->hnd = SNetMemAlloc( sizeof( hnd_collection_t*));

  va_start( args, desc);

  switch( desc) {

    //FILL_HANDLE( PREFIX, INBUF, OUTBUFA, OUTBUFB, REC, FUNA, FUNB, TYPE, PATTERNS, TAGA, TAGB, FILTER)
    case HND_box: {
            HANDLE->box_hnd = SNetMemAlloc( sizeof( box_handle_t));
            HANDLE->box_hnd->inbuf = va_arg( args, snet_buffer_t*);
            HANDLE->box_hnd->outbuf_a = va_arg( args, snet_buffer_t*);
            HANDLE->box_hnd->rec = va_arg( args, snet_record_t*);
            HANDLE->box_hnd->boxfun_a = va_arg( args, void*);
            HANDLE->box_hnd->type = va_arg( args, snet_typeencoding_t*);
//            FILL_HANDLE( box_hnd, true, true, false, true, true, 
//                         false, true, false, false, false, false);
            break;
    }
    case HND_parallel: {

            HANDLE->parallel_hnd = SNetMemAlloc( sizeof( parallel_handle_t));
            HANDLE->parallel_hnd->inbuf = va_arg( args, snet_buffer_t*);
            HANDLE->parallel_hnd->outbuf_a = va_arg( args, snet_buffer_t*);
            HANDLE->parallel_hnd->outbuf_b = va_arg( args, snet_buffer_t*);
            HANDLE->parallel_hnd->type = va_arg( args, snet_typeencoding_t*);
//            FILL_HANDLE( parallel_hnd, true, true, true, false, false, 
//                         false, true, false, false, false, false);

            break;
    }
    
    case HND_star: {

            HANDLE->star_hnd = SNetMemAlloc( sizeof( star_handle_t));
            HANDLE->star_hnd->inbuf = va_arg( args, snet_buffer_t*);
            HANDLE->star_hnd->outbuf_a = va_arg( args, snet_buffer_t*);
            HANDLE->star_hnd->boxfun_a = va_arg( args, void*);
            HANDLE->star_hnd->boxfun_b = va_arg( args, void*);
            HANDLE->star_hnd->type = va_arg( args, snet_typeencoding_t*);
            HANDLE->star_hnd->is_incarnate = va_arg( args, bool);

//            FILL_HANDLE( star_hnd, true, true, false, false, true,
//                         true, true, false, false, false, false);
            break;
    }

    case HND_sync: {

            HANDLE->sync_hnd = SNetMemAlloc( sizeof( sync_handle_t));
            HANDLE->sync_hnd->inbuf = va_arg( args, snet_buffer_t*);
            HANDLE->sync_hnd->outbuf_a = va_arg( args, snet_buffer_t*);
            HANDLE->sync_hnd->type = va_arg( args, snet_typeencoding_t*);
            HANDLE->sync_hnd->patterns = va_arg( args, snet_typeencoding_t*);
//            HANDLE->sync_hnd->tag_a = va_arg( args, int); 
//            HANDLE->sync_hnd->tag_b = va_arg( args, int);             

//             FILL_HANDLE( sync_hnd, true, true, false, false, false,
//                          false, true, true, true, true, false);
             break;
    }
    
    case HND_split: {

            HANDLE->split_hnd = SNetMemAlloc( sizeof( split_handle_t));
            HANDLE->split_hnd->inbuf = va_arg( args, snet_buffer_t*);
            HANDLE->split_hnd->outbuf_a = va_arg( args, snet_buffer_t*);
            HANDLE->split_hnd->boxfun_a = va_arg( args, void*);
            HANDLE->split_hnd->tag_a = va_arg( args, int); 
            HANDLE->split_hnd->tag_b = va_arg( args, int);   

//            FILL_HANDLE( split_hnd, true, true, false, false, true, 
//                         false, false, false, true, true, false);
            break;
    }
 
    case HND_filter: {

            HANDLE->filter_hnd = SNetMemAlloc( sizeof( filter_handle_t));
            HANDLE->filter_hnd->inbuf = va_arg( args, snet_buffer_t*);
            HANDLE->filter_hnd->outbuf_a = va_arg( args, snet_buffer_t*);
            HANDLE->filter_hnd->in_type = va_arg( args, snet_typeencoding_t*);
            HANDLE->filter_hnd->out_type = va_arg( args, snet_typeencoding_t*);
            HANDLE->filter_hnd->instr_set = va_arg( args, snet_filter_instruction_set_t**);

//            FILL_HANDLE( filter_hnd, true, true, false, false, false,
//                         false, false, false, false, false, true);
            break;
    }
    default: {
            // this should never be reached.
               exit( 1);
    }
    
  }

  va_end( args);

  return( hnd);
}


extern void SNetHndDestroy( snet_handle_t *hnd) {

  switch( hnd->descr) {
    case HND_box: SNetMemFree( HANDLE->box_hnd); break;
    case HND_parallel: SNetMemFree( HANDLE->parallel_hnd); break;
    case HND_sync: SNetMemFree( HANDLE->sync_hnd); break;
    case HND_split: SNetMemFree( HANDLE->split_hnd); break;
    case HND_filter: SNetMemFree( HANDLE->filter_hnd); break;
    case HND_star: SNetMemFree( HANDLE->star_hnd); break;
  }

  SNetMemFree( hnd->hnd);
  SNetMemFree( hnd);
}


extern snet_record_t *SNetHndGetRecord( snet_handle_t *hndl) {
 
  snet_record_t *rec;
  switch( hndl->descr) {
    case HND_box: 
      rec = hndl->hnd->box_hnd->rec; 
      break;
    default: WrongHandleType();
  }

  return( rec);
}

extern void SNetHndSetRecord( snet_handle_t *hndl, snet_record_t *rec) {
 switch( hndl->descr) {
    case HND_box: 
      hndl->hnd->box_hnd->rec = rec; 
      break;
    default: WrongHandleType();
 }
}


extern void SNetHndSetInbuffer( snet_handle_t *hndl, snet_buffer_t *inbuf) {
  switch( hndl->descr) {
    case HND_box: 
      hndl->hnd->box_hnd->inbuf = inbuf; 
      break;
    case HND_parallel: 
      hndl->hnd->parallel_hnd->inbuf = inbuf; 
      break;
    case HND_star: 
      hndl->hnd->star_hnd->inbuf = inbuf; 
      break;
    case HND_split: 
      hndl->hnd->split_hnd->inbuf = inbuf; 
      break;
    case HND_sync: 
      hndl->hnd->sync_hnd->inbuf = inbuf; 
      break;
    case HND_filter: 
      hndl->hnd->filter_hnd->inbuf = inbuf; 
      break;
    default: WrongHandleType();
  }
}

extern snet_buffer_t *SNetHndGetInbuffer( snet_handle_t *hndl) {
  
  snet_buffer_t *inbuf;

  switch( hndl->descr) {
    case HND_box: 
      inbuf = hndl->hnd->box_hnd->inbuf; 
      break;
    case HND_parallel: 
      inbuf = hndl->hnd->parallel_hnd->inbuf; 
      break;
    case HND_star: 
      inbuf = hndl->hnd->star_hnd->inbuf; 
      break;
    case HND_split: 
      inbuf = hndl->hnd->split_hnd->inbuf; 
      break;
    case HND_sync: 
      inbuf = hndl->hnd->sync_hnd->inbuf; 
      break;
    case HND_filter: 
      inbuf = hndl->hnd->filter_hnd->inbuf; 
      break;
    default: WrongHandleType();
  }

  return( inbuf);
}


extern snet_buffer_t *SNetHndGetOutbuffer( snet_handle_t *hndl){

  snet_buffer_t *buf;

  switch( hndl->descr) {
    case HND_box: 
      buf = hndl->hnd->box_hnd->outbuf_a; 
      break;
    case HND_star: 
      buf = hndl->hnd->star_hnd->outbuf_a; 
      break;
    case HND_split: 
      buf = hndl->hnd->split_hnd->outbuf_a; 
      break;
    case HND_sync: 
      buf = hndl->hnd->sync_hnd->outbuf_a; 
      break;
    case HND_filter: 
      buf = hndl->hnd->filter_hnd->outbuf_a; 
      break;
    default: WrongHandleType();
  }

  return( buf);
}

extern snet_buffer_t *SNetHndGetOutbufferA( snet_handle_t *hndl){

  snet_buffer_t *buf;
 
  switch( hndl->descr) {
    case HND_parallel: 
      buf = hndl->hnd->parallel_hnd->outbuf_a; 
      break;
    default: WrongHandleType();
  }

  return( buf);
}

extern snet_buffer_t *SNetHndGetOutbufferB( snet_handle_t *hndl){

  snet_buffer_t *buf;
 
  switch( hndl->descr) {
    case HND_parallel: 
      buf = hndl->hnd->parallel_hnd->outbuf_b; 
      break;
    default: WrongHandleType();
  }

  return( buf);
}


extern void *SNetHndGetBoxfun( snet_handle_t *hndl){

  void *fun;

  switch( hndl->descr) {
    case HND_box: 
      fun =  hndl->hnd->box_hnd->boxfun_a; 
      break;
    case HND_split: 
      fun =  hndl->hnd->split_hnd->boxfun_a; 
      break;
    default: WrongHandleType();
  }
 
  return( fun);
}

extern void *SNetHndGetBoxfunA( snet_handle_t *hndl){

  void *fun;

  switch( hndl->descr) {
    case HND_star: 
      fun =  hndl->hnd->star_hnd->boxfun_a; 
      break;
    default: WrongHandleType();
  }
 
  return( fun);
}

extern void *SNetHndGetBoxfunB( snet_handle_t *hndl){

  void *fun;

  switch( hndl->descr) {
    case HND_star: 
      fun =  hndl->hnd->star_hnd->boxfun_b; 
      break;
    default: WrongHandleType();
  }
 
  return( fun);
}

extern int SNetHndGetTagA( snet_handle_t *hndl) {
 
  int tag;

  switch( hndl->descr) {
    case HND_split: 
      tag = hndl->hnd->split_hnd->tag_a; 
      break;
    default: WrongHandleType();
  }
 
  return( tag);
}

extern int SNetHndGetTagB( snet_handle_t *hndl) {

  int tag;

  switch( hndl->descr) {
    case HND_split: 
      tag = hndl->hnd->split_hnd->tag_b; 
      break;
    default: WrongHandleType();
  }
 
  return( tag);
}


extern snet_filter_instruction_set_t **SNetHndGetFilterInstructions( snet_handle_t *hndl) {

  snet_filter_instruction_set_t **instr;

  switch( hndl->descr) {
    case HND_filter: 
      instr = hndl->hnd->filter_hnd->instr_set; 
      break;
    default: WrongHandleType();
  }
 
  return( instr);
}

extern snet_typeencoding_t *SNetHndGetType( snet_handle_t *hndl) {
 
  snet_typeencoding_t *type;

  switch( hndl->descr) {
    case HND_box: 
      type = hndl->hnd->box_hnd->type; 
      break;
    case HND_parallel: 
      type = hndl->hnd->parallel_hnd->type; 
      break;
    case HND_star: 
      type = hndl->hnd->star_hnd->type; 
      break;
    case HND_sync: 
      type = hndl->hnd->sync_hnd->type; 
      break;
   default: WrongHandleType();
  }
 
  return( type);
}


extern snet_typeencoding_t *SNetHndGetInType( snet_handle_t *hnd) {
  
  snet_typeencoding_t *type;
  switch( hnd->descr) {
    case HND_filter: 
      type = HANDLE->filter_hnd->in_type; 
      break;
   default: WrongHandleType();
  }

  return( type);
}


extern snet_typeencoding_t *SNetHndGetOutType( snet_handle_t *hnd) {
  
  snet_typeencoding_t *type;
  switch( hnd->descr) {
    case HND_filter: 
      type = HANDLE->filter_hnd->out_type; 
      break;
   default: WrongHandleType();
  }

  return( type);
}


extern snet_typeencoding_t *SNetHndGetPatterns( snet_handle_t *hndl) {
  
  snet_typeencoding_t *patterns;

  switch( hndl->descr) {
    case HND_sync: 
      patterns = hndl->hnd->sync_hnd->patterns; 
      break;
    default: WrongHandleType();
  }

  return( patterns);
}


extern bool SNetHndIsIncarnate( snet_handle_t *hnd) {
  
  bool res;

  switch( hnd->descr) {
    case HND_star: 
      res = hnd->hnd->star_hnd->is_incarnate; 
      break;
    default: WrongHandleType();
  }

  return( res);
}
