#ifndef NETWORK_INTERFACE_H
#define NETWORK_INTERFACE_H

/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   12.5.2008
 * -----
 *
 * Description:
 * ------------
 *
 * Main function for S-NET network interface.
 *
 *******************************************************************************/

#include <stdio.h>
#include "snettypes.h"
#include "label.h"
#include "interface.h"
/* Init SNet and start network interface.
 *
 * @notice This call does not initialize any language interfaces!
 *
 * @param argc
 * @param argv 
 * @param const_labels Array of static label names.
 * @param number_of_labels Number of labels in const_labels.
 * @param const_interfaces Array of static interface names.
 * @param number_of_interfaces Number of interfaces in const_interfaces.
 * @param snetfun Topmost snet function.
 *
 * @return 0 if terminated succesfully.
 *
 */

typedef enum snet_runtime {
  Streams,
  Front,
} snet_runtime_t;

int SNetInRun(int argc, char *argv[],
	      char *const_labels[], int number_of_labels, 
	      char *const_interfaces[], int number_of_interfaces, 
	      snet_startup_fun_t fun);

void SNetRuntimeHelpText(void);
void SNetRuntimeStartWait(snet_stream_t *in, snet_info_t *info, snet_stream_t *out);
snet_runtime_t SNetRuntimeGet(void);

#endif /* NETWORK_INTERFACE_H */
