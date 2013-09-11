#include "node.h"

/* Identity node: forward all records as is.  */
static void SNetNodeIdentity(snet_stream_desc_t *desc, snet_record_t *rec)
{
  landing_identity_t    *land = DESC_LAND_SPEC(desc, identity);

  assert(land->outdesc);

  SNetWrite(&land->outdesc, rec, true);
}

/* Terminate an identity landing. */
void SNetTermIdentity(landing_t *land, fifo_t *fifo)
{
  trace(__func__);
  SNetFifoPut(fifo, LAND_SPEC(land, identity)->outdesc);
  SNetFreeLanding(land);
}

/* Delete identity node: should be impossible. */
static void SNetStopIdentity(node_t *node, fifo_t *fifo)
{
  /* impossible */
  trace(__func__);
  assert(0);
}

/* Change a landing into an identity landing. */
void SNetBecomeIdentity(landing_t *land, snet_stream_desc_t *out)
{
  /* One static node for all identities. */
  static node_t identity_node = {
    NODE_identity, SNetNodeIdentity, SNetStopIdentity, SNetTermIdentity
  };
  landing_type_t prev_type = land->type;

  if (SNetDebugGC()) {
    printf("%s: node %s land %s\n",
           __func__, SNetNodeName(land->node), SNetLandingName(land));
  }
  
  /* Destroy old resources */
  SNetCleanupLanding(land);

  /* Init new state */
  land->type = LAND_identity;
  land->node = &identity_node;
  LAND_SPEC(land, identity)->outdesc = out;
  LAND_SPEC(land, identity)->prev_type = prev_type;

  assert(land->refs > 0);
}

