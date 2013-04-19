#include "node.h"

snet_stream_t *SNetSerial(
    snet_stream_t *input,
    snet_info_t *info,
    int location,
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  snet_stream_t *internal, *output;
  snet_locvec_t *locvec;
  bool enterstate;

  trace(__func__);

  locvec = SNetLocvecGet(info);
  enterstate = SNetLocvecSerialEnter(locvec);
    
  /* create operand A */
  internal = (*box_a)(input, info, location);

  SNetLocvecSerialNext(locvec);

  /* create operand B */
  output = (*box_b)(internal, info, location);

  SNetLocvecSerialLeave(locvec, enterstate);

  return output;
}

