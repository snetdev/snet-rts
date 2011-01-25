
#ifndef _LPELIF_H_
#define _LPELIF_H_

#include "snettypes.h"
#include "lpel.h"

struct snet_stream_t {
  int compiler_happyness;
  //lpel_stream_t *stream;  
};


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
 

extern void SNetLpelIfInit( int rank, int num_workers, bool do_excl, int mon_level);

extern void SNetLpelIfDestroy( void);

extern void SNetLpelIfSpawnEntity( void (*fun)(lpel_task_t *t, void *arg),
    void *arg, snet_entity_id_t id, char *label);

extern void SNetLpelIfSpawnWrapper( void (*func)(lpel_task_t *, void*),
    void *arg, char *name);

#endif /* _LPELIF_H_ */
