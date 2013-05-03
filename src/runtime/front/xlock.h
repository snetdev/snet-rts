#ifndef _XLOCK_H
#define _XLOCK_H


#include <semaphore.h>

typedef sem_t lock_t;

#define LOCK(x)         sem_wait(&(x))
#define UNLOCK(x)       sem_post(&(x))
#define TRYLOCK(x)      sem_trywait(&(x))
#define LOCK_INIT(x)    sem_init(&(x), 0, 1)
#define LOCK_INIT2(x,y) sem_init(&(x), 0, (y))
#define LOCK_DESTROY(x) sem_destroy(&(x))


#endif
