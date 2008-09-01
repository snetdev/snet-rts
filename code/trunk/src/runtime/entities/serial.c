#include "serial.h"
#include "buffer.h"
#include "debug.h"
//#define SERIAL_DEBUG

/* ------------------------------------------------------------------------- */
/*  SNetSerial                                                               */
/* ------------------------------------------------------------------------- */

extern snet_tl_stream_t*
SNetSerial(snet_tl_stream_t *input, 
           snet_tl_stream_t* (*box_a)(snet_tl_stream_t*),
           snet_tl_stream_t* (*box_b)(snet_tl_stream_t*)) {

  snet_tl_stream_t *internal_stream;
  snet_tl_stream_t *output;

  #ifdef SERIAL_DEBUG
  SNetUtilDebugNotice("Serial creation started");
  SNetUtilDebugNotice("box_a = %p, box_b = %p", box_a, box_b);
  #endif
  internal_stream = (*box_a)(input);

  #ifdef SERIAL_DEBUG
  SNetUtilDebugNotice("Serial creation halfway done");
  #endif
  output = (*box_b)(internal_stream);

  #ifdef SERIAL_DEBUG
  SNetUtilDebugNotice("-");
  SNetUtilDebugNotice("| SERIAL CREATED");
  SNetUtilDebugNotice("| input: %p", input);
  SNetUtilDebugNotice("| middle stream: %p", internal_stream);
  SNetUtilDebugNotice("| output: %p", output);
  SNetUtilDebugNotice("-");
  #endif
  return(output);
}
