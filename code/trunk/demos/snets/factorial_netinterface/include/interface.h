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

/* Struct to store the interface names and (de)serialization functions */
typedef struct interface interface_t;


/* Init interface structure. 
 *
 * @param names Textual names of the interfaces.
 * @param serialize_fun Array of serialization functions. One per interface.
 * @param deserialize_fun Array of deserialization functions. One per interface.
 * @param len Length of the parameter arrays.
 *
 * @return Initiated interface structure.
 *
 * @notice All the parameter arrays have to be of equal length and the
 *         interface name and the functions must have the same index in the
 *         arrays.
 */

interface_t *initInterfaces(const char *const *names, 
			    char *(*const* serialize_fun)(const void *), 
			    void *(*const* deserialize_fun)(const char*),
			    int len);

/* Free memory allocated to interface structure. 
 *
 * @param interfaces Structure to be freed.
 *
 * @notice This function does NOT free memory allocated for
 *         the interface names! 
 *
 */

void deleteInterfaces(interface_t *interfaces);


/* Maps textual language interface names to interface IDs.
 *
 * @param interfaces Interfaces to search for the name.
 * @param interface Textual name of the wanted ID.
 *
 * @return Integer id corresponding to the given name.
 * @return INTERFACE_UNKNOWN, if the given name didn't match any interfaces.
 *
 */

int interfaceToId(const interface_t *interfaces, const char *interface);

/* Maps language interface IDs to textual interface names.
 *
 * @param interfaces Interfaces to search for the name.
 * @param id ID corresponding to the wanted name.
 *
 * @return textual name of the interface matching the ID.
 * @return NULL, if the given ID didn't match any interfaces.
 *
 */

const char *idToInterface(const interface_t *interfaces, int id);

/* Serializes given data with serialization function corresponding to the
 * interface id.
 *
 * @param interfaces Interfaces to use
 * @param id ID of the interface the data uses
 * @param value Data to be serialized
 *
 * @return Serialized data.
 * @return NULL, if no serialization function was found. 
 *
 */

char *serialize(const interface_t *interfaces, int id, const void *value);

/* Serializes given data with serialization function corresponding to the
 * interface id.
 *
 * @param interfaces Interfaces to use
 * @param id ID of the interface the data uses
 * @param value Data to be deserialized
 *
 * @return deserialized data. Depends on language deserialization function.
 * @return NULL, if no deserialization function was found. 
 *
 */

void *deserialize(const interface_t *interfaces, int id, const char *value);

#endif /* INTERFACE_H_ */
