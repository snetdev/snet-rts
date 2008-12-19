#ifndef _SNET_QUEUE_H_
#define _SNET_QUEUE_H_

#define SNET_QUEUE_SUCCESS 0
#define SNET_QUEUE_ERROR 1

typedef struct snet_queue snet_queue_t;

typedef unsigned int snet_queue_iterator_t;

snet_queue_t *SNetQueueCreate();

void SNetQueueDestroy(snet_queue_t *queue);

int SNetQueueSize(snet_queue_t *queue);

int SNetQueuePut(snet_queue_t *queue, void *value);

void *SNetQueueGet(snet_queue_t *queue);

void *SNetQueuePeek(snet_queue_t *queue);

snet_queue_iterator_t SNetQueueIteratorBegin(snet_queue_t *queue);
snet_queue_iterator_t SNetQueueIteratorEnd(snet_queue_t *queue);
snet_queue_iterator_t SNetQueueIteratorNext(snet_queue_t *queue, snet_queue_iterator_t iterator);
void *SNetQueueIteratorGet(snet_queue_t *queue, snet_queue_iterator_t iterator);
void *SNetQueueIteratorPeek(snet_queue_t *queue, snet_queue_iterator_t iterator);

#endif /* _SNET_QUEUE_H_ */
