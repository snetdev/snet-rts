/*
 * Memory allocator/free functions
 */ 

#ifndef MEMFUN_H
#define MEMFUN_H

#include <stddef.h>


/*
 * Allocates memory of size s.
 * RETURNS: pointer to memory.
 */

void *SNetMemAlloc( size_t s);



/*
 * Frees the memory pointed to by ptr.
 * RETURNS: nothing.
 */

void SNetMemFree( void *ptr);




#endif
