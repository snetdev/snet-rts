#ifndef _SNET_HASHTABLE_H_
#define _SNET_HASHTABLE_H_

#include <stdint.h>

#if __WORDSIZE == 64
#define HASHTABLE_PTR_TO_KEY(ptr) (0 | (uint64_t) ptr) 
#else
#define HASHTABLE_PTR_TO_KEY(ptr) (0 | (uint32_t) ptr); 
#endif

#define SNET_HASHTABLE_SUCCESS 0
#define SNET_HASHTABLE_ERROR   1

typedef int (*snet_hashtable_compare_fun_t)(void *, void*);

typedef struct snet_hashtable snet_hashtable_t;

snet_hashtable_t *SNetHashtableCreate(int size, snet_hashtable_compare_fun_t compare_fun);

void SNetHashtableDestroy(snet_hashtable_t *table);

void *SNetHashtableGet(snet_hashtable_t *table, uint64_t key);

uint64_t SNetHashtableGetKey(snet_hashtable_t *table, void *value);

int SNetHashtablePut(snet_hashtable_t *table, uint64_t key, void *value);

void *SNetHashtableReplace(snet_hashtable_t *table, uint64_t key, void *new_value);

void *SNetHashtableRemove(snet_hashtable_t *table, uint64_t key);

int SNetHashtableSize(snet_hashtable_t *table);

#endif /* _SNET_HASHTABLE_H_ */
