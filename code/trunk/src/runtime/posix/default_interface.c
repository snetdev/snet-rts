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
 * Implementation of default S-Net language interface.
 *
 *******************************************************************************/

#include <stdlib.h>
#include <snetentities.h>
#include <memfun.h>
#include <default_interface.h>

/* Free char * */
static void SNet_default_interface_free( void *ptr) 
{
  if(ptr != NULL){
    SNetMemFree( ptr);
  }
}

/* Free copy char * */
static void *SNet_default_interface_copy( void *ptr)
{
  if(ptr == NULL)
    return NULL;

  char * c = (char *)SNetMemAlloc(sizeof(char) * strlen(ptr));
  return strcpy(c, ptr);
}

/* No need to serialize the data, as its already serialized! */
static int SNet_default_interface_serialize(void *ptr, char **serialized)
{
  if(*serialized != NULL){
    free(*serialized);
    *serialized = NULL;
  }

  if(ptr == NULL){
    *serialized = NULL;
    return 0;
  }

  *serialized =  SNet_default_interface_copy(ptr);
  return strlen(*serialized);
}

/* Data is not serialized in any way, but is passed on as string.  */
static void *SNet_default_interface_deserialize(char *ptr, int len)
{
  return SNet_default_interface_copy(ptr);
}
 
/* Pass all the functions to the RTE*/
void SNet_default_interface_init( int id)
{
  SNetGlobalRegisterInterface(id, &SNet_default_interface_free, 
			      &SNet_default_interface_copy, 
			      &SNet_default_interface_serialize, 
			      &SNet_default_interface_deserialize);  
}



