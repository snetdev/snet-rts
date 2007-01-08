/*
 * Memory allocator/free functions
 */ 

#include <stdlib.h>
#include <memfun.h>
#include <stdio.h>

void *SNetMemAlloc( size_t s) {
  
  void *ptr;

  ptr = malloc( s);

  if( ptr == NULL) {
    printf("\n\n** Fatal Error ** : Unable to Allocate Memory.\n\n");
    exit(1);
  }

  return( ptr);
}


void SNetMemFree( void *ptr) {

  free( ptr);
  ptr = NULL;
}


