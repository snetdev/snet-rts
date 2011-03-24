

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


/* S-Net threading backend interface */
#include "threading.h"

/* LPEL library interface */
#include "lpel.h"

/* provisional assignment module */
#include "assign.h"

extern int SNetNodeLocation;

struct snet_entity_t;
struct snet_stream_t;
struct snet_stream_desc_t;
struct snet_stream_iter_t;

static FILE *mapfile = NULL;
static int mon_level = 0;


static size_t SNetEntityStackSize(snet_entity_type_t type)
{
  size_t stack_size;

  switch(type) {
    case ENTITY_parallel:
    case ENTITY_star:
    case ENTITY_split:
    case ENTITY_sync:
    case ENTITY_filter:
    case ENTITY_collect:
      stack_size = 256*1024; /* 256KB, HIGHLY EXPERIMENTAL! */
      break;
    case ENTITY_box:
    case ENTITY_other:
      stack_size = 8*1024*1024; /*8MB*/
      break;
    default:
      /* we do not want an unhandled case here */
      assert(0);
  }

 return( stack_size);   
}


int SNetThreadingInit(int argc, char **argv)
{
  lpel_config_t config;
  char fname[20+1];
  int num_cpus, num_workers=0;
  int i;

  config.flags = LPEL_FLAG_PINNED;
  config.node = SNetNodeLocation;
  
  for (i=0; i<argc; i++) {
    if(strcmp(argv[i], "-m") == 0 && i + 1 <= argc) {
      /* Monitoring level */
      i = i + 1;
      mon_level = atoi(argv[i]);
    } else if(strcmp(argv[i], "-excl") == 0 ) {
      /* Assign realtime priority to workers*/
      config.flags |= LPEL_FLAG_EXCLUSIVE;
    } else if(strcmp(argv[i], "-w") == 0 && i + 1 <= argc) {
      /* Number of workers */
      i = i + 1;
      num_workers = atoi(argv[i]);
    }
  }

  config.worker_dbg = (mon_level >= 5)? 1 : 0;

  if ( mon_level > 0) {
    if (config.node < 0) {
      snprintf(fname, 20, "tasks.map");
    } else {
      snprintf(fname, 20, "n%02d_tasks.map", config.node);
    }
    /* create a map file */
    mapfile = fopen(fname, "w");
  }

  /* determine number of cpus */
  if ( 0 != LpelGetNumCores( &num_cpus) ) {
    //FIXME SNetUtilDebugFatal("Could not determine number of cores!\n");
    assert(0);
  }

  if (num_workers == 0) {
    config.proc_workers = num_cpus;
    //config.num_workers = num_cpus + 1;
    config.num_workers = num_cpus;
    config.proc_others = 0;
  } else {
    config.proc_workers = num_cpus;
    config.num_workers = num_workers;
    config.proc_others = 0;
  }


  SNetAssignInit(config.num_workers);

  LpelInit(&config);

  return 0;
}


void SNetThreadingStop(void)
{
  LpelStop();
}


int SNetThreadingProcess(void)
{
  /* following call will block until the workers have finished */
  LpelCleanup();
  return 0;
}




int SNetThreadingCleanup(void)
{
  SNetAssignCleanup();

  if (mapfile) {
    (void) fclose(mapfile);
  }

  return 0;
}


int SNetEntitySpawn(snet_entity_info_t info, snet_entityfunc_t func, void *arg)
{
  int mon_flags;
  int do_mon = 0;
  int worker = SNetAssignTask( (info.type==ENTITY_box), info.name );

  lpel_task_t *t = LpelTaskCreate(
      worker,
      (lpel_taskfunc_t) func,
      arg,
      SNetEntityStackSize(info.type)
      );

  /* monitoring levels:
   * 2: collect information for boxes
   * 3: .. with timestamps
   * 4: .. and stream events
   * 5: like 4, for all (but _other) entities
   */
  
  mon_flags = 0;
  switch(mon_level) {
    case 4: mon_flags |= LPEL_MON_TASK_STREAMS;
    case 3: mon_flags |= LPEL_MON_TASK_TIMES;
    case 2:
      if (info.type==ENTITY_box) {
        LpelTaskMonitor(t, info.name, mon_flags);
        do_mon = 1;
      }
      break;

    case 5:
      if (info.type!=ENTITY_other) {
        LpelTaskMonitor(t, info.name, LPEL_MON_TASK_STREAMS | LPEL_MON_TASK_TIMES );
        do_mon = 1;
      }
      break;
    default: /*NOP*/;
  }

  if (do_mon && mapfile) {
    int tid = LpelTaskGetUID(t);
    (void) fprintf(mapfile, "%d: %s\n", tid, info.name);
  }

  LpelTaskRun(t);
  return 0;
}




/**
 * Let the current entity give up execution
 */
void SNetEntityYield(snet_entity_t *self)
{
  LpelTaskYield( (lpel_task_t *)self );
}


/**
 * Let the current entity exit
 */
void SNetEntityExit(snet_entity_t *self)
{
  LpelTaskExit( (lpel_task_t *)self );
}





snet_stream_t *SNetStreamCreate(int capacity)
{
  return (snet_stream_t *)LpelStreamCreate(capacity);
}



snet_stream_desc_t *SNetStreamOpen(snet_entity_t *entity,
    snet_stream_t *stream, char mode)
{
  return (snet_stream_desc_t *) LpelStreamOpen(
      (lpel_task_t *) entity,
      (lpel_stream_t *) stream,
      mode
      );
}


void SNetStreamClose(snet_stream_desc_t *sd, int destroy_stream)
{
  LpelStreamClose(
      (lpel_stream_desc_t *)sd,
      destroy_stream
      );
}




void SNetStreamReplace(snet_stream_desc_t *sd, snet_stream_t *new_stream)
{
  LpelStreamReplace(
      (lpel_stream_desc_t *) sd,
      (lpel_stream_t *) new_stream
      );
}


void *SNetStreamRead(snet_stream_desc_t *sd)
{
  return LpelStreamRead( (lpel_stream_desc_t *)sd );
}


void *SNetStreamPeek(snet_stream_desc_t *sd)
{
  return LpelStreamPeek( (lpel_stream_desc_t *)sd );
}


void SNetStreamWrite(snet_stream_desc_t *sd, void *item)
{
  return LpelStreamWrite( (lpel_stream_desc_t *)sd , item );
}


int SNetStreamTryWrite(snet_stream_desc_t *sd, void *item)
{
  //FIXME
  assert(0);
  return 0;
}


snet_stream_desc_t *SNetStreamPoll(snet_streamset_t *set)
{
  return (snet_stream_desc_t *) LpelStreamPoll(
      (lpel_streamset_t *) set
      );
}



void SNetStreamsetPut(snet_streamset_t *set, snet_stream_desc_t *sd)
{
  LpelStreamsetPut(
      (lpel_streamset_t *) set,
      (lpel_stream_desc_t *) sd
      );
}



int SNetStreamsetRemove(snet_streamset_t *set, snet_stream_desc_t *sd)
{
  return LpelStreamsetRemove(
      (lpel_streamset_t *) set,
      (lpel_stream_desc_t *) sd
      );
}



snet_stream_iter_t *SNetStreamIterCreate(snet_streamset_t *set)
{
  return (snet_stream_iter_t *) LpelStreamIterCreate(
      (lpel_streamset_t *) set
      );
}



void SNetStreamIterDestroy(snet_stream_iter_t *iter)
{
  LpelStreamIterDestroy( (lpel_stream_iter_t *) iter );
}



void SNetStreamIterReset(snet_stream_iter_t *iter, snet_streamset_t *set)
{
  LpelStreamIterReset(
      (lpel_stream_iter_t *) iter,
      (lpel_streamset_t *) set
      );
}



int SNetStreamIterHasNext(snet_stream_iter_t *iter)
{
  return LpelStreamIterHasNext( (lpel_stream_iter_t *) iter );
}


snet_stream_desc_t *SNetStreamIterNext( snet_stream_iter_t *iter)
{
  return (snet_stream_desc_t *) LpelStreamIterNext(
      (lpel_stream_iter_t *) iter
      );
}


void SNetStreamIterAppend( snet_stream_iter_t *iter, snet_stream_desc_t *node)
{
  LpelStreamIterAppend(
      (lpel_stream_iter_t *) iter,
      (lpel_stream_desc_t *) node
      );
}

void SNetStreamIterRemove( snet_stream_iter_t *iter)
{
  LpelStreamIterRemove( (lpel_stream_iter_t *) iter );
}



