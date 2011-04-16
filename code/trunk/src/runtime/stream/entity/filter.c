#include "filter.h"
#include "list.h"
#include "memfun.h"
#include "stdarg.h"
#include "bool.h"
#include "record.h"
#include "snetentities.h"
#include "debug.h"
#include "interface_functions.h"
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

// pass at least one set!! -> lst must not be NULL!
static snet_variant_list_list_t *FilterComputeTypes(
    int num, snet_filter_instr_list_list_t **lists)
{
  int i;
  snet_variant_list_list_t *result = SNetvariant_listListCreate(0, NULL);
  snet_filter_instr_list_t *current_list;
  snet_filter_instr_t *current_instr;
  snet_variant_t *variant;

  for (i = 0; i < num; i++) {
    snet_variant_list_t *list = SNetvariantListCreate(0, NULL);
    LIST_FOR_EACH(lists[i], current_list)
      variant = SNetVariantCreateEmpty();
      LIST_FOR_EACH(current_list, current_instr)
        switch (current_instr->opcode) {
          case snet_tag:
            SNetVariantAddTag(variant, current_instr->name);
            break;
          case snet_btag:
            SNetVariantAddBTag(variant, current_instr->name);
            break;
          case snet_field:
            SNetVariantAddField(variant, current_instr->newName);
            break;
          case create_record: //NOOP
            break;
          default:
            SNetUtilDebugFatal("[Filter] Unknown opcode in filter instruction"
                                " [%d]\n\n",
                              current_instr->opcode);
        }
      END_FOR
      SNetvariantListAppend(list, variant);
    END_FOR
    SNetvariant_listListAppend(result, list);
  }


  return result;
}

/*****************************************************************************/

typedef struct {
  snet_stream_t *input, *output;
  snet_expr_list_t *guard_list;
  snet_variant_t *in_variant;
  snet_variant_list_list_t *variant_list_list;
  snet_filter_instr_list_list_t **instr_lst;
} filter_arg_t;

static void FilterArgDestroy( filter_arg_t *farg)
{
  snet_variant_t *variant;
  snet_variant_list_t *variant_list;
  SNetVariantDestroy(farg->in_variant);

  if (farg->variant_list_list != NULL) {
    LIST_FOR_EACH(farg->variant_list_list, variant_list)
      LIST_FOR_EACH(variant_list, variant)
        SNetVariantDestroy(variant);
      END_FOR

      SNetvariantListDestroy(variant_list);
    END_FOR

    SNetvariant_listListDestroy( farg->variant_list_list);
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
  int i;
  bool done, terminate;
  filter_arg_t *farg = (filter_arg_t *)arg;
  snet_stream_desc_t *instream, *outstream;
  snet_record_t *in_rec;
  snet_variant_t *variant;
  snet_variant_list_t *out_type;
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
            out_type = SNetvariant_listListGet( farg->variant_list_list, i);
            current_list_list = farg->instr_lst[i];
            int j = 0;
            LIST_FOR_EACH(out_type, variant)
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

              SNetRecFlowInherit( farg->in_variant, in_rec, out_rec);
#ifdef DEBUG_FILTER
              SNetUtilDebugNotice("FILTER %x: outputting %x",
                  (unsigned int) outstream, (unsigned int) out_rec);
#endif
              SNetStreamWrite( outstream, out_rec);
              j++;
            END_FOR // forall variants of selected out_type
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
    snet_variant_t *in_variant,
    snet_expr_list_t *guard_exprs, ... )
{
  int i;
  int num_outtypes;
  filter_arg_t *farg;
  snet_stream_t *outstream;
  snet_filter_instr_list_list_t *lst;
  snet_filter_instr_list_list_t **instr_list;
  snet_variant_list_list_t *out_types;
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
    farg->in_variant = in_variant;
    farg->variant_list_list = out_types;
    farg->guard_list = guard_exprs;
    farg->instr_lst = instr_list;

    SNetEntitySpawn( ENTITY_FILTER, FilterTask, (void*)farg);
  } else {
    SNetVariantDestroy(in_variant);
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
    snet_variant_t *in_variant,
    snet_expr_list_t *guard_exprs, ... )
{
  int i;
  int num_outtypes;
  filter_arg_t *farg;
  snet_stream_t *outstream;
  snet_filter_instr_list_list_t **instr_list;
  snet_variant_list_list_t *out_types;
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
    farg->in_variant = in_variant;
    farg->variant_list_list = out_types;
    farg->guard_list = guard_exprs;
    farg->instr_lst = instr_list;

    SNetEntitySpawn( ENTITY_FILTER, FilterTask, (void*)farg );

  } else {
    snet_filter_instr_list_list_t *lst;
    SNetVariantDestroy(in_variant);
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
  snet_variant_t *untouched = farg->in_variant;
  snet_record_t *rec;
  int name, offset, val;
  snet_ref_t *ref;

  instream  = SNetStreamOpen(self, farg->input, 'r');
  outstream = SNetStreamOpen(self, farg->output, 'w');

  // Guards are misused for offset
  offset = SNetEevaluateInt( SNetEgetExpr( farg->guard_list, 0), NULL);

  /* MAIN LOOP */
  while (!terminate) {
    /* read from input stream */
    rec = SNetStreamRead( instream);

    switch (SNetRecGetDescriptor( rec)) {
      case REC_data:
        FOR_EACH_FIELD(rec, name, ref)
          if (!SNetVariantHasField(untouched, name)) {
            SNetRecRenameField( rec, name, name + offset);
          }
        END_FOR

        FOR_EACH_TAG(rec, name, val)
          if (!SNetVariantHasTag(untouched, name)) {
            SNetRecRenameTag( rec, name, name + offset);
          }
        END_FOR

        FOR_EACH_BTAG(rec, name, val)
          if (!SNetVariantHasBTag(untouched, name)) {
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
    snet_variant_t *untouched)
{
  snet_stream_t *outstream;
  filter_arg_t *farg;

  if(location == SNetNodeLocation) {
    outstream = SNetStreamCreate(0);

    farg = (filter_arg_t *) SNetMemAlloc( sizeof( filter_arg_t));
    farg->input  = instream;
    farg->output = outstream;
    farg->in_variant = untouched;
    farg->variant_list_list = NULL; /* out_types */
    farg->guard_list = SNetEcreateList( 1, SNetEconsti( offset));
    farg->instr_lst = NULL; /* instructions */
    
    SNetEntitySpawn( ENTITY_FILTER, NameshiftTask, (void*)farg);
  
  } else {
    SNetVariantDestroy( untouched);
    outstream = instream;
  }

  return( outstream);
}
