#ifndef _SNET_REFERENCE_H_
#define _SNET_REFERENCE_H_

#include <mpi.h>
#include "id.h"

typedef struct snet_ref snet_ref_t;

snet_ref_t *SNetRefCreate(snet_id_t id, int location, 
			  int interface, void *data);

snet_ref_t *SNetRefDestroy(snet_ref_t *ref);

snet_ref_t *SNetRefCopy(snet_ref_t *ref);

int SNetRefGetRefCount(snet_ref_t *ref);

void *SNetRefGetData(snet_ref_t *ref);

void *SNetRefTakeData(snet_ref_t *ref);

int SNetRefGetInterface(snet_ref_t *ref);

int SNetRefPack(snet_ref_t *ref, MPI_Comm comm, int *pos, void *buf, int buf_size);

snet_ref_t *SNetRefUnpack(MPI_Comm comm, int *pos, void *buf, int buf_size);

#endif /* _SNET_REFERENCE_H_ */
