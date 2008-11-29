#ifndef _TEST_SNET_H_
#define _TEST_SNET_H_

#include "snet-gwrt.utc.h"

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

#define E__test__NONE 0
#define F__test__a 1
#define F__test__b 2
#define F__test__c 3
#define F__test__d 4
#define F__test__e 5
#define F__test__f 6
#define T__test__T 7

/*----------------------------------------------------------------------------*/

#define SNET__test__NUMBER_OF_LABELS     8
#define SNET__test__NUMBER_OF_INTERFACES 0


/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

extern const char *snet_test_labels[];
extern const char *snet_test_interfaces[];

/*----------------------------------------------------------------------------*/

extern bool SNet__test___setup(snet_domain_t *snetd);

#endif // _TEST_SNET_H_

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/

