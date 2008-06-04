#ifndef SNET_OBSERVERS_H_
#define SNET_OBSERVERS_H_

/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   4.4.2008
 * -----
 *
 * Description:
 * ------------
 *
 * SNet observers
 *
 *******************************************************************************/

#include "buffer.h"
#include "bool.h"
#include "label.h"
#include "interface.h"

/* Observer data levels. */
#define SNET_OBSERVERS_DATA_LEVEL_NONE 0
#define SNET_OBSERVERS_DATA_LEVEL_TAGS 1
#define SNET_OBSERVERS_DATA_LEVEL_FULL 2

/* Observer types. */
#define SNET_OBSERVERS_TYPE_BEFORE 0
#define SNET_OBSERVERS_TYPE_AFTER 1

snet_buffer_t *SNetObserverSocketBox(snet_buffer_t *inbuf, 
				     const char *addr,
				     int port, 
				     bool interactive, 
				     const char *position, 
				     char type, 
				     char data_level, 
				     const char *code);

snet_buffer_t *SNetObserverFileBox(snet_buffer_t *inbuf, 
				   const char *filename, 
				   const char *position, 
				   char type, 
				   char data_level, 
				   const char *code);

extern int SNetObserverInit(snetin_label_t *labels, 
			    snetin_interface_t *interfaces);

extern void SNetObserverDestroy();

#endif /* SNET_OBSERVERS_H_ */
