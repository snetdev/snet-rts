#ifndef __SVPSNETGWRT_TESTING_TRUN_H
#define __SVPSNETGWRT_TESTING_TRUN_H

#include "snet-gwrt.utc.h"
#include "snet-gwrt.int.utc.h"

/*----------------------------------------------------------------------------*/
/* Unit tests */

extern void idxvec_test();
extern void record_test();
extern void typeencode_test();
extern void references_test();

/*---*/

extern void list_test(const snet_domain_t *snetd);
extern void graph_test(const snet_domain_t *snetd);
extern void buffer_test(const snet_domain_t *snetd);
extern void conslst_test(const snet_domain_t *snetd);

/*---*/

extern void net_ioproc_test(snet_domain_t *snetd);

/*----------------------------------------------------------------------------*/
/* S-Nets */

extern int
SNetMain__ts00(int argc, char* argv[]);

extern bool
SNet__ts00___setup(snet_domain_t *snetd);

/*---*/

extern int
SNetMain__ts01(int argc, char* argv[]);

extern bool
SNet__ts01___setup(snet_domain_t *snetd);

/*---*/

extern int
SNetMain__ts02(int argc, char* argv[]);

extern bool
SNet__ts02___setup(snet_domain_t *snetd);

#endif // __SVPSNETGWRT_TESTING_TRUN_H

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
