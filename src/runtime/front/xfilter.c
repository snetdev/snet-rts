#include <stdarg.h>
#include <stdio.h>
#include "node.h"

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

  trace(__func__);

  instr = SNetNew(snet_filter_instr_t);
  instr->opcode = opcode;
  instr->name = -1;
  instr->newName = -1;
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
  if (instr->expr != NULL) {
    SNetExprDestroy(instr->expr);
  }
  SNetMemFree( instr);
}


/*****************************************************************************/
/* HELPER FUNCTIONS                                                          */
/*****************************************************************************/

/*static*/ void FilterArgsDestroy( filter_arg_t *farg)
{
  SNetVariantDestroy( farg->input_variant);

  if (farg->filter_instructions != NULL) {
    int i;
    snet_expr_t *expr;

    LIST_ENUMERATE(farg->guard_exprs, i, expr) {
      SNetFilterInstrListListDestroy( farg->filter_instructions[i]);
      (void) expr; /* prevent compiler warnings */
    }

    SNetMemFree( farg->filter_instructions);
  }

  SNetExprListDestroy( farg->guard_exprs);
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

/* Apply filter to incoming data record. Return the last outgoing record. */
static snet_record_t *ApplyFilter(
    snet_record_t       *rec,
    const filter_arg_t  *farg,
    landing_siso_t      *land,
    snet_stream_desc_t  *desc)
{
  snet_expr_t           *expr;
  snet_record_t         *out_rec = NULL;
  snet_filter_instr_t   *instr;
  snet_filter_instr_list_t *instr_list;
  bool                   done = false;
  int                    i;

  LIST_ENUMERATE( farg->guard_exprs, i, expr) {
    if (SNetEevaluateBool( expr, rec) && !done) {
      done = true;

      LIST_FOR_EACH(farg->filter_instructions[i], instr_list) {
        if (out_rec != NULL) {
          SNetWrite(&land->outdesc, out_rec, false);
        }
        out_rec = SNetRecCreate( REC_data);
        SNetRecSetInterfaceId( out_rec, SNetRecGetInterfaceId( rec));
        SNetRecSetDataMode( out_rec, SNetRecGetDataMode( rec));
        SNetRecDetrefCopy(out_rec, rec);

        LIST_FOR_EACH(instr_list, instr) {
          switch (instr->opcode) {
            case snet_tag:
              SNetRecSetTag( out_rec, instr->name,
                  SNetEevaluateInt( instr->expr, rec));
              break;
            case snet_btag:
              SNetRecSetBTag( out_rec, instr->name,
                  SNetEevaluateInt( instr->expr, rec));
              break;
            case snet_field:
              SNetRecSetField(out_rec, instr->newName,
                  SNetRecGetField(rec, instr->name));
              break;
            case create_record: /* NOP */
              break;
            default: assert(0);
          }
        }

        SNetRecFlowInherit( farg->input_variant, rec, out_rec);
      } /* forall instruction lists */
    } /* if a guard is true first time */
  }

  SNetRecDetrefDestroy(rec, &land->outdesc);
  SNetRecDestroy( rec);
  assert(done);

  return out_rec;
}

/*****************************************************************************/
/* FILTER TASKS                                                              */
/*****************************************************************************/

/**
 * Filter task
 */
void SNetNodeFilter(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const filter_arg_t    *farg = DESC_NODE_SPEC(desc, filter);
  landing_siso_t        *land = DESC_LAND_SPEC(desc, siso);

  trace(__func__);

  if (land->outdesc == NULL) {
    land->outdesc = SNetStreamOpen(farg->output, desc);
  }
  switch (REC_DESCR(rec)) {
    case REC_data:
      rec = ApplyFilter(rec, farg, land, desc);
      if (rec) {
        SNetWrite(&land->outdesc, rec, true);
      }
      break;

    case REC_detref:
      /* forward record */
      SNetWrite(&land->outdesc, rec, true);
      break;

    case REC_sync:
      SNetRecDestroy(rec);
      break;

    default:
      SNetRecUnknownEnt(__func__, rec, farg->entity);
  }
}

/* Terminate a filter landing. */
void SNetTermFilter(landing_t *land, fifo_t *fifo)
{
  trace(__func__);
  if (LAND_SPEC(land, siso)->outdesc) {
    SNetFifoPut(fifo, LAND_SPEC(land, siso)->outdesc);
  }
  SNetFreeLanding(land);
}

/* Destroy a filter node. */
void SNetStopFilter(node_t *node, fifo_t *fifo)
{
  filter_arg_t *farg = NODE_SPEC(node, filter);
  trace(__func__);
  SNetStopStream(farg->output, fifo);
  SNetVariantDestroy(farg->input_variant);
  if (farg->filter_instructions != NULL) {
    int i;
    snet_expr_t *expr;
    LIST_ENUMERATE(farg->guard_exprs, i, expr) {
      SNetFilterInstrListListDestroy( farg->filter_instructions[i]);
      (void) expr;
    }
    SNetMemFree(farg->filter_instructions);
  }
  SNetExprListDestroy(farg->guard_exprs);
  SNetEntityDestroy(farg->entity);
  SNetMemFree(node);
}

/*****************************************************************************/
/* CREATION FUNCTIONS                                                        */
/*****************************************************************************/

/**
 * Convenience function for creating filter and translate
 */
static snet_stream_t* CreateFilter( snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_variant_t *input_variant,
    snet_expr_list_t *guard_exprs,
    snet_filter_instr_list_list_t **instr_list,
    const char *name)
{
  snet_stream_t *output;
  node_t        *node;
  filter_arg_t *farg;

  /* Check for bypass
   * - if it is a bypass, exit out early and do not create any component
   */
  if ( !FilterIsBypass(input_variant, guard_exprs, instr_list)) {
    assert(guard_exprs != NULL && SNetExprListLength(guard_exprs) > 0);

    output = SNetStreamCreate(0);
    node = SNetNodeNew(NODE_filter, location, &input, 1, &output, 1,
                       SNetNodeFilter, SNetStopFilter, SNetTermFilter);
    farg = NODE_SPEC(node, filter);
    farg->output = output;
    farg->input_variant = input_variant;
    farg->guard_exprs = guard_exprs;
    farg->filter_instructions = instr_list;
    farg->entity = SNetEntityCreate( ENTITY_filter, location, SNetLocvecGet(info),
                                     "<filter>", NULL, (void*)farg);
  } else {
    int i;
    snet_expr_t *expr;

    SNetVariantDestroy(input_variant);
    LIST_ENUMERATE(guard_exprs, i, expr) {
      SNetFilterInstrListListDestroy( instr_list[i]);
      (void) expr; /* prevent compiler warnings */
    }

    SNetMemFree( instr_list);
    SNetExprListDestroy(guard_exprs);
    output = input;
  }

  return output;
}

/**
 * Macro for creating a instr_list from the function varargs;
 * param 'last' refers to the last parameter before the varargs ...
 */
#define BUILD_INSTR_FROM_VARARG(instr_list, num, last) do {\
  va_list args; \
  int i; \
  (instr_list) = SNetNewN(num, snet_filter_instr_list_list_t*); \
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

  trace(__func__);
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

  trace(__func__);
  assert(input_variant != NULL);
  assert(guard_exprs != NULL);

  num_outtypes = SNetExprListLength( guard_exprs);

  /* read in the filter instructions from varargs */
  BUILD_INSTR_FROM_VARARG(instr_list, num_outtypes, guard_exprs);

  return CreateFilter(instream, info, location, input_variant,
      guard_exprs, instr_list, "<translate>");
}


