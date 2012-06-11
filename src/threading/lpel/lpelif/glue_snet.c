

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>


#include "debug.h"

/* S-Net threading backend interface */
#include "threading.h"

#include "lpel.h"

/* info dictionary */
#include "info.h"

/* provisional assignment module */
#include "assign.h"

/* monitoring module */
#include "mon_snet.h"

/* put this into assignment/monitoring module */
#include "distribution.h"

#include <lpel/monitor.h>

//#define USE_PRIORITY

static int num_cpus = 0;
static int num_workers = 0;
static int num_others = 0;


static FILE *mapfile = NULL;
static int mon_flags = 0;

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

  do {
  //  snet_entity_t *new_ent;
    SNetEntitySetStop(ent);
    SNetEntityCall(ent);
  //  new_ent = SNetEntityCopy(ent);
  //  SNetEntityDestroy(ent);
  //  ent = new_ent;
  } while(SNetEntityIsRun(ent));

  SNetEntityDestroy(ent);

  return NULL;
}

#ifdef USE_PRIORITY
static int GetPrio(snet_entity_descr_t type)
{
  switch(type) {
    case ENTITY_box:
    case ENTITY_other: return 0;
    default: return 1;
  }
}
#endif

static int GetRandomNumber(int old_index, int n)
{
  int new_index = old_index;
  while(n > 1 && old_index == new_index) {
    new_index = rand() % n;
  }
  return new_index;
}

int SNetThreadingInit(int argc, char **argv)
{
	lpel_config_t config;
	char fname[20+1];
	int i, res;
	char *mon_elts = NULL;
	memset(&config, 0, sizeof(lpel_config_t));


	config.flags = LPEL_FLAG_PINNED;

	for (i=0; i<argc; i++) {
		if(strcmp(argv[i], "-m") == 0 && i + 1 <= argc) {
			/* Monitoring level */
			i = i + 1;
			mon_elts = argv[i];
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
		} else if(strcmp(argv[i], "-thr") == 0 && i + 1 <= argc) {
                        /* Theshold for placement scheduler */
                        i = i + 1;
                        config.threshold = atof(argv[i]);
                }
#ifdef TASK_SEGMENTATION
                else if(strcmp(argv[i], "-seg") == 0 && i+1 <= argc) {
                       /* If task segementation is used, this is the number of
                        * workers assigned for control tasks
                        */
                       i = i +1;
                       config.segmentation = atoi(argv[i]);
                }
#endif
	}


#ifdef USE_LOGGING
	if (mon_elts != NULL) {
		if (strchr(mon_elts, MON_ALL_FLAG) != NULL) {
			mon_flags = (1<<7) - 1;
		} else {
			if (strchr(mon_elts, MON_MAP_FLAG) != NULL) mon_flags |= SNET_MON_MAP;
			if (strchr(mon_elts, MON_TIME_FLAG) != NULL) mon_flags |= SNET_MON_TIME;
			if (strchr(mon_elts, MON_WORKER_FLAG) != NULL) mon_flags |= SNET_MON_WORKER;
			if (strchr(mon_elts, MON_TASK_FLAG) != NULL) mon_flags |= SNET_MON_TASK;
			if (strchr(mon_elts, MON_STREAM_FLAG) != NULL) mon_flags |= SNET_MON_STREAM;
			if (strchr(mon_elts, MON_MESSAGE_FLAG) != NULL) mon_flags |= SNET_MON_MESSAGE;
			if (strchr(mon_elts, MON_LOAD_FLAG) != NULL) mon_flags |= SNET_MON_LOAD;
		}



		if ( mon_flags & SNET_MON_MAP) {
			snprintf(fname, 20, "n%02d_tasks.map", SNetDistribGetNodeId() );
			/* create a map file */
			mapfile = fopen(fname, "w");
			assert( mapfile != NULL);
			(void) fprintf(mapfile, "%s%c", LOG_FORMAT_VERSION, END_LOG_ENTRY);
		}
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
	SNetThreadingMonInit(&config.mon, SNetDistribGetNodeId(), mon_flags);
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
void CreateNewTask(snet_entity_t *ent, int worker)
{
  snet_entity_descr_t type = SNetEntityDescr(ent);
  const char *name = SNetEntityName(ent);
  int location = SNetEntityNode(ent);
  int prio;

  if ( type != ENTITY_other) {
    assert(location != -1);
  } else {
    worker = -1;
  }

#ifdef USE_PRIORITY
  prio = GetPrio(type);
#else
  prio = 0;
#endif

  lpel_task_t *t = LpelTaskCreate(
                      worker,
                      prio,
                      //(lpel_taskfunc_t) func,
                      EntityTask,
                      ent,
                      GetStacksize(type)
                   );


#ifdef USE_LOGGING
  if (mon_flags & SNET_MON_TASK){
    mon_task_t *mt = SNetThreadingMonTaskCreate(
                               LpelTaskGetID(t),
                               name
                     );
    LpelTaskMonitor(t, mt);
  /* if we monitor the task, we make an entry in the map file */
  }

  if ((mon_flags & SNET_MON_MAP) && mapfile) {
    int tid = LpelTaskGetID(t);
    // FIXME: change to binary format
    (void) fprintf(mapfile, "%d%s %d%c", tid, SNetEntityStr(ent), worker, END_LOG_ENTRY);
  }


#endif

  if (type != ENTITY_box && type != ENTITY_fbbuf) {
    LpelTaskPrio(t, 1);
  }


  //FIXME only for debugging purposes
  //fflush(mapfile);

  LpelTaskRun(t);
}

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
  CreateNewTask(ent, -1);
  return 0;
}

/**
 * Spawn a new task
 */
void SNetThreadingInitSpawn(snet_entity_t *ent, int worker)
{
#ifdef MEASUREMENTS
  snet_entity_descr_t type = SNetEntityDescr(ent);
  if(type != ENTITY_other) {
    LpelWorkerAddTask();
  }
#endif
  CreateNewTask(ent, worker);
}

/**
 * Respawn a current task or if the task is on a different worker, create a
 * new task
 */
void SNetThreadingReSpawn(snet_entity_t *ent)
{
  lpel_task_t *t;
  int current_worker;
  int new_worker;

  t = LpelTaskSelf();
  current_worker = LpelTaskWorkerId(t);
  new_worker = LpelTaskMigrationWorkerId(t);

  if(current_worker != new_worker) {
    /* Migrate */
    printf("migrate\n");
    snet_entity_t *new_ent = SNetEntityCopy(ent);

    CreateNewTask(new_ent, new_worker);
  } else {
    /* Stay on the same worker */
    SNetEntitySetRun(ent);
  }
}

int SNetThreadingInitialWorker(snet_info_t *info, int type)
{
#ifdef TASK_SEGMENTATION
  static int wid_set = -1;
  static snet_info_tag_t control_wid;
  static snet_info_tag_t box_wid;

  if(wid_set == -1) {
    control_wid = SNetInfoCreateTag();
    SNetInfoSetTag(info, control_wid, (uintptr_t)0, NULL);
    box_wid = SNetInfoCreateTag();
    SNetInfoSetTag(info, box_wid, (uintptr_t)0, NULL);
    wid_set = 0;
  }

  switch(type) {
    case 0:
      {
        int wid_index = (int)SNetInfoGetTag(info, box_wid);
        return LpelPlacementSchedulerGetWorker(0, wid_index);
      }
    case 1:
      {
        int wid_index = (int)SNetInfoGetTag(info, control_wid);
        return LpelPlacementSchedulerGetWorker(1, wid_index);
      }
    case 2:
      {
        int box_n = LpelPlacementSchedulerNumWorkers(0);
        int control_n = LpelPlacementSchedulerNumWorkers(1);
        int box_index = (int)SNetInfoGetTag(info, box_wid);
        int control_index = (int)SNetInfoGetTag(info, control_wid);
        int new_box_index = (box_index + 1) % box_n;
        int new_control_index = (control_index +1) % control_n;
        SNetInfoSetTag(info, control_wid, (uintptr_t)new_control_index, NULL);
        SNetInfoSetTag(info, box_wid, (uintptr_t)new_box_index, NULL);
        return LpelPlacementSchedulerGetWorker(1, new_control_index);
      }
    case 3:
      {
        int worker = LpelTaskWorkerId();
        int index = LpelPlacementSchedulerGetIndexWorker(1, worker);
        SNetInfoSetTag(info, control_wid, (uintptr_t)index, NULL);
        return index;
      }
    default:
      return 0;
  }
#else
  static snet_info_tag_t wid = -1;
  if(wid == -1) {
    wid = SNetInfoCreateTag();
    SNetInfoSetTag(info, wid, (uintptr_t)0, NULL);
  }
  switch(type) {
    case 0:
    case 1:
      {
        int wid_index = (int)SNetInfoGetTag(info, wid);
        return LpelPlacementSchedulerGetWorker(0, wid_index);
      }
    case 2:
      {
        int n = LpelPlacementSchedulerNumWorkers(0);
        int index = (int)SNetInfoGetTag(info, wid);
        int new_index = (index +1) % n;
        SNetInfoSetTag(info, wid, (uintptr_t)new_index, NULL);
        return LpelPlacementSchedulerGetWorker(0, new_index);
      }
    case 3:
      {
        int worker = LpelTaskWorkerId();
        int index = LpelPlacementSchedulerGetIndexWorker(0, worker);
        SNetInfoSetTag(info, wid, (uintptr_t)index, NULL);
        return index;
      }
    default:
      return 0;
  }
#endif
}

