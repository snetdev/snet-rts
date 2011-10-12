

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

#include "monitor.h"

static int num_cpus = 0;
static int num_workers = 0;
static int num_others = 0;


static FILE *mapfile = NULL;
static int mon_level = 0;

/**
 * use the Distributed S-Net placement operators for worker placement
 */
static bool dloc_placement = false;


static size_t GetStacksize(snet_entity_descr_t descr)
{
  size_t stack_size;

  switch(descr) {
    case ENTITY_parallel:
    case ENTITY_star:
    case ENTITY_split:
    case ENTITY_fbcoll:
    case ENTITY_fbdisp:
    case ENTITY_fbbuf:
    case ENTITY_sync:
    case ENTITY_filter:
    case ENTITY_nameshift:
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


static void *EntityTask(void *arg)
{
  snet_entity_t *ent = (snet_entity_t *)arg;

  SNetEntityCall(ent);
  SNetEntityDestroy(ent);

  return NULL;
}

int SNetThreadingInit(int argc, char **argv)
{
  lpel_config_t config;
  char fname[20+1];
  int i, res;

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


#ifdef USE_LOGGING
  if ( mon_level >= MON_MAP_LEVEL) {
    snprintf(fname, 20, "n%02d_tasks.map", SNetDistribGetNodeId() );
    /* create a map file */
    mapfile = fopen(fname, "w");
  }
#endif
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
    config.proc_workers = num_cpus;
    config.num_workers = num_workers;
    config.proc_others = num_others;
  }
  num_workers = config.num_workers;

#ifdef USE_LOGGING
  /* initialise monitoring module */
  SNetThreadingMonInit(&config.mon, SNetDistribGetNodeId(), mon_level);
#endif

  SNetAssignInit(config.num_workers);

  res = LpelInit(&config);
  if (res != LPEL_ERR_SUCCESS) {
    SNetUtilDebugFatal("Could not initialize LPEL!\n");
  }
  LpelStart();

  return 0;
}




unsigned long SNetThreadingGetId()
{
  /* FIXME more convenient way */
  /* returns the thread id */
  return (unsigned long) LpelTaskSelf();
}



int SNetThreadingStop(void)
{
  /* send a stop signal to LPEL */
  LpelStop();
  /* following call will block until the workers have finished */
  LpelCleanup();
  return 0;
}


int SNetThreadingCleanup(void)
{
  SNetAssignCleanup();

#ifdef USE_LOGGING
  /* Cleanup monitoring module */
  SNetThreadingMonCleanup();
  if (mapfile) {
    (void) fclose(mapfile);
  }
#endif

  return 0;
}


/**
 * Signal an user event
 */
void SNetThreadingEventSignal(snet_entity_t *ent, snet_moninfo_t *moninfo)
{
  (void) ent; /* NOT USED */
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
int SNetThreadingSpawn(snet_entity_t *ent)
/*
  snet_entity_type_t type,
  snet_locvec_t *locvec,
  int location,
  const char *name,
  snet_entityfunc_t func,
  void *arg
  )
  */
{
  int worker = -1;
  snet_entity_descr_t type = SNetEntityDescr(ent);
  int location = SNetEntityNode(ent);
  const char *name = SNetEntityName(ent);

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
      //(lpel_taskfunc_t) func,
      EntityTask,
      ent,
      GetStacksize(type)
      );


#ifdef USE_LOGGING
  /* saturate mon level */
  if (mon_level > MON_NUM_LEVEL) mon_level = MON_NUM_LEVEL;
  if (mon_level >= MON_BOX_LEVEL) {
	  if (mon_level >= MON_ALL_ENTITY_LEVEL  || type == ENTITY_box) {
		  mon_task_t *mt = SNetThreadingMonTaskCreate(
				  LpelTaskGetID(t),
				  name
		  );
		  LpelTaskMonitor(t, mt);
		  /* if we monitor the task, we make an entry in the map file */
	  }
  }
  if (mon_level >= MON_MAP_LEVEL && mapfile) {
	  int tid = LpelTaskGetID(t);
	  (void) fprintf(mapfile, "%d %s %d\n", tid, SNetEntityStr(ent), worker);
  }


#endif

  if (type != ENTITY_box && type != ENTITY_fbbuf) {
    LpelTaskPrio(t, 1);
  }


//FIXME only for debugging purposes
  //fflush(mapfile);

  LpelTaskRun(t);
  return 0;
}



