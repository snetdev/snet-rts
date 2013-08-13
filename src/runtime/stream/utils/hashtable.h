#ifndef _SNET_HASHTABLE_H_
#define _SNET_HASHTABLE_H_

#include <stdint.h>

#define HASHTABLE_PTR_TO_KEY(ptr) ((uint64_t) (ptr)) 

#define SNET_HASHTABLE_SUCCESS  0
#define SNET_HASHTABLE_ERROR    1

typedef struct snet_hashtable snet_hashtable_t;

snet_hashtable_t *SNetHashtableCreate(int size);

void SNetHashtableDestroy(snet_hashtable_t *table);

void *SNetHashtableGet(snet_hashtable_t *table, uint64_t key);

int SNetHashtablePut(snet_hashtable_t *table, uint64_t key, void *value);

void *SNetHashtableReplace(snet_hashtable_t *table, uint64_t key, void *new_value);

void *SNetHashtableRemove(snet_hashtable_t *table, uint64_t key);

int SNetHashtableSize(snet_hashtable_t *table);

bool SNetHashtableFirstKey(snet_hashtable_t *table, uint64_t *first);

bool SNetHashtableNextKey(snet_hashtable_t *table, uint64_t key, uint64_t *next);

#endif /* _SNET_HASHTABLE_H_ */
