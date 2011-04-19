#include <assert.h>

#include "snetentities.h"
#include "distribution.h"

#include "locvec.h"

/* ------------------------------------------------------------------------- */
/*  SNetSerial                                                               */
/* ------------------------------------------------------------------------- */



/**
 * Serial connector creation function
 */
snet_stream_t *SNetSerial(snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  snet_stream_t *internal_stream;
  snet_stream_t *output;
  snet_locvec_t *locvec;
  bool was_serial = true;

  SNetRouteUpdate(info, input, location);

  locvec = SNetLocvecGet(info);
  if (SNetLocvecToptype(locvec) != LOC_SERIAL) {
    SNetLocvecAppend(locvec, LOC_SERIAL, 1);
    was_serial = false;
  }

  /* create operand A */
  internal_stream = (*box_a)(input, info, location);

  assert( SNetLocvecToptype(locvec) == LOC_SERIAL );
  SNetLocvecTopinc(locvec);

  /* create operand B */
  output = (*box_b)(internal_stream, info, location);

  if (!was_serial) {
    SNetLocvecPop(locvec);
  }

  return(output);
}
