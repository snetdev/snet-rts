#include "snetentities.h"

#include "stream.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif /* DISTRIBUTED_SNET */

//#define SERIAL_DEBUG

/* ------------------------------------------------------------------------- */
/*  SNetSerial                                                               */
/* ------------------------------------------------------------------------- */



/**
 * Serial connector creation function
 */
stream_t *SNetSerial(stream_t *input, 
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t box_a,
    snet_startup_fun_t box_b)
{
  stream_t *internal_stream;
  stream_t *output;

#ifdef DISTRIBUTED_SNET
#ifdef DISTRIBUTED_BUILD_SERIAL
  /* This section forces input stream into root node. 
   * Otherwise, the root node is ignored.
   */
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);
#endif /* DISTRIBUTED_BUILD_SERIAL */
#endif /* DISTRIBUTED_SNET */


#ifdef DISTRIBUTED_SNET
  internal_stream = (*box_a)(input, info, location);
  output = (*box_b)(internal_stream, info, location);
#else
  internal_stream = (*box_a)(input, info);
  output = (*box_b)(internal_stream, info);
#endif /* DISTRIBUTED_SNET */


#ifdef DISTRIBUTED_SNET
#ifdef DISTRIBUTED_BUILD_SERIAL
  /* This section forces output stream into root node. 
   * Otherwise, the root node is ignored.
   */ 
  output = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), output, location); 
#endif /* DISTRIBUTED_BUILD_SERIAL */
#endif /* DISTRIBUTED_SNET */
  return(output);
}
