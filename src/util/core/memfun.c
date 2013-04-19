/*
 * Memory allocator/free functions
 */ 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "memfun.h"

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

void *SNetMemResize( void *ptr, size_t size)
{
  if ((ptr = realloc(ptr, size)) == NULL) {
    SNetMemFailed();
  }
  return ptr;
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

