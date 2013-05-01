#ifndef _LOCVEC_H_
#define _LOCVEC_H_


#include "bool.h"

#include "info.h"



/** forward declaration */

typedef struct snet_locvec_t snet_locvec_t;



snet_locvec_t *SNetLocvecCreate(void);
void SNetLocvecDestroy(snet_locvec_t *vec);

snet_locvec_t *SNetLocvecCopy(snet_locvec_t *vec);

bool SNetLocvecEqual(snet_locvec_t *u, snet_locvec_t *v);
bool SNetLocvecEqualParent(snet_locvec_t *u, snet_locvec_t *v);


/* for serial combinator */
bool SNetLocvecSerialEnter(snet_locvec_t *);
void SNetLocvecSerialNext(snet_locvec_t *);
void SNetLocvecSerialLeave(snet_locvec_t *, bool);

/* for parallel combinator */
void SNetLocvecParallelEnter(snet_locvec_t *);
void SNetLocvecParallelNext(snet_locvec_t *);
void SNetLocvecParallelLeave(snet_locvec_t *);
void SNetLocvecParallelReset(snet_locvec_t *);

/* for star combinator */
bool SNetLocvecStarWithin(snet_locvec_t *);
void SNetLocvecStarEnter(snet_locvec_t *);
void SNetLocvecStarLeave(snet_locvec_t *);
snet_locvec_t *SNetLocvecStarSpawn(snet_locvec_t *);
snet_locvec_t *SNetLocvecStarSpawnRet(snet_locvec_t *);

/* for split combinator */
void SNetLocvecSplitEnter(snet_locvec_t *);
void SNetLocvecSplitLeave(snet_locvec_t *);
snet_locvec_t *SNetLocvecSplitSpawn(snet_locvec_t *, int);

/* for feedback combinator */
void SNetLocvecFeedbackEnter(snet_locvec_t *);
void SNetLocvecFeedbackLeave(snet_locvec_t *);



/* get from info */
snet_locvec_t *SNetLocvecGet(snet_info_t *info);

/* set to info */
void SNetLocvecSet(snet_info_t *info, snet_locvec_t *vec);

int SNetLocvecTopval(snet_locvec_t *locvec);

int SNetLocvecPrint(char *sbuf, int size, snet_locvec_t *vec);

#endif /* _LOCVEC_H_ */
