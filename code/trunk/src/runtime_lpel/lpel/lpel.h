/**
 * MAIN LPEL INTERFACE
 */
#ifndef _LPEL_H_
#define _LPEL_H_



/******************************************************************************/
/*  RETURN TYPES OF LPEL FUNCTIONS                                            */
/******************************************************************************/


/******************************************************************************/
/*  GENERAL CONFIGURATION AND SETUP                                           */
/******************************************************************************/

/**
 * Specification for configuration:
 *
 * proc_workers is the number of processors used for workers.
 * num_workers must be a multiple of proc_workers.
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
} lpel_config_t;

#define LPEL_FLAG_AUTO     (1<<0)
#define LPEL_FLAG_AUTO2    (1<<1)
#define LPEL_FLAG_REALTIME (1<<4)



typedef struct lpel_thread_t    lpel_thread_t;


void LpelInit(    lpel_config_t *cfg);
void LpelCleanup( void);


int LpelNumWorkers(void);


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

typedef struct lpel_task_t lpel_task_t;

typedef void (*lpel_taskfunc_t)( lpel_task_t *self, void *inarg);

typedef struct {
  int flags;
  int stacksize;
} lpel_taskattr_t;

#define LPEL_TASK_ATTR_NONE             (   0)
#define LPEL_TASK_ATTR_ALL              (  -1)
#define LPEL_TASK_ATTR_MONITOR_OUTPUT   (1<<0)
#define LPEL_TASK_ATTR_COLLECT_TIMES    (1<<1)
#define LPEL_TASK_ATTR_COLLECT_STREAMS  (1<<2)




/* stream types */

typedef struct lpel_stream_t         lpel_stream_t;

typedef struct lpel_stream_desc_t    lpel_stream_desc_t;    

typedef lpel_stream_desc_t          *lpel_stream_list_t;

typedef struct lpel_stream_iter_t    lpel_stream_iter_t;



/******************************************************************************/
/*  TASK FUNCTIONS                                                            */
/******************************************************************************/

lpel_task_t *LpelTaskCreate( lpel_taskfunc_t,
    void *inarg, lpel_taskattr_t *attr);

void LpelTaskExit(  lpel_task_t *ct);
void LpelTaskYield( lpel_task_t *ct);


/* TODO refactor */

void _LpelWorkerTaskAssign( lpel_task_t *t, int wid);
void _LpelWorkerWrapperCreate( lpel_task_t *t, char *name);
void _LpelWorkerTerminate( void);



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
void  LpelStreamPoll(    lpel_stream_list_t *list);



void LpelStreamListAppend(  lpel_stream_list_t *lst, lpel_stream_desc_t *node);
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
