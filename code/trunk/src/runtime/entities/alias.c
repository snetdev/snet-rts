#include "alias.h"
#include "buffer.h"
/* ------------------------------------------------------------------------- */
/*  SNetAlias                                                                */
/* ------------------------------------------------------------------------- */

extern snet_buffer_t *SNetAlias( snet_buffer_t *inbuf, 
                                 snet_buffer_t *(*net)(snet_buffer_t*)) {

  return( net( inbuf));
}
