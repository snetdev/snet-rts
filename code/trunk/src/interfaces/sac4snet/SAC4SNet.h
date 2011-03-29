#ifndef _SAC4SNet_h_
#define _SAC4SNet_h_

#include "sacinterface.h"
#include "typeencode.h"

typedef struct container sac4snet_container_t;

struct handle;

void SAC4SNetInit( int id);

void SAC4SNet_outCompound( sac4snet_container_t *c);
void SAC4SNet_out( void *hnd, int variant, ...);

sac4snet_container_t *SAC4SNet_containerCreate( struct handle *hnd, snet_typeencoding_t *type, int var_num);
sac4snet_container_t *SAC4SNet_containerSetField( sac4snet_container_t *c, void *ptr);
sac4snet_container_t *SAC4SNet_containerSetTag( sac4snet_container_t *c, int val);
sac4snet_container_t *SAC4SNet_containerSetBTag( sac4snet_container_t *c, int val);
#endif 
