
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>
#include <assert.h>

#include "threading.h"

#include "entity.h"


static unsigned int entity_count = 0;
static pthread_mutex_t entity_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  entity_cond = PTHREAD_COND_INITIALIZER;

/* prototype for pthread thread function */
static void *SNetEntityThread(void *arg);



int SNetThreadingInit(int entity_cores, int comm_cores)
{
  /* initialize the entity counter to 0 */
  entity_count = 0;

  return 0;
}



int SNetThreadingProcess(void)
{
  /* Wait for the entities */
  pthread_mutex_lock( &entity_lock );
  while (entity_count > 0) {
    pthread_cond_wait( &entity_cond, &entity_lock );
  }
  pthread_mutex_lock( &entity_lock );

  return 0;
}


int SNetThreadingShutdown(void)
{

  return 0;
}


int SNetEntitySpawn(snet_entity_id_t type, snet_entityfunc_t func, void *arg)
{
  int res;
  pthread_t p;
  pthread_attr_t attr;
  int stacksize = 0;

  /* create snet_entity_t */
  snet_entity_t *self = malloc(sizeof(snet_entity_t));
  self->func = func;
  self->inarg = arg;
  pthread_mutex_init( &self->lock, NULL );
  pthread_cond_init( &self->pollcond, NULL );
  self->wakeup_sd = NULL;


  /* increment entity counter */
  pthread_mutex_lock( &entity_lock );
  entity_count += 1;
  pthread_mutex_lock( &entity_lock );


  /* create pthread: */
  stacksize = 0;//FIXME: stacksize dep on entity_id

  /* TODO core affinity */

  (void) pthread_attr_init( &attr);

  res = pthread_attr_setstacksize(&attr, stacksize);
  if (res != 0) {
    //error(1, res, "Cannot set stack size!");
    return 1;
  }

  res = pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
  if (res != 0) {
    //error(1, res, "Cannot set detached!");
    return 1;
  }

  /* spawn off thread */
  res = pthread_create( &p, &attr, SNetEntityThread, self);
  if (res != 0) {
    //error(1, res, "Cannot create pthread!");
    return 1;
  }
  pthread_attr_destroy( &attr);

  return 0;
}



void SNetEntityYield(snet_entity_t *self)
{
  /* NOP,
   * our pthreads are preemptive anyway
   */
}


/**
 * Let the current entity exit
 */
void SNetEntityExit(snet_entity_t *self)
{
  /* cleanup */
  pthread_mutex_destroy( &self->lock );
  pthread_cond_destroy( &self->pollcond );
  free(self); 


  /* decrement and signal entity counter */
  pthread_mutex_lock( &entity_lock );
  entity_count -= 1;
  if (entity_count == 0) {
    pthread_cond_signal( &entity_cond );
  }
  pthread_mutex_lock( &entity_lock );

  
  (void) pthread_exit(NULL);
}






static void *SNetEntityThread(void *arg)
{
  snet_entity_t *ent = (snet_entity_t *)arg;
  ent->func( ent, ent->inarg );
  /* call entity function */
  SNetEntityExit(ent);

  /* following line is not reached, just for compiler happiness */
  return NULL;
}
