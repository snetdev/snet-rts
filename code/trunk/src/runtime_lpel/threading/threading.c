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
#include "threading.h"
#include "assignment.h"

#include "lpel.h"
#include "task.h"
#include "scheduler.h"



void SNetEntitySpawn( taskfunc_t fun, void *arg, snet_entity_id_t id)
{
  task_t *t;
  taskattr_t tattr = {0, 0};

  /* monitoring */
  if (id==ENTITY_box) {
    tattr.flags |= TASK_ATTR_MONITOR;
  }
  //tattr.flags |= TASK_ATTR_MONITOR;

  /* stacksize */
  if (id==ENTITY_box || id==ENTITY_none) {
    tattr.stacksize = 8*1024*1024; /* 8 MB */
  } else {
    tattr.stacksize = 256*1024; /* 256 kB */
  }

  /* create task */
  t = TaskCreate( fun, arg, &tattr);

  /* Call assignment */
  AssignmentAssign(t, id==ENTITY_box);
}


