#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

#include "snettypes.h"
#include "fun.h"

int DistributionInit(int argc, char *argv[]);

void DistributionStart(snet_startup_fun_t fun);
void DistributionStop();
void DistributionDestroy();

snet_stream_t *DistributionWaitForInput();
snet_stream_t *DistributionWaitForOutput();

#endif /* _SNET_DISTRIBUTION_H_ */
