#ifndef DEFAULT_INTERFACE_H_
#define DEFAULT_INTERFACE_H_

/*******************************************************************************
 *
 * $Id$
 *
 * Author: Jukka Julku, VTT Technical Research Centre of Finland
 * -------
 *
 * Date:   18.4.2008
 * -----
 *
 * Description:
 * ------------
 *
 * Default S-Net language interface.
 *
 *******************************************************************************/

/* This interface is meant to handle unknown data (data with unknown language interface).
 * The data cannot be accessed in any way by the user. */

void SNet_default_interface_init( int id);

#endif /* DEFAULT_INTERFACE_H_ */
