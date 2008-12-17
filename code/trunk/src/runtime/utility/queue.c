#include <string.h>

#include "queue.h"
#include "memfun.h"

#define NUM_INITIAL_ELEMENTS 32

struct snet_queue{
  unsigned int head;
  unsigned int tail;
  unsigned int size;
  unsigned int elements;
  void **entries;
  snet_queue_compare_fun_t compare_fun;
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

snet_queue_t *SNetQueueCreate(snet_queue_compare_fun_t compare_fun) 
{
  snet_queue_t *queue;

  queue = SNetMemAlloc(sizeof(snet_queue_t));

  queue->head = 0;
  queue->tail = 0;
  queue->size = NUM_INITIAL_ELEMENTS;
  queue->elements = 0;

  queue->entries = SNetMemAlloc(sizeof(void *) * NUM_INITIAL_ELEMENTS);
  memset(queue->entries, 0, sizeof(void *) * NUM_INITIAL_ELEMENTS);

  queue->compare_fun = compare_fun;

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
  int temp;
  void **new_entries;

  if(queue->tail == queue->head && queue->elements != 0) {
    if(queue->elements < queue->size) {
      /* There is room in the queue, but the queue is fragmented. */

      SNetQueueCompact(queue);

    } else {
      /* No room in the queue. */
      
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
    }
  }

  queue->entries[queue->tail] = value;
  queue->tail = (queue->tail + 1) % queue->size;
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

void *SNetQueueGetMatch(snet_queue_t *queue, void *match)
{
  unsigned int temp;
  unsigned int mark;
  void *value;

  temp = queue->head;
  mark = queue->tail;

  if(queue->elements != 0) {
    while(temp != queue->tail) {
      if(queue->entries[temp] != NULL) {
	if(queue->compare_fun(match, queue->entries[temp])) {
	  value = queue->entries[temp];
	  queue->entries[temp] = NULL;
	  queue->elements--;

	  return value;
	}

	if(mark != queue->tail) {
	  queue->entries[mark] = queue->entries[temp];
	  mark = (mark + 1 ) % queue->size;
	}
	
      } else if(mark != queue->tail) {
	mark = temp;
      }
	
      
      temp = (temp + 1) % queue->size;
    }

    queue->tail = mark;
  }

  return NULL;
}
