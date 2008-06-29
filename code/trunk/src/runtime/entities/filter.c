#include "filter.h"
#include "memfun.h"
#include "threading.h"
#include "stdarg.h"
#include "bool.h"
#include "record.h"
#include "snetentities.h"
#include "debug.h"
#include "interface_functions.h"
#include "typeencode.h"

#define DEBUG_FILTER

/* ------------------------------------------------------------------------- */
/*  SNetFilter                                                               */
/* ------------------------------------------------------------------------- */


extern snet_filter_instruction_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...) {

  va_list args;
  snet_filter_instruction_t *instr;

  instr = SNetMemAlloc( sizeof( snet_filter_instruction_t));
  instr->opcode = opcode;
  instr->data = NULL;
  instr->expr = NULL;

  va_start( args, opcode);   

  switch( opcode) {
#ifdef FILTER_VERSION_2
    case create_record: // noop
    break;
    case snet_tag:
    case snet_btag:
      instr->data = SNetMemAlloc( sizeof( int));
      instr->data[0] = va_arg( args, int);
      instr->expr = va_arg( args, snet_expr_t*);
      break;
#else
    case FLT_add_tag:
    case FLT_strip_field:
    case FLT_strip_tag:
    case FLT_use_field:
    case FLT_use_tag:
      instr->data = SNetMemAlloc( sizeof( int));
      instr->data[0] = va_arg( args, int);
      break;
    case FLT_set_tag:
      instr->data = SNetMemAlloc( sizeof( int));
      //instr->expr = SNetMemAlloc( sizeof( snet_expr_t*));
      instr->data[0] = va_arg( args, int);
      instr->expr = va_arg( args, snet_expr_t*);
      break;
    case FLT_rename_tag:
    case FLT_rename_field:
    case FLT_copy_field:
#endif
#ifdef FILTER_VERSION_2
    case snet_field:
#endif

      instr->data = SNetMemAlloc( 2 * sizeof( int));
      instr->data[0] = va_arg( args, int);
      instr->data[1] = va_arg( args, int);
      break;
    default:
      SNetUtilDebugFatal("[Filter] Filter operation (%d) not implemented\n\n", 
                            opcode);
  }
  
  va_end( args);

  
  return( instr);  
}


extern void SNetDestroyFilterInstruction( snet_filter_instruction_t *instr) {
  
  SNetMemFree( instr->data);
  //SNetMemFree( instr->expr);
  SNetMemFree( instr);
}

extern snet_filter_instruction_set_t *SNetCreateFilterInstructionSet( int num, ...) {

  va_list args;
  int i;
  snet_filter_instruction_set_t *set;

  set = SNetMemAlloc( sizeof( snet_filter_instruction_set_t));

  set->num = num;
  set->instructions = SNetMemAlloc( num * sizeof( snet_filter_instruction_t*));

  va_start( args, num);

  for( i=0; i<num; i++) {
    set->instructions[i] = va_arg( args, snet_filter_instruction_t*);
  }

  va_end( args);

  return( set);
}


extern int SNetFilterGetNumInstructions( snet_filter_instruction_set_t *set) 
{
  if( set == NULL) {
    return( 0);
  }
  else {
    return( set->num);
  }
}


extern snet_filter_instruction_set_list_t *SNetCreateFilterInstructionSetList( int num, ...) {
  va_list args;
  int i;
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


extern void SNetDestroyFilterInstructionSet( snet_filter_instruction_set_t *set) {

  int i;

  for( i=0; i<set->num; i++) {
    SNetDestroyFilterInstruction( set->instructions[i]);
  }

  SNetMemFree( set->instructions);
  SNetMemFree( set);
}
#ifdef FILTER_VERSION_2
inline static void InitTypeArrayEntry( snet_typeencoding_t **type_array,
                                                int i) {
  SNetTencAddVariant( type_array[i], 
    SNetTencVariantEncode( 
      SNetTencCreateEmptyVector( 0),
      SNetTencCreateEmptyVector( 0),
      SNetTencCreateEmptyVector( 0)));
}

// pass at least one set!! -> lst must not be NULL!
static snet_typeencoding_list_t 
*FilterComputeTypes( int num, 
                     snet_filter_instruction_set_list_t **lst) 
{
  int i, j, k;
  snet_typeencoding_t **type_array;
  snet_filter_instruction_set_t **sets;
  snet_filter_instruction_set_t *current_set;
  snet_filter_instruction_t *current_instr;

  type_array = SNetMemAlloc( num * sizeof( snet_typeencoding_t*));

    for( i=0; i<num; i++) {
      
      sets = SNetFilterInstructionsGetSets( lst[i]);
      type_array[i] = SNetTencTypeEncode( 0);
      for( j=0; j<SNetFilterInstructionsGetNumSets( lst[i]); j++) {
        bool variant_created = false;
        current_set = sets[j];
        for( k=0; k<current_set->num; k++) {
          current_instr = current_set->instructions[k];
          switch( current_instr->opcode) {
            case snet_tag:
              if( variant_created) {
                SNetTencAddTag( 
                    SNetTencGetVariant( type_array[i], j+1), 
                    current_instr->data[0]);
              }
              else {
                InitTypeArrayEntry( type_array, i);
                variant_created = true;               
              }
              break;
            case snet_btag:
              if( variant_created) {
                SNetTencAddBTag( 
                    SNetTencGetVariant( type_array[i], j+1), 
                    current_instr->data[0]);
              }
              else {
                InitTypeArrayEntry( type_array, i);
                variant_created = true;               
              }                
              break;
            case snet_field:
              if( variant_created) {
                SNetTencAddField( 
                      SNetTencGetVariant( type_array[i], j+1), 
                      current_instr->data[0]);
              }
              else {
                InitTypeArrayEntry( type_array, i);
                variant_created = true;               
              }              
              break;
            case create_record:
              if( !variant_created) {
                InitTypeArrayEntry( type_array, i);
                variant_created = true;
              }
              else {
                SNetUtilDebugFatal("[Filter] record_create applied to non-empty"
                                    " type variant.");
              }
              break;
            default:
              SNetUtilDebugFatal("[Filter] Unknown opcode in filter instruction"
                                 " [%d]\n\n",
                                current_instr->opcode);
          }
        }
      }
    }


  return( SNetTencCreateTypeEncodingListFromArray( num, type_array));
}

static int getNameFromInstr( snet_filter_instruction_t *instr) 
{
  return( instr->data[0]);
}

static int getFieldNameFromInstr( snet_filter_instruction_t *instr) 
{
  return( instr->data[1]);
}

static snet_expr_t *getExprFromInstr( snet_filter_instruction_t *instr) 
{
  return( instr->expr);
}
 
static snet_filter_instruction_t 
*FilterGetInstruction( snet_filter_instruction_set_t *set, int num) 
{
  if( set == NULL) {
    return( NULL);
  } 
  else {
    return( set->instructions[num]);
  }
}

static snet_filter_instruction_set_t
*FilterGetInstructionSet( snet_filter_instruction_set_list_t *l, int num) 
{
  if( l == NULL) {
    return( NULL);
  }
  else {
    return( l->lst[num]);
  }
}

static bool FilterInTypeHasName( int *names, int num, int name) 
{
  int i;
  bool result;

  result = false;

  for( i=0; i<num; i++) {
    if( names[i] == name) {
      result = true;
      break;
    }
  }

  return( result);
}

static bool FilterInTypeHasField( snet_typeencoding_t *t, int name) 
{
  snet_variantencoding_t *v;

  v = SNetTencGetVariant( t, 1);

  return( FilterInTypeHasName( 
            SNetTencGetFieldNames( v),
            SNetTencGetNumFields( v),
            name));
}

static bool FilterInTypeHasTag( snet_typeencoding_t *t, int name) 
{
  snet_variantencoding_t *v;

  v = SNetTencGetVariant( t, 1);

  return( FilterInTypeHasName( 
            SNetTencGetTagNames( v),
            SNetTencGetNumTags( v),
            name));
}

static snet_record_t 
*FilterInheritFromInrec( snet_typeencoding_t *in_type, 
                         snet_record_t *in_rec, 
                         snet_record_t *out_rec)
{
  int i;
  int *names;

  names = SNetRecGetFieldNames( in_rec);
  void* (*copyfun)(void*);
  copyfun = SNetGlobalGetCopyFun( SNetGlobalGetInterface( 
                                    SNetRecGetInterfaceId( in_rec)));
  for( i=0; i<SNetRecGetNumFields( in_rec); i++) {
    if( !( FilterInTypeHasField( in_type, SNetRecGetFieldNames( in_rec)[i]))) {
      if( SNetRecAddField( out_rec, names[i])) {
        SNetRecSetField( out_rec, names[i], 
                         copyfun( SNetRecGetField( in_rec, names[i]))); 
      }
    }
  }

  names = SNetRecGetTagNames( in_rec);
  for( i=0; i<SNetRecGetNumTags( in_rec); i++) {
    if( !( FilterInTypeHasTag( in_type, SNetRecGetTagNames( in_rec)[i]))) {
      if( SNetRecAddTag( out_rec, names[i])) {
        SNetRecSetTag( out_rec, names[i], 
                         SNetRecGetTag( in_rec, names[i])); 
      }
    }
  }
  
  return( out_rec);
}

static void *FilterThread( void *hnd) 
{
  int i,j,k;
  bool done, terminate;
  snet_buffer_t *inbuf, *outbuf;
  snet_record_t *in_rec;
  snet_expr_list_t *guard_list;
  snet_typeencoding_t *out_type, *in_type;
  snet_typeencoding_list_t *type_list;
  snet_filter_instruction_t *current_instr;
  snet_filter_instruction_set_t *current_set; 
  snet_filter_instruction_set_list_t **instr_lst, *current_lst;

  done = false;
  terminate = false;
  
  inbuf = SNetHndGetInbuffer( hnd);
  outbuf = SNetHndGetOutbuffer( hnd);
  guard_list = SNetHndGetGuardList( hnd);
  in_type = SNetHndGetInType( hnd);
  type_list = SNetHndGetOutTypeList( hnd); 
  instr_lst = SNetHndGetFilterInstructionSetLists( hnd);

  while( !( terminate)) {
    in_rec = SNetBufGet( inbuf);
    done = false;

    switch( SNetRecGetDescriptor( in_rec)) {
      case REC_data:
          for( i=0; i<SNetElistGetNumExpressions( guard_list); i++) {
              if( ( SNetEevaluateBool( SNetEgetExpr( guard_list, i), in_rec)) 
                  && !( done)) { 
                snet_record_t *out_rec = NULL;
                done = true;
                out_type = SNetTencGetTypeEncoding( type_list, i);
                current_lst = instr_lst[i];
                for( j=0; j<SNetTencGetNumVariants( out_type); j++) {
                  out_rec = SNetRecCreate( REC_data, 
                                           SNetTencCopyVariantEncoding( 
                                           SNetTencGetVariant( out_type, j+1)));
                  SNetRecCopyIterations(in_rec, out_rec);
                  SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( in_rec));
		  SNetRecSetDataMode( out_rec, SNetRecGetDataMode( in_rec));

                  current_set = FilterGetInstructionSet( current_lst, j);
                  for( k=0; k<SNetFilterGetNumInstructions( current_set); k++) {
                    current_instr = FilterGetInstruction( current_set, k);
                    switch( current_instr->opcode) {
                      case snet_tag:
                        SNetRecSetTag( 
                            out_rec, 
                            getNameFromInstr( current_instr),
                            SNetEevaluateInt( 
                              getExprFromInstr( current_instr), in_rec));
                        break;
                      case snet_btag:
                        SNetRecSetBTag( 
                            out_rec, 
                            getNameFromInstr( current_instr),
                            SNetEevaluateInt( 
                              getExprFromInstr( current_instr), in_rec));
                        break;
                      case snet_field: {
                        void* (*copyfun)(void*);
                        copyfun = SNetGlobalGetCopyFun( 
                                   SNetGlobalGetInterface( SNetRecGetInterfaceId( in_rec)));
                        SNetRecSetField( 
                            out_rec, 
                            getNameFromInstr( current_instr),
                            copyfun( SNetRecGetField( in_rec, 
                                     getFieldNameFromInstr( current_instr))));
                        }
                        break;
                        case create_record: //noop
                        break;
                      default:
                        SNetUtilDebugFatal("[Filter] Unknown opcode in filter" 
					   " instruction [%d]\n\n",
					   current_instr->opcode);
                    } 
                  } // forall instructions of current_set

                  out_rec = FilterInheritFromInrec( in_type, in_rec, out_rec);
                  #ifdef DEBUG_FILTER
                    SNetUtilDebugNotice("FILTER %x: outputting %x",
                      (unsigned int) outbuf, (unsigned int) out_rec);
                  #endif
                  SNetBufPut( outbuf, out_rec);
                } // forall variants of selected out_type
              } // if guard is true
            } // forall guards
            SNetRecDestroy( in_rec);
            if( !( done)) {
              #ifdef DEBUG_FILTER
              SNetUtilDebugFatal("[Filter] All guards evaluated to FALSE.\n\n");
              #endif
            }
        break; // case rec_data
        case REC_sync:
          SNetHndSetInbuffer( hnd, SNetRecGetBuffer( in_rec));
          inbuf = SNetRecGetBuffer( in_rec);
          SNetRecDestroy( in_rec);
        break;
        case REC_collect:
          #ifdef DEBUG_FILTER
          SNetUtilDebugNotice("[Filter] Unhandled control record, destroying "
                              "it\n\n");
          #endif
          SNetRecDestroy( in_rec);
        break;
        case REC_sort_begin:
          SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
        case REC_sort_end:
          SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
        case REC_terminate:
          terminate = true;
          SNetBufPut( outbuf, in_rec);
          SNetBufBlockUntilEmpty( outbuf);
          SNetBufDestroy( outbuf);
          SNetHndDestroy( hnd);
        break;
        case REC_probe:
          SNetBufPut(SNetHndGetOutbuffer(hnd), in_rec);
        break;
    } // switch rec_descr
    
  } // while not terminate
    
  return( NULL);
}


extern snet_buffer_t 
*SNetFilter( snet_buffer_t *inbuf,
             snet_typeencoding_t *in_type,
             snet_expr_list_t *guards, ... ) 
{

  int i;
  int num_outtypes;
  snet_handle_t *hnd;
  snet_buffer_t *outbuf;
  snet_expr_list_t *guard_expr;
  snet_filter_instruction_set_list_t *lst;
  snet_filter_instruction_set_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  va_list args;

  outbuf = SNetBufCreate( BUFFER_SIZE);
  guard_expr = guards;

  if( guard_expr == NULL) {
    guard_expr = SNetEcreateList( 1, SNetEconstb( true));
  }

  num_outtypes = SNetElistGetNumExpressions( guard_expr);

  instr_list = SNetMemAlloc( num_outtypes * sizeof( snet_filter_instruction_set_list_t*));
  
  va_start( args, guards);
  for( i=0; i<num_outtypes; i++) {
    lst = va_arg( args, snet_filter_instruction_set_list_t*);
    instr_list[i] = lst == NULL ? SNetCreateFilterInstructionList( 0) : lst;
  }
  va_end( args);
  

  out_types = FilterComputeTypes( num_outtypes, instr_list);

  hnd = SNetHndCreate( HND_filter, inbuf, outbuf, in_type, out_types, guard_expr, instr_list);

  SNetThreadCreate( FilterThread, (void*)hnd, ENTITY_filter);

  return( outbuf);
}


extern snet_buffer_t 
*SNetTranslate( snet_buffer_t *inbuf,
                snet_typeencoding_t *in_type,
                snet_expr_list_t *guards, ... ) 
{

  int i;
  int num_outtypes;
  snet_handle_t *hnd;
  snet_buffer_t *outbuf;
  snet_expr_list_t *guard_expr;
  snet_filter_instruction_set_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  va_list args;

  outbuf = SNetBufCreate( BUFFER_SIZE);
  guard_expr = guards;

  if( guard_expr == NULL) {
    guard_expr = SNetEcreateList( 1, SNetEconstb( true));
  }

  num_outtypes = SNetElistGetNumExpressions( guard_expr);

  instr_list = SNetMemAlloc( num_outtypes * sizeof( snet_filter_instruction_set_list_t*));
  
  va_start( args, guards);
  for( i=0; i<num_outtypes; i++) {
    instr_list[i] = va_arg( args, snet_filter_instruction_set_list_t*);
  }
  va_end( args);
  

  out_types = FilterComputeTypes( num_outtypes, instr_list);

  hnd = SNetHndCreate( HND_filter, inbuf, outbuf, in_type, out_types, guard_expr, instr_list);

  SNetThreadCreate( FilterThread, (void*)hnd, ENTITY_filter);

  return( outbuf);
}


static void *NameshiftThread( void *h) 
{
  bool terminate = false;
  snet_handle_t *hnd = (snet_handle_t*)h;
  snet_buffer_t *outbuf, *inbuf;
  snet_variantencoding_t *untouched;
  snet_record_t *rec;
  int i, num, *names, offset;

  inbuf = SNetHndGetInbuffer( hnd);
  outbuf = SNetHndGetOutbuffer( hnd);
  untouched = SNetTencGetVariant( SNetHndGetInType( hnd), 1);

  // Guards are misused for offset 
  offset = SNetEevaluateInt( SNetEgetExpr( SNetHndGetGuardList( hnd), 0), NULL);

  while( !terminate) {
    rec = SNetBufGet( inbuf);

    switch( SNetRecGetDescriptor( rec)) {
      case REC_data:
        names = SNetRecGetUnconsumedFieldNames( rec);
        num = SNetRecGetNumFields( rec);
        for( i=0; i<num; i++) {
          if(!SNetTencContainsFieldName(untouched, names[i])) {
            SNetRecRenameField( rec, names[i], names[i] + offset);
          }
        }
        SNetMemFree( names);

        names = SNetRecGetUnconsumedTagNames( rec);
        num = SNetRecGetNumTags( rec);
        for( i=0; i<num; i++) {
          if(!SNetTencContainsTagName(untouched, names[i])) {
            SNetRecRenameTag( rec, names[i], names[i] + offset);
          }
        }
        SNetMemFree( names);

        names = SNetRecGetUnconsumedBTagNames( rec);
        num = SNetRecGetNumBTags( rec);
        for( i=0; i<num; i++) {
          if(!SNetTencContainsBTagName(untouched, names[i])) {
            SNetRecRenameBTag( rec, names[i], names[i] + offset);
          }
        }
        SNetMemFree( names);

        SNetBufPut( outbuf, rec);
        break;
      case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( rec));
        inbuf = SNetRecGetBuffer( rec);
        SNetRecDestroy( rec);
      break;
      case REC_collect:
        #ifdef DEBUG_FILTER
        SNetUtilDebugNotice("[Filter] Unhandled control record, destroying"
                            " it\n\n");
        #endif
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
        SNetBufPut( outbuf, rec);
        SNetBufBlockUntilEmpty( outbuf);
        SNetBufDestroy( outbuf);
        SNetHndDestroy( hnd);
      break;
      case REC_probe:
        SNetBufPut(SNetHndGetOutbuffer(hnd), rec);
      break;
    }
  }

  return( NULL);
}


extern snet_buffer_t 
*SNetNameShift( snet_buffer_t *inbuf, 
                int offset,
                snet_variantencoding_t *untouched) 
{
  snet_buffer_t *outbuf;
  snet_handle_t *hnd;

  outbuf = SNetBufCreate( BUFFER_SIZE);

  hnd = SNetHndCreate( HND_filter, inbuf, outbuf, 
                       SNetTencTypeEncode( 1, untouched), 
                       NULL, // outtypes
                       SNetEcreateList( 1, SNetEconsti( offset)),
                       NULL); // instructions
  
  SNetThreadCreate( NameshiftThread, (void*)hnd, ENTITY_filter);

  return( outbuf);
}

#else
static void *FilterThread( void *hndl) {

  int i,j;
  snet_variantencoding_t *names;
  snet_handle_t *hnd = (snet_handle_t*) hndl;
  snet_buffer_t *outbuf;
  snet_typeencoding_t *out_type;
  snet_filter_instruction_set_t **instructions, *set;
  snet_filter_instruction_t *instr;
  snet_variantencoding_t *variant;
  snet_record_t *out_rec, *in_rec;
  bool terminate = false;

  outbuf = SNetHndGetOutbuffer( hnd);
//  in_variant = SNetTencGetVariant( SNetHndGetInType( hnd), 1);
  out_type =  SNetHndGetOutType( hnd);

  instructions = SNetHndGetFilterInstructions( hnd);
  #ifdef DEBUG_FILTER
  SNetUtilDebugNotice("[Filter] Use of filter v1 is deprecated.\n\n");
  #endif
  while( !( terminate)) {
    in_rec = SNetBufGet( SNetHndGetInbuffer( hnd));
    switch( SNetRecGetDescriptor( in_rec)) {
      case REC_data:
        for( i=0; i<SNetTencGetNumVariants( out_type); i++) {
        names = SNetTencCopyVariantEncoding( SNetRecGetVariantEncoding( in_rec));
        variant = SNetTencCopyVariantEncoding( SNetTencGetVariant( out_type, i+1));
        out_rec = SNetRecCreate( REC_data, variant);
        SNetRecCopyIterations(in_rec, out_rec);
        SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( in_rec));
        SNetRecSetDataMode( out_rec, SNetRecGetDataMode( in_rec));	
        set = instructions[i];
        // this runs for each filter instruction 
        for( j=0; j<set->num; j++) {
          instr = set->instructions[j];
          switch( instr->opcode) {
            case FLT_strip_tag:
              SNetTencRemoveTag( names, instr->data[0]);
              break;
            case FLT_strip_field:
//              SNetFreeField( 
//                  SNetRecGetField( in_rec, instr->data[0]),
//                  SNetRecGetLanguage( in_rec));
              SNetTencRemoveField( names, instr->data[0]);
              break;
            case FLT_add_tag:
              SNetRecSetTag( out_rec, instr->data[0], 0); 
              break;
            case FLT_set_tag:
              // expressions are ignored for now!
              SNetRecSetTag( out_rec, instr->data[0], 0);
              break;
            case FLT_rename_tag:
              SNetRecSetTag( out_rec, instr->data[1], 
                  SNetRecGetTag( in_rec, instr->data[0]));
              SNetTencRemoveTag( names, instr->data[0]);
              break;
            case FLT_rename_field:
              SNetRecSetField( out_rec, instr->data[1], 
                  SNetRecGetField( in_rec, instr->data[0]));
              SNetTencRemoveField( names, instr->data[0]);
              break;
            case FLT_copy_field: {
              void* (*copyfun)(void*);
              copyfun = GetCopyFun( GetInterface( SNetRecGetInterfaceId( in_rec)));
              SNetRecSetField( out_rec, instr->data[0],
                copyfun( SNetRecGetField( in_rec, instr->data[0])));
              }
              break;
            case FLT_use_tag:
              SNetRecSetTag( out_rec, instr->data[0],
                  SNetRecGetTag( in_rec, instr->data[0]));
              break;
            case FLT_use_field:
              SNetRecSetField( out_rec, instr->data[0],
                  SNetRecGetField( in_rec, instr->data[0]));
              break;
          }
        }
        // flow inherit, remove everthing that is already present in the 
        // outrecord. Renamed fields/tags are removed above.
        // TODO: remove everything that is present in in_type but not in out_type
        for( j=0; j<SNetTencGetNumFields( variant); j++) {
//          SNetFreeField( 
//              SNetRecGetField( in_rec, SNetTencGetFieldNames( variant)[j]),
//              SNetRecGetLanguage( in_rec));
          SNetTencRemoveField( names, SNetTencGetFieldNames( variant)[j]);
        }
        for( j=0; j<SNetTencGetNumTags( variant); j++) {
          SNetTencRemoveTag( names, SNetTencGetTagNames( variant)[j]);
        }
  
       // add everything that is left.
        void* (*copyfun)(void*);
        copyfun = GetCopyFun( GetInterface( SNetRecGetInterfaceId( in_rec)));
        for( j=0; j<SNetTencGetNumFields( names); j++) {
         if( SNetRecAddField( out_rec, SNetTencGetFieldNames( names)[j])) {
            SNetRecSetField( out_rec, 
                SNetTencGetFieldNames( names)[j],
                copyfun( SNetRecGetField( in_rec, SNetTencGetFieldNames( names)[j])));
         }
       }  
      for( j=0; j<SNetTencGetNumTags( names); j++) {
         if( SNetRecAddTag( out_rec, SNetTencGetTagNames( names)[j])) {
            SNetRecSetTag( out_rec, 
                SNetTencGetTagNames( names)[j],
                SNetRecGetTag( in_rec, SNetTencGetTagNames( names)[j]));
         }
      }  

      SNetTencDestroyVariantEncoding( names);  
      SNetBufPut( outbuf, out_rec);
      }
      SNetRecDestroy( in_rec);
      break;
    case REC_sync:
        SNetHndSetInbuffer( hnd, SNetRecGetBuffer( in_rec));
        SNetRecDestroy( in_rec);
      break;
      case REC_collect:
        #ifdef DEBUG_FILTER
        SNetUtilDebugNotice("[Filter] Unhandled control record, destroying "
                            "it\n\n");
        #endif
        SNetRecDestroy( in_rec);
        break;
      case REC_sort_begin:
        SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
      case REC_sort_end:
        SNetBufPut( SNetHndGetOutbuffer( hnd), in_rec);
        break;
    case REC_terminate:
      terminate = true;
      SNetBufPut( outbuf, in_rec);
      SNetBufBlockUntilEmpty( outbuf);
      SNetBufDestroy( outbuf);
      SNetHndDestroy( hnd);
      break;
    }
  }  

  return( NULL);
}



static snet_buffer_t *CreateFilter( snet_buffer_t *inbuf, 
                                    snet_typeencoding_t *in_type,
                                    snet_typeencoding_t *out_type,
                                    snet_filter_instruction_set_t **set,
                                    bool is_translator) {

  snet_buffer_t *outbuf;
  snet_handle_t *hnd;

  outbuf = SNetBufCreate( BUFFER_SIZE);
  hnd = SNetHndCreate( HND_filter, inbuf, outbuf, in_type, out_type, set);


//  if( is_translator) { // for the time being, a translator is just a filter
    SNetThreadCreate( FilterThread, (void*)hnd, ENTITY_filter);
//  } 
//  else {
//    SNetThreadCreate( &box_thread, NULL, FilterThread, (void*)hnd);
//  }
  

  return( outbuf);


}


extern snet_buffer_t *SNetFilter( snet_buffer_t *inbuf, 
                                  snet_typeencoding_t *in_type,   
                                  snet_typeencoding_list_t *out_types, 
                                  snet_expr_list_t *guards,  ...) {
  int i;
  va_list args;

  snet_filter_instruction_set_list_t *lst;
  snet_filter_instruction_set_t **set;
  snet_typeencoding_t *out_type;

  // *
  if( SNetTencGetNumTypes( out_types) > 1) {
    SNetUtilDebugFatal("[Filter] Requested feature not implemented yet\n\n");
  }

  out_type = SNetTencGetTypeEncoding( out_types, 0);

//  set = SNetMemAlloc( SNetTencGetNumVariants( out_type) * sizeof( snet_filter_instruction_set_t*));
  va_start( args, guards);

  for( i=0; i < SNetTencGetNumTypes( out_types) ; i++) {
    lst = va_arg( args, snet_filter_instruction_set_list_t*);
    set = SNetFilterInstructionsGetSets( lst); // !!!!!
  }
  va_end( args);

  return( CreateFilter( inbuf, in_type, out_type, set, false));
}


extern snet_buffer_t *SNetTranslate( snet_buffer_t *inbuf, 
                                     snet_typeencoding_t *in_type,   
                                     snet_typeencoding_t *out_type, ...) {
  int i;
  va_list args;

  snet_filter_instruction_set_t **set;

  set = SNetMemAlloc( SNetTencGetNumVariants( out_type) * sizeof( snet_filter_instruction_set_t*));

  va_start( args, out_type);
  for( i=0; i<SNetTencGetNumVariants( out_type); i++) {
    set[i] = va_arg( args, snet_filter_instruction_set_t*);
  }
  va_end( args);

  return( CreateFilter( inbuf, in_type, out_type, set, true));
}
#endif
