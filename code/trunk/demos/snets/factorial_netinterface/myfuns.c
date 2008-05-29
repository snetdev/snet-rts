#include <stdlib.h>
#include <stdio.h>
#include <myfuns.h>
#include <C2SNet.h>
#include <string.h>

void C2SNetFree( void *ptr)
{
  free( ptr);
}

void *C2SNetCopyFloat( void *ptr)
{
  float *float_ptr = NULL;

  float_ptr = malloc( sizeof( float));
  *float_ptr = *(float *)ptr;
  
  return( float_ptr);
}

void *C2SNetCopyInt( void *ptr)
{
  int *int_ptr = NULL;

  int_ptr = malloc( sizeof( int));
  *int_ptr = *(int *)ptr;
  
  return( int_ptr);
}


int C2SNetSerializeFloat(FILE *file, void* value)
{
  return fprintf(file, "(float)%f", *(float *)value);
}

int C2SNetSerializeInt(FILE *file, void* value)
{
  return fprintf(file, "(int)%d", *(int*)value);
}

void *C2SNetDeserialize(FILE *file) 
{
  int i;
  float f;
  char buf[6];
  int j;

  for(j = 0; j < 5; j++) {
    buf[j] = fgetc(file);
  }

  if(strncmp(buf, "(int)", 5) == 0) {
    if(fscanf(file, "%d", &i) == 1) {
      int *i_ptr = (int *)malloc(sizeof( int));
      *i_ptr = i;
      return (void *)C2SNet_cdataCreate((void *)i_ptr, 
					C2SNetFree,
					C2SNetCopyInt,
					C2SNetSerializeInt);
    }
  }
  else { 

    buf[j++] = fgetc(file);
    buf[j++] = fgetc(file);
  
    if(strncmp(buf, "(float)", 7) == 0) {

      if(fscanf(file, "%f", &f) == 1) {
	float *f_ptr = (float *)malloc(sizeof( float));
	*f_ptr = f;
	
	return (void *)C2SNet_cdataCreate((void *)f_ptr, 
					  C2SNetFree,
					  C2SNetCopyFloat,
					  C2SNetSerializeFloat);
      }
    }
  }
  exit(1);
  return NULL;
}

