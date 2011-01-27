/**
 * MAIN LPEL INTERFACE
 */
#ifndef _LPEL_H_
#define _LPEL_H_



/******************************************************************************/
/*  RETURN VALUES OF LPEL FUNCTIONS                                           */
/******************************************************************************/

#define LPEL_ERR_SUCCESS     0
#define LPEL_ERR_FAIL       -1

#define LPEL_ERR_INVAL       1 /* Invalid argument */
#define LPEL_ERR_ASSIGN      2 /* Cannot assign thread to processors */
#define LPEL_ERR_EXCL        3 /* Cannot assign core exclusively */


/******************************************************************************/
/*  GENERAL CONFIGURATION AND SETUP                                           */
/******************************************************************************/

/**
 * Specification for configuration:
 *
 * num_workers defines the number of worker threads (PThreads) spawned.
 * proc_workers is the number of processors used for workers.
 * proc_others is the number of processors assigned to other than
 *   worker threads.
 * flags:
 *   AUTO - use default setting for num_workers, proc_workers, proc_others
 *   REALTIME - set realtime priority for workers, will succeed only if
 *              there is a 1:1 mapping of workers to procs,
 *              proc_others > 0 and the process has needed privileges.
 */
typedef struct {
  int num_workers;
  int proc_workers;
  int proc_others;
  int flags;
  int node;
  int worker_dbg;
} lpel_config_t;

#define LPEL_FLAG_NONE           (0)
#define LPEL_FLAG_PINNED      (1<<0)
#define LPEL_FLAG_EXCLUSIVE   (1<<1)



typedef struct lpel_thread_t    lpel_thread_t;


int LpelInit( lpel_config_t *cfg);
void LpelCleanup( void);


int LpelGetNumCores( int *result);
int LpelCanSetExclusive( int *result);



/**
 * Aquire a thread from the LPEL
 */
lpel_thread_t *LpelThreadCreate( void (*func)(void *),
    void *arg, int detached);

void LpelThreadJoin( lpel_thread_t *env);


/******************************************************************************/
/*  DATATYPES                                                                 */
/******************************************************************************/

/* task types */

/* the running task */
typedef struct lpel_task_t lpel_task_t;

/* a task request */
typedef struct lpel_taskreq_t lpel_taskreq_t;

/* task function signature */
typedef void (*lpel_taskfunc_t)( lpel_task_t *self, void *inarg);


#define LPEL_TASK_ATTR_NONE                (0)

#define LPEL_TASK_ATTR_JOINABLE         (1<<0)

#define LPEL_TASK_ATTR_MONITOR_OUTPUT   (1<<4)
#define LPEL_TASK_ATTR_MONITOR_TIMES    (1<<5)
#define LPEL_TASK_ATTR_MONITOR_STREAMS  (1<<6)

#define LPEL_TASK_ATTR_MONITOR_ALL  \
  ( LPEL_TASK_ATTR_MONITOR_OUTPUT  |\
    LPEL_TASK_ATTR_MONITOR_TIMES   |\
    LPEL_TASK_ATTR_MONITOR_STREAMS )

/**
 * If a stacksize <= 0 is specified,
 * use the default stacksize
 */
#define LPEL_TASK_ATTR_STACKSIZE_DEFAULT  8192  /* 8k stacksize*/



/* stream types */

typedef struct lpel_stream_t         lpel_stream_t;

typedef struct lpel_stream_desc_t    lpel_stream_desc_t;    

typedef lpel_stream_desc_t          *lpel_stream_list_t;

typedef struct lpel_stream_iter_t    lpel_stream_iter_t;



/******************************************************************************/
/*  TASK FUNCTIONS                                                            */
/******************************************************************************/

lpel_taskreq_t *LpelTaskRequest( lpel_taskfunc_t,
    void *inarg, int flags, int stacksize, int prio);


void  LpelTaskExit(  lpel_task_t *ct, void* joinarg);
void  LpelTaskYield( lpel_task_t *ct);
void* LpelTaskJoin( lpel_task_t *ct, lpel_taskreq_t *child);

unsigned int LpelTaskGetUID( lpel_task_t *t);
unsigned int LpelTaskReqGetUID( lpel_taskreq_t *t);


/******************************************************************************/
/*  WORKER FUNCTIONS                                                          */
/******************************************************************************/

void LpelWorkerTaskAssign( lpel_taskreq_t *t, int wid);
void LpelWorkerWrapperCreate( lpel_taskreq_t *t, char *name);
void LpelWorkerTerminate( void);



/******************************************************************************/
/*  STREAM FUNCTIONS                                                          */
/******************************************************************************/

lpel_stream_t *LpelStreamCreate( void);
void LpelStreamDestroy( lpel_stream_t *s);

lpel_stream_desc_t *
LpelStreamOpen( lpel_task_t *t, lpel_stream_t *s, char mode);

void  LpelStreamClose(   lpel_stream_desc_t *sd, int destroy_s);
void  LpelStreamReplace( lpel_stream_desc_t *sd, lpel_stream_t *snew);
void *LpelStreamPeek(    lpel_stream_desc_t *sd);
void *LpelStreamRead(    lpel_stream_desc_t *sd);
void  LpelStreamWrite(   lpel_stream_desc_t *sd, void *item);

lpel_stream_desc_t *LpelStreamPoll(    lpel_stream_list_t *list);



void LpelStreamListAppend(  lpel_stream_list_t *lst, lpel_stream_desc_t *node);
int  LpelStreamListRemove( lpel_stream_list_t *lst, lpel_stream_desc_t *node);
int  LpelStreamListIsEmpty( lpel_stream_list_t *lst);



lpel_stream_iter_t *LpelStreamIterCreate( lpel_stream_list_t *lst);
void LpelStreamIterDestroy( lpel_stream_iter_t *iter);

void LpelStreamIterReset(   lpel_stream_list_t *lst, lpel_stream_iter_t *iter);
int  LpelStreamIterHasNext( lpel_stream_iter_t *iter);
lpel_stream_desc_t *LpelStreamIterNext( lpel_stream_iter_t *iter);
void LpelStreamIterAppend(  lpel_stream_iter_t *iter, lpel_stream_desc_t *node);
void LpelStreamIterRemove(  lpel_stream_iter_t *iter);




/******************************************************************************/
/*  THREADING FUNCTIONS                                                       */
/******************************************************************************/

extern lpel_thread_t *LpelThreadCreate( void (*func)(void *),
    void *arg, int detached);
extern void LpelThreadJoin( lpel_thread_t *env);





#endif /* _LPEL_H_ */
