

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct snet_task_t *snet_entity_t;

/* S-Net threading backend interface */
#include "threading.h"

/* LPEL library interface */
#include "lpel4snet.h"

/* provisional assignment module */
#include "assign.h"



struct snet_entity_t;
struct snet_stream_t;
struct snet_stream_desc_t;
struct snet_stream_iter_t;

static FILE *mapfile = NULL;
static int mon_level = 0;

/* monitoring user events */
static int monevt_boxstart;
static int monevt_boxstop;
static int monevt_syncfirst;
static int monevt_syncdone;


void SNetThreadingEventBoxStart(void) {
  SNetMonEventSignal(monevt_boxstart);
}

void SNetThreadingEventBoxStop(void) {
  SNetMonEventSignal(monevt_boxstop);
}



static size_t SNetEntityStackSize(snet_entity_type_t type)
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
  snet_config_t config;
  char fname[20+1];
  int num_cpus, num_workers=0;
  int i;

  config.flags = SNET_FLAG_PINNED;
  config.node = SNetDistribGetNodeId();

  for (i=0; i<argc; i++) {
    if(strcmp(argv[i], "-m") == 0 && i + 1 <= argc) {
      /* Monitoring level */
      i = i + 1;
      mon_level = atoi(argv[i]);
    } else if(strcmp(argv[i], "-excl") == 0 ) {
      /* Assign realtime priority to workers*/
      config.flags |= SNET_FLAG_EXCLUSIVE;
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
  if ( 0 != SNetGetNumCores( &num_cpus) ) {
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

  SNetLpInit(&config);

  /* register monitoring user events */
  monevt_boxstart  = SNetMonEventRegister("box_start");
  monevt_boxstop   = SNetMonEventRegister("box_stop");
  monevt_syncfirst = SNetMonEventRegister("sync_first");
  monevt_syncdone  = SNetMonEventRegister("sync_done");

  SNetLpStart();

  return 0;
}


void SNetThreadingStop(void)
{
  SNetLpStop();
}


int SNetThreadingProcess(void)
{
  /* following call will block until the workers have finished */
  SNetLpCleanup();
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
  int worker = -1;

  if ( info.type != ENTITY_other) {
    worker = SNetAssignTask( (info.type==ENTITY_box), info.name );
  }

  snet_entity_t *t = SNetEntityCreate(
      worker,
      (snet_entityfunc_t) func,
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
  mon_flags |= SNET_MON_TASK_USREVT;
  switch(mon_level) {
    case 4: mon_flags |= SNET_MON_TASK_STREAMS;
    case 3: mon_flags |= SNET_MON_TASK_TIMES;
    case 2:
      if (info.type==ENTITY_box) {
        SNetEntityMonitor(t, info.name, mon_flags);
        do_mon = 1;
      }
      break;

    case 5:
      if (info.type!=ENTITY_other) {
        SNetEntityMonitor(t, info.name, SNET_MON_TASK_STREAMS | SNET_MON_TASK_TIMES );
        do_mon = 1;
      }
      break;
    default: /*NOP*/;
  }

  if (do_mon && mapfile) {
    int tid = SNetEntityGetUID(t);
    (void) fprintf(mapfile, "%d: %s\n", tid, info.name);
  }

//FIXME only for debugging purposes
  //fflush(mapfile);

  SNetEntityRun(t);
  return 0;
}



