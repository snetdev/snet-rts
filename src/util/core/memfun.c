/*
 * Memory allocator/free functions
 */ 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "memfun.h"

#ifdef _SNET_MEMFUN_BSD
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

void SNetMemFailed(void)
{
  fprintf(stderr,
          "\n\n** Fatal Error ** : Unable to Allocate Memory: %s.\n\n",
          strerror(errno));
  exit(1);
}

void *SNetMemAlloc( size_t size)
{
  void *ptr = NULL;
  if (size && (ptr = malloc(size)) == NULL) {
    SNetMemFailed();
  }
  return ptr;
}

void *SNetMemCalloc( size_t nmemb, size_t size)
{
  void *ptr = calloc(nmemb, size);
  if (!ptr) { SNetMemFailed(); }
  return ptr;
}

size_t SNetMemSize(void *ptr) {
#ifdef _SNET_MEMFUN_BSD
  return malloc_size(ptr);
#else
  return malloc_usable_size(ptr);
#endif
}


void *SNetMemResize( void *ptr, size_t size)
{
  if (size == 0) {
    SNetMemFree(ptr);
    return NULL;
  }

  /* try to realloc
   * this only fail if size > malloc_size/malloc_usable_size(ptr)
   * and free memory after the block of ptr is not enough to add up to size
   */
  if ((ptr = realloc(ptr, size)) != NULL) 
    return ptr;

  /* allocate new memory */
  void *nptr = SNetMemAlloc(size);
  if (nptr == NULL) {
    SNetMemFailed();
    /* should not reach here */
    return NULL;
  }
  else {
    size_t old_size = SNetMemSize(ptr);   // old_size must be smaller than size
    memcpy(nptr, ptr, old_size);
    SNetMemFree(ptr);
    return nptr;
  }
}

void SNetMemFree( void *ptr)
{
  free(ptr);
}

void* SNetMemAlign( size_t size)
{
  void *vptr;
  int retval;
  size_t remain = size % LINE_SIZE;
  size_t request = remain ? (size + (LINE_SIZE - remain)) : size;

  if ((retval = posix_memalign(&vptr, LINE_SIZE, request)) != 0) {
    errno = retval;
    SNetMemFailed();
  }
  return vptr;
}

char *SNetStrDup(const char *str)
{
  char *ptr = strdup(str);
  if (ptr == NULL) {
    SNetMemFailed();
  }
  return ptr;
}

