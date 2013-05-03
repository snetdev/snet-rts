#include <stdlib.h>
#include <stdio.h>

#include "compute.h"

#define MAXVAL 10


/**
 * Update state by adding inval and emitting new state and old state as output
 * if new state >= MAXVAL, a trigger is emitted instead of a new state.
 */
void *compute( void *hnd, c4snet_data_t *state, c4snet_data_t *inval)
{
  int int_state, int_newstate, int_inval;
  c4snet_data_t *data_newstate, *data_output;

  int_state = *(int *) C4SNetGetData(state);
  int_inval = *(int *) C4SNetGetData(inval);

  /* emit outval */
  data_output = C4SNetCreate(CTYPE_int, 1, &int_state);
  C4SNetOut( hnd, 1, data_output);

  /* update state */
  int_newstate = int_state + int_inval;

  if (int_newstate < MAXVAL) {
    /* emit state */
    data_newstate = C4SNetCreate(CTYPE_int, 1, &int_newstate);
    C4SNetOut( hnd, 2, data_newstate);
  } else {
    /* emit trigger */
    data_newstate = C4SNetCreate(CTYPE_int, 1, &int_newstate);
    C4SNetOut( hnd, 3, data_newstate);
  }

  C4SNetFree(state);
  C4SNetFree(inval);
  return( hnd);
}

