#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

#include "snettypes.h"
#include "stream_layer.h"
#include "fun.h"

int DistributionInit(int argc, char *argv[]);

snet_tl_stream_t *DistributionStart(snet_startup_fun_t fun);
void DistributionStop();
void DistributionDestroy();

snet_tl_stream_t *DistributionWaitForInput();
snet_tl_stream_t *DistributionWaitForOutput();

#endif /* _SNET_DISTRIBUTION_H_ */
