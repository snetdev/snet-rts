#ifndef _SAC4SNet_h_
#define _SAC4SNet_h_

/* SAC4SNetc generated code requires this: */
#include <stdlib.h>
#include <assert.h>

#include "handle.h"
#include "sacinterface.h"
#include "type.h"
#include "variant.h"

typedef struct container sac4snet_container_t;

struct handle;

void SAC4SNetInit( int id, snet_distrib_t distImpl);

void SAC4SNet_out( void *hnd, int variant, ...);

int SAC4SNetParseIntCSV(const char *csv, int *nums);

void SAC4SNetDebugPrintMapping(const char *msg, int *defmap, int cnt);

#endif 
