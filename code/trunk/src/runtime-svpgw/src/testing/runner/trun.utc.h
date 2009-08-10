#ifndef __SVPSNETGWRT_TESTING_TRUN_H
#define __SVPSNETGWRT_TESTING_TRUN_H

#include "snet.utc.h"
#include "snetgw.int.utc.h"

/*----------------------------------------------------------------------------*/
/* Unit tests */

extern void list_test();
extern void buffer_test();
extern void conslst_test();
extern void record_test();
extern void typeencode_test();
extern void references_test();

/*---*/

extern void idxvec_test();
extern void graph_test(const snet_domain_t *snetd);

/*---*/

extern void remote_create_test();

/*----------------------------------------------------------------------------*/
/* S-Nets */

extern 
snet_domain_t* 
SNetSetup__ts00();

extern int
SNetMain__ts00(int argc, char* argv[]);

/*---*/

extern 
snet_domain_t* 
SNetSetup__ts01();

extern int
SNetMain__ts01(int argc, char* argv[]);

/*---*/

extern 
snet_domain_t* 
SNetSetup__ts02();

extern int
SNetMain__ts02(int argc, char* argv[]);

/*---*/

extern 
snet_domain_t* 
SNetSetup__ocrad();

extern int
SNetMain__ocrad(int argc, char* argv[]);

#endif // __SVPSNETGWRT_TESTING_TRUN_H

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
