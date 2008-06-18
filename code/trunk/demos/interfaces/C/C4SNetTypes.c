#include <stdlib.h>
#include <stdio.h>
#include <C4SNetTypes.h>
#include <C4SNet.h>
#include <string.h>
#include "memfun.h"
#include "base64.h"

void C4SNetFree( void *ptr)
{
  SNetMemFree( ptr);
}

void *C4SNetCopy( void *ptr)
{
  C_Data *new;

  new = SNetMemAlloc( sizeof( C_Data));

  new->data = ((C_Data *)ptr)->data;
  new->type = ((C_Data *)ptr)->type;

  return( new);
}

int C4SNetSerialize( FILE *file, void *ptr)
{
  C_Data *temp = (C_Data *)ptr;

  switch(temp->type) {
  case CTYPE_uchar: 
    return fprintf(file, "(unsigned char)%c", (char)(temp->data.uc));
    break;
  case CTYPE_char: 
    return fprintf(file, "(char)%c", (char)(temp->data.c));
    break;
  case CTYPE_ushort: 
    return fprintf(file, "(unsigned short)%hu", (unsigned short)temp->data.us);
    break;
  case CTYPE_short: 
    return fprintf(file, "(short)%hd", (short)temp->data.s);
    break;
  case CTYPE_uint: 
    return fprintf(file, "(unsigned int)%u", (unsigned int)temp->data.ui);
    break;
  case CTYPE_int: 
    return fprintf(file, "(int)%d", (int)temp->data.i);
    break;
  case CTYPE_ulong:
    return fprintf(file, "(unsigned long)%lu", (unsigned long)temp->data.ul);
    break;
  case CTYPE_long:
     return fprintf(file, "(long)%ld", (long)temp->data.l);
    break;
  case CTYPE_float: 
    return fprintf(file, "(float)%f", (double)temp->data.f);	
    break;
  case CTYPE_double: 
    return fprintf(file, "(double)%f", (double)temp->data.d);
    break;
  default:
    return 0;
    break;
  }

  return 0;
}

int C4SNetEncode( FILE *file, void *ptr){
  C_Data *temp = (C_Data *)ptr;
  int size = 0;
  unsigned char *from = 0;
  int i = 0;
  unsigned char c;


  switch(temp->type) {
  case CTYPE_uchar: 
    size = sizeof(char);
    from = (unsigned char *)&temp->data.uc;
    break;
  case CTYPE_char: 
    size = sizeof(signed char);
    from = (unsigned char *)&temp->data.c;
    break;
  case CTYPE_ushort: 
    size = sizeof(unsigned short);
    from = (unsigned char *)&temp->data.us;
    break;
  case CTYPE_short: 
    size = sizeof(short);
    from = (unsigned char *)&temp->data.s;
    break;
  case CTYPE_uint: 
    size = sizeof(unsigned int);
    from = (unsigned char *)&temp->data.ui;
  break;
  case CTYPE_int: 
    size = sizeof(int);
    from = (unsigned char *)&temp->data.i;
    break;
  case CTYPE_ulong:
    size = sizeof(unsigned long);
    from = (unsigned char *)&temp->data.ul;
    break;
  case CTYPE_long:
    size = sizeof(long);
    from = (unsigned char *)&temp->data.l;
    break;
  case CTYPE_float: 
    size = sizeof(float);
    from = (unsigned char *)&temp->data.f;	
    break;
  case CTYPE_double: 
    size = sizeof(float);
    from = (unsigned char *)&temp->data.d;
    break;
  default:
    return 0;
    break;
  }

  c = (unsigned char)temp->type;
  i = Base64encode(file, &c, 1);
  i += Base64encode(file, from, size);

  return i + 1;
}

void *C4SNetDeserialize(FILE *file) 
{
  char buf[32];
  int j;
  char c;
  cdata_types_t t;

  j = 0;
  c = '\0';
  while((c = fgetc(file)) != EOF) {
    buf[j++] = c;
    if(c == '<') {
      ungetc('<', file);
      return NULL;
    }
    if(c == ')') {
      break;
    }
  } 

  buf[j] = '\0';

  if(strcmp(buf, "(unsigned char)") == 0) {
    if(fscanf(file, "%c", &t.c) == 1) {
      t.uc = (unsigned char)t.c;
      return C4SNet_cdataCreate( CTYPE_uchar, &t.uc);
    }
  } 
  else if(strcmp(buf, "(char)") == 0) {
    if(fscanf(file, "%c", &t.c) == 1) {
      return C4SNet_cdataCreate( CTYPE_char, &t.c);
    }
  }
  else if(strcmp(buf, "(unsigned short)") == 0) {
    if( fscanf(file, "%hu", &t.us) == 1) {
      return C4SNet_cdataCreate( CTYPE_ushort, &t.us);
    }
  }
  else if(strcmp(buf, "(short)") == 0) {
    if( fscanf(file, "%hd", &t.s) == 1) {
      return C4SNet_cdataCreate( CTYPE_short, &t.s);
    }
  }
  else if(strcmp(buf, "(unsigned int)") == 0) {
    if( fscanf(file, "%u", &t.ui) == 1) {
      return C4SNet_cdataCreate( CTYPE_uint, &t.ui);
    }
  }
  else if(strcmp(buf, "(int)") == 0) {
    if( fscanf(file, "%d", &t.i) == 1) {
      return C4SNet_cdataCreate( CTYPE_int, &t.i);
    }
  }
  else if(strcmp(buf, "(unsigned long)") == 0) {
    if( fscanf(file, "%lu", &t.ul) == 1) {
      return C4SNet_cdataCreate( CTYPE_ulong, &t.ul);
    }
  }
  else if(strcmp(buf, "(long)") == 0) {
    if( fscanf(file, "%ld", &t.l) == 1) {
      return C4SNet_cdataCreate( CTYPE_long, &t.l);
    }
  }
  else if(strcmp(buf, "(float)") == 0) {
    if( fscanf(file, "%f", &t.f) == 1) {
      return C4SNet_cdataCreate( CTYPE_float, &t.f);
    }
  }
  else if(strcmp(buf, "(double)") == 0) {
    if( fscanf(file, "%f", &t.d) == 1) {
      return C4SNet_cdataCreate( CTYPE_double, &t.d);
    }
  }
 
  return NULL;
}

void *C4SNetDecode(FILE *file) 
{
  int size = 0;
  unsigned char *to = 0;
  int i = 0;

  cdata_types_t data;
  ctype_t type;
  unsigned char c;
  int j;

  j = 0;

  i = Base64decode(file, &c, 1);

  type = (ctype_t)c;

  switch(type) {
  case CTYPE_uchar: 
    size = sizeof(char);
    to = (unsigned char *)&data.uc;
    break;
  case CTYPE_char: 
    size = sizeof(signed char);
    to = (unsigned char *)&data.c;
    break;
  case CTYPE_ushort: 
    size = sizeof(unsigned short);
    to = (unsigned char *)&data.us;
    break;
  case CTYPE_short: 
    size = sizeof(short);
    to = (unsigned char *)&data.s;
    break;
  case CTYPE_uint: 
    size = sizeof(unsigned int);
    to = (unsigned char *)&data.ui;
    break;
  case CTYPE_int: 
    size = sizeof(int);
    to = (unsigned char *)&data.i;
    break;
  case CTYPE_ulong:
    size = sizeof(unsigned long);
    to = (unsigned char *)&data.ul;
    break;
  case CTYPE_long:
    size = sizeof(long);
    to = (unsigned char *)&data.l;
    break;
  case CTYPE_float: 
    size = sizeof(float);
    to = (unsigned char *)&data.f;	
    break;
  case CTYPE_double: 
    size = sizeof(float);
    to = (unsigned char *)&data.d;
    break;
  default:
    return NULL;
    break;
  }

  i += Base64decode(file, to, size);

  return C4SNet_cdataCreate( type, (void *)to);
}
