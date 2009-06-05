#ifdef DISTRIBUTED_SNET
#include <pthread.h>
#include <mpi.h>
#include <string.h>
#include <stdio.h>

#include "omanager.h"
#include "routing.h"
#include "message.h"
#include "memfun.h"
#include "bool.h"

/* TODO: Move MPI-related code out of this file
 * 
 */

#define COUNTER_START SNET_MESSAGE_NUM_CONTROL_MESSAGES

typedef struct {
  int next_id;
  int next_index;
  pthread_mutex_t mutex;
  int self;
  int max_nodes;
  bool terminate;
  snet_tl_stream_t *global_in;
  snet_tl_stream_t *global_out;
  pthread_cond_t input_cond;
  pthread_cond_t output_cond;
} global_routing_info_t;

struct snet_routing_context{
  int id;
  int current_location;
  bool *nodes;
  int parent;
  bool is_master;
  snet_fun_id_t fun_id;
  int tag;
};
 
static global_routing_info_t *r_info = NULL;


/****************************** Global routing info ******************************/

void SNetRoutingInit()
{
  r_info = SNetMemAlloc(sizeof(global_routing_info_t));

  r_info->next_id = 0;
  r_info->next_index = COUNTER_START;

  pthread_mutex_init(&r_info->mutex, NULL);

  MPI_Comm_rank( MPI_COMM_WORLD, &(r_info->self));
  MPI_Comm_size( MPI_COMM_WORLD, &(r_info->max_nodes));

  r_info->global_out = NULL;
  r_info->global_in = NULL;

  r_info->terminate = false;

  pthread_cond_init(&r_info->input_cond, NULL);
  pthread_cond_init(&r_info->output_cond, NULL);
}


void SNetRoutingDestroy()
{
  pthread_mutex_destroy(&r_info->mutex);

  SNetMemFree(r_info);
}

static int SNetRoutingGetNewIndex()
{
  int ret;
  
  pthread_mutex_lock(&r_info->mutex);
  
  ret = r_info->next_index++;
  
  pthread_mutex_unlock(&r_info->mutex);

  return ret;
}

static int SNetRoutingGetSelf() 
{
  return r_info->self;
}

static int SNetRoutingGetNumNodes() 
{
  return r_info->max_nodes;
}

static void SNetRoutingSetGlobalInput(snet_tl_stream_t *stream) 
{
  pthread_mutex_lock(&r_info->mutex);

  r_info->global_in = stream;

  pthread_cond_signal(&r_info->input_cond);

  pthread_mutex_unlock(&r_info->mutex);
}

static void SNetRoutingSetGlobalOutput(snet_tl_stream_t *stream) 
{
  pthread_mutex_lock(&r_info->mutex);

  r_info->global_out = stream;
  
  pthread_cond_signal(&r_info->output_cond);

  pthread_mutex_unlock(&r_info->mutex);
}

snet_tl_stream_t *SNetRoutingGetGlobalInput() 
{
  snet_tl_stream_t *stream;
  
  pthread_mutex_lock(&r_info->mutex);

  stream = r_info->global_in;

  pthread_mutex_unlock(&r_info->mutex);

  return stream;
}

snet_tl_stream_t *SNetRoutingGetGlobalOutput() 
{
  snet_tl_stream_t *stream;

  pthread_mutex_lock(&r_info->mutex);

  stream = r_info->global_out;
  
  pthread_mutex_unlock(&r_info->mutex);

  return stream;
}

snet_tl_stream_t *SNetRoutingWaitForGlobalInput() 
{
  snet_tl_stream_t *stream;
  
  pthread_mutex_lock(&r_info->mutex);

  stream = r_info->global_in;

  while(stream == NULL && r_info->terminate == false) {
    pthread_cond_wait(&r_info->input_cond, &r_info->mutex);

    stream = r_info->global_in;
  }

  pthread_mutex_unlock(&r_info->mutex);

  return stream;
}

snet_tl_stream_t *SNetRoutingWaitForGlobalOutput() 
{
  snet_tl_stream_t *stream;

  pthread_mutex_lock(&r_info->mutex);

  stream = r_info->global_out;

  while(stream == NULL && r_info->terminate == false) {
    pthread_cond_wait(&r_info->output_cond, &r_info->mutex);

    stream = r_info->global_out;
  }

  pthread_mutex_unlock(&r_info->mutex);

  return stream;
}

void SNetRoutingNotifyAll()
{
  pthread_mutex_lock(&r_info->mutex);

  r_info->terminate = true;

  pthread_cond_signal(&r_info->input_cond);
  pthread_cond_signal(&r_info->output_cond);

  pthread_mutex_unlock(&r_info->mutex);
}


int SNetRoutingGetNewID()
{
  int ret;
  
  pthread_mutex_lock(&r_info->mutex);
  
  ret = r_info->next_id++;
  
  pthread_mutex_unlock(&r_info->mutex);

  return ret;
}


/****************************** Message functions ******************************/

static void CreateNetwork(snet_routing_context_t *info, int node)
{
  snet_msg_create_network_t msg;
  MPI_Datatype type;
  const snet_fun_id_t *fun_id;

  msg.op_id = SNetRoutingContextGetID(info);
  msg.parent = SNetRoutingContextGetParent(info);
  msg.tag = SNetRoutingContextGetTag(info);

  fun_id = SNetRoutingContextGetFunID(info);
  msg.fun_id.id = fun_id->id;

  memset(msg.fun_id.lib, 0, sizeof(char) * 32);
  strcpy(msg.fun_id.lib, fun_id->lib);

  type = SNetMessageGetMPIType(SNET_msg_create_network);

  MPI_Send(&msg, 1, type, node, SNET_msg_create_network, MPI_COMM_WORLD);
}

static void UpdateITable(int id, int node, snet_tl_stream_t *stream) 
{
  snet_msg_route_update_t msg;
  MPI_Datatype type;

  msg.op_id = id;
  msg.node = node;
  msg.stream = stream;

  type = SNetMessageGetMPIType(SNET_msg_route_update);

  MPI_Send(&msg, 1, type, SNetRoutingGetSelf(), SNET_msg_route_update, MPI_COMM_WORLD);

}

static void UpdateOTable(int id, int node, int index, snet_tl_stream_t *stream) 
{
  snet_msg_route_index_t msg;
  MPI_Datatype type;

  msg.op_id = id;
  msg.node = SNetRoutingGetSelf();
  msg.index = index;

  OManagerUpdateRoutingTable(stream, node, index);

  type = SNetMessageGetMPIType(SNET_msg_route_index);

  MPI_Send(&msg, 1, type, node, SNET_msg_route_index, MPI_COMM_WORLD);
}


/****************************** Routing context ******************************/

snet_routing_context_t *SNetRoutingContextInit(int id, bool is_master, int parent, snet_fun_id_t *fun_id, int tag)
{
  snet_routing_context_t *new;

  new = SNetMemAlloc(sizeof(snet_routing_context_t));

  new->id = id;

  new->nodes = SNetMemAlloc(sizeof(bool) * SNetRoutingGetNumNodes());
  
  memset(new->nodes, false, sizeof(bool) * SNetRoutingGetNumNodes());

  new->current_location = parent;

  new->is_master = is_master;

  new->parent = parent;

  strcpy(new->fun_id.lib, fun_id->lib);

  new->fun_id.id = fun_id->id;

  new->tag = tag;

  return new;
}

void SNetRoutingContextDestroy(snet_routing_context_t *context)
{
  SNetMemFree(context->nodes);
  SNetMemFree(context);
}

int SNetRoutingContextGetID(snet_routing_context_t *context)
{
  return context->id;
}

void SNetRoutingContextSetLocation(snet_routing_context_t *context, int location)
{
  context->current_location = location;
}

int SNetRoutingContextGetLocation(snet_routing_context_t *context)
{
  return context->current_location;
}

void SNetRoutingContextSetParent(snet_routing_context_t *context, int parent)
{
  context->parent = parent;
}

int SNetRoutingContextGetParent(snet_routing_context_t *context)
{
  return context->parent;
}

bool SNetRoutingContextIsMaster(snet_routing_context_t *context)
{
  return context->is_master;
}

const snet_fun_id_t *SNetRoutingContextGetFunID(snet_routing_context_t *context)
{
  return &context->fun_id;
}

int SNetRoutingContextGetTag(snet_routing_context_t *context)
{
  return context->tag;
}

void SNetRoutingContextSetNodeVisited(snet_routing_context_t *context, int node)
{
  context->nodes[node] = true;
}

bool SNetRoutingContextIsNodeVisited(snet_routing_context_t *context, int node)
{
  return context->nodes[node];
}

snet_tl_stream_t *SNetRoutingContextUpdate(snet_routing_context_t *context, snet_tl_stream_t* stream, int location)
{
  int index;

  int previous = SNetRoutingContextGetLocation(context);

  if(previous != location) {

    if(previous == SNetRoutingGetSelf()) {
  
      index = SNetRoutingGetNewIndex();
     
      UpdateOTable(SNetRoutingContextGetID(context), location, index, stream);

      stream = SNetTlCreateStream(BUFFER_SIZE);

    } else if(location == SNetRoutingGetSelf()) {

      if(previous == SNET_LOCATION_NONE) {

	SNetRoutingSetGlobalInput(stream);

      } else {

	SNetTlSetFlag(stream, true);

	UpdateITable(SNetRoutingContextGetID(context), previous, stream);
      }
    } 


    SNetRoutingContextSetLocation(context, location);
  }

  if(SNetRoutingContextIsNodeVisited(context, location) == false 
     && location !=  SNetRoutingGetSelf()
     && SNetRoutingContextIsMaster(context)) {

    SNetRoutingContextSetNodeVisited(context, location);
    
    CreateNetwork(context, location);
  }

  return stream;
}

snet_tl_stream_t *SNetRoutingContextEnd(snet_routing_context_t *context, snet_tl_stream_t* stream)
{
  int previous = SNetRoutingContextGetLocation(context);
  int location = SNetRoutingContextGetParent(context);
  int index;

  if(location != previous) {
    if(location == SNetRoutingGetSelf()) {

      SNetTlSetFlag(stream, true);

      UpdateITable(SNetRoutingContextGetID(context), previous, stream);

    } else if(previous == SNetRoutingGetSelf()) {

      if(location == SNET_LOCATION_NONE) {

	SNetRoutingSetGlobalOutput(stream);

	stream = NULL;
      } else {
	index = SNetRoutingGetNewIndex();

	UpdateOTable(SNetRoutingContextGetID(context), location, index, stream);

	stream = SNetTlCreateStream(BUFFER_SIZE);
      }
    } 
  }

  SNetRoutingContextSetLocation(context, location);


  return stream;
}

#endif /* DISTRIBUTED_SNET */
