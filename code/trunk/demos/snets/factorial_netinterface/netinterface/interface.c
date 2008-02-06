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

#include <interface.h>
#include <string.h>
#include <memfun.h>

/* Struct to store the interface names */
struct interface{
  char **names;
  int len;
};


snetin_interface_t *SNetInInterfaceInit(char **names, 
					int len){

  snetin_interface_t *temp = SNetMemAlloc(sizeof(snetin_interface_t));
  temp->names = names;
  temp->len = len;
  return temp;
}

void SNetInInterfaceDestroy(snetin_interface_t *interfaces){
  SNetMemFree(interfaces);
}


int SNetInInterfaceToId(const snetin_interface_t *interfaces, const char *interface){
  int id = INTERFACE_UNKNOWN;
  if(interfaces != NULL){
    for(id = 0; id < interfaces->len; id++){
      if(strcmp(interfaces->names[id], interface) == 0){
	return id;
      }
    }
  }
  return INTERFACE_UNKNOWN;
}

const char *SNetInIdToInterface(const snetin_interface_t *interfaces, int id){
  if(interfaces == NULL || id < 0 || id >= interfaces->len){
    return NULL;
  }
  return interfaces->names[id];
}
