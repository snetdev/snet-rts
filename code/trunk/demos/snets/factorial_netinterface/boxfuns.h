#ifndef _boxfuns_h_

#include <C2SNet.h>

void *condif( void *hnd, C_Data *p);

void *leq1( void *hnd, C_Data *x);

void *mult( void *hnd, C_Data *x, C_Data *r);

void *sub( void *hnd, C_Data *x);

#endif
