#ifndef _STREAM_H_
#define _STREAM_H_

#include "lpel.h"



//#define STREAM_POLL_SPINLOCK



/**
 * A stream descriptor
 *
 * A producer/consumer must open a stream before using it, and by opening
 * a stream, a stream descriptor is created and returned. 
 */
struct lpel_stream_desc_t {
  lpel_task_t   *task;        /** the task which opened the stream */
  lpel_stream_t *stream;      /** pointer to the stream */
  char mode;                  /** either 'r' or 'w' */
  char state;                 /** one of IOCR, for monitoring */
  unsigned long counter;      /** counts the number of items transmitted
                                  over the stream descriptor */
  int event_flags;            /** which events happened on that stream */
  /** for organizing in stream lists */
  struct lpel_stream_desc_t *next; 
  /** for maintaining a list of 'dirty' items */
  struct lpel_stream_desc_t *dirty;
};



/**
 * The state of a stream descriptor
 */
#define STDESC_INUSE    'I'
#define STDESC_OPENED   'O'
#define STDESC_CLOSED   'C'
#define STDESC_REPLACED 'R'

/**
 * The event_flags of a stream descriptor
 */
#define STDESC_MOVED    (1<<0)
#define STDESC_WOKEUP   (1<<1)
#define STDESC_WAITON   (1<<2)

/**
 * This special value indicates the end of the dirty list chain.
 * NULL cannot be used as NULL indicates that the SD is not dirty.
 */
#define STDESC_DIRTY_END   ((lpel_stream_desc_t *)-1)





int _LpelStreamResetDirty( lpel_task_t *t,
    void (*callback)(lpel_stream_desc_t *, void*), void *arg);




#endif /* _STREAM_H_ */
