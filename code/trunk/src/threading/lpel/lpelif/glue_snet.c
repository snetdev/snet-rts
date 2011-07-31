

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#include "debug.h"

/* S-Net threading backend interface */
#include "threading.h"

#include "lpel.h"

/* provisional assignment module */
#include "assign.h"

/* monitoring module */
#include "mon_snet.h"

/* put this into assignment/monitoring module */
#include "distribution.h"



static int num_cpus = 0;
static int num_workers = 0;
static int num_others = 0;


static FILE *mapfile = NULL;
static int mon_level = 0;

/**
 * use the Distributed S-Net placement operators for worker placement
 */
static bool dloc_placement = false;


static size_t GetStacksize(snet_entity_type_t type)
{
  size_t stack_size;

  switch(type) {
    case ENTITY_parallel:
    case ENTITY_star:
    case ENTITY_split:
    case ENTITY_fbcoll:
    case ENTITY_fbdisp:
    case ENTITY_fbbuf:
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
  int i;

  memset(&config, 0, sizeof(lpel_config_t));


  config.flags = LPEL_FLAG_PINNED;

  for (i=0; i<argc; i++) {
    if(strcmp(argv[i], "-m") == 0 && i + 1 <= argc) {
      /* Monitoring level */
      i = i + 1;
      mon_level = atoi(argv[i]);
    } else if(strcmp(argv[i], "-excl") == 0 ) {
      /* Assign realtime priority to workers*/
      config.flags |= LPEL_FLAG_EXCLUSIVE;
    } else if(strcmp(argv[i], "-dloc") == 0 ) {
      /* Use distributed s-net location placement */
      dloc_placement = true;
    } else if(strcmp(argv[i], "-wo") == 0 && i + 1 <= argc) {
      /* Number of cores for others */
      i = i + 1;
      num_others = atoi(argv[i]);
    } else if(strcmp(argv[i], "-w") == 0 && i + 1 <= argc) {
      /* Number of workers */
      i = i + 1;
      num_workers = atoi(argv[i]);
    }
  }

  config.worker_dbg = (mon_level >= 5)? 1 : 0;

  //FIXME
  if ( mon_level > 0) {
    snprintf(fname, 20, "n%02d_tasks.map", SNetDistribGetNodeId() );
    /* create a map file */
    mapfile = fopen(fname, "w");
  }

  /* determine number of cpus */
  if ( 0 != LpelGetNumCores( &num_cpus) ) {
    SNetUtilDebugFatal("Could not determine number of cores!\n");
    assert(0);
  }

  if (num_workers == 0) {
    config.proc_workers = num_cpus;
    //config.num_workers = num_cpus + 1;
    config.num_workers = num_cpus;
    config.proc_others = num_others;
  } else {
    config.proc_workers = num_workers;
    config.num_workers = num_workers;
    config.proc_others = num_others;
  }
  num_workers = config.num_workers;

  /* initialise monitoring module */
  SNetThreadingMonInit(&config.mon, SNetDistribGetNodeId());
  SNetAssignInit(config.num_workers);

  LpelInit(&config);

  LpelStart();

  return 0;
}




unsigned long SNetThreadingGetId()
{
  /* FIXME more convenient way */
  /* returns the thread id */
  return (unsigned long) LpelTaskSelf();
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

  /* Cleanup monitoring module */
  SNetThreadingMonCleanup();

  if (mapfile) {
    (void) fclose(mapfile);
  }

  return 0;
}


/**
 * Signal an event
 */
void SNetThreadingEventSignal(snet_moninfo_t *moninfo)
{
  lpel_task_t *t = LpelTaskSelf();
  assert(t != NULL);
  mon_task_t *mt = LpelTaskGetMon(t);
  if (mt != NULL) {
    SNetThreadingMonEvent(mt, moninfo);
  }
}





/*****************************************************************************
 * Spawn a new task
 ****************************************************************************/
int SNetEntitySpawn(
  snet_entity_type_t type,
  snet_locvec_t *locvec,
  int location,
  const char *name,
  snet_entityfunc_t func,
  void *arg
  )
{
  int mon_flags;
  int do_mon = 0;
  int worker = -1;
  char locstr[128];


  // if locvec is NULL then entity_other
  assert(locvec != NULL || type == ENTITY_other);

  if (locvec != NULL) {
    (void) SNetLocvecPrint(locstr, 128, locvec);
  } else {
    locstr[0] = '\0';
  }

  if ( type != ENTITY_other) {
    if (dloc_placement) {
      assert(location != -1);
      worker = location % num_workers;
    } else {
      worker = SNetAssignTask( (type==ENTITY_box), name );
    }
  }

  lpel_task_t *t = LpelTaskCreate(
      worker,
      (lpel_taskfunc_t) func,
      arg,
      GetStacksize(type)
      );

  /* monitoring levels:
   * 2: collect information for boxes
   * 3: .. with timestamps
   * 4: .. and stream events
   * 5: like 4, for all (but _other) entities
   * 6: like 5, also with usrevents
   */

  /* saturate mon level */
  if (mon_level > 6) mon_level = 6;

  mon_flags = 0;
  switch(mon_level) {
    case 4: mon_flags |= SNET_MON_TASK_STREAMS;
    case 3: mon_flags |= SNET_MON_TASK_TIMES;
    case 2:
      if (type==ENTITY_box) {

        mon_task_t *mt = SNetThreadingMonTaskCreate(
          LpelTaskGetID(t), name, mon_flags
          );
        LpelTaskMonitor(t, mt);
        do_mon = 1;
      }
      break;

    case 6:
      if (type==ENTITY_box || type==ENTITY_sync) {
        mon_flags = SNET_MON_TASK_USREVT;
      }
    case 5:
      if (type!=ENTITY_other) {
        mon_task_t *mt = SNetThreadingMonTaskCreate(
          LpelTaskGetID(t), name,
          mon_flags | SNET_MON_TASK_STREAMS | SNET_MON_TASK_TIMES
          );
        LpelTaskMonitor(t, mt);
        do_mon = 1;
      }
      break;

    case 1:
      do_mon = 1;
      break;

    default: /*NOP*/;
  }

  if (type != ENTITY_box) {
    LpelTaskPrio(t, 1);
  }

  if (do_mon && mapfile) {
    int tid = LpelTaskGetID(t);
    (void) fprintf(mapfile, "%d %s %s %d\n", tid, locstr, name, worker);
  }

//FIXME only for debugging purposes
  //fflush(mapfile);

  LpelTaskRun(t);
  return 0;
}



