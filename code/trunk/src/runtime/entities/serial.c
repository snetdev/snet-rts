#include "serial.h"
#include "buffer.h"

/* ------------------------------------------------------------------------- */
/*  SNetSerial                                                               */
/* ------------------------------------------------------------------------- */

extern snet_buffer_t *SNetSerial( snet_buffer_t *inbuf, 
                                  snet_buffer_t* (*box_a)( snet_buffer_t*),
                                  snet_buffer_t* (*box_b)( snet_buffer_t*)) {

  snet_buffer_t *internal_buf;
  snet_buffer_t *outbuf;

  internal_buf = (*box_a)( inbuf);
  outbuf = (*box_b)( internal_buf);

  return( outbuf);
}
