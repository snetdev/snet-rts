#ifndef _SAC4SNet_h_
#define _SAC4SNet_h_

#include <assert.h> /* SAC4SNetc generated code requires this */

#include "handle.h"
#include "sacinterface.h"
#include "type.h"
#include "variant.h"

typedef struct container sac4snet_container_t;

struct handle;

void SAC4SNetInit( int id, snet_distrib_t distImpl);

void SAC4SNet_out( snet_handle_t *hnd, int variant, ...);

#endif 
