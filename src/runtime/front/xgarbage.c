#include "node.h"

/* Garbage node is temporary before termination: never receive new input. */
static void SNetNodeGarbage(snet_stream_desc_t *desc, snet_record_t *rec)
{
  /* impossible */
  trace(__func__);
  assert(0);
}

/* Terminate a garbage landing: decrease references to subsequent resources. */
void SNetTermGarbage(landing_t *land, fifo_t *fifo)
{
  snet_stream_desc_t    *desc = LAND_SPEC(land, siso)->outdesc;
  trace(__func__);
  assert(desc);
  assert(desc->refs > 0);
  SNetLandingDone(desc->landing);
  SNetFifoPut(fifo, desc);
  SNetFreeLanding(land);
}

/* Delete garbage node: should be impossible. */
static void SNetStopGarbage(node_t *node, fifo_t *fifo)
{
  /* impossible */
  trace(__func__);
  assert(0);
}

/* Change an identity landing into a garbage landing. */
void SNetBecomeGarbage(landing_t *land)
{
  /* One static node for all garbage landings. */
  static node_t garbage_node = {
    NODE_garbage, SNetNodeGarbage, SNetStopGarbage, SNetTermGarbage
  };
  landing_identity_t    *ident = LAND_SPEC(land, identity);
  snet_stream_desc_t    *desc = ident->outdesc;

  /* Only identity landings can become a garbage landing. */
  assert(land->type == LAND_identity);

  if (SNetDebugGC()) {
    printf("%s: node %s land %s (prev %s)\n", __func__, SNetNodeName(land->node),
           SNetLandingName(land), SNetLandingTypeName(ident->prev_type));
  }

  /* Init new state to ``garbage'' */
  land->type = LAND_garbage;
  land->node = &garbage_node;

  /* Guarantee that destination stream persists until termination. */
  DESC_INCR(desc);
  LAND_INCR(desc->landing);
  assert(desc == LAND_SPEC(land, siso)->outdesc);
}


