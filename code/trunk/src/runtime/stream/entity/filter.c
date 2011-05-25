#include "expression.h"
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
    SNetExprDestroy(instr->expr);
  }

  SNetMemFree( instr);
}

/*****************************************************************************/

typedef struct {
  snet_stream_t *input, *output;
  snet_variant_t *input_variant;
  snet_expr_list_t *guard_exprs;
  snet_filter_instr_list_list_t **filter_instructions;
} filter_arg_t;

static void FilterArgDestroy( snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs,
    snet_filter_instr_list_list_t **filter_instructions)
{
  SNetVariantDestroy(input_variant);

  if (filter_instructions != NULL) {
    int i;
    snet_expr_t *expr;

    LIST_ENUMERATE(guard_exprs, expr, i)
      SNetFilterInstrListListDestroy(filter_instructions[i]);
    END_ENUMERATE

    SNetMemFree(filter_instructions);
  }

  SNetExprListDestroy( guard_exprs);
}


static bool FilterIsBypass(
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs,
    snet_filter_instr_list_list_t **instr_lists)
{
  snet_filter_instr_list_t *instr_list;

  if ( !SNetVariantIsEmpty(input_variant) ||
      SNetExprListLength( guard_exprs) != 1 ||
      SNetFilterInstrListListLength(instr_lists[0]) != 1) {
    return false;
  }

  instr_list = SNetFilterInstrListListGet(instr_lists[0], 0);
  if ( SNetFilterInstrListLength(instr_list) != 1 ||
      (SNetFilterInstrListGet(instr_list, 0))->opcode != create_record) {
    return false;
  }

  return true;
}


/**
 * Filter task
 */
static void FilterTask(void *arg)
{
  bool done = false, terminate = false;
  filter_arg_t *farg = (filter_arg_t *)arg;

  int i;
  snet_expr_t *expr;
  snet_record_t *in_rec;
  snet_filter_instr_t *instr;
  snet_filter_instr_list_t *instr_list;
  snet_stream_desc_t *instream, *outstream;

#ifdef FILTER_DEBUG
  SNetUtilDebugNotice("(CREATION FILTER)");
#endif
  instream  = SNetStreamOpen(farg->input, 'r');
  outstream = SNetStreamOpen(farg->output, 'w');

  /* MAIN LOOP */
  while (!terminate) {
    /* read from input stream */
    in_rec = SNetStreamRead( instream);
    done = false;

    switch (SNetRecGetDescriptor( in_rec)) {
      case REC_data:
        LIST_ENUMERATE( farg->guard_exprs, expr, i)
          if (SNetEevaluateBool( expr, in_rec) && !done) {
            snet_record_t *out_rec = NULL;
            done = true;

            LIST_FOR_EACH(farg->filter_instructions[i], instr_list)
              out_rec = SNetRecCreate( REC_data);
              SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( in_rec));
              SNetRecSetDataMode( out_rec, SNetRecGetDataMode( in_rec));

              LIST_FOR_EACH(instr_list, instr)
                switch (instr->opcode) {
                  case snet_tag:
                    SNetRecSetTag( out_rec, instr->name,
                        SNetEevaluateInt( instr->expr, in_rec));
                    break;
                  case snet_btag:
                    SNetRecSetBTag( out_rec, instr->name,
                        SNetEevaluateInt( instr->expr, in_rec));
                    break;
                  case snet_field:
                    SNetRecSetField(out_rec, instr->newName,
                        SNetRecGetField(in_rec, instr->name));
                    break;
                  case create_record: //noop
                    break;
                  default:
                    SNetUtilDebugFatal("[Filter] Unknown opcode in filter"
                                       " instruction [%d]\n\n",
                                       instr->opcode);
                }
              END_FOR

              SNetRecFlowInherit( farg->input_variant, in_rec, out_rec);
#ifdef DEBUG_FILTER
              SNetUtilDebugNotice("FILTER %x: outputting %x",
                                  (unsigned int) outstream,
                                  (unsigned int) out_rec);
#endif
              SNetStreamWrite( outstream, out_rec);
            END_FOR // forall instruction lists
          } // if guard is true
        END_ENUMERATE

        SNetRecDestroy( in_rec);
        if (!done) {
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

      case REC_source:
        /* ignore, destroy */
        SNetRecDestroy( in_rec);
        break;

      default:
        SNetUtilDebugNotice("[Filter] Unknown control record destroyed (%d).\n",
                            SNetRecGetDescriptor( in_rec));
        SNetRecDestroy(in_rec);
    }
  } /* MAIN LOOP END */

  SNetStreamClose( outstream, false);
  SNetStreamClose( instream, true);

  FilterArgDestroy(
      farg->input_variant,
      farg->guard_exprs,
      farg->filter_instructions
      );
  SNetMemFree( farg);
}


/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/


/**
 * Convenience function for creating filter and translate
 */
snet_stream_t* CreateFilter( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs,
    snet_filter_instr_list_list_t **instr_list
    )
{
  snet_stream_t *outstream;
  filter_arg_t *farg;

  /* check for bypass */
  if (FilterIsBypass(input_variant, guard_exprs, instr_list)) {
    outstream = instream;
    FilterArgDestroy(input_variant, guard_exprs, instr_list);
    return outstream;
  }

  instream = SNetRouteUpdate(info, instream, location);
  if (location == SNetNodeLocation) {
    outstream = SNetStreamCreate(0);

    farg = (filter_arg_t *) SNetMemAlloc( sizeof( filter_arg_t));
    farg->input  = instream;
    farg->output = outstream;
    farg->input_variant = input_variant;
    farg->guard_exprs = guard_exprs;
    farg->filter_instructions = instr_list;

    SNetEntitySpawn( ENTITY_FILTER, FilterTask, (void*)farg);
  } else {
    FilterArgDestroy(input_variant, guard_exprs, instr_list);
    outstream = instream;
  }

  return outstream;
}

/**
 * Macro for creating a instr_list from the function varargs
 * last refers to the last parameter before the varargs ...
 */
#define BUILD_INSTR_FROM_VARARG(instr_list, num, last) do {\
  va_list args; \
  int i; \
  (instr_list) = SNetMemAlloc( (num) * sizeof( snet_filter_instr_list_list_t*)); \
  va_start( args, (last)); \
  for (i = 0; i < (num); i++) { \
    (instr_list)[i] = va_arg( args, snet_filter_instr_list_list_t*); \
    if ((instr_list)[i] == NULL) { \
      (instr_list)[i] = SNetFilterInstrListListCreate(0); \
    } \
  } \
  va_end( args); \
} while(0);



/**
 * Filter creation function
 */
snet_stream_t* SNetFilter( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs, ...)
{
  int num_outtypes;
  snet_filter_instr_list_list_t **instr_list;

  assert(input_variant != NULL);
  assert(guard_exprs != NULL);

  //FIXME remove following
  if (guard_exprs == NULL) {
    guard_exprs = SNetExprListCreate( 1, SNetEconstb( true));
  }
  num_outtypes = SNetExprListLength( guard_exprs);

  /* read in the filter instructions from varargs */
  BUILD_INSTR_FROM_VARARG(instr_list, num_outtypes, guard_exprs);

  return CreateFilter(instream, info, location, input_variant, guard_exprs, instr_list);
}


/**
 * Translate creation function
 */
snet_stream_t* SNetTranslate( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs, ...)
{
  int num_outtypes;
  snet_filter_instr_list_list_t **instr_list;

  assert(input_variant != NULL);
  assert(guard_exprs != NULL);

  //FIXME remove following
  if (guard_exprs == NULL) {
    guard_exprs = SNetExprListCreate( 1, SNetEconstb( true));
  }
  num_outtypes = SNetExprListLength( guard_exprs);

  /* read in the filter instructions from varargs */
  BUILD_INSTR_FROM_VARARG(instr_list, num_outtypes, guard_exprs);

  return CreateFilter(instream, info, location, input_variant, guard_exprs, instr_list);
}




/**
 * Nameshift task
 */
static void NameshiftTask(void *arg)
{
  bool terminate = false;
  filter_arg_t *farg = (filter_arg_t *)arg;
  snet_stream_desc_t *outstream, *instream;
  snet_variant_t *untouched = farg->input_variant;
  snet_record_t *rec;
  int name, offset, val;
  snet_ref_t *ref;

  instream  = SNetStreamOpen(farg->input, 'r');
  outstream = SNetStreamOpen(farg->output, 'w');

  // Guards are misused for offset
  offset = SNetEevaluateInt( SNetExprListGet( farg->guard_exprs, 0), NULL);

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

      case REC_source:
        /* ignore, destroy */
        SNetRecDestroy( rec);
        break;

      default:
        SNetUtilDebugNotice("[Filter] Unknown control rec destroyed (%d).\n",
                            SNetRecGetDescriptor( rec));
        SNetRecDestroy( rec);
    }
  } /* MAIN LOOP END */

  SNetStreamClose( instream, true);
  SNetStreamClose( outstream, false);

  FilterArgDestroy(
      farg->input_variant,
      farg->guard_exprs,
      farg->filter_instructions
      );
  SNetMemFree( farg);
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
    farg->input_variant = untouched;
    farg->guard_exprs = SNetExprListCreate( 1, SNetEconsti( offset));
    farg->filter_instructions = NULL; /* instructions */

    SNetEntitySpawn( ENTITY_FILTER, NameshiftTask, (void*)farg);
  } else {
    SNetVariantDestroy( untouched);
    outstream = instream;
  }

  return outstream;
}
