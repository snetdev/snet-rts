#include "snetentities.h"
#include "stream.h"

/* ------------------------------------------------------------------------- */
/*  SNetAlias                                                                */
/* ------------------------------------------------------------------------- */

/**
 * Alias creation function
 */
stream_t *SNetAlias( stream_t *inbuf,
#ifdef DISTRIBUTED_SNET
    snet_info_t *info, 
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
