/* $Id$ */

#ifdef USE_CORE_AFFINITY
#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <sched.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "memfun.h"
#include "snetentities.h"
#include "debug.h"

#include "spawn.h"
#include "assignment.h"

#include "lpel.h"


/**
 * Create a task on a scheduler wrapper thread
 */
void SNetSpawnWrapper( lpel_taskfunc_t taskfunc, void *arg,
    char *name)
{
  lpel_task_t *t;
  lpel_taskattr_t attr = { LPEL_TASK_ATTR_ALL, 0};

  t = LpelTaskCreate(taskfunc, arg, &attr);
  return _LpelWorkerWrapperCreate( t, name);
}



void SNetSpawnEntity( lpel_taskfunc_t fun, void *arg, snet_entity_id_t id)
{
  lpel_task_t *t;
  lpel_taskattr_t tattr = { LPEL_TASK_ATTR_ALL, 0};
  int wid;

  /* monitoring */
  if (id!=ENTITY_box) {
    tattr.flags &= ~LPEL_TASK_ATTR_MONITOR_OUTPUT;
  }

  /* stacksize */
  if (id==ENTITY_box || id==ENTITY_none) {
    tattr.stacksize = 8*1024*1024; /* 8 MB */
  } else {
    tattr.stacksize = 256*1024; /* 256 kB */
  }

  /* create task */
  t = LpelTaskCreate( fun, arg, &tattr);

  /* Query assignment module */
  wid = AssignmentGetWID(t, id==ENTITY_box);
  
  /* call worker assignment */
  _LpelWorkerTaskAssign( t, wid);
}


