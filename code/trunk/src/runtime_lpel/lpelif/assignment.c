
#include <pthread.h>

#include "snettypes.h"
#include "bool.h"

#include "lpel.h"


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int num_workers;
static int box_next;


/**
 * Initialize assignment
 */
void AssignmentInit(int lpel_num_workers)
{
  num_workers = lpel_num_workers;
  box_next = 0;
}

/**
 * Get the worker id to assign a task to
 */
int AssignmentGetWID(lpel_taskreq_t *t, bool is_box)
{
  int target = 0;

  pthread_mutex_lock( &lock);
  {
    target = box_next;
    if (is_box) {
      box_next += 1;
      if (box_next == num_workers) box_next = 0;
    }
  }
  pthread_mutex_unlock( &lock);

  return target;
}
