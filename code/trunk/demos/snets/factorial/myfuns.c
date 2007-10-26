#include <stdlib.h>
#include <stdio.h>
#include <myfuns.h>

void myfree( void *ptr)
{
  free( ptr);
}

void *mycopy( void *ptr)
{
  int *int_ptr;
  
  int_ptr = malloc( sizeof( int));
  *int_ptr = *(int*)ptr;

  return( int_ptr);
}



