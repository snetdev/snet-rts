#include "snetentities.h"

/* ------------------------------------------------------------------------- */
/*  SNetAlias                                                                */
/* ------------------------------------------------------------------------- */
snet_stream_t *SNetAlias( snet_stream_t *inbuf,
                          snet_info_t *info,
                          int location,
                          snet_startup_fun_t net)
{
  return net( inbuf, info, location);
}
