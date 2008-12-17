#ifndef _SNET_DATASTORAGE_H_
#define _SNET_DATASTORAGE_H_

#include "id.h"
#include "reference.h"

void SNetDataStorageInit();

void SNetDataStorageDestroy();

snet_ref_t *SNetDataStoragePut(snet_id_t id, snet_ref_t *ref);

snet_ref_t *SNetDataStorageCopy(snet_id_t id);

snet_ref_t *SNetDataStorageGet(snet_id_t id);

void SNetDataStorageRemove(snet_id_t id);

void *SNetDataStorageRemoteFetch(snet_id_t id, int interface, unsigned int location);

void SNetDataStorageRemoteCopy(snet_id_t id, unsigned int location);

void SNetDataStorageRemoteDelete(snet_id_t id, unsigned int location);

#endif /* SNET_DATASTORAGE_H_ */
