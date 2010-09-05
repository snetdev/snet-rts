/*
 * This implements the handle.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <memfun.h>
#include <snetentities.h>
#include <handle.h>

#include "task.h"

#include <bool.h>
#include "snettypes.h"
#include "stream_layer.h"
#include "debug.h"

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

#define FILL_HANDLE( PREFIX, INPUT, OUTPUTA, OUTPUTB, REC, FUNA, FUNB, TYPE, PATTERNS, TAGA, TAGB, FILTER)\
  VAL_OR_NULL( PREFIX, INPUT, inbuf, va_arg( args, buffer_t*))\
  VAL_OR_NULL( PREFIX, OUTPUTA, outbuf_a, va_arg( args, buffer_t*))\
  VAL_OR_NULL( PREFIX, OUTPUTB, outbuf_b, va_arg( args, buffer_t*))\
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
  stream_t *input;
  stream_t *output_a;
  snet_record_t *rec;
  snet_box_fun_t boxfun_a;
  snet_box_sign_t *sign;
  name_mapping_t *mapping;
  task_t *boxtask;
} box_handle_t;

typedef struct {
  stream_t *input;
  stream_t **outputs;
  snet_typeencoding_list_t *type;
  bool is_det;
} parallel_handle_t;

typedef struct {
  stream_t *input;
  stream_t *output_a;
  snet_box_fun_t boxfun_a;
  snet_box_fun_t boxfun_b;
  snet_typeencoding_t *type;
  snet_expr_list_t *guard_list;
  bool is_incarnate;
} star_handle_t;

typedef struct {
  stream_t *input;
  stream_t *output_a;
  snet_typeencoding_t *patterns;
  snet_typeencoding_t *type;
  snet_expr_list_t *guard_list;
} sync_handle_t;

typedef struct {
  stream_t *input;
  stream_t *output_a;
  snet_box_fun_t boxfun_a;
  int tag_a;
  int tag_b;
#ifdef DISTRIBUTED_SNET
  bool split_by_location;
#endif /* DISTRIBUTED_SNET */
} split_handle_t;


typedef struct {
  stream_t *input;
  stream_t *output_a;
  snet_typeencoding_t *in_type;
  snet_typeencoding_list_t *out_types;
  snet_expr_list_t *guard_list;
  snet_filter_instruction_set_list_t **instr_lists;
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
  SNetUtilDebugFatal("Wrong handle was passed!");
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
            BOX_HND( input) = va_arg( args, stream_t*);
            BOX_HND( output_a) = va_arg( args, stream_t*);
            BOX_HND( rec) = va_arg( args, snet_record_t*);
            BOX_HND( boxfun_a) = va_arg( args, void*);
            BOX_HND( sign) = va_arg( args, snet_box_sign_t*);
            BOX_HND( mapping) = NULL;
            BOX_HND( boxtask) = NULL;
            break;
    }
    case HND_parallel: {

            HANDLE( parallel_hnd) = SNetMemAlloc( sizeof( parallel_handle_t));
            PAR_HND( input) = va_arg( args, stream_t*);
            PAR_HND( outputs) = va_arg( args, stream_t**);
            PAR_HND( type) = va_arg( args, snet_typeencoding_list_t*);
            PAR_HND( is_det) = va_arg( args, bool);
            break;
    }

    case HND_star: {

            HANDLE( star_hnd) = SNetMemAlloc( sizeof( star_handle_t));
            STAR_HND( input) = va_arg( args, stream_t*);
            STAR_HND( output_a) = va_arg( args, stream_t*);
            STAR_HND( boxfun_a) = va_arg( args, void*);
            STAR_HND( boxfun_b) = va_arg( args, void*);
            STAR_HND( type) = va_arg( args, snet_typeencoding_t*);
            STAR_HND( guard_list) = va_arg( args, snet_expr_list_t*);
            STAR_HND( is_incarnate) = va_arg( args, bool);

            break;
    }

    case HND_sync: {

            HANDLE( sync_hnd) = SNetMemAlloc( sizeof( sync_handle_t));
            SYNC_HND( input) = va_arg( args, stream_t*);
            SYNC_HND( output_a) = va_arg( args, stream_t*);
            SYNC_HND( type) = va_arg( args, snet_typeencoding_t*);
            SYNC_HND( patterns) = va_arg( args, snet_typeencoding_t*);
            SYNC_HND( guard_list) = va_arg( args, snet_expr_list_t*);
            break;
    }

    case HND_split: {

            HANDLE( split_hnd) = SNetMemAlloc( sizeof( split_handle_t));
            SPLIT_HND( input) = va_arg( args, stream_t*);
            SPLIT_HND( output_a) = va_arg( args, stream_t*);
            SPLIT_HND( boxfun_a) = va_arg( args, void*);
            SPLIT_HND( tag_a) = va_arg( args, int);
            SPLIT_HND( tag_b) = va_arg( args, int);
#ifdef DISTRIBUTED_SNET
	    SPLIT_HND( split_by_location) = va_arg( args, bool);
#endif /* DISTRIBUTED_SNET */
            break;
    }

    case HND_filter:
      HANDLE( filter_hnd) = SNetMemAlloc( sizeof( filter_handle_t));
      FILTER_HND( input) = va_arg( args, stream_t*);
      FILTER_HND( output_a) = va_arg( args, stream_t*);
      FILTER_HND( in_type) = va_arg( args, snet_typeencoding_t*);
      FILTER_HND( out_types) = va_arg( args, snet_typeencoding_list_t*);
      FILTER_HND( guard_list) = va_arg( args, snet_expr_list_t*);
      FILTER_HND( instr_lists) = va_arg( args, snet_filter_instruction_set_list_t**);
      break;
    default: {
      SNetUtilDebugFatal("Cannot create requested handle type");
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


extern int 
SNetFilterInstructionsGetNumSets( snet_filter_instruction_set_list_t *lst) {
  int res;

  res = lst == NULL ? 0 : lst->num;
  
  return( res);  
}

extern snet_filter_instruction_set_t 
**SNetFilterInstructionsGetSets( snet_filter_instruction_set_list_t *lst) {
  
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
      SNetTencBoxSignDestroy( BOX_HND( sign));
      SNetMemFree( HANDLE( box_hnd)); 
      break;
    case HND_parallel: 
      SNetTencDestroyTypeEncodingList( PAR_HND( type));
      SNetMemFree( HANDLE( parallel_hnd)); 
      break;
    case HND_sync: 
      SNetEdestroyList( SYNC_HND(guard_list));
      SNetMemFree( HANDLE( sync_hnd)); 
      break;
    case HND_split: 
      SNetMemFree( HANDLE( split_hnd)); 
    break;
    case HND_filter: 
      SNetDestroyTypeEncoding( FILTER_HND( in_type));
      if(FILTER_HND( out_types) != NULL) {
	SNetTencDestroyTypeEncodingList( FILTER_HND( out_types));
      }

      if(FILTER_HND( instr_lists) != NULL) {
	for( i=0; i<SNetElistGetNumExpressions( FILTER_HND(guard_list)); i++) {
	  SNetDestroyFilterInstructionSetList( FILTER_HND(instr_lists)[i]);
	}                    
	
	SNetMemFree( FILTER_HND(instr_lists));
      }
      SNetEdestroyList( FILTER_HND(guard_list));
      SNetMemFree( HANDLE( filter_hnd)); 
    break;
    case HND_star: 
      SNetEdestroyList( STAR_HND(guard_list));
      SNetDestroyTypeEncoding( STAR_HND( type));
      SNetMemFree( HANDLE( star_hnd)); 
    break;
  }

  SNetMemFree( hnd->hnd);
  SNetMemFree( hnd);
}

extern snet_handle_t *SNetHndCopy( snet_handle_t *hnd) 
{
  printf("\n\n ** Runtime Error ** : Attempt to copy a handle. Handles must not be copied! (This is most likely a language-interface bug.)\n\n");
  exit( 1);
  return( hnd);
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

task_t *SNetHndGetBoxtask( snet_handle_t *hnd) {
  task_t *boxtask;
  switch( hnd->descr) {
    case HND_box: 
      boxtask = BOX_HND( boxtask); 
      break;
    default: WrongHandleType();
  }
  return( boxtask);
}

void SNetHndSetBoxtask( snet_handle_t *hnd, task_t *boxtask) {
 switch( hnd->descr) {
    case HND_box: 
      BOX_HND( boxtask) = boxtask; 
      break;
    default: WrongHandleType();
 }
}

extern void SNetHndSetInput(snet_handle_t *hnd, stream_t *input) {
  switch( hnd->descr) {
    case HND_box:
      BOX_HND( input) = input;
      break;
    case HND_parallel:
      PAR_HND( input) = input;
      break;
    case HND_star:
      STAR_HND( input) = input;
      break;
    case HND_split:
      SPLIT_HND( input) = input;
      break;
    case HND_sync:
      SYNC_HND( input) = input;
      break;
    case HND_filter:
      FILTER_HND( input) = input;
      break;
    default: WrongHandleType();
  }
}

extern stream_t *SNetHndGetInput( snet_handle_t *hnd) {
  stream_t *result;

  switch( hnd->descr) {
    case HND_box:
      result = BOX_HND( input);
      break;
    case HND_parallel:
      result = PAR_HND( input);
      break;
    case HND_star:
      result = STAR_HND( input);
      break;
    case HND_split:
      result = SPLIT_HND( input);
      break;
    case HND_sync:
      result = SYNC_HND( input);
      break;
    case HND_filter:
      result = FILTER_HND( input);
      break;
    default: WrongHandleType();
  }

  return( result);
}


extern stream_t *SNetHndGetOutput( snet_handle_t *hnd){

  stream_t *result;

  switch( hnd->descr) {
    case HND_box:
      result = BOX_HND( output_a);
      break;
    case HND_star:
      result = STAR_HND( output_a);
      break;
    case HND_split:
      result = SPLIT_HND( output_a);
      break;
    case HND_sync:
      result = SYNC_HND( output_a);
      break;
    case HND_filter:
      result = FILTER_HND( output_a);
      break;
    default: WrongHandleType();
  }

  return( result);
}

extern stream_t **SNetHndGetOutputs( snet_handle_t *hnd){

  stream_t **result;
  switch( hnd->descr) {
    case HND_parallel:
      result = PAR_HND( outputs);
      break;
    default: WrongHandleType();
  }

  return( result);
}

extern bool SNetHndIsDet( snet_handle_t *hnd) {

  bool result;

  switch( hnd->descr) {
    case HND_parallel:
      result = PAR_HND( is_det);
      break;
    default: WrongHandleType();
  }

  return( result);
}

#ifdef DISTRIBUTED_SNET
extern bool SNetHndIsSplitByLocation( snet_handle_t *hnd) {

  bool result;

  switch( hnd->descr) {
    case HND_split:
      result = SPLIT_HND( split_by_location); 
      break;
    default: WrongHandleType();
  }

  return( result);
}
#endif /* DISTRIBUTED_SNET */

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


extern snet_filter_instruction_set_list_t
**SNetHndGetFilterInstructionSetLists( snet_handle_t *hnd)
{
  snet_filter_instruction_set_list_t **lst;

  switch( hnd->descr) {
    case HND_filter:
      lst = FILTER_HND( instr_lists);
      break;
    default: WrongHandleType();
  }

  return( lst);
}

extern snet_typeencoding_list_t *SNetHndGetOutTypeList( snet_handle_t *hnd)
{
  snet_typeencoding_list_t *types;

  switch( hnd->descr) {
    case HND_filter:
      types = FILTER_HND( out_types);
      break;
   default: WrongHandleType();
  }

  return( types);
}


extern snet_expr_list_t *SNetHndGetGuardList( snet_handle_t *hnd)
{

  snet_expr_list_t *guards;

  switch( hnd->descr) {
    case HND_filter:
      guards = FILTER_HND( guard_list);
      break;
    case HND_star:
      guards = STAR_HND( guard_list);
      break;
    case HND_sync:
      guards = SYNC_HND( guard_list);
      break;
   default: WrongHandleType();
  }

  return( guards);
}


extern snet_typeencoding_t *SNetHndGetType( snet_handle_t *hnd) {
 
  snet_typeencoding_t *type;

  switch( hnd->descr) {
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

snet_box_sign_t *SNetHndGetBoxSign( snet_handle_t *hnd)
{
  snet_box_sign_t *t;

  switch( hnd->descr) {
    case HND_box: 
      t = BOX_HND( sign); 
      break;
   default: WrongHandleType();
  }

  return( t);
}

extern snet_typeencoding_list_t *SNetHndGetTypeList( snet_handle_t *hnd) {
 
  snet_typeencoding_list_t *type;

  switch( hnd->descr) {
    case HND_parallel: 
      type = PAR_HND( type); 
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


