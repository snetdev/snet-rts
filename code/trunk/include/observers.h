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

#ifndef SNET_OBSERVERS_H_
#define SNET_OBSERVERS_H_

#include <string.h>
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

/** This function creates a new S-Net observer
 *
 * @param inbuf Buffer for incoming records.
 * @param addr URL address of the listener.
 * @param port Port used by the listener.
 * @param interactive "true" if this is an interactive observer, false otherwise. 
 * @param position Position of this observer.
 * @param type Type of this observer. Must be one of the SNET_OBSERVER_TYPE_* values
 * @param data_level Data level provided by the observers. Must be one of the SNET_OBSERVER_LEVEL_* values.
 * @param code Optional application specific code. NULL if the code is not used.
 *
 * @return buffer used for outgoing records.
 *
 */

snet_buffer_t *SNetObserverBox(snet_buffer_t *inbuf, const char *addr, int port, 
			       bool interactive, const char *position, char type, 
			       char data_level, const char *code);

/** This function initializes the observer system.
 *
 * @param labels Set of labels to use.
 * @param interfaces Set of interfaces to use.
 *
 * @return 0, if the observer system was started correctly.
 *
 * @notice Any observers must NOT be created before this call!
 * @notice If return value was not 0, observer system should not be used!
 *
 */

extern int SNetObserverInit(snetin_label_t *labels, snetin_interface_t *interfaces);

/** This function destroys the observer system.
 *
 * @notice Any more observers must NOT be created after this call!
 *
 */

extern void SNetObserverDestroy();

#endif /* SNET_OBSERVERS_H_ */
