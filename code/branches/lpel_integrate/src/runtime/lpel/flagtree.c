/**
 * Implements a flag signalling tree for Collector tasks
 *
 * Assumption: FlagtreeGrow and FlagtreeGather are never
 *             called concurrently on the same flagtree!
 */


#include <stdlib.h>

/* FlagtreePrint will only be included if NDEBUG is not set */
#ifndef NDEBUG
#include <stdio.h>
#include <string.h>
#endif

#include "flagtree.h"



/**
 * Initialise the flagtree
 * 
 * Allocates memory for a given height.
 *
 * @param ft      ptr to flagtree
 * @param height  allocated space will fit a complete binary
 *                tree of the given height 
 * @param lock    RW-Lock for concurrent access
 */
void FlagtreeInit(flagtree_t *ft, int height, rwlock_t *lock)
{
  ft->height = height;
  ft->buf = (int *) calloc( FT_NODES(height), sizeof(int) );
  ft->lock = lock;
}

/**
 * Cleanup the flagtee: free allocated memory.
 *
 * @param ft  ptr to flagtree
 */
void FlagtreeCleanup(flagtree_t *ft)
{
  ft->height = -1;
  free(ft->buf);
}

/**
 * Grow the ft by one level
 *
 * @param ft  ptr to flagtree
 */
void FlagtreeGrow(flagtree_t *ft)
{
  int *old_buf, *new_buf;
  int old_height;
  int i;

  /* store the old values */
  old_height = ft->height;
  old_buf = ft->buf;

  /* allocate space */
  new_buf = (int *) calloc( FT_NODES(old_height+1), sizeof(int) );

  /* lock for writing */
  RwlockWriterLock( ft->lock );
  { /*CS ENTER*/
    ft->height += 1;
    ft->buf = new_buf;
  } /*CS LEAVE*/
  /* unlock for writing */
  RwlockWriterUnlock( ft->lock );
  
  /* set the appropriate flags */
  for (i=0; i<FT_LEAFS(old_height); i++) {
    /* test each leaf in the old ft */
    if ( old_buf[FT_LEAF_TO_IDX(old_height, i)] == 1 ) {
      /* mark this leaf in the new ft as well */
      FlagtreeMark( ft, i, -1);
    }
  }

  /* free the old ft space */
  free(old_buf);
}

/**
 * @pre idx is marked
 */
static int Visit(flagtree_t *ft, int idx, void *arg)
{
  /* preorder: clear current node */
  ft->buf[idx] = 0;
  if ( idx < FT_LEAF_START_IDX(ft->height) ) {
    /* if inner node, descend: */
    int count = 0;
    /* left child */
    if (ft->buf[FT_LEFT_CHILD(idx)] != 0) {
      count += Visit(ft, FT_LEFT_CHILD(idx), arg);
    }
    /* right child */
    if (ft->buf[FT_RIGHT_CHILD(idx)] != 0) {
      count += Visit(ft, FT_RIGHT_CHILD(idx), arg);
    }
    return count;
  } else {
    /* gather leaf of idx */
    ft->gather( FT_IDX_TO_LEAF(ft->height, idx), arg );
    return 1;
  }
}

/**
 * Gather marked leafs recursively,
 * clearing the marks in preorder
 *
 * @param ft  ptr to flagtree
 * @see note of FlagtreeGather
 */
int FlagtreeGatherRec(flagtree_t *ft, flagtree_gather_cb_t gather, void *arg)
{
  ft->gather = gather;
  /* start from root */
  if (ft->buf[0] != 0) {
    return Visit(ft, 0, arg);
  }
  return 0;
};


#ifndef NDEBUG
/**
 * Print a flagtree to stderr
 *
 * @param ft  ptr to flagtree
 */
void FlagtreePrint(flagtree_t *ft)
{
  int lvl, i, j;
  int maxpad = (1<<(ft->height+1))-2;
  int curpad;
  char pad[maxpad+1];

  memset(pad, (int)' ', maxpad);
  curpad = maxpad;
  i=0;
  for (lvl=0; lvl<=ft->height; lvl++) {
    pad[ curpad ] = '\0';
    for (j=0; j<FT_NODES_AT_LEVEL(lvl); j++) {
      fprintf(stderr,"%s", pad);
      if (ft->buf[i] != 0) {
        fprintf(stderr,"(%2d)",i);
      } else {
        fprintf(stderr," %2d ",i);
      }
      fprintf(stderr,"%s",pad);
      i++;
    }
    curpad = curpad - (1<<(ft->height-lvl));
    fprintf(stderr,"\n\n");
  }
}

#endif
