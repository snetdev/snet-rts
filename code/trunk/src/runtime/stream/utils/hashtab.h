
#ifndef _HASHTAB_H_
#define _HASHTAB_H_

typedef struct hashtab hashtab_t;

extern hashtab_t *HashtabCreate( int init_cap2);
extern void HashtabDestroy( hashtab_t *ht);
extern void *HashtabGet( hashtab_t *ht, int key);
extern void  HashtabPut( hashtab_t *ht, int key, void *value);

#ifdef DEBUG
#include <stdio.h> 
extern void HashtabPrintDebug( hashtab_t *ht, FILE *outf);
#endif


#endif /* _HASHTAB_H_ */
