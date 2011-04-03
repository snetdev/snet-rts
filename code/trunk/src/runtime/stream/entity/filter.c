#include "filter.h"
#include "list.h"
#include "memfun.h"
#include "stdarg.h"
#include "bool.h"
#include "record.h"
#include "snetentities.h"
#include "debug.h"
#include "interface_functions.h"
#include "typeencode.h"
#include "distribution.h"

#include "threading.h"

/* ------------------------------------------------------------------------- */
/*  SNetFilter                                                               */
/* ------------------------------------------------------------------------- */

snet_filter_instr_t *SNetCreateFilterInstruction( snet_filter_opcode_t opcode, ...)
{
  va_list args;
  snet_filter_instr_t *instr;

  instr = SNetMemAlloc( sizeof( snet_filter_instr_t));
  instr->opcode = opcode;
  instr->expr = NULL;

  va_start( args, opcode);
  switch (opcode) {
    case create_record: // noop
    break;
    case snet_tag:
    case snet_btag:
      instr->name = va_arg( args, int);
      instr->expr = va_arg( args, snet_expr_t*);
      break;
    case snet_field:
      instr->newName = va_arg( args, int);
      instr->name = va_arg( args, int);
      break;
    default:
      SNetUtilDebugFatal("[Filter] Filter operation (%d) not implemented\n\n",
                         opcode);
  }
  va_end( args);

  return instr;
}

void SNetDestroyFilterInstruction( snet_filter_instr_t *instr)
{
  if(instr->expr != NULL) {
    SNetEdestroyExpr(instr->expr);
  }

  SNetMemFree( instr);
}

inline static void InitTypeArrayEntry(
    snet_typeencoding_t **type_array, int i)
{
  SNetTencAddVariant( type_array[i], 
      SNetTencVariantEncode( 
        SNetTencCreateEmptyVector( 0),
        SNetTencCreateEmptyVector( 0),
        SNetTencCreateEmptyVector( 0)));
}

// pass at least one set!! -> lst must not be NULL!
static snet_typeencoding_list_t *FilterComputeTypes(
    int num, snet_filter_instr_list_list_t **lists)
{
  int i;
  snet_typeencoding_t **type_array;
  snet_filter_instr_list_t *current_list;
  snet_filter_instr_t *current_instr;

  type_array = SNetMemAlloc( num * sizeof( snet_typeencoding_t*));

    for (i=0; i<num; i++) {
      type_array[i] = SNetTencTypeEncode( 0);
      int j = 0;
      LIST_FOR_EACH(lists[i], current_list)
        j++;
        bool variant_created = false;
        LIST_FOR_EACH(current_list, current_instr)
          switch (current_instr->opcode) {
            case snet_tag:
              if (variant_created) {
                SNetTencAddTag( 
                    SNetTencGetVariant( type_array[i], j), 
                    current_instr->name);
              } else {
                InitTypeArrayEntry( type_array, i);
                variant_created = true;
              }
              break;
            case snet_btag:
              if( variant_created) {
                SNetTencAddBTag( 
                    SNetTencGetVariant( type_array[i], j), 
                    current_instr->name);
              } else {
                InitTypeArrayEntry( type_array, i);
                variant_created = true;
              }
              break;
            case snet_field:
              if( variant_created) {
                SNetTencAddField( 
                      SNetTencGetVariant( type_array[i], j), 
                      current_instr->newName);
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
        END_FOR
      END_FOR
    }


  return( SNetTencCreateTypeEncodingListFromArray( num, type_array));
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

static snet_record_t *FilterInheritFromInrec(
    snet_typeencoding_t *in_type,
    snet_record_t *in_rec,
    snet_record_t *out_rec)
{
  int name, val;
  snet_ref_t *ref;

  FOR_EACH_FIELD(in_rec, name, ref)
    if (!FilterInTypeHasField( in_type, name)) {
      SNetRecSetField( out_rec, name, ref);
    }
  END_FOR

  FOR_EACH_TAG(in_rec, name, val)
    if (!FilterInTypeHasTag( in_type, name)) {
      SNetRecSetTag( out_rec, name, val);
    }
  END_FOR

  return out_rec;
}


/*****************************************************************************/

typedef struct {
  snet_stream_t *input, *output;
  snet_expr_list_t *guard_list;
  snet_typeencoding_t *in_type;
  snet_typeencoding_list_t *type_list;
  snet_filter_instr_list_list_t **instr_lst;
} filter_arg_t;

static void FilterArgDestroy( filter_arg_t *farg)
{
  SNetDestroyTypeEncoding( farg->in_type);
  if( farg->type_list != NULL) {
    SNetTencDestroyTypeEncodingList( farg->type_list);
  }

  if( farg->instr_lst != NULL) {
    int i;
    snet_filter_instr_list_t *list;
    snet_filter_instr_t *instr;

    for (i = 0; i < SNetElistGetNumExpressions( farg->guard_list); i++) {
      LIST_FOR_EACH(farg->instr_lst[i], list)
      LIST_FOR_EACH(list, instr)
          SNetDestroyFilterInstruction(instr);
      END_FOR

      SNetfilter_instrListDestroy(list);
      END_FOR
      SNetfilter_instr_listListDestroy(farg->instr_lst[i]);
    }
    SNetMemFree(farg->instr_lst);
  }

  SNetEdestroyList( farg->guard_list);
  SNetMemFree( farg);
}


/**
 * Filter task
 */
static void FilterTask( snet_entity_t *self, void *arg)
{
  int i,j;
  bool done, terminate;
  filter_arg_t *farg = (filter_arg_t *)arg;
  snet_stream_desc_t *instream, *outstream;
  snet_record_t *in_rec;
  snet_typeencoding_t *out_type;
  snet_filter_instr_t *current_instr;
  snet_filter_instr_list_t *current_list;
  snet_filter_instr_list_list_t *current_list_list;

  done = false;
  terminate = false;

#ifdef FILTER_DEBUG
  SNetUtilDebugNotice("(CREATION FILTER)");
#endif
  instream  = SNetStreamOpen(self, farg->input, 'r');
  outstream = SNetStreamOpen(self, farg->output, 'w');

  /* MAIN LOOP */
  while (!terminate) {
    /* read from input stream */
    in_rec = SNetStreamRead( instream);
    done = false;

    switch (SNetRecGetDescriptor( in_rec)) {
      case REC_data:
        for( i=0; i<SNetElistGetNumExpressions( farg->guard_list); i++) {
          if( ( SNetEevaluateBool( SNetEgetExpr( farg->guard_list, i), in_rec)) 
              && !( done)) {
            snet_record_t *out_rec = NULL;
            done = true;
            out_type = SNetTencGetTypeEncoding( farg->type_list, i);
            current_list_list = farg->instr_lst[i];
            for( j=0; j<SNetTencGetNumVariants( out_type); j++) {
              out_rec = SNetRecCreate( REC_data);
              SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( in_rec));
              SNetRecSetDataMode( out_rec, SNetRecGetDataMode( in_rec));

              current_list = SNetfilter_instr_listListGet( current_list_list, j);
              LIST_FOR_EACH(current_list, current_instr)
                switch (current_instr->opcode) {
                  case snet_tag:
                    SNetRecSetTag( out_rec, current_instr->name,
                        SNetEevaluateInt( current_instr->expr, in_rec));
                    break;
                  case snet_btag:
                    SNetRecSetBTag( out_rec, current_instr->name,
                        SNetEevaluateInt( current_instr->expr, in_rec));
                    break;
                  case snet_field:
                    SNetRecSetField(out_rec, current_instr->newName,
                        SNetRecGetField(in_rec, current_instr->name));
                    break;
                  case create_record: //noop
                    break;
                  default:
                    SNetUtilDebugFatal("[Filter] Unknown opcode in filter"
                                       " instruction [%d]\n\n",
                                       current_instr->opcode);
                }
              END_FOR

              out_rec = FilterInheritFromInrec( farg->in_type, in_rec, out_rec);
#ifdef DEBUG_FILTER
              SNetUtilDebugNotice("FILTER %x: outputting %x",
                  (unsigned int) outstream, (unsigned int) out_rec);
#endif
              SNetStreamWrite( outstream, out_rec);
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
          snet_stream_t *newstream = SNetRecGetStream( in_rec);
          SNetStreamReplace( instream, newstream);
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

      case REC_sort_end:
        /* forward sort record */
        SNetStreamWrite( outstream, in_rec);
        break;

      case REC_terminate:
        SNetStreamWrite( outstream, in_rec);
        terminate = true;
        break;

      default:
        SNetUtilDebugNotice("[Filter] Unknown control record destroyed (%d).\n", SNetRecGetDescriptor( in_rec));
        SNetRecDestroy(in_rec);
    }
  } /* MAIN LOOP END */

  SNetStreamClose( outstream, false);
  SNetStreamClose( instream, true);

  FilterArgDestroy( farg);
}

/**
 * Filter creation function
 */
snet_stream_t* SNetFilter( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guard_exprs, ... )
{
  int i;
  int num_outtypes;
  filter_arg_t *farg;
  snet_stream_t *outstream;
  snet_filter_instr_list_list_t *lst;
  snet_filter_instr_list_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  va_list args;

  instream = SNetRouteUpdate(info, instream, location);
  if(location == SNetNodeLocation) {
    outstream = SNetStreamCreate(0);

    if (guard_exprs == NULL) {
      guard_exprs = SNetEcreateList( 1, SNetEconstb( true));
    }

    num_outtypes = SNetElistGetNumExpressions( guard_exprs);
    instr_list = SNetMemAlloc( num_outtypes * sizeof( snet_filter_instr_list_list_t*));

    va_start( args, guard_exprs);
    for( i=0; i<num_outtypes; i++) {
      lst = va_arg( args, snet_filter_instr_list_list_t*);
      instr_list[i] = lst == NULL ? SNetfilter_instr_listListCreate( 0, NULL) : lst;
    }
    va_end( args);

    out_types = FilterComputeTypes( num_outtypes, instr_list);

    farg = (filter_arg_t *) SNetMemAlloc( sizeof( filter_arg_t));
    farg->input  = instream;
    farg->output = outstream;
    farg->in_type = in_type;
    farg->type_list = out_types;
    farg->guard_list = guard_exprs;
    farg->instr_lst = instr_list;

    SNetEntitySpawn( ENTITY_FILTER, FilterTask, (void*)farg);
  } else {
    SNetDestroyTypeEncoding(in_type);
    num_outtypes = SNetElistGetNumExpressions( guard_exprs);
    if(num_outtypes == 0) {
      num_outtypes += 1;
    }
    va_start( args, guard_exprs);
    for (i=0; i<num_outtypes; i++) {
      lst = va_arg( args, snet_filter_instr_list_list_t*);
      if (lst != NULL) {
        snet_filter_instr_list_t *tmp;
        snet_filter_instr_t *instr;
        LIST_FOR_EACH(lst, tmp)
          LIST_FOR_EACH(tmp, instr)
            SNetDestroyFilterInstruction(instr);
          END_FOR

          SNetfilter_instrListDestroy(tmp);
        END_FOR
        SNetfilter_instr_listListDestroy(lst);
      }
    }
    va_end( args);

    SNetEdestroyList( guard_exprs);
    outstream = instream;
  }

  return outstream;
}


/**
 * Translate creation function
 */
snet_stream_t* SNetTranslate( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_typeencoding_t *in_type,
    snet_expr_list_t *guard_exprs, ... )
{
  int i;
  int num_outtypes;
  filter_arg_t *farg;
  snet_stream_t *outstream;
  snet_filter_instr_list_list_t **instr_list;
  snet_typeencoding_list_t *out_types;
  va_list args;

  instream = SNetRouteUpdate(info, instream, location);
  if(location == SNetNodeLocation) {
    outstream = SNetStreamCreate(0);

    if (guard_exprs == NULL) {
      guard_exprs = SNetEcreateList( 1, SNetEconstb( true));
    }

    num_outtypes = SNetElistGetNumExpressions( guard_exprs);
    instr_list = SNetMemAlloc( num_outtypes * sizeof( snet_filter_instr_list_list_t*));

    va_start( args, guard_exprs);
    for( i=0; i<num_outtypes; i++) {
      instr_list[i] = va_arg( args, snet_filter_instr_list_list_t*);
    }
    va_end( args);

    out_types = FilterComputeTypes( num_outtypes, instr_list);

    farg = (filter_arg_t *) SNetMemAlloc( sizeof( filter_arg_t));
    farg->input  = instream;
    farg->output = outstream;
    farg->in_type = in_type;
    farg->type_list = out_types;
    farg->guard_list = guard_exprs;
    farg->instr_lst = instr_list;

    SNetEntitySpawn( ENTITY_FILTER, FilterTask, (void*)farg );

  } else {
    snet_filter_instr_list_list_t *lst;
    SNetDestroyTypeEncoding(in_type);
    num_outtypes = SNetElistGetNumExpressions( guard_exprs);
    if(num_outtypes == 0) {
      num_outtypes += 1;
    }

    va_start( args, guard_exprs);
    for( i=0; i<num_outtypes; i++) {
      lst = va_arg( args, snet_filter_instr_list_list_t*);
      if (lst != NULL) {
        snet_filter_instr_list_t *tmp;
        snet_filter_instr_t *instr;
        LIST_FOR_EACH(lst, tmp)
          LIST_FOR_EACH(tmp, instr)
            SNetDestroyFilterInstruction(instr);
          END_FOR

          SNetfilter_instrListDestroy(tmp);
        END_FOR
        SNetfilter_instr_listListDestroy(lst);
      }
    }
    va_end( args);

    SNetEdestroyList( guard_exprs);
    outstream = instream;
  }

  return outstream;
}


/**
 * Nameshift task
 */
static void NameshiftTask( snet_entity_t *self, void *arg)
{
  bool terminate = false;
  filter_arg_t *farg = (filter_arg_t *)arg;
  snet_stream_desc_t *outstream, *instream;
  snet_variantencoding_t *untouched;
  snet_record_t *rec;
  int name, offset, val;
  snet_ref_t *ref;

  instream  = SNetStreamOpen(self, farg->input, 'r');
  outstream = SNetStreamOpen(self, farg->output, 'w');

  untouched = SNetTencGetVariant( farg->in_type, 1);

  // Guards are misused for offset
  offset = SNetEevaluateInt( SNetEgetExpr( farg->guard_list, 0), NULL);

  /* MAIN LOOP */
  while (!terminate) {
    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch (SNetRecGetDescriptor( rec)) {
      case REC_data:
        FOR_EACH_FIELD(rec, name, ref)
          if (!SNetTencContainsFieldName(untouched, name)) {
            SNetRecRenameField( rec, name, name + offset);
          }
        END_FOR

        FOR_EACH_TAG(rec, name, val)
          if (!SNetTencContainsTagName(untouched, name)) {
            SNetRecRenameTag( rec, name, name + offset);
          }
        END_FOR

        FOR_EACH_BTAG(rec, name, val)
          if (!SNetTencContainsBTagName(untouched, name)) {
            SNetRecRenameBTag( rec, name, name + offset);
          }
        END_FOR

        SNetStreamWrite( outstream, rec);
        break;

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream(rec);
          SNetStreamReplace( instream, newstream);
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

      case REC_sort_end:
        /* forward sort record */
        SNetStreamWrite( outstream, rec);
        break;

      case REC_terminate:
        SNetStreamWrite( outstream, rec);
        terminate = true;
        break;

      default:
        SNetUtilDebugNotice("[Filter] Unknown control rec destroyed (%d).\n",
            SNetRecGetDescriptor( rec));
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  SNetStreamClose( instream, true);
  SNetStreamClose( outstream, false);

  FilterArgDestroy( farg);
}


/**
 * Nameshift creation function
 */
snet_stream_t *SNetNameShift( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    int offset,
    snet_variantencoding_t *untouched)
{
  snet_stream_t *outstream;
  filter_arg_t *farg;

  if(location == SNetNodeLocation) {
    outstream = SNetStreamCreate(0);

    farg = (filter_arg_t *) SNetMemAlloc( sizeof( filter_arg_t));
    farg->input  = instream;
    farg->output = outstream;
    farg->in_type = SNetTencTypeEncode( 1, untouched);
    farg->type_list = NULL; /* out_types */
    farg->guard_list = SNetEcreateList( 1, SNetEconsti( offset));
    farg->instr_lst = NULL; /* instructions */
    
    SNetEntitySpawn( ENTITY_FILTER, NameshiftTask, (void*)farg);
  
  } else {
    SNetTencDestroyVariantEncoding( untouched);
    outstream = instream;
  }

  return( outstream);
}
