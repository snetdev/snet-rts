

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

typedef struct snet_task_t *snet_entity_t;
typedef enum snet_entitystate_t snet_entitystate_t;


/* S-Net threading backend interface */
#include "threading.h"

/* LPEL library interface */
#include "lpel4snet.h"

/* provisional assignment module */
#include "assign.h"

//FIXME put this into assignment/monitoring module
#include "distribution.h"


struct snet_entity_t;
struct snet_stream_t;
struct snet_stream_desc_t;
struct snet_stream_iter_t;

static FILE *mapfile = NULL;
static int mon_level = 0;


#define SNET_MON_TASK_TIMES   (1<<0)
#define SNET_MON_TASK_STREAMS (1<<1)
#define SNET_MON_TASK_USREVT  (1<<2)

void SNetThreadingEventBoxStart(void) {
//  SNetMonEventSignal(monevt_boxstart);
}

void SNetThreadingEventBoxStop(void) {
//  SNetMonEventSignal(monevt_boxstop);
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

  memset(&config, 0, sizeof(snet_config_t));
  config.flags = SNET_FLAG_PINNED;

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
    snprintf(fname, 20, "n%02d_tasks.map", SNetDistribGetNodeId() );
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
  /*
  monevt_boxstart  = SNetMonEventRegister("box_start");
  monevt_boxstop   = SNetMonEventRegister("box_stop");
  monevt_syncfirst = SNetMonEventRegister("sync_first");
  monevt_syncdone  = SNetMonEventRegister("sync_done");
  */

  SNetLpStart();

  return 0;
}

/**
 * Prefix/postfix for monitoring outfiles
 */
//#define MON_PFIX_LEN  16
//static char monitoring_prefix[MON_PFIX_LEN];
//static char monitoring_postfix[MON_PFIX_LEN];

  /* store the prefix */
  /*
  (void) memset(monitoring_prefix,  0, MON_PFIX_LEN);
  if ( prefix != NULL ) {
    (void) strncpy( monitoring_prefix, prefix, MON_PFIX_LEN);
  }
  */

  /* store the postfix */
  /*
  (void) memset(monitoring_postfix, 0, MON_PFIX_LEN);
  if ( postfix != NULL ) {
    (void) strncpy( monitoring_postfix, postfix, MON_PFIX_LEN);
  }
  */

//TODO Monitoring init
  /* initialise monitoring module */
  //FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
  //char node_prefix[16];
  //(void) snprintf(node_prefix, 16, "mon_n%02d_", _lpel_global_config.node);
  //LpelMonInit(node_prefix, ".log");

  /* Cleanup moitoring module */
  //FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME
  //LpelMonCleanup();


#if 0
#define MON_EVENT_TABLE_DELTA 16
#define MON_USREVT_BUFSIZE 64
typedef struct mon_usrevt_t mon_usrevt_t;
  // in mon_task_t:
  struct {
    int cnt, size;
    int start, end;
    mon_usrevt_t *buffer;
  } events;         /** user-defined events */

struct mon_usrevt_t {
  timing_t ts;
  int evt;
};

/**
 * The event table is used to register user events,
 * where each index in the table maps to the name
 * of the user event
 */
static struct {
  int cnt;
  int size;
  char **tab;
} event_table;
/**
 * Print the user events of a task
 */
static void PrintUsrEvt(mon_task_t *mt)
{
  FILE *file = mt->ctx->outfile;

  int pos = (mt->events.cnt <= mt->events.size)
          ? mt->events.start : ((mt->events.end+1)%(mt->events.size));
  fprintf( file,"%d[", mt->events.cnt );
  //fprintf( file,"(pos %d) (start %d) (end %d) (cnt %d) (size %d); ", pos, mt->events.start, mt->events.end, mt->events.cnt, mt->events.size);
  while (pos != mt->events.end) {
    mon_usrevt_t *cur = &mt->events.buffer[pos];
    /* print cur */
    if FLAG_TIMES(mt) {
      PrintNormTS(&cur->ts, file);
    }
    fprintf( file,"%s; ", event_table.tab[cur->evt]);

    /* increment */
    pos++;
    if (pos == mt->events.size) { pos = 0; }
  }
  /* reset */
  mt->events.start = mt->events.end;
  mt->events.cnt = 0;
  fprintf( file,"] " );
}


/**
 * Must be done before actually using the events,
 * typically before LpelStart()
 *
 * the table is growing dynamically
 */
int LpelMonEventRegister(const char *evt_name)
{
  if (event_table.cnt == event_table.size) {
    /* grow */
    char **new_tab = malloc(
        event_table.size + MON_EVENT_TABLE_DELTA*sizeof(char*)
        );
    event_table.size += MON_EVENT_TABLE_DELTA;
    (void) memcpy(new_tab, event_table.tab, event_table.cnt*sizeof(char*));
    free(event_table.tab);
    event_table.tab = new_tab;
  }

  event_table.tab[event_table.cnt] = strdup(evt_name);
  event_table.cnt++;

  return event_table.cnt - 1;
}



void LpelMonEventSignal(int evt)
{
  lpel_task_t *t = LpelTaskSelf();
  assert(t != NULL);
  mon_task_t *mt = t->mon;

  if (mt && FLAG_EVENTS(mt)) {
    mon_usrevt_t *current = &mt->events.buffer[mt->events.end];
    if FLAG_TIMES(mt) TIMESTAMP(&current->ts);
    current->evt = evt;
    /* update counters */
    mt->events.end++;
    if (mt->events.end == mt->events.size) { mt->events.end = 0; }
    mt->events.cnt += 1;
  }
}

// in task destroy:
  if FLAG_EVENTS(mt) {
    free(mt->events.buffer);
  }

// in taskstop:
  /* print user events */
  if (FLAG_EVENTS(mt) && mt->events.cnt>0 ) {
    PrintUsrEvt(mt);
  }


// in moninit: initialize event table
  event_table.cnt = 0;
  event_table.size = MON_EVENT_TABLE_DELTA;
  event_table.tab = malloc( MON_EVENT_TABLE_DELTA*sizeof(char*) );

// in moncleanup
  int i;
  /* free event table */
  for (i=0; i<event_table.cnt; i++) {
    /* the strings */
    free(event_table.tab[i]);
  }
  /* the array itself */
  free(event_table.tab);

// in taskcreate:
  if FLAG_EVENTS(mt) {
    mt->events.size = MON_USREVT_BUFSIZE;
    mt->events.cnt = 0;
    mt->events.start = 0;
    mt->events.end = 0;
    mt->events.buffer = malloc( mt->events.size * sizeof(mon_usrevt_t));
  }


#endif


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
        //FIXME SNetEntityMonitor(t, info.name, mon_flags);
        do_mon = 1;
      }
      break;

    case 5:
      if (info.type!=ENTITY_other) {
        //FIXME SNetEntityMonitor(t, info.name, SNET_MON_TASK_STREAMS | SNET_MON_TASK_TIMES );
        do_mon = 1;
      }
      break;
    default: /*NOP*/;
  }

  if (do_mon && mapfile) {
    int tid = SNetEntityGetID(t);
    (void) fprintf(mapfile, "%d: %s\n", tid, info.name);
  }

//FIXME only for debugging purposes
  //fflush(mapfile);

  SNetEntityRun(t);
  return 0;
}



