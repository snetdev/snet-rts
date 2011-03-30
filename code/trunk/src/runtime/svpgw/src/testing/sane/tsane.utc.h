#ifndef SVPSNETGWRT_TESTING_TSANE_H
#define SVPSNETGWRT_TESTING_TSANE_H

#include "sane.utc.h"
#include "snetgwsane.int.utc.h"

/*---*/

#include "dtypes.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

typedef struct {
    int a;
    int b;

} handle_t;

/*---*/

extern thread void foo(int a, int b);
extern thread void bar(handle_t *hnd);

/*---*/

extern thread void 
copy_data(snet_ref_t ref, shared void *ptr, shared uint32_t sz);

#endif // SVPSNETGWRT_TESTING_TSANE_H

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

