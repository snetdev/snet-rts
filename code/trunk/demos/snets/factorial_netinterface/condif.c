#include <condif.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>


void *condif( void *hnd, C_Data *p)
{
  bool *bool_p;

  c4snet_container_t *c;


  bool_p = (bool *)C4SNet_cdataGetData( p);
  
  if(*bool_p) {
    c = C4SNet_containerCreate( hnd, 1);
  } 
  else {
    c = C4SNet_containerCreate( hnd, 2);
  }

  free(bool_p);
  free(p);

  C4SNet_containerSetTag( c, 0);

  C4SNet_outCompound( c);

  return( hnd);
}
