#include <assert.h>

#include "distribmap.h"
#include "distribution.h"
#include "entities.h"
#include "iomanager.h"
#include "memfun.h"

extern snet_stream_desc_t *sd;
extern snet_dest_stream_map_t *destMap;

void SNetInputManager(snet_entity_t *ent, void *args)
{
  while (RecvRecord());

  assert(SNetDestStreamMapSize(destMap) == 0);
  SNetStreamWrite(sd, NULL);
  SNetStreamClose(sd, false);
}
