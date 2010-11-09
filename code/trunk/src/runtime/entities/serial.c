#include "snetentities.h"
#include "buffer.h"
#include "debug.h"

#ifdef DISTRIBUTED_SNET
#include "routing.h"
#endif /* DISTRIBUTED_SNET */

//#define SERIAL_DEBUG

/* ------------------------------------------------------------------------- */
/*  SNetSerial                                                               */
/* ------------------------------------------------------------------------- */

extern snet_tl_stream_t* SNetSerial(snet_tl_stream_t *input, 
#ifdef DISTRIBUTED_SNET
				    snet_info_t *info, 
				    int location,
#endif /* DISTRIBUTED_SNET */
				    snet_startup_fun_t box_a,
				    snet_startup_fun_t box_b)
{
  snet_tl_stream_t *internal_stream;
  snet_tl_stream_t *output;

#ifdef DISTRIBUTED_SNET
#ifdef DISTRIBUTED_BUILD_SERIAL
  /* This section forces input stream into root node. 
   * Otherwise, the root node is ignored.
   */
  input = SNetRoutingContextUpdate(SNetInfoGetRoutingContext(info), input, location);

#endif /* DISTRIBUTED_BUILD_SERIAL */
#endif /* DISTRIBUTED_SNET */

#ifdef SERIAL_DEBUG
  SNetUtilDebugNotice("Serial creation started");
  SNetUtilDebugNotice("box_a = %p, box_b = %p", box_a, box_b);
#endif

#ifdef DISTRIBUTED_SNET
  internal_stream = (*box_a)(input, info, location);
#else
  internal_stream = (*box_a)(input);
#endif /* DISTRIBUTED_SNET */

#ifdef SERIAL_DEBUG
  SNetUtilDebugNotice("Serial creation halfway done");
#endif

#ifdef DISTRIBUTED_SNET
  output = (*box_b)(internal_stream, info, location);
#else
  output = (*box_b)(internal_stream);
#endif /* DISTRIBUTED_SNET */

#ifdef SERIAL_DEBUG
  SNetUtilDebugNotice("-");
  SNetUtilDebugNotice("| SERIAL CREATED");
  SNetUtilDebugNotice("| input: %p", input);
  SNetUtilDebugNotice("| middle stream: %p", internal_stream);
  SNetUtilDebugNotice("| output: %p", output);
  SNetUtilDebugNotice("-");
#endif

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
