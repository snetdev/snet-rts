#include "snetentities.h"
//#include "lpel.h"

/* ------------------------------------------------------------------------- */
/*  SNetAlias                                                                */
/* ------------------------------------------------------------------------- */

/**
 * Alias creation function
 */
snet_stream_t *SNetAlias( snet_stream_t *inbuf,
    snet_info_t *info, 
#ifdef DISTRIBUTED_SNET
    int location,
#endif /* DISTRIBUTED_SNET */
    snet_startup_fun_t net)
{
#ifdef DISTRIBUTED_SNET
  return( net( inbuf, info, location));
#else
  return( net( inbuf, info));
#endif /* DISTRIBUTED_SNET */
}
