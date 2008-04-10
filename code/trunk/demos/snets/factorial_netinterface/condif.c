#include <condif.h>
#include <stdlib.h>
#include <myfuns.h>
#include <bool.h>
#include <stdio.h>


void *condif( void *hnd, C_Data *p)
{
  bool *bool_p;

  c2snet_container_t *c;


  bool_p = (bool *)C2SNet_cdataGetData( p);
  
  if(*bool_p) {
    c = C2SNet_containerCreate( hnd, 1);
  } 
  else {
    c = C2SNet_containerCreate( hnd, 2);
  }

  free(bool_p);
  free(p);

  C2SNet_containerSetTag( c, 0);

  C2SNet_outCompound( c);

  return( hnd);
}
