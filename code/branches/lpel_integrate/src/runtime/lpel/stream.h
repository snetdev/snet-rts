#ifndef _STREAM_H_
#define _STREAM_H_

#include <stdio.h>
#include <pthread.h>

#include "bool.h"
#include "buffer.h"

typedef struct stream stream_t;


struct stream {
  buffer_t buffer;

  pthread_spinlock_t lock;
  volatile void **flag_ptr;
  volatile void *flag_stub;
};


/** stream modifier handle */
typedef struct stream_desc stream_desc_t;    

/** a handle to the list */
typedef struct stream_desc *stream_list_t;

/** a list iterator */
typedef struct stream_iter stream_iter_t;


struct task;

stream_t *StreamCreate(void);
void StreamDestroy( stream_t *s);
stream_desc_t *StreamOpen( struct task *ct, stream_t *s, char mode);
void StreamClose( stream_desc_t *sd, bool destroy_s);
void StreamReplace( stream_desc_t *sd, stream_t *snew);
void *StreamPeek( stream_desc_t *sd);
void *StreamRead( stream_desc_t *sd);
void StreamWrite( stream_desc_t *sd, void *item);

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
