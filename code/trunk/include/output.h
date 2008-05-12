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

/* Init output system.
 *
 * @param labels Set of labels to use.
 * @param interfaces Set of interfaces to use.
 * @param in_buf Buffer from where the records to output are taken.
 *
 * @return 0   Action is succesful.
 * @return -1 Error occured while starting the system.
 *
 */
int SNetInOutputInit(snetin_label_t *labels, 
		     snetin_interface_t *interfaces,
		     snet_buffer_t *in_buf);


/* Wait until the end of output.
 *
 * @return 0   Action is succesful.
 * @return -1 Error occured while stopping the system.
 *
 */
int SNetInOutputDestroy();


#endif /* OUTPUT_H_ */
