#ifndef _STREAMTAB_H_
#define _STREAMTAB_H_

#include <stdio.h>


#define STREAMTAB_GRP_SIZE  4



/* for convenience */
struct stream;


/**
 * stream table entry
 */
struct streamtbe {
  struct stream *s;
  char state;
  unsigned long cnt;
  struct streamtbe *dirty;  /* for chaining 'dirty' entries */
};

typedef struct streamtbe streamtbe_t;

struct streamgrp {
  struct streamgrp *next;  /* for the chain */
  streamtbe_t tab[STREAMTAB_GRP_SIZE];
};


typedef struct {
  int grp_capacity;
  struct streamgrp **lookup;  /* array of pointers to the grps */
  int idx_grp;                /* current grp for next free entry */
  int idx_tab;                /* current index of the tab within
                                 the grp for next free entry */
  int cnt_deleted;            /* number of deleted table entries */
  streamtbe_t *dirty_list;    /* start of 'dirty' list */
  struct {
    struct streamgrp *hnd;    /*   handle to the chain */
    int count;                /*   total number of chained table entries */
  } chain;                    /* chain of groups */
} streamtab_t;

typedef struct {
  struct streamgrp *grp;  /* pointer to grp for next */
  int tab_idx;            /* current table index within grp */
  int count;              /* number of consumed table entries */
} streamtbe_iter_t;


extern void StreamtabInit(streamtab_t *tab, int init_cap2);
extern void StreamtabCleanup(streamtab_t *tab);
extern streamtbe_t *StreamtabAdd(streamtab_t *tab,
    struct stream *s, int *grp_idx);

extern void StreamtabEvent(streamtab_t *tab, streamtbe_t *tbe);
extern void StreamtabRemove(streamtab_t *tab, streamtbe_t *tbe);
extern void StreamtabReplace(streamtab_t *tab, streamtbe_t *tbe,
    struct stream *s);

extern void StreamtabChainStart(streamtab_t *tab);
extern void StreamtabChainAdd(streamtab_t *tab, int grp_idx);
extern int StreamtabChainNotEmpty(streamtab_t *tab);

extern void StreamtabIterateStart(streamtab_t *tab, streamtbe_iter_t *iter);
extern int  StreamtabIterateHasNext(streamtab_t *tab, streamtbe_iter_t *iter);
extern streamtbe_t *StreamtabIterateNext(streamtab_t *tab,
    streamtbe_iter_t *iter);

extern void StreamtabPrint(streamtab_t *tab, FILE *file);

#ifndef NDEBUG
extern void StreamtabDebug(streamtab_t *tab);
#endif


#endif /* _STREAMTAB_H_ */
