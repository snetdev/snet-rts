#ifndef IOMANAGERS_H
#define IOMANAGERS_H

typedef struct snet_dest snet_dest_t;

#include "threading.h"
#include "record.h"

/* Provided by distrib/common */
void SNetDistribStopOutputManager();

void SNetDistribNewIn(snet_dest_t *dest, snet_stream_t *stream);
void SNetDistribNewOut(snet_dest_t *dest, snet_stream_t *stream);

void SNetOutputManager(snet_entity_t *ent, void *args);
void SNetInputManager(snet_entity_t *ent, void *args);
void SNetDataStorageManager(snet_entity_t *ent, void *args);

snet_ref_t *SNetRefIncoming(int node, int interface, uintptr_t data);
void SNetRefOutgoing(snet_ref_t *ref);
void *SNetRefFetch(snet_ref_t *ref);
void SNetRefUpdate(snet_ref_t *ref, int value);
void SNetRefSet(snet_ref_t *ref, void *data);

/* To be implemented by distrib/implementation */
void SNetDistribNewDynamicCon(snet_dest_t *dest);

void SendRecord(snet_dest_t *dest, snet_record_t *rec);
snet_record_t *RecvRecord(snet_dest_t **dest);

bool SNetDestCompare(snet_dest_t *d1, snet_dest_t *d2);
snet_dest_t *SNetDestCopy(snet_dest_t *dest);
void SNetDestFree(snet_dest_t *dest);

void SNetDistribRemoteFetch(snet_ref_t *ref);
void SNetDistribRemoteUpdate(snet_ref_t *ref, int count);
#endif
