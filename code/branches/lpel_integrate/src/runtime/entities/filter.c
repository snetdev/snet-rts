#include "filter.h"
#include "memfun.h"
#include "stdarg.h"
#include "bool.h"
#include "record.h"
#include "snetentities.h"
#include "debug.h"
#include "interface_functions.h"
#include "typeencode.h"
#include "stream_layer.h"
#include "threading.h" /* for enumeration */

#include "stream.h"
#include "task.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif
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
    case create_record: // noop
    break;
    case snet_tag:
    case snet_btag:
      instr->data = SNetMemAlloc( sizeof( int));
      instr->data[0] = va_arg( args, int);
      instr->expr = va_arg( args, snet_expr_t*);
      break;
    case snet_field:

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


extern void SNetDestroyFilterInstruction( snet_filter_instruction_t *instr) 
{
  if(instr->data != NULL) {
    SNetMemFree( instr->data);
  }
  if(instr->expr != NULL) {
    SNetEdestroyExpr(instr->expr);
  }
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
    if(set->instructions[i] != NULL) {
      SNetDestroyFilterInstruction(set->instructions[i]);
    }
  }

  SNetMemFree( set->instructions);
  SNetMemFree( set);
}

extern void
SNetDestroyFilterInstructionSetList( snet_filter_instruction_set_list_t *lst)
{
  int i;
  
  for( i=0; i<lst->num; i++) {
   if(lst->lst[i] != NULL) {
      SNetDestroyFilterInstructionSet(lst->lst[i]);
    }
  }
  SNetMemFree(lst->lst);
  SNetMemFree(lst);
}

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

static snet_record_t*
FilterInheritFromInrec( snet_typeencoding_t *in_type,
                         snet_record_t *in_rec,
                         snet_record_t *out_rec)
{
  int i;
  int *names;

  names = SNetRecGetUnconsumedFieldNames( in_rec);

  for( i=0; i<SNetRecGetNumFields( in_rec); i++) {
    if( !( FilterInTypeHasField( in_type, names[i]))) {
      if( SNetRecAddField( out_rec, names[i])) {
	SNetRecCopyFieldToRec(in_rec, names[i], 
			      out_rec, names[i]);
      }
    }
  }

  SNetMemFree(names);

  names = SNetRecGetUnconsumedTagNames( in_rec);
  for( i=0; i<SNetRecGetNumTags( in_rec); i++) {
    if( !( FilterInTypeHasTag( in_type, names[i]))) {
      if( SNetRecAddTag( out_rec, names[i])) {
        SNetRecSetTag( out_rec, names[i], 
                         SNetRecGetTag( in_rec, names[i])); 
      }
    }
  }

  SNetMemFree(names);

  return( out_rec);
}



//static void *FilterThread( void *hnd)
static void FilterTask(task_t *self, void *hnd)
{
  int i,j,k;
  bool done, terminate;
  stream_t *instream, *outstream;
  snet_record_t *in_rec;
  snet_expr_list_t *guard_list;
  snet_typeencoding_t *out_type, *in_type;
  snet_typeencoding_list_t *type_list;
  snet_filter_instruction_t *current_instr;
  snet_filter_instruction_set_t *current_set;
  snet_filter_instruction_set_list_t **instr_lst, *current_lst;

  done = false;
  terminate = false;

#ifdef FILTER_DEBUG
  SNetUtilDebugNotice("(CREATION FILTER)");
#endif
  instream = SNetHndGetInput( hnd);
  StreamOpen(self, instream, 'r');

  outstream = SNetHndGetOutput( hnd);
  StreamOpen(self, outstream, 'w');

  guard_list = SNetHndGetGuardList( hnd);
  in_type = SNetHndGetInType( hnd);
  type_list = SNetHndGetOutTypeList( hnd);
  instr_lst = SNetHndGetFilterInstructionSetLists( hnd);

  while( !( terminate)) {
    //in_rec = SNetTlRead( instream);
    in_rec = StreamRead( self, instream);
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
			SNetRecCopyFieldToRec(in_rec, 
					      getFieldNameFromInstr( current_instr), 
					      out_rec, 
					      getNameFromInstr( current_instr));
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
				      (unsigned int) outstream, (unsigned int) out_rec);
#endif
		  //SNetTlWrite( outstream, out_rec);
		  StreamWrite( self, outstream, out_rec);
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
        {
          //instream = SNetRecGetStream( in_rec);
          stream_t *newstream = SNetRecGetStream( in_rec);
          StreamReplace(self, &instream, newstream);

          SNetHndSetInput( hnd, SNetRecGetStream( in_rec));
          SNetRecDestroy( in_rec);
        }
        break;
        case REC_collect:
#ifdef DEBUG_FILTER
          SNetUtilDebugNotice("[Filter] Unhandled control record, destroying "
                              "it\n\n");
#endif
          SNetRecDestroy( in_rec);
        break;
        case REC_sort_begin:
        case REC_sort_end:
          //SNetTlWrite( SNetHndGetOutput( hnd), in_rec);
          StreamWrite( self, outstream, in_rec);
        break;
        case REC_terminate:
          terminate = true;
          StreamWrite( self, outstream, in_rec);
          SNetHndDestroy( hnd);
        break;
        /*
        case REC_probe:
          StreamWrite( outstream, in_rec);
        break;
        */
    default:
      SNetUtilDebugNotice("[Filter] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( in_rec));
      SNetRecDestroy(in_rec);
      break;
    } // switch rec_descr
  } // while not terminate
  StreamClose( self, outstream);
  StreamDestroy( outstream);
  StreamClose( self, instream);
}


extern stream_t* SNetFilter( stream_t *instream,
#ifdef DISTRIBUTED_SNET
				     snet_info_t *info, 
				     int location,
#endif /* DISTRIBUTED_SNET */
				     snet_typeencoding_t *in_type,
				     snet_expr_list_t *guards, ... )
{
  int i;
  int num_outtypes;
  snet_handle_t *hnd;
  stream_t *outstream;
  snet_expr_list_t *guard_expr;
  snet_filter_instruction_set_list_t *lst;
  snet_filter_instruction_set_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  va_list args;

#ifdef DISTRIBUTED_SNET
  instream = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), instream, location); 

  if(location == SNetIDServiceGetNodeID()) {

#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Filter created");
#endif /* DISTRIBUTED_DEBUG */

#endif /* DISTRIBUTED_SNET */

    outstream = StreamCreate(); //SNetTlCreateStream(BUFFER_SIZE);
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
    
    hnd = SNetHndCreate(
        HND_filter,
        instream, outstream,
        in_type, out_types,
        guard_expr, instr_list
        );
    
    SNetEntitySpawn( FilterTask, (void*)hnd, ENTITY_filter);
    
#ifdef DISTRIBUTED_SNET
  } else {
    SNetDestroyTypeEncoding(in_type);
    num_outtypes = SNetElistGetNumExpressions( guards);
    if(num_outtypes == 0) {
      num_outtypes += 1;
    }
    va_start( args, guards);
    for( i=0; i<num_outtypes; i++) {
      lst = va_arg( args, snet_filter_instruction_set_list_t*);
      if(lst != NULL) {
	SNetDestroyFilterInstructionSetList(lst);
      }
    }
    va_end( args);
    
    SNetEdestroyList( guards);
    outstream = instream;
  }
#endif /* DISTRIBUTED_SNET */
  return( outstream);
}


extern stream_t* SNetTranslate( stream_t *instream,
#ifdef DISTRIBUTED_SNET
					snet_info_t *info, 
					int location,
#endif /* DISTRIBUTED_SNET */
					snet_typeencoding_t *in_type,
					snet_expr_list_t *guards, ... )
{
  int i;
  int num_outtypes;
  snet_handle_t *hnd;
  stream_t *outstream;
  snet_expr_list_t *guard_expr;
  snet_filter_instruction_set_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  va_list args;

#ifdef DISTRIBUTED_SNET
  instream = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), instream, location); 
  if(location == SNetIDServiceGetNodeID()) {
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Translate created");
#endif /* DISTRIBUTED_DEBUG */
#endif /* DISTRIBUTED_SNET */
    
    outstream = StreamCreate(); //SNetTlCreateStream(BUFFER_SIZE);
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
    
    hnd = SNetHndCreate(
        HND_filter,
        instream, outstream,
        in_type, out_types,
        guard_expr, instr_list
        );
    
    SNetEntitySpawn( FilterTask, (void*)hnd, ENTITY_filter);
  
#ifdef DISTRIBUTED_SNET
  } else {
    SNetDestroyTypeEncoding(in_type);
    
    num_outtypes = SNetElistGetNumExpressions( guards);
    
    if(num_outtypes == 0) {
      num_outtypes += 1;
    }
    
    va_start( args, guards);

    for( i=0; i<num_outtypes; i++) {
      SNetDestroyFilterInstructionSetList(va_arg( args, snet_filter_instruction_set_list_t*));
      
    }

    va_end( args);
    
    SNetEdestroyList( guards);

    outstream = instream;
  }
#endif /* DISTRIBUTED_SNET */

  return( outstream);
}


//static void *NameshiftThread( void *h)
static void NameshiftTask( task_t *self, void *arg)
{
  bool terminate = false;
  snet_handle_t *hnd = (snet_handle_t*)arg;
  stream_t *outstream, *instream;
  snet_variantencoding_t *untouched;
  snet_record_t *rec;
  int i, num, *names, offset;

  instream = SNetHndGetInput( hnd);
  StreamOpen( self, instream, 'r');

  outstream = SNetHndGetOutput( hnd);
  StreamOpen( self, outstream, 'w');

  untouched = SNetTencGetVariant( SNetHndGetInType( hnd), 1);

  // Guards are misused for offset
  offset = SNetEevaluateInt( SNetEgetExpr( SNetHndGetGuardList( hnd), 0), NULL);

  while( !terminate) {
    //rec = SNetTlRead( instream);
    rec = StreamRead( self, instream);

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

        //SNetTlWrite(outstream, rec);
        StreamWrite( self, outstream, rec);
        break;
      case REC_sync:
      {
        stream_t *newstream = SNetRecGetStream(rec);
        SNetHndSetInput( hnd, newstream);
        StreamReplace( self, &instream, newstream);
        SNetRecDestroy( rec);
      }
      break;
      case REC_collect:
        #ifdef DEBUG_FILTER
        SNetUtilDebugNotice("[Filter] Unhandled control record, destroying"
                            " it\n\n");
        #endif
        SNetRecDestroy( rec);
      break;
      case REC_sort_begin:
      case REC_sort_end:
        StreamWrite( self, outstream, rec);
      break;
      case REC_terminate:
        terminate = true;
        StreamWrite( self, outstream, rec);
        SNetHndDestroy( hnd);
      break;
      /*
      case REC_probe:
        SNetTlWrite(SNetHndGetOutput(hnd), rec);
      break;
      */
    default:
      SNetUtilDebugNotice("[Filter] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( rec));
      SNetRecDestroy( rec);
      break;
    }
  }
  StreamClose( self, instream);
  StreamClose( self, outstream);
}


extern stream_t *SNetNameShift( stream_t *instream,
#ifdef DISTRIBUTED_SNET
					snet_info_t *info, 
					int location,
#endif /* DISTRIBUTED_SNET */
					int offset,
					snet_variantencoding_t *untouched)
{
  stream_t *outstream;
  snet_handle_t *hnd;

#ifdef DISTRIBUTED_SNET
  instream = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), instream, location);

  if(location == SNetIDServiceGetNodeID()) {

#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("Nameshift created");
#endif /* DISTRIBUTED_DEBUG */

#endif /* DISTRIBUTED_SNET */

    outstream = StreamCreate(); //SNetTlCreateStream( BUFFER_SIZE);
    
    hnd = SNetHndCreate( HND_filter, instream, outstream,
			 SNetTencTypeEncode( 1, untouched),
			 NULL, // outtypes
			 SNetEcreateList( 1, SNetEconsti( offset)),
			 NULL); // instructions
    
    SNetEntitySpawn( NameshiftTask, (void*)hnd, ENTITY_filter);
  
#ifdef DISTRIBUTED_SNET
  } else {
    
    SNetTencDestroyVariantEncoding( untouched);

    outstream = instream;
  }
#endif /* DISTRIBUTED_SNET */
  
  return( outstream);
}

