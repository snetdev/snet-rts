#include <stdlib.h>
#include <stdio.h>
#include <C2SNetTypes.h>
#include <C2SNet.h>
#include <string.h>

void C2SNetFree( void *ptr)
{
  if(ptr != NULL) {
    free( ptr);
  }
}

void *C2SNetCopyInt( void *ptr)
{
  int *int_ptr = NULL;

  int_ptr = malloc( sizeof( int));
  *int_ptr = *(int *)ptr;
  
  return( int_ptr);
}

int C2SNetSerializeInt(FILE *file, void* value)
{
  return fprintf(file, "(int)%d", *(int*)value);
}

/* This is just for testing. Same as serialization. */
int C2SNetEncodeInt(FILE *file, void* value)
{
  return fprintf(file, "(int)%d", *(int*)value);
}

void *C2SNetCopyFloat( void *ptr)
{
  float *float_ptr = NULL;

  float_ptr = malloc( sizeof( float));
  *float_ptr = *(float *)ptr;
  
  return( float_ptr);
}

int C2SNetSerializeFloat(FILE *file, void* value)
{
  return fprintf(file, "(float)%f", *(float *)value);
}

/* This is just for testing. Same as serialization. */
int C2SNetEncodeFloat(FILE *file, void* value)
{
  return fprintf(file, "(float)%f", *(float *)value);
}



void *C2SNet_deserialize(FILE *file) 
{
  int i;
  float f;
  char buf[6];
  int j;

  for(j = 0; j < 5; j++) {
    buf[j] = fgetc(file);
    if(buf[j] == '<') {
      ungetc('<', file);
      return NULL;
    }
  }

  if(strncmp(buf, "(int)", 5) == 0) {
    if(fscanf(file, "%d", &i) == 1) {
      int *i_ptr = (int *)malloc(sizeof( int));
      *i_ptr = i;
      return (void *)C2SNet_cdataCreate((void *)i_ptr, 
					C2SNetFree,
					C2SNetCopyInt,
					C2SNetSerializeInt,
					C2SNetEncodeInt);
    }
  }
  else { 

    buf[j] = fgetc(file);
    if(buf[j] == '<') {
      ungetc('<', file);
      return NULL;
    }
    buf[++j] = fgetc(file);
    if(buf[j] == '<') {
      ungetc('<', file);
      return NULL;
    }
  
    if(strncmp(buf, "(float)", 7) == 0) {

      if(fscanf(file, "%f", &f) == 1) {
	float *f_ptr = (float *)malloc(sizeof( float));
	*f_ptr = f;
	
	return (void *)C2SNet_cdataCreate((void *)f_ptr, 
					  C2SNetFree,
					  C2SNetCopyFloat,
					  C2SNetSerializeFloat,
					  C2SNetEncodeFloat);
      }
    }
  }
 
  return NULL;
}

/* This is just for testing. Same as serialization. */
void *C2SNet_decode(FILE *file) 
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
					C2SNetSerializeInt,
					C2SNetEncodeInt);
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
					  C2SNetSerializeFloat,
					  C2SNetEncodeFloat);
      }
    }
  }
  
  return NULL;
}
