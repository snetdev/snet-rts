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


int myserializeChar(void* value, char **data){
  *data = (char *)mycopyChar(value);
  
  return strlen(*data);
}

#define SER_BUF_SIZE 32

int myserializeInt(void* value, char **data){
  int i = 1;
  int ret = -1;
    if(*data != NULL){
      free(*data);
    }
    *data = NULL;


  if(value == NULL){
    return 0;
  }

  do{
    if(*data != NULL){
      free(*data);
      *data = NULL;
      i++;
    }
    *data = (char *)malloc(sizeof(char) * SER_BUF_SIZE * i);
    ret = snprintf(*data, SER_BUF_SIZE * i, "int:%d", *((int *)value));
  }while(ret <= 0 || ret >= SER_BUF_SIZE * i);

  return ret;
}

#undef SER_BUF_SIZE

void *mydeserialize(char* value, int len){
  if(value != NULL){
    /* int */
    if(len > 4 && strncmp(value, "int:", 4) == 0){
      int *i = (int *)malloc(sizeof( int));
      *i = atoi(value + 4);
      return (void *)C2SNet_cdataCreate((void *)i, myfree, 
					mycopyInt, myserializeInt);
    }
  }
  /* char or unknown */
  return (void *)C2SNet_cdataCreate(mycopyChar((void *)value), myfree, 
				    mycopyChar, myserializeChar);
}

