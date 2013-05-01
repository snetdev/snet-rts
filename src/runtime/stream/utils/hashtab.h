
#ifndef _HASHTAB_H_
#define _HASHTAB_H_

typedef struct hashtab hashtab_t;
typedef struct hashtab_iter hashtab_iter_t;

hashtab_iter_t *HashtabIterCreate( hashtab_t *ht);
void HashtabIterDestroy( hashtab_iter_t *hti);
int  HashtabIterHasNext( hashtab_iter_t *hti);
void *HashtabIterNext( hashtab_iter_t *hti);
void HashtabIterReset( hashtab_iter_t *hti);

hashtab_t *HashtabCreate( int init_cap2);
void HashtabDestroy( hashtab_t *ht);
void *HashtabGet( hashtab_t *ht, int key);
void  HashtabPut( hashtab_t *ht, int key, void *value);
void **HashtabGetPointer( hashtab_t *ht, int key);


#ifdef DEBUG
#include <stdio.h> 
extern void HashtabPrintDebug( hashtab_t *ht, FILE *outf);
#endif


#endif /* _HASHTAB_H_ */
