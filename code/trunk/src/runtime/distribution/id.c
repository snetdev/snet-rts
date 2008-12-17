#ifdef DISTRIBUTED_SNET
#include <mpi.h>
#include <pthread.h>

#include "id.h"

typedef struct {
  unsigned int next_id;
  int my_id;
  pthread_mutex_t mutex;
} id_service_t;

static id_service_t service;


void SNetIDServiceInit(int id) {
  service.next_id = 1;
  
  pthread_mutex_init(&service.mutex, NULL);

  service.my_id = id;
}

void SNetIDServiceDestroy() {
  pthread_mutex_destroy(&service.mutex);
}


int SNetIDServiceGetNodeID()
{
  return service.my_id;
}

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
