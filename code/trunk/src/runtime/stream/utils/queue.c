#include <string.h>

#include "queue.h"
#include "memfun.h"

/* This isn't exactly a queue anymore, but an ordered list! */

#define NUM_INITIAL_ELEMENTS 32

struct snet_queue{
  unsigned int head;
  unsigned int tail;
  unsigned int size;
  unsigned int elements;
  void **entries;
};

static void SNetQueueCompact(snet_queue_t *queue)
{
  unsigned int temp;
  unsigned int mark;

  if(queue->elements != 0) {
    mark = queue->tail;
    temp = queue->head;
    do {
      if(queue->entries[temp] == NULL && mark == queue->tail) {
	mark = temp;
      } else if(mark != queue->tail) {
	queue->entries[mark] = queue->entries[temp];
	mark = (mark + 1) % queue->size;
      }

      temp = (temp + 1) % queue->size;
    } while(temp != queue->tail);

    queue->tail = (queue->head + queue->elements) % queue->size;
  }
}

static void SNetQueueIncreaseSize(snet_queue_t *queue)
{
  int temp;
  void **new_entries;

  new_entries = SNetMemAlloc(sizeof(void *) * (queue->size + NUM_INITIAL_ELEMENTS));

  temp = 0;

  do {
    if(queue->entries[queue->head] != NULL) {
      new_entries[temp] = queue->entries[queue->head];
      temp++;
    }

    queue->head = (queue->head + 1) % queue->size;
  } while(queue->head != queue->tail);

  SNetMemFree(queue->entries);

  queue->head = 0;
  queue->tail = temp;
  queue->size +=  NUM_INITIAL_ELEMENTS;

  memset(new_entries + queue->tail, 0, sizeof(void *) * (queue->size - queue->tail));

  queue->entries = new_entries;

  return;
}

snet_queue_t *SNetQueueCreate()
{
  snet_queue_t *queue;

  queue = SNetMemAlloc(sizeof(snet_queue_t));

  queue->head = 0;
  queue->tail = 0;
  queue->size = NUM_INITIAL_ELEMENTS;
  queue->elements = 0;

  queue->entries = SNetMemAlloc(sizeof(void *) * NUM_INITIAL_ELEMENTS);
  memset(queue->entries, 0, sizeof(void *) * NUM_INITIAL_ELEMENTS);

  return queue;
}

void SNetQueueDestroy(snet_queue_t *queue)
{
  SNetMemFree(queue->entries);
  SNetMemFree(queue);
}

int SNetQueueSize(snet_queue_t *queue)
{
  return queue->elements;
}

int SNetQueuePut(snet_queue_t *queue, void *value)
{

  /* There is always one empty element - the tail.
   * This makes the iterator simpler.
   */

  if((queue->tail + 1) % queue->size == queue->head) {
    if(queue->elements < queue->size) {
      /* There is room in the queue, but the queue is fragmented. */

      SNetQueueCompact(queue);

    } else {
      /* No room in the queue. */

      SNetQueueIncreaseSize(queue);
    }
  }

  queue->entries[queue->tail] = value;
  queue->tail = (queue->tail + 1) % queue->size;
  queue->entries[queue->tail] = NULL;
  queue->elements++;

  return SNET_QUEUE_SUCCESS;
}

void *SNetQueueGet(snet_queue_t *queue)
{
  void *value;

  if(queue->head != queue->tail) {
    value = queue->entries[queue->head];
    queue->elements--;

    do{
      queue->head = (queue->head + 1) % queue->size;
    } while(queue->head != queue->tail && queue->entries[queue->head] == NULL);

    return value;
  }

  return NULL;
}

void *SNetQueuePeek(snet_queue_t *queue)
{
  if(queue->head != queue->tail) {
    return queue->entries[queue->head];
  }

  return NULL;
}

snet_queue_iterator_t SNetQueueIteratorBegin(snet_queue_t *queue)
{
  return queue->head;
}


snet_queue_iterator_t SNetQueueIteratorEnd(snet_queue_t *queue)
{
  return queue->tail;
}

snet_queue_iterator_t SNetQueueIteratorNext(snet_queue_t *queue, snet_queue_iterator_t iterator)
{
  while(iterator != queue->tail) {
    iterator = (iterator + 1) % queue->size;

    if(queue->entries[iterator] != NULL) {
      break;
    }
  }
  return iterator;
}

void *SNetQueueIteratorGet(snet_queue_t *queue, snet_queue_iterator_t iterator)
{
  void *value;

   if(iterator != queue->tail) {
     value = queue->entries[iterator];
     queue->entries[iterator] = NULL;
     queue->elements--;

     if(queue->head == iterator) {
       do{
	 queue->head = (queue->head + 1) % queue->size;
       } while(queue->head != queue->tail && queue->entries[queue->head] == NULL);
     }

     return value;
  }

  return NULL;
}

void *SNetQueueIteratorPeek(snet_queue_t *queue, snet_queue_iterator_t iterator)
{
  if(iterator != queue->tail) {
    return queue->entries[iterator];
  }

  return NULL;
}
