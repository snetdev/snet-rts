#include <stdlib.h>
#include <stdio.h>
#include <myfuns.h>
#include <C2SNet.h>
#include <string.h>

void myfree( void *ptr)
{
  free( ptr);
}

void *mycopyChar( void *ptr)
{
  char *chr_ptr = (char *)ptr;

  char *c = NULL;
  c = (char *)malloc( sizeof(char) * strlen(chr_ptr));
  c = strcpy(c, chr_ptr);
  
  return(c);
}

void *mycopyInt( void *ptr)
{
  int *int_ptr = NULL;

  int_ptr = malloc( sizeof( int));
  *int_ptr = *(int*)ptr;
  
  return( int_ptr);
}


char *myserializeChar(void* value){
  return mycopyChar((char *)value);
}

#define SER_BUF_SIZE 32

char *myserializeInt(void* value){
  int i = 1;
  char *c = NULL;
  int ret = -1;

  if(value == NULL){
    return NULL;
  }

  do{
    if(c != NULL){
      free(c);
      c = NULL;
      i++;
    }
    c = (char *)malloc(sizeof(char) * SER_BUF_SIZE * i);
    ret = snprintf(c, SER_BUF_SIZE * i, "int:%d", *((int *)value));
  }while(ret <= 0 || ret >= SER_BUF_SIZE * i);
  
  return c;
}

#undef SER_BUF_SIZE

void *mydeserialize(char* value){
  C_Data *field = NULL;
  if(value != NULL){


    if(strncmp(value, "int:", 4) == 0){
      int *i = (int *)malloc(sizeof( int));
      *i = atoi(value + 4);
      field = C2SNet_cdataCreate((void *)i, myfree, mycopyInt, myserializeInt);
    }else{
      field = C2SNet_cdataCreate(mycopyChar((void *)value), myfree, 
				 mycopyChar, myserializeChar);
    }
  }
  return (void *)field;
}

