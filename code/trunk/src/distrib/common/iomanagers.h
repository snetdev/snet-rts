#ifndef IOMANAGERS_H
#define IOMANAGERS_H

typedef struct snet_dest snet_dest_t;

#include "record.h"

/* To be implemented by distrib/implementation */
void SNetDistribNewDynamicCon(snet_dest_t *dest);

void SNetDistribSendRecord(snet_dest_t *dest, snet_record_t *rec);
snet_record_t *SNetDistribRecvRecord(snet_dest_t **dest);

void SNetDistribRemoteRefFetch(snet_ref_t *ref);
void SNetDistribRemoteRefUpdate(snet_ref_t *ref, int count);

bool SNetDestCompare(snet_dest_t *d1, snet_dest_t *d2);
snet_dest_t *SNetDestCopy(snet_dest_t *dest);
void SNetDestFree(snet_dest_t *dest);

void SNetDistribRemoteDestBlock(snet_dest_t *dest);
void SNetDistribRemoteDestUnblock(snet_dest_t *dest);
#endif
