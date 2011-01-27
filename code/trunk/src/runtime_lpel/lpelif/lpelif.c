/* $Id$ */


#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "snetentities.h"
#include "debug.h"

#include "lpelif.h"
#include "assignment.h"

#include "lpel.h"



static FILE *mapfile = NULL;
static int _mon_lvl = 0;


void SNetLpelIfInit( int rank, int num_workers, bool do_excl, int mon_level)
{
  char fname[20+1];
  int num_cpus;
  lpel_config_t config;
  
  _mon_lvl = mon_level;

  config.node = rank;

  config.worker_dbg = (_mon_lvl >= 5)? 1 : 0;

  
  if ( _mon_lvl > 0) {
    if (rank < 0) {
      snprintf(fname, 20, "tasks.map");
    } else {
      snprintf(fname, 20, "n%02d_tasks.map", rank);
    }
    /* create a map file */
    mapfile = fopen(fname, "w");
  }

  /* determine number of cpus */
  if ( 0 != LpelGetNumCores( &num_cpus) ) {
    SNetUtilDebugFatal("Could not determine number of cores!\n");
  }

  config.flags = LPEL_FLAG_PINNED;
  if (do_excl) config.flags |= LPEL_FLAG_EXCLUSIVE;

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

  AssignmentInit( config.num_workers);

  LpelInit(&config);
}

void SNetLpelIfDestroy( void)
{
  LpelCleanup();

  AssignmentCleanup();

  if (mapfile) {
    (void) fclose( mapfile);
  }
}


static int GetMonFlags(bool is_box)
{
  int flags = LPEL_TASK_ATTR_NONE;

  if (_mon_lvl < 1) return LPEL_TASK_ATTR_NONE;
  if (_mon_lvl > 4) return LPEL_TASK_ATTR_MONITOR_ALL;

  switch (_mon_lvl) {
  case 4: flags |= LPEL_TASK_ATTR_MONITOR_ALL;
  case 3: if (is_box) flags |= LPEL_TASK_ATTR_MONITOR_STREAMS;
  case 2: if (is_box) flags |= LPEL_TASK_ATTR_MONITOR_TIMES;
  case 1: if (is_box) flags |= LPEL_TASK_ATTR_MONITOR_OUTPUT;
    break;
  case 0:
    break;
  }
  return flags;
}

/**
 * Create a task on a wrapper thread
 */
void SNetLpelIfSpawnWrapper( lpel_taskfunc_t taskfunc, void *arg,
    char *name)
{
  lpel_taskreq_t *t;
  int stacksize = 0;

  t = LpelTaskRequest(taskfunc, arg, GetMonFlags(false), stacksize, 0);
  return LpelWorkerWrapperCreate( t, name);
}



void SNetLpelIfSpawnEntity( lpel_taskfunc_t fun, void *arg,
  snet_entity_id_t id, char *label)
{
  lpel_taskreq_t *t;
  int stacksize = 0;
  int wid;
  int flags;
  unsigned int tid;
  int prio = 0;


  flags = GetMonFlags(id==ENTITY_box);

  /* stacksize */
  if (id==ENTITY_box) {
    stacksize = 8*1024*1024; /* 8 MB */
  } else {
    stacksize = 256*1024; /* 256 kB */
  }

  /* priority */
  if (id != ENTITY_box) prio = 1;

  /* create task */
  t = LpelTaskRequest( fun, arg, flags, stacksize, prio);
  tid = LpelTaskReqGetUID( t);

  
  switch(id) {
    case ENTITY_parallel_nondet:
    case ENTITY_parallel_det:
      label = "<parallel>"; break;
    case ENTITY_star_nondet:
    case ENTITY_star_det:
      label = "<star>"; break;
    case ENTITY_split_nondet:
    case ENTITY_split_det:
      label = "<split>"; break;
      break;
    case ENTITY_sync:
      label = "<sync>"; break;
    case ENTITY_filter:
      label = "<filter>"; break;
    case ENTITY_collect:
      label = "<collector>"; break;
    default:
      break;
  }

  /* Query assignment module */
  wid = AssignmentGetWID(t, id==ENTITY_box, label);

  if (mapfile && (flags & LPEL_TASK_ATTR_MONITOR_OUTPUT) ) {
    (void) fprintf( mapfile, "%u %s %d\n", tid, label, wid);
    //(void) fflush (mapfile);
  }

  
  /* call worker assignment */
  LpelWorkerTaskAssign( t, wid);
}


