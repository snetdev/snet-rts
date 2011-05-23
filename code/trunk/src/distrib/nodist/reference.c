#include "arch/atomic.h"
#include "interface_functions.h"
#include "memfun.h"
#include "reference.h"

struct snet_ref {
  atomic_t count;
  int interface;
  void *data;
};

snet_ref_t *SNetRefCreate(int interface, void *data)
{
  snet_ref_t *result = SNetMemAlloc(sizeof(snet_ref_t));

  atomic_init(&result->count, 1);
  result->interface = interface;
  result->data = data;

  return result;
}

snet_ref_t *SNetRefCopy(snet_ref_t *ref)
{
  atomic_inc(&ref->count);
  return ref;
}

void *SNetRefGetData(snet_ref_t *ref)
{
  return ref->data;
}

void SNetRefDestroy(snet_ref_t *ref)
{
  if (fetch_and_dec(&ref->count) == 1) {
    SNetInterfaceGet(ref->interface)->freefun(ref->data);
  }
}
