#include <stdlib.h>
#include <stdio.h>
#include <myfuns.h>

void myfree( void *ptr)
{
  free( ptr);
}

void *mycopy( void *ptr)
{
  int *int_ptr = NULL;

  int_ptr = malloc( sizeof( int));
  *int_ptr = *(int*)ptr;
  
  return( int_ptr);
}

void *mydeserialize(const char* value){
  int *i = (int *)malloc(sizeof( int));
  *i = atoi(value);
  return (void *)i;
}

#define SER_BUF_SIZE 32

char *myserialize(const void* value){
  int i = 1;
  char *c = NULL;
  int ret = -1;

  do{
    if(c != NULL){
      free(c);
      c = NULL;
      i++;
    }
    
    c = (char *)malloc(sizeof(char) * SER_BUF_SIZE * i);

    ret = snprintf(c, SER_BUF_SIZE * i, "%d", *((int *)value));
    
  }while(ret <= 0 || ret >= SER_BUF_SIZE * i);
  
  return c;
}

#undef SER_BUF_SIZE

