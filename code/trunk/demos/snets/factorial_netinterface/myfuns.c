#include <stdlib.h>
#include <stdio.h>
#include <myfuns.h>
#include <C2SNet.h>

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
  void *val = NULL;

  if(value != NULL){
    int *i = (int *)malloc(sizeof( int));
    *i = atoi(value);
    val = (void *)i;
  }

  C_Data *field = C2SNet_cdataCreate(val, myfree, mycopy);


  return (void *)field;
}

#define SER_BUF_SIZE 32

char *myserialize(const void* value){
  int i = 1;
  char *c = NULL;
  int ret = -1;

  if(value == NULL){
    return NULL;
  }

  void * val = C2SNet_cdataGetData((C_Data *)value);
  
  if(val == NULL){
    return NULL;
  }
  
  do{
    if(c != NULL){
      free(c);
      c = NULL;
      i++;
    }
    
    c = (char *)malloc(sizeof(char) * SER_BUF_SIZE * i);

    ret = snprintf(c, SER_BUF_SIZE * i, "%d", *((int *)val));
    
  }while(ret <= 0 || ret >= SER_BUF_SIZE * i);
  
  return c;
}

#undef SER_BUF_SIZE

