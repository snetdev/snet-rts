#ifndef OUTPUT_H_
#define OUTPUT_H_

/*******************************************************************************
 *
 * $Id: output.h 2891 2010-10-27 14:48:34Z dlp $
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

#include <stdio.h>
#include "record.h"
#include "label.h"
#include "interface.h"


/* Init output system.
 *
 * @param file File where the output is printed to.
 * @param labels Set of labels to use.
 * @param interfaces Set of interfaces to use.
 * @param in_buf Buffer from where the records to output are taken.
 *
 */
void SNetInOutputInit(FILE *file,
		     snetin_label_t *labels, 
#ifdef DISTRIBUTED_SNET
		     snetin_interface_t *interfaces);
#else /* DISTRIBUTED_SNET */
                     snetin_interface_t *interfaces,
                     snet_stream_t *in_buf);
#endif/* DISTRIBUTED_SNET */
           



#endif /* OUTPUT_H_ */
