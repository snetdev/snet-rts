
#include <stdarg.h>
#include <assert.h>

#include "expression.h"
#include "filter.h"
#include "list.h"
#include "memfun.h"
#include "bool.h"
#include "record.h"
#include "moninfo.h"
#include "locvec.h"
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

/**
 * Argument for filter tasks
 */
typedef struct {
  snet_stream_desc_t *instream, *outstream;
  snet_variant_t *input_variant;
  snet_expr_list_t *guard_exprs;
  snet_filter_instr_list_list_t **filter_instructions;
} filter_arg_t;

static void FilterArgsDestroy( filter_arg_t *farg)
{
  SNetVariantDestroy( farg->input_variant);

  if (farg->filter_instructions != NULL) {
    int i;
    snet_expr_t *expr;
    (void) expr;

    LIST_ENUMERATE(farg->guard_exprs, i, expr) {
      SNetFilterInstrListListDestroy( farg->filter_instructions[i]);
    }

    SNetMemFree( farg->filter_instructions);
  }

  SNetExprListDestroy( farg->guard_exprs);
  SNetMemFree( farg);
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
 * Filter task
 */
static void FilterTask(void *arg)
{
  filter_arg_t *farg = arg;
  snet_expr_t *expr;
  snet_record_t *in_rec = NULL, *out_rec = NULL;
  snet_filter_instr_t *instr;
  snet_filter_instr_list_t *instr_list;
  bool done;
  int i;

  assert( farg->guard_exprs != NULL &&
      SNetExprListLength(farg->guard_exprs) > 0 );

  /* read from input stream */
  in_rec = SNetStreamRead( farg->instream);

  switch (SNetRecGetDescriptor( in_rec)) {
    case REC_data:
      {
        done = false;

#ifdef USE_USER_EVENT_LOGGING
        /* Emit a monitoring message of a record read to be processed by a filter */
        SNetThreadingEventSignal(
            SNetMonInfoCreate( EV_MESSAGE_IN, MON_RECORD, in_rec));
#endif

        LIST_ENUMERATE( farg->guard_exprs, i, expr) {
          if (SNetEevaluateBool( expr, in_rec) && !done) {
            done = true;

            LIST_FOR_EACH(farg->filter_instructions[i], instr_list) {
              out_rec = SNetRecCreate( REC_data);
              SNetRecAddAsParent( out_rec, in_rec );
              SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( in_rec));
              SNetRecSetDataMode( out_rec, SNetRecGetDataMode( in_rec));

              LIST_FOR_EACH(instr_list, instr) {
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
              }

              SNetRecFlowInherit( farg->input_variant, in_rec, out_rec);

#ifdef USE_USER_EVENT_LOGGING
              /* Emit a monitoring message of a record write by a filter started */
              SNetThreadingEventSignal(
                  SNetMonInfoCreate( EV_MESSAGE_OUT, MON_RECORD, out_rec));
#endif
                SNetStreamWrite( farg->outstream, out_rec);
            } /* forall instruction lists */
          } /* if a guard is true first time */
        }

        SNetRecDestroy( in_rec);
        assert(done);
      }
      break; /* case REC_data */

    case REC_sync:
      SNetStreamReplace( farg->instream, SNetRecGetStream( in_rec));
      SNetRecDestroy( in_rec);
      break;

    case REC_terminate:
      /* forward record */
      SNetStreamWrite( farg->outstream, in_rec);
      SNetStreamClose( farg->outstream, false);
      SNetStreamClose( farg->instream, true);

      FilterArgsDestroy( farg);
      return;
    case REC_sort_end:
      /* forward record */
      SNetStreamWrite( farg->outstream, in_rec);
      break;

    case REC_collect:
    default:
      assert(0);
      SNetRecDestroy( in_rec);
      break;
  }

  SNetThreadingRespawn(NULL);
}

/**
 * Nameshift task
 */
static void NameshiftTask(void *arg)
{
  filter_arg_t *farg = arg;
  snet_variant_t *untouched = farg->input_variant;
  snet_record_t *rec;
  int name, offset, val;
  snet_ref_t *field;
  (void) val;
  (void) field;


  /* Guards are misused for offset */
  offset = SNetEevaluateInt( SNetExprListGet( farg->guard_exprs, 0), NULL);

  /* read from input stream */
  rec = SNetStreamRead( farg->instream);

  switch (SNetRecGetDescriptor( rec)) {
    case REC_data:
      RECORD_FOR_EACH_FIELD(rec, name, field) {
        if (!SNetVariantHasField(untouched, name)) {
          SNetRecRenameField( rec, name, name + offset);
        }
      }

      RECORD_FOR_EACH_TAG(rec, name, val) {
        if (!SNetVariantHasTag(untouched, name)) {
          SNetRecRenameTag( rec, name, name + offset);
        }
      }

      RECORD_FOR_EACH_BTAG(rec, name, val) {
        if (!SNetVariantHasBTag(untouched, name)) {
          SNetRecRenameBTag( rec, name, name + offset);
        }
      }

      SNetStreamWrite( farg->outstream, rec);
      break;

    case REC_sync:
      SNetStreamReplace( farg->instream, SNetRecGetStream(rec));
      SNetRecDestroy( rec);
      break;

    case REC_terminate:
      /* forward record */
      SNetStreamWrite( farg->outstream, rec);
      SNetStreamClose( farg->instream, true);
      SNetStreamClose( farg->outstream, false);

      FilterArgsDestroy( farg);
      return;
    case REC_sort_end:
      /* forward record */
      SNetStreamWrite( farg->outstream, rec);
      break;

    case REC_collect:
    default:
      assert(0);
      SNetRecDestroy( rec);
      break;
  }

  SNetThreadingRespawn(NULL);
}

/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/


/**
 * Convenience function for creating filter and translate
 */
static snet_stream_t* CreateFilter( snet_stream_t *instream,
    snet_info_t *info,
    int location,
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs,
    snet_filter_instr_list_list_t **instr_list,
    const char *name
    )
{
  snet_stream_t *outstream;
  filter_arg_t *farg;

  /* Check for bypass
   * - if it is a bypass, exit out early and do not create any component
   */
  instream = SNetRouteUpdate(info, instream, location);
  if(SNetDistribIsNodeLocation(location) &&
      !FilterIsBypass(input_variant, guard_exprs, instr_list)) {
    outstream = SNetStreamCreate(0);

    farg = (filter_arg_t *) SNetMemAlloc( sizeof( filter_arg_t));
    farg->instream  = SNetStreamOpen(instream, 'r');
    farg->outstream = SNetStreamOpen(outstream, 'w');
    farg->input_variant = input_variant;
    farg->guard_exprs = guard_exprs;
    farg->filter_instructions = instr_list;

    SNetThreadingSpawn( ENTITY_filter, location, SNetLocvecGet(info),
          name, &FilterTask, farg);
  } else {
    int i;
    snet_expr_t *expr;
    (void) expr;

    SNetVariantDestroy(input_variant);
    LIST_ENUMERATE(guard_exprs, i, expr) {
      SNetFilterInstrListListDestroy( instr_list[i]);
    }

    SNetMemFree( instr_list);
    SNetExprListDestroy(guard_exprs);
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

  num_outtypes = SNetExprListLength( guard_exprs);

  /* read in the filter instructions from varargs */
  BUILD_INSTR_FROM_VARARG(instr_list, num_outtypes, guard_exprs);

  return CreateFilter(instream, info, location, input_variant,
      guard_exprs, instr_list, "<filter>");
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

  num_outtypes = SNetExprListLength( guard_exprs);

  /* read in the filter instructions from varargs */
  BUILD_INSTR_FROM_VARARG(instr_list, num_outtypes, guard_exprs);

  return CreateFilter(instream, info, location, input_variant,
      guard_exprs, instr_list, "<translate>");
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

  instream = SNetRouteUpdate(info, instream, location);
  if(SNetDistribIsNodeLocation(location)) {
    outstream = SNetStreamCreate(0);

    farg = (filter_arg_t *) SNetMemAlloc( sizeof( filter_arg_t));
    farg->instream  = SNetStreamOpen(instream, 'r');
    farg->outstream = SNetStreamOpen(outstream, 'w');
    farg->input_variant = untouched;
    farg->guard_exprs = SNetExprListCreate( 1, SNetEconsti( offset));
    farg->filter_instructions = NULL; /* instructions */

    SNetThreadingSpawn( ENTITY_nameshift, location, SNetLocvecGet(info),
          "<nameshift>", &NameshiftTask, farg);
  } else {
    SNetVariantDestroy( untouched);
    outstream = instream;
  }

  return outstream;
}
