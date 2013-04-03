/*
 * description: common implementation for glue_hrc & glue_decen
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "glue_snet.h"
#include <lpel/monitor.h>
#include "debug.h"
#include "mon_snet.h"

/* S-Net threading backend interface */
#include "threading.h"

#include "lpel.h"



size_t GetStacksize(snet_entity_descr_t descr)
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


void *EntityTask(void *arg)
{
	snet_entity_t *ent = (snet_entity_t *)arg;

	SNetEntityCall(ent);
	SNetEntityDestroy(ent);

	return NULL;
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





