#ifndef _STREAM_H_
#define _STREAM_H_

#include <stdio.h>
#include <pthread.h>

#include "bool.h"
#include "buffer.h"
#include "atomic.h"


//#define STREAM_POLL_SPINLOCK

/* a stream */
typedef struct stream stream_t;

/** stream modifier handle */
typedef struct stream_desc stream_desc_t;    

/** a handle to the list */
typedef struct stream_desc *stream_list_t;

/** a list iterator */
typedef struct stream_iter stream_iter_t;


struct stream {
  buffer_t buffer;

#ifdef STREAM_POLL_SPINLOCK
  pthread_spinlock_t prod_lock;
#else
  pthread_mutex_t prod_lock;
#endif
  int is_poll;
  stream_desc_t *prod_sd;
  stream_desc_t *cons_sd;
  atomic_t n_sem;
  atomic_t e_sem;
};



struct task;


stream_t *StreamCreate(void);
void StreamDestroy( stream_t *s);
stream_desc_t *StreamOpen( struct task *ct, stream_t *s, char mode);
void StreamClose( stream_desc_t *sd, bool destroy_s);
void StreamReplace( stream_desc_t *sd, stream_t *snew);
void *StreamPeek( stream_desc_t *sd);
void *StreamRead( stream_desc_t *sd);
void StreamWrite( stream_desc_t *sd, void *item);
void StreamPoll( stream_list_t *list);

int StreamPrintDirty( struct task *t, FILE *file);




void StreamListAppend( stream_list_t *lst, stream_desc_t *node);
int StreamListIsEmpty( stream_list_t *lst);

stream_iter_t *StreamIterCreate( stream_list_t *lst);
void StreamIterDestroy( stream_iter_t *iter);
void StreamIterReset( stream_list_t *lst, stream_iter_t *iter);
int StreamIterHasNext( stream_iter_t *iter);
stream_desc_t *StreamIterNext( stream_iter_t *iter);
void StreamIterAppend( stream_iter_t *iter, stream_desc_t *node);
void StreamIterRemove( stream_iter_t *iter);



#endif /* _STREAM_H_ */
