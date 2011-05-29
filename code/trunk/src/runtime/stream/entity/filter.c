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

/*****************************************************************************/
/* FILTER INSTRUCTIONS                                                       */
/*****************************************************************************/


struct filter_instr {
  snet_filter_opcode_t opcode;
  int name, newName;
  snet_expr_t *expr;
};



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
/* HELPER FUNCTIONS                                                          */
/*****************************************************************************/


static void FilterArgsDestroy( snet_variant_t *input_variant,
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



/**
 * Check from the creation function parameters
 * if the filter is a pure bypass, i.e. []
 * TODO also consider [ A -> A ] as bypass
 */
static bool FilterIsBypass(
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs,
    snet_filter_instr_list_list_t **instr_lists)
{
  snet_filter_instr_list_t *instr_list;


  /*
   * TODO
   * Guards: only true (single guard) => only instr_lists[0]
   * Length of instr_lists[0] == 1 (i.e. single output record)
   */
  if ( !SNetVariantIsEmpty(input_variant) ||
      SNetExprListLength( guard_exprs) != 1 ||
      SNetFilterInstrListListLength(instr_lists[0]) != 1) {
    return false;
  }

  /* TODO
   * - instr_list must resemble creation of a record identical to the
   *   input record (input_variant)
   * - instructions for fields/(b)tags of form x = x
   */
  instr_list = SNetFilterInstrListListGet(instr_lists[0], 0);
  if ( SNetFilterInstrListLength(instr_list) != 1 ||
      (SNetFilterInstrListGet(instr_list, 0))->opcode != create_record) {
    return false;
  }

  return true;
}



/*****************************************************************************/
/* FILTER TASKS                                                              */
/*****************************************************************************/

/**
 * Argument for filter tasks
 */
typedef struct {
  snet_stream_t *input, *output;
  snet_variant_t *input_variant;
  snet_expr_list_t *guard_exprs;
  snet_filter_instr_list_list_t **filter_instructions;
} filter_arg_t;


/**
 * Filter task
 */
static void FilterTask(void *arg)
{
  filter_arg_t *farg = (filter_arg_t *)arg;
  snet_expr_t *expr;
  snet_record_t *in_rec;
  snet_filter_instr_t *instr;
  snet_filter_instr_list_t *instr_list;
  snet_stream_desc_t *instream, *outstream;
  bool terminate = false;
  int i;

  assert( farg->guard_exprs != NULL &&
      SNetExprListLength(farg->guard_exprs) > 0 );

  instream  = SNetStreamOpen(farg->input, 'r');
  outstream = SNetStreamOpen(farg->output, 'w');

  /* MAIN LOOP */
  while (!terminate) {
    /* read from input stream */
    in_rec = SNetStreamRead( instream);

    switch (SNetRecGetDescriptor( in_rec)) {
      case REC_data:
        {
          bool done = false;
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
                    case create_record: /* NOP */
                      break;
                    default: assert(0);
                  }
                END_FOR

                SNetRecFlowInherit( farg->input_variant, in_rec, out_rec);
                SNetStreamWrite( outstream, out_rec);
              END_FOR /* forall instruction lists */
            } /* if a guard is true first time */
          END_ENUMERATE

          SNetRecDestroy( in_rec);
          assert(done);
        }
        break; /* case REC_data */

      case REC_sync:
        {
          snet_stream_t *newstream = SNetRecGetStream( in_rec);
          SNetStreamReplace( instream, newstream);
          SNetRecDestroy( in_rec);
        }
        break;

      case REC_terminate:
        terminate = true;
      case REC_sort_end:
        /* forward record */
        SNetStreamWrite( outstream, in_rec);
        break;

      case REC_source:
        /* ignore, destroy */
        SNetRecDestroy( in_rec);
        break;

      case REC_collect:
      default:
        assert(0);
        SNetRecDestroy( in_rec);
        break;
    }
  } /* MAIN LOOP END */

  SNetStreamClose( outstream, false);
  SNetStreamClose( instream, true);

  FilterArgsDestroy(
      farg->input_variant,
      farg->guard_exprs,
      farg->filter_instructions
      );
  SNetMemFree( farg);
}




/**
 * Nameshift task
 */
static void NameshiftTask(void *arg)
{
  filter_arg_t *farg = (filter_arg_t *)arg;
  snet_stream_desc_t *outstream, *instream;
  snet_variant_t *untouched = farg->input_variant;
  snet_record_t *rec;
  bool terminate = false;
  int name, offset, val;
  snet_ref_t *ref;

  instream  = SNetStreamOpen(farg->input, 'r');
  outstream = SNetStreamOpen(farg->output, 'w');

  /* Guards are misused for offset */
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

      case REC_terminate:
        terminate = true;
      case REC_sort_end:
        /* forward record */
        SNetStreamWrite( outstream, rec);
        break;

      case REC_source:
        /* ignore, destroy */
        SNetRecDestroy( rec);
        break;

      case REC_collect:
      default:
        assert(0);
        SNetRecDestroy( rec);
        break;
    }
  } /* MAIN LOOP END */

  SNetStreamClose( instream, true);
  SNetStreamClose( outstream, false);

  FilterArgsDestroy(
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

  /* Check for bypass
   * - if it is a bypass, exit out early and do not create any component
   */
  if (FilterIsBypass(input_variant, guard_exprs, instr_list)) {
    FilterArgsDestroy(input_variant, guard_exprs, instr_list);
    return instream;
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
    FilterArgsDestroy(input_variant, guard_exprs, instr_list);
    outstream = instream;
  }

  return outstream;
}

/**
 * Macro for creating a instr_list from the function varargs;
 * param 'last' refers to the last parameter before the varargs ...
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
