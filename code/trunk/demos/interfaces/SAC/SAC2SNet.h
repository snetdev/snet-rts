#ifndef _SAC2SNet_h_
#define _SAC2SNet_h_

#include <sacinterface.h>

typedef struct container sac2snet_container_t;

void SAC2SNet_init( int id);

void SAC2SNet_outCompound( sac2snet_container_t *c);
void SAC2SNet_out( void *hnd, int variant, ...);

sac2snet_container_t *SAC2SNet_containerCreate( void *hnd, int var_num);
sac2snet_container_t *SAC2SNet_containerSetField( sac2snet_container_t *c, void *ptr);
sac2snet_container_t *SAC2SNet_containerSetTag( sac2snet_container_t *c, int val);
sac2snet_container_t *SAC2SNet_containerSetBTag( sac2snet_container_t *c, int val);
#endif 
