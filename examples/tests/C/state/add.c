#include <stdlib.h>
#include <stdio.h>

#include "add.h"

#define MAXVAL 10


/**
 * Update state by adding inval and emitting new state and old state as output
 * if new state >= MAXVAL, no new state is emitted.
 */
void *add( void *hnd, c4snet_data_t *state, c4snet_data_t *inval)
{
  int int_state, int_newstate, int_inval;
  c4snet_data_t *data_newstate, *data_output;

  int_state = *(int *) C4SNetDataGetData(state);
  int_inval = *(int *) C4SNetDataGetData(inval);

  /* update state */
  int_newstate = int_state + int_inval;

  if (int_newstate < MAXVAL) {
    data_newstate = C4SNetDataCreate( CTYPE_int, &int_newstate);
    C4SNetOut( hnd, 1, data_newstate);
  }

  /* emit old state as output */
  data_output = C4SNetDataCreate( CTYPE_int, &int_state);
  C4SNetOut( hnd, 2, data_output);


  C4SNetDataFree(state);
  C4SNetDataFree(inval);
  return( hnd);
}

