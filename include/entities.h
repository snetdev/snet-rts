#ifndef _ENTITIES_H_
#define _ENTITIES_H_


#include "locvec.h"


/*
 * Following entities exist and must be handled by the backends
 * (can be extended in future implementations):
 */
typedef enum {
  ENTITY_box,
  ENTITY_parallel,
  ENTITY_star,
  ENTITY_split,
  ENTITY_fbcoll,
  ENTITY_fbdisp,
  ENTITY_fbbuf,
  ENTITY_sync,
  ENTITY_filter,
  ENTITY_nameshift,
  ENTITY_collect,
  ENTITY_other
} snet_entity_descr_t;


typedef struct snet_entity_t snet_entity_t;

typedef void (*snet_entityfunc_t)(snet_entity_t *ent, void *arg);

typedef void (*snet_entityinitfunc_t)(snet_entity_t *ent, void *arg);

extern snet_entity_t *SNetEntityCreate(snet_entity_descr_t, ...);

extern void SNetEntityDestroy(snet_entity_t *);

extern snet_entity_t *SNetEntityCopy(snet_entity_t *);

extern void SNetEntityCall(snet_entity_t *);

extern snet_entity_descr_t SNetEntityDescr(snet_entity_t *);

extern snet_locvec_t *SNetEntityGetLocvec(snet_entity_t *);

extern int SNetEntityNode(snet_entity_t *);

extern const char *SNetEntityName(snet_entity_t *);

extern const char *SNetEntityStr(snet_entity_t *);

extern void SNetEntitySetFunction(snet_entity_t *ent, snet_entityfunc_t f);

#endif /* _ENTITIES_H_ */
