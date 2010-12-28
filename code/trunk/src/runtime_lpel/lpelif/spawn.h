/* $Id$ */

#ifndef _SPAWN_H_
#define _SPAWN_H_

#include "lpel.h"



/**
 * snet_entity_id_t type
 */
typedef enum {
  ENTITY_parallel_nondet,
  ENTITY_parallel_det,
  ENTITY_star_nondet,
  ENTITY_star_det,
  ENTITY_split_nondet,
  ENTITY_split_det,
  ENTITY_box,
  ENTITY_sync,
  ENTITY_filter,
  ENTITY_collect,
} snet_entity_id_t;
 

extern void SNetSpawnInit( int node);

extern void SNetSpawnDestroy( void);

extern void SNetSpawnEntity( void (*fun)(lpel_task_t *t, void *arg),
    void *arg, snet_entity_id_t id, char *label);

extern void SNetSpawnWrapper( void (*func)(lpel_task_t *, void*),
    void *arg, char *name);

#endif /* _SPAWN_H_ */					 

