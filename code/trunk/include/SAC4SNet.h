#ifndef _SAC4SNet_h_
#define _SAC4SNet_h_

#include "sacinterface.h"
#include "type.h"
#include "variant.h"

typedef struct container sac4snet_container_t;

struct handle;

void SAC4SNetInit( int id);

void SAC4SNet_out( void *hnd, int variant, ...);

#endif 
