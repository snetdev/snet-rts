#ifndef boxfun_h
#define boxfun_h

#include <handle.h>

snet_handle_t *leq1( snet_handle_t *hnd, void *field_x);
snet_handle_t *condif( snet_handle_t *hnd, void *field_p);
snet_handle_t *dupl( snet_handle_t *hnd, void *field_x, void *field_r);
snet_handle_t *sub( snet_handle_t *hnd, void *field_x);
snet_handle_t *mult( snet_handle_t *hnd, void *field_x, void *field_r);

#endif
