#ifndef _SNET_UBUFFER_H_
#define _SNET_BUFFER_H_

/*
 * $Id
 *
 * This implements an unbounded buffer.
 */

#include <pthread.h>

#include "bool.h"
#include "extended_semaphore.h"

typedef struct snet_ubuffer snet_ubuffer_t;


extern bool SNetUBufIsEmpty(snet_ubuffer_t *buf);

extern snet_ubuffer_t *SNetUBufCreate(pthread_mutex_t *lock);

extern void SNetUBufDestroy( snet_ubuffer_t *buf);

extern bool SNetUBufPut( snet_ubuffer_t *buf, void* elem);

extern void *SNetUBufGet( snet_ubuffer_t *bf);

extern void *SNetUBufShow( snet_ubuffer_t *buf);

extern bool SNetUBufIsEmpty(snet_ubuffer_t *buf);
  
extern void SNetUBufRegisterDispatcher( snet_ubuffer_t *buf, snet_ex_sem_t *sem);

extern bool SNetUBufGetFlag(snet_ubuffer_t *buf);

extern void SNetUBufSetFlag(snet_ubuffer_t *buf, bool flag);

#endif /* _SNET_UBUFFER_H_ */
