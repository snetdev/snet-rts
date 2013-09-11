#include "node.h"
#include "reference.h"

int node_location;

void hello(const char *s)
{
  printf("%s called\n", s);
}

/* Initialize distributed computing. Called by SNetInRun in networkinterface.c. */
void SNetDistribInit(int argc, char **argv, snet_info_t *info)
{
  SNetReferenceInit();
}

/* Start input+output managers. Called by SNetInRun in networkinterface.c. */
void SNetDistribStart(void)
{
}

void SNetDistribGlobalStop(void)
{
}

void SNetDistribWaitExit(snet_info_t *info)
{
  hello(__func__);
}

/* Needed frequently for record IDs and field references. */
int SNetDistribGetNodeId(void)
{
  return node_location;
}

/* Are we identified by location? Needed frequently for field references. */
bool SNetDistribIsNodeLocation(int location)
{
  return true;
}

/* Test for rootness. First call by SNetInRun in networkinterface.c. */
bool SNetDistribIsRootNode(void)
{
  return true;
}

/* Is this application supporting distributed computations? */
bool SNetDistribIsDistributed(void)
{
  return false;
}

/* void SNetDistribSendData(snet_ref_t *ref, void *data, int node)
{
  hello(__func__);
} */

/* Use by distribution layer in C4SNet.c. */
void SNetDistribPack(void *src, ...)
{
  hello(__func__);
}

/* Use by distribution layer in C4SNet.c. */
void SNetDistribUnpack(void *dst, ...)
{
  hello(__func__);
}

/* Request a field from transfer. Called when distributed. */
void SNetDistribFetchRef(snet_ref_t *ref)
{
  hello(__func__);
}

/* Update field reference counter. Called when distributed. */
void SNetDistribUpdateRef(snet_ref_t *ref, int count)
{
  hello(__func__);
}

/* Dummy function needed for networkinterface.c */
snet_stream_t *SNetRouteUpdate(snet_info_t *info, snet_stream_t *in, int loc)
{
  return in;
}

/* Dummy function which is irrelevant for non-Distributed S-Net. */
void SNetInputManagerRun(worker_t *worker)
{
  assert(false);
}

/* Dummy function which is irrelevant for non-Distributed S-Net. */
snet_stream_desc_t *SNetTransferOpen(
    int loc,
    snet_stream_desc_t *desc,
    snet_stream_desc_t *prev)
{
  assert(false);
}

void SNetRecDetrefStackSerialise(snet_record_t *rec, void *buf)
{
  assert(false);
}

void SNetRecDetrefRecSerialise(snet_record_t *rec, void *buf)
{
  assert(false);
}

void SNetRecDetrefStackDeserialise(snet_record_t *rec, void *buf)
{
  assert(false);
}

snet_record_t* SNetRecDetrefRecDeserialise(void *buf)
{
  assert(false);
}

