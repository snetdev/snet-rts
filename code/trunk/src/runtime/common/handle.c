/*
 * This implements the handle.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
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
  int num;
  char **string_names;
  int *int_names;
} name_mapping_t;

typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_record_t *rec;
  void (*boxfun_a)( snet_handle_t*);
  snet_typeencoding_t *type;
  name_mapping_t *mapping;
} box_handle_t;

typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t **buffers;
  snet_typeencoding_t *type;
  bool is_det;
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


#ifdef FILTER_VERSION_2
typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_typeencoding_t *in_type;
  snet_typeencoding_list_t *out_types;
  snet_filter_instruction_list_t *instr_list;
} filter_handle_t;
#else
typedef struct {
  snet_buffer_t *inbuf;
  snet_buffer_t *outbuf_a;
  snet_typeencoding_t *in_type;
  snet_typeencoding_t *out_type;
  snet_filter_instruction_set_t **instr_set;
} filter_handle_t;
#endif

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

#define HANDLE( type) hnd->hnd->type
#define BOX_HND( component) HANDLE( box_hnd)->component
#define BOX_HND_MAPPING( component) HANDLE( box_hnd)->mapping->component
#define PAR_HND( component) HANDLE( parallel_hnd)->component
#define STAR_HND( component) HANDLE( star_hnd)->component
#define SYNC_HND( component) HANDLE( sync_hnd)->component
#define SPLIT_HND( component) HANDLE( split_hnd)->component
#define FILTER_HND( component) HANDLE( filter_hnd)->component

extern snet_handle_t *SNetHndCreate( snet_handledescriptor_t desc, ...) {

  snet_handle_t *hnd;
  va_list args;


  hnd = SNetMemAlloc( sizeof( snet_handle_t));
  hnd->descr = desc;
  hnd->hnd = SNetMemAlloc( sizeof( hnd_collection_t*));

  va_start( args, desc);

  switch( desc) {

    case HND_box: {
            HANDLE( box_hnd) = SNetMemAlloc( sizeof( box_handle_t));
            BOX_HND( inbuf) = va_arg( args, snet_buffer_t*);
            BOX_HND( outbuf_a) = va_arg( args, snet_buffer_t*);
            BOX_HND( rec) = va_arg( args, snet_record_t*);
            BOX_HND( boxfun_a) = va_arg( args, void*);
            BOX_HND( type) = va_arg( args, snet_typeencoding_t*);
            BOX_HND( mapping) = NULL;
            break;
    }
    case HND_parallel: {

            HANDLE( parallel_hnd) = SNetMemAlloc( sizeof( parallel_handle_t));
            PAR_HND( inbuf) = va_arg( args, snet_buffer_t*);
            PAR_HND( buffers) = va_arg( args, snet_buffer_t**);
            PAR_HND( type) = va_arg( args, snet_typeencoding_t*);
            PAR_HND( is_det) = va_arg( args, bool);
            break;
    }
    
    case HND_star: {

            HANDLE( star_hnd) = SNetMemAlloc( sizeof( star_handle_t));
            STAR_HND( inbuf) = va_arg( args, snet_buffer_t*);
            STAR_HND( outbuf_a) = va_arg( args, snet_buffer_t*);
            STAR_HND( boxfun_a) = va_arg( args, void*);
            STAR_HND( boxfun_b) = va_arg( args, void*);
            STAR_HND( type) = va_arg( args, snet_typeencoding_t*);
            STAR_HND(is_incarnate) = va_arg( args, bool);

            break;
    }

    case HND_sync: {

            HANDLE( sync_hnd) = SNetMemAlloc( sizeof( sync_handle_t));
            SYNC_HND( inbuf) = va_arg( args, snet_buffer_t*);
            SYNC_HND( outbuf_a) = va_arg( args, snet_buffer_t*);
            SYNC_HND( type) = va_arg( args, snet_typeencoding_t*);
            SYNC_HND( patterns) = va_arg( args, snet_typeencoding_t*);

             break;
    }
    
    case HND_split: {

            HANDLE( split_hnd) = SNetMemAlloc( sizeof( split_handle_t));
            SPLIT_HND( inbuf) = va_arg( args, snet_buffer_t*);
            SPLIT_HND( outbuf_a) = va_arg( args, snet_buffer_t*);
            SPLIT_HND( boxfun_a) = va_arg( args, void*);
            SPLIT_HND( tag_a) = va_arg( args, int); 
            SPLIT_HND( tag_b) = va_arg( args, int);   

            break;
    }
  
    #ifdef FILTER_VERSION_2
    case HND_filter:
      HANDLE( filter_hnd) = SNetMemAlloc( sizeof( filter_handle_t));
      FILTER_HND( inbuf) = va_arg( args, snet_buffer_t*);
      FILTER_HND( outbuf_a) = va_arg( args, snet_buffer_t*);
      FILTER_HND( in_type) = va_arg( args, snet_typeencoding_t*);      
      
    #else
    case HND_filter: {

            HANDLE( filter_hnd) = SNetMemAlloc( sizeof( filter_handle_t));
            FILTER_HND( inbuf) = va_arg( args, snet_buffer_t*);
            FILTER_HND( outbuf_a) = va_arg( args, snet_buffer_t*);
            FILTER_HND( in_type) = va_arg( args, snet_typeencoding_t*);
            FILTER_HND( out_type) = va_arg( args, snet_typeencoding_t*);
            FILTER_HND( instr_set) = va_arg( args, snet_filter_instruction_set_t**);

            break;
    }
    #endif
    default: {
      printf("\n\n ** Fatal Error ** : Cannot create requested handle type."
             " This is a bug in the runtime system. \n\n");         
      exit( 1);
    }
    
  }

  va_end( args);

  return( hnd);
}

/****/
extern snet_filter_instruction_set_list_t *SNetCreateFilterInstructionList( int num, ...) {
  int i;
  va_list args;
  snet_filter_instruction_set_list_t *lst;

  lst = SNetMemAlloc( sizeof( snet_filter_instruction_set_list_t));
  lst->num = num;
  lst->lst = SNetMemAlloc( num * sizeof( snet_filter_instruction_set_t*));

  va_start( args, num);
  for( i=0; i<num; i++) {
    (lst->lst)[i] = va_arg( args, snet_filter_instruction_set_t*);
  }
  va_end( args);
  
  return( lst);
}


extern int SNetFilterInstructionsGetNumSets( snet_filter_instruction_set_list_t *lst) {
  return( lst->num);  
}

extern snet_filter_instruction_set_t **SNetFilterInstructionsGetSets( snet_filter_instruction_set_list_t *lst) {
  return( lst->lst);
}


/****/



extern void SNetHndDestroy( snet_handle_t *hnd) {

  int i;

  switch( hnd->descr) {
    case HND_box: 
      if( BOX_HND( mapping) != NULL) {
        for( i=0; i<BOX_HND_MAPPING( num); i++) {
          SNetMemFree( BOX_HND_MAPPING( string_names[i]));
        }
        SNetMemFree( BOX_HND_MAPPING( string_names));
        SNetMemFree( BOX_HND_MAPPING( int_names));
      }
      SNetMemFree( HANDLE( box_hnd)); 
      break;
    case HND_parallel: 
      SNetMemFree( HANDLE( parallel_hnd)); 
      break;
    case HND_sync: 
      SNetMemFree( HANDLE( sync_hnd)); 
      break;
    case HND_split: 
      SNetMemFree( HANDLE( split_hnd)); 
    break;
    case HND_filter: 
      SNetMemFree( HANDLE( filter_hnd)); 
    break;
    case HND_star: 
      SNetMemFree( HANDLE( star_hnd)); 
    break;
  }

  SNetMemFree( hnd->hnd);
  SNetMemFree( hnd);
}


extern snet_record_t *SNetHndGetRecord( snet_handle_t *hnd) {
 
  snet_record_t *rec;
  switch( hnd->descr) {
    case HND_box: 
      rec = BOX_HND( rec); 
      break;
    default: WrongHandleType();
  }

  return( rec);
}

extern void SNetHndSetRecord( snet_handle_t *hnd, snet_record_t *rec) {
 switch( hnd->descr) {
    case HND_box: 
      BOX_HND( rec) = rec; 
      break;
    default: WrongHandleType();
 }
}


extern void SNetHndSetInbuffer( snet_handle_t *hnd, snet_buffer_t *inbuf) {
  switch( hnd->descr) {
    case HND_box: 
      BOX_HND( inbuf) = inbuf; 
      break;
    case HND_parallel: 
      PAR_HND( inbuf) = inbuf; 
      break;
    case HND_star: 
      STAR_HND( inbuf) = inbuf; 
      break;
    case HND_split: 
      SPLIT_HND( inbuf) = inbuf; 
      break;
    case HND_sync: 
      SYNC_HND( inbuf) = inbuf; 
      break;
    case HND_filter: 
      FILTER_HND( inbuf) = inbuf; 
      break;
    default: WrongHandleType();
  }
}

extern snet_buffer_t *SNetHndGetInbuffer( snet_handle_t *hnd) {
  
  snet_buffer_t *inbuf;

  switch( hnd->descr) {
    case HND_box: 
      inbuf = BOX_HND( inbuf); 
      break;
    case HND_parallel: 
      inbuf = PAR_HND( inbuf); 
      break;
    case HND_star: 
      inbuf = STAR_HND( inbuf); 
      break;
    case HND_split: 
      inbuf = SPLIT_HND( inbuf); 
      break;
    case HND_sync: 
      inbuf = SYNC_HND( inbuf); 
      break;
    case HND_filter: 
      inbuf = FILTER_HND( inbuf); 
      break;
    default: WrongHandleType();
  }

  return( inbuf);
}


extern snet_buffer_t *SNetHndGetOutbuffer( snet_handle_t *hnd){

  snet_buffer_t *buf;

  switch( hnd->descr) {
    case HND_box: 
      buf = BOX_HND( outbuf_a); 
      break;
    case HND_star: 
      buf = STAR_HND( outbuf_a); 
      break;
    case HND_split: 
      buf = SPLIT_HND( outbuf_a); 
      break;
    case HND_sync: 
      buf = SYNC_HND( outbuf_a); 
      break;
    case HND_filter: 
      buf = FILTER_HND( outbuf_a); 
      break;
    default: WrongHandleType();
  }

  return( buf);
}

extern snet_buffer_t **SNetHndGetOutbuffers( snet_handle_t *hnd){

  snet_buffer_t **buf;
 
  switch( hnd->descr) {
    case HND_parallel: 
      buf = PAR_HND( buffers); 
      break;
    default: WrongHandleType();
  }

  return( buf);
}

extern bool SNetHndIsDet( snet_handle_t *hnd) {

  bool res;

  switch( hnd->descr) {
    case HND_parallel: 
      res = PAR_HND( is_det); 
      break;
    default: WrongHandleType();
  }

  return( res);
}

extern void *SNetHndGetBoxfun( snet_handle_t *hnd){

  void *fun;

  switch( hnd->descr) {
    case HND_box: 
      fun =  BOX_HND( boxfun_a); 
      break;
    case HND_split: 
      fun =  SPLIT_HND( boxfun_a); 
      break;
    default: WrongHandleType();
  }
 
  return( fun);
}

extern void *SNetHndGetBoxfunA( snet_handle_t *hnd){

  void *fun;

  switch( hnd->descr) {
    case HND_star: 
      fun =  STAR_HND( boxfun_a); 
      break;
    default: WrongHandleType();
  }
 
  return( fun);
}

extern void *SNetHndGetBoxfunB( snet_handle_t *hnd){

  void *fun;

  switch( hnd->descr) {
    case HND_star: 
      fun =  STAR_HND( boxfun_b); 
      break;
    default: WrongHandleType();
  }
 
  return( fun);
}

extern int SNetHndGetTagA( snet_handle_t *hnd) {
 
  int tag;

  switch( hnd->descr) {
    case HND_split: 
      tag = SPLIT_HND( tag_a); 
      break;
    default: WrongHandleType();
  }
 
  return( tag);
}

extern int SNetHndGetTagB( snet_handle_t *hnd) {

  int tag;

  switch( hnd->descr) {
    case HND_split: 
      tag = SPLIT_HND( tag_b); 
      break;
    default: WrongHandleType();
  }
 
  return( tag);
}


#ifdef FILTER_VERSION_2
extern snet_filter_instruction_set_list_t
**SNetHndGetFilterInstructionSetList( snet_handle_t *hnd) 
{
  snet_filter_instruction_set_list_t **lst;  

  switch( hnd->descr) {
    case HND_filter: 
      lst = FILTER_HND( instr_list); 
      break;
    default: WrongHandleType();
  }

  return( lst);
}

extern snet_typeencoding_list_t *SNetHndGetTypeList( snet_handle_t *hnd) 
{
 
  snet_typeencoding_list_t *lst;

  switch( hnd->descr) {
    case HND_filter: 
      type = SYNC_HND( typelist); 
      break;
   default: WrongHandleType();
  }
 
  return( lst);
}
#else
extern snet_filter_instruction_set_t **SNetHndGetFilterInstructions( snet_handle_t *hnd) {

  snet_filter_instruction_set_t **instr;

  switch( hnd->descr) {
    case HND_filter: 
      instr = FILTER_HND( instr_set); 
      break;
    default: WrongHandleType();
  }
 
  return( instr);
}
#endif

extern snet_typeencoding_t *SNetHndGetType( snet_handle_t *hnd) {
 
  snet_typeencoding_t *type;

  switch( hnd->descr) {
    case HND_box: 
      type = BOX_HND( type); 
      break;
    case HND_parallel: 
      type = PAR_HND( type); 
      break;
    case HND_star: 
      type = STAR_HND( type); 
      break;
    case HND_sync: 
      type = SYNC_HND( type); 
      break;
   default: WrongHandleType();
  }
 
  return( type);
}



extern snet_typeencoding_t *SNetHndGetInType( snet_handle_t *hnd) {
  
  snet_typeencoding_t *type;
  switch( hnd->descr) {
    case HND_filter: 
      type = FILTER_HND( in_type); 
      break;
   default: WrongHandleType();
  }

  return( type);
}


extern snet_typeencoding_t *SNetHndGetOutType( snet_handle_t *hnd) {
  
  snet_typeencoding_t *type;
  switch( hnd->descr) {
    case HND_filter: 
      type = FILTER_HND( out_type); 
      break;
   default: WrongHandleType();
  }

  return( type);
}


extern snet_typeencoding_t *SNetHndGetPatterns( snet_handle_t *hnd) {
  
  snet_typeencoding_t *patterns;

  switch( hnd->descr) {
    case HND_sync: 
      patterns = SYNC_HND( patterns); 
      break;
    default: WrongHandleType();
  }

  return( patterns);
}


extern bool SNetHndIsIncarnate( snet_handle_t *hnd) {
  
  bool res;

  switch( hnd->descr) {
    case HND_star: 
      res = STAR_HND( is_incarnate); 
      break;
    default: WrongHandleType();
  }

  return( res);
}


extern void SNetHndSetStringNames( snet_handle_t *hnd, int num, ...) {

  int i;
  va_list args;

  va_start( args, num);
 
  switch( hnd->descr) {
    case HND_box:
      BOX_HND( mapping) = SNetMemAlloc( sizeof( name_mapping_t));
      BOX_HND_MAPPING( num) = num;
      BOX_HND_MAPPING( int_names) = SNetMemAlloc( num * sizeof( int));
      BOX_HND_MAPPING( string_names) = SNetMemAlloc( num * sizeof( char*));
      for( i=0; i<num; i++) {
        char *str;

        BOX_HND_MAPPING( int_names[i]) = va_arg( args, int);
        str = va_arg( args, char*);
        BOX_HND_MAPPING( string_names[i]) = 
          SNetMemAlloc( (strlen( str) + 1) * sizeof( char));
        strcpy( BOX_HND_MAPPING( string_names[i]), str);
      }
      break;
    default: WrongHandleType();
  }
}


