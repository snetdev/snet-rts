/**
 * Streamtab to hold opened streams in the TCB.
 *
 */

#include <stdlib.h>
#include <assert.h>

#include "streamtab.h"


#define STBE_OPEN     'O'
#define STBE_CLOSED   'C'
#define STBE_REPLACED 'R'
#define STBE_DELETED  'D'


#define DIRTY_END   ((streamtbe_t *)-1)



/**
 * Mark a table entry as dirty
 */
static inline void MarkDirty(streamtab_t *tab, streamtbe_t *tbe)
{
  /* only add if not dirty yet */
  if (tbe->dirty == NULL) {
    /*
     * Set the dirty ptr of tbe to the dirty_list ptr of the tab
     * and the dirty_list ptr of the tab to tbe, i.e.,
     * append the tbe at the front of the dirty_list.
     * Initially, dirty_list of the tab is empty DIRTY_END (!= NULL)
     */
    tbe->dirty = tab->dirty_list;
    tab->dirty_list = tbe;
  }
}





/**
 * Initialise a streamtab
 *
 * @param tab         streamtab to initialise
 * @param init_cap2   initial capacity of the streamtab.
 *                    streamtab will fit 2^init_cap2 groups.
 * @pre               init_cap2 >= 0
 */
void StreamtabInit(streamtab_t *tab, int init_cap2)
{

  tab->grp_capacity = (1<<init_cap2);
  /* allocate lookup table, initialized to NULL values */
  tab->lookup = (struct streamgrp **) calloc(
      tab->grp_capacity, sizeof(struct streamgrp *)
      );
  /* point to the first position */
  tab->idx_grp = 0;
  tab->idx_tab = 0;
  /* there is no deleted entry initially */
  tab->cnt_deleted = 0;
  /* dirty list is empty */
  tab->dirty_list = DIRTY_END;

  /* group chain is uninitialized */
}


/**
 * Cleanup a streamtab
 *
 * Frees all the allocated memory for the streamtab internal structures.
 *
 * @param tab   streamtab to cleanup
 */
void StreamtabCleanup(streamtab_t *tab)
{
  int i;
  struct streamgrp *grp;

  /* iterate through lookup table, free groups */
  for (i=0; i<tab->grp_capacity; i++) {
    grp = tab->lookup[i];
    if (grp != NULL) free(grp);
  }
  /* free the lookup table itself */
  free(tab->lookup);
}





/**
 * Add a stream to the streamtab
 *
 * @param tab     tab in which to add a stream
 * @param s       stream to add
 * @param grp_idx OUT param; if grp_idx != NULL, group index
 *                will be written to the given location
 * @return        pointer to the stream table entry
 */
streamtbe_t *StreamtabAdd(streamtab_t *tab, struct stream *s, int *grp_idx)
{
  struct streamgrp *grp;
  streamtbe_t *ste;
  int ret_idx;

  /* first try to obtain an deleted entry */
  if (tab->cnt_deleted > 0) {
    int i,j;
    ste = NULL;
    /* iterate through all table entries from the beginning */
    for(i=0; i<=tab->idx_grp; i++) {
      grp = tab->lookup[i];
      for (j=0; j<STREAMTAB_GRP_SIZE; j++) {
        if (grp->tab[j].state == STBE_DELETED) {
          ste = &grp->tab[j];
          ret_idx = i;
          goto found_deleted;
        }
      }
    }
    found_deleted:
    /* one will be found eventually (lower position than idx_grp/idx_tab) */
    assert(ste != NULL);
    tab->cnt_deleted--;
  
  } else {
    /*
     * No deleted entry could be reused. Get a new entry.
     * Assume that tab->idx_grp and tab->idx_tab refer to
     * the next free position, but it has not to exist yet
     */

    /* check if the current capacity is sufficient */
    if (tab->idx_grp == tab->grp_capacity) {
      /* double capacity and resize lookup table*/
      struct streamgrp **tmp;
      int i;

      tab->grp_capacity *= 2;
      tmp = (struct streamgrp **) realloc(
          tab->lookup, tab->grp_capacity * sizeof(struct streamgrp *)
          );

      if (tmp == NULL) {
        /*TODO fatal error */
      }
      tab->lookup = tmp;

      /* initialize newly allocated mem to NULL */
      for (i=tab->idx_grp; i<tab->grp_capacity; i++) {
        tab->lookup[i] = NULL;
      }
    }


    /* if the grp does not exist yet, we have to create a new one */
    if (tab->lookup[tab->idx_grp] == NULL) {
      /* if that is the case, idx_tab must be 0 as well */
      assert( tab->idx_tab == 0 );

      grp  = (struct streamgrp *) malloc( sizeof(struct streamgrp) );
      grp->next = NULL;
      /* put in lookup table */
      tab->lookup[tab->idx_grp] = grp;
    } else {
      grp = tab->lookup[tab->idx_grp];
    }
    /* get a ptr to the table entry */
    ste = &grp->tab[tab->idx_tab];
    ret_idx = tab->idx_grp;

    /* increment the next-free position */
    tab->idx_tab++;
    if (tab->idx_tab == STREAMTAB_GRP_SIZE) {
      /* wrap */
      tab->idx_tab = 0;
      tab->idx_grp++;
    }
  }
  /* ste points to the right table entry */
  /* ret_idx contains the group number of ste */

  /* tab out parameter grp_idx */
  if (grp_idx != NULL) *grp_idx = ret_idx;
  
  /* fill the entry */
  ste->s = s;
  ste->state = STBE_OPEN;
  ste->cnt = 0;
  ste->dirty = NULL;

  /* mark the entry as dirty */
  MarkDirty(tab, ste);

  /* return the pointer to the table entry */
  return ste;
}


/**
 * Signal a state change for the stream
 *
 * Increments the counter of teh table entry and marks it dirty
 */
void StreamtabEvent(streamtab_t *tab, streamtbe_t *tbe)
{
  assert( tbe->state == STBE_OPEN || tbe->state == STBE_REPLACED );

  tbe->cnt++;
  MarkDirty(tab, tbe);
}


/**
 * Remove a table entry
 *
 * It remains in the tab as closed and marked dirty,
 * until the next call to StreamtabPrint
 */
void StreamtabRemove(streamtab_t *tab, streamtbe_t *tbe)
{
  assert( tbe->state == STBE_OPEN || tbe->state == STBE_REPLACED );

  tbe->state = STBE_CLOSED;
  MarkDirty(tab, tbe);
}

/**
 * Replace a stream in the stream table entry
 *
 * Retabs counter, marks it as replaced and dirty
 */
void StreamtabReplace(streamtab_t *tab, streamtbe_t *tbe, struct stream *s)
{
  assert( tbe->state == STBE_OPEN || tbe->state == STBE_REPLACED );

  tbe->s = s;
  tbe->cnt = 0;
  tbe->state = STBE_REPLACED;
  MarkDirty(tab, tbe);
}



/**
 * Start collecting groups in the chain
 *
 * @note  changes only hnd and count of tab->chain
 */
void StreamtabChainStart(streamtab_t *tab)
{
  tab->chain.hnd = NULL;
  tab->chain.count = 0;
}

/**
 * Add to the chain of groups
 *
 * The groups are collected in a circular singly linked list.
 * The chain handle always points to the last group in the list.
 *
 * @note  changes only hnd and count of tab->chain
 * @pre   grp_idx is only added once, the grp with grp_idx exists
 */
void StreamtabChainAdd(streamtab_t *tab, int grp_idx)
{
  struct streamgrp *grp;

  grp = tab->lookup[grp_idx];
  assert( grp != NULL );

  if (tab->chain.hnd == NULL) {
    /* chain is empty */
    tab->chain.hnd = grp;
    grp->next = grp; /* self-loop */
  } else {
    /* insert grp between last (=tab->chain)
       and first (=tab->chain->next) */
    grp->next = tab->chain.hnd->next;
    tab->chain.hnd->next = grp;
    /* chain always points to last grp */
    tab->chain.hnd = grp;
  }
  
  /* count the added tbes */
  {
    int max, i;
    max = ( tab->idx_grp==grp_idx && tab->idx_tab!=0 )
      ? tab->idx_tab : STREAMTAB_GRP_SIZE;
    /* omit the closed and deleted tbes */
    for (i=0; i<max; i++) {
      char st = grp->tab[i].state;
      if (st == STBE_OPEN || st == STBE_REPLACED) {
        tab->chain.count++;
      }
    }
  }
}

/**
 * Check if chain of streamtab is empty
 */
int StreamtabChainNotEmpty(streamtab_t *tab)
{
  return tab->chain.count > 0;
}


/**
 *
 */
void StreamtabIterateStart(streamtab_t *tab, streamtbe_iter_t *iter)
{
  iter->grp = NULL;
  iter->tab_idx = 0;
  iter->count = 0;
  if (tab->chain.hnd != NULL) {
    iter->grp = tab->chain.hnd->next;
  }
}


/**
 * @return the number of entries left to consume
 */
int StreamtabIterateHasNext(streamtab_t *tab, streamtbe_iter_t *iter)
{
  return tab->chain.count - iter->count;
}


/**
 * get the next tbe in the chain
 */
streamtbe_t *StreamtabIterateNext(streamtab_t *tab, streamtbe_iter_t *iter)
{
  streamtbe_t *next;

  //TODO omit deleted!
  do {
  next = &iter->grp->tab[ iter->tab_idx ];

  /* advance next */
  iter->tab_idx++;
  if (iter->tab_idx == STREAMTAB_GRP_SIZE) {
    iter->tab_idx = 0;
    iter->grp = iter->grp->next;
  }
  } while ( next->state == STBE_DELETED || next->state == STBE_CLOSED );
  
  iter->count++;
  return next;
}




/**
 * Prints information about the dirty table entries
 *
 * @param tab   the tab for which the information is to be printed
 * @param file  the file where to print the information to, or NULL
 *              if only the dirty list should be cleared
 * @pre         if file != NULL, it must be open for writing
 */
void StreamtabPrint(streamtab_t *tab, FILE *file)
{
  streamtbe_t *tbe, *next;

  /* Iterate through dirty list */
  tbe = tab->dirty_list;
  
  if (file!=NULL) {
    fprintf( file,"[" );
  }

  while (tbe != DIRTY_END) {
    /* print tbe */
    if (file!=NULL) {
      fprintf( file,
          "%p,%c,%lu;",
          tbe->s, tbe->state, tbe->cnt
          );
    }
    /* update states */
    if (tbe->state == STBE_REPLACED) { tbe->state = STBE_OPEN; }
    if (tbe->state == STBE_CLOSED) {
      tbe->state = STBE_DELETED;
      tab->cnt_deleted++;
    }
    /* get the next dirty entry, and clear the link in the current entry */
    next = tbe->dirty;
    tbe->dirty = NULL;
    tbe = next;
  }
  if (file!=NULL) {
    fprintf( file,"] " );
  }
  /* dirty_list is now empty again */
  tab->dirty_list = DIRTY_END;
}

#ifndef NDEBUG
/**
 * Print complete state of streamtab for debugging purposes
 */
void StreamtabDebug(streamtab_t *tab)
{
  int i,j;
  streamtbe_t *tbe;
  struct streamgrp *grp;

  fprintf(stderr, "\n=== Streamtab state (debug) ============\n");

  fprintf(stderr,
    "Group capacity %d\n"
    "idx_grp %d, idx_tab %d\n"
    "dirty_list %p\n",
    tab->grp_capacity,
    tab->idx_grp, tab->idx_tab,
    tab->dirty_list
    );

  fprintf(stderr,"\nLookup:\n");
  for (i=0; i<tab->grp_capacity; i++) {
    fprintf(stderr,"[%d] %p\n",i, tab->lookup[i]);
  }
  
  fprintf(stderr,"\nDirty list state:\n");
  tbe = tab->dirty_list;
  while (tbe != DIRTY_END) {
    fprintf(stderr, "-> [%p]", tbe);
    tbe = tbe->dirty;
  }
  fprintf(stderr, "-> [%p]\n", tbe);

  for (i=0; i<tab->grp_capacity; i++) {
    grp = tab->lookup[i];
    if (grp != NULL) {
      fprintf(stderr,"\nGroup [%d: %p]\n",i, tab->lookup[i]);
      for (j=0; j<STREAMTAB_GRP_SIZE; j++) {
        tbe = &grp->tab[j];
        if (j>=tab->idx_tab && i>=tab->idx_grp) {
          fprintf(stderr, "[%d: %p]\n", j, tbe);
          continue;
        }
        fprintf(stderr, "[%d: %p] stream %p  state %c  cnt %lu  dirty %p\n",
            j, tbe, tbe->s, tbe->state, tbe->cnt, tbe->dirty
            );
      }
    }
  }

  fprintf(stderr, "\n========================================\n");
}
#endif
