#ifdef DISTRIBUTED_SNET
#include <pthread.h>
#include <mpi.h>
#include <string.h>
#include <stdio.h>

#include "routing.h"
#include "message.h"
#include "memfun.h"
#include "bool.h"

/* TODO: Move MPI-related code out of this file
 * 
 */

#define MAX_LEVELS 32
#define FIRST_NODE -1

struct snet_routing_info{
  int id;
  int level;
  int *stack;
  int stack_size;
  int *nodes;
  int parent;
  int starter;
  snet_fun_id_t fun_id;
  int tag;
};

typedef struct {
  int next_id;
  pthread_mutex_t mutex;
  int self;
  int max_nodes;
  int next_index;
  snet_tl_stream_t *omngr;
  bool terminate;
  snet_tl_stream_t *global_in;
  snet_tl_stream_t *global_out;
  pthread_cond_t input_cond;
  pthread_cond_t output_cond;
} global_routing_info_t;
 
static global_routing_info_t *routing_info = NULL;


static void CreateNetwork(snet_routing_info_t *info, int node)
{
  snet_msg_create_network_t msg;
  MPI_Datatype type;

  msg.op_id = info->id;
  msg.parent = info->parent;
  msg.fun_id = info->fun_id.fun_id;
  msg.tag = info->tag;

  memset(msg.lib, 0, sizeof(char) * 32);
  strcpy(msg.lib, info->fun_id.lib);

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

  MPI_Send(&msg, 1, type, routing_info->self, SNET_msg_route_update, MPI_COMM_WORLD);
}

static void UpdateOTable(int id, int node, int index, snet_tl_stream_t *stream) 
{
  snet_msg_route_index_t msg;
  MPI_Datatype type;

  msg.op_id = id;
  msg.node = routing_info->self;
  msg.index = index;

  SNetTlWrite(routing_info->omngr, SNetRecCreate(REC_route_update, node, index, stream));

  type = SNetMessageGetMPIType(SNET_msg_route_index);

  MPI_Send(&msg, 1, type, node, SNET_msg_route_index, MPI_COMM_WORLD);
}


void SNetRoutingInit(snet_tl_stream_t *omngr)
{
  routing_info = SNetMemAlloc(sizeof(global_routing_info_t));

  routing_info->next_id = 0;
  routing_info->next_index = 0;

  pthread_mutex_init(&routing_info->mutex, NULL);

  MPI_Comm_rank( MPI_COMM_WORLD, &(routing_info->self));
  MPI_Comm_size( MPI_COMM_WORLD, &(routing_info->max_nodes));

  routing_info->omngr = omngr;

  routing_info->global_out = NULL;
  routing_info->global_in = NULL;

  routing_info->terminate = false;

  pthread_cond_init(&routing_info->input_cond, NULL);
  pthread_cond_init(&routing_info->output_cond, NULL);
}


void SNetRoutingDestroy()
{
  pthread_mutex_destroy(&routing_info->mutex);

  SNetMemFree(routing_info);
}


snet_tl_stream_t *SNetRoutingGetGlobalInput() 
{
  snet_tl_stream_t *stream;
  
  pthread_mutex_lock(&routing_info->mutex);

  stream = routing_info->global_in;

  pthread_mutex_unlock(&routing_info->mutex);

  return stream;
}

snet_tl_stream_t *SNetRoutingGetGlobalOutput() 
{
  snet_tl_stream_t *stream;

  pthread_mutex_lock(&routing_info->mutex);

  stream = routing_info->global_out;
  
  pthread_mutex_unlock(&routing_info->mutex);

  return stream;
}

snet_tl_stream_t *SNetRoutingWaitForGlobalInput() 
{
  snet_tl_stream_t *stream;
  
  pthread_mutex_lock(&routing_info->mutex);

  stream = routing_info->global_in;

  while(stream == NULL && routing_info->terminate == false) {
    pthread_cond_wait(&routing_info->input_cond, &routing_info->mutex);

    stream = routing_info->global_in;
  }

  pthread_mutex_unlock(&routing_info->mutex);

  return stream;
}

snet_tl_stream_t *SNetRoutingWaitForGlobalOutput() 
{
  snet_tl_stream_t *stream;

  pthread_mutex_lock(&routing_info->mutex);

  stream = routing_info->global_out;

  while(stream == NULL && routing_info->terminate == false) {
    pthread_cond_wait(&routing_info->output_cond, &routing_info->mutex);

    stream = routing_info->global_out;
  }

  pthread_mutex_unlock(&routing_info->mutex);

  return stream;
}

void SNetRoutingNotifyAll()
{
  pthread_mutex_lock(&routing_info->mutex);

  routing_info->terminate = true;

  pthread_cond_signal(&routing_info->input_cond);
  pthread_cond_signal(&routing_info->output_cond);

  pthread_mutex_unlock(&routing_info->mutex);
}


int SNetRoutingGetNewID()
{
  int ret;
  
  pthread_mutex_lock(&routing_info->mutex);
  
  ret = routing_info->next_id++;
  
  pthread_mutex_unlock(&routing_info->mutex);

  return ret;
}

snet_routing_info_t *SNetRoutingInfoInit(int id, int starter_node, int parent_node, snet_fun_id_t *fun_id, int tag) 
{
  snet_routing_info_t *info;

  info = SNetMemAlloc(sizeof(snet_routing_info_t));

  info->id = id;

  info->level = 0;

  info->stack_size = MAX_LEVELS;
  info->stack = SNetMemAlloc(sizeof(int) * info->stack_size);

  memset(info->stack, 0, sizeof(int) * info->stack_size);

  info->nodes = SNetMemAlloc(sizeof(int) * routing_info->max_nodes);
  
  memset(info->nodes, 0, sizeof(int) * routing_info->max_nodes);

  info->stack[info->level] = parent_node;

  info->starter = starter_node;

  info->parent = parent_node;

  strcpy(info->fun_id.lib, fun_id->lib);
  info->fun_id.fun_id = fun_id->fun_id;

  info->tag = tag;

  SNetRoutingInfoPushLevel(info);


  return info;
}

void SNetRoutingInfoDestroy(snet_routing_info_t *info) 
{
  SNetMemFree(info->stack);
  SNetMemFree(info->nodes);
  SNetMemFree(info);
}

snet_tl_stream_t *SNetRoutingInfoUpdate(snet_routing_info_t *info, int node, snet_tl_stream_t *stream)
{
  int index;
  if(info->stack[info->level] != node) {

    if(info->stack[info->level] == routing_info->self) {

      pthread_mutex_lock(&routing_info->mutex);

      index = routing_info->next_index++;

      pthread_mutex_unlock(&routing_info->mutex);
      
      UpdateOTable(info->id, node, index, stream);

      stream = SNetTlCreateUnboundedStream();

    } else if(node == routing_info->self) {

      if(info->stack[info->level] == FIRST_NODE) {

	pthread_mutex_lock(&routing_info->mutex);

	routing_info->global_in = stream;

	pthread_cond_signal(&routing_info->input_cond);

	pthread_mutex_unlock(&routing_info->mutex);

      } else {
	SNetTlSetFlag(stream, true);

	UpdateITable(info->id, info->stack[info->level], stream);
      }
    }
    
    info->stack[info->level] = node;
  }

  if(info->nodes[node] == 0 && node != routing_info->self && info->starter == routing_info->self) {
    info->nodes[node] = 1;
    
    CreateNetwork(info, node);
  }

  return stream;
}

void SNetRoutingInfoPushLevel(snet_routing_info_t *info)
{
  int *new;
  int size;

  info->level++;

  if(info->level == info->stack_size) {
    size = info->stack_size + MAX_LEVELS;

    new = SNetMemAlloc(sizeof(int) * size);

    memcpy(new, info->stack, info->stack_size);
  }

  info->stack[info->level] = info->stack[info->level - 1];
}

snet_tl_stream_t *SNetRoutingInfoPopLevel(snet_routing_info_t *info, snet_tl_stream_t *stream)
{
  int index;

  info->level--;

  if(info->stack[info->level] != info->stack[info->level + 1]) {
    if(info->stack[info->level] == routing_info->self) {

      SNetTlSetFlag(stream, true);

      UpdateITable(info->id, info->stack[info->level + 1], stream);

    } else if(info->stack[info->level + 1] == routing_info->self) {

      if(info->stack[info->level] == FIRST_NODE) {

	pthread_mutex_lock(&routing_info->mutex);

	routing_info->global_out = stream;

	pthread_cond_signal(&routing_info->output_cond);

	pthread_mutex_unlock(&routing_info->mutex);
      } else {

	pthread_mutex_lock(&routing_info->mutex);

	index = routing_info->next_index++;

	pthread_mutex_unlock(&routing_info->mutex);
	
	UpdateOTable(info->id, info->stack[info->level], index, stream);
      }
 
      stream = SNetTlCreateUnboundedStream();  
    }
  }

  return stream;
}

snet_tl_stream_t *SNetRoutingInfoFinalize(snet_routing_info_t *info, snet_tl_stream_t *stream)
{
  stream = SNetRoutingInfoPopLevel(info, stream);
  
  if(info->stack[info->level] == FIRST_NODE
     && info->stack[info->level + 1] == routing_info->self) {
    SNetTlMarkObsolete(stream);
    stream = NULL;  
  } else if(info->stack[info->level] == routing_info->self) {
    // Do nothing?

  } else {  
    SNetTlMarkObsolete(stream);
    stream = NULL;  
  }
  
  return stream;
}

#endif /* DISTRIBUTED_SNET */
