/*
 * Memory allocator/free functions
 */ 

#ifndef MEMFUN_H
#define MEMFUN_H

#include <stddef.h>

/* The processor cache line size for memory alignment. */
#ifndef LINE_SIZE
#define LINE_SIZE       64
#endif

/*
 * SNetNew allocates memory to hold values of parameter 'type'.
 * It returns this memory cast to the appropriate type.
 * This prevents allocating memory for the wrong types.
 * SNetNewN allocates an array of 'N' memory cells of type 'type'.
 * SNetNewAlign allocates memory on cache line aligned boundaries.
 */
#define SNetNew(type)           ((type *) SNetMemAlloc(sizeof(type)))
#define SNetNewN(num, type)     ((type *) SNetMemAlloc((num) * sizeof(type)))
#define SNetNewAlign(type)      ((type *) SNetMemAlign(sizeof(type)))
#define SNetNewAlignN(num,type) ((type *) SNetMemAlign((num) * sizeof(type)))

/*
 * Free memory which was allocated with SNetNew or SNetNewAlign.
 */
#define SNetDelete(ptr)         SNetMemFree(ptr)
#define SNetDeleteN(num, ptr)   SNetMemFree(ptr)


/*
 * Process out-of-memory errors.
 */
void SNetMemFailed(void);


/*
 * Allocates memory of size s.
 * RETURNS: pointer to memory.
 */
void *SNetMemAlloc( size_t s);


/*
 * Resize previously allocated memory.
 */
void *SNetMemResize( void *ptr, size_t size);


/*
 * Frees the memory pointed to by ptr.
 */
void SNetMemFree( void *ptr);


/*
 * Frees the memory pointed to by ptr.
 */
void SNetMemSizeFree( void *ptr, size_t size);


/*
 * Allocate memory which is aligned to a cache line boundary.
 */
void* SNetMemAlign( size_t size);

#endif
