
#include <string.h>
#include <pthread.h>

#include "snettypes.h"
#include "bool.h"
#include "memfun.h"
#include "lpel.h"


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int num_workers;
static int box_next;
static int non_box;
static int *boxcnt;

static int blockcnt = 0;

typedef struct nameround {
  struct nameround *next;
  char *boxname;
  int   round;
} nameround_t;


static nameround_t *head = NULL;

static int findMinBoxes( void)
{
  int i, i_min=0, minval=0;
  for (i=0; i<num_workers; i++) {
    if (boxcnt[i] < minval) {
      minval = boxcnt[i];
      i_min = i;
    }
  }
  return i_min;
}

/**
 * Initialize assignment
 */
void AssignmentInit(int lpel_num_workers)
{
  num_workers = lpel_num_workers;
  boxcnt = (int*) SNetMemAlloc( num_workers * sizeof(int));
  non_box = num_workers -1;
  box_next = 0;
}

void AssignmentCleanup( void)
{
  SNetMemFree( boxcnt);
}

/**
 * Get the worker id to assign a task to
 */
int AssignmentGetWID(lpel_taskreq_t *t, bool is_box, char *boxname)
{
  int target = 0;


#if 0
  pthread_mutex_lock( &lock);
  {
    target = box_next;
    if (is_box) {
//      blockcnt++;
//      if (blockcnt == 100) {
//        blockcnt = 0;
        {
          box_next += 1;
          if (box_next == num_workers) box_next = 0;
        }
//      }
    }
  }
  pthread_mutex_unlock( &lock);

#else  
  pthread_mutex_lock( &lock);
  if (is_box) {
    /* lookup name in list */
    nameround_t *n = head;
    while (n != NULL) {
      if ( 0 == strcmp(n->boxname, boxname)) break;
      n = n->next;
    }
    if (n == NULL) {
      /* create entry */
      n = (nameround_t *) SNetMemAlloc( sizeof(nameround_t));
      n->round = findMinBoxes();
      n->boxname = boxname;
      n->next = head;
      head = n;
    }
    target = n->round;
    n->round = (n->round + 1) % num_workers;
    boxcnt[target]++;
  } else {
    target = non_box;
    non_box = (non_box + 1) % num_workers;
  }
  pthread_mutex_unlock( &lock);
#endif

  return target;
}
