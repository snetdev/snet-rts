#include "alias.h"
#include "stream_layer.h"
/* ------------------------------------------------------------------------- */
/*  SNetAlias                                                                */
/* ------------------------------------------------------------------------- */

extern snet_tl_stream_t *SNetAlias( snet_tl_stream_t *inbuf,
                                 snet_tl_stream_t *(*net)(snet_tl_stream_t*)) {

  return( net( inbuf));
}
