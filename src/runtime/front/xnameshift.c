#include "node.h"

/* Nameshift node */
void SNetNodeNameShift(snet_stream_desc_t *desc, snet_record_t *rec)
{
  const filter_arg_t    *farg = DESC_NODE_SPEC(desc, filter);
  landing_siso_t        *land = DESC_LAND_SPEC(desc, siso);
  snet_variant_t        *untouched = farg->input_variant;
  snet_ref_t            *field;
  int name, offset, val;

  trace(__func__);

  /* Guards are misused for offset */
  offset = SNetEevaluateInt( SNetExprListGet( farg->guard_exprs, 0), NULL);

  if (land->outdesc == NULL) {
    land->outdesc = SNetStreamOpen(farg->output, desc);
  }
  switch (REC_DESCR(rec)) {
    case REC_data:
      RECORD_FOR_EACH_FIELD(rec, name, field) {
        if (!SNetVariantHasField(untouched, name)) {
          SNetRecRenameField( rec, name, name + offset);
        }
        (void) field;   /* prevent compiler warnings */
      }

      RECORD_FOR_EACH_TAG(rec, name, val) {
        if (!SNetVariantHasTag(untouched, name)) {
          SNetRecRenameTag( rec, name, name + offset);
        }
        (void) val;   /* prevent compiler warnings */
      }

      RECORD_FOR_EACH_BTAG(rec, name, val) {
        if (!SNetVariantHasBTag(untouched, name)) {
          SNetRecRenameBTag( rec, name, name + offset);
        }
      }

      SNetWrite(&land->outdesc, rec, true);
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

/* Terminate a nameshift landing. */
void SNetTermNameShift(landing_t *land, fifo_t *fifo)
{
  trace(__func__);
  if (LAND_SPEC(land, siso)->outdesc) {
    SNetFifoPut(fifo, LAND_SPEC(land, siso)->outdesc);
  }
  SNetFreeLanding(land);
}

/* Destroy a nameshift node. */
void SNetStopNameShift(node_t *node, fifo_t *fifo)
{
  filter_arg_t *farg = NODE_SPEC(node, filter);
  trace(__func__);
  SNetStopStream(farg->output, fifo);
  SNetVariantDestroy( farg->input_variant);
  if (farg->filter_instructions != NULL) {
    int i;
    snet_expr_t *expr;
    LIST_ENUMERATE(farg->guard_exprs, i, expr) {
      SNetFilterInstrListListDestroy( farg->filter_instructions[i]);
      (void) expr;
    }
    SNetMemFree( farg->filter_instructions);
  }
  SNetExprListDestroy( farg->guard_exprs);
  SNetEntityDestroy(farg->entity);
  SNetMemFree(node);
}

snet_stream_t *SNetNameShift(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    int offset,
    snet_variant_t *untouched)
{
  snet_stream_t *output;
  node_t        *node;
  filter_arg_t *farg;

  trace(__func__);

  output = SNetStreamCreate(0);
  node = SNetNodeNew(NODE_nameshift, location, &input, 1, &output, 1,
                     SNetNodeNameShift, SNetStopNameShift, SNetTermNameShift);
  farg = NODE_SPEC(node, filter);
  farg->output = output;
  farg->input_variant = untouched;
  farg->guard_exprs = SNetExprListCreate( 1, SNetEconsti( offset));
  farg->filter_instructions = NULL; /* instructions */
  farg->entity = SNetEntityCreate( ENTITY_nameshift, location, SNetLocvecGet(info),
                                   "<nameshift>", NULL, (void*)farg);

  return output;
}

