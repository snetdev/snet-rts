#ifndef _STREAM_H_
#define _STREAM_H_

#include <stdio.h>

#include "buffer.h"
#include "bool.h"


typedef struct buffer stream_t;

/** stream modifier handle */
typedef struct stream_mh stream_mh_t;    

/** a handle to the list */
typedef struct stream_mh *stream_list_t;

/** a list iterator */
typedef struct stream_iter stream_iter_t;


struct task;

stream_t *StreamCreate(void);
stream_mh_t *StreamOpen( struct task *ct, stream_t *s, char mode);
void StreamClose( stream_mh_t *mh, bool destroy_s);
void StreamReplace( stream_mh_t *mh, stream_t *snew);
void *StreamPeek( stream_mh_t *mh);
void *StreamRead( stream_mh_t *mh);
void StreamWrite( stream_mh_t *mh, void *item);

int StreamPrintDirty( struct task *t, FILE *file);




void StreamListAppend( stream_list_t *lst, stream_mh_t *node);
int StreamListIsEmpty( stream_list_t *lst);

stream_iter_t *StreamIterCreate( stream_list_t *lst);
void StreamIterDestroy( stream_iter_t *iter);
void StreamIterReset( stream_list_t *lst, stream_iter_t *iter);
int StreamIterHasNext( stream_iter_t *iter);
stream_mh_t *StreamIterNext( stream_iter_t *iter);
void StreamIterAppend( stream_iter_t *iter, stream_mh_t *node);
void StreamIterRemove( stream_iter_t *iter);



#endif /* _STREAM_H_ */
