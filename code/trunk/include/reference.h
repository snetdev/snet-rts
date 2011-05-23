#ifndef _SNET_REFERENCE_H_
#define _SNET_REFERENCE_H_

typedef struct snet_ref snet_ref_t;

snet_ref_t *SNetRefCreate(int interface, void *data);
snet_ref_t *SNetRefCopy(snet_ref_t *ref);
void *SNetRefGetData(snet_ref_t *ref);
void SNetRefDestroy(snet_ref_t *ref);

#endif /* _SNET_REFERENCE_H_ */
