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
 * Language interface functions for S-NET network interface.
 *
 *******************************************************************************/

#define INTERFACE_UNKNOWN -1

/* Struct to store the interface names */
typedef struct interface snetin_interface_t;


/* Init interface structure. 
 *
 * @param names Textual names of the interfaces.
 * @param len Length of the parameter array.
 *
 * @return Initiated interface structure.
 *
 */

snetin_interface_t *SNetInInterfaceInit(char **names, 
					int len);

/* Free memory allocated to interface structure. 
 *
 * @param interfaces Structure to be freed.
 *
 * @notice This function does NOT free memory allocated for
 *         the interface names! 
 *
 */

void SNetInInterfaceDestroy(snetin_interface_t *interfaces);


/* Maps textual language interface names to interface IDs.
 *
 * @param interfaces Interfaces to search for the name.
 * @param interface Textual name of the wanted ID.
 *
 * @return Integer id corresponding to the given name.
 * @return INTERFACE_UNKNOWN, if the given name didn't match any interfaces.
 *
 */

int SNetInInterfaceToId(const snetin_interface_t *interfaces, const char *interface);

/* Maps language interface IDs to textual interface names.
 *
 * @param interfaces Interfaces to search for the name.
 * @param id ID corresponding to the wanted name.
 *
 * @return textual name of the interface matching the ID.
 * @return NULL, if the given ID didn't match any interfaces.
 *
 */

const char *SNetInIdToInterface(const snetin_interface_t *interfaces, int id);

#endif /* INTERFACE_H_ */
