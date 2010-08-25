

#include "boxfun.h"
#include <snetentities.h>
#include <memfun.h>
#include <bool.h>

snet_handle_t *leq1( snet_handle_t *hnd, void *field_x) {

  bool *p;
  int *f_x;
  

  f_x = SNetMemAlloc( sizeof( int));
  *f_x = *(int*)field_x; 

  p = SNetMemAlloc( sizeof( bool));

  if( *f_x <= 1) {
    *p = true;
  }
  else {
    *p = false;
  }
  
  SNetOutRaw( hnd, 1, (void*)f_x, (void*)p);

  return( hnd);
}


snet_handle_t *condif( snet_handle_t *hnd, void *field_p) {

   bool *p = (bool*) field_p;

  if( *p) {
    SNetOutRaw( hnd, 1, 0);
  }
  else {
    SNetOutRaw( hnd, 2, 0);
  }

  SNetMemFree( p);

  return( hnd);
}



snet_handle_t *dupl( snet_handle_t *hnd, void *field_x, void *field_r) {

  int *f_x1, *f_x2, *f_r;
  
  f_x1 = SNetMemAlloc( sizeof( int));
  *f_x1 = *(int*)field_x;

  f_x2 = SNetMemAlloc( sizeof( int));
  *f_x2 = *(int*)field_x;
  
  f_r = SNetMemAlloc( sizeof( int));
  *f_r = *(int*)field_r;


  SNetOutRaw( hnd, 1, (void*)f_x1);
  SNetOutRaw( hnd, 2, (void*)f_x2, (void*)f_r);

  return( hnd);
}


snet_handle_t *sub( snet_handle_t *hnd, void *field_x) {

  int *a;

  a = SNetMemAlloc( sizeof( int));

  *a = *((int*)field_x) - 1;
  SNetOutRaw( hnd, 1, (void*)a);

  return( hnd);
}


snet_handle_t *mult( snet_handle_t *hnd, void *field_x, void *field_r) {

  int *b;

  b = SNetMemAlloc( sizeof( int));

  *b = *((int*)field_x) * *((int*)field_r);
  SNetOutRaw( hnd, 1, (void*)b);

  return( hnd);
}

