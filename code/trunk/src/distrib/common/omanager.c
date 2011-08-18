#include <assert.h>

#include "distribmap.h"
#include "entities.h"
#include "iomanager.h"
#include "memfun.h"
#include "record.h"

void SNetOutputManager(snet_entity_t *ent, snet_stream_t *instream)
{
  snet_streamset_t outgoing = NULL;
  snet_stream_desc_t *input = SNetStreamOpen(instream, 'r');
  snet_stream_dest_map_t *streamMap = SNetStreamDestMapCreate(0);

  SNetStreamsetPut(&outgoing, input);

  while (true) {
    snet_stream_desc_t *sd = SNetStreamPoll(&outgoing);

    if (sd == input) {
      snet_dest_stream_tuple_t *tuple = SNetStreamRead(sd);

      if (tuple == NULL) {
        SNetStreamClose(input, true);
        SNetStreamsetRemove(&outgoing, input);

        assert(SNetStreamDestMapSize(streamMap) == 0);
        assert(outgoing == NULL);
        return;
      }

      sd = SNetStreamOpen(tuple->stream, 'r');
      SNetStreamDestMapSet(streamMap, sd, tuple->dest);
      SNetStreamsetPut(&outgoing, sd);

      SNetMemFree(tuple);
    } else {
      snet_dest_t *dest = NULL;
      snet_record_t *rec = SNetStreamRead(sd);

      switch (SNetRecGetDescriptor(rec)) {
        case REC_sync:
          SNetStreamReplace(sd, SNetRecGetStream(rec));
          break;
        case REC_terminate:
          dest = SNetStreamDestMapTake(streamMap, sd);
          SNetStreamsetRemove(&outgoing, sd);
          SendRecord(dest, rec);
          SNetDestFree(dest);
          break;
        default:
          SendRecord(SNetStreamDestMapGet(streamMap, sd), rec);
          break;
      }

      SNetRecDestroy(rec);
    }
  }
}
