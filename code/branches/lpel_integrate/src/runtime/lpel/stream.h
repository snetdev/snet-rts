#ifndef _STREAM_H_
#define _STREAM_H_

#include <stdio.h>
#include "buffer.h"
#include "bool.h"


typedef struct buffer stream_t;

typedef struct stream_mh stream_mh_t;

struct task;

stream_t *StreamCreate(void);
stream_mh_t *StreamOpen( struct task *ct, stream_t *s, char mode);
void StreamClose( stream_mh_t *mh, bool destroy_s);
void StreamReplace( stream_mh_t *mh, stream_t *snew);
void *StreamPeek( stream_mh_t *mh);
void *StreamRead( stream_mh_t *mh);
void StreamWrite( stream_mh_t *mh, void *item);

int StreamPrintDirty( struct task *t, FILE *file);


#endif /* _STREAM_H_ */
