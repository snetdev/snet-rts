#include "snetentities.h"
#include "distribution.h"

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

  SNetRouteUpdate(info, input, location);
  internal_stream = (*box_a)(input, info, location);
  output = (*box_b)(internal_stream, info, location);

  return(output);
}
