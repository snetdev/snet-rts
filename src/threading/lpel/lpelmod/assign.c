

#include <string.h>
#include <limits.h>
#include <pthread.h>

#include "snettypes.h"
#include "memfun.h"

#include "mon_snet.h"
#include "assign.h"


static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static int num_workers;
static int box_next;
static int non_box;
static unsigned int *boxcnt = NULL;

/* function to assign box task */
static int (*assign_box) (const char*) = NULL;

/* function to assign non-box task */
static int (*assign_nonbox) () = NULL;



typedef struct nameround {
	struct nameround *next;
	char *boxname;
	int   round;
} nameround_t;


static nameround_t *head = NULL;

static int findMinBoxes( void)
{
	int i;
	unsigned int i_min=0, minval=UINT_MAX;
	for (i=0; i<num_workers; i++) {
		if (boxcnt[i] < minval) {
			minval = boxcnt[i];
			i_min = i;
		}
	}
	return i_min;
}

/*
 * round-robin assign for boxes
 */
static int rr_box(const char *boxname){

	int target = 0;
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
		n->boxname = strdup(boxname);
		n->next = head;
		head = n;
	}
	target = n->round;
	n->round = (n->round + 1) % num_workers;
	boxcnt[target]++;
	return target;
}

/*
 * load-balance assign for boxes
 */
static int lb_box(const char *boxname) {
	// get para boxname to be consistent
	return SNetGetMaxWait();
}

/*
 * round-robin assign for non-boxes
 */
static int rr_nonbox(){
	int target = 0;

	target = non_box;
	non_box = (non_box + 1) % num_workers;

	return target;
}

/*
 * load-balance assign for non-boxes
 */
static int lb_nonbox() {
	return SNetGetMaxWait();
}


/**
 * Initialize assignment
 */
void SNetAssignInit(int lpel_num_workers, int alloc_mode)
{
	int i;
	num_workers = lpel_num_workers;
	boxcnt = (unsigned int*) SNetMemAlloc( num_workers * sizeof(unsigned int));
	for(i=0;i<num_workers;i++) {
		boxcnt[i] = 0;
	}
	non_box = num_workers -1;
	box_next = 0;
	switch (alloc_mode) {
	case ALLOC_RR_MODE:
		assign_box = &rr_box;
		assign_nonbox = &rr_nonbox;
		break;
	case ALLOC_LB_MODE:
		assign_box = &lb_box;
		assign_nonbox = &lb_nonbox;
		break;
	case ALLOC_LB_BOX_MODE:
		assign_box = &lb_box;
		assign_nonbox = &rr_nonbox;
		break;
	default:
		assign_box = &rr_box;
		assign_nonbox = &rr_nonbox;
		break;
	}
}


void SNetAssignCleanup( void)
{
	SNetMemFree( boxcnt);
	while (head != NULL) {
		nameround_t *n = head;
		head = n->next;
		SNetMemFree(n->boxname);
		SNetMemFree(n);
	}
}

/**
 * Get the worker id to assign a task to
 */
int SNetAssignTask(int is_box, const char *boxname)
{
	int target = 0;

#if 0
	pthread_mutex_lock( &lock);
	{
		target = box_next;
		if (is_box) {
			box_next += 1;
			if (box_next == num_workers) box_next = 0;
		}
	}
	pthread_mutex_unlock( &lock);

#else
	pthread_mutex_lock( &lock);
	if (is_box) {
		target = (*assign_box)(boxname);
	}
	else {
		target = (*assign_nonbox)();
	}

	pthread_mutex_unlock( &lock);
#endif

	return target;
}



