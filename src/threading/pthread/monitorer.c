/**
 * Implementation of a monitoring thread for the pthreads backend.
 *
 * 2011/07/29
 * Daniel Prokesch <dlp@snet-home.org>
 */

#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "memfun.h"
#include "moninfo.h"
#include "threading.h"

#ifdef USE_USER_EVENT_LOGGING

struct mlist_node_t {
  struct mlist_node_t *next;
  snet_moninfo_t      *moninfo;
  struct timeval       timestamp;
  const char          *label;
};


typedef struct mlist_node_t mlist_node_t;



/** static global variables */

static mlist_node_t    *queue;
static pthread_mutex_t  queue_lock;
static pthread_cond_t   queue_cond;
static bool queue_term;

static mlist_node_t    *free_list;
static pthread_mutex_t free_list_lock;

static pthread_t mon_thread;

static FILE *mon_file;
static struct timeval mon_reftime;

static void *MonitorThread(void *arg);


static inline void GetTimestamp(struct timeval *t)
{
  struct timeval now;

  t->tv_sec  = mon_reftime.tv_sec;
  t->tv_usec = mon_reftime.tv_usec;

  gettimeofday(&now, NULL);
  if (now.tv_usec < mon_reftime.tv_usec) {
    now.tv_usec += 1000000L;
    now.tv_sec  -= 1L;
  }
  t->tv_usec = now.tv_usec - t->tv_usec;
  t->tv_sec  = now.tv_sec  - t->tv_sec;
}


static mlist_node_t *GetFree(void)
{
  mlist_node_t *node = NULL;

  pthread_mutex_lock(&free_list_lock);
  if (free_list != NULL) {
    /* pop free node off */
    node = free_list;
    free_list = node->next; /* can be NULL */
  }
  pthread_mutex_unlock(&free_list_lock);
  /* if no free node, allocate new node */
  if (node == NULL) {
    node = SNetMemAlloc(sizeof(mlist_node_t));
  }
  return node;
}

static void PutFree(mlist_node_t *node)
{
  pthread_mutex_lock(&free_list_lock);
  if ( free_list == NULL) {
    node->next = NULL;
  } else {
    node->next = free_list;
  }
  free_list = node;
  pthread_mutex_unlock(&free_list_lock);
}

static void FreeFree(void)
{
  mlist_node_t *node;
  /* free all free nodes */
  pthread_mutex_lock( &free_list_lock);
  while (free_list != NULL) {
    /* pop free node off */
    node = free_list;
    free_list = node->next; /* can be NULL */
    /* free the memory for the node */
    SNetMemFree( node);
  }
  pthread_mutex_unlock( &free_list_lock);
}


static void EnqueueSignal(mlist_node_t *node)
{
  /* ENQUEUE & SIGNAL */
  pthread_mutex_lock(&queue_lock);
  if ( queue == NULL) {
    /* list is empty */
    queue = node;
    node->next = node; /* self-loop */
    /* signal monitor thread */
    pthread_cond_signal(&queue_cond);
  } else {
    /* insert between last node=queue
       and first node=queue->next */
    node->next = queue->next;
    queue->next = node;
    queue = node;
  }
  pthread_mutex_unlock(&queue_lock);
}


static mlist_node_t *DequeueWait(void)
{
  mlist_node_t *node = NULL;

  pthread_mutex_lock(&queue_lock);
  while( queue == NULL && queue_term == false) {
      pthread_cond_wait(&queue_cond, &queue_lock);
  }


  /* get first node (handle points to last) */
  if (queue != NULL) {
    node = queue->next;
    if (node == queue) {
      /* self-loop, just single node */
      queue = NULL;
    } else {
      queue->next = node->next;
    }
  }
  assert(node != NULL || queue_term);
  pthread_mutex_unlock(&queue_lock);

  return node;

}


void SNetThreadingMonitoringInit(char *fname)
{
  /* initialize lists */
  queue = NULL;
  free_list = NULL;

  /* initialize mutexes, condvar */
  pthread_mutex_init( &free_list_lock, NULL);
  pthread_mutex_init( &queue_lock, NULL);
  pthread_cond_init(  &queue_cond, NULL);

  mon_file = fopen(fname, "w");
  assert(mon_file != NULL);

  /* get reference time */
  gettimeofday(&mon_reftime, NULL);

  /* Spawn the monitoring thread */
  (void) pthread_create( &mon_thread, NULL, MonitorThread, NULL/*arg*/);
  //(void) pthread_detach( mon_thread );

}



void SNetThreadingMonitoringCleanup(void)
{
  /* signal the monitoring thread to terminate */
  pthread_mutex_lock(&queue_lock);
  queue_term = true;
  pthread_cond_signal(&queue_cond);
  pthread_mutex_unlock(&queue_lock);

  pthread_join(mon_thread, NULL);

  /* upon termination, deallocate the list nodes */
  FreeFree();
  /* destroy sync primitives */
  pthread_mutex_destroy( &free_list_lock);
  pthread_mutex_destroy( &queue_lock);
  pthread_cond_destroy(  &queue_cond);

  /* close the file */
  fclose(mon_file);
}




void SNetThreadingMonitoringAppend(snet_moninfo_t *moninfo, const char *label)
{
  assert(moninfo != NULL);

  mlist_node_t *node = GetFree();

  /* set the data */
  node->moninfo = moninfo;
  /* copy str */
  node->label = strcpy(
      SNetMemAlloc( (strlen(label)+1) * sizeof(char) ),
      label
      );

  /* get current timestamp */
  GetTimestamp(&node->timestamp);

  EnqueueSignal(node);
}



static void ProcessMonInfo(snet_moninfo_t *moninfo, struct timeval *timestamp, const char *label)
{
  /* timestamp */
  fprintf(mon_file,
      "%lu.%06lu %s ",
      timestamp->tv_sec, (unsigned long) timestamp->tv_usec,
      label
      );
  /* moninfo */
  SNetMonInfoPrint(mon_file, moninfo);
  fprintf(mon_file,"\n");
}


/**
 * Monitoring thread
 *
 * There exists only one.
 */
static void *MonitorThread(void *arg)
{
  mlist_node_t *node = NULL;
  (void) arg; /* NOT USED */

  while(1) { /* MAIN EVENT LOOP */

    /* read from the queue */
    node = DequeueWait();

    /* processed all requests and termination signalled */
    if (node==NULL) break;


    /* now process the moninfo */
    ProcessMonInfo(node->moninfo, &node->timestamp, node->label);

    /* destroy the moninfo */
    SNetMonInfoDestroy(node->moninfo);
    /* free the label */
    SNetMemFree( (void*) node->label);

    /* free node */
    PutFree(node);


  } /* END MAIN LOOP */

  return NULL;
}

#else

void SNetThreadingMonitoringCleanup(void)
{
}
#endif

