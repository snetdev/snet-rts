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
 * Dispatcher used by SNet observer system.
 *
 *******************************************************************************/

#ifndef SNET_DISPATCHER_H_
#define SNET_DISPATCHER_H_

#include <bool.h>

/** This function call registers observer to listener in address addr listening port port.
 *
 * @param self Observer id of the calling observer.
 * @param addr URL address of the listener.
 * @param port Port which the listener listens to.
 * @param interactive "true", if the observer should wait for reply message, "false" otherwise.
 *
 * @return Descriptor of the connection. This might NOT be the actual file descriptor!
 * @return -1, if the connection could not be created.
 *
 */

int SNetDispatcherAdd(int self, const char *addr, int port, bool interactive);

/** This function call unregisters an observer-listener mapping made previously
 *  with call to SNetDispatcherAdd().
 *
 * @param self Observer id of the calling observer.
 * @param fid The descriptor of the connection returned by SNetDispatcherAdd().
 *
 */

void SNetDispatcherRemove(int self, int fid);

/** Send message to a registered connection.
 *
 * @param self Observer id of the calling observer.
 * @param fid The descriptor of the connection returned by SNetDispatcherAdd().
 * @param buffer Message to be sent.
 * @param buflen Length of the message.
 * @param interactive "true", if the observer should wait for reply message, "false" otherwise.
 *
 * @return Length of characters actually sent.
 * @return -1, if the operation failed.
 *
 */

int SNetDispatcherSend(int self, int fid, const char *buffer, int buflen, bool interactive);


/** This function starts the dispatcher system.
 *
 * @return 0, if the dispatcher was initiated correctly.
 *
 * @notice Any dispatcher functions must NOT be called before this function call!
 * @notice If return value was not 0, dispatcher should not be used!
 *
 */

int SNetDispatcherInit();

/** This function destroys the dispatcher system.
 *
 * @notice Any dispatcher functions must NOT be called after this function call!
 *
 */

void SNetDispatcherDestroy();
 
#endif /* SNET_DISPATCHER_H_ */
