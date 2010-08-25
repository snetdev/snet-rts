#ifndef EX_SEM_H
#define EX_SEM_H

#include "pthread.h"
#include "bool.h"

typedef struct extended_semaphore snet_ex_sem_t;

snet_ex_sem_t *SNetExSemCreate(pthread_mutex_t *access,
                               bool has_min_value, int min_value,
                               bool has_max_value, int max_value,
                               int initial_value);
void SNetExSemDestroy(snet_ex_sem_t *target);
snet_ex_sem_t *SNetExSemIncrement(snet_ex_sem_t *target);
snet_ex_sem_t *SNetExSemDecrement(snet_ex_sem_t *target);
void SNetExSemWaitWhileMinValue(snet_ex_sem_t *target);
void SNetExSemWaitWhileMaxValue(snet_ex_sem_t *target);

int SNetExSemGetValue(snet_ex_sem_t *value_provider);
bool SNetExSemIsMaximal(snet_ex_sem_t *target);
bool SNetExSemIsMinimal(snet_ex_sem_t *target);

#endif
