

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>


#include "debug.h"
#include "memfun.h"

/* S-Net threading backend interface */
#include "threading.h"

#include <lpel.h>

/* provisional assignment module */
#include "assign.h"

/* monitoring module */
#include "mon_snet.h"

/* put this into assignment/monitoring module */
#include "distribution.h"

#include <lpel/monitor.h>

static int num_workers = 0;
static FILE *mapfile = NULL;
static int mon_flags = 0;

/**
 * use the Distributed S-Net placement operators for worker placement
 */
static bool dloc_placement = false;


static size_t GetStacksize(snet_entity_t ent)
{
	size_t stack_size;

	switch(ent) {
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


int SNetThreadingInit(int argc, char **argv)
{
  lpel_config_t config;
  char fname[20+1];
  int num_others = 0;
  char *mon_elts = NULL;

  LpelInit(&config);

  config.flags = LPEL_FLAG_PINNED;
  config.threshold = 0;

  for (int i = 0; i < argc; i++) {
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
    } else if(strcmp(argv[i], "-threshold") == 0 && i + 1 <= argc) {
      /* Threshold for placement scheduler */
      i = i + 1;
      config.threshold = atof(argv[i]);
    } else if(strcmp(argv[i], "-placement") == 0) {
      config.placement = 1;
    }
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

  config.proc_others = num_others;
  config.num_workers = num_workers ? num_workers : config.num_workers;

#ifdef USE_LOGGING
  /* initialise monitoring module */
  SNetThreadingMonInit(&config.mon, SNetDistribGetNodeId(), mon_flags);
#endif

  SNetAssignInit(config.num_workers);

  if (LpelStart(&config)) SNetUtilDebugFatal("Could not initialize LPEL!");

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
void SNetThreadingEventSignal(snet_moninfo_t *moninfo)
{
	lpel_task_t *t = LpelTaskSelf();
	assert(t != NULL);
	mon_task_t *mt = LpelTaskGetMon(t);
	if (mt != NULL) {
		SNetThreadingMonEvent(mt, moninfo);
	}
}

void SNetThreadingSpawn(snet_entity_t ent, int loc, snet_locvec_t *locvec,
                        const char *name, snet_taskfun_t func, void *arg)
{
  int locveclen, namelen, size;
  char *buf = NULL;
  int worker = -1;
  /* if locvec is NULL then entity_other */
  assert(locvec != NULL || ent == ENTITY_other);

  locveclen = locvec ? SNetLocvecPrintSize(locvec) : 0;
  namelen = name ? strlen(name) : 0;
  size = locveclen + namelen + 2;
  buf = SNetMemAlloc(size);
  if (locvec) SNetLocvecPrint(buf, locvec);
  buf[locveclen] = ' ';
  strncpy(buf + locveclen + 1, name, namelen);
  buf[size-1] = '\0';

  if (ent != ENTITY_other) {
    if (dloc_placement) {
      assert(loc != -1);
      worker = loc % num_workers;
    } else {
      worker = SNetAssignTask( (ent==ENTITY_box), name );
    }
  }

  lpel_task_t *t = LpelTaskCreate(worker, func, arg, GetStacksize(ent));

  LpelSetName(t, buf);
  LpelSetNameDestructor(t, &SNetMemFree);

#ifdef USE_LOGGING
  if (mon_flags & SNET_MON_TASK){
    mon_task_t *mt = SNetThreadingMonTaskCreate(LpelTaskGetID(t), name);
    LpelTaskMonitor(t, mt);
    /* if we monitor the task, we make an entry in the map file */
  }

  if ((mon_flags & SNET_MON_MAP) && mapfile) {
    // FIXME: change to binary format
    fprintf(mapfile, "%d%s %d%c", LpelTaskGetID(t), buf, worker, END_LOG_ENTRY);
  }


#endif

#ifndef NO_PRIORITY
  if (ent != ENTITY_box && ent != ENTITY_fbbuf) {
    LpelTaskPrio(t, 1);
  }
#endif


  LpelTaskRun(t);
}

/**
 * Respawn a current task or if the task is on a different worker, create a
 * new task
 */
void SNetThreadingRespawn(snet_taskfun_t f)
{ LpelTaskRespawn(f); }

const char *SNetThreadingGetName(void)
{ return LpelGetName(LpelTaskSelf()); }
