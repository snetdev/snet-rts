#ifdef DISTRIBUTED_SNET
/** <!--********************************************************************-->
 * $Id: routing.c 2864 2010-09-17 11:28:30Z dlp $
 *
 * @file routing.c
 *
 * @brief Functions used to construct the network in distributed S-Net.
 *
 *
 * @author Jukka Julku, VTT Technical Research Centre of Finland
 *
 * @date 13.1.2009
 *
 *****************************************************************************/

#include <pthread.h>
#include <mpi.h>
#include <string.h>
#include <stdio.h>

#include "omanager.h"
#include "routing.h"
#include "message.h"
#include "memfun.h"
#include "bool.h"

#include "debug.h"

/* TODO: Move MPI-related code out of this file
 * 
 */

#define COUNTER_START SNET_MESSAGE_NUM_CONTROL_MESSAGES

/** @struct global_routing_info_
 *
 *   @brief Global routing data.
 *
 */
typedef struct {
  int next_id;                   /**< Next unused task ID. */
  int next_index;                /**< Next unused stream index. */
  pthread_mutex_t mutex;         /**< Mutex to guard this structure. */
  int self;                      /**< Node's own rank. */
  int max_nodes;                 /**< Number of nodes. */
  bool terminate;                /**< True if SNetDistributionStop has been called. */
  stream_t *global_in;   /**< Global input stream if in this node. */
  stream_t *global_out;  /**< Global output stream if in this node. */
  pthread_cond_t input_cond;     /**< Signalled if global input is in this node. */
  pthread_cond_t output_cond;    /**< Signalled if global output is in this node. */
} global_routing_info_t;

/** @struct snet_routing_context
 *
 *   @brief Information about the network creation task.
 *
 */
struct snet_routing_context{
  snet_id_t id;          /**< Id of the creation task. */
  int current_location;  /**< Current location. */
  bool *nodes;           /**< List of visited nodes. */
  int parent;            /**< Parent node of the creation task. */
  bool is_master;        /**< Is this node the master node?. */
  snet_fun_id_t fun_id;  /**< Function id for the start-point. */
  int tag;               /**< Tag in case the parent is a location split node. */
};
 
/* Global routing data. */
static global_routing_info_t *r_info = NULL;

static snet_id_t RoutingContextGetID(snet_routing_context_t *context);
static bool RoutingContextIsMaster(snet_routing_context_t *context);
static snet_fun_id_t *RoutingContextGetFunID(snet_routing_context_t *context);
static int RoutingContextGetTag(snet_routing_context_t *context);


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

static int RoutingGetNewIndex()
{
  int ret;
  
  pthread_mutex_lock(&r_info->mutex);
  
  ret = r_info->next_index++;
  
  pthread_mutex_unlock(&r_info->mutex);

  return ret;
}

static int RoutingGetSelf() 
{
  return r_info->self;
}

static int RoutingGetNumNodes() 
{
  return r_info->max_nodes;
}

static void RoutingSetGlobalInput(stream_t *stream) 
{
  pthread_mutex_lock(&r_info->mutex);

  r_info->global_in = stream;

  pthread_cond_signal(&r_info->input_cond);

  pthread_mutex_unlock(&r_info->mutex);
}

static void RoutingSetGlobalOutput(stream_t *stream) 
{
  pthread_mutex_lock(&r_info->mutex);

  r_info->global_out = stream;
  
  pthread_cond_signal(&r_info->output_cond);

  pthread_mutex_unlock(&r_info->mutex);
}

stream_t *SNetRoutingGetGlobalInput() 
{
  stream_t *stream;
  
  pthread_mutex_lock(&r_info->mutex);

  stream = r_info->global_in;

  pthread_mutex_unlock(&r_info->mutex);

  return stream;
}

stream_t *SNetRoutingGetGlobalOutput() 
{
  stream_t *stream;

  pthread_mutex_lock(&r_info->mutex);

  stream = r_info->global_out;
  
  pthread_mutex_unlock(&r_info->mutex);

  return stream;
}

stream_t *SNetRoutingWaitForGlobalInput() 
{
  stream_t *stream;
  
  pthread_mutex_lock(&r_info->mutex);

  stream = r_info->global_in;

  while(stream == NULL && r_info->terminate == false) {
    pthread_cond_wait(&r_info->input_cond, &r_info->mutex);

    stream = r_info->global_in;
  }

  pthread_mutex_unlock(&r_info->mutex);

  return stream;
}

stream_t *SNetRoutingWaitForGlobalOutput() 
{
  stream_t *stream;

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


snet_id_t SNetRoutingGetNewID()
{
  int ret;
  
  pthread_mutex_lock(&r_info->mutex);
  
  ret = r_info->next_id++;
  
  pthread_mutex_unlock(&r_info->mutex);

  return SNET_ID_CREATE(RoutingGetSelf(), ret);
}


/****************************** Message functions ******************************/

static void CreateNetwork(snet_routing_context_t *info, int node)
{
  snet_msg_create_network_t msg;
  MPI_Datatype type;
  const snet_fun_id_t *fun_id;

  msg.op_id = RoutingContextGetID(info);
  msg.parent = SNetRoutingContextGetParent(info);
  msg.tag = RoutingContextGetTag(info);

  fun_id = RoutingContextGetFunID(info);
  msg.fun_id.id = fun_id->id;

  memset(msg.fun_id.lib, 0, sizeof(char) * 32);
  strcpy(msg.fun_id.lib, fun_id->lib);

  type = SNetMessageGetMPIType(SNET_msg_create_network);

  MPI_Send(&msg, 1, type, node, SNET_msg_create_network, MPI_COMM_WORLD);
}

static void UpdateITable(snet_id_t id, int node, stream_t *stream) 
{
  snet_msg_route_update_t msg;
  MPI_Datatype type;

  msg.op_id = id;
  msg.node = node;
  msg.stream = stream;

  type = SNetMessageGetMPIType(SNET_msg_route_update);

#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("%lld: Update Input Manager: %d->%d", id, node, RoutingGetSelf());
#endif /* DISTRIBUTED_DEBUG */

  MPI_Send(&msg, 1, type, RoutingGetSelf(), SNET_msg_route_update, MPI_COMM_WORLD);

}

static void UpdateOTable(snet_id_t id, int node, int index, stream_t *stream) 
{
  snet_msg_route_index_t msg;
  MPI_Datatype type;

  msg.op_id = id;
  msg.node = RoutingGetSelf();
  msg.index = index;

  SNetOManagerUpdateRoutingTable(stream, node, index);

  type = SNetMessageGetMPIType(SNET_msg_route_index);

#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("%lld: Update Output Manager: %d:%d->%d", id, RoutingGetSelf(), index, node);
#endif /* DISTRIBUTED_DEBUG */

  MPI_Send(&msg, 1, type, node, SNET_msg_route_index, MPI_COMM_WORLD);
}


/****************************** Routing context ******************************/

snet_routing_context_t *SNetRoutingContextInit(snet_id_t id, bool is_master, int parent, snet_fun_id_t *fun_id, int tag)
{
  snet_routing_context_t *new;

  new = SNetMemAlloc(sizeof(snet_routing_context_t));

  new->id = id;

  new->nodes = SNetMemAlloc(sizeof(bool) * RoutingGetNumNodes());
  
  memset(new->nodes, false, sizeof(bool) * RoutingGetNumNodes());

  new->current_location = parent;

  new->is_master = is_master;

  new->parent = parent;

  strcpy(new->fun_id.lib, fun_id->lib);

  new->fun_id.id = fun_id->id;

  new->tag = tag;

  return new;
}

snet_routing_context_t * SNetRoutingContextCopy(snet_routing_context_t *original)
{
  snet_routing_context_t *new;

  new = SNetMemAlloc(sizeof(snet_routing_context_t));

  new->id = original->id;

  new->nodes = SNetMemAlloc(sizeof(bool) * RoutingGetNumNodes());
  
  memcpy(new->nodes, original->nodes, sizeof(bool) * RoutingGetNumNodes());

  new->current_location = original->parent;

  new->is_master = original->is_master;

  new->parent = original->parent;

  strcpy(new->fun_id.lib, original->fun_id.lib);

  new->fun_id.id = original->fun_id.id;

  new->tag = original->tag;

  return new;
}

void SNetRoutingContextDestroy(snet_routing_context_t *context)
{
  SNetMemFree(context->nodes);
  SNetMemFree(context);
}

static snet_id_t RoutingContextGetID(snet_routing_context_t *context)
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

static bool RoutingContextIsMaster(snet_routing_context_t *context)
{
  return context->is_master;
}

static snet_fun_id_t *RoutingContextGetFunID(snet_routing_context_t *context)
{
  return &context->fun_id;
}

static int RoutingContextGetTag(snet_routing_context_t *context)
{
  return context->tag;
}

static void RoutingContextSetNodeVisited(snet_routing_context_t *context, int node)
{
  context->nodes[node] = true;
}

static bool RoutingContextIsNodeVisited(snet_routing_context_t *context, int node)
{
  return context->nodes[node];
}

/** <!--********************************************************************-->
 *
 * @fn  stream_t *SNetRoutingContextUpdate(snet_routing_context_t *context, stream_t* stream, int location)
 *
 *   @brief  Update routing context when a new component is to be built
 *
 *           Checks for transitions between the locations and adjust the routing context
 *           accordingly. Creates connections between nodes.
 *
 *   @param context  Routing context
 *   @param stream   The input stream
 *   @param index    Location where the component is built
 *
 *   @return The input stream of to be given to the component.
 *
 ******************************************************************************/

stream_t *SNetRoutingContextUpdate(snet_routing_context_t *context, stream_t* stream, int location)
{
  int index;

  int previous = SNetRoutingContextGetLocation(context);

  if(previous != location) {

    if(previous == RoutingGetSelf()) {
  
      index = RoutingGetNewIndex();
     
      UpdateOTable(RoutingContextGetID(context), location, index, stream);

      stream = StreamCreate();

    } else if(location == RoutingGetSelf()) {

      if(previous == SNET_LOCATION_NONE) {

	RoutingSetGlobalInput(stream);

      } else {

	//SNetTlSetFlag(stream, true);

	UpdateITable(RoutingContextGetID(context), previous, stream);
      }
    } 


    SNetRoutingContextSetLocation(context, location);
  }

  if(RoutingContextIsNodeVisited(context, location) == false 
     && location !=  RoutingGetSelf()
     && RoutingContextIsMaster(context)) {

    RoutingContextSetNodeVisited(context, location);
    
#ifdef DISTRIBUTED_DEBUG
    SNetUtilDebugNotice("%ld: Call Create Network (%d)", context->id, location);
#endif /* DISTRIBUTED_DEBUG */

    CreateNetwork(context, location);
  }

  return stream;
}

/** <!--********************************************************************-->
 *
 * @fn  stream_t *SNetRoutingContextEnd(snet_routing_context_t *context, stream_t* stream)
 *
 *   @brief  Update routing context at the end of the network creation.
 *
 *           Checks for transitions between the locations and adjust the routing context
 *           accordingly. Creates connections between nodes.
 *
 *   @param context  Routing context
 *   @param stream   The input stream
 *   @param index    Location where the component is built
 *
 *   @return The input stream of to be given to the component or NULL if no such a stream
 *
 ******************************************************************************/

stream_t *SNetRoutingContextEnd(snet_routing_context_t *context, stream_t* stream)
{
  int previous = SNetRoutingContextGetLocation(context);
  int location = SNetRoutingContextGetParent(context);
  int index;

  if(location != previous) {
    if(location == RoutingGetSelf()) {

      //SNetTlSetFlag(stream, true);

      UpdateITable(RoutingContextGetID(context), previous, stream);

    } else if(previous == RoutingGetSelf()) {

      if(location == SNET_LOCATION_NONE) {

	RoutingSetGlobalOutput(stream);

	stream = NULL;
      } else {
	index = RoutingGetNewIndex();

	UpdateOTable(RoutingContextGetID(context), location, index, stream);

	stream = StreamCreate();
      }
    } 
  }

  SNetRoutingContextSetLocation(context, location);


  return stream;
}

#endif /* DISTRIBUTED_SNET */
