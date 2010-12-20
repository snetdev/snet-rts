/* $Id$ */

#ifndef _SPAWN_H_
#define _SPAWN_H_


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
  ENTITY_dist,
  ENTITY_none
} snet_entity_id_t;
 


struct task;

extern void SNetSpawnEntity( void (*fun)(struct task *t, void *arg),
    void *arg, snet_entity_id_t id);

extern void SNetSpawnWrapper( void (*func)(struct task *, void*),
    void *arg, char *name);

#endif /* _SPAWN_H_ */					 

