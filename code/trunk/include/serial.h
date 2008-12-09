#ifndef SERIAL_HEADER
#define SERIAL_HEADER

#include "snettypes.h"
#include "stream_layer.h"

snet_tl_stream_t *SNetSerial(snet_tl_stream_t *instream,
#ifdef DISTRIBUTED_SNET
			     snet_dist_info_t *info, 
			     int location,
#endif /* DISTRIBUTED_SNET */
			     snet_startup_fun_t box_a,
			     snet_startup_fun_t box_b);
#endif
