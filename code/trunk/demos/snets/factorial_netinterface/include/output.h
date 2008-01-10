#ifndef OUTPUT_H_
#define OUTPUT_H_

/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   10.1.2008
 * -----
 *
 * Description:
 * ------------
 *
 * Output functions for S-NET network interface.
 *
 *******************************************************************************/

#include <record.h>
#include <buffer.h>
#include <label.h>
#include <interface.h>

/* Init output system to use given labels and language interfaces.
 *
 * @param labels Labels to be used for fields/tags/btags
 * @param interfaces Language interface mappings to be used.
 *
 */

void initOutput(label_t *labels, interface_t *interfaces);


/* Start output system.
 *
 * @param in_buf Buffer from where the records to output are taken.
 *
 * @return 0   Action is succesful.
 * @return -1 Error occured while starting the system.
 *
 */
int startOutput(snet_buffer_t *in_buf);


/* Stop the output system.
 *
 * @return 0   Action is succesful.
 * @return -1 Error occured while stopping the system.
 *
 */
int blockUntilEndOfOutput();


#endif /* OUTPUT_H_ */
