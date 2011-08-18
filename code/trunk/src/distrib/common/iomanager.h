#ifndef IOMANAGER_H
#define IOMANAGER_H

typedef struct snet_dest snet_dest_t;

#include "threading.h"
#include "record.h"
#include "distribmap.h"

typedef struct {
  snet_dest_t *dest;
  snet_stream_t *stream;
} snet_dest_stream_tuple_t;

void SendRecord(snet_dest_t *dest, snet_record_t *rec);
bool RecvRecord(void);

bool SNetDestCompare(snet_dest_t *d1, snet_dest_t *d2);
snet_dest_t *SNetDestCopy(snet_dest_t *dest);
void SNetDestFree(snet_dest_t *dest);

void SNetOutputManager(snet_entity_t *ent, snet_stream_t *instream);
void SNetInputManager(snet_entity_t *ent, void *args);
#endif
