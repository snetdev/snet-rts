#ifndef _SAC4SNet_h_
#define _SAC4SNet_h_

#include <sacinterface.h>

typedef struct container sac4snet_container_t;

void SAC4SNetInit( int id);

void SAC4SNet_outCompound( sac4snet_container_t *c);
void SAC4SNet_out( void *hnd, int variant, ...);

sac4snet_container_t *SAC4SNet_containerCreate( void *hnd, int var_num);
sac4snet_container_t *SAC4SNet_containerSetField( sac4snet_container_t *c, void *ptr);
sac4snet_container_t *SAC4SNet_containerSetTag( sac4snet_container_t *c, int val);
sac4snet_container_t *SAC4SNet_containerSetBTag( sac4snet_container_t *c, int val);
#endif 
