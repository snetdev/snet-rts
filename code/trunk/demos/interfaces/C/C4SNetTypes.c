#include <stdlib.h>
#include <stdio.h>
#include <C4SNetTypes.h>
#include <C4SNet.h>
#include <string.h>

#define ID_INT   1
#define ID_FLOAT 2

void C4SNetFree( void *ptr)
{
  if(ptr != NULL) {
    free( ptr);
  }
}

void *C4SNetCopyInt( void *ptr)
{
  int *int_ptr = NULL;

  int_ptr = malloc( sizeof( int));
  *int_ptr = *(int *)ptr;
  
  return( int_ptr);
}

int C4SNetSerializeInt(FILE *file, void* value)
{
  return fprintf(file, "(int)%d", *(int*)value);
}

/* This is just for testing. Same as serialization. */
int C4SNetEncodeInt(FILE *file, void* value)
{
  int i;
  
  fputc(ID_INT, file);

  for(i = 0; i < sizeof( int); i++) {
    fputc(((char *)value)[i], file);
  }

  return sizeof( float);
}

void *C4SNetCopyFloat( void *ptr)
{
  float *float_ptr = NULL;

  float_ptr = malloc( sizeof( float));
  *float_ptr = *(float *)ptr;
  
  return( float_ptr);
}

int C4SNetSerializeFloat(FILE *file, void* value)
{
  return fprintf(file, "(float)%f", *(float *)value);
}

/* This is just for testing. Same as serialization. */
int C4SNetEncodeFloat(FILE *file, void* value)
{
  int i;
  
  fputc(ID_FLOAT, file);

  for(i = 0; i < sizeof( float); i++) {
    fputc(((char *)value)[i], file);
  }

  return sizeof( float);
}



void *C4SNet_deserialize(FILE *file) 
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
      return (void *)C4SNet_cdataCreate((void *)i_ptr, 
					C4SNetFree,
					C4SNetCopyInt,
					C4SNetSerializeInt,
					C4SNetEncodeInt);
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
	
	return (void *)C4SNet_cdataCreate((void *)f_ptr, 
					  C4SNetFree,
					  C4SNetCopyFloat,
					  C4SNetSerializeFloat,
					  C4SNetEncodeFloat);
      }
    }
  }
 
  return NULL;
}
/*
void *C4SNet_decode(FILE *file) 
{
  return C4SNet_deserialize(file); 
}
*/

void *C4SNet_decode(FILE *file) 
{
  char *ptr;
  int i;
  char c;

  c = fgetc(file);
  if(c == '<') {
    ungetc('<', file);
    return NULL;
  }

  switch(c){
  case ID_INT:
    ptr = (char *)malloc(sizeof( int));
    for(i = 0; i < sizeof(int); i++) {
      ptr[i] = fgetc(file);
      if(ptr[i] == '<') {
	ungetc('<', file);
	return NULL;
      }
    }

    return (void *)C4SNet_cdataCreate((void *)ptr, 
				      C4SNetFree,
				      C4SNetCopyInt,
				      C4SNetSerializeInt,
				      C4SNetEncodeInt);
    break;
  case ID_FLOAT:
    ptr = (char *)malloc(sizeof( float));
    for(i = 0; i < sizeof(float); i++) {
      ptr[i] = fgetc(file);
      if(ptr[i] == '<') {
	ungetc('<', file);
	return NULL;
      }
    }

    return (void *)C4SNet_cdataCreate((void *)ptr, 
				      C4SNetFree,
				      C4SNetCopyFloat,
				      C4SNetSerializeFloat,
				      C4SNetEncodeFloat);
    break;
  default:
    return NULL;
  }
  
  return NULL;
}

