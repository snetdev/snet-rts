#include <boxfuns.h>
#include <stdlib.h>
#include <myfuns.h>
#include <bool.h>



void *condif( void *hnd, C_Data *x)
{
  bool bool_p;

  c2snet_container_t *c;


  bool_p = *(bool*)C2SNet_cdataGetData( x);
  
  if( bool_p) {
    c = C2SNet_containerCreate( hnd, 1);
  } 
  else {
    c = C2SNet_containerCreate( hnd, 2);
  }

  C2SNet_containerSetTag( c, 0);

  C2SNet_out( c);

  return( hnd);
}

void *leq1( void *hnd, C_Data *x)
{
  bool *bool_p;
  int int_x;

  C_Data *result;

  bool_p = malloc( sizeof( bool));

  int_x= *(int*)C2SNet_cdataGetData( x);
  

  *bool_p = ( int_x <= 1);

  result = C2SNet_cdataCreate( bool_p, &myfree, &mycopy);

  
  C2SNet_outRaw( hnd, 1, x, result);
  return( hnd);
}

void *mult( void *hnd, C_Data *x, C_Data *r)
{  
  C_Data *result;

  int int_x, int_r, *int_rr;
  int_rr = malloc( sizeof( int));

  int_x = *(int*) C2SNet_cdataGetData( x);
  int_r = *(int*) C2SNet_cdataGetData( r);
  *int_rr = int_x * int_r;

  result = C2SNet_cdataCreate( int_rr, &myfree, &mycopy);

  C2SNet_outRaw( hnd, 1, result);
  return( hnd);
}

void *sub( void *hnd, C_Data *x)
{
  int *int_x;
  c2snet_container_t *c;
  C_Data *result;

  int_x = malloc( sizeof( int));

  *int_x= *(int*)C2SNet_cdataGetData( x);
  *int_x -= 1;

  result = C2SNet_cdataCreate( int_x, &myfree, &mycopy);

  c = C2SNet_containerCreate( hnd, 1);
  C2SNet_containerSetField( c, result);

  C2SNet_out( c);

  return( hnd);
}


