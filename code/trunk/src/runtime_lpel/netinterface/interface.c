/*******************************************************************************
 *
 * $Id: interface.c 2521 2009-08-07 09:24:01Z jju $
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

#include <interface.h>
#include <string.h>
#include <memfun.h>

/* Allows only static interfaces.
 * -> No locking required!
 */

/* Struct to store the static and current temporary names */
struct interface{
  char **interfaces;                 /* static interfaces*/
  int number_of_interfaces;          /* number of static interfaces*/
};


snetin_interface_t *SNetInInterfaceInit(char **interfaces, int len){
  snetin_interface_t *temp;

  temp = SNetMemAlloc(sizeof(snetin_interface_t));

  temp->interfaces = interfaces;
  
  temp->number_of_interfaces = len;
  
  return temp;
}


void SNetInInterfaceDestroy(snetin_interface_t *interfaces){
  SNetMemFree(interfaces);
}


int SNetInInterfaceToId(snetin_interface_t *interfaces, const char *interface){
  int i = 0;

  if(interface == NULL || interfaces == NULL){
    return SNET_INTERFACE_ERROR;
  }
  
  /* search static interfaces */
  for(i = 0; i < interfaces->number_of_interfaces; i++) {
    if(strcmp(interfaces->interfaces[i], interface) == 0) {
      return i;
    }
  }

  return SNET_INTERFACE_ERROR;
}


char *SNetInIdToInterface(snetin_interface_t *interfaces, int i){
  char *t = NULL;

  if(interfaces == NULL || i < 0){
    return NULL;
  }

  /* search static interfaces */
  if(i < interfaces->number_of_interfaces){
    t = SNetMemAlloc(sizeof(char) * (strlen(interfaces->interfaces[i]) + 1));
    return  strcpy(t, interfaces->interfaces[i]);
  }

  return NULL;
}
