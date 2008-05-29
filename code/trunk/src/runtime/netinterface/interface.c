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

#include <interface.h>
#include <string.h>
#include <pthread.h>
#include <memfun.h>

/* TODO: 
 * The reference counting system might not work correctly if the
 * SNet runtime can copy unknown data fields. Check how this work
 * and adjust reference counting accordingly.
 */

/* Struct to store temporary interfaces and their number mappings*/
typedef struct temp_interface{
  char *interface;
  int index;
  int ref_count;
  struct temp_interface *next;
} temp_interface_t;

/* Struct to store the static and current temporary names */
struct interface{
  char **interfaces;                 /* static interfaces*/
  int number_of_interfaces;          /* number of static interfaces*/
  pthread_mutex_t mutex;              /* mutex for access control to temporary interfaces */
  temp_interface_t *temp_interfaces; /* temporary interfaces */
};


snetin_interface_t *SNetInInterfaceInit(char **interfaces, int len){
  snetin_interface_t *temp = SNetMemAlloc(sizeof(snetin_interface_t));
  temp->interfaces = interfaces;
  
  temp->number_of_interfaces = len;
  pthread_mutex_init(&temp->mutex, NULL);
  temp->temp_interfaces = NULL;
  
  return temp;
}


void SNetInInterfaceDestroy(snetin_interface_t *interfaces){
  pthread_mutex_destroy(&interfaces->mutex);

  /* remove are temporary interfaces */
  while(interfaces->temp_interfaces != NULL){
    temp_interface_t *temp = interfaces->temp_interfaces->next;
    SNetMemFree(interfaces->temp_interfaces);
    interfaces->temp_interfaces = temp;
  }
  SNetMemFree(interfaces);
}


int SNetInInterfaceToId(snetin_interface_t *interfaces, const char *interface){
  int i = 0;
  int index = SNET_INTERFACE_ERROR;
  if(interface == NULL || interfaces == NULL){
    return index;
  }
  
  /* search static interfaces */
  for(i = 0; i < interfaces->number_of_interfaces; i++) {
    if(strcmp(interfaces->interfaces[i], interface) == 0) {
      return i;
    }
  }
  
  /* search temporary interfaces */
  pthread_mutex_lock(&interfaces->mutex);

  temp_interface_t *tempinterface = interfaces->temp_interfaces;
  while(tempinterface != NULL){
    if(strcmp(tempinterface->interface, interface) == 0) {
      //tempinterface->ref_count++;
      index = tempinterface->index;
      
      pthread_mutex_unlock(&interfaces->mutex);
      return index;
    }
    tempinterface = tempinterface->next;
  }
  
  /* Some kind of default interface? */

  /* Unknown interface, create new one */
  /* 
 temp_interface_t *new_interface = SNetMemAlloc(sizeof(temp_interface_t)); 
  char *t = SNetMemAlloc(sizeof(char) * (strlen(interface) + 1));
  new_interface->interface = strcpy(t, interface);

  if(interfaces->temp_interfaces == NULL) {
    new_interface->index = interfaces->number_of_interfaces;
  }
  else{
    new_interface->index = interfaces->temp_interfaces->index + 1;
  }
  
  new_interface->next = interfaces->temp_interfaces;
  interfaces->temp_interfaces = new_interface;

  //new_interface->ref_count = 1;

  index = new_interface->index;
  
  SNet_default_interface_init(index);
*/
  index = -1;

  pthread_mutex_unlock(&interfaces->mutex);
  return index;
}


char *SNetInIdToInterface(snetin_interface_t *interfaces, int i){
  if(interfaces == NULL || i < 0){
    return NULL;
  }

  /* search static interfaces */
  if(i < interfaces->number_of_interfaces){
    char *t = SNetMemAlloc(sizeof(char) * (strlen(interfaces->interfaces[i]) + 1));
    return  strcpy(t, interfaces->interfaces[i]);
  }
  
  /*search temporary interfaces */
  pthread_mutex_lock(&interfaces->mutex);

  temp_interface_t *tempinterface = interfaces->temp_interfaces;
  temp_interface_t *last = NULL;
  char *t = NULL;
  while(tempinterface != NULL){
    if(tempinterface->index == i){
      t = SNetMemAlloc(sizeof(char) * (strlen(tempinterface->interface) + 1));
      t = strcpy(t, tempinterface->interface);
      //remove interface if the reference count goes to zero
      /*if(--tempinterface->ref_count == 0){
	if(tempinterface == interfaces->temp_interfaces){
	  interfaces->temp_interfaces = tempinterface->next;
	}
	else{
	  last->next = tempinterface->next;
	}
	SNetMemFree(tempinterface->interface);
	SNetMemFree(tempinterface);
	tempinterface = NULL;
	}
      */
      break;
    }
    last = tempinterface;
    tempinterface = tempinterface->next;
  }

  pthread_mutex_unlock(&interfaces->mutex);
  return t;
}
