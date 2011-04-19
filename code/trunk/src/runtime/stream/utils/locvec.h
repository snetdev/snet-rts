#ifndef _LOCVEC_H_
#define _LOCVEC_H_


#include <stdint.h>
#include <stdio.h>
#include "bool.h"

#include "info.h"

typedef enum {
  LOC_SERIAL = 'S',
  LOC_PARALLEL = 'P',
  LOC_SPLIT = 'I',
  LOC_STAR = 'R',
  LOC_FEEDBACK = 'F'
} snet_loctype_t;

typedef uint64_t snet_locitem_t;

typedef struct snet_locvec_t snet_locvec_t;



snet_locvec_t *SNetLocvecCreate(void);
void SNetLocvecDestroy(snet_locvec_t *vec);
snet_locvec_t *SNetLocvecCopy(snet_locvec_t *vec);
void SNetLocvecAppend(snet_locvec_t *vec, snet_loctype_t type, int num);
void SNetLocvecPop(snet_locvec_t *vec);
int SNetLocvecToptype(snet_locvec_t *vec);
void SNetLocvecTopinc(snet_locvec_t *vec);
bool SNetLocvecEqual(snet_locvec_t *u, snet_locvec_t *v);
bool SNetLocvecEqualParent(snet_locvec_t *u, snet_locvec_t *v);

snet_locvec_t *SNetLocvecGet(snet_info_t *info);
void SNetLocvecSet(snet_info_t *info, snet_locvec_t *vec);

void SNetLocvecPrint(FILE *file, snet_locvec_t *vec);

#endif /* _LOCVEC_H_ */
