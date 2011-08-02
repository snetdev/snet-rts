#ifndef _ATOMICCNT_H_
#define _ATOMICCNT_H_


/**
 * Atomic counter for atomic fetch and increment
 * and fetch and decrement operations.
 */
typedef struct snet_atomiccnt snet_atomiccnt_t;

/**
 * Create an atomic counter
 */
snet_atomiccnt_t *SNetAtomicCntCreate(int val);

/**
 * Destroy an atomic counter
 */
void SNetAtomicCntDestroy(snet_atomiccnt_t *cnt);

/**
 * Atomic fetch-and-increment
 */
int SNetAtomicCntFetchAndInc(snet_atomiccnt_t *cnt);

/**
 * Atomic fetch-and-decrement
 */
int SNetAtomicCntFetchAndDec(snet_atomiccnt_t *cnt);



#endif /* _ATOMICCNT_H_ */
