#ifndef _SNET_QUEUE_H_
#define _SNET_QUEUE_H_

#define SNET_QUEUE_SUCCESS 0
#define SNET_QUEUE_ERROR 1

typedef int (*snet_queue_compare_fun_t)(void *, void *);

typedef struct snet_queue snet_queue_t;

snet_queue_t *SNetQueueCreate(snet_queue_compare_fun_t compare_fun);

void SNetQueueDestroy(snet_queue_t *queue);

int SNetQueueSize(snet_queue_t *queue);

int SNetQueuePut(snet_queue_t *queue, void *value);

void *SNetQueueGet(snet_queue_t *queue);

void *SNetQueueGetMatch(snet_queue_t *queue, void *match);

#endif /* _SNET_QUEUE_H_ */
