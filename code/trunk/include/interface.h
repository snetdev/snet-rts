#ifndef INTERFACE_H_
#define INTERFACE_H_

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
 * Interface functions for S-NET network interface.
 *
 *******************************************************************************/

#define SNET_INTERFACE_ERROR -1

/* Struct to store the static and temporary names */
typedef struct interface snetin_interface_t;


/* Init new interfaces structure with given statistic names 
 *
 * @param static_interfaces Static interfaces to be used.
 * @param len Number of static interfaces.
 *
 * @return Initiated interface structure.
 *
 */

extern snetin_interface_t *SNetInInterfaceInit(char **static_interfaces, int len);

/* Free memory used in interface structure.
 *
 * @param Interface structure to be deleted.
 *
 */

extern void SNetInInterfaceDestroy(snetin_interface_t *interfaces);

/* Search for index by given interface from the structure.
 * If the given interface is not found, a new interface with 
 * the next possible index is added to the structure.
 *
 * @param interfaces Interface structure to be searched.
 * @param interface The name to search for.
 *
 * @return Index of the given interface.
 * @return SNET_INTERFACE_ERROR, if the interface parameter given was NULL.
 *
 */

extern int SNetInInterfaceToId(snetin_interface_t *interfaces, const char *interface);

/* Search for interface by given index from the structure.
 *
 * @param interfaces Interface structure to be searched.
 * @param index Index to search for.
 *
 * @return Interface corresponding to the given index.
 * @return or NULL if no interface with the given index was found.
 *
 */


extern char *SNetInIdToInterface(snetin_interface_t *interfaces, int index);

#endif /* INTERFACE_H_ */

