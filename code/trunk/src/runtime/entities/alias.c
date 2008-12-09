#include "alias.h"
#include "stream_layer.h"
/* ------------------------------------------------------------------------- */
/*  SNetAlias                                                                */
/* ------------------------------------------------------------------------- */

extern snet_tl_stream_t *SNetAlias( snet_tl_stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
				    snet_dist_info_t *info, 
				    int location,
#endif /* DISTRIBUTED_SNET */
				    snet_startup_fun_t net)
{
#ifdef DISTRIBUTED_SNET
  return( net( inbuf, info, location));
#else
  return( net( inbuf));
#endif /* DISTRIBUTED_SNET */
}
