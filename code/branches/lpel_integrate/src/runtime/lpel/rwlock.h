#ifndef _RWLOCK_H_
#define _RWLOCK_H_


#include <malloc.h>
#include <assert.h>
#include "sysdep.h"


/**
 * A Readers/Writer Lock, for a single writer and a
 * fixed number of readers (specified upon initialisation)
 * - using local spinning
 *
 * @TODO This implementation heavily relies on the properties of x86's
 *       memory model and XCHG behaviour.
 *       Adaptations are required to fit other architectures.
 *
 * @see Intel(R) 64 and IA-32 Architecures Software Developer's Manual,
 *      Volume 3A, Chapter 8.2 [June 2010]
 *      (esp. 8.2.3.9 "Loads and Stores Are Not Reordered with
 *       Locked Instructions")
 */

#define intxCacheline (64/sizeof(int))

struct rwlock_flag {
  volatile int l;
  int padding[intxCacheline-1];
};

typedef struct {
  struct rwlock_flag writer;
  int num_readers;
  struct rwlock_flag *readers;
} rwlock_t;

static inline void RwlockInit( rwlock_t *v, int num_readers )
{
  v->writer.l = 0;
  v->num_readers = num_readers;
  v->readers = (struct rwlock_flag *) calloc(num_readers, sizeof(struct rwlock_flag));
}

static inline void RwlockCleanup( rwlock_t *v )
{
  free(v->readers);
}

static inline void RwlockReaderLock( rwlock_t *v, int ridx )
{
  while(1) {
    while( v->writer.l != 0 ); /*spin*/
    
    /* set me as trying */
    (void) xchg(&v->readers[ridx].l, 1);

    if (v->writer.l == 0) {
      /* no writer: success! */
      break;
    }

    /* backoff to let writer go through */
    (void) xchg(&v->readers[ridx].l, 0);
  }
}

static inline void RwlockReaderUnlock( rwlock_t *v, int ridx )
{
  v->readers[ridx].l = 0;
}


static inline void RwlockWriterLock( rwlock_t *v )
{
  int i;
  /* assume no competing writer and a sane lock/unlock usage */
  assert( v->writer.l == 0 );

  (void) xchg(&v->writer.l, 1);
  /*
   * now write lock is held, but we have to wait until current
   * readers have finished
   */
  for (i=0; i<v->num_readers; i++) {
    while( v->readers[i].l != 0 ); /*spin*/
  }
}

static inline void RwlockWriterUnlock( rwlock_t *v )
{
  v->writer.l = 0;
}

#endif /* _RWLOCK_H_ */
