#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "glue_snet.h"
#include "threading.h"
#include "hrc_lpel.h"
#include <lpel/monitor.h>
#include "debug.h"

/* provisional assignment module */
#include "assign.h"

/* monitoring module */
#include "mon_snet.h"

/* put this into assignment/monitoring module */
#include "distribution.h"


static int num_cpus = 0;
static int num_workers = -1;
static int proc_others = 0;
static int proc_workers = -1;
static int mon_flags = 0;
static int rec_lim = 1;
static FILE *mapfile = NULL;

/**
 * use the Distributed S-Net placement operators for worker placement
 */
static bool dloc_placement = false;
static bool sosi_placement = false;

int SNetThreadingInit(int argc, char **argv)
{
	lpel_config_t config;
	int i;
	char *mon_elts = NULL;
	memset(&config, 0, sizeof(lpel_config_t));

	char fname[20+1];
	int priorf = 1;

	config.type = HRC_LPEL;

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
		} else if(strcmp(argv[i], "-co") == 0 && i + 1 <= argc) {
			/* Number of cores for others */
			i = i + 1;
			proc_others = atoi(argv[i]);
		} else if(strcmp(argv[i], "-cw") == 0 && i + 1 <= argc) {
			/* Number of cores for others */
			i = i + 1;
			proc_workers = atoi(argv[i]);
		} else if(strcmp(argv[i], "-w") == 0 && i + 1 <= argc) {
			/* Number of workers */
			i = i + 1;
			num_workers = atoi(argv[i]);
		} else if(strcmp(argv[i], "-sosi") == 0) {
			sosi_placement = true;
		} else if (strcmp(argv[i], "-np") == 0) { // no pinned
			config.flags ^= LPEL_FLAG_PINNED;
		} else if(strcmp(argv[i], "-pf") == 0 && i + 1 <= argc) {
			i = i + 1;
			priorf = atoi(argv[i]);
		} else if(strcmp(argv[i], "-rl") == 0 && i + 1 <= argc) {
			i = i + 1;
			rec_lim = atoi(argv[i]);
		}
	}

	LpelTaskSetPriorityFunc(priorf);

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

	if (num_workers == -1)
		config.num_workers = num_cpus;
	else
		config.num_workers = num_workers;

	if (proc_workers == -1)
		config.proc_workers = num_cpus;
	else
		config.proc_workers = proc_workers;

	config.proc_others = proc_others;

#ifdef USE_LOGGING
	/* initialise monitoring module */
	SNetThreadingMonInit(&config.mon, SNetDistribGetNodeId(), mon_flags);
#endif

	LpelInit(&config);

	if (LpelStart(&config)) SNetUtilDebugFatal("Could not initialize LPEL!");

	return 0;
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
	snet_entity_descr_t type = SNetEntityDescr(ent);
	const char *name = SNetEntityName(ent);
	int location = LPEL_MAP_MASTER;
	int l1 = strlen(SNET_SOURCE_PREFIX);
	int l2 = strlen(SNET_SINK_PREFIX);
	if ((sosi_placement && (strnstr(name, SNET_SOURCE_PREFIX, l1) || strnstr(name, SNET_SINK_PREFIX, l2))) 	// sosi placemnet and entity is source/sink
			|| type == ENTITY_other)	// wrappers
		location = LPEL_MAP_OTHERS;

	lpel_task_t *t = LpelTaskCreate(
			location,
			//(lpel_taskfunc_t) func,
			EntityTask,
			ent,
			GetStacksize(type)
	);

	LpelTaskSetRecLimit(t, rec_lim);

#ifdef USE_LOGGING
	if (mon_flags & SNET_MON_TASK){
		mon_task_t *mt = SNetThreadingMonTaskCreate(
				LpelTaskGetId(t),
				name
		);
		LpelTaskMonitor(t, mt);
		/* if we monitor the task, we make an entry in the map file */
	}

	if ((mon_flags & SNET_MON_MAP) && mapfile) {
		int tid = LpelTaskGetId(t);
		(void) fprintf(mapfile, "%d%s%c", tid, SNetEntityStr(ent), END_LOG_ENTRY);
	}


#endif

	LpelTaskStart(t);
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
