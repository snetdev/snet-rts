#include <condif.h>
#include <stdlib.h>
#include <bool.h>
#include <stdio.h>


void *condif( void *hnd, c4snet_data_t *p)
{
  bool bool_p;

  c4snet_container_t *c;


  bool_p = *(bool *)C4SNetDataGetData( p);
 
  if(bool_p) {
    c = C4SNetContainerCreate( hnd, 1);
  } 
  else {
    c = C4SNetContainerCreate( hnd, 2);
  }
  
  C4SNetDataFree(p);


  C4SNetContainerSetTag( c, 0);
  
  C4SNetContainerOut( c);
  
  return( hnd);
}
