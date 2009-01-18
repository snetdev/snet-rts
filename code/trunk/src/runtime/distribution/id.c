#ifdef DISTRIBUTED_SNET

/** <!--********************************************************************-->
 * $Id$
 *
 * @file id.c
 *
 * @brief These function can be used to generate globally unique IDs.
 *
 *  Each ID is 64 bit unsigned integer and consist of two parts:
 *  - The node ID (The first 32 bits)
 *  - A locally unique ID (the last 32 bits)
 *
 *  These together form a globally unique ID.
 *
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 13.1.2009
 *
 *****************************************************************************/


#include <mpi.h>
#include <pthread.h>

#include "id.h"

/** <!--********************************************************************-->
 *
 * @name Fun
 *
 * <!--
 * void SNetIDServiceInit(int id) : Initializes the ID-Service.
 * void SNetIDServiceDestroy() : Frees resources reserved by the ID-service.
 * int SNetIDServiceGetNodeID() : Returns the ID of the node.
 * snet_id_t SNetIDServiceGetID() : Returns new globally unique ID.
 * -->
 *
 *****************************************************************************/
/*@{*/


/** @struct id_service_t;
 *
 *   @brief Type to hold data needed by ID-service.
 *
 */

typedef struct {
  unsigned int next_id;  /**< Next unused ID. */
  int my_id;             /**< ID of the node. */
  pthread_mutex_t mutex; /**< Mutex to guard access into 'next_id'. */
} id_service_t;

static id_service_t service;


/** <!--********************************************************************-->
 *
 * @fn  void SNetIDServiceInit(int id) 
 *
 *   @brief  Initializes the ID-service.
 *
 *
 *   @param id  ID of the node.
 *
 *
 ******************************************************************************/

void SNetIDServiceInit(int id) 
{
  service.next_id = 1;
  
  pthread_mutex_init(&service.mutex, NULL);

  service.my_id = id;
}


/** <!--********************************************************************-->
 *
 * @fn  void SNetIDServiceDestroy()
 *
 *   @brief  Frees resources allocated to the ID-service.
 *
 ******************************************************************************/

void SNetIDServiceDestroy() 
{
  pthread_mutex_destroy(&service.mutex);
}


/** <!--********************************************************************-->
 *
 * @fn  int SNetIDServiceGetNodeID()
 *
 *   @brief  Returns node's ID.
 *
 *   @return  ID of the node.
 *
 ******************************************************************************/

int SNetIDServiceGetNodeID()
{
  return service.my_id;
}


/** <!--********************************************************************-->
 *
 * @fn  snet_id_t SNetIDServiceGetID()
 *
 *   @brief  Returns a new globally unique ID.
 *
 *
 *   @return  The id, or SNET_ID_NO_ID in case of an error.
 *
 ******************************************************************************/
snet_id_t SNetIDServiceGetID()
{
  snet_id_t id = SNET_ID_NO_ID;

  pthread_mutex_lock(&service.mutex);
 
  if(service.next_id != SNET_ID_NO_ID) {
    id = SNET_ID_CREATE(service.my_id, service.next_id++);
  }

  pthread_mutex_unlock(&service.mutex);

  return id;
}
#endif /* DISTRIBUTED_SNET */
