#ifndef _FLAGTREE_H_
#define _FLAGTREE_H_

/* number of nodes for a given height h: 2^(h+1)-1*/
#define FT_NODES(h)           ( (1<<((h)+1)) - 1 )

/* number of leafs for a given height h: 2^h */
#define FT_LEAFS(h)           (1<<(h))

/* number of nodes at a certain level v (root=0): 2^v */
#define FT_NODES_AT_LEVEL(v)  (1<<(v))

/* number of flags to set for a given height h: h+1 */
#define FT_CNT_FLAGS(h)       ((h)+1)

/* which index of the tree the leafs start for a given height h: 2^h - 1 */
#define FT_LEAF_START_IDX(h)  ( (1<<(h)) - 1 )


/* mapping from an index i of the leafs
 * to the index in the heap, depending on height h:
 * i + (2^h - 1)
 * e.g.
 *  leaf #0 -> index 7  if h=3
 *  leaf #3 -> index 10 if h=3
 */
#define FT_LEAF_TO_IDX(h,i)   ( (i) + (1<<(h)) - 1 )

/* inverse function of FT_LEAF_TO_IDX: i+1 - 2^h */
#define FT_IDX_TO_LEAF(h,i)   ( (i)+1 - (1<<(h)) )

/* parent index of index i: floor( (i-1)/2 ) */
#define FT_PARENT(i)          ( ((i)-1)/2 )

/* left child index of index i: 2i+1 */
#define FT_LEFT_CHILD(i)      ( 2*(i) + 1 )

/* right child index of index i: 2i+2 = 2(i+1)*/
#define FT_RIGHT_CHILD(i)     ( 2*((i)+1) )


/* include a Readers/Writer lock for concurrent access */
#include "rwlock.h"

/* include to have memory barriers */
#include "sysdep.h"

/** the callback function for gathering */
typedef void (*flagtree_gather_cb_t)(int i, void *arg);

typedef struct {
  int height;
  int *buf;
  rwlock_t *lock;
  flagtree_gather_cb_t gather;
} flagtree_t;


/**
 * mark the tree up to the root starting with leaf #idx
 *
 * @param ft      the flagtree
 * @param idx     the leaf index to mark upwards from
 * @param reader  if (reader>=0) then the rwlock is aquired for that reader
 * @pre           0 <= idx < FT_LEAFS(ft.height) [=2^height]
 * @pre           if the lock is not aquired in FlagtreeMark,
  *               FlagtreeGrow must not be executed concurrently
 */
static inline void FlagtreeMark(flagtree_t *ft, int idx, int reader)
{
  /* lock for reading */
  if (reader>=0) RwlockReaderLock( ft->lock, reader );

  int j = FT_LEAF_TO_IDX(ft->height, idx);
  ft->buf[j] = 1;
  while (j != 0) {
    WMB(); /* enforce ordering for marking upward the tree, a no-op for x86 */
    j = FT_PARENT(j);
    ft->buf[j] = 1;
  }
  /* unlock for reading */
  if (reader>=0) RwlockReaderUnlock( ft->lock, reader );
}

#if 0
/** Gathering procedure without GOTOs */
static inline int FlagtreeGatherNoGoto(flagtree_t *ft, flagtree_gather_cb_t gather, void *arg)
{
  int prev, cur, next;
  /* number of gathered leafs */
  int count = 0;
  /* nothing to do if root not set */
  if (ft->buf[0] == 0) return 0;
  /* register callback */
  ft->gather = gather;
  /* start from root */
  prev = cur = 0;
  do {
#ifndef NDEBUG
    /* for stepwise debugging: */
    /* FlagtreePrint(ft); */
#endif
    if (prev == FT_PARENT(cur)) {
      /* clear cur */
      ft->buf[cur] = 0;
      if ( cur < FT_LEAF_START_IDX(ft->height) ) {
        if ( ft->buf[FT_LEFT_CHILD(cur)] != 0 ) { next = FT_LEFT_CHILD(cur); }
        else if ( ft->buf[FT_RIGHT_CHILD(cur)] != 0 ) { next = FT_RIGHT_CHILD(cur); }
        else { next = FT_PARENT(cur); }
      } else {
        /* gather leaf of idx */
        ft->gather( FT_IDX_TO_LEAF(ft->height, cur), arg );
        /* increment gathered leaf count */
        count++;
        next = FT_PARENT(cur);
      }

    } else if (prev == FT_LEFT_CHILD(cur)) {
      if ( ft->buf[FT_RIGHT_CHILD(cur)] != 0 ) { next = FT_RIGHT_CHILD(cur); }
      else { next = FT_PARENT(cur); }

    } else if (prev == FT_RIGHT_CHILD(cur)) {
      next = FT_PARENT(cur);
    }
    prev = cur;
    cur = next;
  } while (cur != prev);
  /* cur == prev only at root */
  return count;
}
#endif


/**
 * Gather marked leafs iteratively,
 * clearing the marks in preorder
 * @param ft    the flagtree to gather from
 * @pre         FlagtreeGather and FlagtreeGrow must not be executed concurrently
 * @note  TODO  This works for now only in x86 (guaranteed, w/o fences).
 *              As a workaround, for the procedure of gathering, the RW-Lock must be aquired for writing.
 */
static inline int FlagtreeGather(flagtree_t *ft, flagtree_gather_cb_t gather, void *arg)
{
  int prev, cur, next;
  /* number of gathered leafs */
  int count = 0;
  /* nothing to do if root not set */
  if (ft->buf[0] == 0) return 0;
  
  /* register callback */
  ft->gather = gather;
  /* start from root */
  prev = cur = 0;

  /*TODO LOCK_WRITE */
  do {
#ifndef NDEBUG
    /* for stepwise debugging: */
    /*FlagtreePrint(ft);*/
#endif
    if (prev == FT_PARENT(cur)) {
      /* clear cur (preorder), XCHG in x86 is not reordered w.r.t. earlier/later loads */
      (void) xchg(&ft->buf[cur], 0);
      /* no WMB(); clearing the flag can be delayed */
      if ( cur >= FT_LEAF_START_IDX(ft->height) ) {
        /* gather leaf of idx */
        ft->gather( FT_IDX_TO_LEAF(ft->height, cur), arg );
        /* increment gathered leaf count */
        count++;
        goto lab_parent;
      }
    } else if (prev == FT_LEFT_CHILD(cur)) {
      goto lab_right;
    } else if (prev == FT_RIGHT_CHILD(cur)) {
      goto lab_parent;
    }

    if ( ft->buf[FT_LEFT_CHILD(cur)] != 0 ) {
      next = FT_LEFT_CHILD(cur);
      goto lab_out;
    }

lab_right:
    if ( ft->buf[FT_RIGHT_CHILD(cur)] != 0 ) {
      next = FT_RIGHT_CHILD(cur);
      goto lab_out;
    }

lab_parent:
    next = FT_PARENT(cur);

lab_out:
    prev = cur;
    cur = next;
  } while (cur != prev); /* cur == prev only at root */
  
  /*TODO UNLOCK_WRITE */
  return count;
}


extern void FlagtreeInit(flagtree_t *ft, int height, rwlock_t *lock);
extern void FlagtreeCleanup(flagtree_t *ft);
extern void FlagtreeGrow(flagtree_t *ft);
extern int  FlagtreeGatherRec(flagtree_t *ft, flagtree_gather_cb_t gather, void *arg);

#ifndef NDEBUG
extern void FlagtreePrint(flagtree_t *ft);
#endif

#endif /* _FLAGTREE_H_ */
