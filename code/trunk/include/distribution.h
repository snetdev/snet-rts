#ifndef _SNET_DISTRIBUTION_H_
#define _SNET_DISTRIBUTION_H_

#include "snettypes.h"
#include "stream_layer.h"
#include "fun.h"

snet_tl_stream_t *DistributionInit(int argc, char *argv[], snet_startup_fun_t fun);

int DistributionDestroy();

snet_tl_stream_t *DistributionWaitForInput();
snet_tl_stream_t *DistributionWaitForOutput();

#endif /* _SNET_DISTRIBUTION_H_ */
