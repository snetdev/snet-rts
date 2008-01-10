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

#include "interface.h"
#include "str.h"
#include <memfun.h>

interface_t *initInterfaces(const char *const *names, 
			    char *(*const *serialize_fun)(const void *), 
			    void *(*const *deserialize_fun)(const char*),
			    int len){

  interface_t *temp = SNetMemAlloc(sizeof(interface_t));
  temp->names = names;
  temp->serialize_fun = serialize_fun;
  temp->deserialize_fun = deserialize_fun;
  temp->len = len;
  return temp;
}

void deleteInterfaces(interface_t *interfaces){
  SNetMemFree(interfaces);
}


int interfaceToId(const interface_t *interfaces, const char *interface){
  int id = INTERFACE_UNKNOWN;
  if(interfaces != NULL){
    for(id = 0; id < interfaces->len; id++){
      if(STRcmp(interfaces->names[id], interface) == 0){
	return id;
      }
    }
  }
  return INTERFACE_UNKNOWN;
}

const char *idToInterface(const interface_t *interfaces, int id){
  if(interfaces == NULL || id < 0 || id >= interfaces->len){
    return NULL;
  }
  return interfaces->names[id];
}


char *serialize(const interface_t *interfaces, int id, const void *value){
  if(interfaces == NULL || id < 0 || id >= interfaces->len){
    return NULL;
  }
  return interfaces->serialize_fun[id](value);
}

void *deserialize(const interface_t *interfaces, int id, const char *value){
  if(interfaces == NULL || id < 0 || id >= interfaces->len){
    return NULL;
  }
  return interfaces->deserialize_fun[id](value);
}
